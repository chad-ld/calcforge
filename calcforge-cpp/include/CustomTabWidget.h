#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QTabWidget>
#include <QTabBar>
#include <QString>
#include <QMap>

/**
 * Custom QTabWidget that properly handles ampersand characters in tab names
 * without Qt's mnemonic interpretation interfering with display
 */
class CustomTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit CustomTabWidget(QWidget* parent = nullptr);
    
    // Override tab text methods to handle ampersands properly
    void setTabText(int index, const QString& text);
    QString tabText(int index) const;
    
    // Add tab with proper ampersand handling
    int addTab(QWidget* widget, const QString& label);
    int addTab(QWidget* widget, const QIcon& icon, const QString& label);

private:
    // Store the original (unescaped) tab names
    QMap<int, QString> m_originalTabNames;
    
    // Helper methods
    QString escapeAmpersands(const QString& text) const;
    QString unescapeAmpersands(const QString& text) const;
    void updateTabMapping(int index, const QString& originalText);
    void removeTabMapping(int index);
    void shiftTabMappings(int removedIndex);

private slots:
    void onTabRemoved(int index);
};

#endif // CUSTOMTABWIDGET_H
