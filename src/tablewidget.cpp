/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2024 Gideon van der Kolf
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#include "tablewidget.h"
#include "ui_tablewidget.h"

#include <QFileInfo>
#include <QScrollBar>
#include <QDebug>

TableWidget::TableWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TableWidget)
{
    ui->setupUi(this);

    ui->tabWidget->setCurrentWidget(ui->tab_plot);
    updateWidgetsOnColumnSelectionChange();
}

TableWidget::~TableWidget()
{
    delete ui;
}

void TableWidget::setData(CsvPtr csv)
{
    mCsv = csv;
    if (!mCsv) { return; }
    MatrixPtr mat = csv->matrix;

    // Fill column tree widgets
    foreach (QString heading, mat->headings()) {
        ui->treeWidget_cols_x->addTopLevelItem(new QTreeWidgetItem({heading}));
        ui->treeWidget_cols_y->addTopLevelItem(new QTreeWidgetItem({heading}));
    }
    selectDefaultColumns();

    setRangeToAll();

    // Create table widget
    QTableWidget* w = ui->tableWidget;
    connect(w->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [=](int /*value*/)
    {
        loadVisibleRows();
    });

    w->setRowCount(mat->rowCount());
    w->setColumnCount(mat->colCount());

    // Set headings
    w->setHorizontalHeaderLabels(mat->headings());

    loadVisibleRows();

    // Update info in GUI

    ui->lineEdit_filename->setText(csv->fileInfo.filename);
    ui->label_loadtime->setText(QString("Loaded in %1 s")
                                    .arg(csv->importInfo.timeSecs));
    ui->label_filesize->setText(QLocale().formattedDataSize(
        QFileInfo(csv->fileInfo.filename).size()));
    ui->label_colsrows->setText(QString("%1 columns, %2 rows")
                                    .arg(mat->colCount())
                                    .arg(mat->rowCount()));

    ui->label_errors_total->setText(QString("%1 errors").arg(mat->errorCount));
    ui->label_errors_value->setText(QString("%1 value errors")
                                    .arg(mat->valueConversionErrorCount));
    ui->label_errors_excessCols->setText(QString("%1 excess column errors")
                                         .arg(mat->excessColsErrorCount));
    ui->label_errors_insufCols->setText(QString("%1 insufficient column errors")
                                        .arg(mat->insufficientColsErrorCount));
}

void TableWidget::setRangeToAll()
{
    if (!mCsv) { return; }
    ui->lineEdit_range->setText(
        Range("All", 0, mCsv->matrix->rowCount()).toString());
}

void TableWidget::loadVisibleRows()
{
    QTableWidget* w = ui->tableWidget; // Shorthand

    if (w->rowCount() == 0) { return; }
    int visibleCount = w->viewport()->height() / w->rowHeight(0);
    int rowFirst = w->verticalScrollBar()->value();
    int rowLast = rowFirst + visibleCount;

    loadRows(rowFirst, rowLast);
}

void TableWidget::loadRows(int from, int to)
{
    if (!mCsv) { return; }
    MatrixPtr mat = mCsv->matrix;
    QTableWidget* w = ui->tableWidget;

    to = qMin(to, mat->rowCount() - 1);

    for (int icol = 0; icol < mat->colCount(); icol++) {

        QVector<double> &col = mat->data[icol];
        QVector<Matrix::MetaDataPtr> &metaCol = mat->metaData[icol];

        for (int irow = from; irow <= to; irow++) {

            if (w->item(irow, icol)) { continue; } // Skip those already filled

            QTableWidgetItem* cell = new QTableWidgetItem(QString::number(col[irow]));
            cell->setFlags(cell->flags() & ~Qt::ItemIsEditable);
            Matrix::MetaDataPtr md = metaCol[irow];
            if (md) {
                cell->setBackgroundColor(Qt::red);
                cell->setToolTip(md->errorString());
                cell->setText(md->originalValue);
            }
            w->setItem(irow, icol, cell);
        }
    }
}

void TableWidget::selectDefaultColumns()
{
    if (!mCsv) { return; }
    MatrixPtr mat = mCsv->matrix;

    // Try to select time for x-axis, otherwise index column

    // Prefer a column heading with one of these exact names
    QStringList exact {"time", "t"};
    // Otherwise opt for one that contains one of these
    QStringList contain {"time"};

    int iExact = -1;
    int iContain = -1;
    for (int i = 0; i < mat->headingCount(); i++) {
        QString h = mat->heading(i).trimmed().toLower();
        foreach (QString x, exact) {
            if (h == x) {
                iExact = i;
                break;
            }
        }
        foreach (QString c, contain) {
            if ( (iContain < 0) && (h.contains(c)) ) {
                iContain = i;
            }
        }
    }
    int iSelect = 0;
    if (iExact >= 0) {
        // Select exact match
        iSelect = iExact;
    } else if (iContain >= 0) {
        // Select semi match
        iSelect = iContain;
    }
    ui->treeWidget_cols_x->setCurrentItem(
                ui->treeWidget_cols_x->topLevelItem(iSelect));

    // Select something else for y-axis

    iSelect = (iSelect + 1) % ui->treeWidget_cols_y->topLevelItemCount();
    ui->treeWidget_cols_y->setCurrentItem(
                ui->treeWidget_cols_y->topLevelItem(iSelect));
}

void TableWidget::showNextError(ErrorType errorType, bool nextNotPrev)
{
    if (!mCsv) { return; }
    if (ui->tableWidget->columnCount() == 0) { return; }
    if (ui->tableWidget->rowCount() == 0) { return; }

    int istartrow = ui->tableWidget->currentRow();
    int istartcol = ui->tableWidget->currentColumn();

    int rowCount = mCsv->matrix->rowCount();
    int colCount = mCsv->matrix->colCount();

    if ((istartcol < 0) || (istartrow < 0)) {
        istartcol = 0;
        istartrow = 0;
    } else {
        if (nextNotPrev) {
            ++istartcol;
            if (istartcol >= colCount) {
                istartcol = 0;
                ++istartrow;
            }
        } else {
            --istartcol;
            if (istartcol < 0) {
                istartcol = colCount - 1;
                --istartrow;
            }
        }
    }

    bool found = false;
    int irow = istartrow;
    int icol = istartcol;
    int incr = nextNotPrev ? 1 : -1;
    for (irow = istartrow; (irow >= 0) && (irow < rowCount); irow += incr) {
        if (irow > istartrow) { istartcol = 0; }
        if (irow < istartrow) { istartcol = colCount - 1; }
        for (icol = istartcol; (icol >= 0) && (icol < colCount); icol += incr) {
            Matrix::MetaDataPtr md = mCsv->matrix->metaData[icol][irow];
            if (md) {
                switch (errorType) {
                case All:
                    found = md->hasError();
                    break;
                case ValueError:
                    found = md->valueConversionError;
                    break;
                case ExcessCols:
                    found = md->excessColsError;
                    break;
                case InsufCols:
                    found = md->insufficientColsError;
                    break;
                }
                if (found) { break; }
            }
        }
        if (found) { break; }
    }
    if (found) {
        ui->tableWidget->scrollToItem(ui->tableWidget->item(irow, icol));
        ui->tableWidget->setCurrentCell(irow, icol);
        ui->tableWidget->setFocus();
    }
}

TableWidget::ColSelection TableWidget::getColumnsSelection()
{
    ColSelection selection;

    selection.xcols = getTreeWidgetTopLevelItemSelection(ui->treeWidget_cols_x);
    selection.ycols = getTreeWidgetTopLevelItemSelection(ui->treeWidget_cols_y);

    return selection;
}

QList<int> TableWidget::getTreeWidgetTopLevelItemSelection(QTreeWidget* treeWidget)
{
    QList<int> ret;
    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        if (treeWidget->topLevelItem(i)->isSelected()) {
            ret.append(i);
        }
    }
    return ret;
}

void TableWidget::updateWidgetsOnColumnSelectionChange()
{
    bool xsel = ui->treeWidget_cols_x->selectedItems().count() > 0;
    bool ysel = ui->treeWidget_cols_y->selectedItems().count() > 0;
    ui->pushButton_addToPlot->setEnabled(xsel && ysel);
}

void TableWidget::setRange(RangePtr range)
{
    ui->lineEdit_range->setText(range->toString());
}

void TableWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    loadVisibleRows();
}

void TableWidget::on_toolButton_errors_value_prev_clicked()
{
    showNextError(ValueError, false);
}

void TableWidget::on_toolButton_errors_value_next_clicked()
{
    showNextError(ValueError, true);
}

void TableWidget::on_toolButton_errors_insufCols_prev_clicked()
{
    showNextError(InsufCols, false);
}

void TableWidget::on_toolButton_errors_insufCols_next_clicked()
{
    showNextError(InsufCols, true);
}

void TableWidget::on_toolButton_errors_excessCols_prev_clicked()
{
    showNextError(ExcessCols, false);
}

void TableWidget::on_toolButton_errors_excessCols_next_clicked()
{
    showNextError(ExcessCols, true);
}

void TableWidget::on_toolButton_errors_total_prev_clicked()
{
    showNextError(All, false);
}

void TableWidget::on_toolButton_errors_total_next_clicked()
{
    showNextError(All, true);
}

void TableWidget::on_pushButton_addToPlot_clicked()
{
    ColSelection selection = getColumnsSelection();

    if (selection.xcols.isEmpty()) { return; }
    if (selection.ycols.isEmpty()) { return; }

    Range range;
    if (!range.fromString(ui->lineEdit_range->text())) { return; }

    emit addToPlot(selection.xcols.value(0), selection.ycols, range);
}

void TableWidget::on_treeWidget_cols_x_itemSelectionChanged()
{
    updateWidgetsOnColumnSelectionChange();
}

void TableWidget::on_treeWidget_cols_y_itemSelectionChanged()
{
    updateWidgetsOnColumnSelectionChange();
}

void TableWidget::on_toolButton_range_clicked()
{
    QMenu* menu = new QMenu();
    connect(menu, &QMenu::triggered, this, [=]()
    {
        menu->deleteLater();
    });
    menu->addAction("All", this, &TableWidget::setRangeToAll);
    if (!mCsv) { return; }
    foreach (RangePtr range, mCsv->ranges) {
        menu->addAction(range->name, this, [this, rangeWkPtr = range.toWeakRef()]()
        {
            RangePtr range(rangeWkPtr);
            if (!range) { return; }
            setRange(range);
        });
    }
    menu->popup(QCursor::pos());
}


