# CalcForge Performance Optimization Plan

## Executive Summary

The current CalcForge application experiences 0.5-1 second delays when modifying LN references or function expressions due to several performance bottlenecks. This document outlines a comprehensive optimization strategy broken into stages to improve responsiveness while maintaining all existing functionality.

## Root Cause Analysis

### Performance Bottlenecks Identified:

1. **Full Sheet Re-evaluation**: Every text change triggers complete sheet recalculation via `evaluate()` method
2. **Inefficient LN Reference Processing**: The `process_ln_refs()` method uses multiple regex passes and iterative replacement
3. **Cross-Sheet Cache Rebuilding**: Cross-sheet caches are rebuilt too frequently during navigation and editing
4. **Excessive Highlighting Operations**: The `highlightCurrentLine()` method processes cross-sheet references even for simple edits
5. **Timer-Based Delays**: Current 300ms evaluation timer adds perceived latency
6. **Redundant Dependency Graph Rebuilds**: The dependency graph is rebuilt on every text change

## Optimization Strategy - 5 Stages

### Stage 1: Smart Change Detection and Evaluation Debouncing
**Target**: Eliminate unnecessary full evaluations
**Risk Level**: Low
**Expected Performance Gain**: 60-70%

**Changes:**
- Implement line-level change detection instead of full-text comparison
- Add intelligent debouncing with shorter delays for simple math expressions
- Skip evaluation when only whitespace or comments change
- Cache evaluation results to avoid recomputing unchanged expressions

### Stage 2: Optimized LN Reference Processing
**Target**: Accelerate LN reference resolution
**Risk Level**: Low-Medium
**Expected Performance Gain**: 40-50%

**Changes:**
- Pre-compile regex patterns for LN reference detection
- Implement single-pass LN reference replacement instead of iterative
- Cache LN reference parsing results per line
- Use more efficient string replacement algorithms

### Stage 3: Selective Line Re-evaluation
**Target**: Only re-evaluate changed lines and their dependents
**Risk Level**: Medium
**Expected Performance Gain**: 70-80%

**Changes:**
- Build line-level dependency tracking within sheets
- Implement selective evaluation that only processes changed lines
- Create efficient line dependency graph for internal references
- Maintain separate caches for different types of expressions

### Stage 4: Cross-Sheet Optimization Refinements
**Target**: Optimize cross-sheet reference handling
**Risk Level**: Medium-High
**Expected Performance Gain**: 30-40%

**Changes:**
- Implement smarter cross-sheet cache invalidation
- Add reference counting for cross-sheet dependencies
- Optimize cross-sheet value lookup with better indexing
- Reduce frequency of dependency graph rebuilds

### Stage 5: UI and Highlighting Optimizations
**Target**: Reduce visual update overhead
**Risk Level**: Low
**Expected Performance Gain**: 20-30%

**Changes:**
- Debounce highlighting operations more aggressively
- Cache highlighting states to avoid redundant calculations
- Optimize visual updates during rapid text changes
- Implement progressive highlighting for large sheets

## Detailed Implementation Plan

### Stage 1: Smart Change Detection and Evaluation Debouncing

#### Files to Modify:
- `calcforge.py` - Worksheet class methods

#### Changes:
1. **Add line-level change detection in `on_text_potentially_changed()`**
   ```python
   def detect_changed_lines(self, old_text, new_text):
       # Compare line by line to identify actual changes
       # Return set of changed line numbers
   ```

2. **Implement smart evaluation timer in Worksheet class**
   ```python
   def start_smart_evaluation_timer(self, changed_lines):
       # Use shorter delay for simple math, longer for complex expressions
       # Skip evaluation for whitespace-only changes
   ```

3. **Add expression result caching**
   ```python
   def cache_evaluation_result(self, line_content, result):
       # Cache results by line content hash
       # Return cached result if line unchanged
   ```

#### Testing Requirements:
- Verify that whitespace changes don't trigger evaluation
- Confirm simple math expressions evaluate quickly
- Test that complex expressions still work correctly

### Stage 2: Optimized LN Reference Processing

#### Files to Modify:
- `calcforge.py` - EditorCrossSheetMixin class

#### Changes:
1. **Pre-compile regex patterns in class initialization**
   ```python
   def __init__(self):
       self._ln_pattern = re.compile(r'\bLN(\d+)\b', re.IGNORECASE)
       self._cross_sheet_pattern = re.compile(r'\bS\.([^.]+)\.LN(\d+)\b', re.IGNORECASE)
   ```

2. **Optimize `process_ln_refs()` with single-pass replacement**
   ```python
   def process_ln_refs_optimized(self, expr):
       # Single regex pass with efficient callback
       # Use direct string building instead of iterative replacement
   ```

3. **Add LN reference parsing cache**
   ```python
   def get_cached_ln_refs(self, line_content):
       # Cache parsed LN references per line
       # Invalidate cache only when line changes
   ```

#### Testing Requirements:
- Verify all LN reference formats still work
- Test cross-sheet references function correctly
- Confirm performance improvement in reference-heavy sheets

### Stage 3: Selective Line Re-evaluation

#### Files to Modify:
- `calcforge.py` - Worksheet class evaluation methods

#### Changes:
1. **Build internal line dependency graph**
   ```python
   def build_line_dependencies(self):
       # Track which lines reference which other lines
       # Update incrementally as lines change
   ```

2. **Implement selective evaluation method**
   ```python
   def evaluate_changed_lines_only(self, changed_lines):
       # Only re-evaluate changed lines and their dependents
       # Preserve results for unchanged lines
   ```

3. **Add dependency-aware result caching**
   ```python
   def update_dependent_lines(self, changed_line, new_value):
       # Efficiently update lines that depend on changed line
       # Minimize cascading recalculations
   ```

#### Testing Requirements:
- Verify dependent lines update correctly
- Test complex dependency chains
- Confirm no regression in calculation accuracy

### Stage 4: Cross-Sheet Optimization Refinements

#### Files to Modify:
- `calcforge.py` - Calculator class and cross-sheet methods

#### Changes:
1. **Implement smart cache invalidation**
   ```python
   def invalidate_cross_sheet_cache_selectively(self, changed_sheet, changed_lines):
       # Only invalidate relevant cache entries
       # Use reference counting for cache management
   ```

2. **Optimize dependency graph updates**
   ```python
   def update_dependency_graph_incrementally(self, sheet_idx, old_refs, new_refs):
       # Update graph incrementally instead of full rebuild
       # Track reference additions/removals efficiently
   ```

#### Testing Requirements:
- Verify cross-sheet references update correctly
- Test dependency graph accuracy
- Confirm cache consistency

### Stage 5: UI and Highlighting Optimizations

#### Files to Modify:
- `calcforge.py` - EditorLineManagementMixin methods

#### Changes:
1. **Optimize highlighting debouncing**
   ```python
   def setup_smart_highlighting_timer(self):
       # Longer delays during rapid text changes
       # Immediate highlighting for navigation-only changes
   ```

2. **Cache highlighting calculations**
   ```python
   def cache_line_highlights(self, line_number, highlights):
       # Cache highlighting results per line
       # Invalidate only when line content changes
   ```

#### Testing Requirements:
- Verify highlighting still works correctly
- Test performance during rapid typing
- Confirm visual consistency

## Implementation Timeline

### Phase 1 (Stage 1): 
**Duration**: 1-2 sessions
**Focus**: Smart change detection and debouncing
**Risk**: Low
**Benefits**: Immediate 60-70% performance improvement

### Phase 2 (Stage 2):
**Duration**: 1-2 sessions  
**Focus**: LN reference processing optimization
**Risk**: Low-Medium
**Benefits**: Additional 40-50% improvement for reference-heavy operations

### Phase 3 (Stage 3):
**Duration**: 2-3 sessions
**Focus**: Selective line evaluation
**Risk**: Medium
**Benefits**: 70-80% improvement for large sheets

### Phase 4 (Stage 4):
**Duration**: 1-2 sessions
**Focus**: Cross-sheet optimizations
**Risk**: Medium-High
**Benefits**: 30-40% improvement for multi-sheet operations

### Phase 5 (Stage 5):
**Duration**: 1 session
**Focus**: UI optimizations
**Risk**: Low
**Benefits**: 20-30% improvement in perceived responsiveness

## Testing Strategy

### After Each Stage:
1. **Functional Testing**: Verify all existing functionality works
2. **Performance Testing**: Measure evaluation times before/after
3. **Regression Testing**: Test complex formulas and cross-sheet references
4. **User Experience Testing**: Verify responsiveness improvements

### Test Cases:
1. Simple math expressions (1+2, 100*5)
2. LN references (LN1 + LN2) 
3. Cross-sheet references (S.Sheet2.LN1)
4. Complex nested expressions
5. Large sheets (100+ lines)
6. Multiple sheets with cross-references

## Expected Results

### Performance Improvements:
- **Stage 1**: 60-70% faster for typical operations
- **Stage 2**: Additional 40-50% for LN-heavy expressions  
- **Stage 3**: Additional 70-80% for large sheets
- **Stage 4**: Additional 30-40% for cross-sheet operations
- **Stage 5**: Additional 20-30% perceived improvement

### Overall Target:
- **Current**: 500-1000ms delays for LN/function changes
- **After optimization**: 50-100ms delays (10x improvement)
- **Simple math**: Near-instant (<20ms)

## Risk Mitigation

### Code Safety:
- Implement each stage as separate, testable units
- Maintain full backward compatibility
- Add comprehensive logging for debugging
- Create rollback procedures for each stage

### Testing Safety:
- Test each stage thoroughly before proceeding
- Maintain test cases for all functionality
- Verify calculations remain accurate
- Check edge cases and error handling

## Success Criteria

### Performance Metrics:
- [ ] LN reference modifications complete in <100ms
- [ ] Function expression changes complete in <100ms  
- [ ] Simple math expressions complete in <20ms
- [ ] Cross-sheet updates complete in <150ms
- [ ] Large sheet operations (100+ lines) complete in <200ms

### Functional Requirements:
- [ ] All existing calculations work correctly
- [ ] Cross-sheet references function properly
- [ ] Error handling remains intact
- [ ] UI responsiveness improved
- [ ] No regression in any existing features

## Implementation Process

### Before Starting Each Stage:
1. Create backup of current working version
2. Review the specific stage implementation plan
3. Set up performance measurement baseline
4. Prepare test cases for the stage

### During Implementation:
1. Make incremental changes with frequent testing
2. Maintain detailed notes on changes made
3. Test each modification before proceeding
4. Monitor for any regressions or issues

### After Completing Each Stage:
1. Run full test suite
2. Measure performance improvements
3. Document any issues or learnings
4. Get user approval before proceeding to next stage

This optimization plan provides a structured approach to dramatically improve CalcForge performance while maintaining code stability and functionality. Each stage builds upon the previous improvements and can be implemented and tested independently. 