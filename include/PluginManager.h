#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QLibrary>
#include <QHash>
#include <QList>
#include <memory>
#include <vector>
#include <unordered_map>
#include "ICalculatorPlugin.h"
#include "ICalculator.h"
#include "CalcForgeResult.h"

/**
 * Plugin Manager for CalcForge calculator plugins
 * 
 * Phase 4.2: Plugin Architecture
 * Handles discovery, loading, registration, and management of calculator plugins.
 * Supports both built-in calculators and external plugin DLLs.
 */
class PluginManager : public QObject
{
    Q_OBJECT

public:
    explicit PluginManager(QObject* parent = nullptr);
    ~PluginManager();

    // Plugin discovery and loading
    void discoverPlugins(const QString& pluginDirectory = QString());
    CalcForgeResult<bool> loadPlugin(const QString& pluginPath);
    CalcForgeResult<bool> unloadPlugin(const QString& pluginName);
    void loadAllPlugins();
    void unloadAllPlugins();

    // Plugin management
    QList<PluginMetadata> getAvailablePlugins() const;
    QList<PluginMetadata> getLoadedPlugins() const;
    PluginMetadata getPluginMetadata(const QString& pluginName) const;
    bool isPluginLoaded(const QString& pluginName) const;
    bool isPluginEnabled(const QString& pluginName) const;
    void setPluginEnabled(const QString& pluginName, bool enabled);

    // Calculator registration and access
    void registerBuiltInCalculators();
    CalcForgeResult<bool> registerCalculator(std::unique_ptr<ICalculatorFactory> factory);
    std::vector<std::unique_ptr<ICalculator>> getCalculators() const;
    std::unique_ptr<ICalculator> getCalculator(const QString& calculatorType) const;
    QStringList getAvailableCalculatorTypes() const;

    // Plugin information
    QString getCalcForgeVersion() const;
    QString getDefaultPluginDirectory() const;
    int getLoadedPluginCount() const;
    int getAvailableCalculatorCount() const;

    // Error handling
    QString getLastError() const;
    QStringList getLoadErrors() const;

signals:
    void pluginLoaded(const QString& pluginName);
    void pluginUnloaded(const QString& pluginName);
    void pluginLoadFailed(const QString& pluginPath, const QString& error);
    void calculatorRegistered(const QString& calculatorType);

private slots:
    void onPluginDirectoryChanged();

private:
    // Plugin loading helpers
    CalcForgeResult<PluginMetadata> loadPluginMetadata(const QString& pluginPath);
    CalcForgeResult<ICalculatorPlugin*> loadPluginFromLibrary(const QString& pluginPath);
    bool validatePlugin(ICalculatorPlugin* plugin) const;
    void registerPluginCalculator(ICalculatorPlugin* plugin);

    // Built-in calculator registration
    void registerUnitConverter();
    void registerTimecodeCalculator();
    void registerAspectRatioCalculator();
    void registerDateCalculator();
    void registerCurrencyConverter();

    // Internal data structures
    struct LoadedPlugin {
        std::unique_ptr<QLibrary> library;
        ICalculatorPlugin* plugin;
        PluginMetadata metadata;
        CreatePluginFunction createFunction;
        DestroyPluginFunction destroyFunction;

        // Make it movable
        LoadedPlugin() = default;
        LoadedPlugin(LoadedPlugin&&) = default;
        LoadedPlugin& operator=(LoadedPlugin&&) = default;

        // Disable copy
        LoadedPlugin(const LoadedPlugin&) = delete;
        LoadedPlugin& operator=(const LoadedPlugin&) = delete;
    };

    std::unordered_map<std::string, LoadedPlugin> m_loadedPlugins;  // Plugin name -> LoadedPlugin
    QHash<QString, PluginMetadata> m_availablePlugins;  // Plugin name -> Metadata
    std::unordered_map<std::string, std::unique_ptr<ICalculatorFactory>> m_calculatorFactories;  // Type -> Factory
    
    QString m_pluginDirectory;
    QString m_lastError;
    QStringList m_loadErrors;
    
    static const QString CALCFORGE_VERSION;
    static const QString DEFAULT_PLUGIN_SUBDIR;
};

#endif // PLUGINMANAGER_H
