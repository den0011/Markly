#include <QtTest>

#include <QMenu>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QToolBar>
#include <QToolButton>
#include <QUrl>

#include "mainwindow.h"

// Exercises the main window through its own widgets.
//
// File drops are the fiddly part: Qt hands a drag to the innermost widget that
// accepts drops and never retries the parent when that widget rejects it, so
// the routing is driven here with synthesized events. The rest checks wiring
// that Designer cannot express and that therefore only exists in code.
class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void dropOnEditorOpensFile();
    void dropOnPreviewOpensFile();
    void dragOfUnsupportedFileIsRejected();
    void dragOfPlainTextIsLeftToTheEditor();

    void headingLevelsCollapseIntoOneToolbarButton();
    void headingShortcutsSurviveTheMove();
    void headingLevelsStillInsertTheirMarker();

private:
    QWidget *editorViewport(MainWindow *window) const;
    QWidget *previewViewport(MainWindow *window) const;

    // Builds a mime payload naming `path` the way a file manager would.
    static QMimeData *fileDrag(const QString &path);

    static bool sendDrag(QWidget *target, const QMimeData *mime, QEvent::Type type);

    // A real drop is always preceded by DragEnter and DragMove, and the text
    // panes rely on that: they set up their drop cursor while the drag moves.
    // Returns whether the final Drop was accepted.
    static bool performDrop(QWidget *target, const QMimeData *mime);

    QString writeDocument(const QString &name, const QString &contents);

    QTemporaryDir m_dir;
};

void TestMainWindow::initTestCase()
{
    QVERIFY2(m_dir.isValid(), qPrintable(m_dir.errorString()));

    // Keep the test away from the real application's stored settings.
    QCoreApplication::setOrganizationName(QStringLiteral("Markly"));
    QCoreApplication::setApplicationName(QStringLiteral("MarklyFileDropTest"));
}

QString TestMainWindow::writeDocument(const QString &name, const QString &contents)
{
    const QString path = m_dir.filePath(name);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return QString();
    file.write(contents.toUtf8());
    file.close();

    return path;
}

QMimeData *TestMainWindow::fileDrag(const QString &path)
{
    QMimeData *mime = new QMimeData;
    mime->setUrls(QList<QUrl>() << QUrl::fromLocalFile(path));
    return mime;
}

bool TestMainWindow::sendDrag(QWidget *target, const QMimeData *mime, QEvent::Type type)
{
    const QPoint pos = target->rect().center();

    if (type == QEvent::DragEnter) {
        QDragEnterEvent event(pos, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(target, &event);
        return event.isAccepted();
    }

    if (type == QEvent::DragMove) {
        QDragMoveEvent event(pos, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(target, &event);
        return event.isAccepted();
    }

    QDropEvent event(pos, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(target, &event);
    return event.isAccepted();
}

bool TestMainWindow::performDrop(QWidget *target, const QMimeData *mime)
{
    sendDrag(target, mime, QEvent::DragEnter);
    sendDrag(target, mime, QEvent::DragMove);
    return sendDrag(target, mime, QEvent::Drop);
}

QWidget *TestMainWindow::editorViewport(MainWindow *window) const
{
    QPlainTextEdit *editor = window->findChild<QPlainTextEdit *>(QStringLiteral("editor"));
    return editor ? editor->viewport() : nullptr;
}

QWidget *TestMainWindow::previewViewport(MainWindow *window) const
{
    QTextBrowser *preview = window->findChild<QTextBrowser *>(QStringLiteral("preview"));
    return preview ? preview->viewport() : nullptr;
}

void TestMainWindow::dropOnEditorOpensFile()
{
    const QString path = writeDocument(QStringLiteral("dropped.md"),
                                       QStringLiteral("# Dropped\n\nBody text.\n"));
    QVERIFY(!path.isEmpty());

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QWidget *viewport = editorViewport(&window);
    QVERIFY(viewport);

    QScopedPointer<QMimeData> mime(fileDrag(path));
    QVERIFY2(sendDrag(viewport, mime.data(), QEvent::DragEnter),
             "the editor viewport must advertise itself as a drop target");
    QVERIFY(performDrop(viewport, mime.data()));

    // The open is deferred to the event loop so that it never blocks the
    // drag source.
    QPlainTextEdit *editor = window.findChild<QPlainTextEdit *>(QStringLiteral("editor"));
    QTRY_COMPARE(editor->toPlainText(), QStringLiteral("# Dropped\n\nBody text.\n"));
    QVERIFY(window.windowTitle().contains(QStringLiteral("dropped.md")));
}

void TestMainWindow::dropOnPreviewOpensFile()
{
    const QString path = writeDocument(QStringLiteral("preview-drop.md"),
                                       QStringLiteral("Dropped on the preview.\n"));
    QVERIFY(!path.isEmpty());

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QWidget *viewport = previewViewport(&window);
    QVERIFY(viewport);

    QScopedPointer<QMimeData> mime(fileDrag(path));
    QVERIFY(sendDrag(viewport, mime.data(), QEvent::DragEnter));
    QVERIFY(performDrop(viewport, mime.data()));

    QPlainTextEdit *editor = window.findChild<QPlainTextEdit *>(QStringLiteral("editor"));
    QTRY_COMPARE(editor->toPlainText(), QStringLiteral("Dropped on the preview.\n"));
}

void TestMainWindow::dragOfUnsupportedFileIsRejected()
{
    const QString path = writeDocument(QStringLiteral("payload.bin"),
                                       QStringLiteral("not a document"));
    QVERIFY(!path.isEmpty());

    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // The window frame has no other drop handling, so a rejected drag shows up
    // as an unaccepted event.
    QScopedPointer<QMimeData> mime(fileDrag(path));
    QVERIFY(!sendDrag(&window, mime.data(), QEvent::DragEnter));
}

void TestMainWindow::dragOfPlainTextIsLeftToTheEditor()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QPlainTextEdit *editor = window.findChild<QPlainTextEdit *>(QStringLiteral("editor"));
    QVERIFY(editor);
    editor->setPlainText(QStringLiteral("existing"));

    QScopedPointer<QMimeData> mime(new QMimeData);
    mime->setText(QStringLiteral("dragged text"));

    // The filter must not swallow this: dragging text around inside the editor
    // has to keep working, so the editor itself should insert the payload.
    QVERIFY(performDrop(editor->viewport(), mime.data()));
    QVERIFY2(editor->toPlainText().contains(QStringLiteral("dragged text")),
             qPrintable(editor->toPlainText()));
}

void TestMainWindow::headingLevelsCollapseIntoOneToolbarButton()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QToolBar *toolBar = window.findChild<QToolBar *>(QStringLiteral("toolBar"));
    QAction *headingMenu = window.findChild<QAction *>(QStringLiteral("actionHeadingMenu"));
    QVERIFY(toolBar && headingMenu);

    // One button in the toolbar, four levels hanging off it.
    QVERIFY(toolBar->actions().contains(headingMenu));
    QVERIFY(!toolBar->actions().contains(
        window.findChild<QAction *>(QStringLiteral("actionHeading1"))));

    QVERIFY2(headingMenu->menu(), "the toolbar action must carry the level menu");
    QCOMPARE(headingMenu->menu()->actions().size(), 4);

    QToolButton *button = qobject_cast<QToolButton *>(toolBar->widgetForAction(headingMenu));
    QVERIFY2(button, "the toolbar must have built a tool button for the action");
    QCOMPARE(button->popupMode(), QToolButton::InstantPopup);
}

void TestMainWindow::headingShortcutsSurviveTheMove()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    // Moving the levels off the toolbar must not cost them their shortcuts;
    // they keep their scope through the Format menu.
    for (int level = 1; level <= 4; ++level) {
        QAction *action = window.findChild<QAction *>(
            QStringLiteral("actionHeading%1").arg(level));
        QVERIFY2(action, qPrintable(QStringLiteral("missing level %1").arg(level)));
        QCOMPARE(action->shortcut(), QKeySequence(QStringLiteral("Ctrl+%1").arg(level)));
        QVERIFY(action->isEnabled());
    }
}

void TestMainWindow::headingLevelsStillInsertTheirMarker()
{
    MainWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QPlainTextEdit *editor = window.findChild<QPlainTextEdit *>(QStringLiteral("editor"));
    QVERIFY(editor);

    for (int level = 1; level <= 4; ++level) {
        editor->setPlainText(QStringLiteral("Title"));

        QAction *action = window.findChild<QAction *>(
            QStringLiteral("actionHeading%1").arg(level));
        QVERIFY(action);
        action->trigger();

        QCOMPARE(editor->toPlainText(),
                 QString(level, QLatin1Char('#')) + QStringLiteral(" Title"));
    }
}

QTEST_MAIN(TestMainWindow)

#include "tst_mainwindow.moc"
