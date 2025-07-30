/**
 * CalcForge Electron Test Script
 * Tests the Electron integration without full startup
 */

const { app, BrowserWindow } = require('electron');
const path = require('path');

// Test configuration
const TEST_CONFIG = {
    timeout: 10000,
    windowWidth: 800,
    windowHeight: 600
};

let testWindow = null;
let testResults = [];

/**
 * Add test result
 */
function addTestResult(name, success, message = '') {
    testResults.push({
        name,
        success,
        message,
        timestamp: new Date().toISOString()
    });
    
    const status = success ? '✅ PASS' : '❌ FAIL';
    console.log(`${status}: ${name}${message ? ' - ' + message : ''}`);
}

/**
 * Test window creation
 */
async function testWindowCreation() {
    try {
        testWindow = new BrowserWindow({
            width: TEST_CONFIG.windowWidth,
            height: TEST_CONFIG.windowHeight,
            show: false,
            webPreferences: {
                nodeIntegration: false,
                contextIsolation: true,
                preload: path.join(__dirname, 'electron', 'preload.js')
            }
        });
        
        addTestResult('Window Creation', true);
        return true;
    } catch (error) {
        addTestResult('Window Creation', false, error.message);
        return false;
    }
}

/**
 * Test preload script
 */
async function testPreloadScript() {
    return new Promise((resolve) => {
        if (!testWindow) {
            addTestResult('Preload Script', false, 'No test window available');
            resolve(false);
            return;
        }
        
        // Load a simple HTML page to test preload
        const testHtml = `
            <!DOCTYPE html>
            <html>
            <head><title>Test</title></head>
            <body>
                <script>
                    if (typeof window.electronAPI !== 'undefined') {
                        console.log('Preload script loaded successfully');
                        window.electronAPI.getAppInfo().then(() => {
                            document.title = 'Preload Test Success';
                        }).catch(() => {
                            document.title = 'Preload Test Failed';
                        });
                    } else {
                        document.title = 'Preload Test Failed';
                    }
                </script>
            </body>
            </html>
        `;
        
        testWindow.loadURL(`data:text/html;charset=utf-8,${encodeURIComponent(testHtml)}`);
        
        setTimeout(() => {
            testWindow.webContents.executeJavaScript('document.title').then((title) => {
                const success = title === 'Preload Test Success';
                addTestResult('Preload Script', success, title);
                resolve(success);
            }).catch((error) => {
                addTestResult('Preload Script', false, error.message);
                resolve(false);
            });
        }, 2000);
    });
}

/**
 * Test frontend loading
 */
async function testFrontendLoading() {
    return new Promise((resolve) => {
        if (!testWindow) {
            addTestResult('Frontend Loading', false, 'No test window available');
            resolve(false);
            return;
        }
        
        const frontendPath = path.join(__dirname, 'frontend', 'src', 'index.html');
        
        testWindow.webContents.once('did-finish-load', () => {
            // Check if the page loaded successfully
            testWindow.webContents.executeJavaScript(`
                document.querySelector('.app-title') ? 'CalcForge UI Loaded' : 'UI Not Found'
            `).then((result) => {
                const success = result === 'CalcForge UI Loaded';
                addTestResult('Frontend Loading', success, result);
                resolve(success);
            }).catch((error) => {
                addTestResult('Frontend Loading', false, error.message);
                resolve(false);
            });
        });
        
        testWindow.webContents.once('did-fail-load', (event, errorCode, errorDescription) => {
            addTestResult('Frontend Loading', false, `${errorCode}: ${errorDescription}`);
            resolve(false);
        });
        
        testWindow.loadFile(frontendPath);
    });
}

/**
 * Test IPC communication
 */
async function testIpcCommunication() {
    return new Promise((resolve) => {
        if (!testWindow) {
            addTestResult('IPC Communication', false, 'No test window available');
            resolve(false);
            return;
        }
        
        // Test getting app info via IPC
        testWindow.webContents.executeJavaScript(`
            window.electronAPI ? window.electronAPI.getAppInfo() : Promise.reject('No electronAPI')
        `).then((appInfo) => {
            const success = appInfo && appInfo.name && appInfo.version;
            addTestResult('IPC Communication', success, success ? `App: ${appInfo.name} v${appInfo.version}` : 'Invalid app info');
            resolve(success);
        }).catch((error) => {
            addTestResult('IPC Communication', false, error.toString());
            resolve(false);
        });
    });
}

/**
 * Run all tests
 */
async function runTests() {
    console.log('🧪 Starting CalcForge Electron Tests...\n');
    
    const tests = [
        { name: 'Window Creation', fn: testWindowCreation },
        { name: 'Preload Script', fn: testPreloadScript },
        { name: 'Frontend Loading', fn: testFrontendLoading },
        { name: 'IPC Communication', fn: testIpcCommunication }
    ];
    
    let passCount = 0;
    
    for (const test of tests) {
        try {
            const result = await test.fn();
            if (result) passCount++;
        } catch (error) {
            addTestResult(test.name, false, `Exception: ${error.message}`);
        }
    }
    
    // Print summary
    console.log('\n📊 Test Summary:');
    console.log(`Total Tests: ${tests.length}`);
    console.log(`Passed: ${passCount}`);
    console.log(`Failed: ${tests.length - passCount}`);
    console.log(`Success Rate: ${Math.round((passCount / tests.length) * 100)}%`);
    
    if (passCount === tests.length) {
        console.log('\n🎉 All tests passed! Electron integration is working correctly.');
    } else {
        console.log('\n⚠️  Some tests failed. Check the output above for details.');
    }
    
    // Cleanup
    if (testWindow) {
        testWindow.close();
    }
    
    app.quit();
}

/**
 * App event handlers
 */
app.whenReady().then(() => {
    runTests();
});

app.on('window-all-closed', () => {
    app.quit();
});

// Handle timeout
setTimeout(() => {
    console.log('\n⏰ Test timeout reached');
    addTestResult('Overall Test Suite', false, 'Timeout after ' + TEST_CONFIG.timeout + 'ms');
    app.quit();
}, TEST_CONFIG.timeout);
