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

        // Smart incremental calculation
        this.dependencyTracker = new DependencyTracker();
        this.lastCalculatedContent = '';
        this.lastResults = [];
        this.useIncrementalCalculation = true; // Re-enabled with fallback approach

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

        // Step 2.1: Initialize overlay
        this.updateOverlay();

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

        // Schedule syntax highlighting
        this.scheduleSyntaxHighlighting();

        // Update overlay with small delay to ensure DOM is updated
        setTimeout(() => {
            this.updateOverlay();
        }, 0);
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

            // Multiple overlay updates to ensure perfect registration
            setTimeout(() => {
                this.updateOverlay();
                this.updateLineNumbers();

                // Second update to catch any DOM settling
                setTimeout(() => {
                    this.updateOverlay();
                }, 5);

                // Third update for stubborn cases
                setTimeout(() => {
                    this.updateOverlay();
                }, 15);
            }, 0);
        }
        
        // Handle arrow keys for navigation
        if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(event.key)) {
            // Let default arrow key behavior happen
            setTimeout(() => {
                this.updateCurrentLine();
                this.ensureCursorVisible();
            }, 0);
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
        // Use requestAnimationFrame for smoother scroll sync
        if (this.scrollSyncFrame) {
            cancelAnimationFrame(this.scrollSyncFrame);
        }

        this.scrollSyncFrame = requestAnimationFrame(() => {
            const scrollTop = this.editor.scrollTop;
            const scrollLeft = this.editor.scrollLeft;

            if (this.resultsDisplay) {
                this.resultsDisplay.scrollTop = scrollTop;
            }

            // Update line number scroll with precise synchronization
            if (this.editorLineNumbers) {
                this.editorLineNumbers.scrollTop = scrollTop;
            }
            if (this.resultsLineNumbers) {
                this.resultsLineNumbers.scrollTop = scrollTop;
            }

            // Sync syntax overlay scroll (both vertical and horizontal)
            if (this.syntaxOverlay) {
                this.syntaxOverlay.scrollTop = scrollTop;
                this.syntaxOverlay.scrollLeft = scrollLeft;
            }
        });
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
            PerformanceDebug.logDebounceSkip('calculation');
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
            // Don't log debounce skip for syntax highlighting to reduce console spam
        }

        this.syntaxTimeout = setTimeout(() => {
            this.updateSyntaxHighlighting();
        }, 25); // Even faster response - 25ms for near-instant highlighting
    }
    
    /**
     * Calculate all expressions (with smart incremental updates)
     */
    async calculateAll() {
        if (this.isCalculating) return;

        const text = this.getEditorText();
        const lines = text.split('\n');

        if (lines.length === 0) {
            this.updateResults([]);
            this.lastCalculatedContent = '';
            this.lastResults = [];
            return;
        }

        PerformanceDebug.startTimer('calculation_total');

        this.isCalculating = true;
        this.showCalculating(true);

        const startTime = performance.now();

        try {
            // Check if we can use incremental calculation
            if (this.useIncrementalCalculation && this.canUseIncrementalUpdate(text, lines)) {
                await this.calculateIncremental(text, lines);
            } else {
                await this.calculateFull(text, lines);
            }

            const endTime = performance.now();
            const calculationTime = Math.round(endTime - startTime);
            this.updateCalculationTime(calculationTime);

        } catch (error) {
            console.error('Calculation failed:', error);
            this.showError('Calculation failed: ' + error.message);
            PerformanceDebug.logEvent('calculation_error', { error: error.message });
        } finally {
            this.isCalculating = false;
            this.showCalculating(false);
        }
    }

    /**
     * Check if we can use incremental calculation
     */
    canUseIncrementalUpdate(text, lines) {
        // Use incremental if we have previous results and content hasn't changed dramatically
        if (!this.lastCalculatedContent || this.lastResults.length === 0) {
            return false;
        }

        // If line count changed significantly, do full recalculation
        const lineCountDiff = Math.abs(lines.length - this.lastResults.length);
        if (lineCountDiff > 5) {
            PerformanceDebug.logEvent('incremental_skipped', { reason: 'line_count_change', diff: lineCountDiff });
            return false;
        }

        return true;
    }

    /**
     * Perform incremental calculation (only changed lines and their dependents)
     */
    async calculateIncremental(text, lines) {
        PerformanceDebug.startTimer('calculation_incremental');

        // Find which lines have actually changed
        const changedLines = this.findChangedLines(lines);

        if (changedLines.length === 0) {
            // No changes, use cached results
            PerformanceDebug.endTimer('calculation_incremental', {
                changedLines: 0,
                strategy: 'cached_results'
            });
            return;
        }

        // Update dependency graph for changed lines
        changedLines.forEach(lineNum => {
            const oldContent = this.getLastLineContent(lineNum);
            const newContent = lines[lineNum - 1] || '';
            this.dependencyTracker.updateLineDependencies(lineNum, oldContent, newContent);
        });

        // Get all lines that need recalculation (changed + dependents)
        const linesToRecalc = this.dependencyTracker.getLinesToRecalculate(changedLines, lines);

        if (linesToRecalc.size === 0) {
            PerformanceDebug.endTimer('calculation_incremental', {
                changedLines: changedLines.length,
                strategy: 'no_recalc_needed'
            });
            return;
        }

        // Use selective calculation API
        const expressions = lines.map(line => line.trim());
        const lineNumbers = Array.from(linesToRecalc);

        PerformanceDebug.startTimer('calculation_api_call');
        const selectiveResults = await this.api.calculateSelective(expressions, lineNumbers);
        PerformanceDebug.endTimer('calculation_api_call', {
            expressionCount: expressions.length,
            calculatedLines: lineNumbers.length,
            strategy: 'selective'
        });

        // Merge selective results with cached results
        const mergedResults = this.mergeResults(selectiveResults, lineNumbers, lines.length);

        PerformanceDebug.startTimer('ui_update_results');
        this.updateResults(mergedResults);
        PerformanceDebug.endTimer('ui_update_results', {
            resultCount: mergedResults.length,
            updatedLines: lineNumbers.length
        });

        // Update cache
        this.lastCalculatedContent = text;
        this.lastResults = mergedResults;

        PerformanceDebug.endTimer('calculation_incremental', {
            changedLines: changedLines.length,
            recalculatedLines: linesToRecalc.size,
            totalLines: lines.length,
            strategy: 'incremental_success'
        });
    }

    /**
     * Perform full calculation (fallback)
     */
    async calculateFull(text, lines) {
        PerformanceDebug.startTimer('calculation_full');

        // Rebuild dependency graph
        this.dependencyTracker.buildDependencyGraph(lines);

        // Filter out empty lines and comments for calculation
        const expressions = lines.map(line => line.trim());

        PerformanceDebug.startTimer('calculation_api_call');
        // Use batch calculation for efficiency
        const results = await this.api.calculateBatch(expressions);
        PerformanceDebug.endTimer('calculation_api_call', {
            expressionCount: expressions.length,
            expressions: expressions.slice(0, 3), // Log first 3 expressions for context
            strategy: 'full_batch'
        });

        PerformanceDebug.startTimer('ui_update_results');
        this.updateResults(results);
        PerformanceDebug.endTimer('ui_update_results', { resultCount: results.length });

        // Update cache
        this.lastCalculatedContent = text;
        this.lastResults = results;

        PerformanceDebug.endTimer('calculation_full', {
            expressionCount: expressions.length,
            firstExpression: expressions[0] || 'empty',
            strategy: 'full_recalc'
        });
    }

    /**
     * Find which lines have changed since last calculation
     */
    findChangedLines(currentLines) {
        const changedLines = [];
        const lastLines = this.lastCalculatedContent.split('\n');

        const maxLines = Math.max(currentLines.length, lastLines.length);

        for (let i = 0; i < maxLines; i++) {
            const currentLine = currentLines[i] || '';
            const lastLine = lastLines[i] || '';

            if (currentLine !== lastLine) {
                changedLines.push(i + 1); // 1-based line numbers
            }
        }

        return changedLines;
    }

    /**
     * Get the last content for a specific line
     */
    getLastLineContent(lineNum) {
        if (!this.lastCalculatedContent) return '';
        const lastLines = this.lastCalculatedContent.split('\n');
        return lastLines[lineNum - 1] || '';
    }

    /**
     * Merge selective calculation results with cached results
     */
    mergeResults(selectiveResults, updatedLineNumbers, totalLines) {
        // Start with cached results or empty array
        const mergedResults = [...this.lastResults];

        // Ensure array is the right size
        while (mergedResults.length < totalLines) {
            mergedResults.push({ value: '', unit: '', error: null, highlights: [] });
        }

        // Update with new results
        updatedLineNumbers.forEach(lineNum => {
            const result = selectiveResults[lineNum];
            if (result) {
                mergedResults[lineNum - 1] = result; // Convert to 0-based index
            }
        });

        return mergedResults;
    }

    /**
     * Toggle incremental calculation on/off
     */
    toggleIncrementalCalculation() {
        this.useIncrementalCalculation = !this.useIncrementalCalculation;
        this.dependencyTracker.setEnabled(this.useIncrementalCalculation);

        console.log(`%c[CALC] Incremental calculation ${this.useIncrementalCalculation ? 'enabled' : 'disabled'}`,
                   `color: ${this.useIncrementalCalculation ? 'green' : 'red'}; font-weight: bold`);

        // Clear cache to force full recalculation on next update
        this.lastCalculatedContent = '';
        this.lastResults = [];

        return this.useIncrementalCalculation;
    }
    
    /**
     * Get exact line height to match textarea
     */
    getExactLineHeight() {
        const textareaStyle = window.getComputedStyle(this.editor);
        return parseFloat(textareaStyle.lineHeight) || (this.currentFontSize * 1.5);
    }





    /**
     * Update line numbers
     */
    updateLineNumbers() {
        PerformanceDebug.startTimer('ui_update_line_numbers');

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

            // Add extra spacing at bottom to match textarea padding behavior
            const extraSpacing = '<div class="line-number" style="height: 25px; visibility: hidden;"></div>';

            this.editorLineNumbers.innerHTML = lineNumbersHTML + extraSpacing;

            // Apply current font size to newly created line number elements
            const lineNumberElements = this.editorLineNumbers.querySelectorAll('.line-number');
            const exactLineHeight = this.getExactLineHeight();

            lineNumberElements.forEach(element => {
                element.style.fontSize = `${this.currentFontSize}px`;
                element.style.lineHeight = `${exactLineHeight}px`;
                element.style.height = `${exactLineHeight}px`;
                element.style.margin = '0';
                element.style.padding = '0';
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

                return `<div class="${className}">${lineNum}</div>`;
            }).join('');

            // Add extra spacing at bottom to match textarea padding behavior
            const extraSpacing = '<div class="line-number" style="height: 25px; visibility: hidden;"></div>';

            this.resultsLineNumbers.innerHTML = resultLineNumbersHTML + extraSpacing;

            // Apply current font size to newly created line number elements
            const lineNumberElements = this.resultsLineNumbers.querySelectorAll('.line-number');
            const exactLineHeight = this.getExactLineHeight();

            lineNumberElements.forEach(element => {
                element.style.fontSize = `${this.currentFontSize}px`;
                element.style.lineHeight = `${exactLineHeight}px`;
                element.style.height = `${exactLineHeight}px`;
                element.style.margin = '0';
                element.style.padding = '0';
            });
        }

        PerformanceDebug.endTimer('ui_update_line_numbers', {
            lineCount: lines.length,
            textLength: text.length
        });

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

        PerformanceDebug.startTimer('ui_render_results');

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
                // Show the comment text in results column with green formatting
                content = `<span class="syntax-comment">${this.escapeHtml(line.trim())}</span>`;
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

            // For comment lines, content already contains HTML, so don't escape it
            const displayContent = isComment ? content : this.escapeHtml(content);
            return `<div class="${className}">${displayContent}</div>`;
        }).join('');

        this.resultsDisplay.innerHTML = resultHTML;

        // SYNC FIX: Apply exact line height to result lines to match line numbers
        const exactLineHeight = this.getExactLineHeight();
        const resultLines = this.resultsDisplay.querySelectorAll('.result-line');
        resultLines.forEach(element => {
            element.style.fontSize = `${this.currentFontSize}px`;
            element.style.lineHeight = `${exactLineHeight}px`;
            element.style.height = `${exactLineHeight}px`;
            element.style.margin = '0';
            element.style.padding = '0';
        });

        PerformanceDebug.endTimer('ui_render_results', {
            lineCount: lines.length,
            resultCount: results.length,
            htmlLength: resultHTML.length
        });

        // SYNC FIX: Ensure line numbers are synchronized after results update
        this.updateLineNumbers();

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

    /**
     * Update syntax overlay with font synchronization (syntax highlighting handled separately)
     */
    updateOverlay() {
        if (!this.syntaxOverlay) {
            return;
        }

        PerformanceDebug.startTimer('ui_update_overlay');

        const text = this.getEditorText();

        // Sync overlay font metrics with textarea's actual computed styles
        const textareaStyle = window.getComputedStyle(this.editor);
        this.syntaxOverlay.style.fontSize = textareaStyle.fontSize;
        this.syntaxOverlay.style.lineHeight = textareaStyle.lineHeight;
        this.syntaxOverlay.style.fontFamily = textareaStyle.fontFamily;
        this.syntaxOverlay.style.padding = textareaStyle.padding;

        // Only update content if syntax highlighting has never been applied
        if (!this.lastHighlightedText) {
            // Initial fallback to plain text before first syntax highlighting
            this.syntaxOverlay.innerHTML = this.escapeHtml(text);
        }
        // If syntax highlighting is active, let it handle content updates

        // Ensure scroll position is synchronized
        this.syntaxOverlay.scrollTop = this.editor.scrollTop;
        this.syntaxOverlay.scrollLeft = this.editor.scrollLeft;

        PerformanceDebug.endTimer('ui_update_overlay', {
            textLength: text.length,
            lineCount: text.split('\n').length
        });
    }

    /**
     * Apply syntax highlights using modern overlay approach (like VS Code)
     */
    applySyntaxHighlightsWithCSS(highlights, text) {
        if (!this.syntaxOverlay || !highlights || highlights.length === 0) {
            // Fallback to plain text if no highlights
            if (this.syntaxOverlay) {
                this.syntaxOverlay.innerHTML = this.escapeHtml(text);
            }
            return;
        }

        // Performance optimization: prevent unnecessary updates
        this.isApplyingHighlights = true;

        try {
            // Convert text to highlighted HTML using backend highlight data
            const highlightedHTML = this.applyHighlightsToText(text, highlights);

            // Update overlay with highlighted content
            this.syntaxOverlay.innerHTML = highlightedHTML;

            // Ensure scroll position stays synchronized
            this.syntaxOverlay.scrollTop = this.editor.scrollTop;
            this.syntaxOverlay.scrollLeft = this.editor.scrollLeft;

        } catch (error) {
            console.error('Error applying syntax highlights:', error);
            // Fallback to plain text on error
            this.syntaxOverlay.innerHTML = this.escapeHtml(text);
        } finally {
            this.isApplyingHighlights = false;
        }
    }

    /**
     * Convert text and highlight data to HTML with CSS classes (efficient algorithm)
     */
    applyHighlightsToText(text, highlights) {
        if (!highlights || highlights.length === 0) {
            return this.escapeHtml(text);
        }

        // Sort highlights by start position for efficient processing
        const sortedHighlights = [...highlights].sort((a, b) => a.start - b.start);

        let result = '';
        let currentPos = 0;

        for (const highlight of sortedHighlights) {
            const start = highlight.start;
            const end = start + highlight.length;

            // Skip invalid highlights
            if (start < 0 || end > text.length || start >= end) {
                continue;
            }

            // Add any unhighlighted text before this highlight
            if (start > currentPos) {
                result += this.escapeHtml(text.substring(currentPos, start));
            }

            // Add the highlighted text with appropriate CSS class
            const highlightedText = text.substring(start, end);
            const cssClass = this.getCSSClassForHighlight(highlight);
            result += `<span class="${cssClass}">${this.escapeHtml(highlightedText)}</span>`;

            currentPos = end;
        }

        // Add any remaining unhighlighted text
        if (currentPos < text.length) {
            result += this.escapeHtml(text.substring(currentPos));
        }

        return result;
    }

    /**
     * Convert highlight data to appropriate CSS class
     */
    getCSSClassForHighlight(highlight) {
        // Backend returns exact CSS class names in the "class" property
        if (highlight.class) {
            // Handle special LN variable classes that need color rotation
            if (highlight.class === 'syntax-ln-ref' && highlight.ln_number) {
                const colorIndex = ((highlight.ln_number - 1) % 17) + 1; // Use all 17 LN colors
                return `syntax-ln-${colorIndex}`;
            }

            // For all other classes, use the backend's class name directly
            return highlight.class;
        }

        // Fallback for legacy highlight formats (shouldn't be needed with current backend)
        const type = highlight.type || highlight.token_type || highlight.kind || highlight.category;

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
            case 'paren':
                return 'syntax-paren';
            case 'error':
                return 'syntax-error';
            case 'ln_variable':
                if (highlight.ln_number) {
                    const colorIndex = ((highlight.ln_number - 1) % 17) + 1;
                    return `syntax-ln-${colorIndex}`;
                }
                return 'syntax-ln-ref';
            default:
                // Return empty string for unknown types (no highlighting)
                return '';
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
                    PerformanceDebug.logOverlayShow();
                }
            }, this.showOverlayThreshold);
        } else {
            // Clear the overlay timeout if calculation finished quickly
            if (this.overlayTimeout) {
                clearTimeout(this.overlayTimeout);
                this.overlayTimeout = null;
                PerformanceDebug.logOverlayHide('calculation_finished_quickly');
            }

            // Only hide if minimum time has passed and overlay is visible
            if (loadingOverlay.classList.contains('visible')) {
                const elapsed = performance.now() - (this.calculatingStartTime || 0);
                const remainingTime = Math.max(0, this.minCalculatingTime - elapsed);

                if (remainingTime > 0) {
                    setTimeout(() => {
                        loadingOverlay.classList.remove('visible');
                        PerformanceDebug.logOverlayHide('minimum_time_elapsed');
                    }, remainingTime);
                } else {
                    loadingOverlay.classList.remove('visible');
                    PerformanceDebug.logOverlayHide('calculation_complete');
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

    /**
     * Special key handlers
     */
    handleEnterKey(event) {
        // Update UI after Enter key creates new line
        setTimeout(() => {
            this.updateLineNumbers();
            this.updateCurrentLine();
            this.scheduleCalculation();
            this.updateOverlay();
        }, 0);
    }

    handleTabKey(event) {
        event.preventDefault();
        this.insertText('    ');
    }

    handleCopy(event) {
        const selectedText = this.getSelectedText();
        if (selectedText) {
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

        this.updateLineNumbers();
        this.updateOverlay();

        // Only schedule calculation if not explicitly skipped (e.g., during tab switches)
        if (!skipCalculation) {
            this.scheduleCalculation();
        }
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
        return this.editor.value || '';
    }

    setEditorText(text) {
        if (!this.editor) return;
        this.editor.value = text || '';
    }

    /**
     * Tab caching methods for performance optimization
     */
    getLastResults() {
        return this.lastResults || [];
    }

    getLastCalculatedContent() {
        return this.lastCalculatedContent || '';
    }

    setLastResults(results, content) {
        this.lastResults = results;
        this.lastCalculatedContent = content;
    }

    getSelection() {
        if (!this.editor) return { start: 0, end: 0 };
        return {
            start: this.editor.selectionStart || 0,
            end: this.editor.selectionEnd || 0
        };
    }

    setSelection(start, end) {
        if (!this.editor) return;
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
        // Use unitless line-height to match CSS exactly
        const lineHeight = '1.5';

        // Update both expression editor and results display
        if (this.editor) {
            this.editor.style.fontSize = `${this.currentFontSize}px`;
            this.editor.style.lineHeight = lineHeight;
        }

        if (this.resultsDisplay) {
            this.resultsDisplay.style.fontSize = `${this.currentFontSize}px`;
            this.resultsDisplay.style.lineHeight = lineHeight;
        }

        // Update syntax overlay to match new font size
        if (this.syntaxOverlay) {
            this.syntaxOverlay.style.fontSize = `${this.currentFontSize}px`;
            this.syntaxOverlay.style.lineHeight = lineHeight;
        }

        // Update line number containers
        if (this.editorLineNumbers) {
            this.editorLineNumbers.style.fontSize = `${this.currentFontSize}px`;
            this.editorLineNumbers.style.lineHeight = lineHeight;

            // Update all individual line number elements
            const lineNumberElements = this.editorLineNumbers.querySelectorAll('.line-number');
            lineNumberElements.forEach(element => {
                element.style.fontSize = `${this.currentFontSize}px`;
                element.style.lineHeight = lineHeight;
            });
        }

        if (this.resultsLineNumbers) {
            this.resultsLineNumbers.style.fontSize = `${this.currentFontSize}px`;
            this.resultsLineNumbers.style.lineHeight = lineHeight;

            // Update all individual line number elements
            const lineNumberElements = this.resultsLineNumbers.querySelectorAll('.line-number');
            lineNumberElements.forEach(element => {
                element.style.fontSize = `${this.currentFontSize}px`;
                element.style.lineHeight = lineHeight;
            });
        }

        // Update line numbers after font size change (this will recreate them with proper styling)
        this.updateLineNumbers();

        // Update syntax overlay after font size change
        this.updateOverlay();

        // Update existing result lines with new font size
        this.updateResultLinesFontSize();

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
     * Update font size of existing result lines
     */
    updateResultLinesFontSize() {
        if (!this.resultsDisplay) return;

        const exactLineHeight = this.getExactLineHeight();
        const resultLines = this.resultsDisplay.querySelectorAll('.result-line');

        resultLines.forEach(element => {
            element.style.fontSize = `${this.currentFontSize}px`;
            element.style.lineHeight = `${exactLineHeight}px`;
            element.style.height = `${exactLineHeight}px`;
        });
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
     * Ensure cursor is visible in textarea (fixes arrow key navigation issues)
     */
    ensureCursorVisible() {
        if (!this.editor) return;

        const textarea = this.editor;
        const cursorPosition = textarea.selectionStart;

        // Get the text up to cursor position
        const textBeforeCursor = textarea.value.substring(0, cursorPosition);
        const lines = textBeforeCursor.split('\n');
        const currentLineIndex = lines.length - 1;

        // Calculate line height based on current font size (more accurate)
        const computedStyle = getComputedStyle(textarea);
        let lineHeight;

        if (computedStyle.lineHeight === 'normal' || computedStyle.lineHeight === '1.5') {
            // Use current font size * 1.5 for unitless line-height
            lineHeight = this.currentFontSize * 1.5;
        } else {
            // Parse pixel value
            lineHeight = parseFloat(computedStyle.lineHeight) || (this.currentFontSize * 1.5);
        }

        const padding = parseFloat(computedStyle.paddingTop) || 8;
        const bottomPadding = parseFloat(computedStyle.paddingBottom) || 25;

        // Calculate cursor Y position
        const cursorY = currentLineIndex * lineHeight + padding;

        // Get textarea dimensions
        const textareaHeight = textarea.clientHeight;
        const scrollTop = textarea.scrollTop;

        // Check if cursor is below visible area (account for bottom padding)
        const visibleBottom = scrollTop + textareaHeight - bottomPadding - lineHeight; // Extra line height buffer

        let scrollChanged = false;

        if (cursorY > visibleBottom) {
            // Scroll down to show cursor with proper spacing
            textarea.scrollTop = cursorY - textareaHeight + bottomPadding + (lineHeight * 2);
            scrollChanged = true;
        }

        // Check if cursor is above visible area
        const visibleTop = scrollTop;
        if (cursorY < visibleTop) {
            // Scroll up to show cursor with some buffer
            textarea.scrollTop = Math.max(0, cursorY - lineHeight);
            scrollChanged = true;
        }

        // Note: Scroll sync will be handled automatically by the scroll event listener
    }

    /**
     * Set up scroll synchronization between expression and results columns
     */
    setupScrollSync() {
        // IMPROVED: Better scroll synchronization with requestAnimationFrame

        // Prevent infinite scroll loops
        this.isScrollSyncing = false;

        // Sync results -> expression (when user scrolls in results panel)
        if (this.resultsDisplay) {
            this.resultsDisplay.addEventListener('scroll', () => {
                if (this.isScrollSyncing) return;
                this.isScrollSyncing = true;

                // Use requestAnimationFrame for smooth sync
                requestAnimationFrame(() => {
                    this.editor.scrollTop = this.resultsDisplay.scrollTop;

                    setTimeout(() => {
                        this.isScrollSyncing = false;
                    }, 10);
                });
            });
        }
    }
}

// Export for use in other modules
window.EditorManager = EditorManager;
