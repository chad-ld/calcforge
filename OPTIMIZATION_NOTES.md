# Tab Switching Performance Optimizations

## Overview
This document describes the performance optimizations implemented to reduce the delay when switching between tabs in calcforge.

## Problem
Previously, when switching tabs, the application would perform a full evaluation of the entire sheet to ensure cross-sheet references were up to date. This caused a noticeable half-second delay when switching tabs, especially on sheets with many lines.

## Solution Implementation (v3.1)

The optimization implements a smart evaluation system based on change tracking and selective evaluation:

### 1. Change Tracking System
- **`Calculator._sheet_changed_flags`**: Dictionary tracking which sheets have been modified (sheet_index -> bool)
- **`Calculator._last_active_sheet`**: Reference to the previously active sheet
- **Sheet marking**: Sheets are automatically flagged as changed when their text content is modified
- **Flag management**: Flags are properly maintained when tabs are added, removed, or reordered

### 2. Selective Tab Switching Logic
The new `on_tab_changed` method:
- Only performs evaluation if the previous sheet was actually modified
- Uses selective evaluation that targets only cross-sheet reference lines
- Automatically resets change flags after evaluation
- Maintains editor focus for smooth user experience

### 3. Cross-Sheet Selective Evaluation
- **`evaluate_cross_sheet_lines_only()`**: New method that only evaluates lines containing cross-sheet references
- **Pattern detection**: Uses regex `\b[sS]\.\w+\.[lL][nN]\d+\b` to identify cross-sheet references
- **Partial updates**: Preserves existing results for non-cross-sheet lines
- **Performance tracking**: Includes debug logging for optimization monitoring

### 4. Helper Methods
- **`_evaluate_selective_lines()`**: Core selective evaluation logic
- **`_reset_change_flag()`**: Properly resets flags after evaluation
- **`_mark_sheet_as_changed()`**: Flags sheets when content changes

## Performance Benefits

### Before Optimization:
- **Every tab switch**: Full sheet evaluation (potentially hundreds of lines)
- **Consistent delay**: ~500ms regardless of whether changes occurred
- **Unnecessary work**: Re-evaluating unchanged cross-sheet references

### After Optimization:
- **No evaluation**: When switching to a sheet where the previous sheet wasn't modified
- **Selective evaluation**: Only cross-sheet lines when previous sheet was modified  
- **Dramatic improvement**: Near-instant tab switching in most cases
- **Smart updates**: Only processes what actually needs updating

## Implementation Details

### Change Detection
```python
def on_text_potentially_changed(self):
    if current_text != self._last_text_content:
        self._mark_sheet_as_changed()  # Flag this sheet as modified
        # ... continue with normal evaluation
```

### Smart Tab Switching
```python
def on_tab_changed(self, index):
    # Only evaluate if previous sheet was changed AND current sheet has cross-sheet refs
    if previous_sheet_was_changed:
        current_sheet.evaluate_cross_sheet_lines_only()  # Selective evaluation
    else:
        # No evaluation needed - instant tab switch
```

### Cross-Sheet Detection
- Checks both original and preprocessed lines to catch all variations
- Handles case-insensitive patterns (`s.sheet.ln1` and `S.Sheet.LN1`)
- Preserves all non-cross-sheet line results

## Edge Cases Handled
- **Tab reordering**: Change flags are properly updated when tabs are moved
- **Tab deletion**: Flags are cleaned up and reindexed for remaining tabs
- **Initial load**: All loaded worksheets start with `change_needed = false`
- **New tabs**: Automatically initialized with `change_needed = false`

## Future Enhancements
- Could be further optimized by tracking which specific sheets are referenced
- Could implement dependency graphs to only update dependent sheets
- Could cache cross-sheet reference patterns to avoid regex on every evaluation

## Backward Compatibility
- Fallback to full evaluation if selective method doesn't exist
- All existing functionality preserved
- Debug logging can be disabled for production use

This optimization represents a significant improvement in user experience, eliminating the frustrating delay when navigating between sheets while maintaining full cross-sheet reference accuracy. 