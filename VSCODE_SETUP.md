# VS Code C++ Setup for CalcForge

## 🎯 Overview
This guide shows how to set up Visual Studio Code for C++ development with Qt6, perfect for building CalcForge.

## 📋 Prerequisites

### **1. Visual Studio Code Extensions**
✅ You already have the Microsoft C++ extensions installed!

Make sure you have these extensions:
- **C/C++** (ms-vscode.cpptools)
- **C/C++ Extension Pack** (ms-vscode.cpptools-extension-pack)
- **CMake Tools** (ms-vscode.cmake-tools)

### **2. C++ Compiler Options**

#### **Option A: MSVC (Recommended for VS Code)**
- **Download**: [Build Tools for Visual Studio 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
- **Pros**: Best VS Code integration, excellent debugging, optimized for Windows
- **Installation**:
  1. Download Visual Studio Installer
  2. Install "C++ build tools" workload
  3. Includes MSVC compiler, Windows SDK, CMake

#### **Option B: MinGW-w64**
- **Download**: [MSYS2](https://www.msys2.org/) or [WinLibs](https://winlibs.com/)
- **Pros**: Easy Qt setup, cross-platform, package manager
- **Installation**:
  1. Download MSYS2 installer
  2. Install and run: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake`
  3. Add `C:\msys64\mingw64\bin` to PATH

#### **Option C: Clang/LLVM**
- **Download**: [LLVM](https://releases.llvm.org/download.html)
- **Pros**: Modern compiler, fast compilation, great error messages
- **Installation**: Download Windows installer, add to PATH

### **3. CMake**
- **Download**: [CMake](https://cmake.org/download/)
- **Installation**: Download Windows installer, check "Add to PATH"

### **4. Qt6**
- **Download**: [Qt Online Installer](https://www.qt.io/download-qt-installer)
- **Installation**: 
  1. Create Qt account (free)
  2. Install Qt 6.5+ with MinGW compiler
  3. Add Qt bin directory to PATH (e.g., `C:\Qt\6.5.0\mingw_64\bin`)

## 🔧 VS Code Configuration

### **1. Create .vscode/settings.json**
```json
{
    "cmake.configureOnOpen": true,
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.generator": "MinGW Makefiles",
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.compilerPath": "C:/msys64/mingw64/bin/g++.exe"
}
```

### **2. Create .vscode/c_cpp_properties.json**
```json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**",
                "C:/Qt/6.5.0/mingw_64/include/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "compilerPath": "C:/msys64/mingw64/bin/g++.exe",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-x64",
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
```

### **3. Create .vscode/tasks.json**
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "CMake Configure",
            "type": "shell",
            "command": "cmake",
            "args": [
                "-B", "build",
                "-G", "MinGW Makefiles"
            ],
            "group": "build",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        },
        {
            "label": "CMake Build",
            "type": "shell",
            "command": "cmake",
            "args": [
                "--build", "build"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "dependsOn": "CMake Configure",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        }
    ]
}
```

## 🚀 Quick Start

### **1. Install Tools (Recommended: MinGW)**
```bash
# Download and install MSYS2 from https://www.msys2.org/
# Then run in MSYS2 terminal:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-qt6

# Add to Windows PATH:
# C:\msys64\mingw64\bin
```

### **2. Open in VS Code**
```bash
code calcforge-cpp
```

### **3. Build with VS Code**
- **Ctrl+Shift+P** → "CMake: Configure"
- **Ctrl+Shift+P** → "CMake: Build"
- Or use **Ctrl+Shift+B** (Build task)

### **4. Run Application**
```bash
./build/CalcForge.exe
```

## 🎯 Benefits of VS Code Setup

### **✅ Advantages:**
- **Lightweight**: Faster than full Visual Studio
- **Cross-platform**: Same setup works on Linux/macOS
- **Excellent IntelliSense**: Great code completion and error detection
- **Integrated Terminal**: Build and run without leaving editor
- **Git Integration**: Built-in version control
- **Extensions**: Rich ecosystem for C++ development

### **🔧 VS Code Features:**
- **CMake Integration**: Visual CMake configuration and building
- **Debugging**: Full debugging support with breakpoints
- **Code Navigation**: Go to definition, find references
- **Error Highlighting**: Real-time error detection
- **Auto-formatting**: Consistent code style

## 📋 Troubleshooting

### **Common Issues:**
1. **"Qt not found"**: Make sure Qt bin directory is in PATH
2. **"Compiler not found"**: Verify compiler installation and PATH
3. **"CMake not found"**: Install CMake and add to PATH
4. **IntelliSense errors**: Check c_cpp_properties.json paths

### **Verify Installation:**
```bash
# Check compiler
g++ --version

# Check CMake  
cmake --version

# Check Qt
qmake --version
```

This setup gives you a powerful, lightweight C++ development environment perfect for building CalcForge!
