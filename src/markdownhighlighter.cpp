#include "markdownhighlighter.h"

#include <QFont>

namespace {

QTextCharFormat makeFormat(const QColor &color,
                           bool bold = false,
                           bool italic = false,
                           bool strikeOut = false)
{
    QTextCharFormat format;
    format.setForeground(color);
    if (bold)
        format.setFontWeight(QFont::Bold);
    format.setFontItalic(italic);
    format.setFontStrikeOut(strikeOut);
    return format;
}

enum BlockState { NormalState = 0, InsideFence = 1 };

} // namespace

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_fence(QStringLiteral("^ {0,3}(`{3,}|~{3,})"))
{
    const QColor heading(0x1f, 0x6f, 0xeb);
    const QColor emphasis(0x24, 0x29, 0x2f);
    const QColor code(0xb3, 0x1d, 0x28);
    const QColor link(0x0a, 0x69, 0x69);
    const QColor quote(0x6a, 0x73, 0x7d);
    const QColor marker(0x95, 0x3b, 0x00);

    m_rules.append({ QRegularExpression(QStringLiteral("^ {0,3}#{1,6}[ \t].*$")),
                     makeFormat(heading, true), 0 });
    m_rules.append({ QRegularExpression(QStringLiteral("^ {0,3}>.*$")),
                     makeFormat(quote, false, true), 0 });
    m_rules.append({ QRegularExpression(QStringLiteral("^ {0,3}([-*+]|\\d{1,9}[.)])[ \t]")),
                     makeFormat(marker, true), 1 });
    m_rules.append({ QRegularExpression(
                         QStringLiteral("^ {0,3}((\\*[ \t]*){3,}|(-[ \t]*){3,}|(_[ \t]*){3,})$")),
                     makeFormat(marker, true), 0 });
    m_rules.append({ QRegularExpression(QStringLiteral("\\*\\*(?!\\s)(.+?)(?<!\\s)\\*\\*")),
                     makeFormat(emphasis, true), 0 });
    m_rules.append({ QRegularExpression(QStringLiteral("(?<![\\w*])\\*(?!\\s)(.+?)(?<!\\s)\\*(?![\\w*])")),
                     makeFormat(emphasis, false, true), 0 });
    m_rules.append({ QRegularExpression(QStringLiteral("~~(?!\\s)(.+?)(?<!\\s)~~")),
                     makeFormat(emphasis, false, false, true), 0 });
    m_rules.append({ QRegularExpression(QStringLiteral("`[^`\n]+`")),
                     makeFormat(code), 0 });
    m_rules.append({ QRegularExpression(QStringLiteral("!?\\[[^\\]]*\\]\\([^)]*\\)")),
                     makeFormat(link), 0 });

    m_codeBlockFormat = makeFormat(code);
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    const int previousState = previousBlockState();
    const bool isFenceLine = m_fence.match(text).hasMatch();

    if (previousState == InsideFence) {
        setFormat(0, text.length(), m_codeBlockFormat);
        setCurrentBlockState(isFenceLine ? NormalState : InsideFence);
        return;
    }

    if (isFenceLine) {
        setFormat(0, text.length(), m_codeBlockFormat);
        setCurrentBlockState(InsideFence);
        return;
    }

    setCurrentBlockState(NormalState);

    for (const Rule &rule : qAsConst(m_rules)) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(rule.captureGroup),
                      match.capturedLength(rule.captureGroup),
                      rule.format);
        }
    }
}
