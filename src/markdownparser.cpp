#include "markdownparser.h"

#include <QRegularExpression>
#include <QSet>
#include <QVector>

namespace {

// Placeholders for protected spans. Both characters are control codes that
// cannot legitimately appear in a Markdown source file.
const QChar kMarkStart = QChar(0x0001);
const QChar kMarkEnd   = QChar(0x0002);

const char *kEscapable = "\\`*_{}[]()#+-.!|~<>\"'";

QString escapeHtml(const QString &in)
{
    QString out;
    out.reserve(in.size());
    for (int i = 0; i < in.size(); ++i) {
        const QChar c = in.at(i);
        if (c == QLatin1Char('&'))
            out += QLatin1String("&amp;");
        else if (c == QLatin1Char('<'))
            out += QLatin1String("&lt;");
        else if (c == QLatin1Char('>'))
            out += QLatin1String("&gt;");
        else
            out += c;
    }
    return out;
}

bool isBlank(const QString &line)
{
    return line.trimmed().isEmpty();
}

// Inline HTML that is handed to Qt's rich text engine unchanged instead of
// being escaped. The list is deliberately a whitelist of formatting tags that
// Qt actually renders: anything else — script, style, iframe, block level
// markup Qt cannot lay out — stays visible as literal text rather than being
// silently dropped.
bool isPassThroughTag(const QString &name)
{
    static const QSet<QString> allowed = {
        QStringLiteral("a"),     QStringLiteral("abbr"),   QStringLiteral("b"),
        QStringLiteral("big"),   QStringLiteral("br"),     QStringLiteral("cite"),
        QStringLiteral("code"),  QStringLiteral("del"),    QStringLiteral("em"),
        QStringLiteral("i"),     QStringLiteral("img"),    QStringLiteral("ins"),
        QStringLiteral("kbd"),   QStringLiteral("mark"),   QStringLiteral("q"),
        QStringLiteral("s"),     QStringLiteral("samp"),   QStringLiteral("small"),
        QStringLiteral("span"),  QStringLiteral("strike"), QStringLiteral("strong"),
        QStringLiteral("sub"),   QStringLiteral("sup"),    QStringLiteral("tt"),
        QStringLiteral("u"),     QStringLiteral("var"),
    };

    return allowed.contains(name);
}

// Visual indentation in columns, with tabs expanded to the next multiple of 4.
int indentWidth(const QString &line, int *firstNonSpace = nullptr)
{
    int width = 0;
    int i = 0;
    for (; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == QLatin1Char(' '))
            width += 1;
        else if (c == QLatin1Char('\t'))
            width += 4 - (width % 4);
        else
            break;
    }
    if (firstNonSpace)
        *firstNonSpace = i;
    return width;
}

// Drops up to `columns` of leading whitespace, keeping the remainder intact.
QString stripIndent(const QString &line, int columns)
{
    int width = 0;
    int i = 0;
    while (i < line.size() && width < columns) {
        const QChar c = line.at(i);
        if (c == QLatin1Char(' '))
            width += 1;
        else if (c == QLatin1Char('\t'))
            width += 4 - (width % 4);
        else
            break;
        ++i;
    }
    return line.mid(i);
}

const QRegularExpression &reThematicBreak()
{
    static const QRegularExpression re(
        QStringLiteral("^ {0,3}(?:(?:\\*[ \t]*){3,}|(?:-[ \t]*){3,}|(?:_[ \t]*){3,})$"));
    return re;
}

const QRegularExpression &reAtxHeading()
{
    static const QRegularExpression re(
        QStringLiteral("^ {0,3}(#{1,6})(?:[ \t]+(.*?))?[ \t]*#*[ \t]*$"));
    return re;
}

const QRegularExpression &reFence()
{
    static const QRegularExpression re(
        QStringLiteral("^( {0,3})(`{3,}|~{3,})[ \t]*([^`]*?)[ \t]*$"));
    return re;
}

const QRegularExpression &reBlockquote()
{
    static const QRegularExpression re(QStringLiteral("^ {0,3}>"));
    return re;
}

const QRegularExpression &reBulletItem()
{
    static const QRegularExpression re(
        QStringLiteral("^( {0,3})([-*+])([ \t]+(.*)|[ \t]*)$"));
    return re;
}

const QRegularExpression &reOrderedItem()
{
    static const QRegularExpression re(
        QStringLiteral("^( {0,3})(\\d{1,9})([.)])([ \t]+(.*)|[ \t]*)$"));
    return re;
}

const QRegularExpression &reSetextH1()
{
    static const QRegularExpression re(QStringLiteral("^ {0,3}=+[ \t]*$"));
    return re;
}

const QRegularExpression &reSetextH2()
{
    static const QRegularExpression re(QStringLiteral("^ {0,3}-+[ \t]*$"));
    return re;
}

const QRegularExpression &reTableDelimiter()
{
    static const QRegularExpression re(
        QStringLiteral("^ {0,3}\\|?[ \t]*:?-+:?[ \t]*(\\|[ \t]*:?-+:?[ \t]*)*\\|?[ \t]*$"));
    return re;
}

// True when `line` starts some block that a paragraph cannot swallow.
bool startsNewBlock(const QString &line)
{
    return reThematicBreak().match(line).hasMatch()
        || reAtxHeading().match(line).hasMatch()
        || reFence().match(line).hasMatch()
        || reBlockquote().match(line).hasMatch()
        || reBulletItem().match(line).hasMatch()
        || reOrderedItem().match(line).hasMatch();
}

// Splits a table row on unescaped pipes, dropping the optional outer ones.
QStringList splitTableRow(const QString &line)
{
    QString row = line.trimmed();
    if (row.startsWith(QLatin1Char('|')))
        row.remove(0, 1);
    if (row.endsWith(QLatin1Char('|')) && !row.endsWith(QLatin1String("\\|")))
        row.chop(1);

    QStringList cells;
    QString current;
    for (int i = 0; i < row.size(); ++i) {
        const QChar c = row.at(i);
        if (c == QLatin1Char('\\') && i + 1 < row.size() && row.at(i + 1) == QLatin1Char('|')) {
            current += QLatin1Char('|');
            ++i;
        } else if (c == QLatin1Char('|')) {
            cells << current.trimmed();
            current.clear();
        } else {
            current += c;
        }
    }
    cells << current.trimmed();
    return cells;
}

} // namespace

QString MarkdownParser::toHtml(const QString &markdown) const
{
    static const QRegularExpression lineBreak(QStringLiteral("\r\n|\n|\r"));
    const QStringList lines = markdown.split(lineBreak);

    QString body;
    parseBlocks(lines, 0, lines.size(), &body);
    return body;
}

QString MarkdownParser::toHtmlDocument(const QString &markdown,
                                       const QString &title,
                                       const QString &css) const
{
    return QStringLiteral(
               "<!DOCTYPE html>\n<html>\n<head>\n"
               "<meta charset=\"utf-8\" />\n"
               "<title>%1</title>\n"
               "<style type=\"text/css\">\n%2</style>\n"
               "</head>\n<body>\n%3</body>\n</html>\n")
        .arg(escapeHtml(title), css, toHtml(markdown));
}

void MarkdownParser::parseBlocks(const QStringList &lines, int from, int to, QString *out) const
{
    int i = from;
    while (i < to) {
        const QString &line = lines.at(i);

        if (isBlank(line)) {
            ++i;
            continue;
        }

        if (reFence().match(line).hasMatch()) {
            parseFencedCode(lines, &i, to, out);
            continue;
        }

        if (reThematicBreak().match(line).hasMatch()) {
            *out += QLatin1String("<hr />\n");
            ++i;
            continue;
        }

        const QRegularExpressionMatch heading = reAtxHeading().match(line);
        if (heading.hasMatch()) {
            const int level = heading.captured(1).size();
            const QString text = parseInline(heading.captured(2));
            *out += QStringLiteral("<h%1>%2</h%1>\n").arg(level).arg(text);
            ++i;
            continue;
        }

        if (reBlockquote().match(line).hasMatch()) {
            parseBlockquote(lines, &i, to, out);
            continue;
        }

        if (reBulletItem().match(line).hasMatch() || reOrderedItem().match(line).hasMatch()) {
            parseList(lines, &i, to, out);
            continue;
        }

        if (line.contains(QLatin1Char('|')) && i + 1 < to
            && reTableDelimiter().match(lines.at(i + 1)).hasMatch()) {
            parseTable(lines, &i, to, out);
            continue;
        }

        if (indentWidth(line) >= 4) {
            parseIndentedCode(lines, &i, to, out);
            continue;
        }

        parseParagraph(lines, &i, to, out);
    }
}

void MarkdownParser::parseFencedCode(const QStringList &lines, int *i, int to, QString *out) const
{
    const QRegularExpressionMatch open = reFence().match(lines.at(*i));
    const int baseIndent = open.captured(1).size();
    const QString fence = open.captured(2);
    const QChar fenceChar = fence.at(0);
    const QString language = open.captured(3).section(QLatin1Char(' '), 0, 0);

    QStringList code;
    int j = *i + 1;
    for (; j < to; ++j) {
        const QRegularExpressionMatch close = reFence().match(lines.at(j));
        if (close.hasMatch() && close.captured(2).at(0) == fenceChar
            && close.captured(2).size() >= fence.size() && close.captured(3).isEmpty()) {
            ++j;
            break;
        }
        code << stripIndent(lines.at(j), baseIndent);
    }
    *i = j;

    const QString classAttr = language.isEmpty()
        ? QString()
        : QStringLiteral(" class=\"language-%1\"").arg(escapeHtml(language));
    *out += QStringLiteral("<pre><code%1>%2</code></pre>\n")
                .arg(classAttr, escapeHtml(code.join(QLatin1Char('\n'))));
}

void MarkdownParser::parseIndentedCode(const QStringList &lines, int *i, int to, QString *out) const
{
    // Blank lines belong to the block only when more indented code follows, so
    // collect them speculatively and drop whatever trails at the end.
    QStringList code;
    int j = *i;
    int lastContent = *i - 1;
    for (; j < to; ++j) {
        const QString &line = lines.at(j);
        if (isBlank(line)) {
            code << QString();
            continue;
        }
        if (indentWidth(line) < 4)
            break;
        code << stripIndent(line, 4);
        lastContent = j;
    }
    *i = lastContent + 1;

    while (!code.isEmpty() && code.last().isEmpty())
        code.removeLast();

    *out += QStringLiteral("<pre><code>%1</code></pre>\n")
                .arg(escapeHtml(code.join(QLatin1Char('\n'))));
}

void MarkdownParser::parseBlockquote(const QStringList &lines, int *i, int to, QString *out) const
{
    QStringList inner;
    int j = *i;
    for (; j < to; ++j) {
        const QString &line = lines.at(j);
        if (reBlockquote().match(line).hasMatch()) {
            QString stripped = line;
            stripped.remove(0, stripped.indexOf(QLatin1Char('>')) + 1);
            if (stripped.startsWith(QLatin1Char(' ')))
                stripped.remove(0, 1);
            inner << stripped;
        } else if (isBlank(line)) {
            break;
        } else if (startsNewBlock(line)) {
            break;
        } else {
            inner << line; // lazy continuation
        }
    }
    *i = j;

    QString body;
    parseBlocks(inner, 0, inner.size(), &body);
    *out += QStringLiteral("<blockquote>\n%1</blockquote>\n").arg(body);
}

void MarkdownParser::parseList(const QStringList &lines, int *i, int to, QString *out) const
{
    const QRegularExpressionMatch firstBullet = reBulletItem().match(lines.at(*i));
    const bool ordered = !firstBullet.hasMatch();

    QString startNumber;
    if (ordered)
        startNumber = reOrderedItem().match(lines.at(*i)).captured(2);

    QVector<QStringList> items;
    bool loose = false;
    bool pendingBlank = false;
    int contentIndent = 0;

    int j = *i;
    while (j < to) {
        const QString &line = lines.at(j);

        if (isBlank(line)) {
            pendingBlank = true;
            ++j;
            continue;
        }

        const QRegularExpressionMatch bullet = reBulletItem().match(line);
        const QRegularExpressionMatch numbered = reOrderedItem().match(line);
        const bool isItem = ordered ? numbered.hasMatch() : bullet.hasMatch();
        const int lineIndent = indentWidth(line);

        // A new item of this list: same kind, not indented far enough to nest.
        if (isItem && (items.isEmpty() || lineIndent < contentIndent)) {
            if (pendingBlank && !items.isEmpty())
                loose = true;
            pendingBlank = false;

            const QRegularExpressionMatch &m = ordered ? numbered : bullet;
            const QString marker = ordered ? m.captured(2) + m.captured(3) : m.captured(2);
            const QString rest = ordered ? m.captured(5) : m.captured(4);
            contentIndent = lineIndent + marker.size() + 1;

            items << QStringList(rest);
            ++j;
            continue;
        }

        // Continuation of the current item, either indented or lazy.
        if (!items.isEmpty() && (lineIndent >= contentIndent || (!pendingBlank && !isItem))) {
            if (pendingBlank) {
                loose = true;
                items.last() << QString();
                pendingBlank = false;
            }
            items.last() << stripIndent(line, contentIndent);
            ++j;
            continue;
        }

        break;
    }
    *i = j;

    static const QRegularExpression taskMarker(QStringLiteral("^\\[([ xX])\\][ \t]+"));

    QString body;
    for (int k = 0; k < items.size(); ++k) {
        QStringList content = items.at(k);

        QString checkbox;
        if (!content.isEmpty()) {
            const QRegularExpressionMatch task = taskMarker.match(content.first());
            if (task.hasMatch()) {
                const bool checked = task.captured(1).toLower() == QLatin1String("x");
                checkbox = checked ? QStringLiteral("☑ ") : QStringLiteral("☐ ");
                content[0] = content.first().mid(task.capturedLength());
            }
        }

        QString itemHtml;
        parseBlocks(content, 0, content.size(), &itemHtml);

        // A tight list renders its items inline, without paragraph spacing.
        if (!loose && itemHtml.startsWith(QLatin1String("<p>"))
            && itemHtml.indexOf(QLatin1String("</p>")) == itemHtml.length() - 5) {
            itemHtml = itemHtml.mid(3, itemHtml.length() - 3 - 5);
        }

        body += QStringLiteral("<li>%1%2</li>\n").arg(checkbox, itemHtml.trimmed());
    }

    if (ordered) {
        const QString attr = (startNumber == QLatin1String("1"))
            ? QString()
            : QStringLiteral(" start=\"%1\"").arg(startNumber.toInt());
        *out += QStringLiteral("<ol%1>\n%2</ol>\n").arg(attr, body);
    } else {
        *out += QStringLiteral("<ul>\n%1</ul>\n").arg(body);
    }
}

void MarkdownParser::parseTable(const QStringList &lines, int *i, int to, QString *out) const
{
    const QStringList headers = splitTableRow(lines.at(*i));
    const QStringList delimiters = splitTableRow(lines.at(*i + 1));

    QStringList alignments;
    for (int c = 0; c < delimiters.size(); ++c) {
        const QString spec = delimiters.at(c);
        const bool left = spec.startsWith(QLatin1Char(':'));
        const bool right = spec.endsWith(QLatin1Char(':'));
        if (left && right)
            alignments << QStringLiteral("center");
        else if (right)
            alignments << QStringLiteral("right");
        else
            alignments << QStringLiteral("left");
    }

    const auto alignFor = [&alignments](int column) {
        return column < alignments.size() ? alignments.at(column) : QStringLiteral("left");
    };

    QString head;
    for (int c = 0; c < headers.size(); ++c) {
        head += QStringLiteral("<th align=\"%1\">%2</th>")
                    .arg(alignFor(c), parseInline(headers.at(c)));
    }

    QString body;
    int j = *i + 2;
    for (; j < to; ++j) {
        const QString &line = lines.at(j);
        if (isBlank(line) || !line.contains(QLatin1Char('|')))
            break;

        const QStringList cells = splitTableRow(line);
        QString row;
        for (int c = 0; c < headers.size(); ++c) {
            const QString cell = c < cells.size() ? cells.at(c) : QString();
            row += QStringLiteral("<td align=\"%1\">%2</td>")
                       .arg(alignFor(c), parseInline(cell));
        }
        body += QStringLiteral("<tr>%1</tr>\n").arg(row);
    }
    *i = j;

    // Qt's rich text engine ignores most table CSS, so styling stays on the tag.
    *out += QStringLiteral("<table border=\"1\" cellspacing=\"0\" cellpadding=\"5\">\n"
                           "<tr>%1</tr>\n%2</table>\n")
                .arg(head, body);
}

void MarkdownParser::parseParagraph(const QStringList &lines, int *i, int to, QString *out) const
{
    QStringList text;
    int j = *i;
    for (; j < to; ++j) {
        const QString &line = lines.at(j);
        if (isBlank(line))
            break;

        if (!text.isEmpty()) {
            if (reSetextH1().match(line).hasMatch()) {
                *out += QStringLiteral("<h1>%1</h1>\n")
                            .arg(parseInline(text.join(QLatin1Char('\n'))));
                *i = j + 1;
                return;
            }
            // A dashed line closing an open paragraph is a setext heading, not
            // a thematic break; a standalone one never reaches this point.
            if (reSetextH2().match(line).hasMatch()) {
                *out += QStringLiteral("<h2>%1</h2>\n")
                            .arg(parseInline(text.join(QLatin1Char('\n'))));
                *i = j + 1;
                return;
            }
            if (startsNewBlock(line))
                break;
        }

        // Leading whitespace is insignificant, but trailing spaces are not:
        // two of them before the newline mean a hard line break.
        int firstNonSpace = 0;
        indentWidth(line, &firstNonSpace);
        text << line.mid(firstNonSpace);
    }
    *i = j;

    if (!text.isEmpty())
        *out += QStringLiteral("<p>%1</p>\n").arg(parseInline(text.join(QLatin1Char('\n'))));
}

QString MarkdownParser::parseInline(const QString &text) const
{
    QStringList store;
    QString work = escapeHtml(protectSpans(text, &store));

    // Hard line break: two or more trailing spaces before a newline.
    static const QRegularExpression hardBreak(QStringLiteral("[ \t]{2,}\n"));
    work.replace(hardBreak, QStringLiteral("<br />\n"));

    static const QRegularExpression image(
        QStringLiteral("!\\[([^\\]]*)\\]\\(\\s*([^)\\s]+)(?:\\s+\"([^\"]*)\")?\\s*\\)"));
    work.replace(image, QStringLiteral("<img src=\"\\2\" alt=\"\\1\" title=\"\\3\" />"));

    static const QRegularExpression link(
        QStringLiteral("\\[([^\\]]*)\\]\\(\\s*([^)\\s]+)(?:\\s+\"([^\"]*)\")?\\s*\\)"));
    work.replace(link, QStringLiteral("<a href=\"\\2\" title=\"\\3\">\\1</a>"));

    static const QRegularExpression autolink(
        QStringLiteral("&lt;((?:https?|ftp|mailto):[^\\s&]+)&gt;"));
    work.replace(autolink, QStringLiteral("<a href=\"\\1\">\\1</a>"));

    struct Rule { const char *pattern; const char *replacement; };
    static const Rule rules[] = {
        { "\\*\\*\\*(?!\\s)(.+?)(?<!\\s)\\*\\*\\*", "<b><i>\\1</i></b>" },
        { "\\*\\*(?!\\s)(.+?)(?<!\\s)\\*\\*",       "<b>\\1</b>" },
        { "(?<![\\w\\\\])__(?!\\s)(.+?)(?<!\\s)__(?![\\w])", "<b>\\1</b>" },
        { "~~(?!\\s)(.+?)(?<!\\s)~~",               "<s>\\1</s>" },
        { "\\*(?!\\s)(.+?)(?<!\\s)\\*",             "<i>\\1</i>" },
        { "(?<![\\w\\\\])_(?!\\s)(.+?)(?<!\\s)_(?![\\w])",   "<i>\\1</i>" },
    };
    for (const Rule &rule : rules) {
        const QRegularExpression re(QLatin1String(rule.pattern),
                                    QRegularExpression::DotMatchesEverythingOption);
        work.replace(re, QLatin1String(rule.replacement));
    }

    return restoreSpans(work, store);
}

QString MarkdownParser::protectSpans(const QString &text, QStringList *store) const
{
    const auto reserve = [store](const QString &html) {
        store->append(html);
        return kMarkStart + QString::number(store->size() - 1) + kMarkEnd;
    };

    QString out;
    out.reserve(text.size());

    const int n = text.size();
    int i = 0;
    while (i < n) {
        const QChar c = text.at(i);

        if (c == QLatin1Char('\\') && i + 1 < n) {
            const QChar next = text.at(i + 1);
            if (next == QLatin1Char('\n')) {
                out += reserve(QStringLiteral("<br />"));
                out += QLatin1Char('\n');
                i += 2;
                continue;
            }
            if (QString::fromLatin1(kEscapable).contains(next)) {
                out += reserve(escapeHtml(QString(next)));
                i += 2;
                continue;
            }
        }

        if (c == QLatin1Char('<')) {
            const QStringRef rest = text.midRef(i);

            // Comments carry authoring notes and should not reach the reader.
            static const QRegularExpression comment(QStringLiteral("^<!--.*?-->"),
                                                    QRegularExpression::DotMatchesEverythingOption);
            const QRegularExpressionMatch hidden = comment.match(rest);
            if (hidden.hasMatch()) {
                i += hidden.capturedLength();
                continue;
            }

            static const QRegularExpression tag(
                QStringLiteral("^</?([a-zA-Z][a-zA-Z0-9]*)[^<>]*>"));
            const QRegularExpressionMatch inlineTag = tag.match(rest);
            if (inlineTag.hasMatch()
                && isPassThroughTag(inlineTag.captured(1).toLower())) {
                out += reserve(inlineTag.captured(0));
                i += inlineTag.capturedLength();
                continue;
            }
        }

        if (c == QLatin1Char('`')) {
            int ticks = 0;
            while (i + ticks < n && text.at(i + ticks) == QLatin1Char('`'))
                ++ticks;

            const QString fence(ticks, QLatin1Char('`'));
            int close = text.indexOf(fence, i + ticks);
            // The closing run must be exactly as long as the opening one.
            while (close >= 0 && close + ticks < n && text.at(close + ticks) == QLatin1Char('`'))
                close = text.indexOf(fence, close + 1);

            if (close >= 0) {
                QString code = text.mid(i + ticks, close - i - ticks);
                code.replace(QLatin1Char('\n'), QLatin1Char(' '));
                if (code.size() > 1 && code.startsWith(QLatin1Char(' '))
                    && code.endsWith(QLatin1Char(' '))) {
                    code = code.mid(1, code.size() - 2);
                }
                out += reserve(QStringLiteral("<code>%1</code>").arg(escapeHtml(code)));
                i = close + ticks;
                continue;
            }
        }

        out += c;
        ++i;
    }
    return out;
}

QString MarkdownParser::restoreSpans(const QString &text, const QStringList &store) const
{
    if (store.isEmpty())
        return text;

    static const QRegularExpression marker(
        QStringLiteral("\x01(\\d+)\x02"));

    QString out;
    int last = 0;
    QRegularExpressionMatchIterator it = marker.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        out += text.mid(last, m.capturedStart() - last);
        const int index = m.captured(1).toInt();
        if (index >= 0 && index < store.size())
            out += store.at(index);
        last = m.capturedEnd();
    }
    out += text.mid(last);
    return out;
}
