# CalcForge Performance Debugging Guide

## Overview

CalcForge now includes a comprehensive performance debugging system that helps identify performance bottlenecks and sluggishness in the application. This system can be easily toggled on/off without code changes.

## Quick Start

1. **Open the app** and press `F12` to open Developer Tools
2. **In the Console tab**, you'll see the performance debugging system has loaded
3. **Type these commands** to interact with the system:
   - `perfReport()` - Show detailed performance report
   - `perfReset()` - Reset all performance metrics
   - `perfToggle()` - Enable/disable performance debugging

## What Gets Tracked

The performance debugging system automatically tracks:

### 🧮 Calculations
- Total calculation count
- Average calculation time
- Slowest and fastest calculations
- Calculations per second
- Individual expression performance

### 🌐 Network Requests
- API call count and timing
- Network request performance
- Request/response sizes
- Error tracking

### 🎨 UI Updates
- Line number updates
- Results display rendering
- Syntax overlay updates
- UI update frequency

### ⏳ Debouncing
- How often calculations are skipped due to debouncing
- Debounce effectiveness

### 🔄 Loading Overlay
- How often the "processing" overlay appears
- Overlay show/hide timing

## Understanding the Performance Report

When you run `perfReport()`, you'll see output like this:

```
📊 CalcForge Performance Report
⏱️ Session Info
  Duration: 45.23s

🧮 Calculations
  Total: 15
  Average Time: 23.45ms
  Rate: 0.33/sec
  Slowest: 156.78ms (complex_expression)
  Fastest: 8.12ms (1+1)

🌐 Network
  Total Requests: 15
  Average Time: 18.23ms
  Rate: 0.33/sec

🎨 UI Updates
  Total Updates: 45
  Average Time: 2.34ms
  Rate: 1.00/sec

⏳ Debouncing
  Skips: 23
  Rate: 0.51/sec

🔄 Loading Overlay
  Shows: 3
  Rate: 0.07/sec
```

## Interpreting Results

### 🚨 Performance Issues to Look For

1. **High Calculation Times**
   - Average > 50ms: Moderate concern
   - Average > 100ms: Significant concern
   - Individual calculations > 200ms: Investigate specific expressions

2. **High Network Times**
   - Average > 100ms: Network/backend bottleneck
   - High request rate with slow responses: Backend overload

3. **Frequent UI Updates**
   - UI updates > 10/sec: Possible UI thrashing
   - High average UI update time: DOM manipulation issues

4. **Excessive Overlay Shows**
   - Shows > 1/sec: Calculations taking too long
   - Many shows with quick hides: Threshold tuning needed

5. **Low Debounce Effectiveness**
   - High skip rate: Good (prevents unnecessary work)
   - Low skip rate: May need longer debounce delays

## Common Performance Bottlenecks

### 1. **"Processing" Overlay Appearing for Simple Calculations**
**Symptoms:** Overlay shows for `1+1` type calculations
**Likely Causes:**
- Network latency to backend
- Backend processing overhead
- UI update delays

**Investigation:**
```javascript
// Check if network is the bottleneck
perfReport()
// Look at Network > Average Time vs Calculations > Average Time
```

### 2. **Sluggish Typing Response**
**Symptoms:** Delay between typing and UI updates
**Likely Causes:**
- Excessive UI updates
- Heavy DOM manipulation
- Inefficient debouncing

**Investigation:**
```javascript
// Type a few characters, then check:
perfReport()
// Look at UI Updates > Rate and Debouncing > Rate
```

### 3. **Slow Calculation Results**
**Symptoms:** Long delays before results appear
**Likely Causes:**
- Backend calculation complexity
- Network issues
- Large data processing

**Investigation:**
```javascript
// Enter some calculations, then:
perfReport()
// Look at Calculations > Average Time and slowest calculation
```

## Debugging Workflow

1. **Start Fresh**
   ```javascript
   perfReset()
   ```

2. **Reproduce the Issue**
   - Type the problematic input
   - Perform the slow operation
   - Note the user experience

3. **Check Performance**
   ```javascript
   perfReport()
   ```

4. **Analyze Results**
   - Identify the highest time consumers
   - Look for unusual patterns
   - Compare rates and averages

5. **Make Changes**
   - Adjust debounce timings
   - Optimize UI updates
   - Investigate backend issues

6. **Verify Improvements**
   ```javascript
   perfReset()
   // Repeat the operation
   perfReport()
   ```

## Configuration

### Enabling/Disabling
```javascript
// Disable all performance tracking
PerformanceDebug.setEnabled(false)

// Re-enable
PerformanceDebug.setEnabled(true)

// Or use the shortcut
perfToggle()
```

### Custom Tracking
```javascript
// Add custom timing
PerformanceDebug.startTimer('my_operation')
// ... do something ...
PerformanceDebug.endTimer('my_operation', { customData: 'value' })

// Log custom events
PerformanceDebug.logEvent('user_action', { action: 'clicked_button' })
```

## Performance Targets

### Good Performance
- Calculation average: < 30ms
- Network average: < 50ms
- UI update average: < 5ms
- Overlay shows: < 0.1/sec for simple operations

### Acceptable Performance
- Calculation average: < 100ms
- Network average: < 150ms
- UI update average: < 15ms
- Overlay shows: < 0.5/sec

### Poor Performance (Needs Investigation)
- Calculation average: > 100ms
- Network average: > 150ms
- UI update average: > 15ms
- Overlay shows: > 1/sec for simple operations

## Next Steps

After identifying performance issues:

1. **Backend Issues**: Check `calcforge_engine.py` and `api_server.py`
2. **Network Issues**: Investigate WebSocket vs REST API usage
3. **UI Issues**: Optimize DOM manipulation in `editor.js`
4. **Debouncing Issues**: Adjust timing constants in `editor.js`

The performance debugging system provides the data - use it to guide optimization efforts!
