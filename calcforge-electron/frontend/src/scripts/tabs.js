/**
 * CalcForge Tab Manager
 * Handles worksheet tabs and switching between them
 */

class TabManager {
    constructor(api, editor) {
        this.api = api;
        this.editor = editor;
        this.tabs = new Map();
        this.activeTabId = null;
        this.nextTabId = 1;
        this.tabBar = null;
        this.newTabButton = null;
        
        // Bind methods
        this.onTabClick = this.onTabClick.bind(this);
        this.onTabClose = this.onTabClose.bind(this);
        this.onNewTab = this.onNewTab.bind(this);
        this.onTabDoubleClick = this.onTabDoubleClick.bind(this);
    }
    
    /**
     * Initialize the tab manager
     */
    init() {
        this.tabBar = document.getElementById('tab-bar');
        this.newTabButton = document.getElementById('new-tab-button');
        
        if (!this.tabBar) {
            console.error('Tab bar element not found');
            return false;
        }
        
        // Set up event listeners
        if (this.newTabButton) {
            this.newTabButton.addEventListener('click', this.onNewTab);
        }
        
        // Create initial tab
        this.createTab('Sheet 1');
        
        return true;
    }
    
    /**
     * Create a new tab
     */
    createTab(name = null, content = '') {
        const tabId = this.nextTabId++;
        
        if (!name) {
            name = `Sheet ${tabId}`;
        }
        
        // Create tab data
        const tabData = {
            id: tabId,
            name: name,
            content: content,
            modified: false,
            created: new Date(),
            lastModified: new Date()
        };
        
        this.tabs.set(tabId, tabData);
        
        // Create tab element
        this.createTabElement(tabData);
        
        // Switch to new tab
        this.switchToTab(tabId);
        
        return tabId;
    }
    
    /**
     * Create tab DOM element
     */
    createTabElement(tabData) {
        const tabElement = document.createElement('div');
        tabElement.className = 'tab';
        tabElement.dataset.tabId = tabData.id;
        
        tabElement.innerHTML = `
            <span class="tab-name">${this.escapeHtml(tabData.name)}</span>
            <button class="tab-close" title="Close tab">×</button>
        `;
        
        // Add event listeners
        tabElement.addEventListener('click', this.onTabClick);
        tabElement.addEventListener('dblclick', this.onTabDoubleClick);
        
        const closeButton = tabElement.querySelector('.tab-close');
        closeButton.addEventListener('click', this.onTabClose);
        
        this.tabBar.appendChild(tabElement);
        
        return tabElement;
    }
    
    /**
     * Handle tab click
     */
    onTabClick(event) {
        if (event.target.classList.contains('tab-close')) {
            return; // Close button handles its own click
        }
        
        const tabElement = event.currentTarget;
        const tabId = parseInt(tabElement.dataset.tabId);
        this.switchToTab(tabId);
    }
    
    /**
     * Handle tab close
     */
    onTabClose(event) {
        event.stopPropagation();
        
        const tabElement = event.target.closest('.tab');
        const tabId = parseInt(tabElement.dataset.tabId);
        
        this.closeTab(tabId);
    }
    
    /**
     * Handle new tab button
     */
    onNewTab() {
        this.createTab();
    }
    
    /**
     * Handle tab double-click for renaming
     */
    onTabDoubleClick(event) {
        const tabElement = event.currentTarget;
        const tabId = parseInt(tabElement.dataset.tabId);
        this.renameTab(tabId);
    }
    
    /**
     * Switch to a specific tab
     */
    switchToTab(tabId) {
        if (!this.tabs.has(tabId)) {
            console.error(`Tab ${tabId} not found`);
            return false;
        }
        
        // Save current tab content
        if (this.activeTabId && this.editor) {
            const currentTab = this.tabs.get(this.activeTabId);
            if (currentTab) {
                currentTab.content = this.editor.getText();
                currentTab.lastModified = new Date();
            }
        }
        
        // Update active tab
        this.activeTabId = tabId;
        const newTab = this.tabs.get(tabId);
        
        // Update tab visual state
        this.updateTabStates();
        
        // Load tab content into editor
        if (this.editor) {
            this.editor.setText(newTab.content);
        }
        
        // Update worksheets in API for cross-sheet references
        this.updateWorksheets();
        
        return true;
    }
    
    /**
     * Close a tab
     */
    closeTab(tabId) {
        if (!this.tabs.has(tabId)) {
            return false;
        }
        
        // Don't close the last tab
        if (this.tabs.size <= 1) {
            return false;
        }
        
        const tab = this.tabs.get(tabId);
        
        // Check if tab has unsaved changes
        if (tab.modified) {
            const confirmed = confirm(`Tab "${tab.name}" has unsaved changes. Close anyway?`);
            if (!confirmed) {
                return false;
            }
        }
        
        // Remove tab element
        const tabElement = this.tabBar.querySelector(`[data-tab-id="${tabId}"]`);
        if (tabElement) {
            tabElement.remove();
        }
        
        // Remove from tabs map
        this.tabs.delete(tabId);
        
        // If this was the active tab, switch to another
        if (this.activeTabId === tabId) {
            const remainingTabs = Array.from(this.tabs.keys());
            if (remainingTabs.length > 0) {
                this.switchToTab(remainingTabs[0]);
            } else {
                this.activeTabId = null;
            }
        }
        
        return true;
    }
    
    /**
     * Rename a tab
     */
    renameTab(tabId) {
        const tab = this.tabs.get(tabId);
        if (!tab) return false;
        
        const newName = prompt('Enter new tab name:', tab.name);
        if (newName && newName.trim() && newName !== tab.name) {
            tab.name = newName.trim();
            tab.lastModified = new Date();
            
            // Update tab element
            const tabElement = this.tabBar.querySelector(`[data-tab-id="${tabId}"]`);
            if (tabElement) {
                const nameElement = tabElement.querySelector('.tab-name');
                nameElement.textContent = tab.name;
            }
            
            // Update worksheets in API
            this.updateWorksheets();
            
            return true;
        }
        
        return false;
    }
    
    /**
     * Update visual state of all tabs
     */
    updateTabStates() {
        this.tabBar.querySelectorAll('.tab').forEach(tabElement => {
            const tabId = parseInt(tabElement.dataset.tabId);
            const isActive = tabId === this.activeTabId;
            
            tabElement.classList.toggle('active', isActive);
        });
    }
    
    /**
     * Update worksheets in API for cross-sheet references
     */
    async updateWorksheets() {
        try {
            const worksheets = {};
            
            for (const [tabId, tab] of this.tabs) {
                worksheets[tabId] = {
                    id: tabId,
                    name: tab.name,
                    content: tab.content
                };
            }
            
            await this.api.updateWorksheets(worksheets);
        } catch (error) {
            console.error('Failed to update worksheets:', error);
        }
    }
    
    /**
     * Mark current tab as modified
     */
    markCurrentTabModified() {
        if (this.activeTabId) {
            const tab = this.tabs.get(this.activeTabId);
            if (tab) {
                tab.modified = true;
                tab.lastModified = new Date();
                
                // Update tab visual indicator
                const tabElement = this.tabBar.querySelector(`[data-tab-id="${this.activeTabId}"]`);
                if (tabElement) {
                    tabElement.classList.add('modified');
                }
            }
        }
    }
    
    /**
     * Mark current tab as saved
     */
    markCurrentTabSaved() {
        if (this.activeTabId) {
            const tab = this.tabs.get(this.activeTabId);
            if (tab) {
                tab.modified = false;
                
                // Update tab visual indicator
                const tabElement = this.tabBar.querySelector(`[data-tab-id="${this.activeTabId}"]`);
                if (tabElement) {
                    tabElement.classList.remove('modified');
                }
            }
        }
    }
    
    /**
     * Get all tabs data
     */
    getAllTabs() {
        const tabsData = {};
        
        for (const [tabId, tab] of this.tabs) {
            tabsData[tab.name] = tab.content;
        }
        
        return tabsData;
    }
    
    /**
     * Load tabs from data
     */
    loadTabs(tabsData) {
        // Clear existing tabs
        this.tabs.clear();
        this.tabBar.innerHTML = '';
        this.activeTabId = null;
        this.nextTabId = 1;
        
        // Create tabs from data
        let firstTabId = null;
        for (const [name, content] of Object.entries(tabsData)) {
            const tabId = this.createTab(name, content);
            if (firstTabId === null) {
                firstTabId = tabId;
            }
        }
        
        // If no tabs were created, create a default one
        if (this.tabs.size === 0) {
            firstTabId = this.createTab('Sheet 1');
        }
        
        // Switch to first tab
        if (firstTabId) {
            this.switchToTab(firstTabId);
        }
    }
    
    /**
     * Get current tab
     */
    getCurrentTab() {
        return this.activeTabId ? this.tabs.get(this.activeTabId) : null;
    }
    
    /**
     * Get current tab content
     */
    getCurrentTabContent() {
        const currentTab = this.getCurrentTab();
        return currentTab ? currentTab.content : '';
    }
    
    /**
     * Utility method
     */
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }
}

// Export for use in other modules
window.TabManager = TabManager;
