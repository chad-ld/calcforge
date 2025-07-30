#include "TimecodeCalculator.h"
#include "Logger.h"
#include <QDebug>
#include <QRegularExpression>
#include <cmath>

TimecodeCalculator::TimecodeCalculator()
{
    // Initialize regex patterns for timecode matching
    m_timecodePattern = QRegularExpression(R"(^\d{1,2}[:\.]\d{1,2}[:\.]\d{1,2}[:\.]\d{1,2}$)");
    m_numericPattern = QRegularExpression(R"(^\d+$)");
}

TimecodeResult TimecodeCalculator::TC(double fps, const QString &expression)
{
    try {
        if (fps <= 0) {
            return TimecodeResult::error("Framerate must be positive");
        }
        
        QString expr = expression.trimmed();
        
        // If it's a simple number, convert frames to timecode
        if (m_numericPattern.match(expr).hasMatch()) {
            bool ok;
            int frames = expr.toInt(&ok);
            if (ok) {
                return TimecodeResult::success(framesToTimecode(frames, fps));
            }
        }
        
        // If it's a single timecode without arithmetic, return frame count
        if (isTimecodeFormat(expr)) {
            int frames = timecodeToFrames(expr, fps);
            return TimecodeResult::success(QString::number(frames));
        }
        
        // Handle timecode arithmetic
        QString result = evaluateTimecodeExpression(fps, expr);
        return TimecodeResult::success(result);
        
    } catch (const TimecodeException &e) {
        LOG_DEBUG(QString("Timecode calculation error: %1").arg(e.what()));
        return TimecodeResult::error(QString("Error: %1").arg(e.what()));
    } catch (const std::exception &e) {
        LOG_DEBUG(QString("Timecode calculation error: %1").arg(e.what()));
        return TimecodeResult::error(QString("Error: %1").arg(e.what()));
    }
}

TimecodeComponents TimecodeCalculator::parseTimecode(const QString &timecodeStr)
{
    // Replace periods with colons for consistent parsing
    QString normalized = timecodeStr;
    normalized.replace('.', ':');
    
    QStringList parts = normalized.split(':');
    
    if (parts.size() != 4) {
        throw TimecodeException(QString("Invalid timecode format: %1. Expected HH:MM:SS:FF").arg(timecodeStr));
    }
    
    bool ok;
    int hours = parts[0].toInt(&ok);
    if (!ok) throw TimecodeException(QString("Invalid hours in timecode: %1").arg(parts[0]));
    
    int minutes = parts[1].toInt(&ok);
    if (!ok) throw TimecodeException(QString("Invalid minutes in timecode: %1").arg(parts[1]));
    
    int seconds = parts[2].toInt(&ok);
    if (!ok) throw TimecodeException(QString("Invalid seconds in timecode: %1").arg(parts[2]));
    
    int frames = parts[3].toInt(&ok);
    if (!ok) throw TimecodeException(QString("Invalid frames in timecode: %1").arg(parts[3]));
    
    // Validate ranges
    if (hours < 0 || minutes < 0 || minutes >= 60 || seconds < 0 || seconds >= 60 || frames < 0) {
        throw TimecodeException(QString("Invalid timecode values in: %1").arg(timecodeStr));
    }
    
    return TimecodeComponents(hours, minutes, seconds, frames);
}

int TimecodeCalculator::timecodeToFrames(const QString &timecodeStr, double fps)
{
    // If it's already a number, return it
    bool ok;
    int directFrames = timecodeStr.toInt(&ok);
    if (ok) {
        return directFrames;
    }
    
    TimecodeComponents tc = parseTimecode(timecodeStr);
    validateTimecode(tc, fps);
    
    if (std::abs(fps - 29.97) < 0.01) {
        // 29.97 fps drop frame
        int totalMinutes = (60 * tc.hours) + tc.minutes;
        int totalFrames = (tc.hours * 3600 * 30) + (tc.minutes * 60 * 30) + (tc.seconds * 30) + tc.frames;
        int drops = calculateDropFrames(totalMinutes);
        return totalFrames - drops;
    }
    else if (std::abs(fps - 59.94) < 0.01) {
        // 59.94 fps drop frame
        int totalMinutes = (60 * tc.hours) + tc.minutes;
        int totalFrames = (tc.hours * 3600 * 60) + (tc.minutes * 60 * 60) + (tc.seconds * 60) + tc.frames;
        int drops = 4 * (totalMinutes - totalMinutes / 10);
        return totalFrames - drops;
    }
    else if (std::abs(fps - 23.976) < 0.01) {
        // 23.976 fps - exact NTSC frame rate
        double exactFps = 24000.0 / 1001.0;
        int totalSeconds = tc.hours * 3600 + tc.minutes * 60 + tc.seconds;
        return static_cast<int>(std::round(totalSeconds * exactFps)) + tc.frames;
    }
    else {
        // Non-drop frame rates - simple calculation
        int totalSeconds = tc.hours * 3600 + tc.minutes * 60 + tc.seconds;
        return static_cast<int>(totalSeconds * fps) + tc.frames;
    }
}

QString TimecodeCalculator::framesToTimecode(int frameCount, double fps)
{
    QString sign = "";
    if (frameCount < 0) {
        sign = "-";
        frameCount = std::abs(frameCount);
    }
    
    int hours, minutes, seconds, frames;
    
    if (std::abs(fps - 29.97) < 0.01) {
        // 29.97 fps drop frame
        int totalMinutes = frameCount / (30 * 60);
        int drops = calculateDropFrames(totalMinutes);
        int realFrames = frameCount + drops;
        
        frames = realFrames % 30;
        int totalSeconds = realFrames / 30;
        seconds = totalSeconds % 60;
        totalMinutes = totalSeconds / 60;
        hours = totalMinutes / 60;
        minutes = totalMinutes % 60;
    }
    else if (std::abs(fps - 59.94) < 0.01) {
        // 59.94 fps drop frame
        int totalMinutes = frameCount / (60 * 60);
        int drops = 4 * (totalMinutes - totalMinutes / 10);
        int realFrames = frameCount + drops;
        
        frames = realFrames % 60;
        int totalSeconds = realFrames / 60;
        seconds = totalSeconds % 60;
        totalMinutes = totalSeconds / 60;
        hours = totalMinutes / 60;
        minutes = totalMinutes % 60;
    }
    else {
        // Non-drop frame rates
        double totalSeconds;
        if (std::abs(fps - 23.976) < 0.01) {
            double exactFps = 24000.0 / 1001.0;
            totalSeconds = frameCount / exactFps;
        } else {
            totalSeconds = frameCount / fps;
        }
        
        hours = static_cast<int>(totalSeconds / 3600);
        minutes = static_cast<int>((static_cast<int>(totalSeconds) % 3600) / 60);
        seconds = static_cast<int>(totalSeconds) % 60;
        frames = static_cast<int>(std::round((totalSeconds - static_cast<int>(totalSeconds)) * fps));
        
        // Handle frame overflow
        if (frames >= static_cast<int>(std::round(fps))) {
            frames = 0;
            seconds++;
            if (seconds >= 60) {
                seconds = 0;
                minutes++;
                if (minutes >= 60) {
                    minutes = 0;
                    hours++;
                }
            }
        }
    }
    
    return QString("%1%2:%3:%4:%5")
        .arg(sign)
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(frames, 2, 10, QChar('0'));
}

QString TimecodeCalculator::evaluateTimecodeExpression(double fps, const QString &expression)
{
    // If it's just a number, convert it to timecode
    bool ok;
    int directFrames = expression.toInt(&ok);
    if (ok) {
        return framesToTimecode(directFrames, fps);
    }
    
    QStringList tokens = tokenizeExpression(expression);
    if (tokens.isEmpty()) {
        throw TimecodeException("No valid timecode or numeric values found in expression");
    }
    
    double result = 0.0;
    QChar currentOp = '+';
    bool hasResult = false;
    
    for (const QString &token : tokens) {
        if (token == "+" || token == "-" || token == "*" || token == "/") {
            currentOp = token[0];
            continue;
        }
        
        try {
            double frames;
            if (isTimecodeFormat(token)) {
                frames = timecodeToFrames(token, fps);
            } else {
                frames = token.toDouble(&ok);
                if (!ok) {
                    throw TimecodeException(QString("Invalid numeric value: %1").arg(token));
                }
            }
            
            if (!hasResult) {
                result = frames;
                hasResult = true;
            } else {
                result = applyOperation(result, frames, currentOp);
            }
        } catch (const std::exception &e) {
            throw TimecodeException(QString("Error in timecode expression: %1").arg(e.what()));
        }
    }
    
    if (!hasResult) {
        throw TimecodeException("No valid timecode or numeric values found in expression");
    }
    
    return framesToTimecode(static_cast<int>(std::round(result)), fps);
}

bool TimecodeCalculator::isTimecodeFormat(const QString &str) const
{
    return m_timecodePattern.match(str).hasMatch();
}

bool TimecodeCalculator::isDropFrameRate(double fps) const
{
    return (std::abs(fps - 29.97) < 0.01) || (std::abs(fps - 59.94) < 0.01);
}

void TimecodeCalculator::validateTimecode(const TimecodeComponents &components, double fps)
{
    // Validate frame count against fps
    int maxFrames = static_cast<int>(fps);
    if (fps != static_cast<int>(fps)) {
        maxFrames = static_cast<int>(fps) + 1;
    }

    if (components.frames >= maxFrames) {
        throw TimecodeException(QString("Frame count %1 exceeds maximum for %2 fps (max: %3)")
                           .arg(components.frames).arg(fps).arg(maxFrames - 1));
    }
}

int TimecodeCalculator::calculateDropFrames(int totalMinutes) const
{
    // Drop frame calculation: 2 frames dropped every minute except every 10th minute
    return 2 * (totalMinutes - totalMinutes / 10);
}

QStringList TimecodeCalculator::tokenizeExpression(const QString &expression) const
{
    QStringList tokens;
    QString currentToken;

    for (int i = 0; i < expression.length(); ++i) {
        QChar c = expression[i];

        if (c.isSpace()) {
            if (!currentToken.isEmpty()) {
                tokens.append(currentToken);
                currentToken.clear();
            }
        }
        else if (c == '+' || c == '-' || c == '*' || c == '/') {
            if (!currentToken.isEmpty()) {
                tokens.append(currentToken);
                currentToken.clear();
            }
            tokens.append(QString(c));
        }
        else {
            currentToken.append(c);
        }
    }

    if (!currentToken.isEmpty()) {
        tokens.append(currentToken);
    }

    return tokens;
}

double TimecodeCalculator::applyOperation(double left, double right, QChar operation) const
{
    switch (operation.toLatin1()) {
        case '+':
            return left + right;
        case '-':
            return left - right;
        case '*':
            return left * right;
        case '/':
            if (right == 0) {
                throw TimecodeException("Division by zero in timecode expression");
            }
            return left / right;
        default:
            throw TimecodeException(QString("Unknown operation: %1").arg(operation));
    }
}
