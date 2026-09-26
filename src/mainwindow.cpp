#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "actionicons.h"
#include "editor.h"
#include "finddialog.h"
#include "imagedialog.h"
#include "linkdialog.h"
#include "previewview.h"
#include "tabledialog.h"
#include "translationmanager.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTextDocument>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

namespace {

const int kMaxRecentFiles = 8;
const int kPreviewDelayMs = 220;

// `background` and `text` must match `body { background-color }` and
// `body { color }` in the stylesheet they sit next to: the CSS styles the text,
// the two colours style the widget behind it.
struct PreviewTheme
{
    const char *styleSheet;
    const char *background;
    const char *text;
};

const PreviewTheme kLightPreview = { ":/preview-light.css", "#ffffff", "#24292f" };
const PreviewTheme kDarkPreview  = { ":/preview-dark.css",  "#0d1117", "#d1d7e0" };

bool isSupportedDocument(const QString &path)
{
    static const QStringList suffixes = QStringList()
        << QStringLiteral("md") << QStringLiteral("markdown")
        << QStringLiteral("mdown") << QStringLiteral("mkd")
        << QStringLiteral("txt");

    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

// The first dragged local file Markly knows how to open, or an empty string.
// Dropping a selection of several files opens only this one.
QString droppedDocument(const QMimeData *mime)
{
    if (!mime || !mime->hasUrls())
        return QString();

    const QList<QUrl> urls = mime->urls();
    for (const QUrl &url : urls) {
        if (!url.isLocalFile())
            continue;

        const QString path = url.toLocalFile();
        if (isSupportedDocument(path))
            return path;
    }

    return QString();
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_ui(new Ui::MainWindow)
    , m_previewTimer(new QTimer(this))
{
    m_ui->setupUi(this);

    m_ui->splitter->setStretchFactor(0, 1);
    m_ui->splitter->setStretchFactor(1, 1);

    // Re-rendering on every keystroke is wasteful on large documents, so the
    // preview trails the editor by a short idle delay.
    m_previewTimer->setSingleShot(true);
    m_previewTimer->setInterval(kPreviewDelayMs);
    connect(m_previewTimer, &QTimer::timeout, this, &MainWindow::refreshPreview);

    connectFileActions();
    connectEditActions();
    connectFormatActions();
    connectViewActions();
    connectLanguageActions();
    setupStatusBar();
    setupFileDrop();
    applyActionIcons();

    connect(m_ui->editor, &Editor::textChanged, this, &MainWindow::onTextChanged);
    connect(m_ui->editor->document(), &QTextDocument::modificationChanged,
            this, &MainWindow::onModificationChanged);
    connect(m_ui->editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::syncPreviewToEditor);

    loadSettings();
    setCurrentFile(QString());
    refreshPreview();
    updateStatusBar();
}

MainWindow::~MainWindow() = default;

void MainWindow::connectFileActions()
{
    connect(m_ui->actionNew, &QAction::triggered, this, &MainWindow::newDocument);
    connect(m_ui->actionOpen, &QAction::triggered, this, &MainWindow::openDocument);
    connect(m_ui->actionSave, &QAction::triggered, this, &MainWindow::saveDocument);
    connect(m_ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveDocumentAs);
    connect(m_ui->actionPrint, &QAction::triggered, this, &MainWindow::printDocument);
    connect(m_ui->actionPrintPreview, &QAction::triggered, this, &MainWindow::showPrintPreview);
    connect(m_ui->actionExportHtml, &QAction::triggered, this, &MainWindow::exportHtml);
    connect(m_ui->actionExportPdf, &QAction::triggered, this, &MainWindow::exportPdf);
    connect(m_ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    connect(m_ui->actionAbout, &QAction::triggered, this, [this] {
        QMessageBox::about(this, tr("About Markly"),
                           tr("<h3>Markly %1</h3>"
                              "<p>A simple, fast Markdown editor and reader with live preview.</p>"
                              "<p>Built with Qt %2.</p>")
                               .arg(QApplication::applicationVersion(),
                                    QLatin1String(qVersion())));
    });
}

void MainWindow::connectEditActions()
{
    Editor *editor = m_ui->editor;

    connect(m_ui->actionUndo, &QAction::triggered, editor, &Editor::undo);
    connect(editor, &Editor::undoAvailable, m_ui->actionUndo, &QAction::setEnabled);

    connect(m_ui->actionRedo, &QAction::triggered, editor, &Editor::redo);
    connect(editor, &Editor::redoAvailable, m_ui->actionRedo, &QAction::setEnabled);

    connect(m_ui->actionCut, &QAction::triggered, editor, &Editor::cut);
    connect(editor, &Editor::copyAvailable, m_ui->actionCut, &QAction::setEnabled);

    connect(m_ui->actionCopy, &QAction::triggered, editor, &Editor::copy);
    connect(editor, &Editor::copyAvailable, m_ui->actionCopy, &QAction::setEnabled);

    connect(m_ui->actionPaste, &QAction::triggered, editor, &Editor::paste);

    connect(m_ui->actionFind, &QAction::triggered, this, &MainWindow::showFindDialog);
}

void MainWindow::connectFormatActions()
{
    Editor *editor = m_ui->editor;

    const auto surround = [this, editor](QAction *action, const QString &marker) {
        connect(action, &QAction::triggered, this, [editor, marker] {
            editor->surroundSelection(marker, marker);
        });
    };

    surround(m_ui->actionBold, QStringLiteral("**"));
    surround(m_ui->actionItalic, QStringLiteral("*"));
    surround(m_ui->actionStrikethrough, QStringLiteral("~~"));
    surround(m_ui->actionInlineCode, QStringLiteral("`"));

    const auto prefix = [this, editor](QAction *action, const QString &marker) {
        connect(action, &QAction::triggered, this, [editor, marker] {
            editor->prefixLines(marker);
        });
    };

    prefix(m_ui->actionHeading1, QStringLiteral("# "));
    prefix(m_ui->actionHeading2, QStringLiteral("## "));
    prefix(m_ui->actionHeading3, QStringLiteral("### "));
    prefix(m_ui->actionHeading4, QStringLiteral("#### "));

    // Designer can neither attach a menu to an action nor reach the tool button
    // that the toolbar creates for it, so the drop-down is assembled here. The
    // levels stay in the Format menu too, which is where their Ctrl+1..4
    // shortcuts get their scope.
    m_ui->actionHeadingMenu->setMenu(m_ui->menuHeading);

    if (QToolButton *button = qobject_cast<QToolButton *>(
            m_ui->toolBar->widgetForAction(m_ui->actionHeadingMenu))) {
        button->setPopupMode(QToolButton::InstantPopup);
    }
    prefix(m_ui->actionBulletList, QStringLiteral("- "));
    prefix(m_ui->actionNumberedList, QStringLiteral("1. "));
    prefix(m_ui->actionBlockquote, QStringLiteral("> "));

    connect(m_ui->actionLink, &QAction::triggered, this, &MainWindow::insertLink);
    connect(m_ui->actionImage, &QAction::triggered, this, &MainWindow::insertImage);
    connect(m_ui->actionTable, &QAction::triggered, this, &MainWindow::insertTable);
}

void MainWindow::insertLink()
{
    LinkDialog dialog(m_currentFile, this);
    dialog.setLinkText(m_ui->editor->textCursor().selectedText());

    if (dialog.exec() == QDialog::Accepted)
        m_ui->editor->insertSnippet(dialog.markdown());
}

void MainWindow::insertImage()
{
    ImageDialog dialog(m_currentFile, this);
    dialog.setAltText(m_ui->editor->textCursor().selectedText());

    if (dialog.exec() == QDialog::Accepted)
        m_ui->editor->insertSnippet(dialog.markdown());
}

void MainWindow::insertTable()
{
    TableDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted)
        m_ui->editor->insertBlockSnippet(dialog.markdown());
}

void MainWindow::showFindDialog()
{
    // One instance for the window's lifetime, so the search term and option
    // checkboxes survive between openings instead of resetting every time.
    if (!m_findDialog)
        m_findDialog = new FindDialog(m_ui->editor, this);

    m_findDialog->setSearchText(m_ui->editor->textCursor().selectedText());
    m_findDialog->show();
    m_findDialog->raise();
    m_findDialog->activateWindow();
    m_findDialog->focusSearchField();
}

void MainWindow::connectViewActions()
{
    m_ui->actionEditorOnly->setData(EditorOnly);
    m_ui->actionSplitView->setData(SplitView);
    m_ui->actionPreviewOnly->setData(PreviewOnly);

    // The three modes are mutually exclusive; Designer cannot express that, so
    // the group is built here.
    QActionGroup *viewGroup = new QActionGroup(this);
    viewGroup->addAction(m_ui->actionEditorOnly);
    viewGroup->addAction(m_ui->actionSplitView);
    viewGroup->addAction(m_ui->actionPreviewOnly);

    connect(viewGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        m_viewMode = ViewMode(action->data().toInt());
        applyViewMode();
    });

    connect(m_ui->actionDarkPreview, &QAction::toggled, this, &MainWindow::toggleTheme);
    connect(m_ui->actionLineNumbers, &QAction::toggled,
            m_ui->editor, &Editor::setShowLineNumbers);

    connect(m_ui->actionWordWrap, &QAction::toggled, this, [this](bool on) {
        m_ui->editor->setLineWrapMode(on ? QPlainTextEdit::WidgetWidth
                                         : QPlainTextEdit::NoWrap);
    });

    connect(m_ui->actionSyncScroll, &QAction::toggled, this, [this](bool on) {
        m_syncScroll = on;
    });

    connect(m_ui->actionZoomIn, &QAction::triggered, m_ui->preview, &PreviewView::zoomIn);
    connect(m_ui->actionZoomOut, &QAction::triggered, m_ui->preview, &PreviewView::zoomOut);
    connect(m_ui->actionResetZoom, &QAction::triggered, m_ui->preview, &PreviewView::resetZoom);
}

void MainWindow::connectLanguageActions()
{
    m_ui->actionLanguageEnglish->setData(TranslationManager::sourceLanguage());
    m_ui->actionLanguageRussian->setData(QStringLiteral("ru"));

    QActionGroup *languageGroup = new QActionGroup(this);
    languageGroup->addAction(m_ui->actionLanguageEnglish);
    languageGroup->addAction(m_ui->actionLanguageRussian);

    const QString active = TranslationManager::instance().currentLanguage();
    for (QAction *action : languageGroup->actions())
        action->setChecked(action->data().toString() == active);

    connect(languageGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        const QString code = action->data().toString();
        if (!TranslationManager::instance().setLanguage(code)) {
            QMessageBox::warning(this, tr("Markly"),
                                 tr("The translation for this language is not available."));
            // Keep the menu in sync with what is actually loaded.
            const QString active = TranslationManager::instance().currentLanguage();
            for (QAction *other : action->actionGroup()->actions())
                other->setChecked(other->data().toString() == active);
            return;
        }
        TranslationManager::rememberLanguage(code);
    });
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        m_ui->retranslateUi(this);

        // These strings are built at runtime, so retranslateUi() cannot reach
        // them.
        updateRecentFilesMenu();
        updateWindowTitle();
        updateStatusBar();
        if (m_currentFile.isEmpty())
            m_fileLabel->setText(tr("Untitled"));
    }

    // The icons are painted in the palette's text colour, so a theme switch
    // would otherwise leave them unreadable.
    if (event->type() == QEvent::PaletteChange)
        applyActionIcons();

    QMainWindow::changeEvent(event);
}

void MainWindow::setupStatusBar()
{
    // Designer has no editor for status bar children, so these stay in code.
    m_fileLabel = new QLabel(this);
    m_statsLabel = new QLabel(this);

    m_ui->statusbar->addWidget(m_fileLabel, 1);
    m_ui->statusbar->addPermanentWidget(m_statsLabel);
}

void MainWindow::applyActionIcons()
{
    using Kind = ActionIcons::Kind;

    const struct { QAction *action; Kind kind; } bindings[] = {
        { m_ui->actionNew,           Kind::New },
        { m_ui->actionOpen,          Kind::Open },
        { m_ui->actionSave,          Kind::Save },
        { m_ui->actionSaveAs,        Kind::SaveAs },
        { m_ui->actionPrint,         Kind::Print },
        { m_ui->actionPrintPreview,  Kind::PrintPreview },
        { m_ui->actionExportHtml,    Kind::ExportHtml },
        { m_ui->actionExportPdf,     Kind::ExportPdf },
        { m_ui->actionExit,          Kind::Exit },
        { m_ui->actionUndo,          Kind::Undo },
        { m_ui->actionRedo,          Kind::Redo },
        { m_ui->actionCut,           Kind::Cut },
        { m_ui->actionCopy,          Kind::Copy },
        { m_ui->actionPaste,         Kind::Paste },
        { m_ui->actionFind,          Kind::ResetZoom },
        { m_ui->actionBold,          Kind::Bold },
        { m_ui->actionItalic,        Kind::Italic },
        { m_ui->actionStrikethrough, Kind::Strikethrough },
        { m_ui->actionInlineCode,    Kind::InlineCode },
        { m_ui->actionHeadingMenu,   Kind::Heading },
        { m_ui->actionHeading1,      Kind::Heading1 },
        { m_ui->actionHeading2,      Kind::Heading2 },
        { m_ui->actionHeading3,      Kind::Heading3 },
        { m_ui->actionHeading4,      Kind::Heading4 },
        { m_ui->actionBulletList,    Kind::BulletList },
        { m_ui->actionNumberedList,  Kind::NumberedList },
        { m_ui->actionBlockquote,    Kind::Blockquote },
        { m_ui->actionLink,          Kind::Link },
        { m_ui->actionImage,         Kind::Image },
        { m_ui->actionTable,         Kind::Table },
        { m_ui->actionEditorOnly,    Kind::EditorOnly },
        { m_ui->actionSplitView,     Kind::SplitView },
        { m_ui->actionPreviewOnly,   Kind::PreviewOnly },
        { m_ui->actionDarkPreview,   Kind::DarkPreview },
        { m_ui->actionLineNumbers,   Kind::LineNumbers },
        { m_ui->actionWordWrap,      Kind::WordWrap },
        { m_ui->actionSyncScroll,    Kind::SyncScroll },
        { m_ui->actionZoomIn,        Kind::ZoomIn },
        { m_ui->actionZoomOut,       Kind::ZoomOut },
        { m_ui->actionResetZoom,     Kind::ResetZoom },
        { m_ui->actionAbout,         Kind::About },
    };

    for (const auto &binding : bindings)
        binding.action->setIcon(ActionIcons::icon(binding.kind, palette()));
}

void MainWindow::setupFileDrop()
{
    setAcceptDrops(true);

    // Both panes' viewports accept drops themselves, and Qt does not pass a
    // drag the viewport rejects on to the parent widget, so the main window
    // would never see it. Filtering their events is what makes the whole
    // window, and not just its frame, a drop target.
    m_ui->editor->viewport()->installEventFilter(this);
    m_ui->preview->viewport()->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::Drop: {
        // QDragEnterEvent and QDragMoveEvent both derive from QDropEvent.
        QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
        const QString path = droppedDocument(dropEvent->mimeData());

        // Anything that is not a file we can open — dragging text inside the
        // editor, most importantly — stays the pane's business.
        if (path.isEmpty())
            break;

        dropEvent->acceptProposedAction();
        if (event->type() == QEvent::Drop)
            openDroppedDocument(path);
        return true;
    }
    default:
        break;
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (!droppedDocument(event->mimeData()).isEmpty())
        event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (!droppedDocument(event->mimeData()).isEmpty())
        event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QString path = droppedDocument(event->mimeData());
    if (path.isEmpty())
        return;

    event->acceptProposedAction();
    openDroppedDocument(path);
}

void MainWindow::openDroppedDocument(const QString &path)
{
    raise();
    activateWindow();

    // The drag source stays blocked until the drop handler returns, so the
    // "save your changes?" prompt must not run inside it.
    QTimer::singleShot(0, this, [this, path] {
        if (maybeSave())
            openFile(path);
    });
}

void MainWindow::onTextChanged()
{
    m_previewTimer->start();
    updateStatusBar();
}

void MainWindow::refreshPreview()
{
    m_ui->preview->setHtmlPreserveScroll(m_parser.toHtml(m_ui->editor->toPlainText()));
}

void MainWindow::updateStatusBar()
{
    const QString text = m_ui->editor->toPlainText();

    static const QRegularExpression wordSeparator(QStringLiteral("\\s+"));
    const int words = text.trimmed().isEmpty()
        ? 0
        : text.trimmed().split(wordSeparator).size();

    m_statsLabel->setText(tr("%1 words   %2 chars   %3 lines")
                              .arg(words)
                              .arg(text.length())
                              .arg(m_ui->editor->blockCount()));
}

void MainWindow::onModificationChanged(bool modified)
{
    setWindowModified(modified);
    updateWindowTitle();
}

void MainWindow::syncPreviewToEditor()
{
    if (!m_syncScroll || m_viewMode != SplitView || m_syncing)
        return;

    const QScrollBar *bar = m_ui->editor->verticalScrollBar();
    if (bar->maximum() <= bar->minimum())
        return;

    m_syncing = true;
    m_ui->preview->setScrollRatio(double(bar->value() - bar->minimum())
                                  / double(bar->maximum() - bar->minimum()));
    m_syncing = false;
}

void MainWindow::applyViewMode()
{
    m_ui->editor->setVisible(m_viewMode != PreviewOnly);
    m_ui->preview->setVisible(m_viewMode != EditorOnly);

    if (m_viewMode == SplitView) {
        const int half = qMax(200, m_ui->splitter->width() / 2);
        m_ui->splitter->setSizes(QList<int>() << half << half);
    }
}

void MainWindow::toggleTheme(bool dark)
{
    m_darkTheme = dark;

    const PreviewTheme &theme = dark ? kDarkPreview : kLightPreview;
    m_ui->preview->setTheme(loadStyleSheet(theme.styleSheet),
                            QColor(QLatin1String(theme.background)),
                            QColor(QLatin1String(theme.text)));
}

QString MainWindow::markdownFilter()
{
    return tr("Markdown (*.md *.markdown *.mdown *.mkd *.txt);;All Files (*)");
}

QString MainWindow::loadStyleSheet(const char *resource)
{
    QFile file{QLatin1String(resource)};
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();
    return QString::fromUtf8(file.readAll());
}

void MainWindow::newDocument()
{
    if (!maybeSave())
        return;

    m_ui->editor->clear();
    setCurrentFile(QString());
    refreshPreview();
}

void MainWindow::openDocument()
{
    if (!maybeSave())
        return;

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Markdown File"),
        m_currentFile.isEmpty() ? QString() : QFileInfo(m_currentFile).absolutePath(),
        markdownFilter());

    if (!path.isEmpty())
        openFile(path);
}

bool MainWindow::openFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Markly"),
                             tr("Cannot read %1:\n%2.")
                                 .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    m_ui->editor->setPlainText(stream.readAll());

    setCurrentFile(path);
    addToRecentFiles(path);
    refreshPreview();
    m_ui->statusbar->showMessage(tr("Opened %1").arg(QFileInfo(path).fileName()), 3000);
    return true;
}

bool MainWindow::saveDocument()
{
    if (m_currentFile.isEmpty())
        return saveDocumentAs();
    return writeToFile(m_currentFile);
}

bool MainWindow::saveDocumentAs()
{
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save Markdown File"),
        m_currentFile.isEmpty() ? QStringLiteral("untitled.md") : m_currentFile,
        markdownFilter());

    if (path.isEmpty())
        return false;

    return writeToFile(path);
}

bool MainWindow::writeToFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Markly"),
                             tr("Cannot write %1:\n%2.")
                                 .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << m_ui->editor->toPlainText();
    file.close();

    setCurrentFile(path);
    addToRecentFiles(path);
    m_ui->statusbar->showMessage(tr("Saved %1").arg(QFileInfo(path).fileName()), 3000);
    return true;
}

void MainWindow::preparePrintDocument(QTextDocument *document) const
{
    document->setDefaultStyleSheet(loadStyleSheet(kLightPreview.styleSheet));
    document->setHtml(m_parser.toHtml(m_ui->editor->toPlainText()));
    document->setDocumentMargin(20);

    if (!m_currentFile.isEmpty()) {
        document->setBaseUrl(QUrl::fromLocalFile(
            QFileInfo(m_currentFile).absolutePath() + QLatin1Char('/')));
    }
}

void MainWindow::printDocument()
{
    QTextDocument document;
    preparePrintDocument(&document);

    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(m_currentFile.isEmpty() ? tr("Untitled")
                                               : QFileInfo(m_currentFile).completeBaseName());

    QPrintDialog dialog(&printer, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    document.print(&printer);
    m_ui->statusbar->showMessage(tr("Sent to printer"), 3000);
}

void MainWindow::showPrintPreview()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setDocName(m_currentFile.isEmpty() ? tr("Untitled")
                                               : QFileInfo(m_currentFile).completeBaseName());

    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, [this](QPrinter *target) {
        QTextDocument document;
        preparePrintDocument(&document);
        document.print(target);
    });
    preview.exec();
}

void MainWindow::exportHtml()
{
    const QString suggested = m_currentFile.isEmpty()
        ? QStringLiteral("untitled.html")
        : QFileInfo(m_currentFile).absolutePath() + QLatin1Char('/')
              + QFileInfo(m_currentFile).completeBaseName() + QStringLiteral(".html");

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export HTML"), suggested, tr("HTML (*.html *.htm);;All Files (*)"));

    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Markly"),
                             tr("Cannot write %1:\n%2.")
                                 .arg(QDir::toNativeSeparators(path), file.errorString()));
        return;
    }

    const QString title = m_currentFile.isEmpty()
        ? tr("Untitled")
        : QFileInfo(m_currentFile).completeBaseName();

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << m_parser.toHtmlDocument(m_ui->editor->toPlainText(), title,
                                      m_ui->preview->styleSheetSource());

    m_ui->statusbar->showMessage(tr("Exported %1").arg(QFileInfo(path).fileName()), 3000);
}

void MainWindow::exportPdf()
{
    const QString suggested = m_currentFile.isEmpty()
        ? QStringLiteral("untitled.pdf")
        : QFileInfo(m_currentFile).absolutePath() + QLatin1Char('/')
              + QFileInfo(m_currentFile).completeBaseName() + QStringLiteral(".pdf");

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Export PDF"), suggested, tr("PDF (*.pdf);;All Files (*)"));

    if (path.isEmpty())
        return;

    QTextDocument document;
    preparePrintDocument(&document);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    document.print(&printer);

    m_ui->statusbar->showMessage(tr("Exported %1").arg(QFileInfo(path).fileName()), 3000);
}

void MainWindow::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action || !maybeSave())
        return;

    const QString path = action->data().toString();
    if (!QFile::exists(path)) {
        m_recentFiles.removeAll(path);
        updateRecentFilesMenu();
        QMessageBox::warning(this, tr("Markly"),
                             tr("%1 no longer exists.").arg(QDir::toNativeSeparators(path)));
        return;
    }

    openFile(path);
}

void MainWindow::setCurrentFile(const QString &path)
{
    m_currentFile = path;
    m_ui->editor->document()->setModified(false);
    setWindowModified(false);

    m_ui->preview->setDocumentDirectory(path.isEmpty() ? QString()
                                                       : QFileInfo(path).absolutePath());
    m_fileLabel->setText(path.isEmpty() ? tr("Untitled")
                                        : QDir::toNativeSeparators(path));
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    const QString name = m_currentFile.isEmpty()
        ? tr("Untitled")
        : QFileInfo(m_currentFile).fileName();

    setWindowTitle(tr("%1[*] - Markly").arg(name));
}

void MainWindow::addToRecentFiles(const QString &path)
{
    const QString absolute = QFileInfo(path).absoluteFilePath();
    m_recentFiles.removeAll(absolute);
    m_recentFiles.prepend(absolute);
    while (m_recentFiles.size() > kMaxRecentFiles)
        m_recentFiles.removeLast();

    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
    QMenu *menu = m_ui->menuRecent;
    menu->clear();
    menu->setEnabled(!m_recentFiles.isEmpty());

    for (int i = 0; i < m_recentFiles.size(); ++i) {
        const QString path = m_recentFiles.at(i);
        QAction *action = menu->addAction(
            tr("&%1  %2").arg(i + 1).arg(QFileInfo(path).fileName()));
        action->setData(path);
        action->setStatusTip(QDir::toNativeSeparators(path));
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    }

    if (!m_recentFiles.isEmpty()) {
        menu->addSeparator();
        QAction *clear = menu->addAction(tr("&Clear List"));
        connect(clear, &QAction::triggered, this, [this] {
            m_recentFiles.clear();
            updateRecentFilesMenu();
        });
    }
}

bool MainWindow::maybeSave()
{
    if (!m_ui->editor->document()->isModified())
        return true;

    const QMessageBox::StandardButton answer = QMessageBox::warning(
        this, tr("Markly"),
        tr("The document has been modified.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (answer == QMessageBox::Save)
        return saveDocument();

    return answer != QMessageBox::Cancel;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!maybeSave()) {
        event->ignore();
        return;
    }

    saveSettings();
    event->accept();
}

void MainWindow::loadSettings()
{
    QSettings settings;

    if (!restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray()))
        resize(1200, 780);
    restoreState(settings.value(QStringLiteral("window/state")).toByteArray());

    m_recentFiles = settings.value(QStringLiteral("files/recent")).toStringList();
    updateRecentFilesMenu();

    m_viewMode = ViewMode(settings.value(QStringLiteral("view/mode"), SplitView).toInt());
    switch (m_viewMode) {
    case EditorOnly:  m_ui->actionEditorOnly->setChecked(true);  break;
    case PreviewOnly: m_ui->actionPreviewOnly->setChecked(true); break;
    case SplitView:   m_ui->actionSplitView->setChecked(true);   break;
    }
    applyViewMode();

    const QByteArray splitterState =
        settings.value(QStringLiteral("view/splitter")).toByteArray();
    if (!splitterState.isEmpty())
        m_ui->splitter->restoreState(splitterState);

    m_ui->actionDarkPreview->setChecked(
        settings.value(QStringLiteral("view/darkPreview"), false).toBool());
    // toggled() only fires on a real change, so seed the stylesheet explicitly.
    toggleTheme(m_ui->actionDarkPreview->isChecked());

    m_ui->actionLineNumbers->setChecked(
        settings.value(QStringLiteral("editor/lineNumbers"), true).toBool());
    m_ui->actionWordWrap->setChecked(
        settings.value(QStringLiteral("editor/wordWrap"), true).toBool());
    m_ui->actionSyncScroll->setChecked(
        settings.value(QStringLiteral("view/syncScroll"), true).toBool());
}

void MainWindow::saveSettings()
{
    QSettings settings;

    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
    settings.setValue(QStringLiteral("files/recent"), m_recentFiles);
    settings.setValue(QStringLiteral("view/mode"), int(m_viewMode));
    settings.setValue(QStringLiteral("view/splitter"), m_ui->splitter->saveState());
    settings.setValue(QStringLiteral("view/darkPreview"), m_darkTheme);
    settings.setValue(QStringLiteral("view/syncScroll"), m_syncScroll);
    settings.setValue(QStringLiteral("editor/lineNumbers"), m_ui->editor->showLineNumbers());
    settings.setValue(QStringLiteral("editor/wordWrap"),
                      m_ui->editor->lineWrapMode() == QPlainTextEdit::WidgetWidth);
}
