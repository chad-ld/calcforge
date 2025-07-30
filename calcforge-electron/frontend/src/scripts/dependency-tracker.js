/**
 * CalcForge Dependency Tracker
 * 
 * Tracks line dependencies for LN variables and cross-sheet references
 * to enable smart incremental calculation updates.
 */

class DependencyTracker {
    constructor() {
        this.lineDependencies = new Map(); // line_num -> Set of lines that depend on it
        this.lineReferences = new Map();   // line_num -> Set of lines it references
        this.crossSheetRefs = new Map();   // line_num -> Set of cross-sheet refs it uses
        this.lastContent = new Map();      // line_num -> last content hash
        this.enabled = true;
    }
    
    /**
     * Analyze all lines and build dependency graph
     */
    buildDependencyGraph(lines) {
        if (!this.enabled) return;
        
        PerformanceDebug.startTimer('dependency_build_graph');
        
        // Clear existing dependencies
        this.lineDependencies.clear();
        this.lineReferences.clear();
        this.crossSheetRefs.clear();
        
        lines.forEach((line, index) => {
            const lineNum = index + 1;
            this.analyzeLine(lineNum, line);
        });
        
        PerformanceDebug.endTimer('dependency_build_graph', { 
            lineCount: lines.length,
            dependencyCount: this.lineDependencies.size
        });
    }
    
    /**
     * Analyze a single line for dependencies
     */
    analyzeLine(lineNum, content) {
        const trimmed = content.trim();
        
        // Skip comments and empty lines
        if (!trimmed || trimmed.startsWith(':::')) {
            return;
        }
        
        // Find LN references (e.g., LN1, LN5, ln10)
        const lnRefs = this.findLNReferences(content);
        if (lnRefs.length > 0) {
            this.lineReferences.set(lineNum, new Set(lnRefs));
            
            // Update reverse mapping - for each referenced line, note that current line depends on it
            lnRefs.forEach(refLine => {
                if (!this.lineDependencies.has(refLine)) {
                    this.lineDependencies.set(refLine, new Set());
                }
                this.lineDependencies.get(refLine).add(lineNum);
            });
        }
        
        // Find cross-sheet references (e.g., S.Sheet2.LN5, s.budget.ln3)
        const crossRefs = this.findCrossSheetReferences(content);
        if (crossRefs.length > 0) {
            this.crossSheetRefs.set(lineNum, new Set(crossRefs));
        }
    }
    
    /**
     * Find LN references in a line
     */
    findLNReferences(content) {
        const lnPattern = /\bLN(\d+)\b/gi;
        const matches = [];
        let match;
        
        while ((match = lnPattern.exec(content)) !== null) {
            const refLineNum = parseInt(match[1]);
            matches.push(refLineNum);
        }
        
        return [...new Set(matches)]; // Remove duplicates
    }
    
    /**
     * Find cross-sheet references in a line
     */
    findCrossSheetReferences(content) {
        const crossPattern = /\bS\.([^.]+)\.LN(\d+)\b/gi;
        const matches = [];
        let match;
        
        while ((match = crossPattern.exec(content)) !== null) {
            const sheetName = match[1];
            const lineNum = parseInt(match[2]);
            matches.push({ sheet: sheetName, line: lineNum });
        }
        
        return matches;
    }
    
    /**
     * Update dependencies when a single line changes
     */
    updateLineDependencies(lineNum, oldContent, newContent) {
        if (!this.enabled) return;
        
        PerformanceDebug.startTimer('dependency_update_line');
        
        // Remove old dependencies for this line
        const oldRefs = this.lineReferences.get(lineNum) || new Set();
        oldRefs.forEach(refLine => {
            const dependents = this.lineDependencies.get(refLine);
            if (dependents) {
                dependents.delete(lineNum);
                if (dependents.size === 0) {
                    this.lineDependencies.delete(refLine);
                }
            }
        });
        
        // Remove old cross-sheet references
        this.crossSheetRefs.delete(lineNum);
        this.lineReferences.delete(lineNum);
        
        // Add new dependencies
        this.analyzeLine(lineNum, newContent);
        
        PerformanceDebug.endTimer('dependency_update_line', { 
            lineNum,
            oldRefsCount: oldRefs.size,
            newRefsCount: (this.lineReferences.get(lineNum) || new Set()).size
        });
    }
    
    /**
     * Get all lines that depend on the given line (directly or indirectly)
     */
    getDependentLines(lineNum) {
        const dependents = new Set();
        const toProcess = [lineNum];
        const processed = new Set();
        
        while (toProcess.length > 0) {
            const currentLine = toProcess.pop();
            if (processed.has(currentLine)) continue;
            processed.add(currentLine);
            
            const directDependents = this.lineDependencies.get(currentLine) || new Set();
            directDependents.forEach(dependent => {
                if (!dependents.has(dependent)) {
                    dependents.add(dependent);
                    toProcess.push(dependent); // Check for transitive dependencies
                }
            });
        }
        
        return dependents;
    }
    
    /**
     * Determine which lines need recalculation based on changes
     */
    getLinesToRecalculate(changedLines, allLines) {
        if (!this.enabled) {
            // If dependency tracking is disabled, recalculate everything
            return new Set(allLines.map((_, index) => index + 1));
        }
        
        PerformanceDebug.startTimer('dependency_get_recalc_lines');
        
        const linesToRecalc = new Set();
        
        // Add all changed lines
        changedLines.forEach(lineNum => linesToRecalc.add(lineNum));
        
        // Add all lines that depend on changed lines
        changedLines.forEach(lineNum => {
            const dependents = this.getDependentLines(lineNum);
            dependents.forEach(dependent => linesToRecalc.add(dependent));
        });
        
        // Filter out comments and empty lines
        const validLines = new Set();
        allLines.forEach((line, index) => {
            const lineNum = index + 1;
            const trimmed = line.trim();
            if (trimmed && !trimmed.startsWith(':::')) {
                validLines.add(lineNum);
            }
        });
        
        // Only return lines that actually need calculation
        const result = new Set([...linesToRecalc].filter(lineNum => validLines.has(lineNum)));
        
        PerformanceDebug.endTimer('dependency_get_recalc_lines', {
            changedCount: changedLines.length,
            dependentCount: linesToRecalc.size,
            validCount: result.size,
            totalLines: allLines.length
        });
        
        return result;
    }
    
    /**
     * Check if content has actually changed (using hash comparison)
     */
    hasContentChanged(lineNum, content) {
        const hash = this.hashContent(content);
        const lastHash = this.lastContent.get(lineNum);
        
        if (lastHash !== hash) {
            this.lastContent.set(lineNum, hash);
            return true;
        }
        
        return false;
    }
    
    /**
     * Simple hash function for content comparison
     */
    hashContent(content) {
        let hash = 0;
        for (let i = 0; i < content.length; i++) {
            const char = content.charCodeAt(i);
            hash = ((hash << 5) - hash) + char;
            hash = hash & hash; // Convert to 32-bit integer
        }
        return hash;
    }
    
    /**
     * Get lines that have cross-sheet references
     */
    getLinesWithCrossSheetRefs() {
        return new Set(this.crossSheetRefs.keys());
    }
    
    /**
     * Enable or disable dependency tracking
     */
    setEnabled(enabled) {
        this.enabled = enabled;
        PerformanceDebug.logEvent('dependency_tracking_toggled', { enabled });
    }
    
    /**
     * Get dependency statistics for debugging
     */
    getStats() {
        return {
            totalLines: this.lastContent.size,
            linesWithDependencies: this.lineReferences.size,
            linesWithDependents: this.lineDependencies.size,
            linesWithCrossSheetRefs: this.crossSheetRefs.size,
            enabled: this.enabled
        };
    }
    
    /**
     * Clear all dependency data
     */
    clear() {
        this.lineDependencies.clear();
        this.lineReferences.clear();
        this.crossSheetRefs.clear();
        this.lastContent.clear();
    }
}

// Make available globally
window.DependencyTracker = DependencyTracker;

// Add console commands for debugging
window.depStats = () => {
    if (window.calcForgeApp && window.calcForgeApp.editor && window.calcForgeApp.editor.dependencyTracker) {
        const stats = window.calcForgeApp.editor.dependencyTracker.getStats();
        console.group('%c📊 Dependency Tracker Stats', 'font-size: 14px; font-weight: bold; color: #2196F3');
        console.log(`Total Lines: ${stats.totalLines}`);
        console.log(`Lines with Dependencies: ${stats.linesWithDependencies}`);
        console.log(`Lines with Dependents: ${stats.linesWithDependents}`);
        console.log(`Lines with Cross-Sheet Refs: ${stats.linesWithCrossSheetRefs}`);
        console.log(`Enabled: ${stats.enabled}`);
        console.groupEnd();
        return stats;
    } else {
        console.log('Dependency tracker not available');
        return null;
    }
};

window.toggleIncremental = () => {
    if (window.calcForgeApp && window.calcForgeApp.editor) {
        return window.calcForgeApp.editor.toggleIncrementalCalculation();
    } else {
        console.log('Editor not available');
        return false;
    }
};

console.log('%c🔗 Dependency Tracker Loaded', 'color: #4CAF50; font-weight: bold; font-size: 14px');
console.log('%cUse depStats() to see dependency info, toggleIncremental() to enable/disable smart calculation', 'color: #666');
