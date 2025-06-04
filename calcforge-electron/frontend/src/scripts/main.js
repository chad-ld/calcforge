/**
 * CalcForge Main Application
 * Initializes and coordinates all application components
 */

class CalcForgeApp {
    constructor() {
        this.api = null;
        this.editor = null;
        this.tabs = null;
        this.autocomplete = null;
        this.splitter = null;
        this.electron = null;
        this.isInitialized = false;

        // Bind methods
        this.onDOMContentLoaded = this.onDOMContentLoaded.bind(this);
        this.onBeforeUnload = this.onBeforeUnload.bind(this);
        this.onResize = this.onResize.bind(this);
        this.setupEventListeners = this.setupEventListeners.bind(this);
    }
    
    /**
     * Initialize the application
     */
    async init() {
        console.log('Initializing CalcForge...');
        
        try {
            // Initialize API connection
            this.api = new CalcForgeAPI();
            const connected = await this.api.connect();
            
            if (!connected) {
                this.showError('Failed to connect to CalcForge backend. Please ensure the server is running.');
                return false;
            }
            
            // Initialize editor
            this.editor = new EditorManager(this.api);
            if (!this.editor.init()) {
                this.showError('Failed to initialize editor');
                return false;
            }
            
            // Initialize tab manager
            this.tabs = new TabManager(this.api, this.editor);
            if (!this.tabs.init()) {
                this.showError('Failed to initialize tab manager');
                return false;
            }
            
            // Initialize autocomplete
            this.autocomplete = new AutocompleteManager(this.api, this.editor);
            await this.autocomplete.init();

            // Initialize Electron integration
            if (typeof window.ElectronIntegration !== 'undefined') {
                this.electron = new ElectronIntegration();
                await this.electron.init();
            }

            // Initialize splitter
            this.initSplitter();

            // Set up event listeners
            this.setupEventListeners();

            // Set up file operations (only for web mode)
            if (!this.electron || !this.electron.isElectronApp()) {
                this.setupFileOperations();
            }

            this.isInitialized = true;
            console.log('CalcForge initialized successfully');

            return true;
            
        } catch (error) {
            console.error('Failed to initialize CalcForge:', error);
            this.showError('Application initialization failed: ' + error.message);
            return false;
        }
    }
    
    /**
     * Set up event listeners
     */
    setupEventListeners() {
        // Window events
        window.addEventListener('beforeunload', this.onBeforeUnload);
        window.addEventListener('resize', this.onResize);
        
        // Header controls
        const newTabBtn = document.getElementById('new-tab-btn');
        if (newTabBtn) {
            newTabBtn.addEventListener('click', () => this.tabs.createTab());
        }
        
        const saveBtn = document.getElementById('save-btn');
        if (saveBtn) {
            saveBtn.addEventListener('click', () => this.saveFile());
        }
        
        const loadBtn = document.getElementById('load-btn');
        if (loadBtn) {
            loadBtn.addEventListener('click', () => this.loadFile());
        }
        
        const helpBtn = document.getElementById('help-btn');
        if (helpBtn) {
            helpBtn.addEventListener('click', () => this.showHelp());
        }
        
        const stayOnTopCheckbox = document.getElementById('stay-on-top');
        if (stayOnTopCheckbox) {
            stayOnTopCheckbox.addEventListener('change', (e) => {
                this.setStayOnTop(e.target.checked);
            });
        }
        
        // Panel controls
        const clearBtn = document.getElementById('clear-btn');
        if (clearBtn) {
            clearBtn.addEventListener('click', () => this.clearCurrentTab());
        }
        
        const undoBtn = document.getElementById('undo-btn');
        if (undoBtn) {
            undoBtn.addEventListener('click', () => this.editor.undo());
        }
        
        const redoBtn = document.getElementById('redo-btn');
        if (redoBtn) {
            redoBtn.addEventListener('click', () => this.editor.redo());
        }
        
        const copyResultsBtn = document.getElementById('copy-results-btn');
        if (copyResultsBtn) {
            copyResultsBtn.addEventListener('click', () => this.copyResults());
        }
        
        const exportBtn = document.getElementById('export-btn');
        if (exportBtn) {
            exportBtn.addEventListener('click', () => this.exportResults());
        }
        
        // Error toast close
        const toastClose = document.getElementById('toast-close');
        if (toastClose) {
            toastClose.addEventListener('click', () => this.hideError());
        }
        
        // Help modal close
        const helpClose = document.getElementById('help-close');
        if (helpClose) {
            helpClose.addEventListener('click', () => this.hideHelp());
        }
        
        // Modal overlay clicks
        const helpModal = document.getElementById('help-modal');
        if (helpModal) {
            helpModal.addEventListener('click', (e) => {
                if (e.target === helpModal) {
                    this.hideHelp();
                }
            });
        }
        
        // Keyboard shortcuts
        document.addEventListener('keydown', (e) => {
            if (e.ctrlKey || e.metaKey) {
                switch (e.key) {
                    case 's':
                        e.preventDefault();
                        this.saveFile();
                        break;
                    case 'o':
                        e.preventDefault();
                        this.loadFile();
                        break;
                    case 't':
                        e.preventDefault();
                        this.tabs.createTab();
                        break;
                    case 'w':
                        e.preventDefault();
                        if (this.tabs.activeTabId) {
                            this.tabs.closeTab(this.tabs.activeTabId);
                        }
                        break;
                    case 'h':
                        e.preventDefault();
                        this.showHelp();
                        break;
                }
            }
            
            if (e.key === 'F1') {
                e.preventDefault();
                this.showHelp();
            }
        });
    }
    
    /**
     * Initialize splitter for resizing panels
     */
    initSplitter() {
        this.splitter = document.getElementById('splitter');
        if (!this.splitter) return;
        
        let isResizing = false;
        let startX = 0;
        let startLeftWidth = 0;
        
        this.splitter.addEventListener('mousedown', (e) => {
            isResizing = true;
            startX = e.clientX;
            
            const editorPanel = document.querySelector('.editor-panel');
            if (editorPanel) {
                startLeftWidth = editorPanel.offsetWidth;
            }
            
            document.body.style.cursor = 'col-resize';
            document.body.style.userSelect = 'none';
            
            e.preventDefault();
        });
        
        document.addEventListener('mousemove', (e) => {
            if (!isResizing) return;
            
            const deltaX = e.clientX - startX;
            const newLeftWidth = startLeftWidth + deltaX;
            const minWidth = 300;
            const maxWidth = window.innerWidth - 300;
            
            if (newLeftWidth >= minWidth && newLeftWidth <= maxWidth) {
                const editorPanel = document.querySelector('.editor-panel');
                const resultsPanel = document.querySelector('.results-panel');
                
                if (editorPanel && resultsPanel) {
                    editorPanel.style.flex = `0 0 ${newLeftWidth}px`;
                    resultsPanel.style.flex = '1';
                }
            }
        });
        
        document.addEventListener('mouseup', () => {
            if (isResizing) {
                isResizing = false;
                document.body.style.cursor = '';
                document.body.style.userSelect = '';
            }
        });
    }
    
    /**
     * Set up file operations
     */
    setupFileOperations() {
        // File input for loading
        const fileInput = document.createElement('input');
        fileInput.type = 'file';
        fileInput.accept = '.json,.cf';
        fileInput.style.display = 'none';
        document.body.appendChild(fileInput);
        
        fileInput.addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (file) {
                this.loadFileFromInput(file);
            }
        });
        
        this.fileInput = fileInput;
    }
    
    /**
     * File operations
     */
    saveFile() {
        try {
            const data = this.tabs.getAllTabs();
            const json = JSON.stringify(data, null, 2);
            const blob = new Blob([json], { type: 'application/json' });
            const url = URL.createObjectURL(blob);
            
            const a = document.createElement('a');
            a.href = url;
            a.download = 'calcforge-worksheet.json';
            a.click();
            
            URL.revokeObjectURL(url);
            
            // Mark current tab as saved
            this.tabs.markCurrentTabSaved();
            
            this.showSuccess('File saved successfully');
            
        } catch (error) {
            console.error('Save failed:', error);
            this.showError('Failed to save file: ' + error.message);
        }
    }
    
    loadFile() {
        this.fileInput.click();
    }
    
    async loadFileFromInput(file) {
        try {
            const text = await file.text();
            const data = JSON.parse(text);
            
            this.tabs.loadTabs(data);
            this.showSuccess('File loaded successfully');
            
        } catch (error) {
            console.error('Load failed:', error);
            this.showError('Failed to load file: ' + error.message);
        }
    }
    
    /**
     * UI operations
     */
    clearCurrentTab() {
        if (confirm('Clear all expressions in current tab?')) {
            this.editor.clear();
            this.tabs.markCurrentTabModified();
        }
    }
    
    copyResults() {
        const resultsDisplay = document.getElementById('results-display');
        if (resultsDisplay) {
            const text = resultsDisplay.textContent;
            navigator.clipboard.writeText(text).then(() => {
                this.showSuccess('Results copied to clipboard');
            }).catch((error) => {
                this.showError('Failed to copy results');
            });
        }
    }
    
    exportResults() {
        try {
            const resultsDisplay = document.getElementById('results-display');
            if (resultsDisplay) {
                const text = resultsDisplay.textContent;
                const blob = new Blob([text], { type: 'text/plain' });
                const url = URL.createObjectURL(blob);
                
                const a = document.createElement('a');
                a.href = url;
                a.download = 'calcforge-results.txt';
                a.click();
                
                URL.revokeObjectURL(url);
                this.showSuccess('Results exported successfully');
            }
        } catch (error) {
            this.showError('Failed to export results');
        }
    }
    
    setStayOnTop(enabled) {
        // This would be handled by Electron main process
        console.log('Stay on top:', enabled);
    }
    
    showHelp() {
        const helpModal = document.getElementById('help-modal');
        if (helpModal) {
            helpModal.classList.add('visible');
        }
    }
    
    hideHelp() {
        const helpModal = document.getElementById('help-modal');
        if (helpModal) {
            helpModal.classList.remove('visible');
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
                this.hideError();
            }, 5000);
        }
    }
    
    hideError() {
        const errorToast = document.getElementById('error-toast');
        if (errorToast) {
            errorToast.classList.remove('visible');
        }
    }
    
    showSuccess(message) {
        // For now, just log success messages
        console.log('Success:', message);
    }
    
    /**
     * Event handlers
     */
    onBeforeUnload(event) {
        // Check for unsaved changes
        if (this.tabs) {
            for (const [tabId, tab] of this.tabs.tabs) {
                if (tab.modified) {
                    event.preventDefault();
                    event.returnValue = 'You have unsaved changes. Are you sure you want to leave?';
                    return event.returnValue;
                }
            }
        }
    }
    
    onResize() {
        // Handle window resize if needed
        if (this.autocomplete && this.autocomplete.isVisible) {
            this.autocomplete.positionPopup();
        }
    }
    
    /**
     * Cleanup
     */
    destroy() {
        if (this.api) {
            this.api.disconnect();
        }
        
        window.removeEventListener('beforeunload', this.onBeforeUnload);
        window.removeEventListener('resize', this.onResize);
        
        this.isInitialized = false;
    }
}

// Initialize application when DOM is ready
document.addEventListener('DOMContentLoaded', async () => {
    window.calcForgeApp = new CalcForgeApp();
    await window.calcForgeApp.init();
});

// Export for global access
window.CalcForgeApp = CalcForgeApp;
