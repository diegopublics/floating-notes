#include "MarkdownEditor.h"

#include "core/NoteBodyFormat.h"

#include <QBoxLayout>
#include <QAction>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QInputDialog>
#include <QIcon>
#include <QMenu>
#include <QPixmap>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QToolButton>

namespace {
QToolButton *createToolButton(const QString &text, const QString &toolTip, QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setText(text);
    button->setToolTip(toolTip);
    button->setCursor(Qt::PointingHandCursor);
    button->setAutoRaise(true);
    button->setFixedSize(26, 24);
    return button;
}

QIcon colorIcon(const QColor &color)
{
    QPixmap pixmap(14, 14);
    pixmap.fill(color);
    return QIcon(pixmap);
}

const QList<QColor> TextColors = {
    QColor(QStringLiteral("#28262a")), QColor(QStringLiteral("#b3261e")),
    QColor(QStringLiteral("#c25a12")), QColor(QStringLiteral("#9d7500")),
    QColor(QStringLiteral("#30743a")), QColor(QStringLiteral("#047c79")),
    QColor(QStringLiteral("#2468a5")), QColor(QStringLiteral("#5047a5")),
    QColor(QStringLiteral("#7a3d98")), QColor(QStringLiteral("#a52a68")),
};

const QList<QColor> HighlightColors = {
    QColor(QStringLiteral("#f5d873")), QColor(QStringLiteral("#f8bd9a")),
    QColor(QStringLiteral("#f6a8b8")), QColor(QStringLiteral("#d9c0f3")),
    QColor(QStringLiteral("#a9d8fb")), QColor(QStringLiteral("#a8e1d1")),
    QColor(QStringLiteral("#c7e8a4")), QColor(QStringLiteral("#f0d7a8")),
    QColor(QStringLiteral("#d7e0e9")), QColor(QStringLiteral("#e1d1bd")),
};
}

MarkdownEditor::MarkdownEditor(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toolbar = new QFrame(this);
    toolbar->setObjectName(QStringLiteral("markdownToolbar"));
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(4, 3, 4, 3);
    toolbarLayout->setSpacing(1);

    const auto addButton = [toolbar, toolbarLayout](const QString &text, const QString &toolTip, auto callback) {
        auto *button = createToolButton(text, toolTip, toolbar);
        QObject::connect(button, &QToolButton::clicked, toolbar, callback);
        toolbarLayout->addWidget(button);
        return button;
    };
    addButton(QStringLiteral("H"), tr("Heading"), [this] { applyHeading(); });
    m_boldButton = addButton(QStringLiteral("B"), tr("Bold"), [this] {
        QTextCharFormat format;
        format.setFontWeight(m_editor->fontWeight() >= QFont::Bold ? QFont::Normal : QFont::Bold);
        applyCharacterFormat(format);
    });
    m_boldButton->setCheckable(true);
    m_italicButton = addButton(QStringLiteral("I"), tr("Italic"), [this] {
        QTextCharFormat format;
        format.setFontItalic(!m_editor->fontItalic());
        applyCharacterFormat(format);
    });
    m_italicButton->setCheckable(true);
    m_strikeButton = addButton(QStringLiteral("S"), tr("Strikethrough"), [this] {
        QTextCharFormat format;
        format.setFontStrikeOut(!m_editor->currentCharFormat().fontStrikeOut());
        applyCharacterFormat(format);
    });
    m_strikeButton->setCheckable(true);
    m_codeButton = addButton(QStringLiteral("</>"), tr("Inline code"), [this] {
        QTextCharFormat format;
        const bool useFixedPitch = !m_editor->currentCharFormat().fontFixedPitch();
        format.setFontFixedPitch(useFixedPitch);
        if (useFixedPitch) {
            format.setFontFamilies({QStringLiteral("Consolas"), QStringLiteral("Monospace")});
            format.setBackground(QColor(80, 68, 72, 38));
        } else {
            format.setFontFamilies(m_editor->font().families());
            format.clearBackground();
        }
        applyCharacterFormat(format);
    });
    m_codeButton->setCheckable(true);
    addButton(QStringLiteral("Link"), tr("Insert link"), [this] { insertLink(); });
    auto *textColorButton = createToolButton(QStringLiteral("A"), tr("Text color"), toolbar);
    auto *textColorMenu = new QMenu(textColorButton);
    auto *clearTextColorAction = textColorMenu->addAction(tr("Clear text color"));
    connect(clearTextColorAction, &QAction::triggered, this, [this] {
        clearCharacterProperty(QTextFormat::ForegroundBrush);
    });
    textColorMenu->addSeparator();
    for (const QColor &color : TextColors) {
        auto *action = textColorMenu->addAction(colorIcon(color), color.name());
        connect(action, &QAction::triggered, this, [this, color] { applyTextColor(color); });
    }
    textColorButton->setMenu(textColorMenu);
    textColorButton->setPopupMode(QToolButton::InstantPopup);
    toolbarLayout->addWidget(textColorButton);
    auto *highlightButton = createToolButton(QStringLiteral("HA"), tr("Text highlight color"), toolbar);
    highlightButton->setFixedWidth(29);
    auto *highlightMenu = new QMenu(highlightButton);
    auto *clearHighlightAction = highlightMenu->addAction(tr("Clear highlight color"));
    connect(clearHighlightAction, &QAction::triggered, this, [this] {
        clearCharacterProperty(QTextFormat::BackgroundBrush);
    });
    highlightMenu->addSeparator();
    for (const QColor &color : HighlightColors) {
        auto *action = highlightMenu->addAction(colorIcon(color), color.name());
        connect(action, &QAction::triggered, this, [this, color] { applyHighlightColor(color); });
    }
    highlightButton->setMenu(highlightMenu);
    highlightButton->setPopupMode(QToolButton::InstantPopup);
    toolbarLayout->addWidget(highlightButton);
    addButton(QStringLiteral("*"), tr("Bulleted list"), [this] { applyList(QTextListFormat::ListDisc); });
    addButton(QStringLiteral("1."), tr("Numbered list"), [this] { applyList(QTextListFormat::ListDecimal); });
    addButton(tr("Task"), tr("Task list"), [this] { insertChecklistItem(); });
    addButton(QStringLiteral(">"), tr("Quote"), [this] {
        QTextCursor cursor = m_editor->textCursor();
        QTextBlockFormat format;
        format.setLeftMargin(18);
        cursor.mergeBlockFormat(format);
    });
    toolbarLayout->addStretch();
    layout->addWidget(toolbar);

    m_editor = new QTextEdit(this);
    m_editor->setObjectName(QStringLiteral("richTextEditor"));
    m_editor->setAcceptRichText(false);
    m_editor->setTabChangesFocus(true);
    connect(m_editor, &QTextEdit::textChanged, this, &MarkdownEditor::textChanged);
    connect(m_editor, &QTextEdit::cursorPositionChanged, this, [this] {
        updateToolbarState();
        emit cursorPositionChanged();
    });
    layout->addWidget(m_editor, 1);
}

int MarkdownEditor::characterCount() const
{
    return m_editor->document()->characterCount();
}

QString MarkdownEditor::storage() const
{
    return NoteBodyFormat::toStorage(m_editor->document());
}

QTextCursor MarkdownEditor::textCursor() const
{
    return m_editor->textCursor();
}

void MarkdownEditor::setEditorFont(const QFont &font)
{
    m_editor->setFont(font);
    m_editor->document()->setDefaultFont(font);
}

void MarkdownEditor::setMarkdown(const QString &markdown)
{
    NoteBodyFormat::loadInto(m_editor->document(), markdown);
    m_editor->document()->clearUndoRedoStacks();
    updateToolbarState();
}

void MarkdownEditor::setPlaceholderText(const QString &text)
{
    m_editor->setPlaceholderText(text);
}

void MarkdownEditor::setTextCursor(const QTextCursor &cursor)
{
    m_editor->setTextCursor(cursor);
}

void MarkdownEditor::focusEditor(Qt::FocusReason reason)
{
    m_editor->setFocus(reason);
}

void MarkdownEditor::insertChecklistItem()
{
    QTextCursor cursor = m_editor->textCursor();
    QTextListFormat listFormat;
    listFormat.setStyle(QTextListFormat::ListDisc);
    cursor.createList(listFormat);
    QTextBlockFormat blockFormat = cursor.blockFormat();
    blockFormat.setMarker(QTextBlockFormat::MarkerType::Unchecked);
    cursor.mergeBlockFormat(blockFormat);
    m_editor->setTextCursor(cursor);
}

void MarkdownEditor::applyCharacterFormat(const QTextCharFormat &format)
{
    m_editor->mergeCurrentCharFormat(format);
    m_editor->setFocus(Qt::ShortcutFocusReason);
}

void MarkdownEditor::applyTextColor(const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);
    applyCharacterFormat(format);
}

void MarkdownEditor::applyHighlightColor(const QColor &color)
{
    QTextCharFormat format;
    format.setBackground(color);
    applyCharacterFormat(format);
}

void MarkdownEditor::clearCharacterProperty(int property)
{
    QTextCursor selection = m_editor->textCursor();
    if (!selection.hasSelection()) {
        QTextCharFormat format = m_editor->currentCharFormat();
        format.clearProperty(property);
        m_editor->setCurrentCharFormat(format);
        m_editor->setFocus(Qt::ShortcutFocusReason);
        return;
    }

    const int start = selection.selectionStart();
    const int end = selection.selectionEnd();
    selection.beginEditBlock();
    for (int position = start; position < end; ++position) {
        QTextCursor character(m_editor->document());
        character.setPosition(position);
        character.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        QTextCharFormat format = character.charFormat();
        format.clearProperty(property);
        character.setCharFormat(format);
    }
    selection.endEditBlock();
    m_editor->setTextCursor(selection);
    m_editor->setFocus(Qt::ShortcutFocusReason);
}

void MarkdownEditor::applyHeading()
{
    QTextCursor cursor = m_editor->textCursor();
    QTextBlockFormat format = cursor.blockFormat();
    format.setHeadingLevel(format.headingLevel() == 2 ? 0 : 2);
    cursor.mergeBlockFormat(format);
    m_editor->setTextCursor(cursor);
    m_editor->setFocus(Qt::ShortcutFocusReason);
}

void MarkdownEditor::applyList(QTextListFormat::Style style)
{
    QTextCursor cursor = m_editor->textCursor();
    QTextListFormat format;
    format.setStyle(style);
    cursor.createList(format);
    m_editor->setTextCursor(cursor);
    m_editor->setFocus(Qt::ShortcutFocusReason);
}

void MarkdownEditor::insertLink()
{
    QTextCursor cursor = m_editor->textCursor();
    QString text = cursor.selectedText();
    if (text.isEmpty()) {
        text = QInputDialog::getText(this, tr("Insert link"), tr("Text"));
        if (text.isEmpty()) {
            return;
        }
        cursor.insertText(text);
        cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, text.size());
    }

    const QString url = QInputDialog::getText(this, tr("Insert link"), tr("URL"));
    if (url.isEmpty()) {
        return;
    }
    QTextCharFormat format;
    format.setAnchor(true);
    format.setAnchorHref(url);
    format.setForeground(QColor(QStringLiteral("#2866a4")));
    format.setFontUnderline(true);
    cursor.mergeCharFormat(format);
    m_editor->setTextCursor(cursor);
}

void MarkdownEditor::updateToolbarState()
{
    const QTextCharFormat format = m_editor->currentCharFormat();
    m_boldButton->setChecked(m_editor->fontWeight() >= QFont::Bold);
    m_italicButton->setChecked(m_editor->fontItalic());
    m_strikeButton->setChecked(format.fontStrikeOut());
    m_codeButton->setChecked(format.fontFixedPitch());
}
