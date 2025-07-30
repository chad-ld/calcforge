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
**Status**: ✅ COMPLETED

**Implementation Notes:**
- Pre-compiled regex patterns added to FormulaEditor.__init__()
- Optimized process_ln_refs() method with single-pass replacement
- Added LN reference result caching with automatic cache management
- Cache cleared on each evaluation cycle to ensure consistency
- Fixed tab switching highlighting issue by resetting rapid navigation state
- Added forced highlighting update after tab switch and evaluation

**Changes:**
- Pre-compile regex patterns for LN reference detection
- Implement single-pass LN reference replacement instead of iterative
- Cache LN reference parsing results per line
- Use more efficient string replacement algorithms

### Stage 3: Selective Line Re-evaluation

#### Overview:
**Target**: Only re-evaluate changed lines and their dependents
**Risk Level**: Medium
**Expected Performance Gain**: 70-80%
**Duration**: 2-3 sessions broken into 3 sub-stages

#### Sub-Stage Breakdown:

#### Stage 3.1: Line Dependency Graph Infrastructure
**Focus**: Build the foundation for tracking line-to-line dependencies within sheets
**Duration**: 1 session
**Risk Level**: Low-Medium

**Files to Modify:**
- `calcforge.py` - Worksheet class (new dependency tracking methods)

**Implementation Details:**
1. **Create dependency graph data structures**
   ```python
   def __init__(self):
       # Add to Worksheet.__init__()
       self.line_dependencies = {}  # {line_num: set of lines that depend on it}
       self.line_references = {}    # {line_num: set of lines it references}
       self.dependency_graph_cache = {}  # Cache for expensive lookups
   ```

2. **Implement dependency analysis method**
   ```python
   def build_line_dependencies(self):
       """Analyze all lines to build dependency graph"""
       # Parse each line to find LN references
       # Build bidirectional dependency mapping
       # Track internal sheet references only (not cross-sheet)
   ```

3. **Add incremental dependency updates**
   ```python
   def update_line_dependencies(self, line_number, old_content, new_content):
       """Update dependencies when a single line changes"""
       # Remove old dependencies for this line
       # Parse new content for LN references
       # Update bidirectional mappings efficiently
   ```

4. **Create dependency lookup helpers**
   ```python
   def get_dependent_lines(self, line_number):
       """Get all lines that depend on the given line"""
       # Return cached result if available
       # Traverse dependency graph efficiently
   
   def get_dependency_chain(self, changed_lines):
       """Get complete chain of lines affected by changes"""
       # Use breadth-first search to find all dependents
       # Avoid infinite loops in circular dependencies
   ```

**Testing Requirements:**
- Verify dependency graph builds correctly for various LN reference patterns
- Test incremental updates when lines change
- Confirm bidirectional mappings are consistent
- Test performance with large sheets (100+ lines)

#### Stage 3.2: Selective Evaluation Engine
**Focus**: Implement the core selective evaluation logic
**Duration**: 1 session
**Risk Level**: Medium

**Files to Modify:**
- `calcforge.py` - Worksheet class evaluation methods

**Implementation Details:**
1. **Create selective evaluation method**
   ```python
   def evaluate_changed_lines_only(self, changed_lines):
       """Only re-evaluate changed lines and their dependents"""
       # Get complete dependency chain
       # Preserve results for unchanged lines
       # Evaluate in dependency order to avoid multiple passes
       # Update only affected result displays
   ```

2. **Implement dependency-aware evaluation order**
   ```python
   def get_evaluation_order(self, lines_to_evaluate):
       """Return lines in dependency-safe evaluation order"""
       # Topological sort of dependency graph
       # Handle circular dependencies gracefully
       # Ensure prerequisites are evaluated first
   ```

3. **Add result preservation mechanism**
   ```python
   def preserve_unchanged_results(self, all_lines, lines_to_evaluate):
       """Keep existing results for lines not being re-evaluated"""
       # Copy existing results for unchanged lines
       # Maintain line-to-result mappings
       # Handle result formatting consistency
   ```

4. **Integrate with existing evaluation pipeline**
   ```python
   def evaluate(self):
       """Enhanced evaluate method with selective option"""
       # Detect if selective evaluation is beneficial
       # Fall back to full evaluation for major changes
       # Maintain compatibility with existing code
   ```

**Testing Requirements:**
- Verify selective evaluation produces same results as full evaluation
- Test dependency order calculations
- Confirm unchanged lines preserve their results
- Test integration with existing evaluation triggers

#### Stage 3.3: Dependency-Aware Caching System
**Focus**: Optimize caching and result propagation
**Duration**: 1 session
**Risk Level**: Low-Medium

**Files to Modify:**
- `calcforge.py` - Worksheet class caching methods

**Implementation Details:**
1. **Implement dependency-aware result caching**
   ```python
   def cache_line_result_with_dependencies(self, line_number, content, result, dependencies):
       """Cache result with its dependency fingerprint"""
       # Store result with dependency hash
       # Track which lines this result depends on
       # Enable efficient cache invalidation
   ```

2. **Create efficient update propagation**
   ```python
   def update_dependent_lines(self, changed_line, new_value):
       """Efficiently propagate changes to dependent lines"""
       # Update only lines that reference the changed line
       # Use cached dependency mappings
       # Minimize cascading recalculations
       # Batch updates for multiple dependent lines
   ```

3. **Add smart cache invalidation**
   ```python
   def invalidate_dependency_cache(self, changed_lines):
       """Invalidate only relevant cache entries"""
       # Find all cache entries affected by changes
       # Use dependency graph to minimize invalidation
       # Preserve unaffected cached results
   ```

4. **Optimize memory usage and performance**
   ```python
   def cleanup_dependency_caches(self):
       """Clean up unused cache entries and optimize memory"""
       # Remove stale cache entries
       # Compress dependency graph storage
       # Balance memory usage vs. lookup speed
   ```

**Testing Requirements:**
- Verify cache invalidation works correctly
- Test result propagation accuracy
- Confirm memory usage doesn't grow unbounded
- Test performance with complex dependency chains

#### Overall Stage 3 Testing Requirements:
- Verify dependent lines update correctly across all sub-stages
- Test complex dependency chains and circular references
- Confirm no regression in calculation accuracy
- Test performance improvement with large sheets
- Verify integration with existing cross-sheet functionality
- Test edge cases: empty lines, comments, malformed expressions

#### Expected Performance Impact:
- **Stage 3.1**: Infrastructure setup, minimal performance change
- **Stage 3.2**: Major performance improvement (50-60% of target gain)
- **Stage 3.3**: Additional optimization (remaining 10-20% of target gain)
- **Combined**: 70-80% performance improvement for large sheets with dependencies

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
**Duration**: 2-3 sessions (broken into 3 sub-stages)
**Focus**: Selective line evaluation
**Risk**: Medium
**Benefits**: 70-80% improvement for large sheets

#### Sub-Stage Breakdown:
- **Stage 3.1** (1 session): Line Dependency Graph Infrastructure - Low-Medium risk
- **Stage 3.2** (1 session): Selective Evaluation Engine - Medium risk  
- **Stage 3.3** (1 session): Dependency-Aware Caching System - Low-Medium risk

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