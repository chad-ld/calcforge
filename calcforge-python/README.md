# CalcForge Python Version

## 🐍 Original Python Qt Implementation

This is the original CalcForge application built with Python and PySide6 (Qt for Python).

### **Features**
- ✅ Complete mathematical calculation engine
- ✅ Unit conversion system using Pint library
- ✅ Date/time arithmetic and timecode calculations
- ✅ Cross-sheet references with LN variables
- ✅ Syntax highlighting and auto-completion
- ✅ Multi-tab worksheet support
- ✅ Native Qt desktop interface

### **Requirements**
- Python 3.7+
- PySide6
- Pint (for unit conversions)
- Requests (for currency conversion)

### **Installation**
```bash
pip install PySide6 pint requests
python calcforge.py
```

### **Building Executable**
```bash
pip install pyinstaller
pyinstaller calcforge.spec
```

### **Files**
- `calcforge.py` - Main application file
- `calcforge.spec` - PyInstaller build specification
- `setup.py` - Python package setup
- `worksheets.json` - Default worksheet data
- `build/` - PyInstaller build artifacts
- `dist/` - Built executable output

### **Status**
✅ **Stable** - Fully functional original version
🔄 **Maintenance** - Bug fixes and minor improvements only

This version serves as the reference implementation for feature parity in other versions.
