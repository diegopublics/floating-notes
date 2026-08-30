#include "EdgeDockWindow.h"

#include "app/AppSettings.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCursor>
#include <QEasingCurve>
#include <QFontDatabase>
#include <QGraphicsDropShadowEffect>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScreen>
#include <QTextCursor>
#include <QTimer>

#include <algorithm>
#include <array>
#include <utility>

namespace {
constexpr int CollapsedWidth = 14;
constexpr int FanWidth = 50;
constexpr int TabWidth = 30;
constexpr int TabHeight = 122;
constexpr int TabLap = 54;
constexpr int TabPitch = 52;
constexpr qreal TabLeanDegrees = 3.0;
constexpr int EditorWidth = 460;
constexpr int EditorHeight = 380;
constexpr int EditorGutterWidth = 30;
constexpr int PlusButtonSize = 28;
constexpr int CollapseDelayMs = 180;
constexpr int SaveDelayMs = 250;
constexpr int UndoDelayMs = 7000;
constexpr int MaxDeckButtons = 8;
constexpr int MaxPillDashes = 14;

const std::array<QString, 8> AvailableColors = {
    QStringLiteral("#fce795"), QStringLiteral("#fbcfa6"),
    QStringLiteral("#fac4d1"), QStringLiteral("#d9c7fa"),
    QStringLiteral("#beddfa"), QStringLiteral("#b4e8d0"),
    QStringLiteral("#e3d3b4"), QStringLiteral("#cbd6e2"),
};
const std::array<QString, 8> DashColors = {
    QStringLiteral("#e0ad08"), QStringLiteral("#e2762a"),
    QStringLiteral("#dc4570"), QStringLiteral("#7c4dee"),
    QStringLiteral("#2280d6"), QStringLiteral("#0e9b6e"),
    QStringLiteral("#a37b3c"), QStringLiteral("#4e6579"),
};

QString dashColorForPaper(const QString &paper)
{
    const auto it = std::find(AvailableColors.cbegin(), AvailableColors.cend(), paper.toLower());
    return it == AvailableColors.cend() ? paper : DashColors.at(std::distance(AvailableColors.cbegin(), it));
}

QString paperColor(const QString &storedColor)
{
    const QString color = storedColor.toLower();
    if (color == QStringLiteral("#e8b457")) return AvailableColors.at(0);
    if (color == QStringLiteral("#79a7d3")) return AvailableColors.at(4);
    if (color == QStringLiteral("#8dbf76")) return AvailableColors.at(5);
    if (color == QStringLiteral("#d97f73")) return AvailableColors.at(2);
    if (color == QStringLiteral("#b391d6")) return AvailableColors.at(3);
    return color;
}

QColor inkColor(const QString &paper)
{
    const QColor color(paper);
    const int luminance = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
    return luminance > 155 ? QColor("#38252d") : QColor("#fff8f1");
}

QString firstAvailableFont(const QStringList &families)
{
    const QStringList installed = QFontDatabase::families();
    for (const QString &family : families) {
        if (installed.contains(family, Qt::CaseInsensitive)) {
            return family;
        }
    }
    return {};
}

QFont noteFont(AppSettings::NoteFont profile, int pointSize, QFont::Weight weight = QFont::Normal)
{
    QString family;
    switch (profile) {
    case AppSettings::NoteFont::Playful: family = firstAvailableFont({"Comic Sans MS", "Segoe Print", "Sans Serif"}); break;
    case AppSettings::NoteFont::Handwritten: family = firstAvailableFont({"Segoe Print", "Bradley Hand ITC", "Comic Sans MS", "Sans Serif"}); break;
    case AppSettings::NoteFont::Rounded: family = firstAvailableFont({"Arial Rounded MT Bold", "Trebuchet MS", "Verdana", "Sans Serif"}); break;
    case AppSettings::NoteFont::Clean: family = firstAvailableFont({"Segoe UI", "Arial", "Sans Serif"}); break;
    case AppSettings::NoteFont::Classic: family = firstAvailableFont({"Georgia", "Times New Roman", "Serif"}); break;
    }
    QFont font;
    if (!family.isEmpty()) font.setFamily(family);
    font.setPointSize(pointSize);
    font.setWeight(weight);
    return font;
}

void drawSoftShadow(QPainter &painter, const QPainterPath &path, qreal horizontalOffset, qreal verticalOffset)
{
    painter.setPen(Qt::NoPen);
    constexpr int ShadowLayers = 7;
    for (int layer = ShadowLayers; layer > 0; --layer) {
        const qreal progress = static_cast<qreal>(layer) / ShadowLayers;
        painter.setBrush(QColor(18, 12, 14, 7 + (ShadowLayers - layer) * 3));
        painter.drawPath(path.translated(horizontalOffset * progress, verticalOffset * progress));
    }
}

class StickyTabButton final : public QPushButton
{
public:
    explicit StickyTabButton(QWidget *parent = nullptr) : QPushButton(parent)
    {
        setFlat(true);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
        m_shadow = new QGraphicsDropShadowEffect(this);
        m_shadow->setBlurRadius(8.0);
        m_shadow->setColor(QColor(15, 10, 12, 120));
        setGraphicsEffect(m_shadow);
    }

    void setNote(const Note &note, AppSettings::Edge edge, AppSettings::NoteFont fontProfile, int visibleStrip)
    {
        m_note = note;
        m_edge = edge;
        m_fontProfile = fontProfile;
        m_visibleStrip = visibleStrip;
        m_shadow->setOffset(edge == AppSettings::Edge::Right ? -3.0 : 3.0, 3.0);
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        const bool onRight = m_edge == AppSettings::Edge::Right;
        const QColor paper(m_note.color.isEmpty() ? AvailableColors.front() : paperColor(m_note.color));
        painter.save();
        if (onRight) {
            painter.translate(width(), 0);
            painter.rotate(-TabLeanDegrees);
            painter.translate(-TabWidth, 0);
        } else {
            painter.rotate(TabLeanDegrees);
        }

        const QRectF card(0, 1, TabWidth, height() - 4);
        QPainterPath path;
        constexpr qreal radius = 11.0;
        if (onRight) {
            path.moveTo(card.left(), card.top() + radius);
            path.quadTo(card.left(), card.top(), card.left() + radius, card.top());
            path.lineTo(card.right(), card.top()); path.lineTo(card.right(), card.bottom());
            path.lineTo(card.left() + radius, card.bottom());
            path.quadTo(card.left(), card.bottom(), card.left(), card.bottom() - radius);
        } else {
            path.moveTo(card.left(), card.top()); path.lineTo(card.right() - radius, card.top());
            path.quadTo(card.right(), card.top(), card.right(), card.top() + radius);
            path.lineTo(card.right(), card.bottom() - radius);
            path.quadTo(card.right(), card.bottom(), card.right() - radius, card.bottom());
            path.lineTo(card.left(), card.bottom());
        }
        path.closeSubpath();
        painter.setPen(Qt::NoPen);
        drawSoftShadow(painter, path, onRight ? -7.0 : 7.0, 7.0);
        painter.setBrush(QColor(30, 19, 22, 42));
        painter.drawPath(path.translated(0, 2.5));
        painter.setBrush(paper);
        painter.drawPath(path);
        painter.setPen(QPen(QColor(53, 35, 39, 30), 0.7));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);

        const int strip = std::max(28, std::min(m_visibleStrip, height() - 8));
        painter.translate(TabWidth / 2.0, strip / 2.0 + 3.0);
        painter.rotate(onRight ? 90.0 : -90.0);
        QFont font = noteFont(m_fontProfile, 10, QFont::Medium);
        font.setPointSizeF(9.5);
        font.setStyleStrategy(QFont::PreferAntialias);
        font.setHintingPreference(QFont::PreferNoHinting);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 0.1);
        painter.setFont(font);
        painter.setPen(inkColor(paper.name()));
        painter.drawText(QRect(-strip / 2 + 5, -TabWidth / 2, strip - 10, TabWidth), Qt::AlignCenter, m_note.title.simplified().toUpper());
        painter.restore();
    }

private:
    Note m_note;
    AppSettings::Edge m_edge = AppSettings::Edge::Right;
    AppSettings::NoteFont m_fontProfile = AppSettings::NoteFont::Playful;
    QGraphicsDropShadowEffect *m_shadow = nullptr;
    int m_visibleStrip = TabPitch;
};
}

EdgeDockWindow::EdgeDockWindow(NoteRepository *repository, AppSettings *settings, QScreen *screen,
                               std::function<void()> allNotesCallback, std::function<void()> settingsCallback,
                               std::function<void()> hideCallback, QWidget *parent)
    : QWidget(parent), m_repository(repository), m_settings(settings), m_screen(screen),
      m_allNotesCallback(std::move(allNotesCallback)), m_settingsCallback(std::move(settingsCallback)),
      m_hideCallback(std::move(hideCallback))
{
    setWindowTitle(QStringLiteral("Floating Notes"));
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_geometryAnimation = new QPropertyAnimation(this, "geometry", this);
    m_geometryAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_geometryAnimation, &QPropertyAnimation::finished, this, &EdgeDockWindow::updateContentVisibility);
    m_collapseTimer = new QTimer(this);
    m_collapseTimer->setSingleShot(true); m_collapseTimer->setInterval(CollapseDelayMs);
    connect(m_collapseTimer, &QTimer::timeout, this, [this] {
        if (!m_keepOpen && !geometry().contains(QCursor::pos())) {
            setExpanded(false);
        }
    });
    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true); m_saveTimer->setInterval(SaveDelayMs);
    connect(m_saveTimer, &QTimer::timeout, this, &EdgeDockWindow::saveCurrentNote);
    m_deleteUndoTimer = new QTimer(this);
    m_deleteUndoTimer->setSingleShot(true); m_deleteUndoTimer->setInterval(UndoDelayMs);
    connect(m_deleteUndoTimer, &QTimer::timeout, this, &EdgeDockWindow::finalizePendingDelete);
    connect(qApp, &QApplication::focusChanged, this, [this](QWidget *, QWidget *now) {
        if (m_expanded && !m_keepOpen && (now == nullptr || (now != this && !isAncestorOf(now))) && !geometry().contains(QCursor::pos())) {
            saveCurrentNote(); scheduleCollapse();
        }
    });
    buildContent();
    applySettings();
    refreshNotes();
    resize(CollapsedWidth, pillHeight());
    positionNearScreenEdge();
}

void EdgeDockWindow::buildContent()
{
    for (int index = 0; index < MaxDeckButtons; ++index) {
        auto *button = new StickyTabButton(this);
        button->hide();
        connect(button, &QPushButton::clicked, this, [this, index] { selectNote(index); });
        m_noteButtons.append(button);
    }
    m_moreButton = new QPushButton(this);
    m_moreButton->setFixedSize(TabWidth, 34); m_moreButton->setCursor(Qt::PointingHandCursor); m_moreButton->hide();
    connect(m_moreButton, &QPushButton::clicked, this, [this] { if (m_allNotesCallback) m_allNotesCallback(); });
    m_createButton = new QPushButton(QStringLiteral("+"), this);
    m_createButton->setFixedSize(PlusButtonSize, PlusButtonSize); m_createButton->setCursor(Qt::PointingHandCursor);
    connect(m_createButton, &QPushButton::clicked, this, &EdgeDockWindow::createNoteAndFocus);

    m_editorPanel = new QWidget(this); m_editorPanel->hide();
    auto *layout = new QVBoxLayout(m_editorPanel);
    layout->setContentsMargins(16, 8, 16, 12); layout->setSpacing(2);
    auto *header = new QHBoxLayout; header->setContentsMargins(0, 0, 0, 0); header->setSpacing(8);
    m_titleEdit = new QLineEdit(m_editorPanel); m_titleEdit->setPlaceholderText(QStringLiteral("New note"));
    connect(m_titleEdit, &QLineEdit::textChanged, this, &EdgeDockWindow::scheduleSave); header->addWidget(m_titleEdit, 1);
    m_statusLabel = new QLabel(m_editorPanel); m_statusLabel->setMinimumWidth(88); m_statusLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter); header->addWidget(m_statusLabel);
    m_pinButton = new QPushButton(QStringLiteral("Pin"), m_editorPanel);
    m_pinButton->setCheckable(true);
    m_pinButton->setToolTip(QStringLiteral("Keep this note open"));
    connect(m_pinButton, &QPushButton::toggled, this, [this](bool pinned) {
        m_keepOpen = pinned;
        if (pinned) {
            m_collapseTimer->stop();
        }
    });
    header->addWidget(m_pinButton);
    m_checklistButton = new QPushButton(QStringLiteral("☷"), m_editorPanel); m_checklistButton->setToolTip(QStringLiteral("Insert checklist item"));
    connect(m_checklistButton, &QPushButton::clicked, this, &EdgeDockWindow::insertChecklistLine); header->addWidget(m_checklistButton);
    m_allNotesButton = new QPushButton(QStringLiteral("⌕"), m_editorPanel); m_allNotesButton->setToolTip(QStringLiteral("All notes"));
    connect(m_allNotesButton, &QPushButton::clicked, this, [this] { if (m_allNotesCallback) m_allNotesCallback(); }); header->addWidget(m_allNotesButton);
    layout->addLayout(header);
    m_tagsEdit = new QLineEdit(m_editorPanel); m_tagsEdit->hide(); connect(m_tagsEdit, &QLineEdit::textChanged, this, &EdgeDockWindow::scheduleSave);
    m_bodyEdit = new QPlainTextEdit(m_editorPanel); m_bodyEdit->setPlaceholderText(QStringLiteral("Write a note...")); m_bodyEdit->setTabChangesFocus(true);
    connect(m_bodyEdit, &QPlainTextEdit::textChanged, this, &EdgeDockWindow::scheduleSave); layout->addWidget(m_bodyEdit, 1);
    auto *footer = new QHBoxLayout; footer->setContentsMargins(0, 0, 0, 0); footer->setSpacing(10);
    for (const QString &color : AvailableColors) {
        auto *button = new QPushButton(m_editorPanel); button->setFixedSize(18, 18); button->setProperty("noteColor", color);
        connect(button, &QPushButton::clicked, this, [this, color] { updateNoteColor(color); }); m_colorButtons.append(button); footer->addWidget(button);
    }
    footer->addStretch();
    m_archiveButton = new QPushButton(QStringLiteral("Archive"), m_editorPanel); connect(m_archiveButton, &QPushButton::clicked, this, &EdgeDockWindow::archiveSelectedNote); footer->addWidget(m_archiveButton);
    m_deleteButton = new QPushButton(QStringLiteral("Delete"), m_editorPanel); connect(m_deleteButton, &QPushButton::clicked, this, &EdgeDockWindow::deleteCurrentNote); footer->addWidget(m_deleteButton);
    m_hideButton = new QPushButton(QStringLiteral("Close"), m_editorPanel); connect(m_hideButton, &QPushButton::clicked, this, [this] { setExpanded(false); }); footer->addWidget(m_hideButton);
    layout->addLayout(footer);
    m_undoButton = new QPushButton(QStringLiteral("Undo delete"), m_editorPanel); m_undoButton->hide(); connect(m_undoButton, &QPushButton::clicked, this, &EdgeDockWindow::undoDelete);
}

void EdgeDockWindow::applySettings()
{
    m_geometryAnimation->setDuration(m_settings->animationDurationMs());
    m_titleEdit->setFont(noteFont(m_settings->noteFont(), 13, QFont::DemiBold));
    m_tagsEdit->setFont(noteFont(m_settings->noteFont(), 9));
    m_bodyEdit->setFont(noteFont(m_settings->noteFont(), 14));
    updateNoteButtons(); layoutDeck(); positionNearScreenEdge();
}

void EdgeDockWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate && m_expanded && !m_keepOpen && !geometry().contains(QCursor::pos())) { saveCurrentNote(); scheduleCollapse(); }
    QWidget::changeEvent(event);
}

void EdgeDockWindow::refreshNotes()
{
    saveCurrentNote();
    const int selectedId = m_currentNoteIndex >= 0 && m_currentNoteIndex < m_notes.size() ? m_notes.at(m_currentNoteIndex).id : 0;
    QString errorMessage; m_notes = m_repository->loadNotes(NoteRepository::NoteListFilter::Active, {}, &errorMessage);
    if (!errorMessage.isEmpty()) setStatusText(QStringLiteral("Load error: %1").arg(errorMessage));
    if (m_editorVisible) {
        const auto selected = std::find_if(m_notes.cbegin(), m_notes.cend(), [selectedId](const Note &note) { return note.id == selectedId; });
        m_currentNoteIndex = selected == m_notes.cend() ? -1 : static_cast<int>(std::distance(m_notes.cbegin(), selected));
    }
    updateEditorFromCurrentNote(); updateNoteButtons(); layoutDeck(); positionNearScreenEdge();
}

void EdgeDockWindow::enterEvent(QEnterEvent *event) { m_collapseTimer->stop(); setExpanded(true); QWidget::enterEvent(event); }
void EdgeDockWindow::keyPressEvent(QKeyEvent *event) { if (event->key() == Qt::Key_Escape) { setExpanded(false); return; } QWidget::keyPressEvent(event); }
void EdgeDockWindow::leaveEvent(QEvent *event)
{
    QWidget *focus = QApplication::focusWidget();
    if (!m_keepOpen && (focus == nullptr || (focus != this && !isAncestorOf(focus)))) scheduleCollapse();
    QWidget::leaveEvent(event);
}
void EdgeDockWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) { setExpanded(true); event->accept(); return; }
    QWidget::mousePressEvent(event);
}

void EdgeDockWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    if (!m_expanded) {
        painter.setPen(Qt::NoPen); painter.setBrush(QColor(38, 29, 31, 220)); painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 6, 6);
        const int shown = std::max(1, std::min(static_cast<int>(m_notes.size()), MaxPillDashes));
        for (int index = 0, y = 7; index < shown; ++index, y += 19) {
            const QString paper = m_notes.isEmpty() ? AvailableColors.front() : paperColor(m_notes.at(index).color);
            painter.setBrush(QColor(dashColorForPaper(paper))); painter.drawRoundedRect(QRectF(3.5, y, 7, 14), 2.5, 2.5);
        }
        return;
    }
    if (!m_editorVisible || m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) {
        painter.setPen(QPen(QColor(255, 255, 255, 105), 1, Qt::DashLine));
        const int lineX = m_settings->preferredEdge() == AppSettings::Edge::Right ? width() - 4 : 3; painter.drawLine(lineX, 5, lineX, height() - 5); return;
    }
    const Note &note = m_notes.at(m_currentNoteIndex); const QString paperName = paperColor(note.color); const QColor paper(paperName); const QColor dash(dashColorForPaper(paperName)); const bool onRight = m_settings->preferredEdge() == AppSettings::Edge::Right;
    const QRectF sheet = onRight ? QRectF(EditorGutterWidth, 0, width() - EditorGutterWidth, height() - 1) : QRectF(0, 0, width() - EditorGutterWidth, height() - 1);
    QPainterPath path; constexpr qreal radius = 14.0;
    if (onRight) {
        path.moveTo(sheet.left() + radius, sheet.top()); path.lineTo(sheet.right(), sheet.top()); path.lineTo(sheet.right(), sheet.bottom()); path.lineTo(sheet.left() + radius, sheet.bottom());
        path.quadTo(sheet.left(), sheet.bottom(), sheet.left(), sheet.bottom() - radius); path.lineTo(sheet.left(), sheet.top() + radius); path.quadTo(sheet.left(), sheet.top(), sheet.left() + radius, sheet.top());
    } else {
        path.moveTo(sheet.left(), sheet.top()); path.lineTo(sheet.right() - radius, sheet.top()); path.quadTo(sheet.right(), sheet.top(), sheet.right(), sheet.top() + radius);
        path.lineTo(sheet.right(), sheet.bottom() - radius); path.quadTo(sheet.right(), sheet.bottom(), sheet.right() - radius, sheet.bottom()); path.lineTo(sheet.left(), sheet.bottom());
    }
    path.closeSubpath(); painter.setPen(Qt::NoPen); drawSoftShadow(painter, path, onRight ? -12.0 : 12.0, 13.0); painter.setBrush(paper); painter.drawPath(path);
    const QRect gutter = onRight ? QRect(0, 0, EditorGutterWidth, height()) : QRect(width() - EditorGutterWidth, 0, EditorGutterWidth, height()); painter.fillRect(gutter, QColor(dash.red(), dash.green(), dash.blue(), 42));
    const int lineX = onRight ? EditorGutterWidth : width() - EditorGutterWidth - 1; const QColor ink = inkColor(paperName); painter.setPen(QPen(QColor(ink.red(), ink.green(), ink.blue(), 72), 1, Qt::DashLine)); painter.drawLine(lineX, 0, lineX, height());
    painter.save(); painter.translate(gutter.center().x(), gutter.center().y()); painter.rotate(onRight ? 90 : -90); painter.setFont(noteFont(m_settings->noteFont(), 10, QFont::DemiBold)); painter.setPen(QColor(ink.red(), ink.green(), ink.blue(), 170)); painter.drawText(QRect(-height() / 2 + 16, -EditorGutterWidth / 2, height() - 32, EditorGutterWidth), Qt::AlignCenter, note.title.simplified().toUpper()); painter.restore();
}

void EdgeDockWindow::resizeEvent(QResizeEvent *event) { layoutDeck(); layoutEditor(); updateContentVisibility(); QWidget::resizeEvent(event); }
void EdgeDockWindow::positionNearScreenEdge() { setGeometry(targetGeometryForWidth(width())); }
QRect EdgeDockWindow::targetGeometryForWidth(int targetWidth) const
{
    const QScreen *screen = m_screen != nullptr ? m_screen.data() : QGuiApplication::primaryScreen(); const int height = deckHeight();
    if (screen == nullptr) return QRect(pos(), QSize(targetWidth, height));
    const QRect available = screen->availableGeometry(); const int y = available.y() + (available.height() - height) / 2;
    const int x = m_settings->preferredEdge() == AppSettings::Edge::Right ? available.right() - targetWidth + 1 : available.x(); return QRect(x, y, targetWidth, height);
}

void EdgeDockWindow::selectNote(int index)
{
    if (index < 0 || index >= m_notes.size()) return;
    saveCurrentNote(); m_currentNoteIndex = index; setEditorVisible(true); updateEditorFromCurrentNote(); updateNoteButtons(); updateColorButtons();
}
void EdgeDockWindow::createNoteAndFocus()
{
    saveCurrentNote(); Note note; QString errorMessage;
    if (!m_repository->createNote(&note, &errorMessage)) { setStatusText(QStringLiteral("Create error: %1").arg(errorMessage)); return; }
    note.color = AvailableColors.front(); m_repository->updateNoteColor(note.id, note.color, nullptr); m_notes.append(note); m_currentNoteIndex = m_notes.size() - 1;
    setExpanded(true); setEditorVisible(true); updateEditorFromCurrentNote(); m_titleEdit->setFocus(Qt::MouseFocusReason); m_titleEdit->selectAll();
}
void EdgeDockWindow::archiveSelectedNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) return;
    saveCurrentNote(); QString errorMessage;
    if (!m_repository->archiveNote(m_notes.at(m_currentNoteIndex).id, &errorMessage)) { setStatusText(QStringLiteral("Archive error: %1").arg(errorMessage)); return; }
    m_notes.removeAt(m_currentNoteIndex); m_currentNoteIndex = -1; setExpanded(false); updateNoteButtons();
}
void EdgeDockWindow::deleteCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) return;
    finalizePendingDelete(); saveCurrentNote(); const int index = m_currentNoteIndex; m_pendingDeleteNoteId = m_notes.at(index).id; QString errorMessage;
    if (!m_repository->markNoteDeleted(m_pendingDeleteNoteId, &errorMessage)) { setStatusText(QStringLiteral("Delete error: %1").arg(errorMessage)); m_pendingDeleteNoteId = 0; return; }
    m_notes.removeAt(index); m_currentNoteIndex = -1; m_undoButton->show(); m_deleteUndoTimer->start(); setExpanded(false); updateNoteButtons();
}
void EdgeDockWindow::undoDelete()
{
    if (m_pendingDeleteNoteId == 0) return;
    const int id = m_pendingDeleteNoteId; m_pendingDeleteNoteId = 0; m_deleteUndoTimer->stop(); m_undoButton->hide(); QString errorMessage;
    if (!m_repository->restoreDeletedNote(id, &errorMessage)) { setStatusText(QStringLiteral("Undo error: %1").arg(errorMessage)); return; } refreshNotes();
}
void EdgeDockWindow::finalizePendingDelete()
{
    if (m_pendingDeleteNoteId == 0) return;
    const int id = m_pendingDeleteNoteId; m_pendingDeleteNoteId = 0; m_deleteUndoTimer->stop(); m_undoButton->hide(); QString errorMessage; m_repository->deleteNotePermanently(id, &errorMessage);
}
void EdgeDockWindow::insertChecklistLine()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) return;
    QTextCursor cursor = m_bodyEdit->textCursor(); cursor.insertText(QStringLiteral("- [ ] ")); m_bodyEdit->setTextCursor(cursor); m_bodyEdit->setFocus(Qt::ShortcutFocusReason); scheduleSave();
}
void EdgeDockWindow::scheduleSave()
{
    if (!m_loadingEditor && m_currentNoteIndex >= 0 && m_currentNoteIndex < m_notes.size()) { m_saveTimer->start(); setStatusText(QStringLiteral("Editing...")); }
}
void EdgeDockWindow::saveCurrentNoteNow() { saveCurrentNote(); }
void EdgeDockWindow::saveCurrentNote()
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) return;
    Note &note = m_notes[m_currentNoteIndex]; note.title = m_titleEdit->text().trimmed(); if (note.title.isEmpty()) note.title = QStringLiteral("Untitled"); note.tags = m_tagsEdit->text().trimmed(); note.body = m_bodyEdit->toPlainText();
    QString errorMessage;
    if (m_repository->saveNote(note, &errorMessage)) { setStatusText(QStringLiteral("Saved · just now")); updateNoteButtons(); } else setStatusText(QStringLiteral("Save error: %1").arg(errorMessage));
}
void EdgeDockWindow::scheduleCollapse() { if (m_expanded && !m_keepOpen) m_collapseTimer->start(); }
void EdgeDockWindow::setExpanded(bool expanded)
{
    if (m_expanded == expanded) return;
    m_expanded = expanded;
    if (!expanded) {
        saveCurrentNote();
        m_keepOpen = false;
        m_pinButton->setChecked(false);
        setEditorVisible(false);
    } else m_collapseTimer->stop();
    m_geometryAnimation->stop(); m_geometryAnimation->setEasingCurve(expanded ? QEasingCurve::OutCubic : QEasingCurve::InCubic); m_geometryAnimation->setStartValue(geometry()); m_geometryAnimation->setEndValue(targetGeometryForWidth(expanded ? expandedWidth() : CollapsedWidth)); m_geometryAnimation->start(); updateContentVisibility(); layoutDeck(); update();
}
void EdgeDockWindow::setEditorVisible(bool visible)
{
    m_editorVisible = visible; if (!visible) m_currentNoteIndex = -1;
    if (m_expanded) { m_geometryAnimation->stop(); m_geometryAnimation->setStartValue(geometry()); m_geometryAnimation->setEndValue(targetGeometryForWidth(expandedWidth())); m_geometryAnimation->start(); }
    layoutDeck(); layoutEditor(); updateContentVisibility(); update();
}
void EdgeDockWindow::setStatusText(const QString &text) { m_statusLabel->setText(text); }
int EdgeDockWindow::pillHeight() const { const int shown = std::max(1, std::min(static_cast<int>(m_notes.size()), MaxPillDashes)); return 14 + shown * 14 + (shown - 1) * 5; }
int EdgeDockWindow::fanHeight() const
{
    const int count = visibleNoteCount();
    const int tabStackHeight = count == 0
        ? TabHeight
        : (count - 1) * TabPitch + TabHeight;
    const int moreHeight = m_notes.size() > count ? 41 : 0;
    return tabStackHeight + 7 + moreHeight + 5 + PlusButtonSize + 4;
}
int EdgeDockWindow::deckHeight() const { if (!m_expanded) return pillHeight(); return m_editorVisible ? EditorHeight : fanHeight(); }
int EdgeDockWindow::expandedWidth() const { return m_editorVisible ? EditorWidth + EditorGutterWidth : FanWidth; }
int EdgeDockWindow::visibleNoteCount() const { return std::min(static_cast<int>(m_notes.size()), std::clamp(m_settings->maxVisibleNotes(), 1, MaxDeckButtons)); }
void EdgeDockWindow::updateColorButtons()
{
    const QString selected = m_currentNoteIndex >= 0 && m_currentNoteIndex < m_notes.size() ? m_notes.at(m_currentNoteIndex).color.toLower() : QString();
    for (QPushButton *button : m_colorButtons) {
        const QString color = button->property("noteColor").toString();
        button->setStyleSheet(QStringLiteral("QPushButton { background: %1; border: %2px solid %3; border-radius: 9px; padding: 0; } QPushButton:hover { border-color: #38252d; }").arg(color).arg(color == selected ? 3 : 0).arg(color == selected ? QStringLiteral("#6d3d55") : QStringLiteral("transparent")));
    }
}
void EdgeDockWindow::updateContentVisibility()
{
    const bool showFan = m_expanded && !m_editorVisible;
    for (QPushButton *button : m_noteButtons) button->setVisible(showFan && button->property("hasNote").toBool());
    m_moreButton->setVisible(showFan && m_moreButton->property("hasMore").toBool()); m_createButton->setVisible(showFan); m_editorPanel->setVisible(m_expanded && m_editorVisible);
}
void EdgeDockWindow::updateEditorFromCurrentNote()
{
    m_loadingEditor = true; const bool hasNote = m_currentNoteIndex >= 0 && m_currentNoteIndex < m_notes.size();
    for (QWidget *widget : {static_cast<QWidget *>(m_titleEdit), static_cast<QWidget *>(m_bodyEdit), static_cast<QWidget *>(m_archiveButton), static_cast<QWidget *>(m_deleteButton), static_cast<QWidget *>(m_checklistButton), static_cast<QWidget *>(m_pinButton)}) widget->setEnabled(hasNote);
    for (QPushButton *button : m_colorButtons) button->setEnabled(hasNote);
    if (hasNote) {
        const Note &note = m_notes.at(m_currentNoteIndex); m_titleEdit->setText(note.title); m_tagsEdit->setText(note.tags); m_bodyEdit->setPlainText(note.body); const QColor ink = inkColor(paperColor(note.color));
        m_editorPanel->setStyleSheet(QStringLiteral("QLineEdit, QPlainTextEdit { background: transparent; border: 0; color: %1; selection-background-color: rgba(255,255,255,120); } QLineEdit { padding: 2px 0; } QPlainTextEdit { padding: 9px 8px; } QLabel { color: rgba(%2,%3,%4,130); font-size: 10px; } QPushButton { background: rgba(61,36,47,22); border: 0; border-radius: 6px; color: %1; padding: 3px 9px; font-size: 11px; } QPushButton:hover { background: rgba(61,36,47,42); }").arg(ink.name()).arg(ink.red()).arg(ink.green()).arg(ink.blue()));
        setStatusText(QStringLiteral("Saved · just now"));
    } else { m_titleEdit->clear(); m_tagsEdit->clear(); m_bodyEdit->clear(); }
    m_loadingEditor = false;
}
void EdgeDockWindow::updateNoteButtons()
{
    const int limit = visibleNoteCount();
    for (int index = 0; index < m_noteButtons.size(); ++index) {
        QPushButton *button = m_noteButtons.at(index); const bool hasNote = index < limit && index < m_notes.size(); button->setProperty("hasNote", hasNote); if (!hasNote) continue;
        if (auto *tab = dynamic_cast<StickyTabButton *>(button)) tab->setNote(m_notes.at(index), m_settings->preferredEdge(), m_settings->noteFont(), index == limit - 1 ? TabHeight : TabPitch);
        button->setToolTip(m_notes.at(index).title);
    }
    const int hidden = std::max(0, static_cast<int>(m_notes.size()) - limit); m_moreButton->setProperty("hasMore", hidden > 0); m_moreButton->setText(QStringLiteral("+%1").arg(hidden));
    m_moreButton->setStyleSheet(QStringLiteral("QPushButton { background: rgba(48,31,31,180); color: #f6d8cf; border: 0; border-radius: 10px; font-weight: 600; } QPushButton:hover { background: rgba(48,31,31,225); }"));
    m_createButton->setStyleSheet(QStringLiteral("QPushButton { background: rgba(48,31,31,190); color: #f4d4c5; border: 0; border-radius: 14px; font-size: 18px; font-weight: 600; padding-bottom: 2px; } QPushButton:hover { background: rgba(48,31,31,230); }"));
    updateContentVisibility();
}
void EdgeDockWindow::layoutDeck()
{
    if (!m_expanded || m_editorVisible) return;
    const bool onRight = m_settings->preferredEdge() == AppSettings::Edge::Right; const int x = onRight ? width() - TabWidth : 0; const int count = visibleNoteCount();
    for (int index = 0; index < count; ++index) m_noteButtons.at(index)->setGeometry(0, index * TabPitch, FanWidth, TabHeight);
    int y = count == 0 ? 0 : (count - 1) * TabPitch + TabHeight + 7;
    if (m_notes.size() > count) { m_moreButton->move(x, y); y += 41; }
    m_createButton->move(x + (TabWidth - PlusButtonSize) / 2, y + 5);
}
void EdgeDockWindow::layoutEditor()
{
    if (!m_editorVisible) return;
    m_editorPanel->setGeometry(m_settings->preferredEdge() == AppSettings::Edge::Right ? EditorGutterWidth : 0, 0, EditorWidth, EditorHeight);
}
void EdgeDockWindow::updateNoteColor(const QString &color)
{
    if (m_currentNoteIndex < 0 || m_currentNoteIndex >= m_notes.size()) return;
    Note &note = m_notes[m_currentNoteIndex]; note.color = color; QString errorMessage;
    if (m_repository->updateNoteColor(note.id, color, &errorMessage)) { updateEditorFromCurrentNote(); updateNoteButtons(); updateColorButtons(); update(); } else setStatusText(QStringLiteral("Color error: %1").arg(errorMessage));
}
