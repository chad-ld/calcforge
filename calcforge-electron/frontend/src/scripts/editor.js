/**
 * CalcForge Editor Manager
 * Handles text editing, syntax highlighting, and real-time calculation
 */

class EditorManager {
    constructor(api) {
        this.api = api;
        this.editor = null;
        this.resultsDisplay = null;
        this.syntaxOverlay = null;
        this.editorLineNumbers = null;
        this.resultsLineNumbers = null;
        
        this.currentLine = 1;
        this.totalLines = 0;
        this.isCalculating = false;
        this.calculationTimeout = null;
        this.syntaxTimeout = null;
        this.calculationDelay = 200; // ms - responsive but not too aggressive
        this.calculatingStartTime = null;
        this.minCalculatingTime = 300; // Minimum time to show calculating animation
        
        this.undoStack = [];
        this.redoStack = [];
        this.maxUndoSteps = 50;
        
        // Bind methods
        this.onInput = this.onInput.bind(this);
        this.onKeyDown = this.onKeyDown.bind(this);
        this.onScroll = this.onScroll.bind(this);
        this.onSelectionChange = this.onSelectionChange.bind(this);
        this.onCalculationResult = this.onCalculationResult.bind(this);
        this.onBatchCalculationResult = this.onBatchCalculationResult.bind(this);
    }
    
    /**
     * Initialize the editor
     */
    init() {
        this.editor = document.getElementById('expression-editor');
        this.resultsDisplay = document.getElementById('results-display');
        this.editorLineNumbers = document.getElementById('editor-line-numbers');
        this.resultsLineNumbers = document.getElementById('results-line-numbers');

        if (!this.editor) {
            console.error('Editor element not found');
            return false;
        }

        console.log('Phase 1: Initializing contenteditable with plain white text');
        
        // Set up event listeners for contenteditable
        this.editor.addEventListener('input', this.onInput);
        this.editor.addEventListener('keydown', this.onKeyDown);
        this.editor.addEventListener('scroll', this.onScroll);
        this.editor.addEventListener('blur', this.onSelectionChange);
        this.editor.addEventListener('keyup', this.onSelectionChange);
        this.editor.addEventListener('mouseup', this.onSelectionChange);

        // Set up scroll synchronization between columns
        this.setupScrollSync();
        
        // Listen for API calculation results
        window.addEventListener('calculationResult', this.onCalculationResult);
        window.addEventListener('batchCalculationResult', this.onBatchCalculationResult);
        
        // Initialize with empty content
        this.updateLineNumbers();
        this.updateResults([]);

        // Fix any broken content from previous sessions
        this.fixBrokenContent();

        return true;
    }
    
    /**
     * Handle text input
     */
    onInput(event) {
        // Save state for undo
        this.saveUndoState();

        // Update line numbers
        this.updateLineNumbers();

        // Debounced calculation
        this.scheduleCalculation();

        // Debounced syntax highlighting to prevent flickering
        this.scheduleSyntaxHighlighting();
    }
    
    /**
     * Handle key events
     */
    onKeyDown(event) {
        // Handle special key combinations
        if (event.ctrlKey || event.metaKey) {
            switch (event.key) {
                case 'z':
                    if (event.shiftKey) {
                        this.redo();
                    } else {
                        this.undo();
                    }
                    event.preventDefault();
                    break;
                case 'y':
                    this.redo();
                    event.preventDefault();
                    break;
                case 'a':
                    // Select all - let default behavior happen
                    break;
                case 'c':
                    // Copy - handle in separate method
                    this.handleCopy(event);
                    break;
                case 'v':
                    // Paste - save undo state
                    setTimeout(() => this.saveUndoState(), 0);
                    break;
            }
        }
        
        // Handle Enter key
        if (event.key === 'Enter') {
            // Auto-indent on new line
            this.handleEnterKey(event);
        }
        
        // Handle Tab key
        if (event.key === 'Tab') {
            this.handleTabKey(event);
        }
    }
    
    /**
     * Handle scroll synchronization
     */
    onScroll() {
        if (this.resultsDisplay) {
            this.resultsDisplay.scrollTop = this.editor.scrollTop;
        }

        // Update line number scroll
        if (this.editorLineNumbers) {
            this.editorLineNumbers.scrollTop = this.editor.scrollTop;
        }
        if (this.resultsLineNumbers) {
            this.resultsLineNumbers.scrollTop = this.editor.scrollTop;
        }

        // Sync syntax overlay scroll
        if (this.syntaxOverlay) {
            this.syntaxOverlay.scrollTop = this.editor.scrollTop;
        }
    }
    
    /**
     * Handle selection change
     */
    onSelectionChange() {
        this.updateCurrentLine();
    }
    
    /**
     * Schedule calculation with debouncing
     */
    scheduleCalculation() {
        if (this.calculationTimeout) {
            clearTimeout(this.calculationTimeout);
        }

        this.calculationTimeout = setTimeout(() => {
            this.calculateAll();
        }, this.calculationDelay);
    }

    /**
     * Schedule syntax highlighting with debouncing to prevent flickering
     */
    scheduleSyntaxHighlighting() {
        if (this.syntaxTimeout) {
            clearTimeout(this.syntaxTimeout);
        }

        this.syntaxTimeout = setTimeout(() => {
            this.updateSyntaxHighlighting();
        }, 100); // Quick syntax highlighting
    }
    
    /**
     * Calculate all expressions
     */
    async calculateAll() {
        if (this.isCalculating) return;

        const text = this.getEditorText();
        const lines = text.split('\n');
        
        if (lines.length === 0) {
            this.updateResults([]);
            return;
        }
        
        this.isCalculating = true;
        this.showCalculating(true);
        
        const startTime = performance.now();
        
        try {
            // Filter out empty lines and comments for calculation
            const expressions = lines.map(line => line.trim());
            
            // Use batch calculation for efficiency
            const results = await this.api.calculateBatch(expressions);
            
            const endTime = performance.now();
            const calculationTime = Math.round(endTime - startTime);
            
            this.updateResults(results);
            this.updateCalculationTime(calculationTime);
            
        } catch (error) {
            console.error('Calculation failed:', error);
            this.showError('Calculation failed: ' + error.message);
        } finally {
            this.isCalculating = false;
            this.showCalculating(false);
        }
    }
    
    /**
     * Update line numbers
     */
    updateLineNumbers() {
        const text = this.getEditorText();
        const lines = text.split('\n');
        this.totalLines = lines.length;

        // Update editor line numbers
        if (this.editorLineNumbers) {
            const lineNumbersHTML = lines.map((line, index) => {
                const lineNum = index + 1;
                const isComment = line.trim().startsWith(':::');
                const isEmpty = line.trim() === '';
                const isCurrent = lineNum === this.currentLine;

                let className = 'line-number';
                if (isCurrent) className += ' current';
                if (isComment) className += ' comment';
                if (isEmpty) className += ' empty';

                return `<div class="${className}">${lineNum}</div>`;
            }).join('');

            this.editorLineNumbers.innerHTML = lineNumbersHTML;
        }

        // Update results line numbers
        if (this.resultsLineNumbers) {
            const resultLineNumbersHTML = lines.map((line, index) => {
                const lineNum = index + 1;
                const isComment = line.trim().startsWith(':::');
                const isEmpty = line.trim() === '';
                const isCurrent = lineNum === this.currentLine;

                let className = 'line-number';
                if (isCurrent) className += ' current';
                if (isComment) className += ' comment';
                if (isEmpty) className += ' empty';

                let displayText = lineNum;
                if (isComment) displayText = 'C';

                return `<div class="${className}">${displayText}</div>`;
            }).join('');

            this.resultsLineNumbers.innerHTML = resultLineNumbersHTML;
        }

        // Update status
        this.updateStatus();
    }
    
    /**
     * Update current line number
     */
    updateCurrentLine() {
        const selection = this.getSelection();
        const text = this.getEditorText();
        const textBeforeCursor = text.substring(0, selection.start);
        this.currentLine = textBeforeCursor.split('\n').length;

        this.updateLineNumbers();
        this.updateStatus();
    }
    
    /**
     * Update results display
     */
    updateResults(results) {
        if (!this.resultsDisplay) return;

        const text = this.getEditorText();
        const lines = text.split('\n');
        
        const resultHTML = lines.map((line, index) => {
            const result = results[index];
            const isComment = line.trim().startsWith(':::');
            const isEmpty = line.trim() === '';
            
            let className = 'result-line';
            let content = '';
            
            if (isComment) {
                className += ' comment';
                content = ''; // Comments don't show results
            } else if (isEmpty) {
                className += ' empty';
                content = '';
            } else if (result) {
                if (result.error) {
                    className += ' error';
                    content = result.error;
                } else {
                    let value = result.value;
                    if (result.unit) {
                        content = `${value} ${result.unit}`;
                    } else {
                        content = String(value);
                    }
                }
            } else {
                content = '';
            }
            
            return `<div class="${className}">${this.escapeHtml(content)}</div>`;
        }).join('');
        
        this.resultsDisplay.innerHTML = resultHTML;
        
        // Update last update time
        this.updateLastUpdate();
    }
    
    /**
     * Update syntax highlighting using CSS classes (safe approach)
     */
    async updateSyntaxHighlighting() {
        console.log('CSS-based syntax highlighting enabled');

        if (!this.editor || this.isApplyingHighlights) {
            return;
        }

        const text = this.getEditorText();
        if (!text.trim()) {
            return;
        }

        try {
            // Get highlights from API
            const highlights = await this.api.getSyntaxHighlighting(text);
            if (highlights && highlights.length > 0) {
                this.applySyntaxHighlightsWithCSS(highlights, text);
            }
        } catch (error) {
            console.error('Syntax highlighting failed:', error);
            // Don't break the editor if highlighting fails
        }
    }

    /**
     * Fix broken editor content by converting to plain text
     */
    fixBrokenContent() {
        if (!this.editor) return;

        // Only fix content if it contains broken HTML, not if it has valid syntax highlighting
        const innerHTML = this.editor.innerHTML;

        // Check if content has syntax highlighting spans - if so, don't strip them
        if (innerHTML.includes('<span class="syntax-')) {
            console.log('Content has syntax highlighting, skipping fixBrokenContent');
            return;
        }

        // Get the plain text content (strips all HTML)
        let plainText = this.editor.textContent || '';

        // Clean up any extra whitespace that might be causing spacing issues
        plainText = plainText
            .replace(/\u00A0/g, ' ')  // Replace non-breaking spaces with regular spaces
            .replace(/\s+$/gm, '')    // Remove trailing whitespace from each line
            .replace(/^\s+/gm, '')    // Remove leading whitespace from each line
            .replace(/\n\s*\n\s*\n/g, '\n\n'); // Replace multiple empty lines with just one

        // Clear and reset with cleaned plain text
        this.editor.innerHTML = '';
        this.editor.textContent = plainText;

        console.log('Fixed broken editor content, restored clean plain text');
    }

    /**
     * Apply syntax highlights using CSS classes (safer approach)
     */
    applySyntaxHighlightsWithCSS(highlights, text) {
        // console.log('applySyntaxHighlightsWithCSS called with:', highlights.length, 'highlights');

        if (!this.editor) {
            console.error('Editor element not found in applySyntaxHighlightsWithCSS!');
            return;
        }

        // Set flag to prevent infinite loops
        this.isApplyingHighlights = true;

        // Save cursor position before modifying innerHTML
        const selection = this.saveSelection();

        // Create highlighted version of text using CSS classes
        let highlightedHTML = '';
        let lastIndex = 0;

        // Sort highlights by start position
        highlights.sort((a, b) => a.start - b.start);

        for (const highlight of highlights) {
            // Add text before highlight (escaped)
            highlightedHTML += this.escapeHtml(text.substring(lastIndex, highlight.start));

            // Add highlighted text with CSS class
            const highlightedPart = text.substring(highlight.start, highlight.start + highlight.length);
            const cssClass = this.getCSSClassForHighlight(highlight);

            console.log(`Highlighting "${highlightedPart}" with class "${cssClass}" (type: ${highlight.type})`);
            highlightedHTML += `<span class="${cssClass}">${this.escapeHtml(highlightedPart)}</span>`;

            lastIndex = highlight.start + highlight.length;
        }

        // Add remaining text (escaped)
        highlightedHTML += this.escapeHtml(text.substring(lastIndex));

        // Apply highlighting directly to contenteditable
        this.editor.innerHTML = highlightedHTML;

        console.log('Final highlighted HTML:', this.editor.innerHTML.substring(0, 200) + '...');

        // Restore cursor position
        this.restoreSelection(selection);

        // Clear the flag after a short delay to allow DOM to settle
        setTimeout(() => {
            this.isApplyingHighlights = false;
        }, 100);

        console.log('Applied CSS-based syntax highlighting to contenteditable element');
    }

    /**
     * Convert highlight data to appropriate CSS class
     */
    getCSSClassForHighlight(highlight) {
        // console.log('getCSSClassForHighlight called with:', highlight);

        // Handle LN variables with rotating colors
        if (highlight.class === 'syntax-ln-ref' && highlight.ln_number) {
            const colorIndex = ((highlight.ln_number - 1) % 8) + 1;
            return `syntax-ln-${colorIndex}`;
        }

        // The backend returns a "class" property directly - use it!
        if (highlight.class) {
            // console.log(`Using backend class: "${highlight.class}"`);
            return highlight.class;
        }

        // Fallback: Check different possible property names for type
        const type = highlight.type || highlight.token_type || highlight.kind || highlight.category;

        // Handle LN variables with specific colors
        if (type === 'ln_variable' && highlight.ln_number) {
            const colorIndex = ((highlight.ln_number - 1) % 8) + 1;
            return `syntax-ln-${colorIndex}`;
        }

        // Handle other types
        switch (type) {
            case 'comment':
                return 'syntax-comment';
            case 'number':
                return 'syntax-number';
            case 'operator':
                return 'syntax-operator';
            case 'function':
                return 'syntax-function';
            case 'parenthesis':
                return 'syntax-parenthesis';
            case 'error':
                return 'syntax-error';
            default:
                console.log(`Unknown highlight type: "${type}", using default`);
                return 'syntax-default';
        }
    }

    /**
     * Apply syntax highlights directly to contenteditable (inline approach)
     */
    applySyntaxHighlights(highlights, text) {
        console.log('applySyntaxHighlights called with:', highlights.length, 'highlights');

        if (!this.editor) {
            console.error('Editor element not found in applySyntaxHighlights!');
            return;
        }

        // Set flag to prevent infinite loops
        this.isApplyingHighlights = true;

        // Save cursor position before modifying innerHTML
        const selection = this.saveSelection();

        // Create highlighted version of text using inline spans
        let highlightedHTML = '';
        let lastIndex = 0;

        // Sort highlights by start position
        highlights.sort((a, b) => a.start - b.start);

        for (const highlight of highlights) {
            // Add text before highlight (escaped)
            highlightedHTML += this.escapeHtml(text.substring(lastIndex, highlight.start));

            // Add highlighted text with inline styles
            const highlightedPart = text.substring(highlight.start, highlight.start + highlight.length);
            const className = highlight.class || 'syntax-default';

            // Build style string with color and bold
            let styleProps = [];
            if (highlight.color) {
                styleProps.push(`color: ${highlight.color}`);
            }
            if (highlight.bold) {
                styleProps.push('font-weight: bold');
            }
            const style = styleProps.length > 0 ? `style="${styleProps.join('; ')};"` : '';

            highlightedHTML += `<span class="${className}" ${style}>${this.escapeHtml(highlightedPart)}</span>`;

            lastIndex = highlight.start + highlight.length;
        }

        // Add remaining text (escaped)
        highlightedHTML += this.escapeHtml(text.substring(lastIndex));

        // Apply highlighting directly to contenteditable
        this.editor.innerHTML = highlightedHTML;

        // Restore cursor position
        this.restoreSelection(selection);

        // Clear the flag after a short delay to allow DOM to settle
        setTimeout(() => {
            this.isApplyingHighlights = false;
        }, 100);

        console.log('Applied syntax highlighting to contenteditable element');
    }

    /**
     * Save current cursor/selection position
     */
    saveSelection() {
        const selection = window.getSelection();
        if (selection.rangeCount > 0) {
            const range = selection.getRangeAt(0);
            return {
                start: this.getTextOffset(range.startContainer, range.startOffset),
                end: this.getTextOffset(range.endContainer, range.endOffset)
            };
        }
        return { start: 0, end: 0 };
    }

    /**
     * Restore cursor/selection position
     */
    restoreSelection(savedSelection) {
        if (!savedSelection) return;

        try {
            const range = document.createRange();
            const selection = window.getSelection();

            const startPos = this.getNodeAndOffset(savedSelection.start);
            const endPos = this.getNodeAndOffset(savedSelection.end);

            if (startPos && endPos) {
                range.setStart(startPos.node, startPos.offset);
                range.setEnd(endPos.node, endPos.offset);
                selection.removeAllRanges();
                selection.addRange(range);
            }
        } catch (error) {
            console.warn('Could not restore selection:', error);
        }
    }

    /**
     * Handle calculation result from WebSocket
     */
    onCalculationResult(event) {
        const data = event.detail;
        // Handle single line calculation result
        console.log('Calculation result:', data);
    }
    
    /**
     * Handle batch calculation result from WebSocket
     */
    onBatchCalculationResult(event) {
        const data = event.detail;
        if (data.results) {
            this.updateResults(data.results);
        }
    }

    /**
     * Utility methods
     */
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    showCalculating(show) {
        const loadingOverlay = document.getElementById('loading-overlay');
        if (!loadingOverlay) return;

        if (show) {
            this.calculatingStartTime = performance.now();
            loadingOverlay.classList.add('visible');
        } else {
            // Only hide if minimum time has passed
            const elapsed = performance.now() - (this.calculatingStartTime || 0);
            const remainingTime = Math.max(0, this.minCalculatingTime - elapsed);

            if (remainingTime > 0) {
                setTimeout(() => {
                    loadingOverlay.classList.remove('visible');
                }, remainingTime);
            } else {
                loadingOverlay.classList.remove('visible');
            }
        }
    }

    showError(message) {
        const errorToast = document.getElementById('error-toast');
        const toastMessage = document.getElementById('toast-message');

        if (errorToast && toastMessage) {
            toastMessage.textContent = message;
            errorToast.classList.add('visible');

            // Auto-hide after 5 seconds
            setTimeout(() => {
                errorToast.classList.remove('visible');
            }, 5000);
        }
    }

    updateStatus() {
        const currentLineElement = document.getElementById('current-line');
        const totalLinesElement = document.getElementById('total-lines');

        if (currentLineElement) {
            currentLineElement.textContent = `Line: ${this.currentLine}`;
        }

        if (totalLinesElement) {
            totalLinesElement.textContent = `Total: ${this.totalLines}`;
        }
    }

    updateCalculationTime(time) {
        const calculationTimeElement = document.getElementById('calculation-time');
        if (calculationTimeElement) {
            calculationTimeElement.textContent = `Time: ${time}ms`;
        }
    }

    updateLastUpdate() {
        const lastUpdateElement = document.getElementById('last-update');
        if (lastUpdateElement) {
            const now = new Date();
            const timeString = now.toLocaleTimeString();
            lastUpdateElement.textContent = `Updated: ${timeString}`;
        }
    }

    /**
     * Undo/Redo functionality
     */
    saveUndoState() {
        const state = {
            value: this.editor.value,
            selectionStart: this.editor.selectionStart,
            selectionEnd: this.editor.selectionEnd
        };

        this.undoStack.push(state);

        // Limit undo stack size
        if (this.undoStack.length > this.maxUndoSteps) {
            this.undoStack.shift();
        }

        // Clear redo stack when new action is performed
        this.redoStack = [];
    }

    undo() {
        if (this.undoStack.length === 0) return;

        // Save current state to redo stack
        const currentState = {
            value: this.editor.value,
            selectionStart: this.editor.selectionStart,
            selectionEnd: this.editor.selectionEnd
        };
        this.redoStack.push(currentState);

        // Restore previous state
        const previousState = this.undoStack.pop();
        this.editor.value = previousState.value;
        this.editor.setSelectionRange(previousState.selectionStart, previousState.selectionEnd);

        // Update UI
        this.updateLineNumbers();
        this.scheduleCalculation();
        this.updateSyntaxHighlighting();
    }

    redo() {
        if (this.redoStack.length === 0) return;

        // Save current state to undo stack
        const currentState = {
            value: this.editor.value,
            selectionStart: this.editor.selectionStart,
            selectionEnd: this.editor.selectionEnd
        };
        this.undoStack.push(currentState);

        // Restore next state
        const nextState = this.redoStack.pop();
        this.editor.value = nextState.value;
        this.editor.setSelectionRange(nextState.selectionStart, nextState.selectionEnd);

        // Update UI
        this.updateLineNumbers();
        this.scheduleCalculation();
        this.updateSyntaxHighlighting();
    }

    /**
     * Special key handlers
     */
    handleEnterKey(event) {
        // Phase 1: Let contenteditable handle Enter key naturally
        // Don't prevent default - let the browser create new lines
        console.log('Phase 1: Enter key - letting browser handle naturally');
    }

    handleTabKey(event) {
        event.preventDefault();
        this.insertText('    '); // 4 spaces
    }

    handleCopy(event) {
        // Custom copy behavior - copy only numbers without units
        const selection = this.editor.value.substring(
            this.editor.selectionStart,
            this.editor.selectionEnd
        );

        if (selection) {
            // Extract numbers from selection
            const numbers = selection.match(/\d+(?:\.\d+)?/g);
            if (numbers && numbers.length > 0) {
                navigator.clipboard.writeText(numbers.join('\n'));
                event.preventDefault();
            }
        }
    }

    insertText(text) {
        const start = this.editor.selectionStart;
        const end = this.editor.selectionEnd;
        const value = this.editor.value;

        this.editor.value = value.substring(0, start) + text + value.substring(end);
        this.editor.setSelectionRange(start + text.length, start + text.length);

        // Trigger input event
        this.editor.dispatchEvent(new Event('input'));
    }



    /**
     * Public methods for external use
     */
    getText() {
        return this.getEditorText();
    }

    setText(text) {
        this.setEditorText(text);

        // Fix any broken content that might have been loaded
        this.fixBrokenContent();

        this.updateLineNumbers();
        this.scheduleCalculation();
        this.updateSyntaxHighlighting();
    }

    clear() {
        this.setText('');
    }

    focus() {
        this.editor.focus();
    }

    getSelectedText() {
        const selection = this.getSelection();
        const text = this.getEditorText();
        return text.substring(selection.start, selection.end);
    }

    replaceSelection(text) {
        this.insertText(text);
    }

    /**
     * ContentEditable helper methods
     */
    getEditorText() {
        return this.editor.textContent || '';
    }

    setEditorText(text) {
        this.editor.textContent = text;
    }

    getSelection() {
        const selection = window.getSelection();
        if (selection.rangeCount > 0) {
            const range = selection.getRangeAt(0);
            return {
                start: this.getTextOffset(range.startContainer, range.startOffset),
                end: this.getTextOffset(range.endContainer, range.endOffset)
            };
        }
        return { start: 0, end: 0 };
    }

    setSelection(start, end) {
        const range = document.createRange();
        const selection = window.getSelection();

        const startPos = this.getNodeAndOffset(start);
        const endPos = this.getNodeAndOffset(end);

        if (startPos && endPos) {
            range.setStart(startPos.node, startPos.offset);
            range.setEnd(endPos.node, endPos.offset);
            selection.removeAllRanges();
            selection.addRange(range);
        }
    }

    getTextOffset(node, offset) {
        let textOffset = 0;
        const walker = document.createTreeWalker(
            this.editor,
            NodeFilter.SHOW_TEXT,
            null,
            false
        );

        let currentNode;
        while (currentNode = walker.nextNode()) {
            if (currentNode === node) {
                return textOffset + offset;
            }
            textOffset += currentNode.textContent.length;
        }
        return textOffset;
    }

    getNodeAndOffset(textOffset) {
        let currentOffset = 0;
        const walker = document.createTreeWalker(
            this.editor,
            NodeFilter.SHOW_TEXT,
            null,
            false
        );

        let currentNode;
        while (currentNode = walker.nextNode()) {
            const nodeLength = currentNode.textContent.length;
            if (currentOffset + nodeLength >= textOffset) {
                return {
                    node: currentNode,
                    offset: textOffset - currentOffset
                };
            }
            currentOffset += nodeLength;
        }

        // If we reach here, return the last position
        if (currentNode) {
            return {
                node: currentNode,
                offset: currentNode.textContent.length
            };
        }

        return null;
    }

    /**
     * Set up scroll synchronization between expression and results columns
     */
    setupScrollSync() {
        console.log('Setting up scroll synchronization between columns');

        // Prevent infinite scroll loops
        this.isScrollSyncing = false;

        // Sync expression -> results
        this.editor.addEventListener('scroll', () => {
            if (this.isScrollSyncing) return;
            this.isScrollSyncing = true;

            if (this.resultsDisplay) {
                this.resultsDisplay.scrollTop = this.editor.scrollTop;
            }

            setTimeout(() => {
                this.isScrollSyncing = false;
            }, 10);
        });

        // Sync results -> expression
        if (this.resultsDisplay) {
            this.resultsDisplay.addEventListener('scroll', () => {
                if (this.isScrollSyncing) return;
                this.isScrollSyncing = true;

                this.editor.scrollTop = this.resultsDisplay.scrollTop;

                setTimeout(() => {
                    this.isScrollSyncing = false;
                }, 10);
            });
        }
    }
}

// Export for use in other modules
window.EditorManager = EditorManager;
