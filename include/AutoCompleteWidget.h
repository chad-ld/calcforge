#ifndef AUTOCOMPLETEWIDGET_H
#define AUTOCOMPLETEWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>
#include <QRegularExpression>
#include <QStringList>
#include <QMap>

class ExpressionEditor;

/**
 * Autocomplete popup list widget with custom styling
 */
class AutoCompleteList : public QListWidget
{
    Q_OBJECT

public:
    explicit AutoCompleteList(QWidget *parent = nullptr);
    
protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void itemSelected(const QString &text);
    void escapePressed();
};

/**
 * Description panel showing help text for selected autocomplete item
 */
class AutoCompleteDescriptionBox : public QLabel
{
    Q_OBJECT

public:
    explicit AutoCompleteDescriptionBox(QWidget *parent = nullptr);
    void updateDescription(const QString &text);

private:
    void setupStyling();
};

/**
 * Main autocomplete widget containing list and description panel
 */
class AutoCompleteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AutoCompleteWidget(QWidget *parent = nullptr);
    ~AutoCompleteWidget();

    // Main interface methods
    void showCompletions(const QStringList &completions, const QStringList &descriptions, const QPoint &position);
    void hideCompletions();
    bool isVisible() const;
    
    // Navigation methods
    void selectNext();
    void selectPrevious();
    void selectFirst();
    QString getCurrentSelection() const;
    
    // Event handling
    void handleKeyPress(QKeyEvent *event);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void itemSelected(const QString &text);
    void cancelled();

private slots:
    void onItemSelectionChanged();
    void onItemClicked(QListWidgetItem *item);

private:
    void setupUI();
    void setupStyling();
    void positionWidget(const QPoint &position);
    void updateDescriptionForCurrentItem();

    // UI Components
    QHBoxLayout *m_mainLayout;
    AutoCompleteList *m_listWidget;
    AutoCompleteDescriptionBox *m_descriptionBox;
    
    // Data
    QStringList m_completions;
    QStringList m_descriptions;
    int m_selectedIndex;
    
    // Styling constants
    static const int POPUP_WIDTH;
    static const int POPUP_HEIGHT;
    static const int DESCRIPTION_WIDTH;
    static const QString POPUP_STYLE;
};

/**
 * Autocomplete data structure for function information
 */
struct AutoCompleteFunction
{
    QString name;
    QString description;
    QStringList parameters;
    QString category;
    
    AutoCompleteFunction() = default;
    AutoCompleteFunction(const QString &n, const QString &desc, const QStringList &params = QStringList(), const QString &cat = "function")
        : name(n), description(desc), parameters(params), category(cat) {}
};

/**
 * Main autocomplete manager handling logic and data
 */
class AutoCompleteManager : public QObject
{
    Q_OBJECT

public:
    explicit AutoCompleteManager(ExpressionEditor *editor, QWidget *parent = nullptr);
    ~AutoCompleteManager();

    // Main interface
    void showAutocomplete();
    void hideAutocomplete();
    bool isVisible() const;
    
    // Event handling
    void handleKeyPress(QKeyEvent *event);
    void handleTextChanged();

public slots:
    void onItemSelected(const QString &text);
    void onCancelled();

private:
    void setupFunctions();
    void setupUnits();
    void setupCurrencies();
    
    // Context analysis
    QString getCurrentWord();
    QString getContextType();
    bool isAfterNumber();
    bool isAtStartOfLine();
    bool isAfterPercentileRange();
    bool isAfterConversionTo();
    QString getFunctionContext();
    QStringList getAvailableSheets();
    
    // Filtering
    QStringList filterCompletions(const QString &prefix, const QString &context);
    QStringList getDescriptions(const QStringList &completions, const QString &context);
    
    // Text manipulation
    void insertCompletion(const QString &completion);
    void replaceCurrentWord(const QString &replacement);
    
    // Data structures
    QMap<QString, AutoCompleteFunction> m_functions;
    QStringList m_units;
    QStringList m_currencies;
    QMap<QString, QString> m_functionDescriptions;
    QMap<QString, QStringList> m_functionParameters;
    
    // UI and state
    ExpressionEditor *m_editor;
    AutoCompleteWidget *m_widget;
    QString m_currentWord;
    QString m_currentContext;
    int m_wordStartPos;
    
    // Timing
    QTimer *m_showTimer;
    bool m_startupDelay;
    static const int SHOW_DELAY_MS;
};

#endif // AUTOCOMPLETEWIDGET_H
