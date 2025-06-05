/**
 * CalcForge Electron Main Process
 * Handles window management, menu system, and backend integration
 */

const { app, BrowserWindow, Menu, dialog, shell, ipcMain } = require('electron');
const { autoUpdater } = require('electron-updater');
const Store = require('electron-store');
const windowStateKeeper = require('electron-window-state');
const path = require('path');
const fs = require('fs');
const { spawn } = require('child_process');

// Initialize electron store for settings
const store = new Store();

// Global references
let mainWindow = null;
let backendProcess = null;
let isQuitting = false;
let isDev = false;

// Path to the original worksheets.json file
const originalWorksheetsPath = path.join(__dirname, '..', '..', 'worksheets.json');

// Check if running in development mode
isDev = process.argv.includes('--dev') || process.env.NODE_ENV === 'development';

/**
 * Create the main application window
 */
function createMainWindow() {
    // Load window state
    let mainWindowState = windowStateKeeper({
        defaultWidth: 1200,
        defaultHeight: 800
    });
    
    // Create the browser window
    mainWindow = new BrowserWindow({
        x: mainWindowState.x,
        y: mainWindowState.y,
        width: mainWindowState.width,
        height: mainWindowState.height,
        minWidth: 800,
        minHeight: 600,
        show: false,
        icon: getIconPath(),
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            enableRemoteModule: false,
            preload: path.join(__dirname, 'preload.js'),
            webSecurity: !isDev
        },
        titleBarStyle: process.platform === 'darwin' ? 'hiddenInset' : 'default',
        frame: true,
        backgroundColor: '#0D1117' // GitHub dark theme background
    });
    
    // Let windowStateKeeper manage the window
    mainWindowState.manage(mainWindow);
    
    // Load the frontend
    const frontendPath = isDev
        ? path.join(__dirname, '..', 'frontend', 'src', 'index.html')
        : path.join(__dirname, '..', 'frontend', 'src', 'index.html');

    console.log('Loading frontend from:', frontendPath);
    console.log('File exists:', fs.existsSync(frontendPath));

    mainWindow.loadFile(frontendPath);

    // Show window when ready
    mainWindow.once('ready-to-show', () => {
        mainWindow.show();

        // Always open DevTools in development for debugging
        if (isDev) {
            mainWindow.webContents.openDevTools();
        }

        // DevTools can be opened manually with F12 or Debug menu

        // Set stay on top if enabled
        const stayOnTop = store.get('stayOnTop', true);
        mainWindow.setAlwaysOnTop(stayOnTop);
    });

    // Add error handling for page load
    mainWindow.webContents.on('did-fail-load', (event, errorCode, errorDescription, validatedURL) => {
        console.error('Failed to load page:', errorCode, errorDescription, validatedURL);
    });

    mainWindow.webContents.on('dom-ready', () => {
        console.log('DOM is ready');
    });
    
    // Handle window closed
    mainWindow.on('closed', () => {
        mainWindow = null;
    });
    
    // Handle window close event
    mainWindow.on('close', async (event) => {
        if (!isQuitting) {
            if (process.platform === 'darwin') {
                event.preventDefault();
                mainWindow.hide();
            } else {
                // On Windows/Linux, save worksheets before closing
                event.preventDefault();
                try {
                    // Request the frontend to save worksheets
                    await mainWindow.webContents.executeJavaScript(`
                        if (window.calcForgeApp && window.calcForgeApp.saveWorksheetsOnExit) {
                            window.calcForgeApp.saveWorksheetsOnExit();
                        }
                    `);
                    // Small delay to ensure save completes
                    setTimeout(() => {
                        isQuitting = true;
                        mainWindow.destroy();
                    }, 500);
                } catch (error) {
                    console.error('Error saving worksheets on exit:', error);
                    isQuitting = true;
                    mainWindow.destroy();
                }
            }
        }
    });
    
    // Handle external links
    mainWindow.webContents.setWindowOpenHandler(({ url }) => {
        shell.openExternal(url);
        return { action: 'deny' };
    });
    
    // Set up IPC handlers
    setupIpcHandlers();
    
    return mainWindow;
}

/**
 * Get appropriate icon path for platform
 */
function getIconPath() {
    if (process.platform === 'win32') {
        return path.join(__dirname, '..', 'build', 'icons', 'icon.ico');
    } else if (process.platform === 'darwin') {
        return path.join(__dirname, '..', 'build', 'icons', 'icon.icns');
    } else {
        return path.join(__dirname, '..', 'build', 'icons', 'icon.png');
    }
}

/**
 * Start the Python backend server
 */
function startBackend() {
    if (backendProcess) {
        return Promise.resolve();
    }
    
    return new Promise((resolve, reject) => {
        const backendPath = path.join(__dirname, '..', 'backend');
        const scriptPath = path.join(backendPath, 'api_server.py');
        
        // Check if Python is available
        const pythonCmd = process.platform === 'win32' ? 'python' : 'python3';
        
        console.log('Starting backend server...');
        console.log('Backend path:', backendPath);
        console.log('Script path:', scriptPath);
        
        backendProcess = spawn(pythonCmd, [scriptPath], {
            cwd: backendPath,
            stdio: isDev ? 'inherit' : 'pipe'
        });
        
        backendProcess.on('error', (error) => {
            console.error('Failed to start backend:', error);
            reject(error);
        });
        
        backendProcess.on('exit', (code) => {
            console.log(`Backend process exited with code ${code}`);
            backendProcess = null;
        });
        
        // Wait for server to start
        setTimeout(() => {
            if (backendProcess && !backendProcess.killed) {
                console.log('Backend server started successfully');
                resolve();
            } else {
                reject(new Error('Backend failed to start'));
            }
        }, 3000);
    });
}

/**
 * Stop the backend server
 */
function stopBackend() {
    if (backendProcess) {
        console.log('Stopping backend server...');
        backendProcess.kill();
        backendProcess = null;
    }
}

/**
 * Set up IPC handlers for communication with renderer
 */
function setupIpcHandlers() {
    // Handle stay on top toggle
    ipcMain.handle('set-stay-on-top', (event, enabled) => {
        if (mainWindow) {
            mainWindow.setAlwaysOnTop(enabled);
            store.set('stayOnTop', enabled);
        }
        return enabled;
    });
    
    // Handle file operations
    ipcMain.handle('show-save-dialog', async () => {
        if (!mainWindow) return null;
        
        const result = await dialog.showSaveDialog(mainWindow, {
            title: 'Save CalcForge Worksheet',
            defaultPath: 'calcforge-worksheet.json',
            filters: [
                { name: 'CalcForge Files', extensions: ['json', 'cf'] },
                { name: 'JSON Files', extensions: ['json'] },
                { name: 'All Files', extensions: ['*'] }
            ]
        });
        
        return result;
    });
    
    ipcMain.handle('show-open-dialog', async () => {
        if (!mainWindow) return null;
        
        const result = await dialog.showOpenDialog(mainWindow, {
            title: 'Open CalcForge Worksheet',
            filters: [
                { name: 'CalcForge Files', extensions: ['json', 'cf'] },
                { name: 'JSON Files', extensions: ['json'] },
                { name: 'All Files', extensions: ['*'] }
            ],
            properties: ['openFile']
        });
        
        return result;
    });
    
    // Handle file system operations
    ipcMain.handle('read-file', async (event, filePath) => {
        try {
            const data = fs.readFileSync(filePath, 'utf8');
            return { success: true, data };
        } catch (error) {
            return { success: false, error: error.message };
        }
    });
    
    ipcMain.handle('write-file', async (event, filePath, data) => {
        try {
            fs.writeFileSync(filePath, data, 'utf8');
            return { success: true };
        } catch (error) {
            return { success: false, error: error.message };
        }
    });
    
    // Handle app info
    ipcMain.handle('get-app-info', () => {
        return {
            name: app.getName(),
            version: app.getVersion(),
            platform: process.platform,
            arch: process.arch,
            isDev: isDev
        };
    });
    
    // Handle window controls
    ipcMain.handle('minimize-window', () => {
        if (mainWindow) {
            mainWindow.minimize();
        }
    });
    
    ipcMain.handle('maximize-window', () => {
        if (mainWindow) {
            if (mainWindow.isMaximized()) {
                mainWindow.unmaximize();
            } else {
                mainWindow.maximize();
            }
        }
    });
    
    ipcMain.handle('close-window', () => {
        if (mainWindow) {
            mainWindow.close();
        }
    });

    // Handle loading original worksheets.json
    ipcMain.handle('load-original-worksheets', async () => {
        try {
            console.log('Loading original worksheets from:', originalWorksheetsPath);
            if (fs.existsSync(originalWorksheetsPath)) {
                const data = fs.readFileSync(originalWorksheetsPath, 'utf8');
                return { success: true, data: JSON.parse(data) };
            } else {
                console.log('Original worksheets.json not found, creating empty data');
                return { success: true, data: { "Sheet 1": "" } };
            }
        } catch (error) {
            console.error('Error loading original worksheets:', error);
            return { success: false, error: error.message };
        }
    });

    // Handle saving to original worksheets.json
    ipcMain.handle('save-original-worksheets', async (event, data) => {
        try {
            console.log('Saving worksheets to:', originalWorksheetsPath);
            fs.writeFileSync(originalWorksheetsPath, JSON.stringify(data, null, 2), 'utf8');
            return { success: true };
        } catch (error) {
            console.error('Error saving original worksheets:', error);
            return { success: false, error: error.message };
        }
    });
}

/**
 * Create application menu (hidden for CalcForge)
 */
function createMenu() {
    // Create a minimal menu with DevTools access for debugging
    const template = [
        {
            label: 'Debug',
            submenu: [
                {
                    label: 'Toggle DevTools',
                    accelerator: 'F12',
                    click: () => {
                        if (mainWindow) {
                            mainWindow.webContents.toggleDevTools();
                        }
                    }
                },
                {
                    label: 'Reload',
                    accelerator: 'F5',
                    click: () => {
                        if (mainWindow) {
                            mainWindow.webContents.reload();
                        }
                    }
                }
            ]
        }
    ];

    if (process.platform === 'darwin') {
        // On macOS, add the standard app menu
        template.unshift({
            label: app.getName(),
            submenu: [
                { role: 'about' },
                { type: 'separator' },
                { role: 'hide' },
                { role: 'hideothers' },
                { role: 'unhide' },
                { type: 'separator' },
                { role: 'quit' }
            ]
        });
    }

    const menu = Menu.buildFromTemplate(template);
    Menu.setApplicationMenu(menu);

    // Show menu bar on Windows/Linux for debugging
    if (mainWindow && process.platform !== 'darwin') {
        mainWindow.setMenuBarVisibility(true);
    }
}

/**
 * App event handlers
 */

// App ready event
app.whenReady().then(async () => {
    console.log('CalcForge starting...');

    try {
        // Start backend server
        await startBackend();

        // Create main window
        createMainWindow();

        // Create menu
        createMenu();

        // Set up auto updater
        if (!isDev) {
            autoUpdater.checkForUpdatesAndNotify();
        }

        console.log('CalcForge started successfully');

    } catch (error) {
        console.error('Failed to start CalcForge:', error);

        // Show error dialog
        dialog.showErrorBox(
            'CalcForge Startup Error',
            `Failed to start CalcForge: ${error.message}\n\nPlease ensure Python is installed and try again.`
        );

        app.quit();
    }
});

// All windows closed
app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        isQuitting = true;
        app.quit();
    }
});

// App activate (macOS)
app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
        createMainWindow();
    } else if (mainWindow) {
        mainWindow.show();
    }
});

// Before quit
app.on('before-quit', () => {
    isQuitting = true;
    stopBackend();
});

// App will quit
app.on('will-quit', (event) => {
    if (backendProcess && !backendProcess.killed) {
        event.preventDefault();
        stopBackend();
        setTimeout(() => {
            app.quit();
        }, 1000);
    }
});

/**
 * Auto updater events
 */
autoUpdater.on('checking-for-update', () => {
    console.log('Checking for update...');
});

autoUpdater.on('update-available', (info) => {
    console.log('Update available:', info);
});

autoUpdater.on('update-not-available', (info) => {
    console.log('Update not available:', info);
});

autoUpdater.on('error', (err) => {
    console.log('Error in auto-updater:', err);
});

autoUpdater.on('download-progress', (progressObj) => {
    let log_message = "Download speed: " + progressObj.bytesPerSecond;
    log_message = log_message + ' - Downloaded ' + progressObj.percent + '%';
    log_message = log_message + ' (' + progressObj.transferred + "/" + progressObj.total + ')';
    console.log(log_message);
});

autoUpdater.on('update-downloaded', (info) => {
    console.log('Update downloaded:', info);
    autoUpdater.quitAndInstall();
});

/**
 * Security: Prevent new window creation
 */
app.on('web-contents-created', (event, contents) => {
    contents.on('new-window', (event, navigationUrl) => {
        event.preventDefault();
        shell.openExternal(navigationUrl);
    });
});

/**
 * Handle certificate errors
 */
app.on('certificate-error', (event, webContents, url, error, certificate, callback) => {
    if (isDev) {
        // In development, ignore certificate errors
        event.preventDefault();
        callback(true);
    } else {
        // In production, use default behavior
        callback(false);
    }
});

/**
 * Prevent navigation to external URLs
 */
app.on('web-contents-created', (event, contents) => {
    contents.on('will-navigate', (event, navigationUrl) => {
        const parsedUrl = new URL(navigationUrl);

        if (parsedUrl.origin !== 'file://') {
            event.preventDefault();
        }
    });
});

// Export for testing
module.exports = {
    createMainWindow,
    startBackend,
    stopBackend
};
