/**
 * CalcForge Performance Debugging Class
 * 
 * This class provides comprehensive performance monitoring and debugging
 * capabilities that can be easily toggled on/off without code changes.
 * 
 * Usage:
 * - Set PERFORMANCE_DEBUG_ENABLED to true to enable debugging
 * - Call PerformanceDebug.startTimer(label) to start timing an operation
 * - Call PerformanceDebug.endTimer(label) to end timing and log results
 * - Call PerformanceDebug.logEvent(event, data) to log custom events
 * - Call PerformanceDebug.getReport() to get a comprehensive performance report
 */

class PerformanceDebug {
    static ENABLED = true; // Set to false to disable all performance debugging
    
    static timers = new Map();
    static events = [];
    static metrics = {
        calculationCount: 0,
        totalCalculationTime: 0,
        averageCalculationTime: 0,
        slowestCalculation: { time: 0, expression: '' },
        fastestCalculation: { time: Infinity, expression: '' },
        networkRequests: 0,
        totalNetworkTime: 0,
        averageNetworkTime: 0,
        uiUpdates: 0,
        totalUIUpdateTime: 0,
        averageUIUpdateTime: 0,
        debounceSkips: 0,
        overlayShows: 0,
        overlayHides: 0
    };
    
    static sessionStart = performance.now();
    
    /**
     * Start timing an operation
     */
    static startTimer(label) {
        if (!this.ENABLED) return;
        
        this.timers.set(label, {
            start: performance.now(),
            label: label
        });
        
        this.logEvent('timer_start', { label });
    }
    
    /**
     * End timing an operation and log the result
     */
    static endTimer(label, additionalData = {}) {
        if (!this.ENABLED) return;
        
        const timer = this.timers.get(label);
        if (!timer) {
            console.warn(`PerformanceDebug: Timer '${label}' not found`);
            return 0;
        }
        
        const duration = performance.now() - timer.start;
        this.timers.delete(label);
        
        // Update metrics based on timer type
        this.updateMetrics(label, duration, additionalData);
        
        // Log the timing
        const color = duration > 100 ? 'color: red' : duration > 50 ? 'color: orange' : 'color: green';
        console.log(`%c[PERF] ${label}: ${duration.toFixed(2)}ms`, color, additionalData);
        
        this.logEvent('timer_end', { 
            label, 
            duration: duration.toFixed(2),
            ...additionalData 
        });
        
        return duration;
    }
    
    /**
     * Update internal metrics based on timer results
     */
    static updateMetrics(label, duration, data) {
        if (!this.ENABLED) return;
        
        if (label.includes('calculation')) {
            this.metrics.calculationCount++;
            this.metrics.totalCalculationTime += duration;
            this.metrics.averageCalculationTime = this.metrics.totalCalculationTime / this.metrics.calculationCount;
            
            if (duration > this.metrics.slowestCalculation.time) {
                this.metrics.slowestCalculation = { 
                    time: duration, 
                    expression: data.expression || 'unknown' 
                };
            }
            
            if (duration < this.metrics.fastestCalculation.time) {
                this.metrics.fastestCalculation = { 
                    time: duration, 
                    expression: data.expression || 'unknown' 
                };
            }
        }
        
        if (label.includes('network') || label.includes('api')) {
            this.metrics.networkRequests++;
            this.metrics.totalNetworkTime += duration;
            this.metrics.averageNetworkTime = this.metrics.totalNetworkTime / this.metrics.networkRequests;
        }
        
        if (label.includes('ui') || label.includes('update') || label.includes('render')) {
            this.metrics.uiUpdates++;
            this.metrics.totalUIUpdateTime += duration;
            this.metrics.averageUIUpdateTime = this.metrics.totalUIUpdateTime / this.metrics.uiUpdates;
        }
    }
    
    /**
     * Log a custom event
     */
    static logEvent(event, data = {}) {
        if (!this.ENABLED) return;
        
        this.events.push({
            timestamp: performance.now(),
            event,
            data
        });
        
        // Keep only last 1000 events to prevent memory issues
        if (this.events.length > 1000) {
            this.events = this.events.slice(-1000);
        }
    }
    
    /**
     * Log a debounce skip event
     */
    static logDebounceSkip(operation) {
        if (!this.ENABLED) return;
        
        this.metrics.debounceSkips++;
        this.logEvent('debounce_skip', { operation });
        console.log(`%c[PERF] Debounce skip: ${operation}`, 'color: blue');
    }
    
    /**
     * Log overlay show/hide events
     */
    static logOverlayShow() {
        if (!this.ENABLED) return;
        
        this.metrics.overlayShows++;
        this.logEvent('overlay_show');
        console.log(`%c[PERF] Loading overlay shown`, 'color: purple');
    }
    
    static logOverlayHide(reason = 'calculation_complete') {
        if (!this.ENABLED) return;
        
        this.metrics.overlayHides++;
        this.logEvent('overlay_hide', { reason });
        console.log(`%c[PERF] Loading overlay hidden: ${reason}`, 'color: purple');
    }
    
    /**
     * Get a comprehensive performance report
     */
    static getReport() {
        if (!this.ENABLED) return 'Performance debugging is disabled';
        
        const sessionDuration = (performance.now() - this.sessionStart) / 1000;
        
        const report = {
            sessionDuration: `${sessionDuration.toFixed(2)}s`,
            calculations: {
                total: this.metrics.calculationCount,
                averageTime: `${this.metrics.averageCalculationTime.toFixed(2)}ms`,
                totalTime: `${this.metrics.totalCalculationTime.toFixed(2)}ms`,
                slowest: {
                    time: `${this.metrics.slowestCalculation.time.toFixed(2)}ms`,
                    expression: this.metrics.slowestCalculation.expression
                },
                fastest: {
                    time: this.metrics.fastestCalculation.time === Infinity ? 'N/A' : `${this.metrics.fastestCalculation.time.toFixed(2)}ms`,
                    expression: this.metrics.fastestCalculation.expression
                },
                calculationsPerSecond: (this.metrics.calculationCount / sessionDuration).toFixed(2)
            },
            network: {
                totalRequests: this.metrics.networkRequests,
                averageTime: `${this.metrics.averageNetworkTime.toFixed(2)}ms`,
                totalTime: `${this.metrics.totalNetworkTime.toFixed(2)}ms`,
                requestsPerSecond: (this.metrics.networkRequests / sessionDuration).toFixed(2)
            },
            ui: {
                totalUpdates: this.metrics.uiUpdates,
                averageTime: `${this.metrics.averageUIUpdateTime.toFixed(2)}ms`,
                totalTime: `${this.metrics.totalUIUpdateTime.toFixed(2)}ms`,
                updatesPerSecond: (this.metrics.uiUpdates / sessionDuration).toFixed(2)
            },
            debouncing: {
                skips: this.metrics.debounceSkips,
                skipsPerSecond: (this.metrics.debounceSkips / sessionDuration).toFixed(2)
            },
            overlay: {
                shows: this.metrics.overlayShows,
                hides: this.metrics.overlayHides,
                showsPerSecond: (this.metrics.overlayShows / sessionDuration).toFixed(2)
            },
            recentEvents: this.events.slice(-20) // Last 20 events
        };
        
        return report;
    }
    
    /**
     * Print a formatted performance report to console
     */
    static printReport() {
        if (!this.ENABLED) {
            console.log('Performance debugging is disabled');
            return;
        }
        
        const report = this.getReport();
        
        console.group('%c📊 CalcForge Performance Report', 'font-size: 16px; font-weight: bold; color: #2196F3');
        
        console.group('⏱️ Session Info');
        console.log(`Duration: ${report.sessionDuration}`);
        console.groupEnd();
        
        console.group('🧮 Calculations');
        console.log(`Total: ${report.calculations.total}`);
        console.log(`Average Time: ${report.calculations.averageTime}`);
        console.log(`Total Time: ${report.calculations.totalTime}`);
        console.log(`Rate: ${report.calculations.calculationsPerSecond}/sec`);
        console.log(`Slowest: ${report.calculations.slowest.time} (${report.calculations.slowest.expression})`);
        console.log(`Fastest: ${report.calculations.fastest.time} (${report.calculations.fastest.expression})`);
        console.groupEnd();
        
        console.group('🌐 Network');
        console.log(`Total Requests: ${report.network.totalRequests}`);
        console.log(`Average Time: ${report.network.averageTime}`);
        console.log(`Total Time: ${report.network.totalTime}`);
        console.log(`Rate: ${report.network.requestsPerSecond}/sec`);
        console.groupEnd();
        
        console.group('🎨 UI Updates');
        console.log(`Total Updates: ${report.ui.totalUpdates}`);
        console.log(`Average Time: ${report.ui.averageTime}`);
        console.log(`Total Time: ${report.ui.totalTime}`);
        console.log(`Rate: ${report.ui.updatesPerSecond}/sec`);
        console.groupEnd();
        
        console.group('⏳ Debouncing');
        console.log(`Skips: ${report.debouncing.skips}`);
        console.log(`Rate: ${report.debouncing.skipsPerSecond}/sec`);
        console.groupEnd();
        
        console.group('🔄 Loading Overlay');
        console.log(`Shows: ${report.overlay.shows}`);
        console.log(`Hides: ${report.overlay.hides}`);
        console.log(`Rate: ${report.overlay.showsPerSecond}/sec`);
        console.groupEnd();
        
        console.groupEnd();
    }
    
    /**
     * Reset all metrics and events
     */
    static reset() {
        if (!this.ENABLED) return;
        
        this.timers.clear();
        this.events = [];
        this.metrics = {
            calculationCount: 0,
            totalCalculationTime: 0,
            averageCalculationTime: 0,
            slowestCalculation: { time: 0, expression: '' },
            fastestCalculation: { time: Infinity, expression: '' },
            networkRequests: 0,
            totalNetworkTime: 0,
            averageNetworkTime: 0,
            uiUpdates: 0,
            totalUIUpdateTime: 0,
            averageUIUpdateTime: 0,
            debounceSkips: 0,
            overlayShows: 0,
            overlayHides: 0
        };
        this.sessionStart = performance.now();
        
        console.log('%c[PERF] Performance metrics reset', 'color: green; font-weight: bold');
    }
    
    /**
     * Enable or disable performance debugging
     */
    static setEnabled(enabled) {
        this.ENABLED = enabled;
        console.log(`%c[PERF] Performance debugging ${enabled ? 'enabled' : 'disabled'}`, 
                   `color: ${enabled ? 'green' : 'red'}; font-weight: bold`);
    }
}

// Make available globally for easy console access
window.PerformanceDebug = PerformanceDebug;

// Add some helpful console commands
window.perfReport = () => PerformanceDebug.printReport();
window.perfReset = () => PerformanceDebug.reset();
window.perfToggle = () => PerformanceDebug.setEnabled(!PerformanceDebug.ENABLED);

console.log('%c🚀 CalcForge Performance Debugging Loaded', 'color: #4CAF50; font-weight: bold; font-size: 14px');
console.log('%cUse perfReport() to see performance data, perfReset() to reset, perfToggle() to enable/disable', 'color: #666');
