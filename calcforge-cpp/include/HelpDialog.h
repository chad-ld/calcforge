#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QListWidget>
#include <QScrollBar>
#include <QClipboard>
#include <QTimer>

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = nullptr);
    void setCurrentFilePath(const QString &filePath);

private slots:
    void onTopicSelected(int row);
    void closeDialog();
    void copyFilePathToClipboard();

private:
    void setupUI();
    void setupContent();
    void createTopicList();
    void createContentArea();
    void addTopic(const QString &title, const QString &content);
    
    // UI Components
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_buttonLayout;
    QSplitter *m_splitter;
    QListWidget *m_topicList;
    QTextEdit *m_contentArea;
    QPushButton *m_closeButton;
    QPushButton *m_clipboardButton;
    QLabel *m_filePathLabel;
    
    // Content storage
    QStringList m_topicTitles;
    QStringList m_topicContents;

    // File path storage for clipboard functionality
    QString m_currentFilePath;
};

#endif // HELPDIALOG_H
