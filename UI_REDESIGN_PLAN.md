# CalcForge UI Redesign Implementation Plan

## 🎯 Overview
Transform CalcForge from the current basic Qt interface to a modern GitHub-inspired dark theme with enhanced functionality, while preserving all existing features and syntax highlighting.

## 📋 Current State Analysis
- **Framework**: PySide6 with QTabWidget, QSplitter, QPlainTextEdit
- **Main Classes**: `Calculator` (main window), `Worksheet` (tab content), `FormulaEditor` (input), `ResultsLineNumberArea`
- **Key Features**: Syntax highlighting, line numbers, cross-sheet references, auto-completion
- **Theme**: Basic Qt styling with some dark mode elements

## 🎨 Target Design (from new_design/newdesign.html)
- **Header**: Clean title bar with right-aligned New Tab + Help buttons
- **Tabs**: GitHub-style tabs with close buttons (×) next to text
- **Panels**: Two-column layout with line numbers on both sides
- **Scrollbars**: Custom dark theme scrollbars (vertical + horizontal)
- **Colors**: GitHub dark theme (`#0D1117`, `#161B22`, `#30363D`, `#0c7ff2`)

## 🚀 Implementation Phases

### Phase 1: Core UI Structure Changes

#### 1.1 Header Redesign (`Calculator.__init__()`)
**Current Code Location**: Lines 5724-5765
```python
# Current: Simple horizontal layout
top = QHBoxLayout()
add_btn = QPushButton("+")
help_btn = QPushButton("?")
```

**New Implementation**:
- Remove icon from title area
- Create right-aligned button container
- Style buttons with GitHub theme
- Add proper spacing and margins

#### 1.2 Tab Bar Enhancement (`Calculator.tabs`)
**Current Code Location**: Lines 5767-5773
```python
self.tabs = QTabWidget()
self.tabs.setTabsClosable(True)
```

**New Implementation**:
- Custom tab styling with CSS
- Add close buttons (×) next to tab text
- Implement active tab highlighting
- GitHub-style rounded corners and colors

#### 1.3 Main Panel Layout (`Worksheet.__init__()`)
**Current Code Location**: Lines 3126-3324
```python
self.splitter = QSplitter(Qt.Horizontal)
```

**New Implementation**:
- Enhanced panel containers
- Fixed height for vertical scrolling
- Improved line number integration
- Dark theme panel backgrounds

### Phase 2: Line Numbers Implementation

#### 2.1 Enhanced Line Number Areas
**Current Classes**: `LineNumberArea`, `ResultsLineNumberArea` (Lines 632-688)

**Enhancements**:
- Dedicated dark sidebars (`#0D1117` background)
- Current line highlighting (white/bold)
- Proper border styling (`#30363D`)
- Synchronized scrolling improvements

#### 2.2 Results Panel Line Numbers
**Current Code Location**: Lines 643-688
```python
class ResultsLineNumberArea(LineNumberAreaBase):
```

**New Features**:
- Enhanced visual styling
- Better comment line indicators ("C" in green)
- Improved LN reference display
- Current line synchronization

### Phase 3: Styling and Theme

#### 3.1 Dark Theme Implementation
**Color Scheme**:
```python
GITHUB_COLORS = {
    'bg_primary': '#0D1117',      # Main background
    'bg_secondary': '#161B22',    # Panel backgrounds  
    'bg_tertiary': '#21262D',     # Button/tab backgrounds
    'border': '#30363D',          # Border color
    'text_primary': '#e0e0e0',    # Normal text
    'text_secondary': '#8b949e',  # Secondary text
    'text_current': '#FFFFFF',    # Current line text
    'accent': '#0c7ff2',          # Blue accent color
    'hover': '#4A5568'            # Hover states
}
```

#### 3.2 Custom Scrollbars
**Implementation**: CSS styling for QScrollBar
```css
QScrollBar:vertical {
    background-color: #21262D;
    width: 12px;
    border-radius: 6px;
}
QScrollBar::handle:vertical {
    background-color: #30363D;
    border-radius: 6px;
    border: 3px solid #21262D;
}
```

### Phase 4: Syntax Highlighting Preservation & Enhancement

#### 4.1 Enhanced Color Schemes (`FormulaHighlighter`)
**Current Code Location**: Lines 710-819
```python
class FormulaHighlighter(BaseHighlighter):
```

**Updated Colors**:
```python
SYNTAX_COLORS = {
    'number': '#79C0FF',      # Light blue numbers
    'operator': '#FF7B72',    # Light red operators
    'function': '#D2A8FF',    # Light purple functions
    'paren': '#FFA657',       # Orange parentheses
    'comment': '#8B949E',     # Gray comments
    'ln_reference': '#A5D6FF', # Cyan LN references
    'error': '#F85149',       # Red errors
    'sheet_ref': '#4DA6FF'    # Blue sheet references
}
```

#### 4.2 Line Number Color Coding
**Preserve & Enhance**:
- Regular lines: Numbered (1, 2, 3...)
- Comment lines: "C" marker in green (`#7ED321`)
- LN reference lines: Show actual LN ID numbers
- Current line: Bold white highlighting
- Error lines: Red highlighting for calculation errors

#### 4.3 Cross-Sheet Reference Enhancement
- Sheet names: Blue highlighting (`#4DA6FF`)
- LN numbers: Unique colors from cycling palette
- Hover effects: Subtle highlighting on mouseover

### Phase 5: Enhanced Functionality

#### 5.1 Tab Management
**New Features**:
- Close button (×) functionality for each tab
- Enhanced tab creation with proper styling
- Tab reordering with visual feedback
- Confirmation dialogs with dark theme

#### 5.2 Panel Improvements
**Enhancements**:
- Ensure vertical scrolling works properly
- Synchronized scrolling between editor and results
- Proper content overflow handling
- Fixed panel heights for scrollbar visibility

## 🔧 Technical Implementation Details

### File Modifications Required:
1. **calcforge.py** - Main implementation
2. **constants.py** - Color scheme updates
3. **UI_REDESIGN_PLAN.md** - This documentation

### Key Classes to Modify:
- `Calculator` - Header and tab management
- `Worksheet` - Panel layout and styling
- `FormulaHighlighter` - Syntax highlighting colors
- `LineNumberArea` / `ResultsLineNumberArea` - Line number styling
- `FormulaEditor` - Editor styling and scrollbars

### CSS Styling Strategy:
- Use QWidget.setStyleSheet() for comprehensive theming
- Implement custom scrollbar styling
- Create reusable style constants
- Ensure consistent spacing and margins

## ✅ Testing Checklist

### Functionality Preservation:
- [ ] All syntax highlighting features work
- [ ] LN reference colors cycle properly
- [ ] Cross-sheet references highlight correctly
- [ ] Comment lines show "C" marker in green
- [ ] Current line is bold and white
- [ ] Function names are properly highlighted
- [ ] Parentheses matching works with new colors
- [ ] Error messages are clearly visible
- [ ] Auto-completion still functions
- [ ] Tab management (create, close, rename) works
- [ ] Scrolling synchronization maintained

### Visual Enhancements:
- [ ] GitHub dark theme applied consistently
- [ ] Line numbers visible in both panels
- [ ] Vertical scrollbars appear when needed
- [ ] Horizontal scrollbars work properly
- [ ] Tab close buttons (×) function correctly
- [ ] Header layout matches design
- [ ] Panel spacing and margins correct
- [ ] Current line highlighting prominent

### Performance:
- [ ] Scrolling remains smooth
- [ ] Syntax highlighting performance maintained
- [ ] Tab switching responsive
- [ ] No memory leaks from styling changes

## 🎯 Success Criteria
1. **Visual Fidelity**: Matches new_design/newdesign.html appearance
2. **Feature Preservation**: All existing functionality intact
3. **Enhanced UX**: Improved usability with better visual feedback
4. **Performance**: No degradation in responsiveness
5. **Maintainability**: Clean, well-documented code changes

## 📝 Implementation Notes
- Preserve all existing keyboard shortcuts and functionality
- Maintain backward compatibility with saved worksheets
- Ensure proper error handling for styling failures
- Test on different screen resolutions and DPI settings
- Document any breaking changes or new dependencies

---

**Branch**: `ui-redesign`  
**Target**: Complete visual transformation while preserving all functionality  
**Timeline**: Implement in phases with testing at each stage
