#include "tabledialog.h"
#include "ui_tabledialog.h"

#include <QHeaderView>
#include <QTableWidgetItem>

using MarkdownSnippets::Alignment;

namespace {

const int kDefaultColumns = 2;
const int kDefaultRows = 2;

} // namespace

TableDialog::TableDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::TableDialog)
{
    m_ui->setupUi(this);

    m_ui->grid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui->grid->horizontalHeader()->setSectionsClickable(true);

    setSize(kDefaultColumns, kDefaultRows);

    connect(m_ui->addRowButton, &QPushButton::clicked, this, &TableDialog::addRow);
    connect(m_ui->removeRowButton, &QPushButton::clicked, this, &TableDialog::removeRow);
    connect(m_ui->addColumnButton, &QPushButton::clicked, this, &TableDialog::addColumn);
    connect(m_ui->removeColumnButton, &QPushButton::clicked, this, &TableDialog::removeColumn);

    connect(m_ui->grid, &QTableWidget::currentCellChanged,
            this, &TableDialog::showAlignmentOfCurrentColumn);
    connect(m_ui->grid->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &TableDialog::showAlignmentOfCurrentColumn);
    connect(m_ui->alignmentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TableDialog::applyAlignmentToCurrentColumn);

    updateButtons();
}

TableDialog::~TableDialog() = default;

void TableDialog::setSize(int columns, int rows)
{
    m_ui->grid->setColumnCount(qMax(1, columns));
    m_ui->grid->setRowCount(qMax(1, rows));

    m_alignments.resize(m_ui->grid->columnCount());

    for (int c = 0; c < m_ui->grid->columnCount(); ++c) {
        if (!m_ui->grid->horizontalHeaderItem(c))
            setHeader(c, tr("Column %1").arg(c + 1));
    }

    m_ui->grid->setCurrentCell(0, 0);
    updateButtons();
}

void TableDialog::setHeader(int column, const QString &text)
{
    m_ui->grid->setHorizontalHeaderItem(column, new QTableWidgetItem(text));
}

void TableDialog::setCell(int row, int column, const QString &text)
{
    m_ui->grid->setItem(row, column, new QTableWidgetItem(text));
}

void TableDialog::setAlignment(int column, Alignment alignment)
{
    if (column < 0 || column >= m_alignments.size())
        return;

    m_alignments[column] = alignment;
    if (column == currentColumn())
        showAlignmentOfCurrentColumn();
}

int TableDialog::currentColumn() const
{
    return qMax(0, m_ui->grid->currentColumn());
}

QString TableDialog::markdown() const
{
    QStringList headers;
    for (int c = 0; c < m_ui->grid->columnCount(); ++c) {
        const QTableWidgetItem *header = m_ui->grid->horizontalHeaderItem(c);
        headers << (header ? header->text() : QString());
    }

    QVector<QStringList> rows;
    for (int r = 0; r < m_ui->grid->rowCount(); ++r) {
        QStringList cells;
        for (int c = 0; c < m_ui->grid->columnCount(); ++c) {
            const QTableWidgetItem *cell = m_ui->grid->item(r, c);
            cells << (cell ? cell->text() : QString());
        }
        rows << cells;
    }

    return MarkdownSnippets::pipeTable(headers, rows, m_alignments);
}

void TableDialog::addRow()
{
    m_ui->grid->insertRow(m_ui->grid->rowCount());
    updateButtons();
}

void TableDialog::removeRow()
{
    if (m_ui->grid->rowCount() <= 1)
        return;

    const int row = m_ui->grid->currentRow();
    m_ui->grid->removeRow(row >= 0 ? row : m_ui->grid->rowCount() - 1);
    updateButtons();
}

void TableDialog::addColumn()
{
    const int column = m_ui->grid->columnCount();
    m_ui->grid->insertColumn(column);
    m_alignments.append(Alignment::Left);
    setHeader(column, tr("Column %1").arg(column + 1));
    updateButtons();
}

void TableDialog::removeColumn()
{
    if (m_ui->grid->columnCount() <= 1)
        return;

    const int column = currentColumn();
    m_ui->grid->removeColumn(column);
    if (column < m_alignments.size())
        m_alignments.remove(column);
    updateButtons();
}

void TableDialog::showAlignmentOfCurrentColumn()
{
    const int column = currentColumn();
    if (column >= m_alignments.size())
        return;

    // Reflecting the stored value back into the combo must not be mistaken for
    // the user picking one, or moving the cursor would rewrite the alignment.
    const QSignalBlocker blocker(m_ui->alignmentCombo);
    m_ui->alignmentCombo->setCurrentIndex(int(m_alignments.at(column)));
}

void TableDialog::applyAlignmentToCurrentColumn(int index)
{
    const int column = currentColumn();
    if (column < m_alignments.size())
        m_alignments[column] = Alignment(index);
}

void TableDialog::updateButtons()
{
    m_ui->removeRowButton->setEnabled(m_ui->grid->rowCount() > 1);
    m_ui->removeColumnButton->setEnabled(m_ui->grid->columnCount() > 1);
}
