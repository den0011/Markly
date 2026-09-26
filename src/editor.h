#ifndef EDITOR_H
#define EDITOR_H

#include <QPlainTextEdit>

class MarkdownHighlighter;

// Plain text editor with a line number gutter, current line highlighting and
// the small Markdown conveniences (list continuation, wrapping a selection in
// markers) that the main window's Format menu drives.
class Editor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void paintLineNumbers(QPaintEvent *event);

    // Wraps the selection (or the word under the cursor) in `prefix`/`suffix`.
    // Toggles the markers off when they are already there.
    void surroundSelection(const QString &prefix, const QString &suffix);

    // Prefixes every line touched by the selection with `marker`, toggling it.
    void prefixLines(const QString &marker);

    // Replaces the selection with `markdown`.
    void insertSnippet(const QString &markdown);

    // Same, but first moves to the start of a line and leaves a blank line
    // above: block constructs like tables are only recognised there, and one
    // glued to the paragraph above is swallowed by it.
    void insertBlockSnippet(const QString &markdown);

    void setShowLineNumbers(bool show);
    bool showLineNumbers() const { return m_showLineNumbers; }

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    QWidget *m_lineNumberArea;
    MarkdownHighlighter *m_highlighter;
    bool m_showLineNumbers = true;
};

#endif // EDITOR_H
