#include <QtTest>

#include <QDir>

#include "markdownparser.h"
#include "markdownsnippets.h"

using MarkdownSnippets::Alignment;

class TestMarkdownSnippets : public QObject
{
    Q_OBJECT

private slots:
    void link_data();
    void link();

    void image();

    void pipeTableIsPaddedAndAligned();
    void pipeTableEscapesPipes();
    void pipeTableSurvivesShortRows();

    void linkTargetKeepsWebUrls();
    void linkTargetGoesRelativeToTheDocument();
    void linkTargetFallsBackToFileUrl();
    void linkTargetLeavesRelativeInputAlone();

    // The whole point of generating this text is that the parser reads it back.
    void generatedMarkdownRoundTrips();

private:
    MarkdownParser m_parser;
};

void TestMarkdownSnippets::link_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("expected");

    QTest::newRow("plain")
        << "Qt" << "https://qt.io" << QString()
        << "[Qt](https://qt.io)";
    QTest::newRow("with title")
        << "Qt" << "https://qt.io" << "Home"
        << "[Qt](https://qt.io \"Home\")";
    QTest::newRow("spaces encoded")
        << "License" << "C:/Program Files/app/license.md" << QString()
        << "[License](C:/Program%20Files/app/license.md)";
    QTest::newRow("brackets in label escaped")
        << "a [b] c" << "x.md" << QString()
        << "[a \\[b\\] c](x.md)";
    QTest::newRow("parens in target encoded")
        << "doc" << "C:/tmp/a(1).md" << QString()
        << "[doc](C:/tmp/a%281%29.md)";
}

void TestMarkdownSnippets::link()
{
    QFETCH(QString, text);
    QFETCH(QString, url);
    QFETCH(QString, title);
    QFETCH(QString, expected);

    QCOMPARE(MarkdownSnippets::link(text, url, title), expected);
}

void TestMarkdownSnippets::image()
{
    QCOMPARE(MarkdownSnippets::image(QStringLiteral("shot"), QStringLiteral("img/a.png")),
             QStringLiteral("![shot](img/a.png)"));
}

void TestMarkdownSnippets::pipeTableIsPaddedAndAligned()
{
    const QStringList headers = { QStringLiteral("Name"), QStringLiteral("Qty") };
    const QVector<QStringList> rows = {
        { QStringLiteral("Apples"), QStringLiteral("3") },
    };
    const QVector<Alignment> alignments = { Alignment::Left, Alignment::Right };

    QCOMPARE(MarkdownSnippets::pipeTable(headers, rows, alignments),
             QStringLiteral("| Name   | Qty |\n"
                            "| :----- | --: |\n"
                            "| Apples | 3   |\n"));
}

void TestMarkdownSnippets::pipeTableEscapesPipes()
{
    const QStringList headers = { QStringLiteral("a") };
    const QVector<QStringList> rows = { { QStringLiteral("x | y") } };

    const QString table = MarkdownSnippets::pipeTable(headers, rows, {});
    QVERIFY2(table.contains(QStringLiteral("x \\| y")), qPrintable(table));
}

void TestMarkdownSnippets::pipeTableSurvivesShortRows()
{
    const QStringList headers = { QStringLiteral("a"), QStringLiteral("b") };
    const QVector<QStringList> rows = { { QStringLiteral("1") } };

    const QString table = MarkdownSnippets::pipeTable(headers, rows, {});
    QCOMPARE(table.count(QLatin1Char('\n')), 3);
    QVERIFY2(table.endsWith(QStringLiteral("| 1   |     |\n")), qPrintable(table));
}

void TestMarkdownSnippets::linkTargetKeepsWebUrls()
{
    QCOMPARE(MarkdownSnippets::linkTarget(QStringLiteral("https://qt.io/a b"),
                                          QStringLiteral("C:/docs/x.md"), true),
             QStringLiteral("https://qt.io/a b"));
}

void TestMarkdownSnippets::linkTargetGoesRelativeToTheDocument()
{
    const QString document = QDir::currentPath() + QStringLiteral("/docs/notes.md");
    const QString picture = QDir::currentPath() + QStringLiteral("/docs/img/a.png");

    QCOMPARE(MarkdownSnippets::linkTarget(picture, document, true),
             QStringLiteral("img/a.png"));

    // Opting out keeps the absolute form.
    QVERIFY(MarkdownSnippets::linkTarget(picture, document, false)
                .startsWith(QStringLiteral("file:///")));
}

void TestMarkdownSnippets::linkTargetFallsBackToFileUrl()
{
    // An unsaved document has no folder to be relative to.
    const QString picture = QDir::currentPath() + QStringLiteral("/a.png");
    QVERIFY(MarkdownSnippets::linkTarget(picture, QString(), true)
                .startsWith(QStringLiteral("file:///")));
}

void TestMarkdownSnippets::linkTargetLeavesRelativeInputAlone()
{
    QCOMPARE(MarkdownSnippets::linkTarget(QStringLiteral("img/a.png"),
                                          QStringLiteral("C:/docs/x.md"), true),
             QStringLiteral("img/a.png"));
}

void TestMarkdownSnippets::generatedMarkdownRoundTrips()
{
    const QString markdown =
        MarkdownSnippets::link(QStringLiteral("License"),
                               QStringLiteral("C:/Program Files/app/license.md"));

    QCOMPARE(m_parser.toHtml(markdown),
             QStringLiteral("<p><a href=\"C:/Program%20Files/app/license.md\" "
                            "title=\"\">License</a></p>\n"));

    const QString table = MarkdownSnippets::pipeTable(
        { QStringLiteral("Name"), QStringLiteral("Qty") },
        { { QStringLiteral("Apples"), QStringLiteral("3") } },
        { Alignment::Left, Alignment::Right });

    const QString html = m_parser.toHtml(table);
    QVERIFY2(html.contains(QStringLiteral("<th align=\"left\">Name</th>")), qPrintable(html));
    QVERIFY2(html.contains(QStringLiteral("<th align=\"right\">Qty</th>")), qPrintable(html));
    QVERIFY2(html.contains(QStringLiteral("<td align=\"right\">3</td>")), qPrintable(html));
}

QTEST_APPLESS_MAIN(TestMarkdownSnippets)

#include "tst_markdownsnippets.moc"
