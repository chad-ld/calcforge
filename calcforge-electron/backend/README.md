# CalcForge Backend

This directory contains the extracted backend logic from the original Qt-based CalcForge application. The backend provides a REST API and WebSocket interface for the Electron frontend.

## Architecture

The backend is built using FastAPI and provides the following components:

### Core Components

- **`calcforge_engine.py`** - Main calculation engine with all mathematical operations
- **`api_server.py`** - FastAPI web server with REST and WebSocket endpoints  
- **`syntax_highlighter.py`** - Syntax highlighting logic converted from Qt to CSS classes
- **`worksheet_manager.py`** - Worksheet/tab management and file operations
- **`constants.py`** - All constants, colors, and configuration data

### Features Preserved

✅ **Mathematical Functions** - All math functions (sin, cos, sqrt, etc.)  
✅ **Timecode Calculations** - TC function with frame rate support  
✅ **Unit Conversions** - Using pint library for unit conversions  
✅ **Currency Conversions** - Real-time and fallback currency rates  
✅ **Date Arithmetic** - D function with business day calculations  
✅ **Statistical Functions** - sum, mean, median, etc.  
✅ **Cross-Sheet References** - S.SheetName.LN# syntax  
✅ **LN References** - Line number references with color coding  
✅ **Syntax Highlighting** - Converted to CSS classes for web frontend  
✅ **Aspect Ratio Calculator** - AR function for video dimensions  
✅ **Error Handling** - Comprehensive error reporting  

## Installation

1. **Install Python dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

2. **Run the API server:**
   ```bash
   python api_server.py
   ```

3. **Test the backend:**
   ```bash
   python test_backend.py
   ```

## API Endpoints

### REST API

- **`GET /`** - Health check
- **`POST /api/calculate`** - Calculate single expression
- **`POST /api/calculate-batch`** - Calculate multiple expressions
- **`POST /api/syntax-highlight`** - Get syntax highlighting data
- **`POST /api/worksheets/update`** - Update worksheet data for cross-sheet references
- **`GET /api/functions`** - Get available functions for autocompletion

### WebSocket

- **`WS /ws`** - Real-time calculation updates

## Usage Examples

### Single Calculation
```python
import requests

response = requests.post("http://localhost:8000/api/calculate", json={
    "expression": "2 + 3 * 4",
    "sheet_id": 0,
    "line_num": 1
})

result = response.json()
print(result["value"])  # 14
```

### Batch Calculation
```python
response = requests.post("http://localhost:8000/api/calculate-batch", json={
    "expressions": ["2 + 3", "5 * 4", "sqrt(16)"],
    "sheet_id": 0
})

results = response.json()["results"]
for result in results:
    print(result["value"])
```

### Syntax Highlighting
```python
response = requests.post("http://localhost:8000/api/syntax-highlight", json={
    "text": "sum(LN1:LN5) + TC(24, 100)"
})

highlights = response.json()["highlights"]
for highlight in highlights:
    print(f"{highlight['class']}: {highlight['start']}-{highlight['start']+highlight['length']}")
```

## Testing

Run the test suite to verify all functionality:

```bash
python test_backend.py
```

The test suite covers:
- Basic mathematical calculations
- Timecode functions
- Unit conversions
- Date arithmetic
- Syntax highlighting
- Worksheet management
- Error handling

## Development

### Adding New Functions

1. Add the function to `calcforge_engine.py`
2. Add the function name to `constants.py` FUNCTION_NAMES
3. Update the global namespace in CalcForgeEngine.__init__()
4. Add tests to `test_backend.py`

### API Documentation

When the server is running, visit:
- **Interactive docs:** http://localhost:8000/docs
- **ReDoc:** http://localhost:8000/redoc

## Migration Status

This backend represents **Phase 1** of the Electron migration plan:

✅ **Completed:**
- Core calculation engine extracted
- All mathematical functions preserved
- Syntax highlighting converted to CSS classes
- API server with REST and WebSocket endpoints
- Comprehensive testing

🔄 **Next Steps:**
- Frontend development (Phase 2)
- Electron integration (Phase 3)
- Cross-platform builds (Phase 4)

The backend is now ready to power the Electron frontend while preserving all existing CalcForge functionality.
