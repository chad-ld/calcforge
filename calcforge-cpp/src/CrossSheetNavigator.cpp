#include "CrossSheetNavigator.h"
#include "TabManager.h"
#include "WorksheetWidget.h"
#include "ExpressionEditor.h"
#include "LineChangeDetector.h"
#include "Logger.h"

#include <QRegularExpression>
#include <QTextCursor>
#include <QTextBlock>
#include <QRegularExpressionMatchIterator>

CrossSheetNavigator::CrossSheetNavigator(TabManager* tabManager, QObject* parent)
    : QObject(parent)
    , m_tabManager(tabManager)
    , m_hasNavigationHistory(false)
{
    if (!m_tabManager) {
        LOG_DEBUG("CrossSheetNavigator: Invalid TabManager dependency injected");
        return;
    }
    
    LOG_DEBUG("CrossSheetNavigator: Initialized with dependency injection");
}

void CrossSheetNavigator::navigateToSheet(const QString& sheetName, int lineNumber, int cursorPosition)
{
    if (sheetName.isEmpty()) {
        LOG_DEBUG("CrossSheetNavigator: Cannot navigate to empty sheet name");
        return;
    }
    
    // Save current location to navigation history
    WorksheetWidget* currentSheet = m_tabManager->getCurrentWorksheetWidget();
    if (currentSheet) {
        QString currentSheetName = m_tabManager->getCurrentTabName();
        int currentLine = currentSheet->getEditor()->textCursor().blockNumber() + 1;
        int currentCursor = currentSheet->getEditor()->textCursor().position();
        
        saveNavigationHistory(currentSheetName, currentLine, currentCursor);
    }
    
    // Perform navigation
    performNavigation(sheetName, lineNumber, cursorPosition);
    
    LOG_DEBUG(QString("CrossSheetNavigator: Navigated to sheet '%1', line %2")
              .arg(sheetName).arg(lineNumber));
}

void CrossSheetNavigator::saveNavigationHistory(const QString& sheetName, int lineNumber, int cursorPosition)
{
    if (sheetName.isEmpty() || lineNumber < 0) return;
    
    m_navigationHistory = NavigationHistory(sheetName, lineNumber, cursorPosition);
    m_hasNavigationHistory = true;
    
    LOG_DEBUG(QString("CrossSheetNavigator: Saved navigation history - Sheet: %1, Line: %2, Cursor: %3")
              .arg(sheetName).arg(lineNumber).arg(cursorPosition));
}

bool CrossSheetNavigator::hasNavigationHistory() const
{
    return m_hasNavigationHistory && m_navigationHistory.isValid();
}

void CrossSheetNavigator::returnToPreviousLocation()
{
    if (!hasNavigationHistory()) {
        LOG_DEBUG("CrossSheetNavigator: No navigation history available");
        return;
    }
    
    performNavigation(m_navigationHistory.sheetName, 
                     m_navigationHistory.lineNumber, 
                     m_navigationHistory.cursorPosition);
    
    // Clear navigation history after returning
    m_hasNavigationHistory = false;
    
    LOG_DEBUG(QString("CrossSheetNavigator: Returned to previous location - Sheet: %1, Line: %2")
              .arg(m_navigationHistory.sheetName).arg(m_navigationHistory.lineNumber));
}

double CrossSheetNavigator::getCrossSheetValue(const QString& sheetName, int lineNumber) const
{
    WorksheetWidget* worksheet = m_tabManager->getWorksheetByName(sheetName);
    if (!worksheet) {
        LOG_DEBUG("CrossSheetNavigator: Sheet not found: " + sheetName);
        return 0.0;
    }
    
    double value = worksheet->getLineValue(lineNumber);
    LOG_DEBUG(QString("CrossSheetNavigator: Retrieved value %1 from sheet '%2', line %3")
              .arg(value).arg(sheetName).arg(lineNumber));
    
    return value;
}

void CrossSheetNavigator::triggerCrossSheetRecalculation()
{
    LOG_DEBUG("CrossSheetNavigator: Triggering cross-sheet recalculation");
    recalculateAllWorksheets();
    emit crossSheetRecalculationTriggered();
}

void CrossSheetNavigator::recalculateAllWorksheets()
{
    int tabCount = m_tabManager->getTabCount();
    LOG_DEBUG(QString("CrossSheetNavigator: Recalculating all %1 worksheets").arg(tabCount));
    
    for (int i = 0; i < tabCount; ++i) {
        WorksheetWidget* worksheet = m_tabManager->getWorksheetWidget(i);
        if (worksheet) {
            worksheet->evaluateAndHighlight();
        }
    }
}

bool CrossSheetNavigator::updateCrossSheetReferences(WorksheetWidget* worksheet, 
                                                    const QString& changedSheetName, 
                                                    const QList<LineChange>& changes)
{
    if (!worksheet || changedSheetName.isEmpty() || changes.isEmpty()) {
        return false;
    }
    
    // Check if this worksheet has references to the changed sheet
    if (!hasCrossSheetReferencesToSheet(worksheet, changedSheetName)) {
        return false; // No references to update
    }
    
    QString content = worksheet->getContent();
    QString updatedContent = content;
    bool hasChanges = false;
    
    // Pattern to match cross-sheet references: S.SheetName.LN#
    QRegularExpression crossSheetPattern(
        QString(R"(\bS\.%1\.LN(\d+)\b)").arg(QRegularExpression::escape(changedSheetName)),
        QRegularExpression::CaseInsensitiveOption
    );
    
    QRegularExpressionMatchIterator iterator = crossSheetPattern.globalMatch(content);
    QList<QPair<int, int>> replacements; // position, new line number
    
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        int originalLine = match.captured(1).toInt();
        int newLine = calculateNewLineNumber(originalLine, changes);
        
        if (newLine != originalLine) {
            replacements.append({match.capturedStart(), newLine});
            hasChanges = true;
        }
    }
    
    // Apply replacements in reverse order to maintain positions
    std::sort(replacements.begin(), replacements.end(), 
              [](const QPair<int, int>& a, const QPair<int, int>& b) {
                  return a.first > b.first;
              });
    
    for (const auto& replacement : replacements) {
        QRegularExpressionMatch match = crossSheetPattern.match(updatedContent, replacement.first);
        if (match.hasMatch()) {
            QString oldRef = match.captured(0);
            QString newRef = oldRef;
            newRef.replace(QRegularExpression(R"(LN\d+)"), QString("LN%1").arg(replacement.second));
            
            updatedContent.replace(match.capturedStart(), match.capturedLength(), newRef);
        }
    }
    
    if (hasChanges) {
        worksheet->setContent(updatedContent);
        LOG_DEBUG(QString("CrossSheetNavigator: Updated cross-sheet references in worksheet, %1 changes made")
                  .arg(replacements.size()));
    }
    
    return hasChanges;
}

bool CrossSheetNavigator::hasCircularCrossSheetDependencies(const QString& sheetName) const
{
    return detectSimpleCircularReferences(sheetName);
}

bool CrossSheetNavigator::hasCrossSheetReferencesToSheet(WorksheetWidget* worksheet, const QString& sheetName) const
{
    if (!worksheet || sheetName.isEmpty()) {
        return false;
    }
    
    QString content = worksheet->getContent();
    QRegularExpression pattern(
        QString(R"(\bS\.%1\.LN\d+\b)").arg(QRegularExpression::escape(sheetName)),
        QRegularExpression::CaseInsensitiveOption
    );
    
    return pattern.match(content).hasMatch();
}

int CrossSheetNavigator::calculateNewLineNumber(int originalLine, const QList<LineChange>& changes) const
{
    int newLine = originalLine;
    
    for (const LineChange& change : changes) {
        if (change.type == LineChange::Insertion && change.startLine <= originalLine) {
            // Line inserted at or before this line, shift down
            newLine++;
        } else if (change.type == LineChange::Deletion && change.startLine <= originalLine) {
            // Line deleted at or before this line, shift up
            newLine = qMax(1, newLine - 1);
        }
    }
    
    return newLine;
}

QSet<QString> CrossSheetNavigator::getReferencedSheets(const QString& sheetName) const
{
    WorksheetWidget* worksheet = m_tabManager->getWorksheetByName(sheetName);
    if (!worksheet) {
        return QSet<QString>();
    }
    
    return extractReferencedSheetsFromContent(worksheet->getContent());
}

void CrossSheetNavigator::onLineNumberingChanged(const QString& sheetName, const QList<LineChange>& changes)
{
    if (sheetName.isEmpty() || changes.isEmpty()) {
        return;
    }
    
    LOG_DEBUG(QString("CrossSheetNavigator: Processing line number changes for sheet '%1', %2 changes")
              .arg(sheetName).arg(changes.size()));
    
    // Update cross-sheet references in all other worksheets
    int tabCount = m_tabManager->getTabCount();
    for (int i = 0; i < tabCount; ++i) {
        WorksheetWidget* worksheet = m_tabManager->getWorksheetWidget(i);
        QString currentSheetName = m_tabManager->getTabName(i);
        
        if (worksheet && currentSheetName != sheetName) {
            updateCrossSheetReferences(worksheet, sheetName, changes);
        }
    }
    
    // Trigger recalculation of affected worksheets
    triggerCrossSheetRecalculation();
}

void CrossSheetNavigator::onValuesChanged(const QString& sheetName)
{
    // When values change in a sheet, we may need to recalculate dependent sheets
    QSet<QString> referencedSheets = getReferencedSheets(sheetName);
    
    if (!referencedSheets.isEmpty()) {
        LOG_DEBUG(QString("CrossSheetNavigator: Values changed in sheet '%1', triggering recalculation")
                  .arg(sheetName));
        triggerCrossSheetRecalculation();
    }
}

void CrossSheetNavigator::performNavigation(const QString& sheetName, int lineNumber, int cursorPosition)
{
    // Find the target sheet
    WorksheetWidget* targetSheet = m_tabManager->getWorksheetByName(sheetName);
    if (!targetSheet) {
        LOG_DEBUG("CrossSheetNavigator: Target sheet not found: " + sheetName);
        return;
    }
    
    // Switch to the target sheet's tab
    for (int i = 0; i < m_tabManager->getTabCount(); ++i) {
        if (m_tabManager->getTabName(i).compare(sheetName, Qt::CaseInsensitive) == 0) {
            m_tabManager->navigateToTab(i);
            break;
        }
    }
    
    // Navigate to the specific line and cursor position
    ExpressionEditor* editor = targetSheet->getEditor();
    if (editor && lineNumber > 0) {
        QTextCursor cursor = editor->textCursor();
        
        // Move to the specified line
        cursor.movePosition(QTextCursor::Start);
        for (int i = 1; i < lineNumber && cursor.movePosition(QTextCursor::NextBlock); ++i) {
            // Move to target line
        }
        
        // Set cursor position if specified
        if (cursorPosition >= 0) {
            int blockStart = cursor.position();
            int targetPosition = blockStart + cursorPosition;
            int maxPosition = cursor.block().position() + cursor.block().length() - 1;
            cursor.setPosition(qMin(targetPosition, maxPosition));
        }
        
        editor->setTextCursor(cursor);
        editor->setFocus();
    }
    
    // Highlight incoming cross-sheet references
    highlightIncomingCrossSheetReferences(targetSheet);
    
    emit navigationRequested(sheetName, lineNumber, cursorPosition);
}

void CrossSheetNavigator::highlightIncomingCrossSheetReferences(WorksheetWidget* targetSheet)
{
    if (!targetSheet) return;
    
    // This is a placeholder for highlighting functionality
    // The actual implementation would depend on the highlighting system
    LOG_DEBUG("CrossSheetNavigator: Highlighting incoming cross-sheet references (placeholder)");
}

bool CrossSheetNavigator::detectSimpleCircularReferences(const QString& startSheet) const
{
    QSet<QString> visited;
    QSet<QString> currentPath;
    
    return detectCircularReferencesRecursive(startSheet, visited, currentPath);
}

bool CrossSheetNavigator::detectCircularReferencesRecursive(const QString& sheetName, 
                                                          QSet<QString>& visited, 
                                                          QSet<QString>& currentPath) const
{
    if (currentPath.contains(sheetName)) {
        LOG_DEBUG(QString("CrossSheetNavigator: Circular dependency detected involving sheet '%1'")
                  .arg(sheetName));
        return true; // Circular dependency found
    }
    
    if (visited.contains(sheetName)) {
        return false; // Already processed this path
    }
    
    visited.insert(sheetName);
    currentPath.insert(sheetName);
    
    // Get all sheets referenced by this sheet
    QSet<QString> referencedSheets = getReferencedSheets(sheetName);
    
    for (const QString& referencedSheet : referencedSheets) {
        if (detectCircularReferencesRecursive(referencedSheet, visited, currentPath)) {
            return true;
        }
    }
    
    currentPath.remove(sheetName);
    return false;
}

QSet<QString> CrossSheetNavigator::extractReferencedSheetsFromContent(const QString& content) const
{
    QSet<QString> referencedSheets;
    
    // Pattern to match cross-sheet references: S.SheetName.LN#
    QRegularExpression pattern(R"(\bS\.([^.\s]+)\.LN\d+\b)", QRegularExpression::CaseInsensitiveOption);
    
    QRegularExpressionMatchIterator iterator = pattern.globalMatch(content);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString sheetName = match.captured(1);
        if (!sheetName.isEmpty()) {
            referencedSheets.insert(sheetName);
        }
    }
    
    return referencedSheets;
}