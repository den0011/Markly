#include <QtTest>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

#include "imagedialog.h"
#include "linkdialog.h"
#include "markdownsnippets.h"
#include "tabledialog.h"

using MarkdownSnippets::Alignment;

// The dialogs are thin shells over MarkdownSnippets, so these tests check the
// wiring rather than the formatting: that the fields reach the generator, that
// the relative-path option is disabled when there is nothing to be relative to,
// and that OK stays out of reach until a target is entered.
class TestDialogs : public QObject
{
    Q_OBJECT

private slots:
    void linkDialogBuildsMarkdown();
    void linkDialogFallsBackToTheTargetAsText();
    void linkDialogRefusesAnEmptyTarget();
    void relativeOptionIsUnavailableForAnUnsavedDocument();

    void imageDialogBuildsMarkdown();

    void tableDialogBuildsPipeTable();
    void tableDialogTracksAlignmentPerColumn();
    void tableDialogKeepsAtLeastOneRowAndColumn();

private:
    template <typename Dialog>
    static QLineEdit *lineEdit(Dialog *dialog, const char *name)
    {
        return dialog->template findChild<QLineEdit *>(QLatin1String(name));
    }
};

void TestDialogs::linkDialogBuildsMarkdown()
{
    LinkDialog dialog(QStringLiteral("C:/docs/notes.md"));

    dialog.setLinkText(QStringLiteral("Qt"));
    dialog.setTarget(QStringLiteral("https://qt.io"));
    lineEdit(&dialog, "titleEdit")->setText(QStringLiteral("Home"));

    QCOMPARE(dialog.markdown(), QStringLiteral("[Qt](https://qt.io \"Home\")"));
}

void TestDialogs::linkDialogFallsBackToTheTargetAsText()
{
    LinkDialog dialog{QString()};
    dialog.setTarget(QStringLiteral("https://qt.io"));

    QCOMPARE(dialog.markdown(), QStringLiteral("[https://qt.io](https://qt.io)"));
}

void TestDialogs::linkDialogRefusesAnEmptyTarget()
{
    LinkDialog dialog{QString()};

    QDialogButtonBox *buttons = dialog.findChild<QDialogButtonBox *>();
    QVERIFY(buttons);
    QVERIFY(!buttons->button(QDialogButtonBox::Ok)->isEnabled());

    dialog.setTarget(QStringLiteral("x.md"));
    QVERIFY(buttons->button(QDialogButtonBox::Ok)->isEnabled());
}

void TestDialogs::relativeOptionIsUnavailableForAnUnsavedDocument()
{
    LinkDialog unsaved{QString()};
    QCheckBox *check = unsaved.findChild<QCheckBox *>(QStringLiteral("relativeCheck"));
    QVERIFY(check);
    QVERIFY(!check->isEnabled());
    QVERIFY(!check->isChecked());

    LinkDialog saved(QStringLiteral("C:/docs/notes.md"));
    QVERIFY(saved.findChild<QCheckBox *>(QStringLiteral("relativeCheck"))->isEnabled());
}

void TestDialogs::imageDialogBuildsMarkdown()
{
    const QString document = QDir::currentPath() + QStringLiteral("/docs/notes.md");
    const QString picture = QDir::currentPath() + QStringLiteral("/docs/img/shot.png");

    ImageDialog dialog(document);
    dialog.setAltText(QStringLiteral("Screenshot"));
    dialog.setTarget(picture);

    // The relative-path option is on by default for a saved document.
    QCOMPARE(dialog.markdown(), QStringLiteral("![Screenshot](img/shot.png)"));
}

void TestDialogs::tableDialogBuildsPipeTable()
{
    TableDialog dialog;
    dialog.setSize(2, 1);
    dialog.setHeader(0, QStringLiteral("Name"));
    dialog.setHeader(1, QStringLiteral("Qty"));
    dialog.setCell(0, 0, QStringLiteral("Apples"));
    dialog.setCell(0, 1, QStringLiteral("3"));

    QCOMPARE(dialog.markdown(),
             QStringLiteral("| Name   | Qty |\n"
                            "| :----- | :-- |\n"
                            "| Apples | 3   |\n"));
}

void TestDialogs::tableDialogTracksAlignmentPerColumn()
{
    TableDialog dialog;
    dialog.setSize(2, 1);
    dialog.setHeader(0, QStringLiteral("a"));
    dialog.setHeader(1, QStringLiteral("b"));
    dialog.setAlignment(1, Alignment::Right);

    const QString table = dialog.markdown();
    QVERIFY2(table.contains(QStringLiteral("| :-- | --: |")), qPrintable(table));

    // Moving the cursor must not overwrite what was set for another column.
    QTableWidget *grid = dialog.findChild<QTableWidget *>(QStringLiteral("grid"));
    QVERIFY(grid);
    grid->setCurrentCell(0, 0);
    grid->setCurrentCell(0, 1);
    grid->setCurrentCell(0, 0);

    QCOMPARE(dialog.markdown(), table);
}

void TestDialogs::tableDialogKeepsAtLeastOneRowAndColumn()
{
    TableDialog dialog;
    dialog.setSize(1, 1);

    QTableWidget *grid = dialog.findChild<QTableWidget *>(QStringLiteral("grid"));
    QVERIFY(grid);

    QPushButton *removeRow = dialog.findChild<QPushButton *>(QStringLiteral("removeRowButton"));
    QPushButton *removeColumn = dialog.findChild<QPushButton *>(QStringLiteral("removeColumnButton"));
    QVERIFY(removeRow && removeColumn);

    // A table with no columns cannot be written out at all, so the buttons go
    // dead rather than letting the grid collapse.
    QVERIFY(!removeRow->isEnabled());
    QVERIFY(!removeColumn->isEnabled());
    QCOMPARE(grid->rowCount(), 1);
    QCOMPARE(grid->columnCount(), 1);
}

QTEST_MAIN(TestDialogs)

#include "tst_dialogs.moc"
