#include "Logger.h"
#include <QCoreApplication>
#include <QStandardPaths>

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

Logger::Logger()
    : m_logFile(nullptr)
    , m_stream(nullptr)
{
    initializeLogFile();
}

Logger::~Logger()
{
    if (m_stream) {
        delete m_stream;
    }
    if (m_logFile) {
        m_logFile->close();
        delete m_logFile;
    }
}

void Logger::initializeLogFile()
{
    // Create logs directory in the same directory as the executable
    QString appDir = QCoreApplication::applicationDirPath();
    QDir logsDir(appDir + "/logs");
    if (!logsDir.exists()) {
        logsDir.mkpath(".");
    }
    
    // Create timestamped log file name
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    m_logFileName = logsDir.absoluteFilePath(QString("calcforge_%1.log").arg(timestamp));
    
    // Open log file
    m_logFile = new QFile(m_logFileName);
    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_stream = new QTextStream(m_logFile);
        
        // Write session start header
        writeToFile("=== CalcForge C++ Session Started ===");
        writeToFile(QString("Timestamp: %1").arg(QDateTime::currentDateTime().toString()));
        writeToFile(QString("Log file: %1").arg(m_logFileName));
        writeToFile("=====================================");
    } else {
        qDebug() << "Failed to open log file:" << m_logFileName;
    }
}

void Logger::writeToFile(const QString &message)
{
    if (m_stream) {
        QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        *m_stream << QString("[%1] %2").arg(timestamp, message) << Qt::endl;
        m_stream->flush(); // Ensure immediate write
    }
}

void Logger::log(const QString &message)
{
    writeToFile(message);
}

void Logger::debug(const QString &message)
{
    writeToFile(QString("DEBUG: %1").arg(message));
}

void Logger::error(const QString &message)
{
    writeToFile(QString("ERROR: %1").arg(message));
}

void Logger::info(const QString &message)
{
    writeToFile(QString("INFO: %1").arg(message));
}

void Logger::warning(const QString &message)
{
    writeToFile(QString("WARNING: %1").arg(message));
}
