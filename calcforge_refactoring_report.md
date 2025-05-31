# CalcForge Refactoring Report
*Generated: December 31, 2025*

## Overview
This report identifies refactoring opportunities in the CalcForge application (calcforge.py) with a focus on consolidating duplicate classes and improving code organization.

## 🔴 **CRITICAL ISSUES - Duplicate Classes**

### 1. **DUPLICATE Calculator Classes** 
- **Line 615**: `class Calculator(QWidget): pass` (Forward declaration)
- **Line 4454**: `class Calculator(QWidget):` (Actual implementation)
- **Problem**: The first one is just a forward declaration but it's confusing and could cause issues
- **Solution**: Remove the forward declaration and reorganize the code

### 2. **TWO Syntax Highlighter Classes with Similar Purpose**
- **FormulaHighlighter** (Line 877): Complex highlighting for formulas with colors, LN references, etc.
- **ResultsHighlighter** (Line 4967): Simple highlighting just for error text
- **Solution**: These could potentially be consolidated into a single configurable highlighter class

## 🟡 **MAJOR REFACTORING OPPORTUNITIES - Duplicate Functions**

### 3. **Duplicate preprocess_expression Functions**
- **Line 489**: `def preprocess_expression(expr):`
- **Line 3707**: `def preprocess_expression(expr):` (inside Worksheet.evaluate)
- **Solution**: Extract to a shared utility module

### 4. **Duplicate truncate Functions**
- **Line 1573**: `def truncate_func(self, value, decimals=2):` (in FormulaEditor)
- **Line 3683**: `def truncate(value, decimals=2):` (in Worksheet.evaluate)
- **Solution**: Create a single utility function

### 5. **Duplicate remove_thousands_commas Functions**
- **Line 568**: `def remove_thousands_commas(match):`
- **Line 3786**: `def remove_thousands_commas(match):` (inside Worksheet.evaluate)
- **Solution**: Extract to shared utilities

### 6. **Duplicate repl_num Functions**
- **Line 580**: `def repl_num(m):`
- **Line 3798**: `def repl_num(m):` (inside Worksheet.evaluate)
- **Solution**: Extract to shared utilities

## 🟢 **ARCHITECTURAL IMPROVEMENTS**

### 7. **Line Number Area Classes Could Be Consolidated**
- **LineNumberArea** (Line 795): For the main editor
- **ResultsLineNumberArea** (Line 804): For the results panel
- **Solution**: Create a base `LineNumberAreaBase` class with shared functionality

### 8. **Large Monolithic evaluate() Method**
- The `Worksheet.evaluate()` method (Line 3636) is **massive** - over 800 lines!
- **Solution**: Break into smaller, focused methods:
  - `evaluate_line()`
  - `handle_special_commands()`
  - `process_statistical_functions()`
  - `handle_unit_conversions()`

### 9. **Global Constants Should Be Organized**
- Currency mappings (CURRENCY_ABBR, CURRENCY_DISPLAY, FALLBACK_RATES)
- Unit mappings (UNIT_ABBR, UNIT_DISPLAY)
- **Solution**: Move to a separate `constants.py` or `config.py` file

### 10. **FormulaEditor Class is Too Large**
- **2,000+ lines** with too many responsibilities
- **Solution**: Extract mixins or separate classes for:
  - Auto-completion functionality
  - Cross-sheet reference handling
  - Performance monitoring
  - Event handling

## 🔧 **SPECIFIC CONSOLIDATION OPPORTUNITIES**

### 11. **Shared Utility Functions Module**
Create a `utils.py` module with:
```python
# Expression processing
def preprocess_expression(expr)
def remove_thousands_commas(match)
def repl_num(match)

# Number formatting
def truncate_number(value, decimals=2)
def format_number_for_display(value, line_number)

# Validation
def is_timecode(value)
def get_numeric_value(value)
```

### 12. **Configuration Management Class**
```python
class CalcForgeConfig:
    # All currency, unit, and display mappings
    # Theme and styling constants
    # Default values and settings
```

### 13. **Base Highlighter Class**
```python
class BaseHighlighter(QSyntaxHighlighter):
    def __init__(self, document, highlight_type="formula"):
        # Configurable highlighting for both formula and results
```

### 14. **Event Handler Mixins**
```python
class MouseEventMixin:
class KeyboardEventMixin:
class WheelEventMixin:
```

## 🚀 **RECOMMENDED REFACTORING PLAN**

### Phase 1 - Critical Fixes (Priority: HIGH)
- Remove duplicate Calculator forward declaration
- Consolidate duplicate utility functions
- **Estimated effort**: 2-4 hours

### Phase 2 - Extract Utilities (Priority: HIGH)
- Create `utils.py` with shared functions
- Create `constants.py` with all mappings
- Create `config.py` for settings
- **Estimated effort**: 4-6 hours

### Phase 3 - Break Down Large Classes (Priority: MEDIUM)
- Split `Worksheet.evaluate()` into smaller methods
- Extract mixins from `FormulaEditor`
- **Estimated effort**: 8-12 hours

### Phase 4 - Architectural Improvements (Priority: LOW)
- Consolidate line number area classes
- Create configurable highlighter system
- Implement proper dependency injection
- **Estimated effort**: 6-10 hours

## 📊 **CURRENT CODE STATISTICS**

- **Total file size**: 5,059 lines
- **Number of classes**: 12
- **Duplicate functions identified**: 6 pairs
- **Largest class**: FormulaEditor (~2,000 lines)
- **Largest method**: Worksheet.evaluate() (~800 lines)

## 🎯 **BENEFITS OF REFACTORING**

1. **Maintainability**: Easier to update and debug code
2. **Reusability**: Shared functions can be used across modules
3. **Testability**: Smaller functions are easier to unit test
4. **Performance**: Better code organization can improve runtime
5. **Extensibility**: Cleaner architecture makes adding features easier

## 📝 **NOTES**

- All line numbers are approximate based on current file structure
- This refactoring can be done incrementally without breaking functionality
- Consider creating backup branches before major structural changes
- Unit tests should be added during refactoring to ensure functionality is preserved

---

*This report was generated by analyzing the calcforge.py file for duplicate code patterns and architectural improvements.* 