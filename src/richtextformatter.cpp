#include "richtextformatter.h"

#include <QQuickTextDocument>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

RichTextFormatter::RichTextFormatter(QObject *parent)
    : QObject(parent)
{
}

void RichTextFormatter::setDocument(QQuickTextDocument *document)
{
    if (m_document == document)
        return;
    m_document = document;
    emit documentChanged();
    emit formatChanged();
}

void RichTextFormatter::setCursorPosition(int position)
{
    if (m_cursorPosition == position)
        return;
    m_cursorPosition = position;
    emit cursorPositionChanged();
    emit formatChanged();
}

void RichTextFormatter::setSelectionStart(int position)
{
    if (m_selectionStart == position)
        return;
    m_selectionStart = position;
    emit selectionStartChanged();
    emit formatChanged();
}

void RichTextFormatter::setSelectionEnd(int position)
{
    if (m_selectionEnd == position)
        return;
    m_selectionEnd = position;
    emit selectionEndChanged();
    emit formatChanged();
}

QTextDocument *RichTextFormatter::textDocument() const
{
    return m_document ? m_document->textDocument() : nullptr;
}

QTextCursor RichTextFormatter::textCursor() const
{
    QTextDocument *doc = textDocument();
    if (!doc)
        return QTextCursor();

    QTextCursor cursor(doc);
    if (m_selectionStart != m_selectionEnd) {
        cursor.setPosition(m_selectionStart);
        cursor.setPosition(m_selectionEnd, QTextCursor::KeepAnchor);
    } else {
        cursor.setPosition(qMax(0, m_cursorPosition));
    }
    return cursor;
}

void RichTextFormatter::mergeFormat(const QTextCharFormat &format)
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return;
    // 无选区时选中光标所在单词，否则格式改动无处着落
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    emit formatChanged();
}

bool RichTextFormatter::bold() const
{
    QTextCursor cursor = textCursor();
    return !cursor.isNull() && cursor.charFormat().fontWeight() >= QFont::Bold;
}

bool RichTextFormatter::italic() const
{
    QTextCursor cursor = textCursor();
    return !cursor.isNull() && cursor.charFormat().fontItalic();
}

bool RichTextFormatter::underline() const
{
    QTextCursor cursor = textCursor();
    return !cursor.isNull() && cursor.charFormat().fontUnderline();
}

int RichTextFormatter::fontSize() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return 0;
    const qreal size = cursor.charFormat().fontPointSize();
    return size > 0 ? qRound(size) : qRound(textDocument()->defaultFont().pointSizeF());
}

QColor RichTextFormatter::textColor() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
        return QColor();
    const QBrush brush = cursor.charFormat().foreground();
    return brush.style() == Qt::NoBrush ? QColor(Qt::white) : brush.color();
}

void RichTextFormatter::toggleBold()
{
    QTextCharFormat format;
    format.setFontWeight(bold() ? QFont::Normal : QFont::Bold);
    mergeFormat(format);
}

void RichTextFormatter::toggleItalic()
{
    QTextCharFormat format;
    format.setFontItalic(!italic());
    mergeFormat(format);
}

void RichTextFormatter::toggleUnderline()
{
    QTextCharFormat format;
    format.setFontUnderline(!underline());
    mergeFormat(format);
}

void RichTextFormatter::setFontSize(int pointSize)
{
    if (pointSize <= 0)
        return;
    QTextCharFormat format;
    format.setFontPointSize(pointSize);
    mergeFormat(format);
}

void RichTextFormatter::setColor(const QColor &color)
{
    if (!color.isValid())
        return;
    QTextCharFormat format;
    format.setForeground(color);
    mergeFormat(format);
}
