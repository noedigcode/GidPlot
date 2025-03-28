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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "aboutdialog.h"
#include "csv.h"
#include "CsvImportDialog.h"
#include "ProgressDialog.h"
#include "QCustomPlot/qcustomplot.h"
#include "plotwidget.h"
#include "tablewidget.h"
#include "utils.h"

#include <QFileDialog>
#include <QMainWindow>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    struct Args {
        QString csvFilePath;
    };

    explicit MainWindow(Args args, QWidget *parent = 0);
    ~MainWindow();

private slots:
    void importCsv(Csv::FileInfo info);

    PlotWidget* addPlot(QString title);
    void onPlotAdded(PlotWidget* plot);
    void onPlotRemoved(PlotWidget* plot);
    void addTable(CsvPtr csv);
    void onTablePlot(CsvWeakPtr csvWkPtr, bool newPlot, int ixcol,
                     QList<int> iycols, Range range);

    void plot(CsvPtr csv, int ixcol, QList<int> iycols, Range range);

    void csvImportFinished(CsvPtr csv);
    void csvImportProgress(CsvPtr csv, QString info);

    void on_pushButton_importCsv_clicked();
    void on_action_testPlot_triggered();
    void on_action_importCsv_triggered();
    void onTabCustomContextMenuRequested(const QPoint &pos);

    void on_action_About_triggered();

private:
    Ui::MainWindow *ui;
    CsvImportDialog csvImportDialog {this};
    ProgressDialog progressDialog {this};
    AboutDialog aboutDialog {"", this};

    QList<PlotWidget*> mPlots;
    int mPlotCounter = 0;
    QMap<PlotWidget*, int> mPlotLinkGroups;

    QList<TableWidget*> mTables;

    QMenu tabMenu;

    CsvImporter csvImporter;
    QThread thread;

    void msgBox(QString msg);

    void closeEvent(QCloseEvent* event) override;
};

#endif // MAINWINDOW_H
