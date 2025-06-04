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
        this.calculationDelay = 300; // ms
        
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
        this.syntaxOverlay = document.getElementById('syntax-overlay');
        this.editorLineNumbers = document.getElementById('editor-line-numbers');
        this.resultsLineNumbers = document.getElementById('results-line-numbers');
        
        if (!this.editor) {
            console.error('Editor element not found');
            return false;
        }
        
        // Set up event listeners
        this.editor.addEventListener('input', this.onInput);
        this.editor.addEventListener('keydown', this.onKeyDown);
        this.editor.addEventListener('scroll', this.onScroll);
        this.editor.addEventListener('selectionchange', this.onSelectionChange);
        
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
        
        // Update syntax highlighting
        this.updateSyntaxHighlighting();
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
     * Calculate all expressions
     */
    async calculateAll() {
        if (this.isCalculating) return;
        
        const text = this.editor.value;
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
        const text = this.editor.value;
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
                
                return `<span class="${className}">${lineNum}</span>`;
            }).join('\n');
            
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
                
                return `<span class="${className}">${displayText}</span>`;
            }).join('\n');
            
            this.resultsLineNumbers.innerHTML = resultLineNumbersHTML;
        }
        
        // Update status
        this.updateStatus();
    }
    
    /**
     * Update current line number
     */
    updateCurrentLine() {
        const cursorPosition = this.editor.selectionStart;
        const textBeforeCursor = this.editor.value.substring(0, cursorPosition);
        this.currentLine = textBeforeCursor.split('\n').length;
        
        this.updateLineNumbers();
        this.updateStatus();
    }
    
    /**
     * Update results display
     */
    updateResults(results) {
        if (!this.resultsDisplay) return;
        
        const text = this.editor.value;
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
     * Update syntax highlighting
     */
    async updateSyntaxHighlighting() {
        if (!this.syntaxOverlay) return;
        
        const text = this.editor.value;
        if (!text.trim()) {
            this.syntaxOverlay.innerHTML = '';
            return;
        }
        
        try {
            const highlights = await this.api.getSyntaxHighlighting(text);
            this.applySyntaxHighlights(highlights, text);
        } catch (error) {
            console.error('Syntax highlighting failed:', error);
        }
    }
    
    /**
     * Apply syntax highlights to overlay
     */
    applySyntaxHighlights(highlights, text) {
        if (!this.syntaxOverlay) return;
        
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
            
            highlightedText += `<span class="${className}" style="${style}">${this.escapeHtml(highlightedPart)}</span>`;
            
            lastIndex = highlight.start + highlight.length;
        }
        
        // Add remaining text
        highlightedText += this.escapeHtml(text.substring(lastIndex));
        
        this.syntaxOverlay.innerHTML = highlightedText;
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
        if (loadingOverlay) {
            loadingOverlay.classList.toggle('visible', show);
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
        // Get current line indentation
        const cursorPosition = this.editor.selectionStart;
        const textBeforeCursor = this.editor.value.substring(0, cursorPosition);
        const currentLineStart = textBeforeCursor.lastIndexOf('\n') + 1;
        const currentLine = textBeforeCursor.substring(currentLineStart);
        const indentMatch = currentLine.match(/^(\s*)/);
        const indent = indentMatch ? indentMatch[1] : '';

        // Insert newline with same indentation
        event.preventDefault();
        const newText = '\n' + indent;
        this.insertText(newText);
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
        return this.editor.value;
    }

    setText(text) {
        this.editor.value = text;
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
        return this.editor.value.substring(
            this.editor.selectionStart,
            this.editor.selectionEnd
        );
    }

    replaceSelection(text) {
        this.insertText(text);
    }
}

// Export for use in other modules
window.EditorManager = EditorManager;
