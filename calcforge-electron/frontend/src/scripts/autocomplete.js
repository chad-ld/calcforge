/**
 * CalcForge Autocomplete Manager
 * Handles function autocompletion and help tooltips
 */

class AutocompleteManager {
    constructor(api, editor) {
        this.api = api;
        this.editor = editor;
        this.popup = null;
        this.list = null;
        this.description = null;
        this.isVisible = false;
        this.selectedIndex = 0;
        this.functions = [];
        this.filteredFunctions = [];
        this.currentWord = '';
        this.wordStart = 0;
        
        // Unit completions for conversion
        this.units = [
            // Length units
            'meters', 'feet', 'inches', 'centimeters', 'millimeters', 'kilometers', 'miles', 'yards',
            // Weight units
            'kilograms', 'pounds', 'ounces', 'grams', 'tons',
            // Volume units
            'liters', 'gallons', 'quarts', 'pints', 'cups', 'milliliters',
            // Temperature units
            'celsius', 'fahrenheit', 'kelvin',
            // Time units
            'seconds', 'minutes', 'hours', 'days', 'weeks', 'months', 'years',
            // Currency units
            'dollars', 'euros', 'pounds', 'yen', 'canadian dollars', 'australian dollars'
        ];

        // Function parameter options
        this.functionParameters = {
            'sum': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'mean': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'median': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'mode': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'min': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'max': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'count': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'product': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'variance': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'stdev': ['above', 'below', 'LN1:LN5', '1,2,3,4,5', 'cg-above', 'cg-below'],
            'tc': ['24', '29.97', '30', '23.976', '25', '50', '59.94', '60'],
            'ar': ['1920x1080', '1280x720', '3840x2160', '2560x1440'],
            'tr': ['2', '0', '1', '3', '4'],
            'truncate': ['2', '0', '1', '3', '4']
        };

        // Function descriptions
        this.functionDescriptions = {
            'sum': {
                description: 'Adds numbers from a range of lines',
                examples: ['sum(LN1:LN5)', 'sum(1,3,5)', 'sum(above)', 'sum(below)']
            },
            'mean': {
                description: 'Calculates average of numbers from a range',
                examples: ['mean(LN1:LN5)', 'mean(1-10)']
            },
            'median': {
                description: 'Finds middle value in a range of numbers',
                examples: ['median(LN1:LN5)', 'median(1,2,3,4,5)']
            },
            'mode': {
                description: 'Finds most frequently occurring value',
                examples: ['mode(LN1:LN10)']
            },
            'min': {
                description: 'Finds smallest value in a range',
                examples: ['min(LN1:LN5)', 'min(10,20,30)']
            },
            'max': {
                description: 'Finds largest value in a range',
                examples: ['max(LN1:LN5)', 'max(10,20,30)']
            },
            'count': {
                description: 'Counts non-empty values in a range',
                examples: ['count(LN1:LN10)', 'count(above)']
            },
            'product': {
                description: 'Multiplies all numbers in a range',
                examples: ['product(LN1:LN5)', 'product(2,3,4)']
            },
            'variance': {
                description: 'Calculates variance of numbers in range',
                examples: ['variance(LN1:LN10)']
            },
            'stdev': {
                description: 'Calculates standard deviation of range',
                examples: ['stdev(LN1:LN10)', 'std(LN1:LN10)']
            },
            'tc': {
                description: 'Timecode calculation and conversion',
                examples: ['TC(24, 100)', 'TC(30, "00:01:00:00")', 'TC(29.97, "01:00:00;00")']
            },
            'ar': {
                description: 'Aspect ratio calculation for video dimensions',
                examples: ['AR("1920x1080", "?x720")', 'AR("16:9", "1280x?")']
            },
            'd': {
                description: 'Date arithmetic and business day calculations',
                examples: ['D("July 4, 2023")', 'D("July 4, 2023 + 30")', 'D("July 4, 2023 W+ 5")']
            },
            'tr': {
                description: 'Rounds number to specified decimal places',
                examples: ['TR(3.14159, 2)', 'truncate(10.567, 1)']
            },
            'truncate': {
                description: 'Rounds number to specified decimal places',
                examples: ['truncate(3.14159, 2)', 'TR(10.567, 1)']
            },
            'sqrt': {
                description: 'Calculates square root of a number',
                examples: ['sqrt(16)', 'sqrt(LN1)']
            },
            'sin': {
                description: 'Calculates sine of an angle (radians)',
                examples: ['sin(pi/2)', 'sin(0)', 'sin(radians(90))']
            },
            'cos': {
                description: 'Calculates cosine of an angle (radians)',
                examples: ['cos(0)', 'cos(pi)', 'cos(radians(60))']
            },
            'tan': {
                description: 'Calculates tangent of an angle (radians)',
                examples: ['tan(pi/4)', 'tan(radians(45))']
            },
            'log': {
                description: 'Calculates natural logarithm',
                examples: ['log(e)', 'log(10)', 'log(LN1)']
            },
            'log10': {
                description: 'Calculates base-10 logarithm',
                examples: ['log10(100)', 'log10(1000)']
            },
            'exp': {
                description: 'Calculates e raised to the power of x',
                examples: ['exp(1)', 'exp(LN1)']
            },
            'pow': {
                description: 'Raises first number to power of second',
                examples: ['pow(2, 3)', 'pow(LN1, 2)']
            },
            'abs': {
                description: 'Returns absolute value of number',
                examples: ['abs(-5)', 'abs(LN1)']
            },
            'ceil': {
                description: 'Rounds number up to nearest integer',
                examples: ['ceil(3.2)', 'ceil(-2.8)']
            },
            'floor': {
                description: 'Rounds number down to nearest integer',
                examples: ['floor(3.8)', 'floor(-2.2)']
            }
        };
        
        // Bind methods
        this.onKeyDown = this.onKeyDown.bind(this);
        this.onInput = this.onInput.bind(this);
        this.onItemClick = this.onItemClick.bind(this);
        this.onItemHover = this.onItemHover.bind(this);
    }
    
    /**
     * Initialize the autocomplete manager
     */
    async init() {
        this.popup = document.getElementById('autocomplete-popup');
        this.list = document.getElementById('autocomplete-list');
        this.description = document.getElementById('autocomplete-description');
        
        if (!this.popup || !this.list || !this.description) {
            console.error('Autocomplete elements not found');
            return false;
        }
        
        // Load functions from API
        await this.loadFunctions();
        
        // Set up event listeners
        if (this.editor && this.editor.editor) {
            // Use capture phase to ensure autocomplete handles keys before editor
            this.editor.editor.addEventListener('keydown', this.onKeyDown, true);
            this.editor.editor.addEventListener('input', this.onInput);
        }
        
        // Set up list event listeners
        this.list.addEventListener('click', this.onItemClick);
        this.list.addEventListener('mouseover', this.onItemHover);
        
        return true;
    }
    
    /**
     * Load available functions from API
     */
    async loadFunctions() {
        // Always start with a basic set of functions
        this.functions = [
            { name: 'sin', description: 'Sine function' },
            { name: 'cos', description: 'Cosine function' },
            { name: 'tan', description: 'Tangent function' },
            { name: 'sqrt', description: 'Square root function' },
            { name: 'sum', description: 'Sum of values' },
            { name: 'mean', description: 'Average of values' },
            { name: 'max', description: 'Maximum value' },
            { name: 'min', description: 'Minimum value' },
            { name: 'abs', description: 'Absolute value' },
            { name: 'floor', description: 'Floor function' },
            { name: 'ceil', description: 'Ceiling function' },
            { name: 'log', description: 'Natural logarithm' },
            { name: 'exp', description: 'Exponential function' },
            { name: 'median', description: 'Median value' },
            { name: 'mode', description: 'Mode value' },
            { name: 'count', description: 'Count values' },
            { name: 'product', description: 'Product of values' },
            { name: 'variance', description: 'Variance' },
            { name: 'stdev', description: 'Standard deviation' },
            { name: 'range', description: 'Range of values' }
        ];

        try {
            const apiFunctions = await this.api.getFunctions();
            console.log('Loaded functions from API:', apiFunctions ? apiFunctions.length : 0);

            if (apiFunctions && apiFunctions.length > 0) {
                // Merge API functions with basic functions, avoiding duplicates
                const existingNames = new Set(this.functions.map(f => f.name));
                for (const func of apiFunctions) {
                    if (!existingNames.has(func.name)) {
                        this.functions.push(func);
                    }
                }
            }
        } catch (error) {
            console.error('Failed to load functions from API:', error);
        }

        console.log('Autocomplete initialized with', this.functions.length, 'functions');
    }
    
    /**
     * Handle keydown events
     */
    onKeyDown(event) {
        if (!this.isVisible) {
            // Show autocomplete on Ctrl+Space
            if (event.ctrlKey && event.code === 'Space') {
                event.preventDefault();
                this.showAutocomplete();
                return;
            }
            return;
        }
        
        // Handle navigation when popup is visible
        switch (event.key) {
            case 'ArrowDown':
                event.preventDefault();
                event.stopPropagation();
                this.selectNext();
                return false;
            case 'ArrowUp':
                event.preventDefault();
                event.stopPropagation();
                this.selectPrevious();
                return false;
            case 'Enter':
            case 'Tab':
                if (this.filteredFunctions.length > 0) {
                    console.log('Autocomplete handling Enter/Tab - inserting:', this.filteredFunctions[this.selectedIndex].name);
                    event.preventDefault();
                    event.stopPropagation();
                    this.insertSelected();
                    return false;
                }
                break;
            case 'Escape':
                event.preventDefault();
                event.stopPropagation();
                this.hide();
                return false;
        }
    }
    
    /**
     * Handle input events
     */
    onInput(event) {
        // Get text and cursor position from textarea
        const text = this.editor.getEditorText();
        const selection = this.editor.getSelection();
        const cursorPosition = selection.start;

        // Check if we're on a comment line - disable autocomplete for comments
        const lines = text.split('\n');
        const textBeforeCursor = text.substring(0, cursorPosition);
        const currentLineNumber = textBeforeCursor.split('\n').length - 1;
        const currentLine = lines[currentLineNumber] || '';

        if (currentLine.trim().startsWith(':::')) {
            // We're on a comment line - hide autocomplete and exit
            this.hide();
            return;
        }

        // Handle backspace/delete operations more carefully
        if (event && (event.inputType === 'deleteContentBackward' || event.inputType === 'deleteContentForward')) {
            // For delete operations, only hide if we're not in a valid word anymore
            const wordInfo = this.getCurrentWord(text, cursorPosition);
            if (!wordInfo || wordInfo.word.length === 0) {
                this.hide();
                return;
            }
            // Continue processing if we're still in a valid word
        }

        // Quick exit for non-alphabetic characters at cursor
        const charAtCursor = text && cursorPosition > 0 ? text[cursorPosition - 1] : '';

        // Don't trigger autocomplete for numbers, operators, or spaces unless we're in a function context
        if (charAtCursor && /[0-9+\-*/=\s.]/.test(charAtCursor)) {
            // Only continue if we might be in a function parameter context
            const beforeCursor = text.substring(0, cursorPosition);
            if (!beforeCursor.includes('(') || beforeCursor.lastIndexOf('(') < beforeCursor.lastIndexOf(')')) {
                this.hide();
                return;
            }
        }

        // Find the current word being typed
        const wordInfo = this.getCurrentWord(text, cursorPosition);

        if (wordInfo && wordInfo.word.length > 0) {
            // Only process if the word has changed or context changed
            if (wordInfo.word !== this.currentWord || wordInfo.context !== this.currentContext) {
                this.currentWord = wordInfo.word;
                this.wordStart = wordInfo.start;
                this.currentContext = wordInfo.context || 'function';
                this.currentFunctionName = wordInfo.functionName;

                // Filter functions based on current word and context
                this.filterFunctions(this.currentWord, this.currentContext, this.currentFunctionName);

                if (this.filteredFunctions.length > 0) {
                    this.showAutocomplete();
                } else {
                    this.hide();
                }
            }
        } else {
            this.hide();
        }
    }
    
    /**
     * Get the current word being typed
     */
    getCurrentWord(text, cursorPosition) {
        // Safety check for undefined text
        if (!text || typeof text !== 'string' || cursorPosition < 0) {
            return null;
        }

        // Check for function parameter context
        const beforeCursor = text.substring(0, cursorPosition);
        const afterCursor = text.substring(cursorPosition);

        // Check if we're inside function parentheses
        const functionParamPattern = /(\w+)\s*\(\s*([^)]*)$/i;
        const functionParamMatch = functionParamPattern.exec(beforeCursor);

        if (functionParamMatch) {
            const functionName = functionParamMatch[1].toLowerCase();
            const paramText = functionParamMatch[2];

            // Check if this function has parameter suggestions
            if (this.functionParameters[functionName]) {
                // Find the current parameter being typed
                const paramParts = paramText.split(',');
                const currentParam = paramParts[paramParts.length - 1].trim();

                return {
                    word: currentParam,
                    start: cursorPosition - currentParam.length,
                    end: cursorPosition,
                    context: 'function_param',
                    functionName: functionName
                };
            }
        }

        // Check if we just typed a function with opening parenthesis (immediate parameter suggestions)
        const justAfterFunctionPattern = /(\w+)\s*\(\s*$/i;
        const justAfterFunctionMatch = justAfterFunctionPattern.exec(beforeCursor);

        if (justAfterFunctionMatch) {
            const functionName = justAfterFunctionMatch[1].toLowerCase();

            // Check if this function has parameter suggestions
            if (this.functionParameters[functionName]) {
                return {
                    word: '',
                    start: cursorPosition,
                    end: cursorPosition,
                    context: 'function_param',
                    functionName: functionName
                };
            }
        }

        // Check for unit conversion context (number + word + "to")
        const unitToPattern = /(\d+(?:\.\d+)?)\s+([a-zA-Z\s]+)\s+to\s+([a-zA-Z\s]*)$/i;
        const unitToMatch = unitToPattern.exec(beforeCursor);

        if (unitToMatch) {
            const targetUnit = unitToMatch[3];
            return {
                word: targetUnit,
                start: cursorPosition - targetUnit.length,
                end: cursorPosition,
                context: 'unit_target'
            };
        }

        // Pattern for source unit: "5 doll" (typing "dollars")
        const unitSourcePattern = /(\d+(?:\.\d+)?)\s+([a-zA-Z\s]+)$/i;
        const unitSourceMatch = unitSourcePattern.exec(beforeCursor);

        if (unitSourceMatch && !afterCursor.trim().startsWith('to')) {
            const sourceUnit = unitSourceMatch[2];
            return {
                word: sourceUnit,
                start: cursorPosition - sourceUnit.length,
                end: cursorPosition,
                context: 'unit_source'
            };
        }

        // Find word boundaries for functions
        let start = cursorPosition;
        let end = cursorPosition;

        // Move start backwards to find word start (only letters and underscore)
        while (start > 0 && /[a-zA-Z_]/.test(text[start - 1])) {
            start--;
        }

        // Move end forwards to find word end (only letters and underscore)
        while (end < text.length && /[a-zA-Z_]/.test(text[end])) {
            end++;
        }

        const word = text.substring(start, end);

        // Only show autocomplete for function-like words:
        // 1. Must start with a letter
        // 2. Must be at least 1 character
        // 3. Cursor must be within or at end of word
        // 4. Must not be preceded by a number (to avoid triggering on "100s")
        if (word.length > 0 && /^[a-zA-Z]/.test(word) && cursorPosition >= start && cursorPosition <= end) {
            // Check if there's a number immediately before this word
            const charBeforeWord = start > 0 ? text[start - 1] : '';
            if (/[0-9]/.test(charBeforeWord)) {
                return null; // Don't trigger autocomplete for things like "100s"
            }

            return { word, start, end, context: 'function' };
        }

        return null;
    }
    
    /**
     * Filter functions based on input
     */
    filterFunctions(input, context = 'function', functionName = null) {
        const inputLower = input.toLowerCase().trim();

        if (context === 'function_param' && functionName && this.functionParameters[functionName]) {
            // Filter function parameters - only show if input is empty or matches from start
            this.filteredFunctions = this.functionParameters[functionName]
                .filter(param => {
                    if (inputLower === '') return true;
                    return param.toLowerCase().startsWith(inputLower);
                })
                .map(param => ({ name: param, description: `Parameter: ${param}` }))
                .sort((a, b) => {
                    // Exact matches first
                    if (a.name.toLowerCase() === inputLower) return -1;
                    if (b.name.toLowerCase() === inputLower) return 1;

                    // Then by length
                    return a.name.length - b.name.length;
                });
        } else if (context === 'unit_source' || context === 'unit_target') {
            // Filter units - only match from start of word
            this.filteredFunctions = this.units
                .filter(unit => unit.toLowerCase().startsWith(inputLower))
                .map(unit => ({ name: unit, description: `Unit: ${unit}` }))
                .sort((a, b) => {
                    // Exact matches first
                    if (a.name.toLowerCase() === inputLower) return -1;
                    if (b.name.toLowerCase() === inputLower) return 1;

                    // Then by length
                    return a.name.length - b.name.length;
                });
        } else {
            // Filter functions - only match from start of word
            this.filteredFunctions = this.functions.filter(func =>
                func.name.toLowerCase().startsWith(inputLower)
            ).sort((a, b) => {
                // Exact matches first
                if (a.name.toLowerCase() === inputLower) return -1;
                if (b.name.toLowerCase() === inputLower) return 1;

                // Then by length (shorter first)
                if (a.name.length !== b.name.length) {
                    return a.name.length - b.name.length;
                }

                // Finally alphabetically
                return a.name.localeCompare(b.name);
            });
        }



        // Limit to 5 items maximum
        this.filteredFunctions = this.filteredFunctions.slice(0, 5);
        this.selectedIndex = 0;
    }
    
    /**
     * Show autocomplete popup
     */
    showAutocomplete() {
        if (this.filteredFunctions.length === 0) return;
        
        this.isVisible = true;
        this.popup.classList.add('visible');
        
        // Position popup near cursor
        this.positionPopup();
        
        // Update list
        this.updateList();
        
        // Update description
        this.updateDescription();
    }
    
    /**
     * Hide autocomplete popup
     */
    hide() {
        this.isVisible = false;
        this.popup.classList.remove('visible');
    }
    
    /**
     * Position popup near cursor
     */
    positionPopup() {
        const editor = this.editor.editor;
        const rect = editor.getBoundingClientRect();
        
        // Get cursor position (approximate)
        const lineHeight = parseInt(getComputedStyle(editor).lineHeight);
        const fontSize = parseInt(getComputedStyle(editor).fontSize);
        
        // Position below current line
        const top = rect.top + lineHeight + 5;
        const left = rect.left + 50; // Approximate cursor position
        
        this.popup.style.top = `${top}px`;
        this.popup.style.left = `${left}px`;
        
        // Ensure popup stays within viewport
        const popupRect = this.popup.getBoundingClientRect();
        const viewportWidth = window.innerWidth;
        const viewportHeight = window.innerHeight;
        
        if (popupRect.right > viewportWidth) {
            this.popup.style.left = `${viewportWidth - popupRect.width - 10}px`;
        }
        
        if (popupRect.bottom > viewportHeight) {
            this.popup.style.top = `${rect.top - popupRect.height - 5}px`;
        }
    }
    
    /**
     * Update function list
     */
    updateList() {
        this.list.innerHTML = '';
        
        this.filteredFunctions.forEach((func, index) => {
            const item = document.createElement('div');
            item.className = 'autocomplete-item';
            item.textContent = func.name;
            item.dataset.index = index;
            
            if (index === this.selectedIndex) {
                item.classList.add('selected');
            }
            
            this.list.appendChild(item);
        });
    }
    
    /**
     * Update description panel
     */
    updateDescription() {
        if (this.filteredFunctions.length === 0) return;
        
        const selectedFunc = this.filteredFunctions[this.selectedIndex];
        const funcInfo = this.functionDescriptions[selectedFunc.name.toLowerCase()];
        
        const nameElement = document.getElementById('function-name');
        const descElement = document.getElementById('function-description');
        const examplesElement = document.getElementById('function-examples');
        
        if (nameElement) {
            nameElement.textContent = selectedFunc.name.toUpperCase();
        }
        
        if (descElement) {
            descElement.textContent = funcInfo ? funcInfo.description : selectedFunc.description;
        }
        
        if (examplesElement && funcInfo && funcInfo.examples) {
            examplesElement.innerHTML = funcInfo.examples
                .map(example => `<code>${example}</code>`)
                .join('<br>');
        }
    }
    
    /**
     * Select next item
     */
    selectNext() {
        if (this.filteredFunctions.length === 0) return;
        
        this.selectedIndex = (this.selectedIndex + 1) % this.filteredFunctions.length;
        this.updateList();
        this.updateDescription();
    }
    
    /**
     * Select previous item
     */
    selectPrevious() {
        if (this.filteredFunctions.length === 0) return;
        
        this.selectedIndex = this.selectedIndex === 0 
            ? this.filteredFunctions.length - 1 
            : this.selectedIndex - 1;
        this.updateList();
        this.updateDescription();
    }
    
    /**
     * Insert selected function
     */
    insertSelected() {
        if (this.filteredFunctions.length === 0) return;

        const selectedFunc = this.filteredFunctions[this.selectedIndex];
        console.log('insertSelected called for:', selectedFunc.name, 'context:', this.currentContext);

        // Replace current word with function name
        const start = this.wordStart;
        const end = start + this.currentWord.length;

        const text = this.editor.getEditorText();
        let newText;
        let cursorPos;

        if (this.currentContext === 'function_param') {
            // For function parameters, just replace the current parameter
            newText = text.substring(0, start) + selectedFunc.name + text.substring(end);
            cursorPos = start + selectedFunc.name.length;
        } else if (this.currentContext === 'unit_source' || this.currentContext === 'unit_target') {
            // For units, just replace the word
            if (this.currentContext === 'unit_target') {
                newText = text.substring(0, start) + selectedFunc.name + text.substring(end);
                cursorPos = start + selectedFunc.name.length;
            } else {
                // For source units, add " to " after
                newText = text.substring(0, start) + selectedFunc.name + ' to ' + text.substring(end);
                cursorPos = start + selectedFunc.name.length + 4;
            }
        } else {
            // For functions, add parentheses
            newText = text.substring(0, start) + selectedFunc.name + '(' + text.substring(end);
            cursorPos = start + selectedFunc.name.length + 1;
        }

        // Update the contenteditable element
        this.editor.setEditorText(newText);
        this.editor.setSelection(cursorPos, cursorPos);

        // Trigger input event on the contenteditable element
        this.editor.editor.dispatchEvent(new Event('input'));

        // For functions, immediately check for parameter suggestions
        if (this.currentContext === 'function') {
            // Small delay to let the input event process, then check for parameter autocomplete
            setTimeout(() => {
                this.onInput({ inputType: 'insertText' });
            }, 50);
        } else {
            this.hide();
        }
    }
    
    /**
     * Handle item click
     */
    onItemClick(event) {
        const item = event.target.closest('.autocomplete-item');
        if (item) {
            this.selectedIndex = parseInt(item.dataset.index);
            this.insertSelected();
        }
    }
    
    /**
     * Handle item hover
     */
    onItemHover(event) {
        const item = event.target.closest('.autocomplete-item');
        if (item) {
            this.selectedIndex = parseInt(item.dataset.index);
            this.updateList();
            this.updateDescription();
        }
    }
}

// Export for use in other modules
window.AutocompleteManager = AutocompleteManager;
