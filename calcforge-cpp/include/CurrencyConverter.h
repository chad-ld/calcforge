#ifndef CURRENCYCONVERTER_H
#define CURRENCYCONVERTER_H

#include <QString>
#include <QHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QRegularExpression>
#include <stdexcept>

/**
 * Exception class for currency conversion errors
 */
class CurrencyError : public std::runtime_error
{
public:
    explicit CurrencyError(const QString &message) 
        : std::runtime_error(message.toStdString()) {}
};

/**
 * Result structure for currency conversion operations
 */
struct CurrencyResult
{
    QString value;
    bool isValid;
    QString errorMessage;
    
    CurrencyResult(const QString &val = "", bool valid = true, const QString &error = "")
        : value(val), isValid(valid), errorMessage(error) {}
};

/**
 * Comprehensive currency conversion system for CalcForge C++
 * Handles currency conversions with live exchange rates and fallback rates
 * Supports major world currencies with automatic rate updates
 */
class CurrencyConverter : public QObject
{
    Q_OBJECT

public:
    CurrencyConverter(QObject *parent = nullptr);
    ~CurrencyConverter();
    
    /**
     * Convert currency expression like "100 dollars to euros" or "50 USD to GBP"
     * @param expression Currency conversion expression
     * @return CurrencyResult with converted value or error
     */
    CurrencyResult convertExpression(const QString &expression);
    
    /**
     * Convert between two specific currencies
     * @param amount Amount to convert
     * @param fromCurrency Source currency (USD, EUR, etc.)
     * @param toCurrency Target currency (USD, EUR, etc.)
     * @return CurrencyResult with converted value or error
     */
    CurrencyResult convert(double amount, const QString &fromCurrency, const QString &toCurrency);
    
    /**
     * Get exchange rate between two currencies
     * @param fromCurrency Source currency code
     * @param toCurrency Target currency code
     * @return Exchange rate, or -1 if not available
     */
    double getExchangeRate(const QString &fromCurrency, const QString &toCurrency);
    
    /**
     * Check if a currency code is supported
     * @param currencyCode Currency code to check (USD, EUR, etc.)
     * @return True if currency is supported
     */
    bool isSupportedCurrency(const QString &currencyCode) const;
    
    /**
     * Update exchange rates from API and save to file
     * @return true if successful, false otherwise
     */
    bool updateExchangeRatesFromAPI();
    
    /**
     * Get list of supported currency codes
     * @return List of supported currency abbreviations
     */
    QStringList getSupportedCurrencies() const;

private:

private:
    /**
     * Initialize currency mappings and fallback rates
     */
    void initializeCurrencies();
    
    /**
     * Parse currency conversion expression
     * @param expression Expression to parse
     * @param amount Output: parsed amount
     * @param fromCurrency Output: source currency
     * @param toCurrency Output: target currency
     * @return True if parsing successful
     */
    bool parseExpression(const QString &expression, double &amount, 
                        QString &fromCurrency, QString &toCurrency);
    
    /**
     * Normalize currency name to standard abbreviation
     * @param currencyName Currency name or abbreviation
     * @return Standard currency code (USD, EUR, etc.)
     */
    QString normalizeCurrencyCode(const QString &currencyName) const;
    
    /**
     * Get display name for currency
     * @param currencyCode Currency code
     * @return Display name for the currency
     */
    QString getCurrencyDisplayName(const QString &currencyCode);
    
    /**
     * Format currency result with proper precision
     * @param amount Amount to format
     * @param currencyCode Currency code
     * @return Formatted currency string
     */
    QString formatCurrencyResult(double amount, const QString &currencyCode);
    
    /**
     * Load exchange rates from file
     * @return true if successful, false otherwise
     */
    bool loadExchangeRatesFromFile();

    /**
     * Save exchange rates to file
     * @param rates Exchange rates map
     * @param lastUpdated Last update timestamp
     * @return true if successful, false otherwise
     */
    bool saveExchangeRatesToFile(const QMap<QString, double> &rates, const QString &lastUpdated);

private:
    // Network manager for API requests
    QNetworkAccessManager *m_networkManager;

    // Current exchange rates (base: USD) loaded from file
    QHash<QString, double> m_exchangeRates;

    // File path for exchange rates
    QString m_ratesFilePath;
    
    // Currency name to abbreviation mapping
    QHash<QString, QString> m_currencyAbbreviations;
    
    // Currency code to display name mapping
    QHash<QString, QString> m_currencyDisplayNames;
    
    // Regular expressions for parsing
    QRegularExpression m_conversionPattern;
    QRegularExpression m_numericPattern;
    
    // Last update timestamp string
    QString m_lastUpdated;
};

#endif // CURRENCYCONVERTER_H
