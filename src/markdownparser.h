#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include <QString>
#include <QStringList>

// Converts Markdown into the HTML subset understood by Qt's rich text engine
// (QTextDocument / QTextBrowser). Qt 5.13 has no QTextDocument::setMarkdown and
// QtWebEngine is unavailable on MinGW, so the conversion is done here.
//
// Supported: ATX and setext headings, fenced and indented code, blockquotes,
// ordered/unordered/task lists, GFM tables, thematic breaks, links, autolinks,
// images, emphasis, strong, strikethrough, inline code, hard line breaks and
// backslash escapes. Raw inline HTML is escaped rather than passed through.
class MarkdownParser
{
public:
    // Returns the <body> content only; wrap it yourself if you need a document.
    QString toHtml(const QString &markdown) const;

    // Full standalone document, with `css` inlined into a <style> block.
    QString toHtmlDocument(const QString &markdown,
                           const QString &title,
                           const QString &css) const;

private:
    void parseBlocks(const QStringList &lines, int from, int to, QString *out) const;
    void parseFencedCode(const QStringList &lines, int *i, int to, QString *out) const;
    void parseIndentedCode(const QStringList &lines, int *i, int to, QString *out) const;
    void parseBlockquote(const QStringList &lines, int *i, int to, QString *out) const;
    void parseList(const QStringList &lines, int *i, int to, QString *out) const;
    void parseTable(const QStringList &lines, int *i, int to, QString *out) const;
    void parseParagraph(const QStringList &lines, int *i, int to, QString *out) const;

    QString parseInline(const QString &text) const;

    // Pulls code spans and backslash escapes out of `text` before the rest of
    // the inline rules run, so their content is never re-interpreted.
    QString protectSpans(const QString &text, QStringList *store) const;
    QString restoreSpans(const QString &text, const QStringList &store) const;
};

#endif // MARKDOWNPARSER_H
