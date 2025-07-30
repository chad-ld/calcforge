# CalcForge C++ Version

## ⚡ High-Performance Native C++ Implementation

This will be the native C++ Qt implementation of CalcForge for maximum performance and standalone distribution.

### **Planned Features**
- 🎯 Complete feature parity with Python version
- ⚡ High-performance calculation engine
- 📦 Standalone executable (no dependencies)
- 🖥️ Native desktop experience
- 💾 Minimal memory footprint
- 🚀 Fast startup and calculation times

### **Technology Stack**
- **Framework**: Qt 6.5+ C++
- **Build System**: CMake
- **Compiler**: MSVC 2019+ / GCC 9+ / Clang
- **Dependencies**: Qt6Core, Qt6Widgets, Qt6Network

### **Development Status**
🚧 **Planning Phase** - See `../CALCFORGE_CPP_CONVERSION_PLAN.md` for detailed roadmap

### **Planned Timeline**
- **Week 1**: Project setup and basic UI
- **Week 2**: Calculation engine and core functionality  
- **Week 3**: Advanced features (units, dates, cross-sheet)
- **Week 4**: UI polish, testing, and distribution

### **Build Instructions**

#### **Option A: Visual Studio Code (Recommended)**
See [VSCODE_SETUP.md](./VSCODE_SETUP.md) for complete VS Code setup guide.

**Quick Setup:**
1. Install **MinGW-w64** via [MSYS2](https://www.msys2.org/)
2. Install **Qt6** from [qt.io](https://www.qt.io/download)
3. Install **CMake** from [cmake.org](https://cmake.org/download/)
4. Open folder in VS Code: `code calcforge-cpp`
5. Build with **Ctrl+Shift+B**

#### **Option B: Command Line Build**
```bash
# MinGW build (recommended)
build-mingw.bat

# Or manual build
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

#### **Option C: Visual Studio 2022**
```bash
# Visual Studio build
build.bat

# Or manual build
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

#### **Current Status: Hello World Test**
The current version is a simple "Hello World" Qt application to test the build system.
Once the build is working, we'll implement the full CalcForge functionality.

### **Target Performance Goals**
- **Startup Time**: < 1 second
- **Calculation Speed**: > 1000 lines/second  
- **Memory Usage**: < 50MB
- **Executable Size**: < 20MB

### **Distribution Targets**
- Windows (standalone EXE)
- Linux (AppImage)
- macOS (DMG bundle)

This version will provide the best performance and user experience for CalcForge.
