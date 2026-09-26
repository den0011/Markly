#ifndef MARKDOWNHIGHLIGHTER_H
#define MARKDOWNHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QVector>

// Lightweight Markdown highlighting for the source pane. It deliberately does
// not share code with MarkdownParser: the parser needs a correct block
// structure, the highlighter only needs per-line hints that are cheap enough to
// run on every keystroke.
class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct Rule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
        int captureGroup = 0;
    };

    QVector<Rule> m_rules;
    QTextCharFormat m_codeBlockFormat;
    QRegularExpression m_fence;
};

#endif // MARKDOWNHIGHLIGHTER_H
