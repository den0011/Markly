#ifndef MARKDOWNSNIPPETS_H
#define MARKDOWNSNIPPETS_H

#include <QString>
#include <QStringList>
#include <QVector>

// Builds the Markdown that the insert dialogs paste into the document.
//
// Kept apart from the dialogs so the escaping, the path handling and the table
// layout can be tested without a window: that logic is where the mistakes are,
// the widgets on top of it are plumbing.
namespace MarkdownSnippets {

enum class Alignment { Left, Center, Right };

QString link(const QString &text, const QString &url, const QString &title = QString());
QString image(const QString &altText, const QString &url, const QString &title = QString());

// Pipe table with a padded header rule. `alignments` may be shorter than
// `headers`; missing columns default to Left.
QString pipeTable(const QStringList &headers,
                  const QVector<QStringList> &rows,
                  const QVector<Alignment> &alignments);

// Turns a picked file into the target of a link or image.
//
// Web URLs are returned untouched. A local file is written relative to the
// folder of `documentPath` when `preferRelative` is set and the two can be
// expressed relative to each other — on Windows they cannot when they sit on
// different drives, and an unsaved document has no folder at all, so both fall
// back to an absolute file URL.
QString linkTarget(const QString &target, const QString &documentPath, bool preferRelative);

} // namespace MarkdownSnippets

#endif // MARKDOWNSNIPPETS_H
