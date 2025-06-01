# Item 10: FormulaEditor Class Refactoring Plan
*Generated: January 1, 2025*
*Updated: January 1, 2025 - Stage 1 Complete*

## 📋 **Overview**

**Target**: FormulaEditor class refactoring (Item 10 from calcforge_refactoring_report.md)
**Current State**: Lines 1132-3283 = **2,151 lines** with too many responsibilities
**Goal**: Extract logical groupings into mixins/separate classes for better maintainability

## 🎯 **Problem Statement**

The FormulaEditor class is **massive** with over 2,000 lines and handles multiple responsibilities:
- Text editing and document management
- Auto-completion functionality  
- Cross-sheet reference handling
- Performance monitoring and debugging
- Event handling (mouse, keyboard, resize)
- Text selection and navigation
- Line number management
- Expression evaluation
- Unit conversion handling

This violates the Single Responsibility Principle and makes the code:
- ❌ Hard to maintain and debug
- ❌ Difficult to unit test
- ❌ Prone to merge conflicts
- ❌ Complex to understand and modify

## 📈 **Progress Tracking**

### ✅ **Stage 1: Extract Auto-Completion Functionality** - **COMPLETE**
**Completion Date**: January 1, 2025
**Methods Extracted**: `setup_autocompletion()`, `get_word_under_cursor()`, `get_completions()`, `complete_text()`, `show_completion_popup()`

**Implementation Details**:
- Created `EditorAutoCompletionMixin` class with all auto-completion methods
- Updated `FormulaEditor` to inherit from `EditorAutoCompletionMixin` 
- Successfully extracted 5 auto-completion methods (~370 lines)
- All auto-completion functionality preserved and tested working
- Reduced FormulaEditor responsibility and improved code organization

**Benefits Achieved**:
- ✅ Separated auto-completion concerns from main editor logic
- ✅ Made auto-completion functionality reusable via mixin pattern
- ✅ Improved testability by isolating completion logic
- ✅ Reduced FormulaEditor class complexity

### **✅ Stage 2: Extract Performance Monitoring**
**Target Lines**: ~3078-3123 (Performance analysis and debugging)
**Estimated Effort**: 2-3 hours
**Status**: ✅ **COMPLETE** - Successfully extracted into `EditorPerformanceMonitoringMixin`

#### **Methods Extracted**:
```python
def _log_perf(self, method_name, start_time=None)  # Line 3078 → EditorPerformanceMonitoringMixin
def _check_scroll_sync_issue(self)                 # Line 3105 → EditorPerformanceMonitoringMixin  
def print_perf_summary(self)                       # Line 3123 → EditorPerformanceMonitoringMixin
```

#### **Completed**: `EditorPerformanceMonitoringMixin`
- ✅ All performance monitoring methods extracted
- ✅ Debug functionality isolated 
- ✅ Optional performance tracking
- ✅ Application tested and working

---

### **✅ Stage 3: Extract Cross-Sheet Reference Handling**  
**Target Lines**: ~1969-2060, ~3132-3175 (Cross-sheet and reference functionality)
**Estimated Effort**: 4-5 hours
**Status**: ✅ **COMPLETE** - Successfully extracted into `EditorCrossSheetMixin`

#### **Methods Extracted**:
```python
def get_sheet_value(self, sheet_name, ln_number)  # Line 1969 → EditorCrossSheetMixin
def get_numeric_value(self, value)                # Line 1997 → EditorCrossSheetMixin
def process_ln_refs(self, expr)                   # Line 2010 → EditorCrossSheetMixin
def build_cross_sheet_cache(self)                 # Line 3132 → EditorCrossSheetMixin
def clear_highlighted_sheets_only(self)           # Line 3156 → EditorCrossSheetMixin
```

#### **Related Data**:
- `self._cross_sheet_cache`
- `self._highlighted_sheets`
- Cross-sheet lookup optimization

#### **Completed**: `EditorCrossSheetMixin`
- ✅ All cross-sheet reference logic isolated
- ✅ Performance caching maintained
- ✅ Sheet highlighting functionality preserved
- ✅ Application tested and working

---

### **✅ Stage 4: Extract Text Selection and Navigation**
**Target Lines**: ~2452-2690 (Text selection, navigation, and manipulation)
**Estimated Effort**: 4-5 hours
**Status**: ✅ **COMPLETE** - Successfully extracted into `EditorTextSelectionMixin`

#### **Methods Extracted**:
```python
def select_number_token(self, forward=True)         # Line 2452 → EditorTextSelectionMixin
def expand_selection_with_parens(self)              # Line 2499 → EditorTextSelectionMixin
def find_arithmetic_expression(self, text, pos)     # Line 2589 → EditorTextSelectionMixin
def select_entire_line(self)                        # Line 2604 → EditorTextSelectionMixin
def select_nearest_word_or_number(self)             # Line 2611 → EditorTextSelectionMixin
def get_selected_text(self)                         # Line 2676 → EditorTextSelectionMixin
```

#### **Related Data**:
- Text cursor manipulation
- Selection state management
- Word/number detection logic

#### **Completed**: `EditorTextSelectionMixin`
- ✅ All text selection and navigation logic isolated
- ✅ Smart parentheses-based selection expansion
- ✅ Number token selection functionality preserved
- ✅ Application tested and working

---

### **⏳ Stage 5: Extract Line Management and Highlighting**
**Target Methods**: `assign_stable_ids()`, `reassign_line_ids()`, `highlight_expression()`, `clear_expression_highlight()`, `highlightCurrentLine()`, `_do_highlight_current_line()`, `_do_basic_highlight_only()`
**Estimated Effort**: 3-4 hours
**Target Class**: `EditorLineManagementMixin`

## 🎯 **Refactoring Stages**

### **Stage 1: Extract Auto-Completion Functionality** ✅ **COMPLETE**

**Target Methods**:
- `setup_autocompletion()` - Sets up completion data structures
- `get_word_under_cursor()` - Extracts word at cursor with currency handling
- `get_completions(prefix)` - Generates context-aware completion suggestions  
- `complete_text(item)` - Applies selected completion to text
- `show_completion_popup()` - Displays completion popup with positioning

**Benefits**:
- ✅ Separates auto-completion concerns from main editor logic
- ✅ Makes auto-completion functionality reusable via mixin pattern
- ✅ Improves testability by isolating completion logic
- ✅ Reduces FormulaEditor class complexity

**Implementation**: Create `EditorAutoCompletionMixin` class

## 🚀 **5-Stage Refactoring Plan**

### **✅ Stage 1: Extract Auto-Completion Functionality**
**Target Lines**: ~1290-1635 (Auto-completion related methods)
**Estimated Effort**: 3-4 hours

#### **Methods to Extract**:
```python
def setup_autocompletion(self)           # Line 1290
def get_word_under_cursor(self)          # Line 1397
def get_completions(self, prefix)        # Line 1444
def complete_text(self, item=None)       # Line 1634
def show_completion_popup(self)          # Line 2871
```

#### **Related Data**:
- `self.completion_prefix`
- `self.completion_list`
- `self.base_completions`
- `self.statistical_range_options`
- `self.tc_options`, `self.ar_options`, etc.

#### **Create**: `EditorAutoCompletionMixin`
```python
class EditorAutoCompletionMixin:
    """Handles all auto-completion functionality for the formula editor"""
    
    def setup_autocompletion(self):
        # Setup completion data structures
        
    def get_word_under_cursor(self):
        # Extract word at cursor position
        
    def get_completions(self, prefix):
        # Generate completion suggestions
        
    def complete_text(self, item=None):
        # Apply selected completion
        
    def show_completion_popup(self):
        # Display completion popup
```

#### **Benefits**:
- ✅ Isolate all auto-completion logic 
- ✅ Easier to add new completion types
- ✅ Testable completion algorithms
- ✅ Optional feature (can be disabled)

---

### **⏳ Stage 4: Extract Event Handling** 
**Target Lines**: ~1948-2855 (Mouse, keyboard, and resize events)
**Estimated Effort**: 4-5 hours

#### **Methods to Extract**:
```python
def eventFilter(self, obj, event)                  # Line 1948
def keyPressEvent(self, event)                     # Line 2493
def wheelEvent(self, event)                        # Line 2818
def mousePressEvent(self, event)                   # Line 2829
def mouseReleaseEvent(self, event)                 # Line 2839
def resizeEvent(self, event)                       # Line 2018
def change_font_size(self, delta)                  # Line 2845
def reset_font_size(self)                          # Line 2852
def update_fonts(self)                             # Line 2857
def paintEvent(self, event)                        # Line 2955
```

#### **Related Data**:
- `self.key_filter`
- `self.current_font_size`
- `self.default_font_size`
- Font size management state

#### **Create**: `EditorEventHandlerMixin`
```python
class EditorEventHandlerMixin:
    """Handles all editor events (mouse, keyboard, resize, etc.)"""
    
    def keyPressEvent(self, event):
        # Handle keyboard input
        
    def mousePressEvent(self, event):
        # Handle mouse clicks
        
    def wheelEvent(self, event):
        # Handle mouse wheel (font sizing)
        
    def resizeEvent(self, event):
        # Handle window resize
```

#### **Benefits**:
- ✅ Centralize all event handling logic
- ✅ Easier to test event responses
- ✅ Isolated input/output handling
- ✅ Simplified event debugging

---

### **⏳ Stage 5: Extract Text Selection and Navigation**
**Target Lines**: ~2259-2483 (Text selection, navigation, and manipulation)
**Estimated Effort**: 3-4 hours

#### **Methods to Extract**:
```python
def select_number_token(self, forward=True)        # Line 2259
def expand_selection_with_parens(self)             # Line 2306
def find_arithmetic_expression(self, text, pos)    # Line 2396
def select_entire_line(self)                       # Line 2411
def select_nearest_word_or_number(self)            # Line 2418
def get_selected_text(self)                        # Line 2483
def calculate_subexpression(self, expr)            # Line 1749
def find_operator_results(self, text, block_position) # Line 1775
```

#### **Create**: `EditorSelectionMixin`
```python
class EditorSelectionMixin:
    """Handles text selection, navigation, and manipulation"""
    
    def select_number_token(self, forward=True):
        # Select next/previous number
        
    def expand_selection_with_parens(self):
        # Smart parentheses selection
        
    def select_entire_line(self):
        # Select full line
        
    def find_arithmetic_expression(self, text, pos):
        # Find expression boundaries
```

#### **Benefits**:
- ✅ Isolate text manipulation logic
- ✅ Reusable selection algorithms
- ✅ Easier to add new selection modes
- ✅ Testable navigation features

---

## 🏗️ **Final Architecture**

### **After Refactoring**:
```python
class FormulaEditor(QPlainTextEdit, 
                    EditorAutoCompletionMixin,
                    CrossSheetReferenceMixin, 
                    EditorPerformanceMixin,
                    EditorEventHandlerMixin,
                    EditorSelectionMixin):
    
    def __init__(self, parent):
        """Core initialization only (~100-150 lines)"""
        super().__init__(parent)
        self.parent = parent
        
        # Basic editor setup
        self._setup_editor_appearance()
        self._setup_line_numbering()
        self._setup_document_management()
        
        # Initialize mixins
        self.setup_autocompletion()
        self._setup_performance_monitoring()
        # etc.
    
    # Core editor methods only (~300-400 lines):
    def get_calculator(self):
        """Get parent Calculator instance"""
        
    def assign_stable_ids(self):
        """Assign stable IDs to lines"""
        
    def reassign_line_ids(self):
        """Reassign line IDs after changes"""
        
    def get_numeric_value(self, value):
        """Extract numeric value from result"""
        
    def updateLineNumberArea(self, rect, dy):
        """Update line number area"""
        
    def lineNumberAreaPaintEvent(self, event):
        """Paint line numbers"""
```

### **Line Count Reduction**:
- **Before**: 2,151 lines in one massive class
- **After**: ~400-500 core lines + 5 focused mixins (300-400 lines each)
- **Maintainability**: 📈 Dramatically improved
- **Testability**: 📈 Individual mixins can be unit tested
- **Reusability**: 📈 Mixins can be used in other components

## 🔄 **Implementation Strategy**

### **Development Approach**:
1. **Incremental Refactoring**: One stage at a time
2. **Test-Driven**: Verify functionality after each stage  
3. **Git Tracking**: Commit after each successful stage
4. **Backward Compatibility**: All existing functionality preserved
5. **Performance Validation**: Ensure no performance regression

### **Risk Mitigation**:
- ✅ Create backup branches before each stage
- ✅ Comprehensive testing after each extraction
- ✅ Rollback plan if issues arise
- ✅ User acceptance testing before proceeding

### **Success Criteria**:
- ✅ All functionality preserved
- ✅ No performance regression
- ✅ Application runs smoothly
- ✅ Code is more maintainable and organized
- ✅ Each mixin has clear, single responsibility

## 📊 **Expected Benefits**

### **Maintainability**:
- 🎯 Single Responsibility Principle achieved
- 🎯 Easier to locate and fix bugs
- 🎯 Cleaner code organization
- 🎯 Reduced cognitive load

### **Testability**:
- 🎯 Individual mixins can be unit tested
- 🎯 Isolated functionality testing
- 🎯 Easier to mock dependencies
- 🎯 Better test coverage

### **Performance**:
- 🎯 Optional performance monitoring
- 🎯 Cached cross-sheet lookups
- 🎯 Optimized highlighting algorithms
- 🎯 Lazy loading of features

### **Extensibility**:
- 🎯 New features can be added as mixins
- 🎯 Existing mixins can be enhanced independently
- 🎯 Modular architecture supports growth
- 🎯 Easier to add optional features

## 📝 **Implementation Checklist**

### **Pre-Refactoring**:
- [ ] Create backup branch: `git checkout -b item10-backup`
- [ ] Document current functionality
- [ ] Create test scenarios for validation
- [ ] Set up performance benchmarks

### **Stage 1 - Auto-Completion**:
- [ ] Create `EditorAutoCompletionMixin` class
- [ ] Extract auto-completion methods
- [ ] Update FormulaEditor to inherit from mixin
- [ ] Test auto-completion functionality
- [ ] Commit: "Stage 1: Extract auto-completion functionality"

### **Stage 2 - Cross-Sheet References**:
- [ ] Create `CrossSheetReferenceMixin` class
- [ ] Extract cross-sheet and highlighting methods
- [ ] Update FormulaEditor to use mixin
- [ ] Test cross-sheet references and highlighting
- [ ] Commit: "Stage 2: Extract cross-sheet reference handling"

### **Stage 3 - Performance Monitoring**:
- [ ] Create `EditorPerformanceMixin` class
- [ ] Extract performance and debugging methods
- [ ] Make performance monitoring optional
- [ ] Test performance monitoring features
- [ ] Commit: "Stage 3: Extract performance monitoring"

### **Stage 4 - Event Handling**:
- [ ] Create `EditorEventHandlerMixin` class
- [ ] Extract all event handling methods
- [ ] Test keyboard, mouse, and resize events
- [ ] Verify font sizing functionality
- [ ] Commit: "Stage 4: Extract event handling"

### **Stage 5 - Text Selection**:
- [ ] Create `EditorSelectionMixin` class
- [ ] Extract text selection and navigation methods
- [ ] Test all selection and navigation features
- [ ] Verify expression finding functionality
- [ ] Commit: "Stage 5: Extract text selection and navigation"

### **Post-Refactoring**:
- [ ] Full application testing
- [ ] Performance validation
- [ ] Update refactoring report to mark Item 10 as complete
- [ ] Push all changes to remote repository

---

*This refactoring plan follows the same successful approach used for Item 8 (evaluate() method refactoring) and will significantly improve the maintainability and organization of the FormulaEditor class.* 