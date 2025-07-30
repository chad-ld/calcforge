#ifndef WORKSHEETMODEL_H
#define WORKSHEETMODEL_H

#include <QObject>
#include <QString>
#include <QHash>

/**
 * WorksheetModel - Business logic and data management for worksheets
 * Part of Phase 3.2: Separate Business Logic from UI
 * 
 * Handles:
 * - Content storage and management
 * - Line value caching and retrieval
 * - Data change notifications
 * - Business logic operations
 */
class WorksheetModel : public QObject
{
    Q_OBJECT

public:
    explicit WorksheetModel(QObject *parent = nullptr);
    ~WorksheetModel();

    // Content management
    QString getContent() const;
    void setContent(const QString& content);
    bool isModified() const;
    void setModified(bool modified);

    // Line value management
    double getLineValue(int lineNumber) const;
    bool hasLineValue(int lineNumber) const;
    void setLineValue(int lineNumber, double value);
    void clearLineValue(int lineNumber);
    void clearAllLineValues();

    // Sheet identification
    QString getSheetName() const;
    void setSheetName(const QString& sheetName);

    // Cross-sheet reference support
    bool hasCrossSheetReferences() const;
    void setHasCrossSheetReferences(bool hasReferences);

signals:
    void contentChanged();
    void lineValueChanged(int lineNumber, double value);
    void modificationStateChanged(bool isModified);
    void crossSheetReferencesChanged();

private:
    QString m_content;
    bool m_isModified;
    QString m_sheetName;
    bool m_hasCrossSheetReferences;
    
    // Cache for calculated line values
    QHash<int, double> m_lineValues;
};

#endif // WORKSHEETMODEL_H
