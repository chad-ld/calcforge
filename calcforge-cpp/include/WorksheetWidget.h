#ifndef WORKSHEETWIDGET_H
#define WORKSHEETWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QVBoxLayout>
#include <QSettings>
#include <QTimer>
#include <QTextEdit>
#include <memory>

class ExpressionEditor;
class ResultsDisplay;
class LineNumberArea;
class CalculationEngine;
class DependencyTracker;
class LNReferenceAutoUpdater;
struct LineChange;

// Phase 3.2: Business logic separation
class WorksheetModel;
class CalculationService;

/**
 * Individual worksheet widget containing expression editor and results display
 * Phase 3.2: Now uses WorksheetModel and CalculationService for business logic separation
 * UI-only responsibilities: display, user interaction, visual feedback
 */
class WorksheetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorksheetWidget(QWidget *parent = nullptr);
    ~WorksheetWidget();
    
    // Content management
    QString getContent() const;
    void setContent(const QString &content);
    bool isModified() const;
    void setModified(bool modified);
    
    // Editor access
    ExpressionEditor* getEditor() const { return m_editor; }
    ResultsDisplay* getResults() const { return m_results; }

    // Phase 3.2: Business logic access through model and service
    WorksheetModel* getModel() const { return m_model.get(); }
    CalculationService* getCalculationService() const { return m_calculationService.get(); }

    // Cross-sheet reference support (delegated to model/service)
    double getLineValue(int lineNumber) const;
    bool hasLineValue(int lineNumber) const;
    CalculationEngine* getCalculationEngine() const; // Legacy support
    bool hasCrossSheetReferences() const;
    QString getCurrentSheetName() const;

    // Expression evaluation for tooltips (delegated to service)
    QString evaluateExpression(const QString &expression) const;
    
    // Splitter state
    QByteArray getSplitterState() const;
    void setSplitterState(const QByteArray &state);

signals:
    void contentChanged();
    void crossSheetReferencesChanged();
    void splitterMoved(const QByteArray &newState);
    void lineNumberingChanged(const QString &sheetName, const QList<LineChange> &changes);
    void valuesChanged(const QString &sheetName);

public slots:
    void evaluateAndHighlight();
    void onTextChanged();

public:
    void forceRecalculation();
    void handleCrossSheetBackgroundHighlighting(const QString &currentLineText);

private slots:
    void syncEditorToResults(int value);
    void syncResultsToEditor(int value);

private:
    void clearAllCrossSheetHighlighting();

    // Phase 3.2: Business logic setup
    void setupBusinessLogic();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUI();
    void setupConnections();
    void startEvaluationTimer();

    /**
     * Detect which lines have changed since last evaluation
     * @param currentLines Current line content
     * @return Set of line numbers that changed
     */
    QSet<int> detectChangedLines(const QStringList &currentLines);

    /**
     * Update calculation engine line values after line insertions/deletions
     * @param oldLines Previous line content
     * @param newLines Current line content
     */
    void updateCalculationEngineLineValues(const QStringList &oldLines, const QStringList &newLines);

    /**
     * Perform the actual evaluation of lines
     * @param currentLines Current line content
     * @param changedLines Set of line numbers that changed
     */
    void evaluateLines(const QStringList &currentLines, const QSet<int> &changedLines);
    
    // UI Components
    QVBoxLayout *m_layout;
    ExpressionEditor *m_editor;
    ResultsDisplay *m_results;
    LineNumberArea *m_lineNumberArea;        // Line numbers for expression editor
    LineNumberArea *m_resultsLineNumberArea; // Line numbers for results display
    QSplitter *m_columnsSplitter;            // Splitter between expression and results areas
    
    // Evaluation and timing
    QTimer *m_evaluationTimer;
    bool m_isModified;
    bool m_splitterStateRestored;
    QByteArray m_pendingSplitterState;
    
    // Performance optimization
    QString m_lastContent;
    QStringList m_lastLines;
    QSet<int> m_changedLines;
    QHash<int, QString> m_lineHashes;
    
    // Cross-sheet reference tracking
    bool m_hasCrossSheetRefs;
    
    // Settings
    QSettings *m_settings;

    // Phase 3.2: Business logic separation
    std::unique_ptr<WorksheetModel> m_model;
    std::unique_ptr<CalculationService> m_calculationService;

    // Legacy support (will be removed in future phases)
    CalculationEngine *m_calculationEngine;
    std::unique_ptr<DependencyTracker> m_dependencyTracker;

    // LN Reference Auto-Update System
    std::unique_ptr<LNReferenceAutoUpdater> m_referenceAutoUpdater;
    bool m_isLoadingContent;  // Flag to disable auto-updates during content loading
};

#endif // WORKSHEETWIDGET_H
