#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QTextBlock>
#include <QTextEdit>

class ExpressionEditor;

/**
 * Line number area widget for displaying line numbers next to the expression editor
 * Similar to Qt's Code Editor example but optimized for CalcForge
 */
class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(ExpressionEditor *editor);
    explicit LineNumberArea(QTextEdit *textEdit);  // Generic constructor for ResultsDisplay
    ~LineNumberArea();
    
    // Size management
    QSize sizeHint() const override;
    int getWidth() const;
    void updateWidth();
    
    // Display management
    void updateLineNumbers();
    void setFont(const QFont &font);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupWidget();
    int calculateWidth() const;
    void paintLineNumbers(QPainter &painter, const QRect &rect);

    // Associated editor (can be ExpressionEditor or ResultsDisplay)
    ExpressionEditor *m_editor;
    QTextEdit *m_textEdit;  // Generic text edit for ResultsDisplay
    
    // Display properties
    QFont m_font;
    int m_width;
    int m_digitWidth;
    
    // Colors and styling
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_currentLineColor;
    QColor m_commentLineColor;
};

#endif // LINENUMBERAREA_H
