#include "CustomTabWidget.h"
#include "Logger.h"
#include <QTabBar>

CustomTabWidget::CustomTabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    // Connect to tab removal signal to maintain our mapping
    connect(this, &QTabWidget::tabCloseRequested, this, &CustomTabWidget::onTabRemoved);
    
    // Disable mnemonics on the tab bar
    QTabBar* tabBar = this->tabBar();
    if (tabBar) {
        tabBar->setProperty("_q_no_mnemonic", true);
        tabBar->setFocusPolicy(Qt::NoFocus);
        tabBar->setAttribute(Qt::WA_KeyboardFocusChange, false);
    }
    
    LOG_DEBUG("CustomTabWidget: Initialized with ampersand handling");
}

void CustomTabWidget::setTabText(int index, const QString& text)
{
    if (index < 0 || index >= count()) {
        return;
    }
    
    // Store the original text
    updateTabMapping(index, text);
    
    // Escape ampersands for Qt display
    QString escapedText = escapeAmpersands(text);
    
    // Call the base class method with escaped text
    QTabWidget::setTabText(index, escapedText);
    
    LOG_DEBUG(QString("CustomTabWidget: Set tab text - Original: '%1', Escaped: '%2'")
              .arg(text).arg(escapedText));
}

QString CustomTabWidget::tabText(int index) const
{
    if (index < 0 || index >= count()) {
        return QString();
    }
    
    // Return the original unescaped text if we have it stored
    if (m_originalTabNames.contains(index)) {
        QString originalText = m_originalTabNames.value(index);
        LOG_DEBUG(QString("CustomTabWidget: Get tab text - Returning original: '%1'").arg(originalText));
        return originalText;
    }
    
    // Fallback: get from Qt and unescape
    QString qtText = QTabWidget::tabText(index);
    QString unescapedText = unescapeAmpersands(qtText);
    LOG_DEBUG(QString("CustomTabWidget: Get tab text - Qt text: '%1', Unescaped: '%2'")
              .arg(qtText).arg(unescapedText));
    return unescapedText;
}

int CustomTabWidget::addTab(QWidget* widget, const QString& label)
{
    // Store the original label
    QString escapedLabel = escapeAmpersands(label);
    
    // Add tab with escaped label
    int index = QTabWidget::addTab(widget, escapedLabel);
    
    // Store the original text mapping
    if (index >= 0) {
        updateTabMapping(index, label);
        LOG_DEBUG(QString("CustomTabWidget: Added tab - Original: '%1', Escaped: '%2', Index: %3")
                  .arg(label).arg(escapedLabel).arg(index));
    }
    
    return index;
}

int CustomTabWidget::addTab(QWidget* widget, const QIcon& icon, const QString& label)
{
    // Store the original label
    QString escapedLabel = escapeAmpersands(label);
    
    // Add tab with escaped label
    int index = QTabWidget::addTab(widget, icon, escapedLabel);
    
    // Store the original text mapping
    if (index >= 0) {
        updateTabMapping(index, label);
        LOG_DEBUG(QString("CustomTabWidget: Added tab with icon - Original: '%1', Escaped: '%2', Index: %3")
                  .arg(label).arg(escapedLabel).arg(index));
    }
    
    return index;
}

QString CustomTabWidget::escapeAmpersands(const QString& text) const
{
    QString escaped = text;
    escaped.replace("&", "&&");
    return escaped;
}

QString CustomTabWidget::unescapeAmpersands(const QString& text) const
{
    QString unescaped = text;
    unescaped.replace("&&", "&");
    return unescaped;
}

void CustomTabWidget::updateTabMapping(int index, const QString& originalText)
{
    m_originalTabNames[index] = originalText;
}

void CustomTabWidget::removeTabMapping(int index)
{
    m_originalTabNames.remove(index);
}

void CustomTabWidget::shiftTabMappings(int removedIndex)
{
    // When a tab is removed, we need to shift all mappings for tabs after it
    QMap<int, QString> newMappings;
    
    for (auto it = m_originalTabNames.begin(); it != m_originalTabNames.end(); ++it) {
        int currentIndex = it.key();
        QString text = it.value();
        
        if (currentIndex < removedIndex) {
            // Tabs before the removed one keep their index
            newMappings[currentIndex] = text;
        } else if (currentIndex > removedIndex) {
            // Tabs after the removed one shift down by 1
            newMappings[currentIndex - 1] = text;
        }
        // Skip the removed index
    }
    
    m_originalTabNames = newMappings;
}

void CustomTabWidget::onTabRemoved(int index)
{
    LOG_DEBUG(QString("CustomTabWidget: Tab removed at index %1, updating mappings").arg(index));
    shiftTabMappings(index);
}
 
