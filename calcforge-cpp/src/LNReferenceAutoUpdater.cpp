#include "LNReferenceAutoUpdater.h"
#include "DependencyTracker.h"
#include "Logger.h"
#include <QRegularExpression>

LNReferenceAutoUpdater::LNReferenceAutoUpdater(QObject *parent)
    : QObject(parent)
    , m_changeDetector(std::make_unique<LineChangeDetector>())
    , m_updateEngine(std::make_unique<ReferenceUpdateEngine>())
    , m_autoUpdateEnabled(true)
{
    // Initialize stats
    m_lastStats = UpdateStats();
}

LNReferenceAutoUpdater::~LNReferenceAutoUpdater() = default;

bool LNReferenceAutoUpdater::processTextChanges(const QStringList &oldContent, 
                                               QStringList &newContent,
                                               DependencyTracker *dependencyTracker)
{
    if (!m_autoUpdateEnabled) {
        return false;
    }
    
    // Reset stats
    m_lastStats = UpdateStats();
    
    try {
        // Detect line changes
        QList<LineChange> changes = m_changeDetector->detectChanges(oldContent, newContent);
        
        if (changes.isEmpty()) {
            LOG_DEBUG("No line changes detected, skipping reference updates");
            return false;
        }
        
        m_lastStats.linesChanged = changes.size();
        
        LOG_INFO(QString("Detected %1 line changes, checking for reference updates").arg(changes.size()));
        
        // Check if any changes affect line numbering (insertions/deletions)
        bool hasLineNumberChanges = false;
        for (const LineChange &change : changes) {
            if (change.type == LineChange::Insertion || change.type == LineChange::Deletion) {
                hasLineNumberChanges = true;
                break;
            }
        }
        
        if (!hasLineNumberChanges) {
            LOG_DEBUG("No line insertions/deletions detected, no reference updates needed");
            return false;
        }
        
        // Update references in the content
        bool referencesUpdated = m_updateEngine->updateReferences(newContent, changes);
        
        if (referencesUpdated) {
            // Validate updated references
            if (!validateUpdatedReferences(newContent)) {
                m_lastStats.lastError = "Reference validation failed after update";
                emit updateError(m_lastStats.lastError);
                return false;
            }
            
            // Get affected lines for dependency tracking
            QSet<int> affectedLines = m_changeDetector->getAffectedLines(changes);
            
            // Update dependency tracking if provided
            if (dependencyTracker) {
                updateDependencyTracking(newContent, affectedLines, dependencyTracker);
            }
            
            m_lastStats.referencesUpdated = affectedLines.size();
            
            LOG_INFO(QString("Successfully updated LN references in %1 lines").arg(affectedLines.size()));
            
            // emit referencesUpdated(affectedLines, m_lastStats.referencesUpdated);
            
            return true;
        }
        
        LOG_DEBUG("No LN references needed updating");
        return false;
        
    } catch (const std::exception &e) {
        m_lastStats.lastError = QString("Exception during reference update: %1").arg(e.what());
        LOG_ERROR(m_lastStats.lastError);
        // emit updateError(m_lastStats.lastError);
        return false;
    } catch (...) {
        m_lastStats.lastError = "Unknown exception during reference update";
        LOG_ERROR(m_lastStats.lastError);
        // emit updateError(m_lastStats.lastError);
        return false;
    }
}

void LNReferenceAutoUpdater::setAutoUpdateEnabled(bool enabled)
{
    m_autoUpdateEnabled = enabled;
    LOG_INFO(QString("LN Reference Auto-Update %1").arg(enabled ? "enabled" : "disabled"));
}

bool LNReferenceAutoUpdater::isAutoUpdateEnabled() const
{
    return m_autoUpdateEnabled;
}

LNReferenceAutoUpdater::UpdateStats LNReferenceAutoUpdater::getLastUpdateStats() const
{
    return m_lastStats;
}

void LNReferenceAutoUpdater::updateDependencyTracking(const QStringList &content,
                                                     const QSet<int> &affectedLines,
                                                     DependencyTracker *dependencyTracker)
{
    if (!dependencyTracker) {
        return;
    }
    
    int dependenciesUpdated = 0;
    
    // Update dependencies for all affected lines
    for (int lineNumber : affectedLines) {
        if (lineNumber > 0 && lineNumber <= content.size()) {
            QString expression = content[lineNumber - 1];  // Convert to 0-based index
            dependencyTracker->updateLineDependencies(lineNumber, expression, content.size());
            dependenciesUpdated++;
        }
    }
    
    // Also update dependencies for lines that might reference the affected lines
    for (int i = 1; i <= content.size(); ++i) {
        if (!affectedLines.contains(i)) {
            QString expression = content[i - 1];
            
            // Check if this line references any of the affected lines
            bool referencesAffectedLine = false;
            for (int affectedLine : affectedLines) {
                QString lnRef = QString("LN%1").arg(affectedLine);
                if (expression.contains(lnRef, Qt::CaseInsensitive)) {
                    referencesAffectedLine = true;
                    break;
                }
            }
            
            if (referencesAffectedLine) {
                dependencyTracker->updateLineDependencies(i, expression, content.size());
                dependenciesUpdated++;
            }
        }
    }
    
    m_lastStats.dependenciesUpdated = dependenciesUpdated;
    
    LOG_DEBUG(QString("Updated dependencies for %1 lines").arg(dependenciesUpdated));
}

bool LNReferenceAutoUpdater::validateUpdatedReferences(const QStringList &content)
{
    // Regular expression to find LOCAL LN references (not cross-sheet references)
    // This excludes patterns like "S.SheetName.LN1" and only matches standalone "LN1"
    QRegularExpression lnRegex(R"((?<!S\.[^.]*\.)LN(\d+)\b)", QRegularExpression::CaseInsensitiveOption);

    LOG_DEBUG(QString("Validating updated references in %1 lines").arg(content.size()));

    for (int i = 0; i < content.size(); ++i) {
        const QString &line = content[i];

        LOG_DEBUG(QString("Validating line %1: '%2'").arg(i + 1).arg(line));

        QRegularExpressionMatchIterator iterator = lnRegex.globalMatch(line);
        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();
            bool ok;
            int referencedLine = match.captured(1).toInt(&ok);

            LOG_DEBUG(QString("Found LOCAL LN reference: %1 -> line %2").arg(match.captured(0)).arg(referencedLine));

            if (!ok) {
                LOG_WARNING(QString("Invalid LN reference format in line %1: %2").arg(i + 1).arg(match.captured(0)));
                continue;
            }

            // Check if referenced line exists
            if (referencedLine < 1 || referencedLine > content.size()) {
                LOG_WARNING(QString("LN reference to non-existent line %1 in line %2").arg(referencedLine).arg(i + 1));
                // This is a warning, not an error - allow references to future lines
            }

            // Check for self-reference (only for LOCAL references)
            if (referencedLine == i + 1) {
                LOG_ERROR(QString("Self-reference detected: line %1 references itself (line contains: '%2')").arg(i + 1).arg(line));
                return false;
            }
        }
    }

    LOG_DEBUG("Reference validation completed successfully");
    return true;
}
