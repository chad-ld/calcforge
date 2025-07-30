/**
 * CalcForge Electron Integration
 * Handles Electron-specific functionality and native file operations
 */

class ElectronIntegration {
    constructor() {
        this.isElectron = typeof window.electronAPI !== 'undefined';
        this.appInfo = null;
        
        // Bind methods
        this.onMenuNewTab = this.onMenuNewTab.bind(this);
        this.onMenuOpenFile = this.onMenuOpenFile.bind(this);
        this.onMenuSaveFile = this.onMenuSaveFile.bind(this);
        this.onMenuUndo = this.onMenuUndo.bind(this);
        this.onMenuRedo = this.onMenuRedo.bind(this);
        this.onMenuClearAll = this.onMenuClearAll.bind(this);
        this.onMenuStayOnTop = this.onMenuStayOnTop.bind(this);
        this.onMenuAbout = this.onMenuAbout.bind(this);
        this.onMenuHelp = this.onMenuHelp.bind(this);
    }
    
    /**
     * Initialize Electron integration
     */
    async init() {
        if (!this.isElectron) {
            console.log('Running in browser mode - Electron features disabled');
            return false;
        }
        
        console.log('Initializing Electron integration...');
        
        try {
            // Get app information
            this.appInfo = await window.electronAPI.getAppInfo();
            console.log('App info:', this.appInfo);
            
            // Set up menu event listeners
            this.setupMenuListeners();
            
            // Update UI for Electron
            this.updateUIForElectron();
            
            // Set initial stay on top state
            const stayOnTopCheckbox = document.getElementById('stay-on-top');
            if (stayOnTopCheckbox) {
                stayOnTopCheckbox.addEventListener('change', (e) => {
                    window.electronAPI.setStayOnTop(e.target.checked);
                });
            }
            
            console.log('Electron integration initialized successfully');
            return true;
            
        } catch (error) {
            console.error('Failed to initialize Electron integration:', error);
            return false;
        }
    }
    
    /**
     * Set up menu event listeners
     */
    setupMenuListeners() {
        if (!this.isElectron) return;
        
        window.electronAPI.onMenuNewTab(this.onMenuNewTab);
        window.electronAPI.onMenuOpenFile(this.onMenuOpenFile);
        window.electronAPI.onMenuSaveFile(this.onMenuSaveFile);
        window.electronAPI.onMenuUndo(this.onMenuUndo);
        window.electronAPI.onMenuRedo(this.onMenuRedo);
        window.electronAPI.onMenuClearAll(this.onMenuClearAll);
        window.electronAPI.onMenuStayOnTop(this.onMenuStayOnTop);
        window.electronAPI.onMenuAbout(this.onMenuAbout);
        window.electronAPI.onMenuHelp(this.onMenuHelp);
    }
    
    /**
     * Update UI for Electron environment
     */
    updateUIForElectron() {
        // Update title with version
        if (this.appInfo) {
            const title = document.querySelector('.app-title');
            if (title) {
                title.textContent = `${this.appInfo.name} v${this.appInfo.version}`;
            }
        }
        
        // Hide web-specific elements
        const webOnlyElements = document.querySelectorAll('.web-only');
        webOnlyElements.forEach(element => {
            element.style.display = 'none';
        });
        
        // Show electron-specific elements
        const electronOnlyElements = document.querySelectorAll('.electron-only');
        electronOnlyElements.forEach(element => {
            element.style.display = 'block';
        });
        
        // Update platform-specific styling
        document.body.classList.add(`platform-${this.appInfo.platform}`);
        
        if (this.appInfo.isDev) {
            document.body.classList.add('development');
        }
    }
    
    /**
     * Menu event handlers
     */
    onMenuNewTab() {
        if (window.calcForgeApp && window.calcForgeApp.tabs) {
            window.calcForgeApp.tabs.createTab();
        }
    }
    
    async onMenuOpenFile() {
        try {
            const result = await window.electronAPI.showOpenDialog();
            
            if (!result.canceled && result.filePaths.length > 0) {
                const filePath = result.filePaths[0];
                const fileResult = await window.electronAPI.readFile(filePath);
                
                if (fileResult.success) {
                    const data = JSON.parse(fileResult.data);
                    
                    if (window.calcForgeApp && window.calcForgeApp.tabs) {
                        window.calcForgeApp.tabs.loadTabs(data);
                        this.showSuccess('File loaded successfully');
                    }
                } else {
                    this.showError('Failed to read file: ' + fileResult.error);
                }
            }
        } catch (error) {
            console.error('Open file failed:', error);
            this.showError('Failed to open file: ' + error.message);
        }
    }
    
    async onMenuSaveFile() {
        try {
            const result = await window.electronAPI.showSaveDialog();
            
            if (!result.canceled && result.filePath) {
                if (window.calcForgeApp && window.calcForgeApp.tabs) {
                    const data = window.calcForgeApp.tabs.getAllTabs();
                    const json = JSON.stringify(data, null, 2);
                    
                    const writeResult = await window.electronAPI.writeFile(result.filePath, json);
                    
                    if (writeResult.success) {
                        // Mark current tab as saved
                        window.calcForgeApp.tabs.markCurrentTabSaved();
                        this.showSuccess('File saved successfully');
                    } else {
                        this.showError('Failed to save file: ' + writeResult.error);
                    }
                }
            }
        } catch (error) {
            console.error('Save file failed:', error);
            this.showError('Failed to save file: ' + error.message);
        }
    }
    
    onMenuUndo() {
        if (window.calcForgeApp && window.calcForgeApp.editor) {
            window.calcForgeApp.editor.undo();
        }
    }
    
    onMenuRedo() {
        if (window.calcForgeApp && window.calcForgeApp.editor) {
            window.calcForgeApp.editor.redo();
        }
    }
    
    onMenuClearAll() {
        if (window.calcForgeApp) {
            window.calcForgeApp.clearCurrentTab();
        }
    }
    
    onMenuStayOnTop(event, enabled) {
        const stayOnTopCheckbox = document.getElementById('stay-on-top');
        if (stayOnTopCheckbox) {
            stayOnTopCheckbox.checked = enabled;
        }
    }
    
    onMenuAbout() {
        this.showAboutDialog();
    }
    
    onMenuHelp() {
        if (window.calcForgeApp) {
            window.calcForgeApp.showHelp();
        }
    }
    
    /**
     * Show about dialog
     */
    showAboutDialog() {
        const aboutHtml = `
            <div class="about-dialog">
                <h2>${this.appInfo.name}</h2>
                <p>Version: ${this.appInfo.version}</p>
                <p>Platform: ${this.appInfo.platform} (${this.appInfo.arch})</p>
                <p>Electron: ${window.electronAPI.versions.electron}</p>
                <p>Chrome: ${window.electronAPI.versions.chrome}</p>
                <p>Node.js: ${window.electronAPI.versions.node}</p>
                <br>
                <p>Advanced calculator with timecode, unit conversion, and cross-sheet references.</p>
                <p>© 2024 CalcForge Team</p>
            </div>
        `;
        
        // Create modal for about dialog
        this.showModal('About CalcForge', aboutHtml);
    }
    
    /**
     * Show modal dialog
     */
    showModal(title, content) {
        // Remove existing modal if any
        const existingModal = document.getElementById('electron-modal');
        if (existingModal) {
            existingModal.remove();
        }
        
        // Create modal
        const modal = document.createElement('div');
        modal.id = 'electron-modal';
        modal.className = 'modal-overlay visible';
        modal.innerHTML = `
            <div class="modal">
                <div class="modal-header">
                    <h2>${title}</h2>
                    <button class="modal-close" onclick="this.closest('.modal-overlay').remove()">×</button>
                </div>
                <div class="modal-content">
                    ${content}
                </div>
            </div>
        `;
        
        document.body.appendChild(modal);
        
        // Close on overlay click
        modal.addEventListener('click', (e) => {
            if (e.target === modal) {
                modal.remove();
            }
        });
    }
    
    /**
     * Show success message
     */
    showSuccess(message) {
        console.log('Success:', message);
        // Could integrate with native notifications here
    }
    
    /**
     * Show error message
     */
    showError(message) {
        console.error('Error:', message);
        
        if (window.calcForgeApp) {
            window.calcForgeApp.showError(message);
        }
    }
    
    /**
     * Window controls
     */
    minimizeWindow() {
        if (this.isElectron) {
            window.electronAPI.minimizeWindow();
        }
    }
    
    maximizeWindow() {
        if (this.isElectron) {
            window.electronAPI.maximizeWindow();
        }
    }
    
    closeWindow() {
        if (this.isElectron) {
            window.electronAPI.closeWindow();
        }
    }
    
    /**
     * Check if running in Electron
     */
    isElectronApp() {
        return this.isElectron;
    }
    
    /**
     * Get app information
     */
    getAppInfo() {
        return this.appInfo;
    }
    
    /**
     * Cleanup
     */
    destroy() {
        if (!this.isElectron) return;
        
        // Remove all listeners
        window.electronAPI.removeAllListeners('menu-new-tab');
        window.electronAPI.removeAllListeners('menu-open-file');
        window.electronAPI.removeAllListeners('menu-save-file');
        window.electronAPI.removeAllListeners('menu-undo');
        window.electronAPI.removeAllListeners('menu-redo');
        window.electronAPI.removeAllListeners('menu-clear-all');
        window.electronAPI.removeAllListeners('menu-stay-on-top');
        window.electronAPI.removeAllListeners('menu-about');
        window.electronAPI.removeAllListeners('menu-help');
    }
}

// Export for use in other modules
window.ElectronIntegration = ElectronIntegration;
