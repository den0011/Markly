#include "editor.h"

#include "markdownhighlighter.h"

#include <QAbstractTextDocumentLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QTextBlock>

namespace {

// Gutter widget: it owns no state, it just forwards painting to the editor
// which knows the block geometry.
class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(Editor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
    }

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->paintLineNumbers(event);
    }

private:
    Editor *m_editor;
};

} // namespace

Editor::Editor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
    , m_highlighter(new MarkdownHighlighter(document()))
{
    QFont font(QStringLiteral("Consolas"), 11);
    font.setStyleHint(QFont::Monospace);
    setFont(font);
    setTabStopDistance(4 * fontMetrics().horizontalAdvance(QLatin1Char(' ')));
    setLineWrapMode(QPlainTextEdit::WidgetWidth);
    document()->setDocumentMargin(8);

    connect(this, &Editor::blockCountChanged, this, &Editor::updateLineNumberAreaWidth);
    connect(this, &Editor::updateRequest, this, &Editor::updateLineNumberArea);
    connect(this, &Editor::cursorPositionChanged, this, &Editor::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

int Editor::lineNumberAreaWidth() const
{
    if (!m_showLineNumbers)
        return 0;

    int digits = 1;
    for (int max = qMax(1, blockCount()); max >= 10; max /= 10)
        ++digits;

    return 12 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void Editor::setShowLineNumbers(bool show)
{
    if (m_showLineNumbers == show)
        return;
    m_showLineNumbers = show;
    m_lineNumberArea->setVisible(show);
    updateLineNumberAreaWidth();
}

void Editor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void Editor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy != 0)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void Editor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect area = contentsRect();
    m_lineNumberArea->setGeometry(
        QRect(area.left(), area.top(), lineNumberAreaWidth(), area.height()));
}

void Editor::paintLineNumbers(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::Window));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = int(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + int(blockBoundingRect(block).height());

    const int currentLine = textCursor().blockNumber();

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.setPen(blockNumber == currentLine
                               ? palette().color(QPalette::Text)
                               : palette().color(QPalette::Mid));
            painter.drawText(0, top, m_lineNumberArea->width() - 6,
                             fontMetrics().height(), Qt::AlignRight,
                             QString::number(blockNumber + 1));
        }

        block = block.next();
        top = bottom;
        bottom = top + int(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void Editor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(palette().color(QPalette::AlternateBase));
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        selections.append(selection);
    }

    setExtraSelections(selections);
    m_lineNumberArea->update();
}

void Editor::keyPressEvent(QKeyEvent *event)
{
    // Continue list items on Enter; pressing Enter on an empty item ends the
    // list instead of adding another empty bullet.
    if (event->key() == Qt::Key_Return && !(event->modifiers() & Qt::ShiftModifier)) {
        static const QRegularExpression item(
            QStringLiteral("^([ \t]*)([-*+]|\\d{1,9}[.)])[ \t]+(\\[[ xX]\\][ \t]+)?"));

        QTextCursor cursor = textCursor();
        const QString line = cursor.block().text();
        const QRegularExpressionMatch match = item.match(line);

        if (match.hasMatch() && !cursor.hasSelection()) {
            if (line.mid(match.capturedLength()).trimmed().isEmpty()) {
                cursor.select(QTextCursor::BlockUnderCursor);
                cursor.insertText(QStringLiteral("\n") + match.captured(1));
                setTextCursor(cursor);
                return;
            }

            QString marker = match.captured(2);
            bool ordered = false;
            const int number = marker.leftRef(marker.size() - 1).toInt(&ordered);
            if (ordered)
                marker = QString::number(number + 1) + marker.right(1);

            const QString task = match.captured(3).isEmpty()
                ? QString()
                : QStringLiteral("[ ] ");

            QPlainTextEdit::keyPressEvent(event);
            insertPlainText(match.captured(1) + marker + QLatin1Char(' ') + task);
            return;
        }
    }

    QPlainTextEdit::keyPressEvent(event);
}

void Editor::surroundSelection(const QString &prefix, const QString &suffix)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);

    const QString selected = cursor.selectedText();

    cursor.beginEditBlock();
    if (selected.startsWith(prefix) && selected.endsWith(suffix)
        && selected.size() >= prefix.size() + suffix.size()) {
        cursor.insertText(selected.mid(prefix.size(), selected.size() - prefix.size() - suffix.size()));
    } else {
        cursor.insertText(prefix + selected + suffix);
        if (selected.isEmpty()) {
            // Park the cursor between the markers so typing continues inside.
            cursor.setPosition(cursor.position() - suffix.size());
        }
    }
    cursor.endEditBlock();

    setTextCursor(cursor);
    setFocus();
}

void Editor::prefixLines(const QString &marker)
{
    QTextCursor cursor = textCursor();
    const int start = cursor.selectionStart();
    const int end = cursor.selectionEnd();

    cursor.setPosition(start);
    const int firstBlock = cursor.blockNumber();
    cursor.setPosition(end);
    const int lastBlock = cursor.blockNumber();

    // Toggle off only when every touched line already carries the marker.
    bool allPrefixed = true;
    for (int i = firstBlock; i <= lastBlock; ++i) {
        if (!document()->findBlockByNumber(i).text().startsWith(marker)) {
            allPrefixed = false;
            break;
        }
    }

    cursor.beginEditBlock();
    for (int i = firstBlock; i <= lastBlock; ++i) {
        QTextCursor lineCursor(document()->findBlockByNumber(i));
        lineCursor.movePosition(QTextCursor::StartOfBlock);
        if (allPrefixed) {
            lineCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, marker.size());
            lineCursor.removeSelectedText();
        } else {
            lineCursor.insertText(marker);
        }
    }
    cursor.endEditBlock();

    setFocus();
}

void Editor::insertSnippet(const QString &markdown)
{
    QTextCursor cursor = textCursor();
    cursor.insertText(markdown);
    setTextCursor(cursor);
    setFocus();
}

void Editor::insertBlockSnippet(const QString &markdown)
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    if (!cursor.atBlockStart())
        cursor.insertText(QStringLiteral("\n"));
    if (cursor.blockNumber() > 0 && !cursor.block().previous().text().trimmed().isEmpty())
        cursor.insertText(QStringLiteral("\n"));

    cursor.insertText(markdown);
    cursor.endEditBlock();

    setTextCursor(cursor);
    setFocus();
}
