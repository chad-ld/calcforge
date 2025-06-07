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
        this.overlayTimeout = null;
        this.calculationDelay = 100; // ms - fast but still debounced for typing
        this.calculatingStartTime = null;
        this.minCalculatingTime = 100; // Reduced minimum time to prevent lag
        this.showOverlayThreshold = 200; // Only show overlay if calculation takes longer than this
        
        // Removed undo/redo functionality for simplicity

        // Font size management
        this.currentFontSize = 14; // Default font size
        this.minFontSize = 8;
        this.maxFontSize = 32;

        // Syntax highlighting optimization
        this.lastHighlightedText = '';
        this.isHandlingEnter = false;
        
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
        this.editor = document.getElementById('expression-editor'); // Now a textarea
        this.syntaxOverlay = document.getElementById('syntax-overlay');
        this.resultsDisplay = document.getElementById('results-display');
        this.editorLineNumbers = document.getElementById('editor-line-numbers');
        this.resultsLineNumbers = document.getElementById('results-line-numbers');

        if (!this.editor) {
            console.error('Editor element not found');
            return false;
        }

        // Initialize textarea-based editor with overlay

        // Set up event listeners for textarea
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

        // No need to fix content with textarea - it's always clean text

        // Load saved font size
        this.loadFontSize();

        return true;
    }
    
    /**
     * Handle text input
     */
    onInput(event) {
        // Update line numbers
        this.updateLineNumbers();

        // Debounced calculation
        this.scheduleCalculation();

        // Syntax highlighting completely disabled for now
        // this.scheduleSyntaxHighlighting();
    }
    
    /**
     * Handle key events
     */
    onKeyDown(event) {
        // Handle special key combinations
        if (event.ctrlKey || event.metaKey) {
            switch (event.key) {
                case 'a':
                    // Select all - let default behavior happen
                    break;
                case 'c':
                    // Copy - handle in separate method
                    this.handleCopy(event);
                    break;
                case '+':
                case '=':
                    // Increase font size (handles both + and = keys)
                    this.increaseFontSize();
                    event.preventDefault();
                    break;
                case '-':
                case '_':
                    // Decrease font size (handles both - and _ keys)
                    this.decreaseFontSize();
                    event.preventDefault();
                    break;
                case '0':
                    // Reset font size
                    this.resetFontSize();
                    event.preventDefault();
                    break;
            }
        }
        
        // Handle Enter key
        if (event.key === 'Enter') {
            // Let browser handle Enter completely naturally
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
        }, 150); // Reduced debounce since we made highlighting less aggressive
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

            // Apply current font size to newly created line number elements
            const lineNumberElements = this.editorLineNumbers.querySelectorAll('.line-number');
            const lineHeight = Math.round(this.currentFontSize * 1.5);
            lineNumberElements.forEach(element => {
                element.style.fontSize = `${this.currentFontSize}px`;
                element.style.lineHeight = `${lineHeight}px`;
                element.style.height = `${lineHeight}px`;
            });
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

            // Apply current font size to newly created line number elements
            const lineNumberElements = this.resultsLineNumbers.querySelectorAll('.line-number');
            const lineHeight = Math.round(this.currentFontSize * 1.5);
            lineNumberElements.forEach(element => {
                element.style.fontSize = `${this.currentFontSize}px`;
                element.style.lineHeight = `${lineHeight}px`;
                element.style.height = `${lineHeight}px`;
            });
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
        // CSS-based syntax highlighting enabled

        if (!this.editor || this.isApplyingHighlights || this.isHandlingEnter || this.disableSyntaxHighlighting) {
            return;
        }

        const text = this.getEditorText();
        if (!text.trim()) {
            return;
        }

        // Prevent unnecessary updates if text hasn't changed
        if (this.lastHighlightedText === text) {
            return;
        }

        try {
            // Get highlights from API
            const highlights = await this.api.getSyntaxHighlighting(text);
            if (highlights && highlights.length > 0) {
                this.applySyntaxHighlightsWithCSS(highlights, text);
                this.lastHighlightedText = text;
            }
        } catch (error) {
            console.error('Syntax highlighting failed:', error);
            // Don't break the editor if highlighting fails
        }
    }

    // Removed contenteditable-specific methods - not needed with textarea

    /**
     * Apply syntax highlights using overlay approach (DISABLED - will be replaced)
     */
    applySyntaxHighlightsWithCSS(highlights, text) {
        // Completely disabled - will be replaced with overlay approach
        return;
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
                // Unknown highlight type, using default
                return 'syntax-default';
        }
    }



    // Removed contenteditable selection methods - textarea handles this natively

    /**
     * Handle calculation result from WebSocket
     */
    onCalculationResult(event) {
        const data = event.detail;
        // Handle single line calculation result
        // Handle calculation result
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
            // Only show overlay after a delay to avoid flashing for quick calculations
            this.overlayTimeout = setTimeout(() => {
                if (this.isCalculating) { // Only show if still calculating
                    loadingOverlay.classList.add('visible');
                }
            }, this.showOverlayThreshold);
        } else {
            // Clear the overlay timeout if calculation finished quickly
            if (this.overlayTimeout) {
                clearTimeout(this.overlayTimeout);
                this.overlayTimeout = null;
            }

            // Only hide if minimum time has passed and overlay is visible
            if (loadingOverlay.classList.contains('visible')) {
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

    // Removed all undo/redo functionality for simplicity

    /**
     * Special key handlers
     */
    handleEnterKey(event) {
        // Don't prevent default - let browser handle Enter completely naturally
        // Just update UI after the browser creates the line
        setTimeout(() => {
            this.updateLineNumbers();
            this.updateCurrentLine();
            this.scheduleCalculation();
            // Syntax highlighting completely disabled for now
            // this.scheduleSyntaxHighlighting();
        }, 0);
    }

    handleTabKey(event) {
        event.preventDefault();
        this.insertText('    '); // 4 spaces
    }

    handleCopy(event) {
        // Custom copy behavior - copy only numbers without units
        const selectedText = this.getSelectedText();

        if (selectedText) {
            // Extract numbers from selection
            const numbers = selectedText.match(/\d+(?:\.\d+)?/g);
            if (numbers && numbers.length > 0) {
                navigator.clipboard.writeText(numbers.join('\n'));
                event.preventDefault();
            }
        }
    }

    insertText(text) {
        const selection = this.getSelection();
        const currentText = this.getEditorText();

        const newText = currentText.substring(0, selection.start) + text + currentText.substring(selection.end);
        this.setEditorText(newText);
        this.setSelection(selection.start + text.length, selection.start + text.length);

        // Trigger input event
        this.editor.dispatchEvent(new Event('input'));
    }



    /**
     * Public methods for external use
     */
    getText() {
        return this.getEditorText();
    }

    setText(text, skipCalculation = false) {
        this.setEditorText(text);

        // No need to fix content with textarea - it's always clean text

        this.updateLineNumbers();

        // Only schedule calculation if not explicitly skipped (e.g., during tab switches)
        if (!skipCalculation) {
            this.scheduleCalculation();
        }

        // Syntax highlighting completely disabled for now
        // this.updateSyntaxHighlighting();
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
     * Textarea helper methods
     */
    getEditorText() {
        if (!this.editor) return '';
        // Simple textarea value - no HTML parsing needed
        return this.editor.value || '';
    }

    setEditorText(text) {
        if (!this.editor) return;
        // Simple textarea value assignment
        this.editor.value = text || '';
    }

    getSelection() {
        if (!this.editor) return { start: 0, end: 0 };
        // Simple textarea selection
        return {
            start: this.editor.selectionStart || 0,
            end: this.editor.selectionEnd || 0
        };
    }

    setSelection(start, end) {
        if (!this.editor) return;
        // Simple textarea selection
        this.editor.selectionStart = start;
        this.editor.selectionEnd = end;
        this.editor.focus();
    }

    /**
     * Font size management methods
     */
    increaseFontSize() {
        if (this.currentFontSize < this.maxFontSize) {
            this.currentFontSize += 1;
            this.updateFontSize();
        }
    }

    decreaseFontSize() {
        if (this.currentFontSize > this.minFontSize) {
            this.currentFontSize -= 1;
            this.updateFontSize();
        }
    }

    resetFontSize() {
        this.currentFontSize = 14; // Reset to default
        this.updateFontSize();
    }

    updateFontSize() {
        const lineHeight = Math.round(this.currentFontSize * 1.5);

        // Update both expression editor and results display
        if (this.editor) {
            this.editor.style.fontSize = `${this.currentFontSize}px`;
            this.editor.style.lineHeight = `${lineHeight}px`;
        }

        if (this.resultsDisplay) {
            this.resultsDisplay.style.fontSize = `${this.currentFontSize}px`;
            this.resultsDisplay.style.lineHeight = `${lineHeight}px`;
        }

        // Update line number containers
        if (this.editorLineNumbers) {
            this.editorLineNumbers.style.fontSize = `${this.currentFontSize}px`;
            this.editorLineNumbers.style.lineHeight = `${lineHeight}px`;

            // Update all individual line number elements
            const lineNumberElements = this.editorLineNumbers.querySelectorAll('.line-number');
            lineNumberElements.forEach(element => {
                element.style.fontSize = `${this.currentFontSize}px`;
                element.style.lineHeight = `${lineHeight}px`;
                element.style.height = `${lineHeight}px`;
            });
        }

        if (this.resultsLineNumbers) {
            this.resultsLineNumbers.style.fontSize = `${this.currentFontSize}px`;
            this.resultsLineNumbers.style.lineHeight = `${lineHeight}px`;

            // Update all individual line number elements
            const lineNumberElements = this.resultsLineNumbers.querySelectorAll('.line-number');
            lineNumberElements.forEach(element => {
                element.style.fontSize = `${this.currentFontSize}px`;
                element.style.lineHeight = `${lineHeight}px`;
                element.style.height = `${lineHeight}px`;
            });
        }

        // Update line numbers after font size change (this will recreate them with proper styling)
        this.updateLineNumbers();

        // Store font size preference
        localStorage.setItem('calcforge-font-size', this.currentFontSize.toString());

        // Show brief feedback about font size change
        this.showFontSizeFeedback();
    }

    /**
     * Load saved font size from localStorage
     */
    loadFontSize() {
        const savedSize = localStorage.getItem('calcforge-font-size');
        if (savedSize) {
            const size = parseInt(savedSize, 10);
            if (size >= this.minFontSize && size <= this.maxFontSize) {
                this.currentFontSize = size;
                this.updateFontSize();
            }
        }
    }

    /**
     * Show brief feedback about font size change
     */
    showFontSizeFeedback() {
        // Create or update font size indicator
        let indicator = document.getElementById('font-size-indicator');
        if (!indicator) {
            indicator = document.createElement('div');
            indicator.id = 'font-size-indicator';
            indicator.style.cssText = `
                position: fixed;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
                background: rgba(0, 0, 0, 0.8);
                color: white;
                padding: 8px 16px;
                border-radius: 4px;
                font-family: var(--font-mono);
                font-size: 14px;
                z-index: 9999;
                pointer-events: none;
                opacity: 0;
                transition: opacity 0.2s ease;
            `;
            document.body.appendChild(indicator);
        }

        // Update text and show
        indicator.textContent = `Font Size: ${this.currentFontSize}px`;
        indicator.style.opacity = '1';

        // Hide after 1 second
        clearTimeout(this.fontSizeTimeout);
        this.fontSizeTimeout = setTimeout(() => {
            indicator.style.opacity = '0';
        }, 1000);
    }

    /**
     * Set up scroll synchronization between expression and results columns
     */
    setupScrollSync() {
        // Setting up scroll synchronization between columns

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
