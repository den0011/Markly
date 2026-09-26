#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QScopedPointer>

QT_BEGIN_NAMESPACE
namespace Ui { class FindDialog; }
QT_END_NAMESPACE

class Editor;

// Non-modal find/replace bar for the editor. MainWindow keeps a single
// instance alive for the whole session, so the search term and option
// checkboxes survive between openings instead of resetting every time.
class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(Editor *editor, QWidget *parent = nullptr);
    ~FindDialog() override;

    // Replaces the search field's contents, unless `text` is empty: an empty
    // selection when the dialog is reopened should not clear what was typed
    // last time.
    void setSearchText(const QString &text);
    void focusSearchField();

private slots:
    void findNext();
    void findPrevious();
    void replaceOne();
    void replaceAll();

private:
    bool find(bool backward);
    void showStatus(const QString &text, bool isError);

    QScopedPointer<Ui::FindDialog> m_ui;
    Editor *m_editor;
};

#endif // FINDDIALOG_H
