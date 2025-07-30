# Phase 4.2: Plugin Architecture Implementation

## Overview

Phase 4.2 implements a comprehensive plugin architecture for CalcForge, enabling runtime loading and registration of calculator implementations. This allows for extensibility and modularity, supporting third-party calculator plugins.

## Architecture Components

### 1. Core Interfaces

#### ICalculatorPlugin
- **File**: `include/ICalculatorPlugin.h`
- **Purpose**: Main plugin interface that all calculator plugins must implement
- **Key Methods**:
  - `createCalculator()` - Creates calculator instances
  - `getPluginName()`, `getVersion()`, `getAuthor()` - Plugin metadata
  - `initialize()`, `cleanup()` - Plugin lifecycle management
  - `isCompatible()` - Version compatibility checking

#### BaseCalculatorPlugin
- **Purpose**: Base class providing common plugin functionality
- **Benefits**: Simplifies plugin development by providing default implementations

### 2. Plugin Manager

#### PluginManager Class
- **File**: `include/PluginManager.h`, `src/PluginManager.cpp`
- **Purpose**: Central management of all plugins and calculator registration
- **Key Features**:
  - Plugin discovery and loading from DLL files
  - Runtime calculator registration and factory management
  - Plugin metadata management and compatibility checking
  - Built-in calculator registration (when they implement ICalculator)

#### Key Methods:
- `discoverPlugins()` - Scans plugin directory for available plugins
- `loadPlugin()` / `unloadPlugin()` - Dynamic plugin loading/unloading
- `registerCalculator()` - Registers calculator factories
- `getCalculators()` - Returns all available calculator instances

### 3. Integration with Calculation Engine

#### Enhanced CalculationEngine
- **Plugin-Aware Constructor**: `CalculationEngine(PluginManager* pluginManager)`
- **Plugin-First Evaluation**: Tries plugin calculators before legacy calculators
- **Seamless Fallback**: Falls back to existing calculator implementations

#### Integration Points:
- **MainWindow**: Initializes PluginManager during startup
- **TabManager**: Creates plugin-aware WorksheetWidgets
- **WorksheetWidget**: Uses plugin-aware CalculationEngine

## Plugin Development

### Creating a Plugin

1. **Implement ICalculatorPlugin Interface**:
```cpp
class MyCalculatorPlugin : public BaseCalculatorPlugin
{
public:
    MyCalculatorPlugin() : BaseCalculatorPlugin("My Plugin", "1.0.0", "Author") {}
    
    std::unique_ptr<ICalculator> createCalculator() override {
        return std::make_unique<MyCalculator>();
    }
    
    QString getCalculatorType() const override { return "MyCalculator"; }
    QString getDescription() const override { return "My custom calculator"; }
};
```

2. **Export Plugin Functions**:
```cpp
DECLARE_CALCFORGE_PLUGIN(MyCalculatorPlugin)
```

3. **Implement Calculator Logic**:
```cpp
class MyCalculator : public ICalculator
{
    CalcForgeResult<QString> calculate(const QString& expression) override;
    QString getType() const override { return "MyCalculator"; }
    bool canHandle(const QString& expression) const override;
    // ... other required methods
};
```

### Example Plugin

See `examples/BasicMathPlugin.h` and `examples/BasicMathPlugin.cpp` for a complete example that implements:
- Factorial calculations (e.g., "5!")
- Power operations (e.g., "2^8")
- Square root functions (e.g., "sqrt(16)")

## Technical Implementation Details

### Memory Management
- Uses `std::unique_ptr` for automatic memory management
- Move semantics for efficient plugin and calculator transfer
- RAII principles for plugin lifecycle management

### Container Choices
- `std::unordered_map` instead of `QHash` for move-only types
- `std::vector` instead of `QList` for calculator collections
- Proper handling of non-copyable types

### Error Handling
- `CalcForgeResult<T>` for consistent error reporting
- Comprehensive plugin validation and compatibility checking
- Graceful fallback to legacy calculators

## Current Status

### ✅ Implemented
- Complete plugin interface and base classes
- PluginManager with discovery, loading, and registration
- Integration with CalculationEngine and UI components
- Example plugin demonstrating the architecture
- Comprehensive error handling and logging

### 🚧 In Progress
- Built-in calculator conversion to ICalculator interface
- Plugin directory creation and management
- Runtime plugin loading from external DLLs

### 📋 Future Enhancements
- Plugin configuration and settings management
- Plugin dependency resolution
- Hot-reloading of plugins during development
- Plugin marketplace and distribution system

## Benefits

1. **Extensibility**: Third-party developers can create custom calculators
2. **Modularity**: Calculators can be developed and tested independently
3. **Maintainability**: Clear separation of concerns and interfaces
4. **Performance**: Plugin-first evaluation with efficient fallback
5. **Future-Proof**: Architecture supports advanced plugin features

## Usage

The plugin system is automatically initialized when CalcForge starts:

1. **PluginManager** is created and initialized in MainWindow
2. **Built-in calculators** are registered (when available)
3. **Plugin discovery** scans the plugins directory
4. **Calculator instances** are created on-demand during expression evaluation

Users can add new plugins by placing DLL files in the `plugins/` directory next to the CalcForge executable.

## Migration Path

The plugin architecture is designed to be backward compatible:
- Existing calculator implementations continue to work
- Legacy calculator calls are preserved as fallback
- Gradual migration of built-in calculators to plugin interface
- No breaking changes to existing functionality
