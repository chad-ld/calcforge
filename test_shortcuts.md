# Keyboard Shortcuts Test Guide

## Restored Keyboard Shortcuts in calcforge v3.1

### 1. **Ctrl+C (without selection)**: Copy Answer/Result
- **Test**: Place cursor on any line with a calculated result (don't select text)
- **Action**: Press Ctrl+C
- **Expected**: The result/answer from that line should be copied to clipboard
- **Verify**: Paste elsewhere to confirm the result was copied

### 2. **Ctrl+C (with selection)**: Copy Selected Text 
- **Test**: Select some text in the editor
- **Action**: Press Ctrl+C
- **Expected**: Selected text should be copied
- **Verify**: Paste elsewhere to confirm

### 3. **Alt+C**: Copy Expression Line
- **Test**: Place cursor on any line with an expression
- **Action**: Press Alt+C
- **Expected**: The entire line should be copied to clipboard
- **Verify**: Paste elsewhere to confirm

### 4. **Ctrl+Up Arrow**: Navigate and Select Text Inside Parentheses
- **Test**: Type an expression like `(2 + 3) * 4` and place cursor inside the parentheses
- **Action**: Press Ctrl+Up Arrow
- **Expected**: Should progressively select text within parentheses
- **Verify**: Text inside parentheses should be highlighted

### 5. **Ctrl+Down Arrow**: Select Entire Line
- **Test**: Place cursor anywhere on a line with content
- **Action**: Press Ctrl+Down Arrow
- **Expected**: The entire line should be selected
- **Verify**: Entire line should be highlighted

### 6. **Tab Key Disabled**: No Tab Insertion
- **Test**: Place cursor in the editor
- **Action**: Press Tab key
- **Expected**: No tab character should be inserted
- **Verify**: If autocompletion popup is open, it should complete; otherwise, nothing happens

### 7. **Ctrl+Shift+Left/Right**: Navigate Between Sheets
- **Test**: Create multiple sheets/tabs (click the "+" button)
- **Action**: Press Ctrl+Shift+Left or Ctrl+Shift+Right
- **Expected**: Should switch between tabs
- **Verify**: Active tab should change, wrapping around from last to first

## Test Results
- [ ] Ctrl+C (no selection) works correctly - copies result
- [ ] Ctrl+C (with selection) works correctly - copies selected text
- [ ] Alt+C works correctly - copies expression line
- [ ] Ctrl+Up works correctly - selects within parentheses
- [ ] Ctrl+Down works correctly - selects entire line
- [ ] Tab insertion is disabled
- [ ] Ctrl+Shift+Left/Right works correctly - navigates tabs

## Notes
All keyboard shortcuts should now work as expected. The key fixes include:
- **Ctrl+C without selection** now copies the result/answer from the current line
- **Ctrl+Shift+Left/Right** properly navigates between worksheet tabs
- **Tab key** no longer inserts tab characters
- All other shortcuts work as intended

The application should now behave exactly as it did before your refactoring! 