#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QDebug>

/**
 * Simple logging utility that writes to timestamped log files
 * Creates a new log file for each app session
 */
class Logger
{
public:
    static Logger& instance();
    
    /**
     * Log a message with timestamp
     */
    void log(const QString &message);
    
    /**
     * Log a debug message (prefixed with DEBUG)
     */
    void debug(const QString &message);
    
    /**
     * Log an error message (prefixed with ERROR)
     */
    void error(const QString &message);
    
    /**
     * Log an info message (prefixed with INFO)
     */
    void info(const QString &message);

    /**
     * Log a warning message (prefixed with WARNING)
     */
    void warning(const QString &message);

private:
    Logger();
    ~Logger();
    
    void initializeLogFile();
    void writeToFile(const QString &message);
    
    QFile *m_logFile;
    QTextStream *m_stream;
    QString m_logFileName;
};

// Convenience macros for logging
#define LOG_DEBUG(msg) Logger::instance().debug(msg)
#define LOG_INFO(msg) Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)
#define LOG(msg) Logger::instance().log(msg)

#endif // LOGGER_H
