#ifndef TIMECODECALCULATOR_H
#define TIMECODECALCULATOR_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <cmath>
#include <stdexcept>

/**
 * Timecode calculation result structure
 * Contains the result value and any error information
 */
struct TimecodeResult {
    QString value;
    bool isValid;
    QString errorMessage;

    TimecodeResult() : isValid(false) {}
    TimecodeResult(const QString& val) : value(val), isValid(true) {}
    TimecodeResult(const QString& error, bool) : isValid(false), errorMessage(error) {}
};

/**
 * Timecode parsing structure
 * Contains parsed timecode components
 */
struct TimecodeComponents {
    int hours;
    int minutes;
    int seconds;
    int frames;
    
    TimecodeComponents() : hours(0), minutes(0), seconds(0), frames(0) {}
    TimecodeComponents(int h, int m, int s, int f) : hours(h), minutes(m), seconds(s), frames(f) {}
};

/**
 * Custom exception for timecode errors
 */
class TimecodeError : public std::runtime_error {
public:
    explicit TimecodeError(const QString& message) 
        : std::runtime_error(message.toStdString()) {}
};

/**
 * Comprehensive timecode calculation system for CalcForge C++
 * Handles timecode conversions, arithmetic, and drop frame calculations
 * Supports multiple frame rates including NTSC drop frame rates
 */
class TimecodeCalculator
{
public:
    TimecodeCalculator();
    
    /**
     * Main TC function that handles both conversion and arithmetic
     * @param fps Frame rate (24, 30, 29.97, 59.94, 23.976, etc.)
     * @param expression Timecode expression or frame count
     * @return TimecodeResult with converted value or error
     */
    TimecodeResult TC(double fps, const QString &expression);
    
    /**
     * Parse a timecode string into components
     * @param timecodeStr Timecode string (HH:MM:SS:FF or HH.MM.SS.FF)
     * @return TimecodeComponents with parsed values
     * @throws TimecodeError if parsing fails
     */
    TimecodeComponents parseTimecode(const QString &timecodeStr);
    
    /**
     * Convert timecode string to total frame count
     * @param timecodeStr Timecode string or numeric frame count
     * @param fps Frame rate for conversion
     * @return Total frame count
     * @throws TimecodeError if conversion fails
     */
    int timecodeToFrames(const QString &timecodeStr, double fps);
    
    /**
     * Convert frame count to timecode string
     * @param frameCount Total frame count (can be negative)
     * @param fps Frame rate for conversion
     * @return Formatted timecode string (HH:MM:SS:FF)
     */
    QString framesToTimecode(int frameCount, double fps);
    
    /**
     * Evaluate a timecode arithmetic expression
     * @param fps Frame rate for calculations
     * @param expression Expression with timecodes and arithmetic operators
     * @return Result as timecode string
     * @throws TimecodeError if evaluation fails
     */
    QString evaluateTimecodeExpression(double fps, const QString &expression);
    
    /**
     * Check if a string represents a valid timecode format
     * @param str String to check
     * @return True if string matches timecode pattern
     */
    bool isTimecodeFormat(const QString &str) const;
    
    /**
     * Check if frame rate is a drop frame rate
     * @param fps Frame rate to check
     * @return True if it's a drop frame rate (29.97, 59.94)
     */
    bool isDropFrameRate(double fps) const;

private:
    /**
     * Validate timecode components
     * @param components Timecode components to validate
     * @param fps Frame rate for frame count validation
     * @throws TimecodeError if validation fails
     */
    void validateTimecode(const TimecodeComponents &components, double fps);
    
    /**
     * Calculate drop frame adjustments for NTSC rates
     * @param totalMinutes Total minutes elapsed
     * @return Number of frames to drop
     */
    int calculateDropFrames(int totalMinutes) const;
    
    /**
     * Parse and tokenize timecode arithmetic expression
     * @param expression Expression to parse
     * @return List of tokens (timecodes, numbers, operators)
     */
    QStringList tokenizeExpression(const QString &expression) const;
    
    /**
     * Apply arithmetic operation to frame counts
     * @param left Left operand (frame count)
     * @param right Right operand (frame count)
     * @param operation Operator (+, -, *, /)
     * @return Result frame count
     */
    double applyOperation(double left, double right, QChar operation) const;
    
    // Regular expressions for timecode pattern matching
    QRegularExpression m_timecodePattern;
    QRegularExpression m_numericPattern;
};

#endif // TIMECODECALCULATOR_H
