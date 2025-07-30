#ifndef ICALCULATORPLUGIN_H
#define ICALCULATORPLUGIN_H

#include <QString>
#include <QStringList>
#include <memory>
#include "ICalculator.h"

/**
 * Plugin interface for CalcForge calculator plugins
 * Enables runtime loading and registration of calculator implementations
 * 
 * Phase 4.2: Plugin Architecture
 * This interface allows third-party developers to create calculator plugins
 * that can be loaded at runtime without recompiling CalcForge.
 */
class ICalculatorPlugin
{
public:
    virtual ~ICalculatorPlugin() = default;
    
    /**
     * Create a new instance of the calculator provided by this plugin
     * @return Unique pointer to the calculator instance
     */
    virtual std::unique_ptr<ICalculator> createCalculator() = 0;
    
    /**
     * Get the name of this plugin
     * @return Human-readable plugin name (e.g., "Advanced Math Plugin")
     */
    virtual QString getPluginName() const = 0;
    
    /**
     * Get the version of this plugin
     * @return Version string (e.g., "1.0.0")
     */
    virtual QString getVersion() const = 0;
    
    /**
     * Get the author/vendor of this plugin
     * @return Author name or organization
     */
    virtual QString getAuthor() const = 0;
    
    /**
     * Get a description of what this plugin provides
     * @return Description string for UI/help purposes
     */
    virtual QString getDescription() const = 0;
    
    /**
     * Get the calculator type this plugin provides
     * @return Calculator type identifier (must match ICalculator::getType())
     */
    virtual QString getCalculatorType() const = 0;
    
    /**
     * Get the minimum CalcForge version required for this plugin
     * @return Minimum version string (e.g., "4.0.0")
     */
    virtual QString getMinimumCalcForgeVersion() const = 0;
    
    /**
     * Initialize the plugin (called once when plugin is loaded)
     * @return True if initialization successful, false otherwise
     */
    virtual bool initialize() = 0;
    
    /**
     * Cleanup the plugin (called when plugin is unloaded)
     */
    virtual void cleanup() = 0;
    
    /**
     * Check if this plugin is compatible with the current CalcForge version
     * @param calcforgeVersion Current CalcForge version
     * @return True if compatible, false otherwise
     */
    virtual bool isCompatible(const QString& calcforgeVersion) const = 0;
};

/**
 * Plugin metadata structure for plugin discovery and management
 */
struct PluginMetadata
{
    QString name;
    QString version;
    QString author;
    QString description;
    QString calculatorType;
    QString minimumCalcForgeVersion;
    QString filePath;
    bool isLoaded;
    bool isEnabled;
    
    PluginMetadata() : isLoaded(false), isEnabled(true) {}
};

/**
 * Plugin loading result for error handling
 */
enum class PluginLoadResult
{
    Success,
    FileNotFound,
    InvalidPlugin,
    IncompatibleVersion,
    InitializationFailed,
    AlreadyLoaded,
    DependencyMissing
};

/**
 * Plugin factory function signature
 * Each plugin DLL must export a function with this signature named "createPlugin"
 */
typedef ICalculatorPlugin* (*CreatePluginFunction)();

/**
 * Plugin cleanup function signature
 * Each plugin DLL must export a function with this signature named "destroyPlugin"
 */
typedef void (*DestroyPluginFunction)(ICalculatorPlugin*);

/**
 * Convenience macros for plugin development
 */
#define CALCFORGE_PLUGIN_EXPORT extern "C" __declspec(dllexport)

#define DECLARE_CALCFORGE_PLUGIN(PluginClass) \
    CALCFORGE_PLUGIN_EXPORT ICalculatorPlugin* createPlugin() { \
        return new PluginClass(); \
    } \
    CALCFORGE_PLUGIN_EXPORT void destroyPlugin(ICalculatorPlugin* plugin) { \
        delete plugin; \
    }

/**
 * Base plugin class that provides common functionality
 * Plugin developers can inherit from this instead of implementing ICalculatorPlugin directly
 */
class BaseCalculatorPlugin : public ICalculatorPlugin
{
public:
    BaseCalculatorPlugin(const QString& name, const QString& version, const QString& author)
        : m_name(name), m_version(version), m_author(author) {}
    
    QString getPluginName() const override { return m_name; }
    QString getVersion() const override { return m_version; }
    QString getAuthor() const override { return m_author; }
    QString getMinimumCalcForgeVersion() const override { return "5.0.0"; }
    
    bool initialize() override { return true; }
    void cleanup() override {}
    
    bool isCompatible(const QString& calcforgeVersion) const override {
        // Simple version comparison - can be enhanced
        return calcforgeVersion >= getMinimumCalcForgeVersion();
    }

protected:
    QString m_name;
    QString m_version;
    QString m_author;
};

#endif // ICALCULATORPLUGIN_H
