#ifndef APPLICATIONEVENTS_H
#define APPLICATIONEVENTS_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QByteArray>

/**
 * @brief Central event hub for application-level events
 * 
 * This class handles events related to file operations, tab management,
 * window state changes, and other application-wide concerns. It works
 * alongside WorksheetEvents to provide a complete event-driven architecture.
 * 
 * Phase 4.1: Event System Implementation
 */
class ApplicationEvents : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationEvents(QObject *parent = nullptr);
    ~ApplicationEvents() = default;

    // File operation events
    void emitFileStateChanged(bool hasUnsavedChanges);
    void emitCurrentFileChanged(const QString& filePath);
    void emitRecentFilesChanged(const QStringList& recentFiles);
    void emitFileLoadRequested(const QString& filePath = QString());
    void emitFileSaveRequested(const QString& filePath = QString());
    void emitFileSaveAsRequested();
    
    // Tab management events
    void emitTabAdded(int index, const QString& name);
    void emitTabClosed(int index, const QString& name);
    void emitTabRenamed(int index, const QString& oldName, const QString& newName);
    void emitTabMoved(int from, int to);
    void emitCurrentTabChanged(int index);
    void emitTabSetupRequested(const QString& worksheetName);
    
    // Window state events
    void emitWindowStateChanged();
    void emitStayOnTopChanged(bool enabled);
    void emitFontSizeChanged(int newSize);
    void emitGlobalSplitterStateChanged(const QByteArray& newState);
    
    // Application lifecycle events
    void emitApplicationInitialized();
    void emitApplicationShutdown();
    void emitManagersInitialized();

signals:
    // File operation signals
    void fileStateChanged(bool hasUnsavedChanges);
    void currentFileChanged(const QString& filePath);
    void recentFilesChanged(const QStringList& recentFiles);
    void fileLoadRequested(const QString& filePath);
    void fileSaveRequested(const QString& filePath);
    void fileSaveAsRequested();
    
    // Tab management signals
    void tabAdded(int index, const QString& name);
    void tabClosed(int index, const QString& name);
    void tabRenamed(int index, const QString& oldName, const QString& newName);
    void tabMoved(int from, int to);
    void currentTabChanged(int index);
    void tabSetupRequested(const QString& worksheetName);
    
    // Window state signals
    void windowStateChanged();
    void stayOnTopChanged(bool enabled);
    void fontSizeChanged(int newSize);
    void globalSplitterStateChanged(const QByteArray& newState);
    
    // Application lifecycle signals
    void applicationInitialized();
    void applicationShutdown();
    void managersInitialized();
    
    // Dialog request signals (for maintaining always-on-top behavior)
    void requestTemporaryDisableAlwaysOnTop();
    void requestRestoreAlwaysOnTop();
};

#endif // APPLICATIONEVENTS_H
