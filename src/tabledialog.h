#ifndef TABLEDIALOG_H
#define TABLEDIALOG_H

#include <QDialog>
#include <QScopedPointer>
#include <QVector>

#include "markdownsnippets.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TableDialog; }
QT_END_NAMESPACE

// Grid editor for a GFM pipe table. Headers live in the grid's own header row,
// so the editable area holds body rows only and the two never get confused.
class TableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TableDialog(QWidget *parent = nullptr);
    ~TableDialog() override;

    void setSize(int columns, int rows);
    void setHeader(int column, const QString &text);
    void setCell(int row, int column, const QString &text);
    void setAlignment(int column, MarkdownSnippets::Alignment alignment);

    QString markdown() const;

private slots:
    void addRow();
    void removeRow();
    void addColumn();
    void removeColumn();
    void showAlignmentOfCurrentColumn();
    void applyAlignmentToCurrentColumn(int index);

private:
    int currentColumn() const;
    void updateButtons();

    QScopedPointer<Ui::TableDialog> m_ui;
    QVector<MarkdownSnippets::Alignment> m_alignments;
};

#endif // TABLEDIALOG_H
