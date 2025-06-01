# Tab Switching Performance Optimization Implementation

## Problem Statement
The current `on_tab_changed()` method calls a full `evaluate()` on every tab switch, causing a ~500ms delay when switching tabs due to processing large amounts of data unnecessarily.

## Solution Overview
Implement a smart change tracking system that only evaluates cross-sheet references when:
1. The previous sheet was actually modified 
2. The current sheet contains cross-sheet references

## Implementation Stages

### Stage 1: Basic Change Tracking System ✅ **[COMPLETED]**

**Objectives:**
- Add per-sheet change flags to Calculator
- Modify `on_tab_changed()` to be smart about evaluation 
- Track when sheet content actually changes
- Simple cross-sheet reference detection

**Components to implement:**
- [x] Add `_sheet_changed_flags = {}` to Calculator class
- [x] Add `_last_active_sheet = None` to Calculator class  
- [x] Add `has_cross_sheet_refs = False` to Worksheet class
- [x] Modify `Calculator.on_tab_changed()` for smart evaluation
- [x] Update `Worksheet.on_text_potentially_changed()` to set change flags
- [x] Add cross-sheet reference detection during evaluation
- [x] Handle tab operations (add/remove/rename) to maintain flag integrity

**Expected Performance Gain:**
- Near-instant tab switching when no changes occurred
- Selective evaluation only when actually needed

**Implementation Summary:**
- Added change tracking variables `_sheet_changed_flags` and `_last_active_sheet` to Calculator
- Modified `on_tab_changed()` to only evaluate when necessary based on change flags and cross-sheet dependencies
- Updated `on_text_potentially_changed()` to set change flags when content actually changes
- Added cross-sheet reference detection using regex pattern `\bS\.[^.]+\.LN\d+\b` during evaluation
- Handled tab add/remove/rename operations to maintain flag integrity
- Initialized change flags for both new and loaded worksheets

### Stage 2: Selective Cross-Sheet Evaluation **[PLANNED]**

**Objectives:**
- Only re-evaluate lines containing cross-sheet references
- Preserve existing results for non-cross-sheet lines
- Pattern detection for `S.SheetName.LN#` references

**Components to implement:**
- [ ] Add `evaluate_cross_sheet_lines_only()` method to Worksheet
- [ ] Implement selective line evaluation logic
- [ ] Cross-sheet pattern detection and line filtering
- [ ] Preserve non-cross-sheet results during selective evaluation

**Expected Performance Gain:**
- Even faster evaluation when cross-sheet updates are needed
- Reduced computation overhead on large sheets

### Stage 3: Dependency Graph Optimization **[FUTURE]**

**Objectives:**
- Track which sheets reference which other sheets
- Cascading updates only for actually dependent sheets
- Batch processing for multiple changes

**Components to implement:**
- [ ] Dependency mapping system
- [ ] Granular sheet-to-sheet dependency tracking
- [ ] Intelligent cascade update logic
- [ ] Batch change processing

## Implementation Notes

### Design Decisions Made:
1. **Storage Location**: Calculator level for centralized management
2. **Granularity**: Simple boolean "has cross-sheet refs" initially  
3. **Approach**: One stage at a time for stability and testing

### Key Code Locations:
- `Calculator.on_tab_changed()` - Main tab switching logic
- `Worksheet.on_text_potentially_changed()` - Change detection
- `Calculator.__init__()` - Change tracking initialization

### Performance Targets:
- **Current**: ~500ms delay on every tab switch
- **Stage 1 Goal**: <50ms when no changes, ~200ms when cross-sheet eval needed
- **Stage 2 Goal**: <20ms when no changes, <100ms for selective evaluation

## Testing Strategy

### Stage 1 Testing:
- [ ] Test tab switching with no changes (should be instant)
- [ ] Test tab switching after content changes (should evaluate)
- [ ] Test with/without cross-sheet references
- [ ] Test tab add/remove/rename operations
- [ ] Verify change flags are properly maintained

### Performance Monitoring:
- Use existing `EditorPerformanceMonitoringMixin` for timing
- Add debug logging for change flag operations
- Monitor evaluation frequency before/after optimization

## Rollback Plan
Keep backup of original `on_tab_changed()` implementation in case issues arise:
```python
# Original implementation (backup):
def on_tab_changed_original(self, index):
    if index >= 0:
        current_sheet = self.tabs.widget(index)
        if current_sheet and hasattr(current_sheet, 'evaluate'):
            self.invalidate_all_cross_sheet_caches()
            current_sheet.evaluate()
``` 