#include "ApplicationEvents.h"
#include "Logger.h"

ApplicationEvents::ApplicationEvents(QObject *parent)
    : QObject(parent)
{
    LOG_DEBUG("ApplicationEvents: Application event hub initialized");
}

// File operation events
void ApplicationEvents::emitFileStateChanged(bool hasUnsavedChanges)
{
    LOG_DEBUG(QString("ApplicationEvents: File state changed - Has unsaved changes: %1")
              .arg(hasUnsavedChanges ? "true" : "false"));
    emit fileStateChanged(hasUnsavedChanges);
}

void ApplicationEvents::emitCurrentFileChanged(const QString& filePath)
{
    LOG_DEBUG(QString("ApplicationEvents: Current file changed - Path: %1").arg(filePath));
    emit currentFileChanged(filePath);
}

void ApplicationEvents::emitRecentFilesChanged(const QStringList& recentFiles)
{
    LOG_DEBUG(QString("ApplicationEvents: Recent files changed - Count: %1").arg(recentFiles.size()));
    emit recentFilesChanged(recentFiles);
}

void ApplicationEvents::emitFileLoadRequested(const QString& filePath)
{
    LOG_DEBUG(QString("ApplicationEvents: File load requested - Path: %1").arg(filePath));
    emit fileLoadRequested(filePath);
}

void ApplicationEvents::emitFileSaveRequested(const QString& filePath)
{
    LOG_DEBUG(QString("ApplicationEvents: File save requested - Path: %1").arg(filePath));
    emit fileSaveRequested(filePath);
}

void ApplicationEvents::emitFileSaveAsRequested()
{
    LOG_DEBUG("ApplicationEvents: File save as requested");
    emit fileSaveAsRequested();
}

// Tab management events
void ApplicationEvents::emitTabAdded(int index, const QString& name)
{
    LOG_DEBUG(QString("ApplicationEvents: Tab added - Index: %1, Name: %2").arg(index).arg(name));
    emit tabAdded(index, name);
}

void ApplicationEvents::emitTabClosed(int index, const QString& name)
{
    LOG_DEBUG(QString("ApplicationEvents: Tab closed - Index: %1, Name: %2").arg(index).arg(name));
    emit tabClosed(index, name);
}

void ApplicationEvents::emitTabRenamed(int index, const QString& oldName, const QString& newName)
{
    LOG_DEBUG(QString("ApplicationEvents: Tab renamed - Index: %1, Old: %2, New: %3")
              .arg(index).arg(oldName).arg(newName));
    emit tabRenamed(index, oldName, newName);
}

void ApplicationEvents::emitTabMoved(int from, int to)
{
    LOG_DEBUG(QString("ApplicationEvents: Tab moved - From: %1, To: %2").arg(from).arg(to));
    emit tabMoved(from, to);
}

void ApplicationEvents::emitCurrentTabChanged(int index)
{
    LOG_DEBUG(QString("ApplicationEvents: Current tab changed - Index: %1").arg(index));
    emit currentTabChanged(index);
}

void ApplicationEvents::emitTabSetupRequested(const QString& worksheetName)
{
    LOG_DEBUG(QString("ApplicationEvents: Tab setup requested - Worksheet: %1").arg(worksheetName));
    emit tabSetupRequested(worksheetName);
}

// Window state events
void ApplicationEvents::emitWindowStateChanged()
{
    LOG_DEBUG("ApplicationEvents: Window state changed");
    emit windowStateChanged();
}

void ApplicationEvents::emitStayOnTopChanged(bool enabled)
{
    LOG_DEBUG(QString("ApplicationEvents: Stay on top changed - Enabled: %1")
              .arg(enabled ? "true" : "false"));
    emit stayOnTopChanged(enabled);
}

void ApplicationEvents::emitFontSizeChanged(int newSize)
{
    LOG_DEBUG(QString("ApplicationEvents: Font size changed - New size: %1").arg(newSize));
    emit fontSizeChanged(newSize);
}

void ApplicationEvents::emitGlobalSplitterStateChanged(const QByteArray& newState)
{
    LOG_DEBUG("ApplicationEvents: Global splitter state changed");
    emit globalSplitterStateChanged(newState);
}

// Application lifecycle events
void ApplicationEvents::emitApplicationInitialized()
{
    LOG_DEBUG("ApplicationEvents: Application initialized");
    emit applicationInitialized();
}

void ApplicationEvents::emitApplicationShutdown()
{
    LOG_DEBUG("ApplicationEvents: Application shutdown");
    emit applicationShutdown();
}

void ApplicationEvents::emitManagersInitialized()
{
    LOG_DEBUG("ApplicationEvents: Managers initialized");
    emit managersInitialized();
}
