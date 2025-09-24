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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "defer.h"
#include "utils.h"
#include "version.h"

MainWindow::MainWindow(Args args, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle(QString("%1 %2").arg(APP_NAME).arg(APP_VERSION));
    setWindowState(Qt::WindowMaximized);

    // Default to startup page
    ui->stackedWidget->setCurrentWidget(ui->page_startup);

    // Empty tab widget
    ui->tabWidget->removeTab(0);

    connect(&csvImportDialog, &CsvImportDialog::import, this, &MainWindow::importCsv);

    csvImporter.moveToThread(&thread);
    thread.start();
    connect(&csvImporter, &CsvImporter::importFinished,
            this, &MainWindow::csvImportFinished);
    connect(&csvImporter, &CsvImporter::importProgress,
            this, &MainWindow::csvImportProgress);

    if (!args.csvFilePath.isEmpty()) {
        csvImportDialog.setFile(args.csvFilePath);
        csvImportDialog.show();
    }

    ui->tabWidget->tabBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tabWidget->tabBar(), &QTabBar::customContextMenuRequested,
            this, &MainWindow::onTabCustomContextMenuRequested);
}

MainWindow::~MainWindow()
{
    if (thread.isRunning()) {
        thread.quit();
        thread.wait();
    }

    delete ui;
}

void MainWindow::importCsv(Csv::FileInfo info)
{
    progressDialog.setWindowTitle("Import CSV");
    progressDialog.setTitle("Importing CSV file");
    progressDialog.setInfo("");
    progressDialog.preventClose(true);
    progressDialog.show();

    csvImporter.importCsv(info);
    // Now we wait for the CsvImporter signal to trigger csvImportFinished().
}

PlotWindow *MainWindow::addPlot(QString title)
{
    PlotWindow* p = new PlotWindow();
    connect(p, &PlotWindow::destroyed, this, [=]()
    {
        mPlots.removeAll(p);
        mPlotLinkGroups.remove(p);
        QMetaObject::invokeMethod(this, [=](){ onPlotRemoved(p); });
    });
    connect(p, &PlotWindow::linkGroupChanged, this, [=](int group)
    {
        mPlotLinkGroups.insert(p, group);
    });
    connect(p, &PlotWindow::axisRangesChanged, this, [=](QRectF xyrange)
    {
        int group = mPlotLinkGroups.value(p, 0);
        if (group == 0) { return; }
        foreach (PlotWindow* pp, mPlotLinkGroups.keys()) {
            if (pp == p) { continue; }
            if (mPlotLinkGroups.value(pp) == group) {
                pp->syncAxisRanges(xyrange);
            }
        }
    });
    connect(p, &PlotWindow::dataTipChanged, this, [=](int index)
    {
        int group = mPlotLinkGroups.value(p, 0);
        if (group == 0) { return; }
        foreach (PlotWindow* pp, mPlotLinkGroups.keys()) {
            if (pp == p) { continue; }
            if (mPlotLinkGroups.value(pp) == group) {
                pp->syncDataTip(index);
            }
        }
    });
    connect(p, &PlotWindow::dockWindow, this, [=](PlotWindow::Dock location)
    {
        if (!ui->tabWidget->isPoppedOut(p)) {
            if (location == PlotWindow::DockTab) { return; }
            ui->tabWidget->popoutTabWidget(p);
        }
        QMainWindow* window = ui->tabWidget->tabWindow(p);
        if (!window) { return; }

        QScreen* screen = window->screen();
        if (!screen) { return; }
        QRect rs = screen->availableGeometry();

        QRect rf = window->frameGeometry();
        QRect rw = window->geometry();

        switch (location) {
        case PlotWindow::DockTop:
            window->setGeometry(rs.left() + rw.left() - rf.left(),
                                rs.top() + rw.top() - rf.top(),
                                rs.width() - (rf.width() - rw.width()),
                                rs.height() / 2 - (rf.height() - rw.height()));
            break;
        case PlotWindow::DockBottom:
            window->setGeometry(rs.left() + rw.left() - rf.left(),
                                rs.bottom() - rs.height()/2 + (rw.top() - rf.top()),
                                rs.width() - (rf.width() - rw.width()),
                                rs.height() / 2 - (rf.height() - rw.height()));
            break;
        case PlotWindow::DockLeft:
            window->setGeometry(rs.left() + rw.left() - rf.left(),
                                rs.top() + rw.top() - rf.top(),
                                rs.width() / 2 - (rf.width() - rw.width()),
                                rs.height() - (rf.height() - rw.height()));
            break;
        case PlotWindow::DockRight:
            window->setGeometry(rs.right() - rs.width()/2 + rw.left() - rf.left(),
                                rs.top() + rw.top() - rf.top(),
                                rs.width() / 2 - (rf.width() - rw.width()),
                                rs.height() - (rf.height() - rw.height()));
            break;
        case PlotWindow::DockFloating:
            window->setGeometry(rs.left() + rs.width() * 0.1,
                                rs.top() + rs.height() * 0.1,
                                rs.width() * 0.8,
                                rs.height() * 0.8);
            break;
        case PlotWindow::DockTab:
            // Closing window will pop it back into the tab bar
            window->close();
            break;
        }
    });

    connect(p, &PlotWindow::resizeWindow, this, [=](int width, int height)
    {
        ui->tabWidget->popoutTabWidget(p);
        QMainWindow* w = ui->tabWidget->tabWindow(p);

        width += w->width() - p->plotWidget()->width();
        height += w->height() - p->plotWidget()->height();

        w->resize(width, height);
    });

    title = QString("%1. %2").arg(++mPlotCounter).arg(title);
    p->setWindowTitle(title);
    int tab = ui->tabWidget->insertTab(ui->tabWidget->currentIndex() + 1,
                                       p, QIcon(":/plot"), title);

    ui->tabWidget->setCurrentIndex(tab);
    ui->stackedWidget->setCurrentWidget(ui->page_main);

    mPlots.append(p);
    QMetaObject::invokeMethod(this, [=](){ onPlotAdded(p); });
    return p;
}

void MainWindow::onPlotAdded(PlotWindow* /*plot*/)
{
    foreach (TableWidget* t, mTables) {
        t->setAddToPlotButtonEnabled( !mPlots.isEmpty() );
    }
}

void MainWindow::onPlotRemoved(PlotWindow* /*plot*/)
{
    foreach (TableWidget* t, mTables) {
        t->setAddToPlotButtonEnabled( !mPlots.isEmpty() );
    }
}

void MainWindow::addTable(CsvPtr csv)
{
    TableWidget* t = new TableWidget();
    connect(t, &TableWidget::destroyed, this, [=]()
    {
        mTables.removeAll(t);
    });
    mTables.append(t);
    t->setAddToPlotButtonEnabled( !mPlots.isEmpty() );

    CsvWeakPtr csvWkPtr(csv);
    connect(t, &TableWidget::plot,
            this, [this, csvWkPtr]
            (bool newPlot, int ixcol, QList<int> iycols, Range range)
    {
        onTablePlot(csvWkPtr, newPlot, ixcol, iycols, range);
    });

    int tab = ui->tabWidget->addTab(t, QIcon(":/table"),
                                QFileInfo(csv->fileInfo.filename).baseName());

    ui->tabWidget->setCurrentIndex(tab);
    ui->stackedWidget->setCurrentWidget(ui->page_main);

    t->setData(csv);
}

void MainWindow::onTablePlot(CsvWeakPtr csvWkPtr, bool newPlot, int ixcol,
                             QList<int> iycols, Range range)
{
    if (newPlot) {

        CsvPtr csv2(csvWkPtr);
        if (!csv2) { return; }

        plot(csv2, ixcol, iycols, range);

    } else {
        // Show a menu to select an existing plot.
        QMenu* menu = new QMenu();
        connect(menu, &QMenu::triggered, this, [=]()
        {
            menu->deleteLater();
        });

        foreach (PlotWindow* p, mPlots) {
            menu->addAction(p->windowTitle(), this,
                            [this, csvWkPtr, ixcol, iycols, range, p]()
            {
                CsvPtr csv3(csvWkPtr);
                if (!csv3) { return; }

                foreach (int iycol, iycols) {
                    p->plotData(csv3, ixcol, iycol, range);
                }
            });
        }
        if (menu->isEmpty()) {
            menu->addAction("No existing plots");
        }
        menu->popup(QCursor::pos());
    }
}

void MainWindow::plot(CsvPtr csv, int ixcol, QList<int> iycols, Range range)
{
    MatrixPtr mat = csv->matrix;

    QStringList ylabels;
    foreach (int iycol, iycols) {
        ylabels.append(mat->heading(iycol));
    }
    QString ytext = ylabels.join(",");
    if (ytext.length() > 13) {
        ytext = ytext.left(10) + "...";
    }
    QString xtext = mat->heading(ixcol);
    if (xtext.length() > 13) {
        xtext = xtext.left(10) + "...";
    }

    QString title;
    if (Utils::looksLikeTimeTitle(xtext) == Utils::NoMatch) {
        title = QString("%1 vs %2").arg(ytext).arg(xtext);
    } else {
        title = ytext;
    }

    PlotWindow* p = addPlot(ytext);

    foreach (int iycol, iycols) {
        p->plotData(csv, ixcol, iycol, range);
    }

    p->setTitle(title);
    p->setXLabel(xtext);
    p->setYLabel(ytext);

    p->showAll();
}

void MainWindow::csvImportFinished(CsvPtr csv)
{
    Defer d([=]()
    {
        progressDialog.preventClose(false);
        progressDialog.close();
    });

    if (!csv->importInfo.success) {
        msgBox(csv->importInfo.info);
        return;
    }

    if (csv->matrix.isNull()) { return; }

    progressDialog.setInfo("Generating table...");
    QApplication::processEvents();

    // Add table window
    addTable(csv);

    if (csv->matrix->errorCount) {
        msgBox("Some errors were encountered during import. See the Info tab.");
    }
}

void MainWindow::csvImportProgress(CsvPtr /*csv*/, QString info)
{
    progressDialog.setInfo(info);
}

void MainWindow::msgBox(QString msg)
{
    QMessageBox mb;
    mb.setText(msg);
    mb.exec();
}

void MainWindow::closeEvent(QCloseEvent* /*event*/)
{
    ui->tabWidget->closeAllWindows();
}

void MainWindow::on_pushButton_importCsv_clicked()
{
    on_action_importCsv_triggered();
}

void MainWindow::on_action_testPlot_triggered()
{
    CsvPtr csv(new Csv());
    csv->fileInfo.filename = "Test plot";

    int n = 100000;
    csv->matrix.reset(new Matrix(4));
    MatrixPtr mat = csv->matrix;
    mat->setHeadings({"time", "sin", "cos", "sincos2"});

    // Generate dummy data
    for (int i = 0; i < n; i++) {
        double t = i * 0.001;
        double s = std::sin(t);
        double c = std::cos(t);
        double sc2 = s + std::cos(2*t);
        mat->addRow({t, s, c, sc2});
    }

    addTable(csv);
}

void MainWindow::on_action_importCsv_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this, "Import CSV", "",
                                                    "CSV file (*.csv)");

    if (filename.isEmpty()) { return; }

    csvImportDialog.setFile(filename);
    csvImportDialog.show();
}

void MainWindow::onTabCustomContextMenuRequested(const QPoint &pos)
{
    int itab = ui->tabWidget->tabBar()->tabAt(pos);
    QWidget* w = ui->tabWidget->widget(itab);

    tabMenu.clear();
    tabMenu.addAction("Pop Out", this, [=]()
    {
        ui->tabWidget->popoutTabWidget(w);
    });
    tabMenu.addAction("Close", this, [=]()
    {
        delete ui->tabWidget->widget(itab);
        if (ui->tabWidget->tabAndWindowCount() == 0) {
            ui->stackedWidget->setCurrentWidget(ui->page_startup);
        }
    });
    tabMenu.popup(ui->tabWidget->mapToGlobal(pos));
}

void MainWindow::on_action_About_triggered()
{
    aboutDialog.show();
}

