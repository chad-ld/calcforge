#include "WindowManager.h"
#include "EventBus.h"
#include "Logger.h"

#include <QMainWindow>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QScreen>
#include <QStyle>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

WindowManager::WindowManager(QMainWindow* mainWindow, QSettings* settings, EventBus* eventBus, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_settings(settings)
    , m_eventBus(eventBus)
    , m_bottomLeftCorner(nullptr)
    , m_bottomRightCorner(nullptr)
    , m_leftEdgeIndicator(nullptr)
    , m_rightEdgeIndicator(nullptr)
    , m_bottomEdgeIndicator(nullptr)
    , m_dragging(false)
    , m_resizing(false)
    , m_resizeEdges(Qt::Edges())
    , m_stayOnTop(false)
{
    if (!m_mainWindow || !m_settings) {
        LOG_DEBUG("WindowManager: Invalid dependencies injected");
        return;
    }
    
    // Load stay on top setting
    m_settings->beginGroup("Window");
    m_stayOnTop = m_settings->value("stayOnTop", false).toBool();
    m_settings->endGroup();
    
    // Apply stay on top if enabled
    if (m_stayOnTop) {
        setStayOnTop(true);
    }
    
    LOG_DEBUG("WindowManager: Initialized with dependency injection");
}

WindowManager::~WindowManager()
{
    destroyResizeCorners();
    destroyResizeEdges();
}

void WindowManager::restoreWindowState()
{
    m_settings->beginGroup("Window");
    
    // Restore window geometry
    QByteArray geometry = m_settings->value("geometry").toByteArray();
    if (!geometry.isEmpty()) {
        m_mainWindow->restoreGeometry(geometry);
        LOG_DEBUG("WindowManager: Restored window geometry");
    } else {
        // Default size and center on screen
        m_mainWindow->resize(1000, 700);
        
        QScreen* screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenGeometry = screen->availableGeometry();
            int x = (screenGeometry.width() - m_mainWindow->width()) / 2;
            int y = (screenGeometry.height() - m_mainWindow->height()) / 2;
            m_mainWindow->move(x, y);
        }
        
        LOG_DEBUG("WindowManager: Applied default window size and position");
    }
    
    // Restore window state (maximized, etc.)
    QByteArray windowState = m_settings->value("windowState").toByteArray();
    if (!windowState.isEmpty()) {
        m_mainWindow->restoreState(windowState);
        LOG_DEBUG("WindowManager: Restored window state");
    }
    
    m_settings->endGroup();
}

void WindowManager::saveWindowState()
{
    m_settings->beginGroup("Window");
    m_settings->setValue("geometry", m_mainWindow->saveGeometry());
    m_settings->setValue("windowState", m_mainWindow->saveState());
    m_settings->setValue("stayOnTop", m_stayOnTop);
    m_settings->endGroup();
    m_settings->sync();
    
    LOG_DEBUG("WindowManager: Saved window state");
    if (m_eventBus) {
        m_eventBus->applicationEvents()->emitWindowStateChanged();
    }
}

void WindowManager::setupResizeCorners()
{
    createResizeCorners();
    updateCornerPositions();
    LOG_DEBUG("WindowManager: Setup resize corners");
}

void WindowManager::setupResizeEdges()
{
    createResizeEdges();
    updateEdgePositions();
    LOG_DEBUG("WindowManager: Setup resize edges");
}

void WindowManager::updateCornerPositions()
{
    if (!m_mainWindow) return;

    int width = m_mainWindow->width();
    int height = m_mainWindow->height();

    if (m_bottomLeftCorner) {
        m_bottomLeftCorner->move(0, height - 12);
    }

    if (m_bottomRightCorner) {
        m_bottomRightCorner->move(width - 12, height - 12);
    }
}

void WindowManager::updateEdgePositions()
{
    if (!m_mainWindow) return;
    
    int width = m_mainWindow->width();
    int height = m_mainWindow->height();
    
    if (m_leftEdgeIndicator) {
        m_leftEdgeIndicator->move(0, (height - m_leftEdgeIndicator->height()) / 2);
    }
    
    if (m_rightEdgeIndicator) {
        m_rightEdgeIndicator->move(width - 7, (height - m_rightEdgeIndicator->height()) / 2);
    }
    
    if (m_bottomEdgeIndicator) {
        int bottomX = (width - m_bottomEdgeIndicator->width()) / 2;
        m_bottomEdgeIndicator->move(bottomX, height - 7);
    }
}

bool WindowManager::handleMousePress(QMouseEvent* event)
{
    if (!event) return false;
    
    QPoint pos = event->pos();
    Qt::Edges edges = getResizeEdges(pos);
    
    if (edges != Qt::Edges()) {
        // Start resizing
        startResizing(event->globalPosition().toPoint(), edges);
        return true;
    } else if (event->button() == Qt::LeftButton) {
        // Start dragging
        startDragging(event->globalPosition().toPoint());
        return true;
    }
    
    return false;
}

bool WindowManager::handleMouseMove(QMouseEvent* event)
{
    if (!event) return false;
    
    if (m_resizing) {
        performResize(event->globalPosition().toPoint());
        return true;
    } else if (m_dragging) {
        performDrag(event->globalPosition().toPoint());
        return true;
    } else {
        // Update cursor based on position
        Qt::Edges edges = getResizeEdges(event->pos());
        if (edges & Qt::LeftEdge && edges & Qt::TopEdge) {
            m_mainWindow->setCursor(Qt::SizeFDiagCursor);
        } else if (edges & Qt::RightEdge && edges & Qt::TopEdge) {
            m_mainWindow->setCursor(Qt::SizeBDiagCursor);
        } else if (edges & Qt::LeftEdge && edges & Qt::BottomEdge) {
            m_mainWindow->setCursor(Qt::SizeBDiagCursor);
        } else if (edges & Qt::RightEdge && edges & Qt::BottomEdge) {
            m_mainWindow->setCursor(Qt::SizeFDiagCursor);
        } else if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
            m_mainWindow->setCursor(Qt::SizeHorCursor);
        } else if (edges & (Qt::TopEdge | Qt::BottomEdge)) {
            m_mainWindow->setCursor(Qt::SizeVerCursor);
        } else {
            m_mainWindow->setCursor(Qt::ArrowCursor);
        }
    }
    
    return false;
}

bool WindowManager::handleMouseRelease(QMouseEvent* event)
{
    Q_UNUSED(event)
    
    if (m_dragging || m_resizing) {
        stopDragResize();
        return true;
    }
    
    return false;
}

void WindowManager::handleResize(QResizeEvent* event)
{
    Q_UNUSED(event)
    
    updateCornerPositions();
    updateEdgePositions();
}

Qt::Edges WindowManager::getResizeEdges(const QPoint& pos) const
{
    if (!m_mainWindow) return Qt::Edges();
    
    Qt::Edges edges;
    int width = m_mainWindow->width();
    int height = m_mainWindow->height();
    
    if (pos.x() < RESIZE_BORDER_WIDTH) {
        edges |= Qt::LeftEdge;
    } else if (pos.x() > width - RESIZE_BORDER_WIDTH) {
        edges |= Qt::RightEdge;
    }
    
    if (pos.y() < RESIZE_BORDER_WIDTH) {
        edges |= Qt::TopEdge;
    } else if (pos.y() > height - RESIZE_BORDER_WIDTH) {
        edges |= Qt::BottomEdge;
    }
    
    return edges;
}

void WindowManager::setStayOnTop(bool enabled)
{
    if (m_stayOnTop == enabled) return;
    
    m_stayOnTop = enabled;
    
#ifdef Q_OS_WIN
    // Windows-specific implementation
    HWND hwnd = (HWND)m_mainWindow->winId();
    if (enabled) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    } else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
#else
    // Cross-platform implementation using Qt flags
    Qt::WindowFlags flags = m_mainWindow->windowFlags();
    if (enabled) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    m_mainWindow->setWindowFlags(flags);
    m_mainWindow->show(); // Required to apply flag changes
#endif
    
    // Save setting
    m_settings->beginGroup("Window");
    m_settings->setValue("stayOnTop", enabled);
    m_settings->endGroup();
    m_settings->sync();
    
    LOG_DEBUG(QString("WindowManager: Stay on top %1").arg(enabled ? "enabled" : "disabled"));
    if (m_eventBus) {
        m_eventBus->applicationEvents()->emitStayOnTopChanged(enabled);
    }
}

bool WindowManager::isStayOnTop() const
{
    return m_stayOnTop;
}

void WindowManager::createResizeCorners()
{
    if (!m_mainWindow) return;

    // Create bottom-left corner with visible blue square and hover effect
    m_bottomLeftCorner = new QWidget(m_mainWindow);
    m_bottomLeftCorner->setFixedSize(12, 12);
    m_bottomLeftCorner->setStyleSheet(
        "QWidget {"
        "  background-color: #484F58;"
        "  border: 1px solid #6B7280;"
        "  border-radius: 2px;"
        "}"
        "QWidget:hover {"
        "  background-color: #0c7ff2;"
        "  border: 1px solid #0969DA;"
        "}"
    );
    m_bottomLeftCorner->setCursor(Qt::SizeBDiagCursor);
    m_bottomLeftCorner->setToolTip("Drag to resize window");
    m_bottomLeftCorner->show();

    // Create bottom-right corner with visible blue square and hover effect
    m_bottomRightCorner = new QWidget(m_mainWindow);
    m_bottomRightCorner->setFixedSize(12, 12);
    m_bottomRightCorner->setStyleSheet(
        "QWidget {"
        "  background-color: #484F58;"
        "  border: 1px solid #6B7280;"
        "  border-radius: 2px;"
        "}"
        "QWidget:hover {"
        "  background-color: #0c7ff2;"
        "  border: 1px solid #0969DA;"
        "}"
    );
    m_bottomRightCorner->setCursor(Qt::SizeFDiagCursor);
    m_bottomRightCorner->setToolTip("Drag to resize window");
    m_bottomRightCorner->show();

    LOG_DEBUG("WindowManager: Created visible resize corners with hover effects");
}

void WindowManager::createResizeEdges()
{
    if (!m_mainWindow) return;
    
    // Create left edge indicator
    m_leftEdgeIndicator = new QWidget(m_mainWindow);
    m_leftEdgeIndicator->setFixedSize(7, 50);
    m_leftEdgeIndicator->setStyleSheet("background-color: transparent;");
    m_leftEdgeIndicator->setCursor(Qt::SizeHorCursor);
    m_leftEdgeIndicator->show();
    
    // Create right edge indicator
    m_rightEdgeIndicator = new QWidget(m_mainWindow);
    m_rightEdgeIndicator->setFixedSize(7, 50);
    m_rightEdgeIndicator->setStyleSheet("background-color: transparent;");
    m_rightEdgeIndicator->setCursor(Qt::SizeHorCursor);
    m_rightEdgeIndicator->show();
    
    // Create bottom edge indicator
    m_bottomEdgeIndicator = new QWidget(m_mainWindow);
    m_bottomEdgeIndicator->setFixedSize(50, 7);
    m_bottomEdgeIndicator->setStyleSheet("background-color: transparent;");
    m_bottomEdgeIndicator->setCursor(Qt::SizeVerCursor);
    m_bottomEdgeIndicator->show();
    
    LOG_DEBUG("WindowManager: Created resize edges");
}

void WindowManager::destroyResizeCorners()
{
    if (m_bottomLeftCorner) {
        m_bottomLeftCorner->deleteLater();
        m_bottomLeftCorner = nullptr;
    }
    
    if (m_bottomRightCorner) {
        m_bottomRightCorner->deleteLater();
        m_bottomRightCorner = nullptr;
    }
}

void WindowManager::destroyResizeEdges()
{
    if (m_leftEdgeIndicator) {
        m_leftEdgeIndicator->deleteLater();
        m_leftEdgeIndicator = nullptr;
    }
    
    if (m_rightEdgeIndicator) {
        m_rightEdgeIndicator->deleteLater();
        m_rightEdgeIndicator = nullptr;
    }
    
    if (m_bottomEdgeIndicator) {
        m_bottomEdgeIndicator->deleteLater();
        m_bottomEdgeIndicator = nullptr;
    }
}

void WindowManager::startDragging(const QPoint& globalPos)
{
    m_dragging = true;
    m_dragPosition = globalPos - m_mainWindow->pos();
    LOG_DEBUG("WindowManager: Started window dragging");
}

void WindowManager::startResizing(const QPoint& globalPos, Qt::Edges edges)
{
    m_resizing = true;
    m_resizeEdges = edges;
    m_dragPosition = globalPos;
    LOG_DEBUG("WindowManager: Started window resizing");
}

void WindowManager::performDrag(const QPoint& globalPos)
{
    if (!m_dragging) return;
    
    QPoint newPos = globalPos - m_dragPosition;
    m_mainWindow->move(newPos);
}

void WindowManager::performResize(const QPoint& globalPos)
{
    if (!m_resizing) return;
    
    QRect geometry = calculateNewGeometry(globalPos);
    constrainWindowSize(geometry);
    
    m_mainWindow->setGeometry(geometry);
    m_dragPosition = globalPos;
}

void WindowManager::stopDragResize()
{
    if (m_dragging || m_resizing) {
        m_dragging = false;
        m_resizing = false;
        m_resizeEdges = Qt::Edges();
        m_mainWindow->setCursor(Qt::ArrowCursor);
        
        // Save window state after drag/resize
        saveWindowState();
        
        LOG_DEBUG("WindowManager: Stopped window drag/resize");
    }
}

bool WindowManager::isInResizeArea(const QPoint& pos) const
{
    return getResizeEdges(pos) != Qt::Edges();
}

QRect WindowManager::calculateNewGeometry(const QPoint& globalPos) const
{
    QRect geometry = m_mainWindow->geometry();
    QPoint delta = globalPos - m_dragPosition;
    
    if (m_resizeEdges & Qt::LeftEdge) {
        geometry.setLeft(geometry.left() + delta.x());
    }
    if (m_resizeEdges & Qt::RightEdge) {
        geometry.setRight(geometry.right() + delta.x());
    }
    if (m_resizeEdges & Qt::TopEdge) {
        geometry.setTop(geometry.top() + delta.y());
    }
    if (m_resizeEdges & Qt::BottomEdge) {
        geometry.setBottom(geometry.bottom() + delta.y());
    }
    
    return geometry;
}

void WindowManager::constrainWindowSize(QRect& geometry) const
{
    // Enforce minimum size
    if (geometry.width() < MIN_WINDOW_WIDTH) {
        if (m_resizeEdges & Qt::LeftEdge) {
            geometry.setLeft(geometry.right() - MIN_WINDOW_WIDTH);
        } else {
            geometry.setWidth(MIN_WINDOW_WIDTH);
        }
    }
    
    if (geometry.height() < MIN_WINDOW_HEIGHT) {
        if (m_resizeEdges & Qt::TopEdge) {
            geometry.setTop(geometry.bottom() - MIN_WINDOW_HEIGHT);
        } else {
            geometry.setHeight(MIN_WINDOW_HEIGHT);
        }
    }
    
    // Keep window on screen
    QScreen* screen = QApplication::screenAt(geometry.center());
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        
        if (geometry.left() < screenGeometry.left()) {
            geometry.moveLeft(screenGeometry.left());
        }
        if (geometry.top() < screenGeometry.top()) {
            geometry.moveTop(screenGeometry.top());
        }
        if (geometry.right() > screenGeometry.right()) {
            geometry.moveRight(screenGeometry.right());
        }
        if (geometry.bottom() > screenGeometry.bottom()) {
            geometry.moveBottom(screenGeometry.bottom());
        }
    }
}