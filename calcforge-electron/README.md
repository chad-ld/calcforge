# CalcForge Electron

Modern desktop calculator with advanced features including timecode calculations, unit conversions, and cross-sheet references.

## Features

- **Advanced Mathematics** - All standard math functions plus statistics
- **Timecode Calculations** - Professional video timecode with frame rate support
- **Unit Conversions** - Comprehensive unit conversion system
- **Currency Conversion** - Real-time currency exchange rates
- **Date Arithmetic** - Date calculations with business day support
- **Cross-Sheet References** - Link calculations across multiple worksheets
- **Syntax Highlighting** - Color-coded expressions with error detection
- **Real-time Results** - Instant calculation as you type
- **Professional UI** - GitHub dark theme with modern design

## Installation

### Prerequisites

- **Node.js** (v16 or higher)
- **Python** (v3.8 or higher) with required packages:
  ```bash
  pip install fastapi uvicorn pint requests
  ```

### Development Setup

1. **Clone and install dependencies:**
   ```bash
   git clone <repository-url>
   cd calcforge-electron
   npm install
   ```

2. **Start development mode:**
   ```bash
   npm run dev
   ```
   This will start both the Python backend and Electron app.

3. **Start components separately:**
   ```bash
   # Backend only
   npm run backend
   
   # Electron only (requires backend running)
   npm start
   ```

### Building for Distribution

1. **Build for current platform:**
   ```bash
   npm run build
   ```

2. **Build for specific platforms:**
   ```bash
   npm run build-win    # Windows
   npm run build-mac    # macOS
   npm run build-linux  # Linux
   ```

3. **Create portable build:**
   ```bash
   npm run pack
   ```

## Architecture

### Backend (Python)
- **FastAPI** web server providing REST and WebSocket APIs
- **Calculation Engine** with all mathematical operations
- **Syntax Highlighting** logic converted to CSS classes
- **Worksheet Management** for cross-sheet references

### Frontend (HTML/CSS/JavaScript)
- **Modern Web Technologies** with GitHub dark theme
- **Real-time Communication** with backend via WebSocket
- **Advanced Editor** with syntax highlighting and autocompletion
- **Tab Management** for multiple worksheets

### Electron Integration
- **Native Desktop App** with proper window management
- **File System Access** for save/load operations
- **Native Menus** with keyboard shortcuts
- **Auto-updater** support for seamless updates

## Usage

### Basic Calculations
```
2 + 3 * 4
sqrt(16)
sin(pi/2)
```

### Timecode Functions
```
TC(24, 100)              # 100 frames at 24fps
TC(30, "00:01:00:00")    # Convert timecode to frames
TC(29.97, "01:00:00;00") # Drop frame timecode
```

### Unit Conversions
```
5 feet to meters
100 pounds to kilograms
1 gallon to liters
```

### Currency Conversions
```
100 dollars to euros
50 pounds to yen
```

### Date Arithmetic
```
D(July 4, 2023)          # Format date
D(July 4, 2023 + 30)     # Add 30 days
D(July 4, 2023 W+ 5)     # Add 5 business days
```

### Line References
```
LN1                      # Reference line 1
sum(LN1:LN5)            # Sum lines 1-5
S.Sheet2.LN3            # Cross-sheet reference
```

### Statistical Functions
```
sum(1,2,3,4,5)
mean(LN1:LN10)
median(above)
max(below)
```

## Development

### Project Structure
```
calcforge-electron/
├── backend/             # Python FastAPI backend
│   ├── calcforge_engine.py
│   ├── api_server.py
│   └── ...
├── frontend/            # HTML/CSS/JavaScript frontend
│   └── src/
│       ├── index.html
│       ├── styles/
│       └── scripts/
├── electron/            # Electron main process
│   ├── main.js
│   └── preload.js
├── build/               # Build configuration
└── dist/                # Built applications
```

### Adding New Functions

1. **Backend:** Add function to `calcforge_engine.py`
2. **Frontend:** Update autocompletion in `autocomplete.js`
3. **Constants:** Add to `constants.py` if needed
4. **Tests:** Add tests to `test_backend.py`

### Building Icons

Place icon files in `build/icons/`:
- `icon.ico` (Windows)
- `icon.icns` (macOS)
- `icon.png` (Linux)

## Troubleshooting

### Backend Won't Start
- Ensure Python is installed and in PATH
- Install required packages: `pip install -r backend/requirements.txt`
- Check port 8000 is not in use

### Build Fails
- Ensure all dependencies are installed: `npm install`
- Check Node.js version: `node --version` (should be v16+)
- Clear cache: `npm run clean` (if available)

### App Won't Launch
- Check backend is running on port 8000
- Verify all files are present in build
- Check console for error messages

## License

MIT License - see LICENSE file for details.

## Support

For issues and feature requests, please use the GitHub issue tracker.
