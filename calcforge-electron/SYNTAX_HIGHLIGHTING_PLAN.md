# CalcForge Syntax Highlighting Implementation Plan

## 🎯 Goal
Implement syntax highlighting in CalcForge Electron app using contenteditable approach, starting with plain text and gradually adding colors to match the original Python app.

## 📋 Phase-by-Phase Implementation

### Phase 1: Clean Slate - Plain White Text ✅ **COMPLETED**
**Objective:** Get contenteditable working with plain white text, no highlighting

**Steps:**
1. ✅ Remove all overlay CSS and JavaScript
2. ✅ Set up contenteditable div with basic styling
3. ✅ Ensure Enter key works properly for new lines
4. ✅ Test basic typing, cursor movement, selection
5. ✅ Verify line numbers and results work
6. ✅ **BONUS:** Fixed scrollbar consistency and scroll synchronization
7. ✅ **BONUS:** Complete CSS cleanup - removed all old styling code

**CSS Requirements:**
```css
.expression-editor-div {
    color: var(--syntax-number) !important;  /* Pure white text */
    font-family: 'Roboto Mono', monospace;
    font-size: 14px;
    line-height: 1.5;
    white-space: pre;  /* No word wrapping */
    overflow-x: auto;  /* Horizontal scroll */
}
```

**Success Criteria:**
- ✅ Text appears as plain white in BOTH columns
- ✅ Enter key creates new lines naturally
- ✅ Cursor and selection work normally
- ✅ Line numbers sync correctly
- ✅ Results calculate and display
- ✅ **NEW:** Scrollbars have consistent thickness (12px)
- ✅ **NEW:** Both columns scroll synchronously
- ✅ **NEW:** No word wrapping - horizontal scroll instead
- ✅ **NEW:** Clean CSS base with all old styling removed

**Issues Fixed:**
- ✅ Text visibility (was transparent, now white)
- ✅ Inconsistent column behavior (now both use same approach)
- ✅ Scrollbar thickness mismatch (now consistent)
- ✅ No scroll synchronization (now synced both ways)
- ✅ Lingering old CSS code (completely cleaned up)

---

### Phase 2: Backend Syntax Analysis 🔄 **NEXT**
**Objective:** Ensure backend correctly identifies syntax elements

**Status:** Ready to begin - backend code exists and was tested during overlay phase

**Steps:**
1. ⏳ Test backend syntax highlighter with sample text
2. ⏳ Verify it returns correct highlight ranges
3. ⏳ Check line-by-line processing works (fixed during Phase 1)
4. ⏳ Validate all syntax types are detected

**Expected Backend Output:**
```json
[
  {"start": 0, "length": 3, "class": "syntax-number", "color": "#FFFFFF"},
  {"start": 4, "length": 1, "class": "syntax-operator", "color": "#4A90E2"},
  {"start": 6, "length": 2, "class": "syntax-number", "color": "#FFFFFF"}
]
```

**Test Cases:**
- Numbers: `100`, `3.14`, `0.5`
- Operators: `+`, `-`, `*`, `/`, `^`
- Functions: `sin(90)`, `sqrt(16)`, `TC(24, 100)`
- LN References: `LN1`, `LN2`, `LN10`
- Comments: `:::This is a comment`
- Parentheses: `(100 + 50)`

**Known Working:** Backend was functional during overlay testing, should work immediately

---

### Phase 3: Apply Highlighting to ContentEditable ⏳ **PENDING**
**Objective:** Apply syntax highlighting directly to contenteditable content

**Status:** Waiting for Phase 2 completion

**Implementation Strategy:**
```javascript
function applySyntaxHighlighting(text, highlights) {
    // Save cursor position
    const selection = saveSelection();

    // Build highlighted HTML
    let html = buildHighlightedHTML(text, highlights);

    // Update contenteditable
    editor.innerHTML = html;

    // Restore cursor position
    restoreSelection(selection);
}
```

**Key Challenges:**
- Cursor position preservation (helper methods already implemented)
- Selection handling (helper methods already implemented)
- Enter key behavior (working in Phase 1)
- Undo/redo functionality
- Apply to BOTH columns consistently

---

### Phase 4: Color Implementation - Step by Step

#### Step 4.1: Numbers Only
**Target:** Make numbers white (they already are, but ensure they stay white)
```css
.syntax-number { color: #FFFFFF; }
```

#### Step 4.2: Add Operators
**Target:** Make operators blue
```css
.syntax-operator { color: #4A90E2; }
```

#### Step 4.3: Add Functions
**Target:** Make functions blue (same as operators)
```css
.syntax-function { color: #4A90E2; }
```

#### Step 4.4: Add Parentheses
**Target:** Make parentheses green
```css
.syntax-paren { color: #7ED321; }
```

#### Step 4.5: Add Comments
**Target:** Make comment lines green and italic
```css
.syntax-comment { 
    color: #7ED321; 
    font-style: italic; 
}
```

#### Step 4.6: Add LN References
**Target:** Rotating colors for LN variables
```css
.ln-color-0 { color: #FF9999; font-weight: bold; }
.ln-color-1 { color: #99FF99; font-weight: bold; }
.ln-color-2 { color: #9999FF; font-weight: bold; }
.ln-color-3 { color: #FFFF99; font-weight: bold; }
/* ... more colors */
```

---

## 🎨 Final Color Scheme (Match Python App)

| Element | Color | CSS Class | Example |
|---------|-------|-----------|---------|
| Numbers | White | `.syntax-number` | `100`, `3.14` |
| Operators | Blue | `.syntax-operator` | `+`, `-`, `*`, `/` |
| Functions | Blue | `.syntax-function` | `sin`, `cos`, `sqrt` |
| Parentheses | Green | `.syntax-paren` | `(`, `)` |
| Comments | Green Italic | `.syntax-comment` | `:::Comment` |
| LN1 | Red Bold | `.ln-color-0` | `LN1` |
| LN2 | Green Bold | `.ln-color-1` | `LN2` |
| LN3 | Blue Bold | `.ln-color-2` | `LN3` |
| Errors | Red | `.syntax-error` | Invalid syntax |

---

## 🧪 Testing Strategy

### Phase 1 Tests: ✅ **COMPLETED**
- [x] Type simple text: "hello world"
- [x] Press Enter multiple times
- [x] Select text and delete
- [x] Copy and paste
- [x] Undo and redo
- [x] **BONUS:** Scroll synchronization between columns
- [x] **BONUS:** Consistent scrollbar thickness
- [x] **BONUS:** No word wrapping (horizontal scroll)

### Phase 2 Tests: ⏳ **NEXT**
- [ ] Backend returns highlights for "100 + 50"
- [ ] Backend handles multi-line text
- [ ] Backend identifies all syntax types
- [ ] Line-by-line processing works correctly

### Phase 3 Tests: ⏳ **PENDING**
- [ ] Highlighting applies without breaking cursor
- [ ] Enter key still works after highlighting
- [ ] Selection works with highlighted text
- [ ] Both columns get highlighting consistently

### Phase 4 Tests (per step): ⏳ **PENDING**
- [ ] Only target elements get colored
- [ ] Other elements remain unchanged
- [ ] No visual glitches or flickering
- [ ] Performance remains smooth

---

## 🚨 Known Issues to Avoid

1. **Cursor Jumping:** Save/restore selection properly
2. **Enter Key Breaking:** Don't interfere with default behavior
3. **Performance:** Debounce highlighting updates
4. **Flickering:** Minimize DOM updates
5. **Selection Loss:** Preserve user selections during updates

---

## 📝 Implementation Notes

- Start each phase only after previous phase is 100% working
- Test thoroughly on each step before proceeding
- Keep backup of working state before each change
- Use console.log extensively for debugging
- Test with real CalcForge expressions, not just simple examples

---

## 🎯 Success Metrics

**Phase 1 Success:** ✅ **ACHIEVED** - Plain white text, perfect editing experience, scroll sync, clean CSS base
**Phase 2 Success:** ⏳ Backend returns accurate syntax data
**Phase 3 Success:** ⏳ Highlighting works without breaking editing
**Phase 4 Success:** ⏳ Colors match original Python app exactly

Each phase must be completely stable before moving to the next phase.

---

## 📊 Current Status: **Phase 1 Complete** ✅

### ✅ **Achievements:**
- **Clean contenteditable implementation** with plain white text
- **Both columns synchronized** (scrolling, styling, behavior)
- **No word wrapping** - horizontal scroll instead
- **Consistent scrollbar thickness** throughout app
- **Complete CSS cleanup** - all old styling code removed/disabled
- **Solid foundation** ready for syntax highlighting implementation

### 🎯 **Next Steps:**
1. **Phase 2:** Test backend syntax analysis
2. **Phase 3:** Apply highlighting to contenteditable
3. **Phase 4:** Add colors incrementally

### 🏗️ **Architecture:**
- **Expression Column:** ContentEditable with direct HTML styling
- **Results Column:** HTML with CSS classes (will convert to match expression column)
- **Backend:** Python syntax highlighter (working, needs testing)
- **Synchronization:** Scroll, font, colors all matched between columns
