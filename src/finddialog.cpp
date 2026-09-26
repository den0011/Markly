#include "finddialog.h"
#include "ui_finddialog.h"

#include "editor.h"

#include <QPushButton>
#include <QTextCursor>
#include <QTextDocument>

FindDialog::FindDialog(Editor *editor, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::FindDialog)
    , m_editor(editor)
{
    m_ui->setupUi(this);
    setWindowModality(Qt::NonModal);

    connect(m_ui->findNextButton, &QPushButton::clicked, this, &FindDialog::findNext);
    connect(m_ui->findPreviousButton, &QPushButton::clicked, this, &FindDialog::findPrevious);
    connect(m_ui->replaceButton, &QPushButton::clicked, this, &FindDialog::replaceOne);
    connect(m_ui->replaceAllButton, &QPushButton::clicked, this, &FindDialog::replaceAll);
    connect(m_ui->searchEdit, &QLineEdit::returnPressed, this, &FindDialog::findNext);
    connect(m_ui->replaceEdit, &QLineEdit::returnPressed, this, &FindDialog::replaceOne);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &FindDialog::close);

    // A fresh search term invalidates whatever "not found" message is showing.
    connect(m_ui->searchEdit, &QLineEdit::textChanged, this, [this] { showStatus(QString(), false); });
}

FindDialog::~FindDialog() = default;

void FindDialog::setSearchText(const QString &text)
{
    if (!text.isEmpty())
        m_ui->searchEdit->setText(text);
    m_ui->searchEdit->selectAll();
}

void FindDialog::focusSearchField()
{
    m_ui->searchEdit->setFocus();
}

bool FindDialog::find(bool backward)
{
    const QString term = m_ui->searchEdit->text();
    if (term.isEmpty())
        return false;

    QTextDocument::FindFlags flags;
    if (backward)
        flags |= QTextDocument::FindBackward;
    if (m_ui->caseCheck->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_ui->wholeWordCheck->isChecked())
        flags |= QTextDocument::FindWholeWords;

    if (m_editor->find(term, flags)) {
        showStatus(QString(), false);
        return true;
    }

    // Try once more from the far end of the document, so a match beyond the
    // cursor's starting point is not reported as missing.
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(backward ? QTextCursor::End : QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    if (m_editor->find(term, flags)) {
        showStatus(QString(), false);
        return true;
    }

    showStatus(tr("Phrase not found."), true);
    return false;
}

void FindDialog::findNext()
{
    find(false);
}

void FindDialog::findPrevious()
{
    find(true);
}

void FindDialog::replaceOne()
{
    QTextCursor cursor = m_editor->textCursor();
    const Qt::CaseSensitivity sensitivity =
        m_ui->caseCheck->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    // Only replace when the current selection is itself a match: the dialog
    // may have just been opened, with nothing found yet.
    if (cursor.hasSelection()
        && cursor.selectedText().compare(m_ui->searchEdit->text(), sensitivity) == 0) {
        cursor.insertText(m_ui->replaceEdit->text());
        m_editor->setTextCursor(cursor);
    }

    findNext();
}

void FindDialog::replaceAll()
{
    const QString term = m_ui->searchEdit->text();
    if (term.isEmpty())
        return;

    QTextDocument::FindFlags flags;
    if (m_ui->caseCheck->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_ui->wholeWordCheck->isChecked())
        flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    // One undo step for the whole pass, rather than one per replacement.
    QTextCursor editBlock(m_editor->document());
    editBlock.beginEditBlock();

    int count = 0;
    while (m_editor->find(term, flags)) {
        m_editor->textCursor().insertText(m_ui->replaceEdit->text());
        ++count;
    }

    editBlock.endEditBlock();

    showStatus(count > 0 ? tr("Replaced %1 occurrence(s).").arg(count)
                          : tr("Phrase not found."),
               count == 0);
}

void FindDialog::showStatus(const QString &text, bool isError)
{
    m_ui->statusLabel->setText(text);
    m_ui->statusLabel->setStyleSheet(isError ? QStringLiteral("color: #c0392b;") : QString());
}
