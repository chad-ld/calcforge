# CalcForge v4.0 - Deployment Information

## 📦 **Deployment Package Details**

### **Package Information**
- **Version**: CalcForge v4.0 C++ Native Windows
- **Build Date**: 2025-07-18
- **Platform**: Windows 64-bit
- **Compiler**: MSVC 2022
- **Qt Version**: 6.9.1

### **Package Sizes**
- **Portable ZIP**: 16.2 MB (`CalcForge-Portable.zip`)
- **Extracted Folder**: 38.5 MB (`deploy/CalcForge/`)
- **Main Executable**: CalcForge.exe

### **Distribution Formats**
1. **Portable ZIP** (`CalcForge-Portable.zip`)
   - Complete standalone package
   - No installation required
   - Extract and run anywhere
   - Includes all dependencies

2. **Extracted Folder** (`deploy/CalcForge/`)
   - Ready-to-run deployment
   - All Qt libraries included
   - Sample worksheets included
   - Documentation included

## 🚀 **What's Included**

### **Core Application**
- `CalcForge.exe` - Main application (native Windows executable)
- Complete Qt6 runtime libraries
- Platform-specific plugins for Windows integration
- Image format support (GIF, ICO, JPEG, PDF, SVG)
- Network plugins for currency API integration

### **Documentation**
- `README.md` - Complete user guide and feature overview
- `VERSION.txt` - Detailed version and feature information
- Built-in help system (accessible via ? button in app)

### **Sample Data**
- `example_worksheets.json` - Tutorial and example calculations
- `comprehensive_worksheets.json` - Complete function reference
- `combined_worksheets.json` - Additional sample worksheets
- `exchange_rates.json` - Currency exchange rate data
- `api_key_example.txt` - Template for currency API setup

### **User Data** (Created automatically)
- `worksheets.json` - User's saved worksheets
- `recent_files.json` - Recently opened files list
- `logs/` - Application logs for troubleshooting

## ✨ **Key Features Included**

### **Mathematical Engine**
- ✅ **30+ Mathematical Functions**: sin, cos, tan, sqrt, log, exp, factorial, etc.
- ✅ **15+ Statistical Functions**: sum, mean, median, variance, percentiles, etc.
- ✅ **Expression Parser**: Recursive descent parser with proper precedence
- ✅ **Error Handling**: Graceful error messages and recovery

### **Advanced Features**
- ✅ **LN Reference System**: Reference results from other lines (LN1, LN5, etc.)
- ✅ **Cross-Sheet References**: S.SheetName.LN# for multi-worksheet calculations
- ✅ **Auto-Updates**: LN references update automatically when lines change
- ✅ **Circular Reference Detection**: Prevents infinite calculation loops

### **Unit Conversion System (6 Categories)**
- ✅ **Distance**: meters, feet, inches, miles, kilometers, yards, centimeters
- ✅ **Weight**: pounds, kilograms, grams, ounces, tons
- ✅ **Volume**: liters, gallons, cubic meters, cubic feet, milliliters
- ✅ **Temperature**: Celsius, Fahrenheit, Kelvin
- ✅ **Time**: seconds, minutes, hours, days, weeks
- ✅ **Area**: square meters, acres, hectares, square feet, square miles

### **Special Functions**
- ✅ **Currency Conversions**: 157+ currencies with live API updates
- ✅ **Date Functions**: Professional date arithmetic with business days
- ✅ **Timecode Functions**: Video editing timecode calculations
- ✅ **Aspect Ratio Functions**: Graphics/video resolution calculations
- ✅ **Percentage Functions**: Complete percentage calculation system
- ✅ **Solve Functions**: Equation solving capabilities

### **User Interface**
- ✅ **Material Design**: Modern, clean interface
- ✅ **Tab System**: Multiple worksheets with close buttons
- ✅ **Syntax Highlighting**: Color-coded expressions
- ✅ **Autocomplete**: Function and unit suggestions
- ✅ **Synchronized Scrolling**: Expression and results columns
- ✅ **Keyboard Shortcuts**: Full navigation and control
- ✅ **File Operations**: Save/load with recent files

## 🎯 **Recent Updates in This Build**

### **LN Variable Fixes**
- **Fixed LN processing order**: LN variables now work with all special functions
- **Unit conversions with LN**: `LN5 celsius to kelvin` now works correctly
- **Currency conversions with LN**: `LN10 USD to EUR` now works correctly
- **All special functions**: LN variables work with timecode, dates, percentages, etc.

### **Expanded Unit Conversions**
- **Added area conversions**: acres, hectares, square meters, square feet, etc.
- **Expanded volume conversions**: cubic meters, cubic feet, cubic inches, etc.
- **Added time conversions**: seconds, minutes, hours, days, weeks
- **Updated from 5 to 6 unit categories**

### **Documentation Updates**
- **Updated help system**: All new conversions documented
- **Added LN examples**: Unit and currency conversion examples with LN variables
- **Comprehensive coverage**: All 6 unit categories fully documented

## 🛠 **System Requirements**

### **Minimum Requirements**
- **OS**: Windows 10 (64-bit) or Windows 11
- **RAM**: 4GB minimum
- **Storage**: 50MB free space
- **Network**: Internet connection for currency updates (optional)

### **Recommended**
- **RAM**: 8GB or more
- **Storage**: 100MB free space for user data and logs
- **Network**: Stable internet for real-time currency updates

## 📋 **Installation Instructions**

### **For End Users**
1. Download `CalcForge-Portable.zip`
2. Extract to any folder (e.g., `C:\CalcForge\`)
3. Double-click `CalcForge.exe` to run
4. No installation or admin rights required

### **For Developers**
1. Use the `deploy/CalcForge/` folder for testing
2. All source code available in the main repository
3. Build system uses CMake + MSVC 2022
4. Qt 6.9.1 required for compilation

## 🔧 **Deployment Process**

This deployment was created using:
1. **Build**: `build-quick.bat` (MSVC 2022 + Qt 6.9.1)
2. **Deploy**: `deploy-calcforge.bat` (windeployqt + packaging)
3. **Test**: Verified all features work in deployed version
4. **Package**: Created portable ZIP for distribution

## 📞 **Support Information**

### **Built-in Help**
- Click the `?` button in CalcForge for comprehensive documentation
- Load sample worksheets for examples and tutorials
- Check `logs/` folder for detailed error information

### **File Formats**
- **Worksheets**: JSON format (human-readable, version-control friendly)
- **Exchange Rates**: JSON format (auto-updated via API)
- **Logs**: Plain text format for troubleshooting

---

**CalcForge v4.0 - Professional calculation engine with native Windows performance** 🚀
