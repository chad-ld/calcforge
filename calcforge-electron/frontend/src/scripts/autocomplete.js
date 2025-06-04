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
            this.editor.editor.addEventListener('keydown', this.onKeyDown);
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
        try {
            this.functions = await this.api.getFunctions();
            console.log('Loaded functions:', this.functions.length);
        } catch (error) {
            console.error('Failed to load functions:', error);
            // Use fallback function list
            this.functions = Object.keys(this.functionDescriptions).map(name => ({
                name: name,
                description: this.functionDescriptions[name].description
            }));
        }
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
                this.selectNext();
                break;
            case 'ArrowUp':
                event.preventDefault();
                this.selectPrevious();
                break;
            case 'Enter':
            case 'Tab':
                event.preventDefault();
                this.insertSelected();
                break;
            case 'Escape':
                event.preventDefault();
                this.hide();
                break;
        }
    }
    
    /**
     * Handle input events
     */
    onInput(event) {
        const cursorPosition = this.editor.editor.selectionStart;
        const text = this.editor.editor.value;
        
        // Find the current word being typed
        const wordInfo = this.getCurrentWord(text, cursorPosition);
        
        if (wordInfo && wordInfo.word.length > 0) {
            this.currentWord = wordInfo.word;
            this.wordStart = wordInfo.start;
            
            // Filter functions based on current word
            this.filterFunctions(this.currentWord);
            
            if (this.filteredFunctions.length > 0) {
                this.showAutocomplete();
            } else {
                this.hide();
            }
        } else {
            this.hide();
        }
    }
    
    /**
     * Get the current word being typed
     */
    getCurrentWord(text, cursorPosition) {
        // Find word boundaries
        let start = cursorPosition;
        let end = cursorPosition;
        
        // Move start backwards to find word start
        while (start > 0 && /[a-zA-Z_]/.test(text[start - 1])) {
            start--;
        }
        
        // Move end forwards to find word end
        while (end < text.length && /[a-zA-Z_]/.test(text[end])) {
            end++;
        }
        
        const word = text.substring(start, end);
        
        // Only show autocomplete for function-like words
        if (word.length > 0 && /^[a-zA-Z_]/.test(word)) {
            return { word, start, end };
        }
        
        return null;
    }
    
    /**
     * Filter functions based on input
     */
    filterFunctions(input) {
        const inputLower = input.toLowerCase();
        
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
        const editor = this.editor.editor;
        
        // Replace current word with function name
        const start = this.wordStart;
        const end = start + this.currentWord.length;
        
        const value = editor.value;
        const newValue = value.substring(0, start) + selectedFunc.name + '(' + value.substring(end);
        
        editor.value = newValue;
        editor.setSelectionRange(start + selectedFunc.name.length + 1, start + selectedFunc.name.length + 1);
        
        // Trigger input event
        editor.dispatchEvent(new Event('input'));
        
        this.hide();
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
