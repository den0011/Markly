#include "markdownsnippets.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace {

// Characters that would end the link target early, or be read as a title.
QString encodeTarget(const QString &url)
{
    QString encoded = url;
    encoded.replace(QLatin1String(" "), QLatin1String("%20"));
    encoded.replace(QLatin1String("("), QLatin1String("%28"));
    encoded.replace(QLatin1String(")"), QLatin1String("%29"));
    return encoded;
}

QString escapeLabel(const QString &text)
{
    QString escaped = text;
    escaped.replace(QLatin1String("["), QLatin1String("\\["));
    escaped.replace(QLatin1String("]"), QLatin1String("\\]"));
    return escaped;
}

QString escapeCell(const QString &text)
{
    QString escaped = text.simplified();
    escaped.replace(QLatin1String("|"), QLatin1String("\\|"));
    return escaped;
}

QString titleSuffix(const QString &title)
{
    if (title.trimmed().isEmpty())
        return QString();

    QString quoted = title.trimmed();
    quoted.replace(QLatin1String("\""), QLatin1String("\\\""));
    return QStringLiteral(" \"%1\"").arg(quoted);
}

bool isWebUrl(const QString &target)
{
    const QString scheme = QUrl(target).scheme().toLower();
    return scheme == QLatin1String("http") || scheme == QLatin1String("https")
        || scheme == QLatin1String("ftp") || scheme == QLatin1String("mailto")
        || scheme == QLatin1String("file");
}

// Whether a relative path between the two can exist at all.
bool sharesRoot(const QString &one, const QString &other)
{
#ifdef Q_OS_WIN
    // Across drives there is no relative path, and QDir::relativeFilePath
    // returns nonsense instead of failing, so the drive letters are compared.
    return QDir::toNativeSeparators(one).left(2)
        .compare(QDir::toNativeSeparators(other).left(2), Qt::CaseInsensitive) == 0;
#else
    // Everything hangs off a single root elsewhere.
    Q_UNUSED(one)
    Q_UNUSED(other)
    return true;
#endif
}

QString ruleFor(MarkdownSnippets::Alignment alignment, int width)
{
    const int dashes = qMax(3, width);

    switch (alignment) {
    case MarkdownSnippets::Alignment::Center:
        return QLatin1Char(':') + QString(dashes - 2, QLatin1Char('-')) + QLatin1Char(':');
    case MarkdownSnippets::Alignment::Right:
        return QString(dashes - 1, QLatin1Char('-')) + QLatin1Char(':');
    case MarkdownSnippets::Alignment::Left:
        break;
    }
    return QLatin1Char(':') + QString(dashes - 1, QLatin1Char('-'));
}

} // namespace

namespace MarkdownSnippets {

QString link(const QString &text, const QString &url, const QString &title)
{
    return QStringLiteral("[%1](%2%3)")
        .arg(escapeLabel(text), encodeTarget(url.trimmed()), titleSuffix(title));
}

QString image(const QString &altText, const QString &url, const QString &title)
{
    return QLatin1Char('!') + link(altText, url, title);
}

QString pipeTable(const QStringList &headers,
                  const QVector<QStringList> &rows,
                  const QVector<Alignment> &alignments)
{
    if (headers.isEmpty())
        return QString();

    const int columns = headers.size();

    // Column widths are cosmetic, but a table nobody can read in the source is
    // a table nobody will edit by hand afterwards.
    QVector<int> widths(columns, 3);
    for (int c = 0; c < columns; ++c)
        widths[c] = qMax(widths.at(c), escapeCell(headers.at(c)).length());

    for (const QStringList &row : rows) {
        for (int c = 0; c < qMin(columns, row.size()); ++c)
            widths[c] = qMax(widths.at(c), escapeCell(row.at(c)).length());
    }

    const auto alignmentAt = [&alignments](int column) {
        return column < alignments.size() ? alignments.at(column) : Alignment::Left;
    };

    const auto renderRow = [&](const QStringList &cells) {
        QString line = QStringLiteral("|");
        for (int c = 0; c < columns; ++c) {
            const QString cell = c < cells.size() ? escapeCell(cells.at(c)) : QString();
            line += QLatin1Char(' ') + cell.leftJustified(widths.at(c))
                  + QLatin1String(" |");
        }
        return line;
    };

    QString table = renderRow(headers) + QLatin1Char('\n');

    table += QLatin1Char('|');
    for (int c = 0; c < columns; ++c)
        table += QLatin1Char(' ') + ruleFor(alignmentAt(c), widths.at(c)) + QLatin1String(" |");
    table += QLatin1Char('\n');

    for (const QStringList &row : rows)
        table += renderRow(row) + QLatin1Char('\n');

    return table;
}

QString linkTarget(const QString &target, const QString &documentPath, bool preferRelative)
{
    const QString trimmed = target.trimmed();
    if (trimmed.isEmpty() || isWebUrl(trimmed))
        return trimmed;

    const QFileInfo file(trimmed);
    if (!file.isAbsolute())
        return QDir::fromNativeSeparators(trimmed); // already relative, leave it

    if (preferRelative && !documentPath.isEmpty()) {
        const QDir documentDir = QFileInfo(documentPath).absoluteDir();

        if (sharesRoot(documentDir.absolutePath(), file.absoluteFilePath())) {
            return QDir::fromNativeSeparators(
                documentDir.relativeFilePath(file.absoluteFilePath()));
        }
    }

    return QUrl::fromLocalFile(file.absoluteFilePath()).toString();
}

} // namespace MarkdownSnippets
