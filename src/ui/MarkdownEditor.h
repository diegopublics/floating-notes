#pragma once

#include <QTextCharFormat>
#include <QTextListFormat>
#include <QWidget>

class QFont;
class QColor;
class QTextCursor;
class QTextEdit;
class QToolButton;

class MarkdownEditor final : public QWidget
{
    Q_OBJECT

public:
    explicit MarkdownEditor(QWidget *parent = nullptr);

    int characterCount() const;
    QString storage() const;
    QTextCursor textCursor() const;
    void setEditorFont(const QFont &font);
    void setMarkdown(const QString &markdown);
    void setPlaceholderText(const QString &text);
    void setTextCursor(const QTextCursor &cursor);
    void focusEditor(Qt::FocusReason reason);
    void insertChecklistItem();

signals:
    void textChanged();
    void cursorPositionChanged();

private:
    void applyCharacterFormat(const QTextCharFormat &format);
    void applyTextColor(const QColor &color);
    void applyHighlightColor(const QColor &color);
    void clearCharacterProperty(int property);
    void applyHeading();
    void applyQuote();
    void applyList(QTextListFormat::Style style);
    void insertLink();
    void updateToolbarState();

    QTextEdit *m_editor = nullptr;
    QToolButton *m_headingButton = nullptr;
    QToolButton *m_boldButton = nullptr;
    QToolButton *m_italicButton = nullptr;
    QToolButton *m_strikeButton = nullptr;
    QToolButton *m_codeButton = nullptr;
};
