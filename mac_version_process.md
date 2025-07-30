# CalcForge Mac Version Development Plan

**Created:** July 30, 2025  
**Status:** Planning Phase  
**Estimated Effort:** 5-9 hours total development time

## 🍎 **Current Mac Compatibility Status**

### ✅ **What's Already Mac-Ready**

**1. Build System (CMakeLists.txt)**
- ✅ **Cross-platform CMake** already configured
- ✅ **macOS bundle support** (`MACOSX_BUNDLE TRUE`)
- ✅ **Platform-specific optimizations** (GCC/Clang vs MSVC)
- ✅ **Info.plist reference** for Mac app metadata

**2. Code Compatibility**
- ✅ **Qt6 framework** (fully cross-platform)
- ✅ **C++17 standard** (Mac compatible)
- ✅ **Platform-specific code** already handled with `#ifdef Q_OS_WIN`
- ✅ **Cross-platform fallbacks** implemented (e.g., always-on-top functionality)

**3. GitHub Actions**
- ✅ **macOS build pipeline** already exists (though for old Python version)
- ✅ **DMG creation** and code signing configured
- ✅ **Multi-platform release** system in place

### 🔧 **What Needs to Be Done**

## **1. Missing Mac-Specific Files**

### **Info.plist** (Referenced but missing)
Create `Info.plist` in root directory:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>CalcForge</string>
    <key>CFBundleDisplayName</key>
    <string>CalcForge</string>
    <key>CFBundleIdentifier</key>
    <string>org.calcforge.CalcForge</string>
    <key>CFBundleVersion</key>
    <string>5.0.0</string>
    <key>CFBundleShortVersionString</key>
    <string>5.0</string>
    <key>CFBundleExecutable</key>
    <string>CalcForge</string>
    <key>CFBundleIconFile</key>
    <string>calcforge.icns</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.15</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSRequiresAquaSystemAppearance</key>
    <false/>
</dict>
</plist>
```

### **Mac Icon (calcforge.icns)**
- Convert existing `calcforge.ico` to `calcforge.icns` format
- Mac requires different icon sizes (16x16 to 1024x1024)
- Use `iconutil` or online converter

## **2. Platform-Specific Code Updates**

### **Window Management** (src/MainWindow.cpp)
```cpp
#ifdef Q_OS_MACOS
    // Mac-specific window setup
    setWindowFlags(Qt::Window); // Don't use frameless on Mac
    // Mac has native title bar handling
#elif defined(Q_OS_WIN)
    // Current Windows frameless implementation
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
#endif
```

### **Keyboard Shortcuts**
- Replace Ctrl with Cmd on Mac
- Update shortcut handling for Mac conventions

### **Settings Storage**
- Windows: Registry (`QSettings("CalcForge", "CalcForge")`)
- Mac: `~/Library/Preferences/org.calcforge.CalcForge.plist`

## **3. Build System Updates**

### **CMakeLists.txt additions**
```cmake
# Mac-specific icon handling
if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE calcforge.icns)
    set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/calcforge.icns)
    set_source_files_properties(${ICON_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(CalcForge PRIVATE ${ICON_FILE})
endif()
```

## **4. Deployment System**

### **Mac Deployment Script** (deploy-calcforge-mac.sh)
```bash
#!/bin/bash
echo "Building CalcForge for macOS..."

# Create build directory
mkdir -p build-mac
cd build-mac

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build the app
cmake --build . --config Release

# Deploy Qt libraries
macdeployqt CalcForge.app -dmg

# Code sign (for distribution)
codesign --force --deep --sign - CalcForge.app

echo "Mac deployment complete!"
```

## **5. GitHub Actions Update**

Update `.github/workflows/build-executables.yml` for C++ instead of Python:
```yaml
- name: Install Qt (macOS)
  if: matrix.platform == 'macos'
  run: brew install qt6

- name: Build CalcForge (macOS)
  if: matrix.platform == 'macos'
  run: |
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --config Release
    
- name: Deploy Qt libraries (macOS)
  if: matrix.platform == 'macos'
  run: macdeployqt build/CalcForge.app -dmg
```

---

## 📋 **Implementation Plan**

### **Phase 1: Basic Mac Compatibility** (2-3 hours)
1. ✅ Create `Info.plist` file
2. ✅ Convert icon to `.icns` format  
3. ✅ Update CMakeLists.txt for Mac bundle
4. ✅ Add platform-specific window handling
5. ✅ Test local Mac build

### **Phase 2: Mac-Specific Features** (1-2 hours)
1. ✅ Mac-style menu bar integration
2. ✅ Native Mac shortcuts (Cmd instead of Ctrl)
3. ✅ Mac-specific file dialogs
4. ✅ Retina display support

### **Phase 3: Deployment & Distribution** (1-2 hours)
1. ✅ Create Mac deployment script
2. ✅ Update GitHub Actions for C++ builds
3. ✅ Code signing setup
4. ✅ DMG creation and testing

### **Phase 4: Testing & Polish** (1-2 hours)
1. ✅ Test on different macOS versions
2. ✅ Verify all features work on Mac
3. ✅ Performance optimization for Mac
4. ✅ Documentation updates

---

## 💻 **Requirements for Mac Development**

### **Development Environment**
- **macOS machine** (or VM) for testing
- **Xcode Command Line Tools** (`xcode-select --install`)
- **Qt 6.9.1 for macOS** (`brew install qt6`)
- **CMake** (`brew install cmake`)

### **Optional Tools**
- **Qt Creator** for Mac-specific debugging
- **Instruments** for performance profiling
- **App Store Connect** (for future App Store distribution)

---

## 🚀 **Benefits of Mac Version**

### **Market Expansion**
- **Larger user base** (Mac users often prefer native apps)
- **Professional appearance** (Mac users expect polished software)
- **Cross-platform credibility** (shows serious development)

### **Technical Benefits**
- **Code quality improvement** (cross-platform testing catches bugs)
- **Architecture validation** (proves Qt abstraction works)
- **Future-proofing** (easier to add Linux support later)

### **Distribution Options**
- **Direct download** (like current Windows version)
- **Homebrew package** (for developer users)
- **Mac App Store** (future possibility with code signing)

---

## 📝 **Next Steps**

1. **Decide on development approach** (local Mac vs GitHub Actions testing)
2. **Set up Mac development environment** (if developing locally)
3. **Begin Phase 1 implementation** (basic compatibility files)
4. **Test and iterate** through each phase
5. **Create Mac deployment pipeline** for releases

---

**This plan provides a clear roadmap for creating a native Mac version of CalcForge while leveraging the existing cross-platform foundation.**
