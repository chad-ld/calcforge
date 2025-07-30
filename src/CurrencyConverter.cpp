#include "CurrencyConverter.h"
#include "Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QRegularExpressionMatch>
#include <QUrl>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QEventLoop>
#include <cmath>

CurrencyConverter::CurrencyConverter(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    // Initialize currency mappings
    initializeCurrencies();

    // Set up file path for exchange rates
    QString executableDir = QCoreApplication::applicationDirPath();
    m_ratesFilePath = QDir(executableDir).filePath("exchange_rates.json");

    // Set up regular expressions
    m_conversionPattern = QRegularExpression(
        R"(^([^\s]+)\s+(.+?)\s+to\s+(.+?)$)",
        QRegularExpression::CaseInsensitiveOption
    );
    m_numericPattern = QRegularExpression(R"(^\d+(\.\d+)?$)");

    // Load exchange rates from file
    if (!loadExchangeRatesFromFile()) {
        LOG_DEBUG("Failed to load exchange rates from file, using empty rates");
    }

    LOG_DEBUG("CurrencyConverter initialized with file-based rates");
}

CurrencyConverter::~CurrencyConverter()
{
    // Cleanup handled by Qt parent-child relationship
}

CurrencyResult CurrencyConverter::convertExpression(const QString &expression)
{
    try {
        double amount;
        QString fromCurrency, toCurrency;
        
        if (!parseExpression(expression, amount, fromCurrency, toCurrency)) {
            return CurrencyResult::error("Invalid currency conversion format");
        }
        
        return convert(amount, fromCurrency, toCurrency);
        
    } catch (const CurrencyException &e) {
        return CurrencyResult::error(QString::fromStdString(e.what()));
    }
}

CurrencyResult CurrencyConverter::convert(double amount, const QString &fromCurrency, const QString &toCurrency)
{
    try {
        QString fromCode = normalizeCurrencyCode(fromCurrency);
        QString toCode = normalizeCurrencyCode(toCurrency);
        
        if (fromCode.isEmpty()) {
            return CurrencyResult::error(QString("Unsupported currency: %1").arg(fromCurrency));
        }
        
        if (toCode.isEmpty()) {
            return CurrencyResult::error(QString("Unsupported currency: %1").arg(toCurrency));
        }
        
        double rate = getExchangeRate(fromCode, toCode);
        if (rate < 0) {
            return CurrencyResult::error("Exchange rate not available");
        }
        
        double result = amount * rate;
        QString displayName = getCurrencyDisplayName(toCode);
        
        LOG_DEBUG(QString("Currency conversion: %1 %2 -> %3 %4 (rate: %5)")
                  .arg(amount).arg(fromCode).arg(result).arg(toCode).arg(rate));
        
        return CurrencyResult::success(std::make_pair(result, displayName));
        
    } catch (const CurrencyException &e) {
        return CurrencyResult::error(QString::fromStdString(e.what()));
    }
}

double CurrencyConverter::getExchangeRate(const QString &fromCurrency, const QString &toCurrency)
{
    if (fromCurrency == toCurrency) {
        return 1.0;
    }
    
    // Use rates from file
    if (m_exchangeRates.contains(fromCurrency) && m_exchangeRates.contains(toCurrency)) {
        // Convert via USD (base currency)
        double fromRate = m_exchangeRates[fromCurrency];
        double toRate = m_exchangeRates[toCurrency];

        if (fromRate > 0 && toRate > 0) {
            LOG_DEBUG(QString("Using FILE rates: %1=%2, %3=%4").arg(fromCurrency).arg(fromRate).arg(toCurrency).arg(toRate));
            return toRate / fromRate;
        }
    }
    
    return -1.0; // Rate not available
}

bool CurrencyConverter::isSupportedCurrency(const QString &currencyCode) const
{
    QString normalized = normalizeCurrencyCode(currencyCode);
    return !normalized.isEmpty();
}

bool CurrencyConverter::updateExchangeRatesFromAPI()
{
    // Use open.er-api.com - free API with USD base, no key required
    QString apiUrl = "https://open.er-api.com/v6/latest/USD";
    QUrl url(apiUrl);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "CalcForge/1.0");
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_networkManager->get(request);

    // Wait for the reply synchronously (for simplicity)
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = false;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isObject()) {
            QJsonObject root = doc.object();

            if (root.contains("result") && root["result"].toString() == "success" &&
                root.contains("rates") && root["rates"].isObject()) {

                QJsonObject rates = root["rates"].toObject();
                QMap<QString, double> newRates;

                // Convert QJsonObject to QMap
                for (auto it = rates.begin(); it != rates.end(); ++it) {
                    if (it.value().isDouble()) {
                        newRates[it.key()] = it.value().toDouble();
                    }
                }

                // Add cryptocurrency rates if not present
                if (!newRates.contains("BTC")) {
                    newRates["BTC"] = 0.0000245669; // Approximate BTC rate
                }
                if (!newRates.contains("ETH")) {
                    newRates["ETH"] = 0.000389105; // Approximate ETH rate
                }

                QString lastUpdated = root.value("time_last_update_utc").toString();
                if (lastUpdated.isEmpty()) {
                    lastUpdated = QDateTime::currentDateTime().toString(Qt::RFC2822Date);
                }

                // Save to file and update in-memory rates
                if (saveExchangeRatesToFile(newRates, lastUpdated)) {
                    // Convert QMap to QHash
                    m_exchangeRates.clear();
                    for (auto it = newRates.begin(); it != newRates.end(); ++it) {
                        m_exchangeRates[it.key()] = it.value();
                    }
                    m_lastUpdated = lastUpdated;
                    success = true;
                    LOG_DEBUG(QString("Updated exchange rates for %1 currencies").arg(newRates.size()));
                } else {
                    LOG_DEBUG("Failed to save exchange rates to file");
                }
            } else {
                LOG_DEBUG("Invalid API response structure");
            }
        } else {
            LOG_DEBUG("Invalid JSON response from API");
        }
    } else {
        LOG_DEBUG(QString("API request failed: %1").arg(reply->errorString()));
    }

    reply->deleteLater();
    return success;
}

QStringList CurrencyConverter::getSupportedCurrencies() const
{
    return m_currencyAbbreviations.values();
}

bool CurrencyConverter::loadExchangeRatesFromFile()
{
    QFile file(m_ratesFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_DEBUG(QString("Cannot open exchange rates file: %1").arg(m_ratesFilePath));
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        LOG_DEBUG("Invalid JSON in exchange rates file");
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("rates") || !root["rates"].isObject()) {
        LOG_DEBUG("No rates found in exchange rates file");
        return false;
    }

    QJsonObject rates = root["rates"].toObject();
    m_exchangeRates.clear();

    // Load all rates
    for (auto it = rates.begin(); it != rates.end(); ++it) {
        if (it.value().isDouble()) {
            m_exchangeRates[it.key()] = it.value().toDouble();
        }
    }

    m_lastUpdated = root.value("last_updated").toString();

    LOG_DEBUG(QString("Loaded %1 exchange rates from file").arg(m_exchangeRates.size()));
    LOG_DEBUG(QString("Last updated: %1").arg(m_lastUpdated));
    return true;
}

bool CurrencyConverter::saveExchangeRatesToFile(const QMap<QString, double> &rates, const QString &lastUpdated)
{
    QJsonObject root;
    root["last_updated"] = lastUpdated;
    root["base_currency"] = "USD";

    QJsonObject ratesObj;
    for (auto it = rates.begin(); it != rates.end(); ++it) {
        ratesObj[it.key()] = it.value();
    }
    root["rates"] = ratesObj;

    QJsonDocument doc(root);

    QFile file(m_ratesFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        LOG_DEBUG(QString("Cannot write to exchange rates file: %1").arg(m_ratesFilePath));
        return false;
    }

    file.write(doc.toJson());
    file.close();

    LOG_DEBUG(QString("Saved %1 exchange rates to file").arg(rates.size()));
    return true;
}

void CurrencyConverter::initializeCurrencies()
{
    // Currency name to abbreviation mapping
    m_currencyAbbreviations = {
        // Major currencies
        {"dollar", "USD"}, {"dollars", "USD"}, {"usd", "USD"}, {"us dollar", "USD"}, {"us dollars", "USD"},
        {"euro", "EUR"}, {"euros", "EUR"}, {"eur", "EUR"},
        {"pound", "GBP"}, {"pounds", "GBP"}, {"gbp", "GBP"}, {"sterling", "GBP"}, {"british pound", "GBP"}, {"british pounds", "GBP"},
        {"yen", "JPY"}, {"jpy", "JPY"}, {"japanese yen", "JPY"},
        {"yuan", "CNY"}, {"cny", "CNY"}, {"rmb", "CNY"}, {"chinese yuan", "CNY"},
        {"franc", "CHF"}, {"francs", "CHF"}, {"chf", "CHF"}, {"swiss franc", "CHF"}, {"swiss francs", "CHF"},
        {"krona", "SEK"}, {"kronor", "SEK"}, {"sek", "SEK"}, {"swedish krona", "SEK"}, {"swedish kronor", "SEK"},
        {"krone", "NOK"}, {"kroner", "NOK"}, {"nok", "NOK"}, {"norwegian krone", "NOK"}, {"norwegian kroner", "NOK"},
        {"rupee", "INR"}, {"rupees", "INR"}, {"inr", "INR"}, {"indian rupee", "INR"}, {"indian rupees", "INR"},
        {"won", "KRW"}, {"krw", "KRW"}, {"korean won", "KRW"}, {"south korean won", "KRW"},
        {"real", "BRL"}, {"reais", "BRL"}, {"brl", "BRL"}, {"brazilian real", "BRL"},
        {"ruble", "RUB"}, {"rubles", "RUB"}, {"rub", "RUB"}, {"russian ruble", "RUB"}, {"russian rubles", "RUB"},
        {"peso", "MXN"}, {"pesos", "MXN"}, {"mxn", "MXN"}, {"mexican peso", "MXN"}, {"mexican pesos", "MXN"},
        {"rand", "ZAR"}, {"zar", "ZAR"}, {"south african rand", "ZAR"},
        {"lira", "TRY"}, {"try", "TRY"}, {"turkish lira", "TRY"},
        {"shekel", "ILS"}, {"shekels", "ILS"}, {"ils", "ILS"}, {"israeli shekel", "ILS"}, {"israeli shekels", "ILS"},
        {"dirham", "AED"}, {"dirhams", "AED"}, {"aed", "AED"}, {"uae dirham", "AED"}, {"uae dirhams", "AED"},
        {"riyal", "SAR"}, {"riyals", "SAR"}, {"sar", "SAR"}, {"saudi riyal", "SAR"}, {"saudi riyals", "SAR"},
        {"dinar", "KWD"}, {"dinars", "KWD"}, {"kwd", "KWD"}, {"kuwaiti dinar", "KWD"}, {"kuwaiti dinars", "KWD"},
        // Additional major currencies from exchange rates file
        {"cad", "CAD"}, {"canadian dollar", "CAD"}, {"canadian dollars", "CAD"},
        {"aud", "AUD"}, {"australian dollar", "AUD"}, {"australian dollars", "AUD"},
        {"nzd", "NZD"}, {"new zealand dollar", "NZD"}, {"new zealand dollars", "NZD"},
        {"hkd", "HKD"}, {"hong kong dollar", "HKD"}, {"hong kong dollars", "HKD"},
        {"sgd", "SGD"}, {"singapore dollar", "SGD"}, {"singapore dollars", "SGD"},
        {"thb", "THB"}, {"thai baht", "THB"}, {"baht", "THB"},
        {"dkk", "DKK"}, {"danish krone", "DKK"}, {"danish kroner", "DKK"},
        {"pln", "PLN"}, {"polish zloty", "PLN"}, {"zloty", "PLN"},
        {"czk", "CZK"}, {"czech koruna", "CZK"}, {"koruna", "CZK"},
        {"huf", "HUF"}, {"hungarian forint", "HUF"}, {"forint", "HUF"},
        {"bitcoin", "BTC"}, {"btc", "BTC"},
        {"ethereum", "ETH"}, {"eth", "ETH"}
    };
    
    // Currency display names
    m_currencyDisplayNames = {
        {"USD", "US Dollars"}, {"EUR", "Euros"}, {"GBP", "British Pounds"},
        {"JPY", "Japanese Yen"}, {"CNY", "Chinese Yuan"}, {"CHF", "Swiss Francs"},
        {"SEK", "Swedish Kronor"}, {"NOK", "Norwegian Kroner"}, {"INR", "Indian Rupees"},
        {"KRW", "South Korean Won"}, {"BRL", "Brazilian Reais"}, {"RUB", "Russian Rubles"},
        {"MXN", "Mexican Pesos"}, {"ZAR", "South African Rand"}, {"TRY", "Turkish Lira"},
        {"ILS", "Israeli Shekels"}, {"AED", "UAE Dirhams"}, {"SAR", "Saudi Riyals"},
        {"KWD", "Kuwaiti Dinars"}, {"BTC", "Bitcoin"}, {"ETH", "Ethereum"},
        // Additional currencies
        {"CAD", "Canadian Dollars"}, {"AUD", "Australian Dollars"}, {"NZD", "New Zealand Dollars"},
        {"HKD", "Hong Kong Dollars"}, {"SGD", "Singapore Dollars"}, {"THB", "Thai Baht"},
        {"DKK", "Danish Kroner"}, {"PLN", "Polish Zloty"}, {"CZK", "Czech Koruna"},
        {"HUF", "Hungarian Forint"}
    };
}

bool CurrencyConverter::parseExpression(const QString &expression, double &amount,
                                       QString &fromCurrency, QString &toCurrency)
{
    QRegularExpressionMatch match = m_conversionPattern.match(expression.trimmed());
    if (!match.hasMatch()) {
        return false;
    }

    QString amountStr = match.captured(1).trimmed();
    bool ok;
    amount = amountStr.toDouble(&ok);

    // Check for invalid amount format
    if (!ok) {
        throw CurrencyException(QString("Invalid amount '%1' - must be a valid number").arg(amountStr));
    }

    // Check for negative amount
    if (amount < 0) {
        throw CurrencyException(QString("Invalid amount '%1' - amount cannot be negative").arg(amount));
    }

    fromCurrency = match.captured(2).trimmed();
    toCurrency = match.captured(3).trimmed();

    return true;
}

QString CurrencyConverter::normalizeCurrencyCode(const QString &currencyName) const
{
    QString normalized = currencyName.toLower().trimmed();

    // Direct lookup in abbreviations map
    if (m_currencyAbbreviations.contains(normalized)) {
        return m_currencyAbbreviations[normalized];
    }

    // Check if it's already a valid currency code
    QString upper = currencyName.toUpper().trimmed();
    if (m_currencyDisplayNames.contains(upper)) {
        return upper;
    }

    return QString(); // Not found
}

QString CurrencyConverter::getCurrencyDisplayName(const QString &currencyCode)
{
    return m_currencyDisplayNames.value(currencyCode.toUpper(), currencyCode);
}

QString CurrencyConverter::formatCurrencyResult(double amount, const QString &currencyCode)
{
    QString displayName = getCurrencyDisplayName(currencyCode);

    // Format with appropriate precision
    if (currencyCode == "JPY" || currencyCode == "KRW") {
        // Currencies without decimal places
        return QString("%1 %2").arg(qRound(amount)).arg(displayName);
    } else if (currencyCode == "BTC" || currencyCode == "ETH") {
        // Cryptocurrencies with higher precision
        return QString("%1 %2").arg(amount, 0, 'f', 6).arg(displayName);
    } else {
        // Standard currencies with 2 decimal places
        return QString("%1 %2").arg(amount, 0, 'f', 2).arg(displayName);
    }
}


