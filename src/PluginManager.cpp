#include "PluginManager.h"
#include "UnitConverter.h"
#include "TimecodeCalculator.h"
#include "AspectRatioCalculator.h"
#include "DateCalculator.h"
#include "CurrencyConverter.h"
#include "Logger.h"
#include <QApplication>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDirIterator>

const QString PluginManager::CALCFORGE_VERSION = "5.0.0";
const QString PluginManager::DEFAULT_PLUGIN_SUBDIR = "plugins";

PluginManager::PluginManager(QObject* parent)
    : QObject(parent)
{
    try {
        // Set default plugin directory
        m_pluginDirectory = getDefaultPluginDirectory();

        LOG_DEBUG("PluginManager: Initialized with plugin directory: " + m_pluginDirectory);
    } catch (const std::exception& e) {
        LOG_DEBUG(QString("PluginManager: Initialization error: %1").arg(e.what()));
        m_pluginDirectory = QString(); // Set to empty on error
    } catch (...) {
        LOG_DEBUG("PluginManager: Initialization error: unknown exception");
        m_pluginDirectory = QString(); // Set to empty on error
    }
}

PluginManager::~PluginManager()
{
    unloadAllPlugins();
}

void PluginManager::discoverPlugins(const QString& pluginDirectory)
{
    try {
        QString searchDir = pluginDirectory.isEmpty() ? m_pluginDirectory : pluginDirectory;

        LOG_DEBUG("PluginManager: Discovering plugins in directory: " + searchDir);

        if (searchDir.isEmpty()) {
            LOG_DEBUG("PluginManager: Plugin directory is empty, skipping discovery");
            return;
        }

        QDir dir(searchDir);
        if (!dir.exists()) {
            LOG_DEBUG("PluginManager: Plugin directory does not exist: " + searchDir);
            return;
        }
    
    // Clear previous discovery results
    m_availablePlugins.clear();
    m_loadErrors.clear();
    
    // Search for plugin files (*.dll on Windows, *.so on Linux, *.dylib on macOS)
    QStringList filters;
#ifdef Q_OS_WIN
    filters << "*.dll";
#elif defined(Q_OS_LINUX)
    filters << "*.so";
#elif defined(Q_OS_MACOS)
    filters << "*.dylib";
#endif
    
    QDirIterator iterator(searchDir, filters, QDir::Files, QDirIterator::Subdirectories);
    
    while (iterator.hasNext()) {
        QString pluginPath = iterator.next();
        
        auto metadataResult = loadPluginMetadata(pluginPath);
        if (metadataResult.isValid()) {
            PluginMetadata metadata = metadataResult.value();
            m_availablePlugins[metadata.name] = metadata;
            LOG_DEBUG(QString("PluginManager: Discovered plugin '%1' v%2 at %3")
                     .arg(metadata.name).arg(metadata.version).arg(pluginPath));
        } else {
            QString error = QString("Failed to load metadata for %1: %2")
                           .arg(pluginPath).arg(metadataResult.errorMessage());
            m_loadErrors.append(error);
            LOG_DEBUG("PluginManager: " + error);
        }
    }
    
    LOG_DEBUG(QString("PluginManager: Discovery complete. Found %1 plugins, %2 errors")
             .arg(m_availablePlugins.size()).arg(m_loadErrors.size()));

    } catch (const std::exception& e) {
        LOG_DEBUG(QString("PluginManager: Plugin discovery failed: %1").arg(e.what()));
        m_loadErrors.append(QString("Plugin discovery error: %1").arg(e.what()));
    } catch (...) {
        LOG_DEBUG("PluginManager: Plugin discovery failed with unknown error");
        m_loadErrors.append("Plugin discovery error: unknown exception");
    }
}

CalcForgeResult<bool> PluginManager::loadPlugin(const QString& pluginPath)
{
    QFileInfo fileInfo(pluginPath);
    QString pluginName = fileInfo.baseName();
    std::string pluginNameStd = pluginName.toStdString();

    // Check if already loaded
    if (m_loadedPlugins.find(pluginNameStd) != m_loadedPlugins.end()) {
        return CalcForgeResult<bool>::error("Plugin already loaded: " + pluginName);
    }
    
    LOG_DEBUG("PluginManager: Loading plugin from: " + pluginPath);
    
    // Load plugin from library
    auto pluginResult = loadPluginFromLibrary(pluginPath);
    if (!pluginResult.isValid()) {
        m_lastError = pluginResult.errorMessage();
        emit pluginLoadFailed(pluginPath, m_lastError);
        return CalcForgeResult<bool>::error(m_lastError);
    }

    ICalculatorPlugin* plugin = pluginResult.value();
    
    // Validate plugin
    if (!validatePlugin(plugin)) {
        m_lastError = "Plugin validation failed: " + pluginName;
        delete plugin;
        emit pluginLoadFailed(pluginPath, m_lastError);
        return CalcForgeResult<bool>::error(m_lastError);
    }
    
    // Initialize plugin
    if (!plugin->initialize()) {
        m_lastError = "Plugin initialization failed: " + pluginName;
        delete plugin;
        emit pluginLoadFailed(pluginPath, m_lastError);
        return CalcForgeResult<bool>::error(m_lastError);
    }
    
    // Create plugin metadata
    PluginMetadata metadata;
    metadata.name = plugin->getPluginName();
    metadata.version = plugin->getVersion();
    metadata.author = plugin->getAuthor();
    metadata.description = plugin->getDescription();
    metadata.calculatorType = plugin->getCalculatorType();
    metadata.minimumCalcForgeVersion = plugin->getMinimumCalcForgeVersion();
    metadata.filePath = pluginPath;
    metadata.isLoaded = true;
    metadata.isEnabled = true;
    
    // Store loaded plugin
    LoadedPlugin loadedPlugin;
    loadedPlugin.plugin = plugin;
    loadedPlugin.metadata = metadata;
    // Note: library and function pointers would be set in loadPluginFromLibrary
    
    m_loadedPlugins[pluginNameStd] = std::move(loadedPlugin);
    
    // Register the calculator
    registerPluginCalculator(plugin);
    
    emit pluginLoaded(pluginName);
    LOG_DEBUG(QString("PluginManager: Successfully loaded plugin '%1' v%2")
             .arg(metadata.name).arg(metadata.version));
    
    return CalcForgeResult<bool>::success(true);
}

CalcForgeResult<bool> PluginManager::unloadPlugin(const QString& pluginName)
{
    std::string pluginNameStd = pluginName.toStdString();
    auto it = m_loadedPlugins.find(pluginNameStd);
    if (it == m_loadedPlugins.end()) {
        return CalcForgeResult<bool>::error("Plugin not loaded: " + pluginName);
    }

    LOG_DEBUG("PluginManager: Unloading plugin: " + pluginName);

    LoadedPlugin& loadedPlugin = it->second;
    
    // Cleanup plugin
    if (loadedPlugin.plugin) {
        loadedPlugin.plugin->cleanup();
        
        // Remove calculator factory
        QString calculatorType = loadedPlugin.plugin->getCalculatorType();
        m_calculatorFactories.erase(calculatorType.toStdString());
        
        // Destroy plugin using plugin's destroy function if available
        if (loadedPlugin.destroyFunction) {
            loadedPlugin.destroyFunction(loadedPlugin.plugin);
        } else {
            delete loadedPlugin.plugin;
        }
    }
    
    // Unload library
    if (loadedPlugin.library) {
        loadedPlugin.library->unload();
    }
    
    m_loadedPlugins.erase(pluginNameStd);
    
    emit pluginUnloaded(pluginName);
    LOG_DEBUG("PluginManager: Successfully unloaded plugin: " + pluginName);
    
    return CalcForgeResult<bool>::success(true);
}

void PluginManager::loadAllPlugins()
{
    LOG_DEBUG("PluginManager: Loading all available plugins");
    
    for (const auto& metadata : m_availablePlugins) {
        if (!metadata.isLoaded && metadata.isEnabled) {
            loadPlugin(metadata.filePath);
        }
    }
}

void PluginManager::unloadAllPlugins()
{
    LOG_DEBUG("PluginManager: Unloading all plugins");

    std::vector<QString> pluginNames;
    for (const auto& pair : m_loadedPlugins) {
        pluginNames.push_back(QString::fromStdString(pair.first));
    }

    for (const QString& pluginName : pluginNames) {
        unloadPlugin(pluginName);
    }
}

void PluginManager::registerBuiltInCalculators()
{
    LOG_DEBUG("PluginManager: Registering built-in calculators");

    // TODO: Phase 4.2 - Convert existing calculators to implement ICalculator interface
    // For now, we'll skip built-in calculator registration until they implement ICalculator
    // registerUnitConverter();
    // registerTimecodeCalculator();
    // registerAspectRatioCalculator();
    // registerDateCalculator();
    // registerCurrencyConverter();

    LOG_DEBUG(QString("PluginManager: Built-in calculator registration skipped (need ICalculator implementation)"));
}

CalcForgeResult<bool> PluginManager::registerCalculator(std::unique_ptr<ICalculatorFactory> factory)
{
    if (!factory) {
        return CalcForgeResult<bool>::error("Invalid factory provided");
    }

    QString calculatorType = factory->getCalculatorType();
    std::string calculatorTypeStd = calculatorType.toStdString();

    if (m_calculatorFactories.find(calculatorTypeStd) != m_calculatorFactories.end()) {
        return CalcForgeResult<bool>::error("Calculator type already registered: " + calculatorType);
    }

    m_calculatorFactories[calculatorTypeStd] = std::move(factory);

    emit calculatorRegistered(calculatorType);
    LOG_DEBUG("PluginManager: Registered calculator type: " + calculatorType);

    return CalcForgeResult<bool>::success(true);
}

std::vector<std::unique_ptr<ICalculator>> PluginManager::getCalculators() const
{
    std::vector<std::unique_ptr<ICalculator>> calculators;

    for (const auto& pair : m_calculatorFactories) {
        calculators.push_back(pair.second->createCalculator());
    }

    return calculators;
}

std::unique_ptr<ICalculator> PluginManager::getCalculator(const QString& calculatorType) const
{
    std::string calculatorTypeStd = calculatorType.toStdString();
    auto it = m_calculatorFactories.find(calculatorTypeStd);
    if (it != m_calculatorFactories.end()) {
        return it->second->createCalculator();
    }

    return nullptr;
}

QStringList PluginManager::getAvailableCalculatorTypes() const
{
    QStringList types;
    for (const auto& pair : m_calculatorFactories) {
        types.append(QString::fromStdString(pair.first));
    }
    return types;
}

QString PluginManager::getCalcForgeVersion() const
{
    return CALCFORGE_VERSION;
}

QString PluginManager::getDefaultPluginDirectory() const
{
    QString appDir = QApplication::applicationDirPath();
    return QDir(appDir).absoluteFilePath(DEFAULT_PLUGIN_SUBDIR);
}

QString PluginManager::getLastError() const
{
    return m_lastError;
}

QStringList PluginManager::getLoadErrors() const
{
    return m_loadErrors;
}

QList<PluginMetadata> PluginManager::getAvailablePlugins() const
{
    return m_availablePlugins.values();
}

QList<PluginMetadata> PluginManager::getLoadedPlugins() const
{
    QList<PluginMetadata> loadedPlugins;
    for (const auto& pair : m_loadedPlugins) {
        loadedPlugins.append(pair.second.metadata);
    }
    return loadedPlugins;
}

PluginMetadata PluginManager::getPluginMetadata(const QString& pluginName) const
{
    std::string pluginNameStd = pluginName.toStdString();
    auto loadedIt = m_loadedPlugins.find(pluginNameStd);
    if (loadedIt != m_loadedPlugins.end()) {
        return loadedIt->second.metadata;
    }
    if (m_availablePlugins.contains(pluginName)) {
        return m_availablePlugins[pluginName];
    }
    return PluginMetadata(); // Return empty metadata if not found
}

bool PluginManager::isPluginLoaded(const QString& pluginName) const
{
    std::string pluginNameStd = pluginName.toStdString();
    return m_loadedPlugins.find(pluginNameStd) != m_loadedPlugins.end();
}

bool PluginManager::isPluginEnabled(const QString& pluginName) const
{
    if (m_availablePlugins.contains(pluginName)) {
        return m_availablePlugins[pluginName].isEnabled;
    }
    return false;
}

void PluginManager::setPluginEnabled(const QString& pluginName, bool enabled)
{
    if (m_availablePlugins.contains(pluginName)) {
        m_availablePlugins[pluginName].isEnabled = enabled;
        LOG_DEBUG(QString("PluginManager: Plugin '%1' %2").arg(pluginName).arg(enabled ? "enabled" : "disabled"));
    }
}

int PluginManager::getLoadedPluginCount() const
{
    return m_loadedPlugins.size();
}

int PluginManager::getAvailableCalculatorCount() const
{
    return m_calculatorFactories.size();
}

// Private helper methods
CalcForgeResult<PluginMetadata> PluginManager::loadPluginMetadata(const QString& pluginPath)
{
    // For now, we'll load the plugin temporarily to get metadata
    // In a production system, you might want to store metadata in separate files
    auto pluginResult = loadPluginFromLibrary(pluginPath);
    if (!pluginResult.isValid()) {
        return CalcForgeResult<PluginMetadata>::error(pluginResult.errorMessage());
    }

    ICalculatorPlugin* plugin = pluginResult.value();

    PluginMetadata metadata;
    metadata.name = plugin->getPluginName();
    metadata.version = plugin->getVersion();
    metadata.author = plugin->getAuthor();
    metadata.description = plugin->getDescription();
    metadata.calculatorType = plugin->getCalculatorType();
    metadata.minimumCalcForgeVersion = plugin->getMinimumCalcForgeVersion();
    metadata.filePath = pluginPath;
    metadata.isLoaded = false;
    metadata.isEnabled = true;

    // Clean up temporary plugin instance
    delete plugin;

    return CalcForgeResult<PluginMetadata>::success(metadata);
}

CalcForgeResult<ICalculatorPlugin*> PluginManager::loadPluginFromLibrary(const QString& pluginPath)
{
    auto library = std::make_unique<QLibrary>(pluginPath);

    if (!library->load()) {
        return CalcForgeResult<ICalculatorPlugin*>::error("Failed to load library: " + library->errorString());
    }

    // Get the create function
    CreatePluginFunction createFunction = (CreatePluginFunction)library->resolve("createPlugin");
    if (!createFunction) {
        return CalcForgeResult<ICalculatorPlugin*>::error("Plugin does not export createPlugin function");
    }

    // Create plugin instance
    ICalculatorPlugin* plugin = createFunction();
    if (!plugin) {
        return CalcForgeResult<ICalculatorPlugin*>::error("Failed to create plugin instance");
    }

    return CalcForgeResult<ICalculatorPlugin*>::success(plugin);
}

bool PluginManager::validatePlugin(ICalculatorPlugin* plugin) const
{
    if (!plugin) {
        return false;
    }

    // Check version compatibility
    if (!plugin->isCompatible(CALCFORGE_VERSION)) {
        LOG_DEBUG(QString("PluginManager: Plugin '%1' is not compatible with CalcForge %2")
                 .arg(plugin->getPluginName()).arg(CALCFORGE_VERSION));
        return false;
    }

    // Validate required methods return non-empty values
    if (plugin->getPluginName().isEmpty() ||
        plugin->getVersion().isEmpty() ||
        plugin->getCalculatorType().isEmpty()) {
        LOG_DEBUG("PluginManager: Plugin has empty required fields");
        return false;
    }

    return true;
}

void PluginManager::registerPluginCalculator(ICalculatorPlugin* plugin)
{
    // Create a factory that wraps the plugin's createCalculator method
    class PluginCalculatorFactory : public ICalculatorFactory {
    public:
        PluginCalculatorFactory(ICalculatorPlugin* plugin) : m_plugin(plugin) {}

        std::unique_ptr<ICalculator> createCalculator() override {
            return m_plugin->createCalculator();
        }

        QString getCalculatorType() const override {
            return m_plugin->getCalculatorType();
        }

    private:
        ICalculatorPlugin* m_plugin;
    };

    auto factory = std::make_unique<PluginCalculatorFactory>(plugin);
    registerCalculator(std::move(factory));
}

// Built-in calculator registration methods
// TODO: Phase 4.2 - Implement these once existing calculators implement ICalculator interface
void PluginManager::registerUnitConverter()
{
    // auto factory = std::make_unique<CalculatorFactory<UnitConverter>>();
    // registerCalculator(std::move(factory));
    LOG_DEBUG("PluginManager: UnitConverter registration skipped (needs ICalculator implementation)");
}

void PluginManager::registerTimecodeCalculator()
{
    // auto factory = std::make_unique<CalculatorFactory<TimecodeCalculator>>();
    // registerCalculator(std::move(factory));
    LOG_DEBUG("PluginManager: TimecodeCalculator registration skipped (needs ICalculator implementation)");
}

void PluginManager::registerAspectRatioCalculator()
{
    // auto factory = std::make_unique<CalculatorFactory<AspectRatioCalculator>>();
    // registerCalculator(std::move(factory));
    LOG_DEBUG("PluginManager: AspectRatioCalculator registration skipped (needs ICalculator implementation)");
}

void PluginManager::registerDateCalculator()
{
    // auto factory = std::make_unique<CalculatorFactory<DateCalculator>>();
    // registerCalculator(std::move(factory));
    LOG_DEBUG("PluginManager: DateCalculator registration skipped (needs ICalculator implementation)");
}

void PluginManager::registerCurrencyConverter()
{
    // auto factory = std::make_unique<CalculatorFactory<CurrencyConverter>>();
    // registerCalculator(std::move(factory));
    LOG_DEBUG("PluginManager: CurrencyConverter registration skipped (needs ICalculator implementation)");
}

void PluginManager::onPluginDirectoryChanged()
{
    // Re-discover plugins when directory changes
    discoverPlugins();
}
