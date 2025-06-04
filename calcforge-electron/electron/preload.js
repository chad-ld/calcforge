/**
 * CalcForge Electron Preload Script
 * Provides secure API for renderer process to communicate with main process
 */

const { contextBridge, ipcRenderer } = require('electron');

// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld('electronAPI', {
    // App information
    getAppInfo: () => ipcRenderer.invoke('get-app-info'),
    
    // Window controls
    minimizeWindow: () => ipcRenderer.invoke('minimize-window'),
    maximizeWindow: () => ipcRenderer.invoke('maximize-window'),
    closeWindow: () => ipcRenderer.invoke('close-window'),
    setStayOnTop: (enabled) => ipcRenderer.invoke('set-stay-on-top', enabled),
    
    // File operations
    showSaveDialog: () => ipcRenderer.invoke('show-save-dialog'),
    showOpenDialog: () => ipcRenderer.invoke('show-open-dialog'),
    readFile: (filePath) => ipcRenderer.invoke('read-file', filePath),
    writeFile: (filePath, data) => ipcRenderer.invoke('write-file', filePath, data),
    
    // Menu event listeners
    onMenuNewTab: (callback) => ipcRenderer.on('menu-new-tab', callback),
    onMenuOpenFile: (callback) => ipcRenderer.on('menu-open-file', callback),
    onMenuSaveFile: (callback) => ipcRenderer.on('menu-save-file', callback),
    onMenuUndo: (callback) => ipcRenderer.on('menu-undo', callback),
    onMenuRedo: (callback) => ipcRenderer.on('menu-redo', callback),
    onMenuClearAll: (callback) => ipcRenderer.on('menu-clear-all', callback),
    onMenuStayOnTop: (callback) => ipcRenderer.on('menu-stay-on-top', callback),
    onMenuAbout: (callback) => ipcRenderer.on('menu-about', callback),
    onMenuHelp: (callback) => ipcRenderer.on('menu-help', callback),
    
    // Remove listeners
    removeAllListeners: (channel) => ipcRenderer.removeAllListeners(channel),
    
    // Platform information
    platform: process.platform,
    
    // Version information
    versions: {
        node: process.versions.node,
        chrome: process.versions.chrome,
        electron: process.versions.electron
    }
});

// Expose a limited set of Node.js APIs for file operations
contextBridge.exposeInMainWorld('nodeAPI', {
    path: {
        join: (...args) => require('path').join(...args),
        dirname: (path) => require('path').dirname(path),
        basename: (path) => require('path').basename(path),
        extname: (path) => require('path').extname(path)
    }
});

// Console logging for debugging
if (process.env.NODE_ENV === 'development') {
    contextBridge.exposeInMainWorld('electronDebug', {
        log: (...args) => console.log('[Preload]', ...args),
        error: (...args) => console.error('[Preload]', ...args),
        warn: (...args) => console.warn('[Preload]', ...args)
    });
}

// Security: Remove Node.js globals from renderer process
delete global.require;
delete global.exports;
delete global.module;
delete global.__dirname;
delete global.__filename;
delete global.process;
delete global.Buffer;

console.log('CalcForge preload script loaded successfully');
