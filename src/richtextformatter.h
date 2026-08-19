#pragma once

#include <QColor>
#include <QObject>
#include <QQuickTextDocument>
#include <QtQml/qqmlregistration.h>

class QTextDocument;
class QTextCursor;
class QTextCharFormat;

/*!
 * 桥接 QML TextArea 的底层 QTextDocument，对当前选区施加字符格式。
 *
 * 用法：把 TextArea.textDocument 赋给 document，把 TextArea 的
 * selectionStart / selectionEnd / cursorPosition 绑定到同名属性，
 * 工具栏即可通过 bold/italic/... 只读属性反映光标处的当前格式。
 */
class RichTextFormatter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY selectionStartChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY selectionEndChanged)
    Q_PROPERTY(bool bold READ bold NOTIFY formatChanged)
    Q_PROPERTY(bool italic READ italic NOTIFY formatChanged)
    Q_PROPERTY(bool underline READ underline NOTIFY formatChanged)
    Q_PROPERTY(int fontSize READ fontSize NOTIFY formatChanged)
    Q_PROPERTY(QColor textColor READ textColor NOTIFY formatChanged)

public:
    explicit RichTextFormatter(QObject *parent = nullptr);

    QQuickTextDocument *document() const { return m_document; }
    void setDocument(QQuickTextDocument *document);

    int cursorPosition() const { return m_cursorPosition; }
    void setCursorPosition(int position);

    int selectionStart() const { return m_selectionStart; }
    void setSelectionStart(int position);

    int selectionEnd() const { return m_selectionEnd; }
    void setSelectionEnd(int position);

    bool bold() const;
    bool italic() const;
    bool underline() const;
    int fontSize() const;
    QColor textColor() const;

    Q_INVOKABLE void toggleBold();
    Q_INVOKABLE void toggleItalic();
    Q_INVOKABLE void toggleUnderline();
    Q_INVOKABLE void setFontSize(int pointSize);
    Q_INVOKABLE void setColor(const QColor &color);

signals:
    void documentChanged();
    void cursorPositionChanged();
    void selectionStartChanged();
    void selectionEndChanged();
    void formatChanged();

private:
    QTextDocument *textDocument() const;
    //! 取当前选区的游标；无选区时退化为光标位置（格式将作用于后续输入）。
    QTextCursor textCursor() const;
    void mergeFormat(const QTextCharFormat &format);

    QQuickTextDocument *m_document = nullptr;
    int m_cursorPosition = -1;
    int m_selectionStart = 0;
    int m_selectionEnd = 0;
};
