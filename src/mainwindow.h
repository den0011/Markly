#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScopedPointer>
#include <QStringList>

#include "markdownparser.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QLabel;
class QTextDocument;
class QTimer;

class FindDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool openFile(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

    // Retranslates everything that setupUi() does not own when the active
    // language changes.
    void changeEvent(QEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    // Catches file drops over the editor and preview panes before their own
    // drop handling does.
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void newDocument();
    void openDocument();
    bool saveDocument();
    bool saveDocumentAs();
    void printDocument();
    void showPrintPreview();
    void exportHtml();
    void exportPdf();
    void openRecentFile();

    void onTextChanged();
    void refreshPreview();
    void updateStatusBar();
    void onModificationChanged(bool modified);

    void insertLink();
    void insertImage();
    void insertTable();
    void showFindDialog();

    void applyViewMode();
    void toggleTheme(bool dark);
    void syncPreviewToEditor();

private:
    enum ViewMode { EditorOnly, SplitView, PreviewOnly };

    // The widget tree, actions, menus and toolbar all come from
    // ui/mainwindow.ui; these only wire behaviour onto them.
    void connectFileActions();
    void connectEditActions();
    void connectFormatActions();
    void connectViewActions();
    void connectLanguageActions();
    void setupStatusBar();
    void setupFileDrop();
    void applyActionIcons();
    void openDroppedDocument(const QString &path);

    void loadSettings();
    void saveSettings();

    bool maybeSave();
    bool writeToFile(const QString &path);
    void setCurrentFile(const QString &path);
    void addToRecentFiles(const QString &path);
    void updateRecentFilesMenu();
    void updateWindowTitle();

    static QString loadStyleSheet(const char *resource);

    // Kept as a function rather than a constant so that lupdate can extract it:
    // lupdate only sees string literals passed directly to tr().
    static QString markdownFilter();

    // Builds the document shared by Print, Print Preview and Export PDF:
    // always the light theme, regardless of the on-screen preview, so a dark
    // preview never sends dark-on-dark text to paper.
    void preparePrintDocument(QTextDocument *document) const;

    QScopedPointer<Ui::MainWindow> m_ui;
    QTimer *m_previewTimer;
    MarkdownParser m_parser;
    FindDialog *m_findDialog = nullptr;

    QString m_currentFile;
    QStringList m_recentFiles;
    ViewMode m_viewMode = SplitView;
    bool m_darkTheme = false;
    bool m_syncScroll = true;
    bool m_syncing = false;

    QLabel *m_statsLabel = nullptr;
    QLabel *m_fileLabel = nullptr;
};

#endif // MAINWINDOW_H
