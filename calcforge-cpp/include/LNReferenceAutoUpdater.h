#ifndef LNREFERENCEAUTOUPDATER_H
#define LNREFERENCEAUTOUPDATER_H

#include "LineChangeDetector.h"
#include "ReferenceUpdateEngine.h"
#include <QStringList>
#include <QObject>
#include <memory>

class DependencyTracker;

/**
 * Main coordinator for the LN Reference Auto-Update System
 * Integrates line change detection, reference updating, and dependency tracking
 */
class LNReferenceAutoUpdater : public QObject
{
    Q_OBJECT

public:
    explicit LNReferenceAutoUpdater(QObject *parent = nullptr);
    ~LNReferenceAutoUpdater();
    
    /**
     * Process text changes and update LN references automatically
     * @param oldContent Previous worksheet content
     * @param newContent Current worksheet content (will be modified if references are updated)
     * @param dependencyTracker Dependency tracker to update after changes
     * @return True if any references were updated
     */
    bool processTextChanges(const QStringList &oldContent, 
                           QStringList &newContent,
                           DependencyTracker *dependencyTracker = nullptr);
    
    /**
     * Enable or disable automatic reference updating
     * @param enabled True to enable auto-updating
     */
    void setAutoUpdateEnabled(bool enabled);
    
    /**
     * Check if auto-updating is enabled
     * @return True if auto-updating is enabled
     */
    bool isAutoUpdateEnabled() const;
    
    /**
     * Get statistics about the last update operation
     */
    struct UpdateStats {
        int linesChanged;
        int referencesUpdated;
        int dependenciesUpdated;
        QString lastError;
    };
    
    UpdateStats getLastUpdateStats() const;

signals:
    /**
     * Emitted when references are automatically updated
     * @param lineNumbers Lines that had references updated
     * @param updateCount Number of references updated
     */
    void referencesUpdated(const QSet<int> &lineNumbers, int updateCount);
    
    /**
     * Emitted when an error occurs during auto-update
     * @param error Error message
     */
    void updateError(const QString &error);

private:
    /**
     * Update dependency tracking after reference changes
     * @param content Updated content
     * @param affectedLines Lines that had references updated
     * @param dependencyTracker Dependency tracker to update
     */
    void updateDependencyTracking(const QStringList &content,
                                 const QSet<int> &affectedLines,
                                 DependencyTracker *dependencyTracker);
    
    /**
     * Validate that updated references are still valid
     * @param content Content with updated references
     * @return True if all references are valid
     */
    bool validateUpdatedReferences(const QStringList &content);
    
    std::unique_ptr<LineChangeDetector> m_changeDetector;
    std::unique_ptr<ReferenceUpdateEngine> m_updateEngine;
    
    bool m_autoUpdateEnabled;
    UpdateStats m_lastStats;
};

#endif // LNREFERENCEAUTOUPDATER_H
