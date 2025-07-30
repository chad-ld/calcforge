#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>
#include <QWidget>
#include <QPoint>
#include <QSettings>
#include <QByteArray>
#include "CalcForgeResult.h"

class QMainWindow;
class QMouseEvent;
class QResizeEvent;
class EventBus;

/**
 * WindowManager handles window management operations for CalcForge
 * Including window resizing, dragging, positioning, and window state persistence
 * 
 * This class was extracted from MainWindow as part of Phase 2 refactoring
 * to decompose the God Object pattern and apply dependency injection.
 */
class WindowManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowManager(QMainWindow* mainWindow, QSettings* settings, EventBus* eventBus, QObject* parent = nullptr);
    ~WindowManager();

    // Window state management
    void restoreWindowState();
    void saveWindowState();
    
    // Resize corner and edge management
    void setupResizeCorners();
    void setupResizeEdges();
    void updateCornerPositions();
    void updateEdgePositions();
    
    // Mouse event handling
    bool handleMousePress(QMouseEvent* event);
    bool handleMouseMove(QMouseEvent* event);  
    bool handleMouseRelease(QMouseEvent* event);
    
    // Resize event handling
    void handleResize(QResizeEvent* event);
    
    // Window positioning
    Qt::Edges getResizeEdges(const QPoint& pos) const;
    
    // Stay on top functionality
    void setStayOnTop(bool enabled);
    bool isStayOnTop() const;

signals:
    void windowStateChanged();
    void stayOnTopChanged(bool enabled);

private:
    // Corner and edge widgets
    void createResizeCorners();
    void createResizeEdges();
    void destroyResizeCorners();
    void destroyResizeEdges();
    
    // Window dragging and resizing logic
    void startDragging(const QPoint& globalPos);
    void startResizing(const QPoint& globalPos, Qt::Edges edges);
    void performDrag(const QPoint& globalPos);
    void performResize(const QPoint& globalPos);
    void stopDragResize();
    
    // Helper methods
    bool isInResizeArea(const QPoint& pos) const;
    QRect calculateNewGeometry(const QPoint& globalPos) const;
    void constrainWindowSize(QRect& geometry) const;
    
    // Dependencies (injected)
    QMainWindow* m_mainWindow;
    QSettings* m_settings;
    EventBus* m_eventBus;
    
    // Resize corner widgets
    QWidget* m_bottomLeftCorner;
    QWidget* m_bottomRightCorner;
    
    // Resize edge indicators
    QWidget* m_leftEdgeIndicator;
    QWidget* m_rightEdgeIndicator;
    QWidget* m_bottomEdgeIndicator;
    
    // Window state
    QPoint m_dragPosition;
    bool m_dragging;
    bool m_resizing;
    Qt::Edges m_resizeEdges;
    bool m_stayOnTop;
    
    // Window constraints
    static const int MIN_WINDOW_WIDTH = 400;
    static const int MIN_WINDOW_HEIGHT = 300;
    static const int RESIZE_BORDER_WIDTH = 10;
};

#endif // WINDOWMANAGER_H