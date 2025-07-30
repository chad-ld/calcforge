# CalcForge v5.0 - Portable Distribution

## 🚀 **What is CalcForge?**

CalcForge is a professional-grade calculator application with advanced features for mathematical calculations, unit conversions, currency exchange, and much more. This is the **C++ native Windows version** built with Qt6 for maximum performance.

## 📦 **What's Included**

This portable distribution contains everything you need to run CalcForge:

### **Core Application**
- `CalcForge.exe` - Main application executable
- All required Qt6 libraries (Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Network.dll, etc.)
- Platform-specific plugins for Windows integration

### **Sample Data Files**
- `example_worksheets.json` - Example calculations and tutorials
- `comprehensive_worksheets.json` - Complete function reference examples
- `combined_worksheets.json` - Additional sample worksheets
- `exchange_rates.json` - Currency exchange rate data
- `api_key_example.txt` - Template for currency API setup

### **User Data** (Created automatically)
- `worksheets.json` - Your saved worksheets
- `recent_files.json` - Recently opened files
- `logs/` - Application logs for troubleshooting

## 🎯 **Key Features**

### **📊 Mathematical Functions**
- **Basic Operations**: +, -, *, /, ^, parentheses with proper precedence
- **Scientific Functions**: sin, cos, tan, sqrt, log, exp, abs, round, etc.
- **Statistical Functions**: sum, mean, median, min, max, variance, stdev, percentiles
- **Advanced Functions**: factorial, gcd, lcm, hyperbolic functions

### **🔗 LN References**
- **Line References**: Use LN1, LN5, LN42 to reference results from other lines
- **Cross-Sheet References**: S.SheetName.LN# to reference other worksheets
- **Auto-Updates**: LN references automatically update when lines are inserted/deleted
- **Works with all functions**: LN variables work with unit conversions, currency, etc.

### **📏 Unit Conversions (10 Categories)**
- **Distance**: meters, feet, inches, miles, kilometers, yards, centimeters
- **Weight**: pounds, kilograms, grams, ounces, tons
- **Volume**: liters, gallons, cubic meters, cubic feet, cubic inches, milliliters
- **Temperature**: Celsius, Fahrenheit, Kelvin
- **Time**: seconds, minutes, hours, days, weeks
- **Area**: square meters, acres, hectares, square feet, square miles, square inches
- **Power**: watts, kilowatts, horsepower, BTU per hour
- **Energy**: joules, kilojoules, calories, kilocalories, BTU, kilowatt hours
- **Velocity**: meters per second, kilometers per hour, miles per hour, feet per second, knots, mach
- **Pressure**: pascals, kilopascals, bar, atmospheres, psi, torr, millimeters of mercury

### **💰 Currency Conversions**
- **157+ Currencies**: USD, EUR, GBP, JPY, CAD, AUD, and many more
- **Live Updates**: Click the $ button to fetch current exchange rates
- **Offline Mode**: Uses cached rates when internet is unavailable

### **⚡ Special Functions**
- **Date Arithmetic**: D(July 4, 2023 + 30) → August 3, 2023
- **Timecode Calculations**: TC(24, 01:00:00:00 + 00:30:00:00) for video editing
- **Aspect Ratios**: AR(1920x1080, ?x720) → 1280x720 for graphics/video
- **Percentage Functions**: percent(25%, 1000) → 250, percent(250, %, 1000) → 25%

## 🚀 **Getting Started**

### **Recent Updates**
✅ **Flicker-Free Startup** - Completely eliminated visual flicker with synchronous worksheet loading and deferred calculations
✅ **Performance Optimizations** - Significantly improved startup time from ~3 seconds to ~1 second through lazy loading and deferred operations
✅ **Version 5.0 Release** - Major version update reflecting significant architectural improvements and feature enhancements
✅ **Cross-Sheet Navigation Fix** - Ctrl+Enter and Ctrl+Backspace shortcuts now work correctly with sheet names containing ampersands (&) and other special characters

### **Installation**
1. **No installation required!** This is a portable application
2. Extract the ZIP file to any folder on your computer
3. Double-click `CalcForge.exe` to run

### **First Steps**
1. **Load Examples**: Click the folder icon → Open → `example_worksheets.json`
2. **Try Basic Math**: Type `2 + 2` in the left column, see `4` in the right
3. **Use LN References**: Type `LN1 * 10` to multiply the first line's result by 10
4. **Try Unit Conversion**: Type `100 fahrenheit to celsius`
5. **Get Help**: Click the `?` button for comprehensive documentation

### **Example Calculations**
```
5 + 3 * 2                    → 11
sqrt(16)                     → 4
100 fahrenheit to celsius    → 37.777778 Celsius
1000 watts to horsepower     → 1.341 horsepower
60 mph to km/h               → 96.5606 km/h
14.7 psi to atmospheres      → 1.0 atm
1000 calories to joules      → 4184 joules
mean(1, 5, 10, 15, 20)      → 10.2
percent(25%, 1000)          → 250
D(July 4, 2023 + 30)        → August 3, 2023
```

## 🎨 **User Interface**

### **Layout**
- **Left Column**: Expression Editor - Type your calculations here
- **Right Column**: Results Display - See results automatically
- **Tabs**: Multiple worksheets, click + to add new tabs
- **Splitter**: Drag the middle line to resize columns

### **Keyboard Shortcuts**
- **Ctrl + Period**: Increase font size
- **Ctrl + Comma**: Decrease font size
- **Ctrl + 0**: Reset font size
- **Ctrl + PageUp/PageDown**: Navigate between tabs
- **Ctrl + Left/Right**: Smart selection of numbers and LN references
- **Ctrl + C**: Copy result value when no text is selected

## 🔧 **Advanced Features**

### **Autocomplete**
- Type function names and press Tab for suggestions
- Unit conversions: Type "100 m" → select unit → "to" → select target unit
- Currency: Type "100 USD" → select currency → "to" → select target currency

### **File Operations**
- **Save**: Floppy disk icon to save current worksheets
- **Load**: Folder icon to load worksheet files
- **Recent Files**: Dropdown arrows for quick access to recent files

### **Currency API Setup** (Optional)
1. Get a free API key from exchangerate-api.com
2. Create `api_key.txt` in the CalcForge folder
3. Put your API key in the file
4. Click the $ button to update exchange rates

## 📁 **File Formats**

CalcForge uses JSON format for worksheets:
- **Portable**: Works across different computers
- **Human-readable**: Can be edited in text editors if needed
- **Version-controlled**: Works well with Git and other VCS

## 🛠 **Troubleshooting**

### **Common Issues**
- **Won't start**: Make sure all DLL files are in the same folder as CalcForge.exe
- **Missing results**: Check syntax - functions are case-sensitive
- **Currency not updating**: Check internet connection and API key setup
- **Performance**: Check logs/ folder for any error messages

### **System Requirements**
- **Windows 10/11** (64-bit)
- **4GB RAM** minimum, 8GB recommended
- **50MB disk space** for application and data files
- **Internet connection** for currency updates (optional)

## 📞 **Support**

- **Help**: Click the `?` button in CalcForge for comprehensive documentation
- **Examples**: Load `comprehensive_worksheets.json` for function reference
- **Logs**: Check the `logs/` folder for detailed error information

## 🎉 **Version Information**

**CalcForge v5.0 C++**
- Built with Qt 6.9.1 and MSVC 2022
- All features from Python and Electron versions
- Native Windows performance
- Complete unit conversion system with 10 categories
- Extended unit system: power, energy, velocity, pressure conversions
- LN variables work with all special functions
- Professional-grade calculation engine

---

**Enjoy using CalcForge!** 🚀
