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
     * Update syntax highlighting - PHASE 1: DISABLED
     */
    async updateSyntaxHighlighting() {
        console.log('Phase 1: Syntax highlighting disabled - plain white text only');
        // Phase 1: No highlighting, just plain white text
        // This will be implemented in later phases
        return;
    }
    
    /**
     * Apply syntax highlights to overlay
     */
    applySyntaxHighlights(highlights, text) {
        console.log('applySyntaxHighlights called with:', highlights.length, 'highlights');
        if (!this.syntaxOverlay) {
            console.error('Syntax overlay not found in applySyntaxHighlights!');
            return;
        }

        // Create highlighted version of text
        let highlightedText = '';
        let lastIndex = 0;

        // Sort highlights by start position
        highlights.sort((a, b) => a.start - b.start);

        for (const highlight of highlights) {
            // Add text before highlight
            highlightedText += this.escapeHtml(text.substring(lastIndex, highlight.start));

            // Add highlighted text
            const highlightedPart = text.substring(highlight.start, highlight.start + highlight.length);
            const className = highlight.class || 'syntax-default';
            const style = highlight.color ? `color: ${highlight.color};` : '';

            console.log('Highlighting:', highlightedPart, 'with class:', className, 'and style:', style);
            highlightedText += `<span class="${className}" style="${style}">${this.escapeHtml(highlightedPart)}</span>`;

            lastIndex = highlight.start + highlight.length;
        }

        // Add remaining text
        highlightedText += this.escapeHtml(text.substring(lastIndex));

        console.log('Final highlighted text:', highlightedText);
        console.log('Syntax overlay element:', this.syntaxOverlay);
        console.log('Setting overlay innerHTML...');
        this.syntaxOverlay.innerHTML = highlightedText;
        console.log('Overlay innerHTML after setting:', this.syntaxOverlay.innerHTML);
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
