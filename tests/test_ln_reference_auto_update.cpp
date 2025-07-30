#include <QTest>
#include <QStringList>
#include "LineChangeDetector.h"
#include "ReferenceUpdateEngine.h"
#include "LNReferenceAutoUpdater.h"

/**
 * Test suite for LN Reference Auto-Update System
 */
class TestLNReferenceAutoUpdate : public QObject
{
    Q_OBJECT

private slots:
    void testLineChangeDetection();
    void testLNReferenceUpdate();
    void testStatisticalRangeUpdate();
    void testComplexScenarios();
    void testEdgeCases();

private:
    void verifyLineChange(const LineChange &change, LineChange::Type expectedType, 
                         int expectedStart, int expectedCount);
};

void TestLNReferenceAutoUpdate::testLineChangeDetection()
{
    LineChangeDetector detector;
    
    // Test 1: Line insertion
    QStringList oldLines = {"1 + 1", "2 + 2", "3 + 3"};
    QStringList newLines = {"1 + 1", "NEW LINE", "2 + 2", "3 + 3"};
    
    QList<LineChange> changes = detector.detectChanges(oldLines, newLines);
    QCOMPARE(changes.size(), 1);
    verifyLineChange(changes[0], LineChange::Insertion, 2, 1);
    
    // Test 2: Line deletion
    oldLines = {"1 + 1", "2 + 2", "3 + 3"};
    newLines = {"1 + 1", "3 + 3"};
    
    changes = detector.detectChanges(oldLines, newLines);
    QCOMPARE(changes.size(), 1);
    verifyLineChange(changes[0], LineChange::Deletion, 2, 1);
    
    // Test 3: Multiple insertions
    oldLines = {"1 + 1", "2 + 2"};
    newLines = {"1 + 1", "NEW1", "NEW2", "2 + 2"};
    
    changes = detector.detectChanges(oldLines, newLines);
    QCOMPARE(changes.size(), 1);
    verifyLineChange(changes[0], LineChange::Insertion, 2, 2);
}

void TestLNReferenceAutoUpdate::testLNReferenceUpdate()
{
    ReferenceUpdateEngine engine;
    
    // Test 1: Simple LN reference update after insertion
    QStringList content = {"1 + 1", "LN1 + 5", "LN2 * 2"};
    
    LineChange insertion(LineChange::Insertion, 1, 1);  // Insert at line 1
    QList<LineChange> changes = {insertion};
    
    bool updated = engine.updateReferences(content, changes);
    QVERIFY(updated);
    
    // After inserting at line 1, LN1 should become LN2, LN2 should become LN3
    QCOMPARE(content[1], QString("LN2 + 5"));
    QCOMPARE(content[2], QString("LN3 * 2"));
    
    // Test 2: LN reference update after deletion
    content = {"1 + 1", "2 + 2", "LN1 + LN2"};
    
    LineChange deletion(LineChange::Deletion, 1, 1);  // Delete line 1
    changes = {deletion};
    
    updated = engine.updateReferences(content, changes);
    QVERIFY(updated);
    
    // After deleting line 1, LN1 becomes invalid (0), LN2 becomes LN1
    QCOMPARE(content[2], QString("0 + LN1"));
}

void TestLNReferenceAutoUpdate::testStatisticalRangeUpdate()
{
    ReferenceUpdateEngine engine;
    
    // Test 1: Range notation update
    QStringList content = {"1", "2", "3", "sum(1-3)"};
    
    LineChange insertion(LineChange::Insertion, 1, 1);  // Insert at line 1
    QList<LineChange> changes = {insertion};
    
    bool updated = engine.updateReferences(content, changes);
    QVERIFY(updated);
    
    // After inserting at line 1, sum(1-3) should become sum(2-4)
    QCOMPARE(content[3], QString("sum(2-4)"));
    
    // Test 2: Comma-separated list update
    content = {"1", "2", "3", "mean(1,2,3)"};
    
    updated = engine.updateReferences(content, changes);
    QVERIFY(updated);
    
    // After inserting at line 1, mean(1,2,3) should become mean(2,3,4)
    QCOMPARE(content[3], QString("mean(2,3,4)"));
}

void TestLNReferenceAutoUpdate::testComplexScenarios()
{
    ReferenceUpdateEngine engine;
    
    // Test: Multiple references in one expression
    QStringList content = {"1", "2", "3", "LN1 + LN2 + sum(1-3) + mean(1,2)"};
    
    LineChange insertion(LineChange::Insertion, 2, 1);  // Insert at line 2
    QList<LineChange> changes = {insertion};
    
    bool updated = engine.updateReferences(content, changes);
    QVERIFY(updated);
    
    // After inserting at line 2:
    // - LN1 stays LN1 (before insertion)
    // - LN2 becomes LN3 (after insertion)
    // - sum(1-3) becomes sum(1-4) (range extends)
    // - mean(1,2) becomes mean(1,3) (second number shifts)
    QCOMPARE(content[3], QString("LN1 + LN3 + sum(1-4) + mean(1,3)"));
}

void TestLNReferenceAutoUpdate::testEdgeCases()
{
    ReferenceUpdateEngine engine;
    
    // Test 1: Self-reference (should be handled gracefully)
    QStringList content = {"LN1 + 1"};
    
    LineChange insertion(LineChange::Insertion, 1, 1);
    QList<LineChange> changes = {insertion};
    
    bool updated = engine.updateReferences(content, changes);
    QVERIFY(updated);
    
    // LN1 should become LN2 after insertion
    QCOMPARE(content[0], QString("LN2 + 1"));
    
    // Test 2: Reference to non-existent line
    content = {"LN100"};
    
    updated = engine.updateReferences(content, changes);
    QVERIFY(updated);
    
    // LN100 should become LN101 after insertion
    QCOMPARE(content[0], QString("LN101"));
    
    // Test 3: Case insensitive references
    content = {"ln1 + LN2"};
    
    updated = engine.updateReferences(content, changes);
    QVERIFY(updated);
    
    // Both should be updated while preserving case
    QCOMPARE(content[0], QString("ln2 + LN3"));
}

void TestLNReferenceAutoUpdate::verifyLineChange(const LineChange &change, LineChange::Type expectedType, 
                                               int expectedStart, int expectedCount)
{
    QCOMPARE(change.type, expectedType);
    QCOMPARE(change.startLine, expectedStart);
    QCOMPARE(change.count, expectedCount);
}

// Test main function
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    TestLNReferenceAutoUpdate test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_ln_reference_auto_update.moc"
