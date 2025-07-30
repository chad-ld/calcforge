# CalcForge Syntax Highlighting Implementation Plan - OVERLAY APPROACH

## Current Status: Textarea Foundation Complete ✅
**Clean textarea-based editor with overlay architecture ready**

## Overview
This document outlines the step-by-step implementation of syntax highlighting for CalcForge using the **overlay approach** - the industry standard used by VS Code, Monaco Editor, CodeMirror, and all modern code editors.

## Architecture: Textarea + Overlay System
**Why this approach?**
- ✅ **Non-destructive**: Text input never gets corrupted
- ✅ **Stable cursor**: Textarea handles all cursor positioning naturally  
- ✅ **Reliable line breaks**: Natural `\n` characters, no HTML interference
- ✅ **Industry standard**: Used by VS Code, GitHub, CodePen, JSFiddle, etc.
- ✅ **No contenteditable issues**: Eliminates all cursor jumping problems

### Technical Architecture:
```html
<div class="editor-wrapper">
  <textarea class="expression-editor-textarea"></textarea>  <!-- Text input -->
  <div class="syntax-overlay"></div>                       <!-- Syntax highlighting -->
  <div class="line-numbers"></div>                         <!-- Line numbers -->
</div>
```

## Phase 1: Foundation ✅ COMPLETE
**Goal: Clean textarea-based editor working perfectly**

### Status: ✅ COMPLETE
- ✅ Replaced contenteditable with textarea
- ✅ All text appears as plain white
- ✅ Line numbers work correctly  
- ✅ Enter key creates new lines naturally
- ✅ Calculations work on each line
- ✅ Autocomplete disabled on comment lines
- ✅ No syntax highlighting yet (intentional)
- ✅ No contenteditable code remaining
- ✅ No duplicate functions

### Key Implementation Details:
- Using `<textarea>` for text input
- Simple `textarea.value` for text handling
- Simple `textarea.selectionStart/End` for cursor
- CSS overlay positioned exactly over textarea
- All contenteditable complexity removed

## Phase 2: Overlay Implementation (NEXT)
**Goal: Implement overlay-based syntax highlighting step by step**

### Baby Steps Approach:
Each step is small, testable, and reversible. No step should break existing functionality.

### Step 2.1: Basic Text Mirroring ⏳ NEXT
**Goal: Mirror plain text from textarea to overlay (no colors yet)**

**Implementation:**
1. Create `updateOverlay()` method
2. Copy text from `textarea.value` to overlay
3. Ensure perfect font/spacing alignment
4. Test that overlay text matches textarea exactly

**Expected Result:**
- Overlay shows same text as textarea (invisible/transparent)
- Perfect alignment between textarea and overlay
- No visual change to user
- Foundation ready for highlighting

**Success Criteria:**
- Text in overlay matches textarea character-for-character
- Line breaks align perfectly
- Font metrics identical
- Scrolling synchronized

### Step 2.2: Basic Number Highlighting
**Goal: Highlight numbers in white (same as current)**

**Implementation:**
1. Add simple regex to find numbers
2. Wrap numbers in `<span class="syntax-number">` 
3. Apply white color via CSS
4. Test that highlighting doesn't break alignment

**Expected Result:**
- Numbers appear white (same as current)
- No visual change, but infrastructure working
- Overlay system proven functional

**Colors:**
- Numbers: `#FFFFFF` (white)
- Everything else: transparent

### Step 2.3: Add Operator Colors
**Goal: Add bright orange color for operators**

**Implementation:**
1. Add regex to find operators (`+`, `-`, `*`, `/`, `=`, etc.)
2. Wrap in `<span class="syntax-operator">`
3. Apply orange color via CSS

**Expected Result:**
- Numbers: white
- Operators: bright orange
- Everything else: white

**Colors:**
- Numbers: `#FFFFFF` (white)
- Operators: `#FF8C00` (bright orange)
- Everything else: `#FFFFFF` (white)

### Step 2.4: Add Function Colors  
**Goal: Add bright orange color for function names**

**Implementation:**
1. Add regex to find function names
2. Wrap in `<span class="syntax-function">`
3. Apply orange color (same as operators)

**Colors:**
- Numbers: `#FFFFFF` (white)
- Operators: `#FF8C00` (bright orange)
- Functions: `#FF8C00` (bright orange)
- Everything else: `#FFFFFF` (white)

### Step 2.5: Add Parentheses Colors
**Goal: Add bright green color for parentheses**

**Implementation:**
1. Add regex to find parentheses `(` and `)`
2. Wrap in `<span class="syntax-parenthesis">`
3. Apply green color

**Colors:**
- Numbers: `#FFFFFF` (white)
- Operators: `#FF8C00` (bright orange)
- Functions: `#FF8C00` (bright orange)  
- Parentheses: `#00FF00` (bright green)
- Everything else: `#FFFFFF` (white)

### Step 2.6: Add Comment Colors
**Goal: Add bright green and bold for comments (lines starting with :::)**

**Implementation:**
1. Add regex to find comment lines
2. Wrap entire comment lines in `<span class="syntax-comment">`
3. Apply green + bold styling

**Colors:**
- Numbers: `#FFFFFF` (white)
- Operators: `#FF8C00` (bright orange)
- Functions: `#FF8C00` (bright orange)
- Parentheses: `#00FF00` (bright green)
- Comments: `#00FF00` (bright green) + `font-weight: bold`
- Everything else: `#FFFFFF` (white)

### Step 2.7: Add Error Colors
**Goal: Add red color for syntax errors**

**Implementation:**
1. Integrate with backend error detection
2. Wrap error tokens in `<span class="syntax-error">`
3. Apply red color

**Colors:**
- Numbers: `#FFFFFF` (white)
- Operators: `#FF8C00` (bright orange)
- Functions: `#FF8C00` (bright orange)
- Parentheses: `#00FF00` (bright green)
- Comments: `#00FF00` (bright green) + `font-weight: bold`
- Errors: `#FF0000` (red)
- Everything else: `#FFFFFF` (white)

### Step 2.8: Add LN Variable Colors
**Goal: Add rotating colors for LN variables (LN1, LN2, etc.)**

**Implementation:**
1. Add regex to find LN variables
2. Determine LN number and assign rotating color
3. Wrap in appropriate `<span class="syntax-ln-X">` class

**LN Variable Colors (rotating):**
1. `#FF6B6B` (coral red)
2. `#4ECDC4` (teal)  
3. `#45B7D1` (sky blue)
4. `#96CEB4` (mint green)
5. `#FFEAA7` (light yellow)
6. `#DDA0DD` (plum)
7. `#98D8C8` (mint)
8. `#F7DC6F` (light gold)

**Final Color Scheme:**
- Numbers: `#FFFFFF` (white)
- Operators: `#FF8C00` (bright orange)
- Functions: `#FF8C00` (bright orange)
- Parentheses: `#00FF00` (bright green)
- Comments: `#00FF00` (bright green) + `font-weight: bold`
- Errors: `#FF0000` (red)
- LN Variables: Rotating colors (8 colors, cycle through)

## Phase 3: Backend Integration
**Goal: Replace simple regex with backend API highlighting**

### Step 3.1: API Integration
**Goal: Use backend syntax highlighting API instead of simple regex**

**Implementation:**
1. Enable `this.api.getSyntaxHighlighting(text)` calls
2. Replace regex highlighting with API response
3. Map API highlight data to CSS classes
4. Maintain same visual appearance

### Step 3.2: Advanced Features
**Goal: Add sophisticated highlighting features**

**Features:**
- Real-time error highlighting
- Context-aware function highlighting
- Unit detection and highlighting
- Cross-reference highlighting (LN variables)

## Phase 4: Optimization and Polish
**Goal: Optimize performance and add advanced features**

### Step 4.1: Performance Optimization
- Implement smart diffing to only update changed parts
- Optimize highlighting for large documents  
- Add caching for repeated highlighting requests
- Debounce highlighting updates during rapid typing

### Step 4.2: Advanced Features
- Bracket matching
- Hover tooltips for functions
- Color customization options
- Accessibility improvements

## Technical Implementation Details

### CSS Classes Structure:
```css
.syntax-overlay {
    position: absolute;
    top: 0; left: 0; right: 0; bottom: 0;
    font-family: 'Roboto Mono', monospace;
    font-size: 14px;
    line-height: 1.5;
    padding: var(--spacing-sm);
    pointer-events: none;
    color: transparent; /* Hide base text */
    white-space: pre;
    overflow: hidden;
    z-index: 2;
}

.syntax-number { color: #FFFFFF; }
.syntax-operator { color: #FF8C00; }
.syntax-function { color: #FF8C00; }
.syntax-parenthesis { color: #00FF00; }
.syntax-comment { color: #00FF00; font-weight: bold; }
.syntax-error { color: #FF0000; }
.syntax-ln-1 { color: #FF6B6B; }
.syntax-ln-2 { color: #4ECDC4; }
/* ... etc for all 8 LN colors */
```

### Overlay Update Method:
```javascript
updateOverlay() {
    if (!this.syntaxOverlay) return;
    
    const text = this.getEditorText();
    const highlightedHTML = this.applyHighlighting(text);
    
    this.syntaxOverlay.innerHTML = highlightedHTML;
    
    // Sync scroll position
    this.syntaxOverlay.scrollTop = this.editor.scrollTop;
    this.syntaxOverlay.scrollLeft = this.editor.scrollLeft;
}
```

### Scroll Synchronization:
```javascript
onScroll() {
    // Sync overlay scroll with textarea
    if (this.syntaxOverlay) {
        this.syntaxOverlay.scrollTop = this.editor.scrollTop;
        this.syntaxOverlay.scrollLeft = this.editor.scrollLeft;
    }
    
    // Existing scroll sync for results
    if (this.resultsDisplay) {
        this.resultsDisplay.scrollTop = this.editor.scrollTop;
    }
}
```

## Testing Strategy

### Each Step Testing:
1. **Visual verification**: Colors appear correctly
2. **Alignment testing**: Overlay text aligns with textarea
3. **Scroll testing**: Overlay scrolls with textarea
4. **Performance testing**: No lag during typing
5. **Functionality testing**: Calculations, autocomplete still work

### Regression Testing:
- Ensure calculations still work correctly
- Verify line numbers update properly
- Check autocomplete functionality
- Test copy/paste operations
- Verify Enter key behavior
- Test font size changes

## Risk Mitigation

### Known Risks:
1. **Font alignment**: Overlay might not align perfectly with textarea
2. **Performance**: Real-time highlighting might slow down typing
3. **Browser differences**: Font rendering varies between browsers

### Mitigation Strategies:
1. **Careful CSS**: Identical font metrics between textarea and overlay
2. **Debouncing**: Limit highlighting frequency during rapid typing
3. **Fallback**: Disable highlighting if alignment issues detected
4. **Testing**: Test across different browsers and font sizes

## Success Criteria

### Phase 2 Success:
- ✅ All syntax elements have correct colors
- ✅ Perfect alignment between textarea and overlay
- ✅ No performance degradation during normal typing
- ✅ All existing functionality (calculations, autocomplete) works
- ✅ Visual appearance matches the original Python CalcForge app
- ✅ Solid, crisp text colors (no glows, shadows, or effects)

### Overall Success:
- Syntax highlighting that enhances usability without breaking functionality
- Performance that feels responsive and natural
- Visual design that matches user expectations
- Robust implementation that handles edge cases gracefully
- Industry-standard overlay approach that's maintainable and extensible

## Key Differences from Previous Approach

### ❌ Old ContentEditable Approach:
- Used `contenteditable="true"` div
- Applied highlighting by replacing `innerHTML`
- Caused cursor jumping and line disappearing
- Complex DOM manipulation and selection handling
- Unreliable text structure

### ✅ New Overlay Approach:
- Uses simple `<textarea>` for text input
- Applies highlighting via positioned overlay
- Stable cursor positioning (textarea handles it)
- Simple text handling (`textarea.value`)
- Industry-standard architecture

### Why This Will Work:
1. **Separation of concerns**: Text input and highlighting are separate
2. **Proven approach**: Used by all major code editors
3. **No interference**: Highlighting doesn't affect text input
4. **Simple debugging**: Easy to disable overlay if issues arise
5. **Maintainable**: Clear, understandable code structure

## Next Steps

### Immediate Next Step: Step 2.1 - Basic Text Mirroring
Ready to implement the first overlay step:
1. Add `updateOverlay()` method to editor.js
2. Call it from `onInput()` event
3. Test perfect text alignment
4. Verify no performance issues

**This approach will finally give us stable, reliable syntax highlighting that works like professional code editors!**
