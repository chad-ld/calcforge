# CalcForge Project Structure

## 📁 Repository Organization

This repository contains three different implementations of CalcForge, each optimized for different use cases and deployment scenarios.

```
calcforge/
├── 📄 CALCFORGE_CPP_CONVERSION_PLAN.md    # Detailed C++ conversion roadmap
├── 📄 PROJECT_STRUCTURE.md                # This file
├── 📄 README.md                           # Main project overview
├── 📄 LICENSE                             # Project license
├── 🖼️ calcforge.ico/.png/.icns            # Shared application icons
├── 📄 calcforge.code-workspace             # VS Code workspace
│
├── 📁 calcforge-python/                   # 🐍 Original Python Implementation
│   ├── 📄 README.md                       # Python version documentation
│   ├── 🐍 calcforge.py                    # Main Python application
│   ├── 📄 calcforge.spec                  # PyInstaller build spec
│   ├── 📄 setup.py                        # Python package setup
│   ├── 📄 worksheets.json                 # Default worksheet data
│   ├── 🖼️ calcforge.ico/.png/.icns        # Application icons
│   ├── 📁 build/                          # PyInstaller build artifacts
│   ├── 📁 dist/                           # Built Python executable
│   └── 📁 __pycache__/                    # Python cache files
│
├── 📁 calcforge-electron/                 # ⚡ Modern Electron Implementation
│   ├── 📄 README.md                       # Electron version documentation
│   ├── 📄 package.json                    # Node.js dependencies and scripts
│   ├── 📄 package-lock.json               # Locked dependency versions
│   ├── 📁 electron/                       # Electron main process
│   │   ├── 🔧 main.js                     # Main Electron process
│   │   └── 🔧 preload.js                  # Preload script
│   ├── 📁 frontend/                       # HTML/CSS/JavaScript UI
│   │   └── 📁 src/                        # Frontend source files
│   │       ├── 🌐 index.html              # Main HTML file
│   │       ├── 🎨 styles/                 # CSS stylesheets
│   │       └── 🔧 scripts/                # JavaScript modules
│   ├── 📁 backend/                        # Python FastAPI backend
│   │   ├── 🐍 api_server.py               # FastAPI web server
│   │   ├── 🐍 calcforge_engine.py         # Calculation engine
│   │   ├── 🐍 syntax_highlighter.py       # Syntax highlighting
│   │   ├── 🐍 worksheet_manager.py        # Worksheet management
│   │   ├── 🐍 constants.py                # Shared constants
│   │   └── 📄 requirements.txt            # Python dependencies
│   ├── 📁 build/                          # Electron build configuration
│   │   └── 📁 icons/                      # Application icons
│   ├── 📁 dist/                           # Built Electron executables
│   │   ├── 💻 CalcForge 4.0.0.exe         # Portable executable
│   │   └── 📦 CalcForge Setup 4.0.0.exe   # NSIS installer
│   └── 📁 logs/                           # Application logs
│
├── 📁 calcforge-cpp/                      # 🚀 High-Performance C++ Implementation
│   ├── 📄 README.md                       # C++ version documentation
│   ├── 🔧 CMakeLists.txt                  # CMake build configuration
│   ├── 🖼️ calcforge.ico/.png/.icns        # Application icons
│   ├── 📁 src/                            # C++ source files
│   │   └── 🔧 main.cpp                    # Basic Qt application (placeholder)
│   ├── 📁 include/                        # C++ header files
│   └── 📁 resources/                      # Application resources
│
└── 📁 new_design/                         # 🎨 UI Design References
    ├── 🖼️ interface_changes.png           # UI mockups
    └── 🌐 newdesign.html                  # HTML design reference
```

## 🎯 Implementation Comparison

| Feature | Python | Electron | C++ |
|---------|--------|----------|-----|
| **Performance** | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Memory Usage** | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Startup Time** | ⭐⭐⭐ | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Cross-Platform** | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Development Speed** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| **Modern UI** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Distribution** | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

## 🚀 Current Status

### **calcforge-python/** ✅ **STABLE**
- **Status**: Complete and functional
- **Use Case**: Reference implementation, development testing
- **Pros**: Fast development, complete feature set
- **Cons**: Requires Python runtime

### **calcforge-electron/** ✅ **COMPLETE**
- **Status**: Feature complete, builds to EXE
- **Use Case**: Modern UI, cross-platform distribution
- **Pros**: Modern web UI, good cross-platform support
- **Cons**: Larger memory footprint, requires Python backend

### **calcforge-cpp/** 🚧 **PLANNED**
- **Status**: Basic structure created, ready for development
- **Use Case**: Maximum performance, standalone distribution
- **Pros**: Best performance, smallest footprint, truly standalone
- **Cons**: Longer development time

## 🛠️ Development Workflow

### **Working on Python Version**
```bash
cd calcforge-python
python calcforge.py
```

### **Working on Electron Version**
```bash
cd calcforge-electron
npm run dev                    # Development mode
npm run build-win             # Build Windows EXE
```

### **Working on C++ Version** (Future)
```bash
cd calcforge-cpp
mkdir build && cd build
cmake ..
make
```

## 📋 Next Steps

1. **Python Version**: Maintenance mode - bug fixes only
2. **Electron Version**: Polish and optimize existing build
3. **C++ Version**: Follow the [conversion plan](./CALCFORGE_CPP_CONVERSION_PLAN.md)

## 🎯 Recommended Usage

- **Development/Testing**: Use `calcforge-python/`
- **Modern UI/Distribution**: Use `calcforge-electron/`
- **Maximum Performance**: Use `calcforge-cpp/` (when complete)

Each implementation maintains feature parity while optimizing for different priorities and use cases.
