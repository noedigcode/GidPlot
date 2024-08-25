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

#ifndef TABLEWIDGET_H
#define TABLEWIDGET_H

#include "csv.h"

#include <QMenu>
#include <QWidget>
#include <QTableWidgetItem>
#include <QTreeWidget>


namespace Ui {
class TableWidget;
}

class TableWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TableWidget(QWidget *parent = 0);
    ~TableWidget();

    void setData(CsvPtr csv);

signals:
    void addToPlot(int ixcol, QList<int> iycols, Range range);

private slots:
    void setRangeToAll();

    void on_toolButton_errors_value_prev_clicked();
    void on_toolButton_errors_value_next_clicked();
    void on_toolButton_errors_insufCols_prev_clicked();
    void on_toolButton_errors_insufCols_next_clicked();
    void on_toolButton_errors_excessCols_prev_clicked();
    void on_toolButton_errors_excessCols_next_clicked();
    void on_toolButton_errors_total_prev_clicked();
    void on_toolButton_errors_total_next_clicked();
    void on_pushButton_addToPlot_clicked();
    void on_treeWidget_cols_x_itemSelectionChanged();
    void on_treeWidget_cols_y_itemSelectionChanged();
    void on_toolButton_range_clicked();

private:
    Ui::TableWidget *ui;

    CsvPtr mCsv;

    void loadVisibleRows();
    void loadRows(int from, int to);
    void selectDefaultColumns();

    enum ErrorType {All, ValueError, ExcessCols, InsufCols};
    void showNextError(ErrorType errorType, bool nextNotPrev);

    struct ColSelection
    {
        QList<int> xcols;
        QList<int> ycols;
    };
    ColSelection getColumnsSelection();
    QList<int> getTreeWidgetTopLevelItemSelection(QTreeWidget* treeWidget);
    void updateWidgetsOnColumnSelectionChange();

    void setRange(RangePtr range);

    void resizeEvent(QResizeEvent* event);
};

#endif // TABLEWIDGET_H
