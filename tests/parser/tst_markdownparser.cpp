#include <QtTest>

#include "markdownparser.h"

class TestMarkdownParser : public QObject
{
    Q_OBJECT

private slots:
    void blocks_data();
    void blocks();

    void inlineFormatting_data();
    void inlineFormatting();

    void tightListDropsParagraph();
    void looseListKeepsParagraph();
    void nestedListIsIndented();
    void fencedCodeIsNotReinterpreted();
    void codeSpanIsNotReinterpreted();
    void table();
    void escapedCharactersStayLiteral();

    void inlineHtmlIsPassedThrough_data();
    void inlineHtmlIsPassedThrough();
    void unknownHtmlStaysLiteral();
    void htmlCommentsAreDropped();

private:
    MarkdownParser m_parser;
};

void TestMarkdownParser::blocks_data()
{
    QTest::addColumn<QString>("markdown");
    QTest::addColumn<QString>("expected");

    QTest::newRow("atx h1")        << "# Title"            << "<h1>Title</h1>\n";
    QTest::newRow("atx h6")        << "###### Deep"        << "<h6>Deep</h6>\n";
    QTest::newRow("atx closed")    << "## Title ##"        << "<h2>Title</h2>\n";
    QTest::newRow("setext h1")     << "Title\n====="       << "<h1>Title</h1>\n";
    QTest::newRow("setext h2")     << "Title\n-----"       << "<h2>Title</h2>\n";
    QTest::newRow("paragraph")     << "Hello world"        << "<p>Hello world</p>\n";
    QTest::newRow("hr dashes")     << "---"                << "<hr />\n";
    QTest::newRow("hr stars")      << "* * *"              << "<hr />\n";
    QTest::newRow("blockquote")    << "> quoted"
                                   << "<blockquote>\n<p>quoted</p>\n</blockquote>\n";
    QTest::newRow("indented code") << "    code()"
                                   << "<pre><code>code()</code></pre>\n";
    QTest::newRow("hard break")    << "one  \ntwo"
                                   << "<p>one<br />\ntwo</p>\n";
}

void TestMarkdownParser::blocks()
{
    QFETCH(QString, markdown);
    QFETCH(QString, expected);

    QCOMPARE(m_parser.toHtml(markdown), expected);
}

void TestMarkdownParser::inlineFormatting_data()
{
    QTest::addColumn<QString>("markdown");
    QTest::addColumn<QString>("expected");

    QTest::newRow("strong stars")  << "**bold**"     << "<p><b>bold</b></p>\n";
    QTest::newRow("strong under")  << "__bold__"     << "<p><b>bold</b></p>\n";
    QTest::newRow("em stars")      << "*it*"         << "<p><i>it</i></p>\n";
    QTest::newRow("em under")      << "_it_"         << "<p><i>it</i></p>\n";
    QTest::newRow("strong em")     << "***both***"   << "<p><b><i>both</i></b></p>\n";
    QTest::newRow("strike")        << "~~gone~~"     << "<p><s>gone</s></p>\n";
    QTest::newRow("intraword")     << "snake_case_x" << "<p>snake_case_x</p>\n";
    QTest::newRow("link")
        << "[Qt](https://qt.io)"
        << "<p><a href=\"https://qt.io\" title=\"\">Qt</a></p>\n";
    QTest::newRow("autolink")
        << "<https://qt.io>"
        << "<p><a href=\"https://qt.io\">https://qt.io</a></p>\n";
    QTest::newRow("image")
        << "![alt](cat.png)"
        << "<p><img src=\"cat.png\" alt=\"alt\" title=\"\" /></p>\n";
    QTest::newRow("html escaped")
        << "a < b & c > d"
        << "<p>a &lt; b &amp; c &gt; d</p>\n";
}

void TestMarkdownParser::inlineFormatting()
{
    QFETCH(QString, markdown);
    QFETCH(QString, expected);

    QCOMPARE(m_parser.toHtml(markdown), expected);
}

void TestMarkdownParser::tightListDropsParagraph()
{
    const QString html = m_parser.toHtml(QStringLiteral("- one\n- two"));
    QCOMPARE(html, QStringLiteral("<ul>\n<li>one</li>\n<li>two</li>\n</ul>\n"));
}

void TestMarkdownParser::looseListKeepsParagraph()
{
    const QString html = m_parser.toHtml(QStringLiteral("- one\n\n- two"));
    QVERIFY2(html.contains(QStringLiteral("<li><p>one</p></li>")), qPrintable(html));
}

void TestMarkdownParser::nestedListIsIndented()
{
    const QString html = m_parser.toHtml(QStringLiteral("- outer\n  - inner"));
    QVERIFY2(html.contains(QStringLiteral("<ul>\n<li>inner</li>\n</ul>")), qPrintable(html));
}

void TestMarkdownParser::fencedCodeIsNotReinterpreted()
{
    const QString html = m_parser.toHtml(
        QStringLiteral("```cpp\nif (a < b) { *p = 1; }\n```"));

    QCOMPARE(html, QStringLiteral("<pre><code class=\"language-cpp\">"
                                  "if (a &lt; b) { *p = 1; }</code></pre>\n"));
}

void TestMarkdownParser::codeSpanIsNotReinterpreted()
{
    const QString html = m_parser.toHtml(QStringLiteral("use `a * b * c` here"));
    QCOMPARE(html, QStringLiteral("<p>use <code>a * b * c</code> here</p>\n"));
}

void TestMarkdownParser::table()
{
    const QString html = m_parser.toHtml(
        QStringLiteral("| a | b |\n|:--|--:|\n| 1 | 2 |"));

    QVERIFY2(html.contains(QStringLiteral("<th align=\"left\">a</th>")), qPrintable(html));
    QVERIFY2(html.contains(QStringLiteral("<th align=\"right\">b</th>")), qPrintable(html));
    QVERIFY2(html.contains(QStringLiteral("<td align=\"left\">1</td>")), qPrintable(html));
    QVERIFY2(html.contains(QStringLiteral("<td align=\"right\">2</td>")), qPrintable(html));
}

void TestMarkdownParser::escapedCharactersStayLiteral()
{
    QCOMPARE(m_parser.toHtml(QStringLiteral("\\*not italic\\*")),
             QStringLiteral("<p>*not italic*</p>\n"));
}

void TestMarkdownParser::inlineHtmlIsPassedThrough_data()
{
    QTest::addColumn<QString>("markdown");
    QTest::addColumn<QString>("expected");

    QTest::newRow("small")
        << "234<small>2342</small>234"
        << "<p>234<small>2342</small>234</p>\n";
    QTest::newRow("sup")   << "x<sup>2</sup>"   << "<p>x<sup>2</sup></p>\n";
    QTest::newRow("kbd")   << "<kbd>Ctrl</kbd>" << "<p><kbd>Ctrl</kbd></p>\n";
    QTest::newRow("br")    << "a<br/>b"         << "<p>a<br/>b</p>\n";
    QTest::newRow("attrs")
        << "<span style=\"color:red\">hot</span>"
        << "<p><span style=\"color:red\">hot</span></p>\n";
    QTest::newRow("uppercase") << "<SMALL>x</SMALL>" << "<p><SMALL>x</SMALL></p>\n";
}

void TestMarkdownParser::inlineHtmlIsPassedThrough()
{
    QFETCH(QString, markdown);
    QFETCH(QString, expected);

    QCOMPARE(m_parser.toHtml(markdown), expected);
}

void TestMarkdownParser::unknownHtmlStaysLiteral()
{
    // Escaped rather than dropped: silently swallowing markup the renderer
    // cannot show would hide the author's own content from them.
    QCOMPARE(m_parser.toHtml(QStringLiteral("<script>bad()</script>")),
             QStringLiteral("<p>&lt;script&gt;bad()&lt;/script&gt;</p>\n"));
    QCOMPARE(m_parser.toHtml(QStringLiteral("2 < 3 and 4 > 1")),
             QStringLiteral("<p>2 &lt; 3 and 4 &gt; 1</p>\n"));
}

void TestMarkdownParser::htmlCommentsAreDropped()
{
    QCOMPARE(m_parser.toHtml(QStringLiteral("before<!-- note -->after")),
             QStringLiteral("<p>beforeafter</p>\n"));
}

QTEST_APPLESS_MAIN(TestMarkdownParser)

#include "tst_markdownparser.moc"
