# CalcForge C++ Version
CalcForge is a high-performance C++ number cruncher that feels like a scientific calculator and spreadsheet had a baby... and then mutated an extra arm!

The idea for CalcForge came about because I found myself constantly using the default Windows calculator for quick number crunching. This could be anything from standard math operations to converting timecode for animation/film/video projects. The default calculator was fine for super simple stuff, but I was frustrated by its limitations and how it stored data long term. I knew I could open up a spreadsheet app and do more advanced number crunching that would resolve a lot of those issues, but that also seemed like using a bazooka to kill a fly. 

I searched around the web and found a few apps that attempted to hit the middle ground I was looking for, but they had all either stopped development, sported clunky UI's, or just lacked the entire suite of features I desired. Having exhausted my options, I decided to take matters into my own hands, and thus CalcForge was born! I started off writing it in Python, tried Electron to see if there were benefits from that approach, and then finally decided on a pure C++ approach for the best performance. Hope someone finds it useful and I'd love to hear any feedback or ideas for additional features!

## ⚡ High-Performance Native C++ Implementation

### **✅ IMPLEMENTED FEATURES**

#### **🚀 Core Application**
- ✅ **Complete UI**: Custom header, dynamic tabs, expression/results panels
- ✅ **Calculation Engine**: Recursive descent parser with proper operator precedence
- ✅ **LN Reference System**: Auto-updating line references (LN1, LN2, etc.)
- ✅ **Cross-Sheet References**: S.SheetName.LN5 syntax with error handling
- ✅ **Syntax Highlighting**: Color-coded expressions with 17 LN variable colors
- ✅ **Auto-completion**: Functions, units, currencies, cross-sheet references
- ✅ **File Operations**: Save/load worksheets, recent files, JSON format

#### **🧮 Mathematical Functions** ✅ **100% COMPLETE - FULL FEATURE PARITY**
- ✅ **Basic Math**: sin, cos, tan, sqrt, abs, log, log10, log2, exp, floor, ceil
- ✅ **Inverse Trig**: asin, acos, atan ✅ **IMPLEMENTED**
- ✅ **Hyperbolic**: sinh, cosh, tanh, asinh, acosh, atanh ✅ **IMPLEMENTED**
- ✅ **Utilities**: degrees, radians, factorial, gcd, lcm, pow ✅ **IMPLEMENTED**
- ✅ **Statistical Functions**: sum, mean, min, max, median, variance, stdev, mode, perc5, perc95, meanfps ✅ **ALL IMPLEMENTED**
- ✅ **Range Operations**: sum(1-5), mean(above), max(below), percentile methods

#### **🔄 Advanced Features** ✅ **100% COMPLETE**
- ✅ **Unit Conversion**: Distance, weight, volume, temperature, time (5 categories)
- ✅ **Currency Conversion**: 157+ currencies with live API updates via $ button
- ✅ **Date Functions**: Professional date arithmetic with business day support
- ✅ **Timecode Functions**: Complete TC() system with drop frame support
- ✅ **Aspect Ratio**: AR() calculator for video/graphics resolution scaling
- ✅ **Percentage Functions**: Complete percent() system with 4 calculation phases
- ✅ **Cross-Sheet LN Auto-Updates**: Updates LN references across ALL sheets ✅ **IMPLEMENTED**
- ✅ **Circular Reference Detection**: Advanced cycle detection ✅ **IMPLEMENTED**

#### **🎮 User Experience** ✅ **100% COMPLETE**
- ✅ **Keyboard Shortcuts**: Font size, tab navigation, smart text selection
- ✅ **Synchronized Scrolling**: Perfect alignment between expression/results panels
- ✅ **Material Design**: Flat scrollbars, modern styling
- ✅ **Performance**: Fast startup, efficient memory usage (~15-20MB)

### **✅ FEATURE PARITY ACHIEVED - NO CRITICAL GAPS**
- **Mathematical Functions**: ALL 25+ functions implemented ✅ **COMPLETE**
- **Advanced Cross-Sheet**: ALL features implemented ✅ **COMPLETE**
- **Production Ready**: Feature-complete and ready for professional use ✅

### **🛠️ Technology Stack**
- **Framework**: Qt 6.9.1 C++ with MSVC 2022
- **Build System**: CMake with automated batch scripts
- **Performance**: Optimized Release builds, efficient algorithms
- **Dependencies**: Qt6Core, Qt6Widgets, Qt6Network (bundled in executable)

### **🚀 Quick Start**

#### **Prerequisites**
- **Qt 6.9.1** with MSVC 2022 64-bit component
- **Visual Studio 2022** with C++ development tools
- **CMake 3.16+** (included with Visual Studio)

#### **⚡ Fast Development Build** (Recommended)
```bash
cd calcforge-cpp

# Kill any running CalcForge processes (REQUIRED)
taskkill /F /IM CalcForge.exe

# Quick incremental build (10-30 seconds)
.\build-quick.bat

# Launch application
.\run-calcforge.bat
```

#### **🔧 Full Clean Build** (For releases)
```bash
cd calcforge-cpp

# Kill any running CalcForge processes (REQUIRED)
taskkill /F /IM CalcForge.exe

# Full rebuild (1-2 minutes)
.\build.bat

# Launch application
.\run-calcforge.bat
```

#### **⚠️ CRITICAL: Process Management**
**Always kill existing CalcForge processes before building!**
- Multiple instances cause file access errors during builds
- Use: `taskkill /F /IM CalcForge.exe` before each build

### **📊 Current Performance Metrics**
- **Startup Time**: ~2-3 seconds (target: <1 second) 🔄
- **Calculation Speed**: >100 lines/second for basic arithmetic ✅
- **Memory Usage**: ~15-20MB for typical worksheets ✅
- **Executable Size**: ~8MB standalone ✅
- **UI Responsiveness**: Smooth scrolling and real-time calculation ✅

### **🎯 Usage Examples**

```cpp
// Basic calculations with LN references
100 + 50                    // Line 1: 150
LN1 * 2                     // Line 2: 300 (references line 1)
sqrt(LN2)                   // Line 3: 17.32 (square root of line 2)

// Unit conversions
5 miles to km               // 8.047 kilometers
10 pounds to kg             // 4.536 kilograms
100 fahrenheit to celsius   // 37.78 celsius

// Currency conversions (157+ currencies)
100 usd to eur              // Live exchange rate conversion
50 gbp to jpy               // British pounds to Japanese yen

// Date arithmetic
D.July 4, 2023 + 30         // August 3, 2023
D.01.01.2024 W+ 5           // Add 5 business days

// Timecode operations
TC(24, 01:00:00:00 + 00:30:00:00)  // Timecode arithmetic

// Aspect ratio calculations
AR(1920x1080, ?x2000)       // Solve for width: 3556x2000

// Statistical functions
sum(1-5)                    // Sum of lines 1 through 5
mean(above)                 // Average of all lines above
perc95(1-10, linear)        // 95th percentile with linear interpolation

// Cross-sheet references
S.Budget.LN5                // Reference line 5 from Budget sheet
max(LN1, S.Data.LN3, 100)   // Mix local and cross-sheet references
```

### **📁 Important File Locations**
- **Source**: `calcforge-cpp/` (for editing code)
- **Executable**: `calcforge-cpp/build/Release/CalcForge.exe`
- **Worksheets**: `calcforge-cpp/build/Release/worksheets.json` (runtime location)
- **Exchange Rates**: `calcforge-cpp/build/Release/exchange_rates.json`

### **🔗 Additional Documentation**
- **Detailed Implementation Status**: See `../CALCFORGE_CPP_CONVERSION_PLAN.md`
- **Performance Optimization**: See `PERFORMANCE_BUILD.md`
- **VS Code Setup**: See `VSCODE_SETUP.md`

### **🎉 Ready for Daily Use**
CalcForge C++ is now a fully functional advanced calculator with comprehensive mathematical capabilities, perfect for professional use in engineering, finance, video production, and scientific calculations.
