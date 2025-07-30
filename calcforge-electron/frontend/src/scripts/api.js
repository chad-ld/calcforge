/**
 * CalcForge API Communication Module
 * Handles all communication with the backend API server
 */

class CalcForgeAPI {
    constructor() {
        this.baseURL = 'http://localhost:8000';
        this.websocket = null;
        this.isConnected = false;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 5;
        this.reconnectDelay = 1000;
        this.messageHandlers = new Map();
        this.requestId = 0;
        
        // Bind methods
        this.onWebSocketOpen = this.onWebSocketOpen.bind(this);
        this.onWebSocketMessage = this.onWebSocketMessage.bind(this);
        this.onWebSocketClose = this.onWebSocketClose.bind(this);
        this.onWebSocketError = this.onWebSocketError.bind(this);
    }
    
    /**
     * Initialize the API connection
     */
    async connect() {
        try {
            // Test REST API connection
            const response = await fetch(`${this.baseURL}/`);
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }
            
            // Skip WebSocket connection for now (using REST API only)
            // this.connectWebSocket();
            this.isConnected = true;
            this.updateConnectionStatus('connected', 'Connected');
            
            return true;
        } catch (error) {
            console.error('Failed to connect to API:', error);
            this.updateConnectionStatus('error', 'Connection failed');
            return false;
        }
    }
    
    /**
     * Connect to WebSocket for real-time updates
     */
    connectWebSocket() {
        try {
            const wsURL = this.baseURL.replace('http', 'ws') + '/ws';
            this.websocket = new WebSocket(wsURL);
            
            this.websocket.onopen = this.onWebSocketOpen;
            this.websocket.onmessage = this.onWebSocketMessage;
            this.websocket.onclose = this.onWebSocketClose;
            this.websocket.onerror = this.onWebSocketError;
            
        } catch (error) {
            console.error('WebSocket connection failed:', error);
            this.updateConnectionStatus('error', 'WebSocket failed');
        }
    }
    
    /**
     * WebSocket event handlers
     */
    onWebSocketOpen() {
        console.log('WebSocket connected');
        this.isConnected = true;
        this.reconnectAttempts = 0;
        this.updateConnectionStatus('connected', 'Connected');
    }
    
    onWebSocketMessage(event) {
        try {
            const data = JSON.parse(event.data);
            
            // Handle different message types
            if (data.type === 'calculation_result') {
                this.handleCalculationResult(data);
            } else if (data.type === 'batch_calculation_result') {
                this.handleBatchCalculationResult(data);
            } else if (data.request_id && this.messageHandlers.has(data.request_id)) {
                // Handle request-response pattern
                const handler = this.messageHandlers.get(data.request_id);
                handler.resolve(data);
                this.messageHandlers.delete(data.request_id);
            }
        } catch (error) {
            console.error('Error parsing WebSocket message:', error);
        }
    }
    
    onWebSocketClose() {
        console.log('WebSocket disconnected');
        this.isConnected = false;
        this.updateConnectionStatus('disconnected', 'Disconnected');
        
        // Attempt to reconnect
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            this.reconnectAttempts++;
            setTimeout(() => {
                console.log(`Reconnection attempt ${this.reconnectAttempts}`);
                this.connectWebSocket();
            }, this.reconnectDelay * this.reconnectAttempts);
        }
    }
    
    onWebSocketError(error) {
        console.error('WebSocket error:', error);
        this.updateConnectionStatus('error', 'Connection error');
    }
    
    /**
     * Update connection status in UI
     */
    updateConnectionStatus(status, message) {
        const indicator = document.getElementById('status-indicator');
        const text = document.getElementById('status-text');
        const apiStatus = document.getElementById('api-status');
        
        if (indicator) {
            indicator.className = `status-indicator ${status}`;
        }
        
        if (text) {
            text.textContent = message;
        }
        
        if (apiStatus) {
            apiStatus.textContent = `API: ${message}`;
        }
        
        // Dispatch custom event for other components
        window.dispatchEvent(new CustomEvent('connectionStatusChanged', {
            detail: { status, message }
        }));
    }
    
    /**
     * Calculate a single expression
     */
    async calculateExpression(expression, sheetId = 0, lineNum = 1) {
        try {
            const response = await fetch(`${this.baseURL}/api/calculate`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    expression,
                    sheet_id: sheetId,
                    line_num: lineNum
                })
            });
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }
            
            return await response.json();
        } catch (error) {
            console.error('Calculation error:', error);
            return {
                value: '',
                unit: '',
                error: `Network Error: ${error.message}`,
                highlights: []
            };
        }
    }
    
    /**
     * Calculate multiple expressions in batch
     */
    async calculateBatch(expressions, sheetId = 0) {
        PerformanceDebug.startTimer('network_batch_calculation');

        try {
            const response = await fetch(`${this.baseURL}/api/calculate-batch`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    expressions,
                    sheet_id: sheetId
                })
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const data = await response.json();

            PerformanceDebug.endTimer('network_batch_calculation', {
                expressionCount: expressions.length,
                responseSize: JSON.stringify(data).length
            });

            return data.results;
        } catch (error) {
            console.error('Batch calculation error:', error);

            PerformanceDebug.endTimer('network_batch_calculation', {
                error: error.message,
                expressionCount: expressions.length
            });

            // Return error results for each expression
            return expressions.map(() => ({
                value: '',
                unit: '',
                error: `Network Error: ${error.message}`,
                highlights: []
            }));
        }
    }

    /**
     * Calculate only specific lines for incremental updates
     * FALLBACK: Uses batch endpoint but only sends changed lines for better performance
     */
    async calculateSelective(expressions, lineNumbers, sheetId = 0) {
        PerformanceDebug.startTimer('network_selective_calculation');

        try {
            // PERFORMANCE OPTIMIZATION: Instead of sending all expressions,
            // only send the expressions for lines that actually changed
            const selectiveExpressions = [];
            const lineMapping = {}; // Maps result index to original line number

            lineNumbers.forEach((lineNum, index) => {
                if (lineNum >= 1 && lineNum <= expressions.length) {
                    selectiveExpressions.push(expressions[lineNum - 1]); // Convert to 0-based
                    lineMapping[index] = lineNum;
                }
            });

            console.log(`PERFORMANCE: Selective calculation - sending ${selectiveExpressions.length} expressions instead of ${expressions.length}`);

            // Use the working batch endpoint with only the changed expressions
            const response = await fetch(`${this.baseURL}/api/calculate-batch`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({
                    expressions: selectiveExpressions,
                    sheet_id: sheetId
                })
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const batchData = await response.json();

            // Convert batch results back to selective format
            const selectiveResults = {};
            batchData.results.forEach((result, index) => {
                const originalLineNum = lineMapping[index];
                if (originalLineNum) {
                    selectiveResults[originalLineNum] = result;
                }
            });

            PerformanceDebug.endTimer('network_selective_calculation', {
                expressionCount: expressions.length,
                sentExpressions: selectiveExpressions.length,
                calculatedLines: lineNumbers.length,
                responseSize: JSON.stringify(selectiveResults).length,
                optimization: `${Math.round((1 - selectiveExpressions.length / expressions.length) * 100)}% reduction`
            });

            return selectiveResults; // Returns {lineNum: result} mapping
        } catch (error) {
            console.error('Selective calculation error:', error);

            PerformanceDebug.endTimer('network_selective_calculation', {
                error: error.message,
                expressionCount: expressions.length,
                calculatedLines: lineNumbers.length
            });

            // Return error results for requested lines
            const errorResults = {};
            lineNumbers.forEach(lineNum => {
                errorResults[lineNum] = {
                    value: '',
                    unit: '',
                    error: `Network Error: ${error.message}`,
                    highlights: []
                };
            });
            return errorResults;
        }
    }
    
    /**
     * Get syntax highlighting for text
     */
    async getSyntaxHighlighting(text) {
        try {
            const response = await fetch(`${this.baseURL}/api/syntax-highlight`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ text })
            });

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            const data = await response.json();
            return data.highlights;
        } catch (error) {
            console.error('Syntax highlighting error:', error);
            return [];
        }
    }
    
    /**
     * Update worksheet data for cross-sheet references
     */
    async updateWorksheets(worksheets) {
        try {
            const response = await fetch(`${this.baseURL}/api/worksheets/update`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ worksheets })
            });
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }
            
            return await response.json();
        } catch (error) {
            console.error('Worksheet update error:', error);
            return { status: 'error', message: error.message };
        }
    }
    
    /**
     * Get available functions for autocompletion
     */
    async getFunctions() {
        try {
            const response = await fetch(`${this.baseURL}/api/functions`);
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }
            
            const data = await response.json();
            return data.functions;
        } catch (error) {
            console.error('Functions fetch error:', error);
            return [];
        }
    }
    
    /**
     * Send real-time calculation via WebSocket
     */
    sendCalculation(expression, sheetId = 0, lineNum = 1) {
        if (!this.isConnected || !this.websocket) {
            console.warn('WebSocket not connected, falling back to REST API');
            return this.calculateExpression(expression, sheetId, lineNum);
        }
        
        const message = {
            type: 'calculate',
            expression,
            sheet_id: sheetId,
            line_num: lineNum,
            request_id: ++this.requestId
        };
        
        this.websocket.send(JSON.stringify(message));
        
        // Return a promise that resolves when the response is received
        return new Promise((resolve, reject) => {
            this.messageHandlers.set(message.request_id, { resolve, reject });
            
            // Set timeout for the request
            setTimeout(() => {
                if (this.messageHandlers.has(message.request_id)) {
                    this.messageHandlers.delete(message.request_id);
                    reject(new Error('Request timeout'));
                }
            }, 5000);
        });
    }
    
    /**
     * Send batch calculation via WebSocket
     */
    sendBatchCalculation(expressions, sheetId = 0) {
        if (!this.isConnected || !this.websocket) {
            console.warn('WebSocket not connected, falling back to REST API');
            return this.calculateBatch(expressions, sheetId);
        }
        
        const message = {
            type: 'batch_calculate',
            expressions,
            sheet_id: sheetId,
            request_id: ++this.requestId
        };
        
        this.websocket.send(JSON.stringify(message));
        
        return new Promise((resolve, reject) => {
            this.messageHandlers.set(message.request_id, { resolve, reject });
            
            setTimeout(() => {
                if (this.messageHandlers.has(message.request_id)) {
                    this.messageHandlers.delete(message.request_id);
                    reject(new Error('Request timeout'));
                }
            }, 10000);
        });
    }
    
    /**
     * Handle calculation result from WebSocket
     */
    handleCalculationResult(data) {
        // Dispatch event for other components to handle
        window.dispatchEvent(new CustomEvent('calculationResult', {
            detail: data
        }));
    }
    
    /**
     * Handle batch calculation result from WebSocket
     */
    handleBatchCalculationResult(data) {
        // Dispatch event for other components to handle
        window.dispatchEvent(new CustomEvent('batchCalculationResult', {
            detail: data
        }));
    }
    
    /**
     * Disconnect from API
     */
    disconnect() {
        if (this.websocket) {
            this.websocket.close();
            this.websocket = null;
        }
        this.isConnected = false;
        this.updateConnectionStatus('disconnected', 'Disconnected');
    }
}

// Export for use in other modules
window.CalcForgeAPI = CalcForgeAPI;
