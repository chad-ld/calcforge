# CalcForge C++ Developer Guide & Features v5.0

## Table of Contents
1. [Development Workflow & Guidelines](#development-workflow--guidelines)
2. [Build System & Environment](#build-system--environment)
3. [Architecture Overview](#architecture-overview)
4. [Core Features & Functionality](#core-features--functionality)
5. [Plugin System Development](#plugin-system-development)
6. [Testing Framework](#testing-framework)
7. [Event System](#event-system)
8. [File Management](#file-management)
9. [UI Components](#ui-components)
10. [Future Development Guidelines](#future-development-guidelines)

---

## Development Workflow & Guidelines

### 🔄 Standard Development Process

**1. Pre-Development Checks:**
- Always kill existing CalcForge processes: `taskkill /F /IM CalcForge.exe`
- Use `build-quick.bat` for faster compilation during development
- Check Qt 6.9.1 MSVC 2022 64-bit installation before building

**2. Build Process:**
```bash
# Clean build (recommended for major changes)
.\build-clean.bat

# Quick build (for minor changes)
.\build-quick.bat

# Run application with Qt environment after building to test
.\run-calcforge.bat
```

**3. Error Logging & Debugging:**
- **Debug Logging**: Enabled via `Logger.h` with `LOG_DEBUG()`, `LOG_INFO()`, `LOG_ERROR()`
- **Error Handling**: Use `CalcForgeResult<T>` for consistent error reporting
- **Crash Investigation**: Check debug logs for initialization errors
- **Performance Monitoring**: Use Task Manager during development

**4. User Testing Protocol:**
- **Always ask permission** before git commits/pushes
- **Wait for user confirmation** that features work correctly
- **Keep debugging code** in place until user confirms fixes
- **Test all affected functionality** after changes

**5. Git Workflow:**
- Work on `calcforge-cpp` branch
- User controls all commits and pushes manually
- Provide detailed commit messages with feature descriptions
- Always verify functionality before requesting commits

### 🚨 Critical Development Rules

**Memory Management:**
- Use `std::unique_ptr` for automatic memory management
- Apply RAII principles for resource management
- Prefer move semantics over copying for performance

**Container Usage:**
- Use `std::vector` instead of `QList` for move-only types
- Use `std::unordered_map` instead of `QHash` for complex types
- Handle QString ↔ std::string conversions properly

**Error Handling:**
- Always use `CalcForgeResult<T>` for operations that can fail
- Implement comprehensive try-catch blocks for plugin operations
- Provide meaningful error messages and logging

---

## Build System & Environment

### 📦 Dependencies & Requirements

**Required Software:**
- **Qt 6.9.1** with MSVC 2022 64-bit components
- **Microsoft Visual Studio 2022** Build Tools
- **CMake 3.16+** for build configuration
- **Git** for version control

**Project Structure:**
```
calcforge-cpp/
├── include/           # Header files
├── src/              # Source files
├── examples/         # Plugin examples
├── docs/             # Documentation
├── build/            # Build output (generated)
├── CMakeLists.txt    # Build configuration
├── build-clean.bat   # Clean build script
├── build-quick.bat   # Quick build script
└── run-calcforge.bat # Application launcher
```

**Build Configuration:**
- **Target**: Windows x64 Release builds
- **Compiler**: MSVC 2022 with C++17 standard
- **Qt Modules**: Core, Widgets, Gui
- **Output**: Standalone EXE (1MB+) with minimal dependencies

### 🔧 Build Scripts

**build-clean.bat:**
- Removes existing build directory
- Runs full CMake configuration
- Performs complete rebuild
- Use for major changes or after adding new files

**build-quick.bat:**
- Incremental compilation
- Faster for minor code changes
- Preserves CMake cache
- Recommended for development iterations

**run-calcforge.bat:**
- Sets up Qt environment paths
- Checks for required DLLs
- Launches CalcForge with error capture
- Provides detailed startup diagnostics

---

## Architecture Overview

### 🏗️ Modern C++ Architecture (Post-Refactor)

CalcForge has been completely transformed from a monolithic application to a modern, modular architecture:

**Core Principles:**
- **Dependency Injection**: Manager classes with clear interfaces
- **Event-Driven Architecture**: Decoupled component communication
- **Plugin System**: Extensible calculator framework
- **Separation of Concerns**: UI, business logic, and data clearly separated

**Architecture Layers:**
```
┌─────────────────────────────────────────┐
│              UI Layer                   │
│  MainWindow, WorksheetWidget, Dialogs  │
├─────────────────────────────────────────┤
│            Manager Layer                │
│ FileManager, TabManager, WindowManager │
├─────────────────────────────────────────┤
│           Business Logic                │
│ CalculationService, WorksheetModel     │
├─────────────────────────────────────────┤
│            Event System                 │
│    EventBus, ApplicationEvents         │
├─────────────────────────────────────────┤
│           Plugin System                 │
│   PluginManager, ICalculatorPlugin     │
├─────────────────────────────────────────┤
│            Core Engine                  │
│  CalculationEngine, Calculator Classes │
└─────────────────────────────────────────┘
```

### 📋 Manager Classes (Dependency Injection)

**FileManager:**
- Handles all file operations (load, save, recent files)
- Manages workspace modification state
- Provides save prompts and file validation

**TabManager:**
- Controls tab creation, deletion, and navigation
- Manages tab state and font synchronization
- Handles tab reordering and renaming

**WindowManager:**
- Window positioning, sizing, and state management
- Always-on-top functionality
- Cross-platform window behavior

**CrossSheetNavigator:**
- Cross-sheet reference navigation and validation
- Sheet dependency tracking
- Reference auto-update coordination

---

## Core Features & Functionality

### 🧮 Calculation Engine

**Mathematical Operations:**
- Basic arithmetic with proper order of operations
- Advanced functions: trigonometric, logarithmic, exponential
- Constants: π (pi), e, and custom user-defined constants
- Parentheses grouping and nested expressions

**Specialized Calculators:**
- **Unit Converter**: Length, weight, temperature, volume conversions
- **Timecode Calculator**: Frame-based video editing calculations
- **Aspect Ratio Calculator**: Video/image dimension calculations
- **Date Calculator**: Date arithmetic and business day calculations
- **Currency Converter**: Live exchange rate conversions
- **Percentage Calculator**: Various percentage calculation types

**Advanced Features:**
- **LN Variables**: Reference results from specific lines (`LN1`, `LN2`, etc.)
- **Cross-Sheet References**: `S("SheetName", LN1)` for multi-sheet calculations
- **Auto-Update System**: Automatic LN reference updates when lines are inserted/deleted
- **Comment Lines**: Lines starting with `:::` for documentation
- **Solve Functions**: Linear, quadratic, and transcendental equation solving

### 📊 Expression Evaluation System

**Evaluation Pipeline:**
1. **Plugin Calculators** (if available) - tried first
2. **Specialized Calculators** - timecode, unit conversion, etc.
3. **Mathematical Parser** - standard arithmetic expressions
4. **Error Handling** - comprehensive error reporting

**LN Reference System:**
- Automatic line numbering and value storage
- Cross-sheet reference resolution
- Position-preserving auto-updates during line insertion/deletion
- Circular reference detection and prevention

**Result Formatting:**
- Intelligent number formatting (integers vs. decimals)
- Unit preservation in conversion results
- Currency symbol and formatting
- Scientific notation for large/small numbers

---

## Plugin System Development

### 🔌 Plugin Architecture (Phase 4.2)

**Plugin Interface (`ICalculatorPlugin`):**
```cpp
class ICalculatorPlugin {
public:
    virtual ~ICalculatorPlugin() = default;
    virtual std::unique_ptr<ICalculator> createCalculator() = 0;
    virtual QString getPluginName() const = 0;
    virtual QString getVersion() const = 0;
    virtual QString getAuthor() const = 0;
    virtual QString getDescription() const = 0;
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    virtual bool isCompatible(const QString& calcforgeVersion) const = 0;
};
```

**Calculator Interface (`ICalculator`):**
```cpp
class ICalculator {
public:
    virtual ~ICalculator() = default;
    virtual CalcForgeResult<QString> calculate(const QString& expression) = 0;
    virtual QString getType() const = 0;
    virtual bool canHandle(const QString& expression) const = 0;
    virtual QString getDescription() const = 0;
    virtual int getPriority() const = 0;
    virtual QStringList getExamples() const = 0;
};
```

### 🛠️ Creating a Plugin

**1. Basic Plugin Structure:**
```cpp
// MyPlugin.h
class MyCalculator : public ICalculator {
    CalcForgeResult<QString> calculate(const QString& expression) override;
    QString getType() const override { return "MyCalculator"; }
    bool canHandle(const QString& expression) const override;
    QString getDescription() const override;
    QStringList getExamples() const override;
};

class MyPlugin : public BaseCalculatorPlugin {
public:
    MyPlugin() : BaseCalculatorPlugin("My Plugin", "1.0.0", "Author") {}
    std::unique_ptr<ICalculator> createCalculator() override;
    QString getCalculatorType() const override { return "MyCalculator"; }
    QString getDescription() const override;
};

// Export the plugin
DECLARE_CALCFORGE_PLUGIN(MyPlugin)
```

**2. Plugin Development Workflow:**
- Inherit from `BaseCalculatorPlugin` for easier development
- Implement `ICalculator` interface for calculation logic
- Use `DECLARE_CALCFORGE_PLUGIN` macro for DLL export
- Test with example expressions and edge cases
- Provide comprehensive examples and documentation

**3. Plugin Manager Integration:**
- Plugins are automatically discovered in `plugins/` directory
- Runtime loading and unloading support
- Version compatibility checking
- Error handling and graceful fallback

### 📁 Plugin Directory Structure
```
calcforge-cpp/
├── plugins/              # Plugin DLLs (runtime loading)
├── examples/             # Plugin source examples
│   ├── BasicMathPlugin.h
│   └── BasicMathPlugin.cpp
└── docs/
    └── Phase4.2-PluginArchitecture.md
```

**Current Status:**
- ✅ Complete plugin architecture implemented
- ✅ Example plugin (BasicMathPlugin) available
- 🚧 Plugin system temporarily disabled due to initialization issue
- 🔄 Ready for safe re-enablement and external plugin development

---

## Testing Framework

### 🧪 Easy Testing Architecture

**Test-Friendly Design:**
- **Manager Classes**: Easy to mock and test in isolation
- **Dependency Injection**: Testable components with clear interfaces
- **CalcForgeResult<T>**: Consistent error handling for test validation
- **Event System**: Testable component communication

**Testing Strategies:**

**1. Unit Testing Calculator Logic:**
```cpp
// Example: Testing calculation engine
CalculationEngine engine;
QString result = engine.evaluateExpression("2 + 3 * 4", 1);
ASSERT_EQ(result, "14");

// Test LN references
engine.evaluateExpression("10", 1);  // LN1 = 10
result = engine.evaluateExpression("LN1 * 2", 2);
ASSERT_EQ(result, "20");
```

**2. Integration Testing with Managers:**
```cpp
// Example: Testing file operations
FileManager fileManager(tabWidget, tabManager, settings, eventBus);
auto result = fileManager.saveWorksheet("test.json");
ASSERT_TRUE(result.isValid());
```

**3. Plugin Testing:**
```cpp
// Example: Testing plugin functionality
MyCalculator calc;
auto result = calc.calculate("5!");
ASSERT_TRUE(result.isValid());
ASSERT_EQ(result.value(), "120");
```

**Testing Best Practices:**
- Test both success and failure cases
- Use `CalcForgeResult<T>` for comprehensive error validation
- Mock dependencies using manager interfaces
- Test event system communication
- Validate UI state changes through manager classes

### 🔍 Debugging & Diagnostics

**Logging System:**
```cpp
LOG_DEBUG("Detailed debugging information");
LOG_INFO("General information messages");
LOG_ERROR("Error conditions and failures");
```

**Error Investigation:**
- Check debug logs for initialization errors
- Monitor memory usage during development
- Use CalcForgeResult error messages for troubleshooting
- Enable verbose logging for plugin operations

**Performance Testing:**
- Use Release builds for performance testing
- Monitor startup time and calculation speed
- Test with large worksheets and complex expressions
- Validate memory usage and cleanup

---

## Event System

### 📡 Event-Driven Architecture (Phase 4.1)

**EventBus System:**
- **Centralized Communication**: All components communicate through EventBus
- **Decoupled Architecture**: Components don't directly reference each other
- **Type-Safe Events**: Strongly typed event parameters
- **Comprehensive Logging**: All events logged for debugging

**Event Categories:**

**1. Application Events:**
```cpp
// Tab management events
applicationEvents()->emitTabAdded(index, name);
applicationEvents()->emitTabClosed(index, name);
applicationEvents()->emitTabRenamed(index, oldName, newName);
applicationEvents()->emitTabMoved(from, to);
applicationEvents()->emitCurrentTabChanged(index);

// File management events
applicationEvents()->emitFileLoaded(filePath);
applicationEvents()->emitFileSaved(filePath);
applicationEvents()->emitWorkspaceModified();
```

**2. Worksheet Events:**
```cpp
// Content change events
worksheetEvents()->emitContentChanged(lineNumber, newContent);
worksheetEvents()->emitCalculationCompleted(lineNumber, result);
worksheetEvents()->emitErrorOccurred(lineNumber, errorMessage);

// Navigation events
worksheetEvents()->emitCursorPositionChanged(line, column);
worksheetEvents()->emitSelectionChanged(startLine, endLine);
```

**Event Usage Patterns:**
```cpp
// Connecting to events
connect(eventBus->applicationEvents(), &ApplicationEvents::tabAdded,
        this, &MyClass::onTabAdded);

// Emitting events
if (m_eventBus) {
    m_eventBus->applicationEvents()->emitTabAdded(index, name);
}
```

**Benefits:**
- **Testability**: Easy to mock and verify event communication
- **Maintainability**: Clear component boundaries and responsibilities
- **Extensibility**: New components can easily integrate with existing events
- **Debugging**: Comprehensive event logging for troubleshooting

---

## File Management

### 💾 Comprehensive File System

**File Operations:**
- **Load/Save Worksheets**: JSON-based worksheet persistence
- **Recent Files**: Automatic recent file tracking and management
- **Auto-Save Protection**: Prevents data loss with save prompts
- **File Validation**: Comprehensive file format and content validation

**Workspace Change Detection:**
- **Content Modifications**: Text changes in expression or results
- **Tab Operations**: Create, rename, close, reorder tabs
- **Save Prompts**: Automatic prompts when closing with unsaved changes
- **Modification Tracking**: Precise tracking of all workspace changes

**File Format (JSON):**
```json
{
  "version": "5.0",
  "tabs": [
    {
      "name": "Sheet1",
      "content": "2 + 2\n5 * 3\nLN1 + LN2"
    }
  ],
  "settings": {
    "fontSize": 16,
    "splitterPosition": 400
  }
}
```

**Recent Files Management:**
- **Automatic Tracking**: Files added to recent list on successful operations
- **Smart Filtering**: Only Save As operations add to recent files
- **Persistence**: Recent files saved in application settings
- **Validation**: Invalid or missing files automatically removed

**File Manager API:**
```cpp
// Loading and saving
CalcForgeResult<bool> loadWorksheet(const QString& filePath);
CalcForgeResult<bool> saveWorksheet(const QString& filePath);
CalcForgeResult<bool> saveWorksheetAs(const QString& filePath);

// Recent files
QStringList getRecentFiles() const;
void addToRecentFiles(const QString& filePath);

// Modification tracking
void markAsModified();
bool isModified() const;
bool promptToSave();
```

---

## UI Components

### 🎨 Modern Material Design Interface

**Main Window Architecture:**
- **Minimal UI**: No visible menu/toolbar, clean interface
- **Tab System**: Dynamic width tabs with close buttons
- **Splitter Layout**: Resizable expression/results columns
- **Status Integration**: Real-time calculation feedback

**Tab Management:**
- **Dynamic Sizing**: Tab width based on content with 8px padding
- **Close Buttons**: Small grey 'x' buttons with 8px gap from text
- **Reordering**: Drag-and-drop tab reordering with change detection
- **Renaming**: Double-click tab names for inline editing

**Text Editing Features:**
- **Synchronized Scrolling**: Expression and results scroll together
- **Line Numbers**: Always visible with current line highlighting
- **Syntax Highlighting**: Color-coded expressions (numbers, operators, functions)
- **Font Management**: Synchronized font sizing across all tabs

**Keyboard Shortcuts:**
```
Ctrl+Period     - Increase font size
Ctrl+Comma      - Decrease font size
Ctrl+0          - Reset font size
Ctrl+PageUp     - Previous tab
Ctrl+PageDown   - Next tab
Ctrl+Left/Right - Smart selection (numbers, LN references)
Ctrl+C          - Copy result value (when no text selected)
```

**Visual Design:**
- **Material Design**: Flat, modern interface elements
- **Color Scheme**: Professional color palette with accessibility support
- **Responsive Layout**: Adapts to different window sizes
- **Visual Feedback**: Hover effects and state indicators

**Splitter System:**
- **Shared Position**: All tabs use the same splitter position
- **Visual Indicators**: Thin grey lines with blue hover feedback
- **Resize Handles**: Large grab areas for easy resizing
- **Persistence**: Splitter position saved with worksheets

---

## Future Development Guidelines

### 🚀 Development Roadmap & Best Practices

**Immediate Opportunities:**
1. **Plugin System Re-enablement**: Resolve initialization crash and enable plugin loading
2. **Built-in Calculator Migration**: Convert existing calculators to ICalculator interface
3. **Plugin Marketplace**: Framework for plugin distribution and management
4. **Advanced Testing**: Comprehensive test suite with automated validation

**Architecture Extensions:**
- **Configuration System**: Plugin and application configuration management
- **Dependency Resolution**: Plugin dependency tracking and loading order
- **Hot Reloading**: Development-time plugin reloading without restart
- **Performance Optimization**: Calculation caching and optimization

**Development Standards:**

**Code Quality:**
- Follow RAII principles for all resource management
- Use CalcForgeResult<T> for all operations that can fail
- Implement comprehensive logging for debugging
- Maintain clear separation of concerns

**Testing Requirements:**
- Unit tests for all calculator logic
- Integration tests for manager classes
- Plugin compatibility testing
- Performance regression testing

**Documentation Standards:**
- Update this guide for all new features
- Maintain plugin development examples
- Document all public APIs and interfaces
- Provide migration guides for breaking changes

**Git Workflow:**
- Always work on feature branches
- Comprehensive commit messages with feature descriptions
- User approval required for all commits and pushes
- Maintain backward compatibility

### 📋 Feature Implementation Checklist

**For New Features:**
- [ ] Design follows existing architecture patterns
- [ ] Implements proper error handling with CalcForgeResult<T>
- [ ] Includes comprehensive logging
- [ ] Integrates with event system where appropriate
- [ ] Maintains backward compatibility
- [ ] Includes example usage and documentation
- [ ] Tested with both success and failure cases
- [ ] User approval obtained before committing

**For Plugin Development:**
- [ ] Implements ICalculator interface correctly
- [ ] Provides meaningful examples and descriptions
- [ ] Handles edge cases and invalid input gracefully
- [ ] Includes version compatibility checking
- [ ] Follows plugin export conventions
- [ ] Tested with CalcForge plugin system

**For UI Changes:**
- [ ] Follows Material Design principles
- [ ] Maintains accessibility standards
- [ ] Responsive to different window sizes
- [ ] Integrates with existing keyboard shortcuts
- [ ] Preserves user preferences and settings

---

## Conclusion

CalcForge C++ v5.0 represents a complete architectural transformation from a monolithic application to a modern, extensible platform. The comprehensive refactoring has established:

- **Solid Foundation**: Modern C++ architecture with clear patterns
- **Extensibility**: Plugin system ready for third-party development
- **Maintainability**: Well-separated concerns and testable components
- **Performance**: Optimized build system and efficient algorithms
- **User Experience**: Polished interface with comprehensive functionality

This developer guide provides the foundation for continued development, ensuring consistency, quality, and architectural integrity as CalcForge evolves into the future.

---

*CalcForge C++ Developer Guide v5.0 - Complete architectural documentation for modern calculator platform development.*
