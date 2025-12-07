/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2025 Gideon van der Kolf
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

#include "plotwindow.h"
#include "ui_plotwindow.h"

#include "matrix.h"

#include <QWeakPointer>


PlotWindow::PlotWindow(int tag, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PlotWindow),
    mTag(tag)
{
    ui->setupUi(this);

    // Initialise plot font
#if defined(Q_OS_WIN)
    mPlotFont = QFont("Tahoma");
#else
    mPlotFont = this->font();
#endif
    // Set explicit fonts of all plot elements. This is required as, on Windows,
    // if the font is the default MS Shell Dlg 2, exported SVGs don't load
    // properly in some programs.
    setPlotFont(mPlotFont);

    connect(ui->plot, &QCustomPlot::plottableClick, this, &PlotWindow::plottableClick);
    connect(ui->plot, &QCustomPlot::mouseMove, this, &PlotWindow::onPlotMouseMove);
    connect(ui->plot, &QCustomPlot::mousePress, this, &PlotWindow::onPlotMousePress);
    connect(ui->plot, &QCustomPlot::mouseRelease, this, &PlotWindow::onPlotMouseRelease);
    connect(ui->plot, &QCustomPlot::mouseDoubleClick, this, &PlotWindow::onPlotDoubleClick);
    connect(ui->plot, &QCustomPlot::axisDoubleClick, this, &PlotWindow::onAxisDoubleClick);
    connect(ui->plot, &QCustomPlot::itemDoubleClick, this, &PlotWindow::onPlotItemDoubleClick);

    ui->plot->setInteraction(QCP::iRangeDrag, true);
    ui->plot->setInteraction(QCP::iRangeZoom, true);
    ui->plot->axisRect()->setRangeZoom(Qt::Horizontal);
    ui->plot->setInteraction(QCP::iSelectPlottables, true);

    ui->plot->addLayer("crosshairs", ui->plot->layer("main"), QCustomPlot::limAbove);
    ui->plot->addLayer("markers", ui->plot->layer("crosshairs"), QCustomPlot::limAbove);
    ui->plot->addLayer("marker-labels", ui->plot->layer("marker-labels"), QCustomPlot::limAbove);

    ui->plot->installEventFilter(this);
}

PlotWindow::~PlotWindow()
{
    delete ui;
}

int PlotWindow::tag()
{
    return mTag;
}

QList<SubplotPtr> PlotWindow::subplots()
{
    return mSubplots;
}

QCustomPlot *PlotWindow::plotWidget()
{
    return ui->plot;
}

void PlotWindow::plotData(CsvPtr csv, int ixcol, int iycol, Range range)
{
    SubplotPtr subplot = mSubplots.value(0);
    if (!subplot) {
        subplot.reset(new Subplot(ui->plot->axisRect()));
        initSubplot(subplot);
    }

    plotData(subplot, csv, ixcol, iycol, range);
}

void PlotWindow::plotData(SubplotPtr subplot, CsvPtr csv, int ixcol, int iycol, Range range)
{
    if (!subplot) { return; }

    subplot->plotData(csv, ixcol, iycol, range);
}

SubplotPtr PlotWindow::addSubplot()
{
    QCPAxisRect* bottomAxisRect = new QCPAxisRect(ui->plot);
    ui->plot->plotLayout()->addElement(ui->plot->plotLayout()->rowCount(), 0, bottomAxisRect);
    SubplotPtr subplot(new Subplot(bottomAxisRect));
    initSubplot(subplot);
    return subplot;
}

void PlotWindow::setTitle(QString title)
{
    if (!mPlotTitle) {
        mPlotTitle = new QCPTextElement(ui->plot);
        connect(mPlotTitle, &QCPTextElement::doubleClicked,
                this, &PlotWindow::onTitleDoubleClick);

        ui->plot->plotLayout()->insertRow(0);
        ui->plot->plotLayout()->addElement(0, 0, mPlotTitle);
    }

    mPlotTitle->setText(title);
    ui->plot->replot();

    emit titleSet(title);
}

void PlotWindow::setXLabel(QString xlabel)
{
    SubplotPtr subplot = mSubplots.value(0);
    if (!subplot) { return; }
    subplot->setXLabel(xlabel);
}

void PlotWindow::setYLabel(QString ylabel)
{
    SubplotPtr subplot = mSubplots.value(0);
    if (!subplot) { return; }
    subplot->setYLabel(ylabel);
}

void PlotWindow::syncAxisRanges(int linkGroup, QRectF xyrange)
{
    foreach (SubplotPtr subplot, mSubplots) {
        if (!subplot->linkGroup) { continue; }
        if (subplot->linkGroup != linkGroup) { continue; }
        subplot->syncAxisRanges(xyrange);
    }
}

void PlotWindow::syncDataTip(int linkGroup, int index)
{
    foreach (SubplotPtr subplot, mSubplots) {
        if (!subplot->linkGroup) { continue; }
        if (subplot->linkGroup != linkGroup) { continue; }
        subplot->syncDataTip(index);
    }
}

bool PlotWindow::eventFilter(QObject* /*watched*/, QEvent *event)
{
    bool keyPressOrRelease =    (event->type() == QEvent::KeyPress)
                             || (event->type() == QEvent::KeyRelease);

    if (keyPressOrRelease) {
        foreach (SubplotPtr subplot, mSubplots) {
            subplot->keyEvent(event);
        }
    }

    return false;
}

void PlotWindow::resizeEvent(QResizeEvent* /*event*/)
{
    foreach (SubplotPtr subplot, mSubplots) {
        subplot->resizeEvent();
    }
}

void PlotWindow::showEvent(QShowEvent* /*event*/)
{
    foreach (SubplotPtr subplot, mSubplots) {
        subplot->showEvent();
    }
}

void PlotWindow::initSubplot(SubplotPtr subplot)
{
    connect(subplot.data(), &Subplot::axisRangesChanged,
            this, [this, sWkPtr = subplot.toWeakRef()]
            (int linkGroup, QRectF xyrange)
    {
        SubplotPtr s(sWkPtr);
        if (!s) { return; }
        onAxisRangesChanged(s, linkGroup, xyrange);
    });

    connect(subplot.data(), &Subplot::dataTipChanged,
            this, [this, sWkPtr = subplot.toWeakRef()]
            (int linkGroup, int index)
    {
        SubplotPtr s(sWkPtr);
        if (!s) { return; }
        onDataTipChanged(s, linkGroup, index);
    });

    connect(subplot.data(), &Subplot::linkSettingsTriggered,
            this, [this, sWkPtr = subplot.toWeakRef()]()\
    {
        SubplotPtr s(sWkPtr);
        if (!s) { return; }
        emit linkSettingsTriggered(s);
    });

    mSubplots.append(subplot);

    subplot->tag = QString("Subplot %1").arg(mSubplots.count());
}

void PlotWindow::storeAndDisableCrosshairsOfAllSubplots()
{
    foreach (SubplotPtr subplot, mSubplots) {
        subplot->storeAndDisableCrosshairs();
    }
}

void PlotWindow::restoreCrosshairsOfAllSubplots()
{
    foreach (SubplotPtr subplot, mSubplots) {
        subplot->restoreCrosshairs();
    }
}

void PlotWindow::plottableClick(QCPAbstractPlottable* /*plottable*/,
                                int /*dataIndex*/, QMouseEvent* /*event*/)
{

}

void PlotWindow::onPlotMouseMove(QMouseEvent* /*event*/)
{

}

void PlotWindow::onPlotMousePress(QMouseEvent* /*event*/)
{

}

void PlotWindow::onPlotDoubleClick(QMouseEvent* /*event*/)
{

}

void PlotWindow::onAxisDoubleClick(QCPAxis* /*axis*/, QCPAxis::SelectablePart /*part*/,
                                   QMouseEvent* /*event*/)
{

}

void PlotWindow::onPlotItemDoubleClick(QCPAbstractItem* /*item*/, QMouseEvent* /*event*/)
{

}

void PlotWindow::onTitleDoubleClick(QMouseEvent* /*event*/)
{
    bool ok;
    QString title = QInputDialog::getText(this, "Title", "Title",
                                          QLineEdit::Normal,
                                          mPlotTitle->text(),
                                          &ok);
    if (ok) {
        setTitle(title);
    }
}

void PlotWindow::onPlotMouseRelease(QMouseEvent* /*event*/)
{

}

QByteArray PlotWindow::plotToSvg()
{
    QByteArray svgData;
    QBuffer buffer(&svgData);
    buffer.open(QIODevice::WriteOnly);

    ui->plot->saveSvg(&buffer);

    buffer.close();

    return svgData;
}

QCPLegend *PlotWindow::findLegendInAxisRect(QCPAxisRect *axisRect)
{
    if (!axisRect) { return nullptr; }

    QCPLegend* legend = nullptr;

    for (int i = 0; i < axisRect->insetLayout()->elementCount(); ++i) {
        legend = qobject_cast<QCPLegend*>(axisRect->insetLayout()->elementAt(i));
        if (legend) {
            break;
        }
    }

    return legend;
}

QCPLegend *PlotWindow::findLegendUnderPos(QPoint pos)
{
    QCPLegend* legend = findLegendInAxisRect(ui->plot->axisRectAt(pos));
    if (legend && (legend->selectTest(pos, false) >= 0)) {
        return legend;
    }

    return nullptr;
}

void PlotWindow::onAxisRangesChanged(SubplotPtr subplot, int linkGroup, QRectF xyrange)
{
    foreach (SubplotPtr s, mSubplots) {
        if (s == subplot) { continue; }
        if (!s->linkGroup) { continue; }
        if (s->linkGroup != linkGroup) { continue; }
        s->syncAxisRanges(xyrange);
    }
    emit axisRangesChanged(linkGroup, xyrange);
}

void PlotWindow::onDataTipChanged(SubplotPtr subplot, int linkGroup, int index)
{
    foreach (SubplotPtr s, mSubplots) {
        if (s == subplot) { continue; }
        if (!s->linkGroup) { continue; }
        if (s->linkGroup != linkGroup) { continue; }
        s->syncDataTip(index);
    }
    emit dataTipChanged(linkGroup, index);
}

void PlotWindow::on_action_Dock_to_Screen_Top_triggered()
{
    emit requestWindowDock(DockTop);
}

void PlotWindow::on_action_Dock_to_Screen_Bottom_triggered()
{
    emit requestWindowDock(DockBottom);
}

void PlotWindow::on_action_Dock_to_Screen_Left_triggered()
{
    emit requestWindowDock(DockLeft);
}

void PlotWindow::on_action_Dock_to_Screen_Right_triggered()
{
    emit requestWindowDock(DockRight);
}

void PlotWindow::on_action_Undocked_triggered()
{
    emit requestWindowDock(DockFloating);
}

void PlotWindow::on_action_Tab_in_Main_Window_triggered()
{
    emit requestWindowDock(DockTab);
}

void PlotWindow::on_action_Resize_Plot_triggered()
{
    bool ok = false;
    QString res = QInputDialog::getItem(this, "Resize Plot", "Size in pixels",
                                        {"700x450", "800x600", "1024x768"},
                                        0,
                                        true,
                                        &ok);
    if (!ok) { return; }
    QStringList terms = res.split("x");
    int x = terms.value(0).toInt(&ok);
    if (!ok || (x < 10)) { return; }
    int y = terms.value(1).toInt(&ok);
    if (!ok || (y < 10)) { return; }

    emit requestWindowResize(x, y);
}

void PlotWindow::setPlotFont(QFont font)
{
    mPlotFont = font;

    ui->plot->setFont(mPlotFont);
    for (auto axis : ui->plot->axisRect()->axes()) {
        axis->setLabelFont(ui->plot->font());
        axis->setTickLabelFont(ui->plot->font());
    }
    ui->plot->legend->setFont(ui->plot->font());
}

void PlotWindow::on_action_Save_as_PDF_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "Save as PDF",
                                                "",
                                                "PDF (*.pdf)");
    if (path.isEmpty()) { return; }

    storeAndDisableCrosshairsOfAllSubplots();

    bool ok = ui->plot->savePdf(path,0, 0, QCP::epNoCosmetic);
    if (!ok) {
        QMessageBox::critical(this, "Save to PDF failed",
                              "Failed to save to PDF: " + path);
    }

    restoreCrosshairsOfAllSubplots();
}

void PlotWindow::on_action_Copy_PNG_triggered()
{
    storeAndDisableCrosshairsOfAllSubplots();
    QImage image = ui->plot->toPixmap(0, 0, 2.0).toImage();
    restoreCrosshairsOfAllSubplots();

    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setImage(image);
}

void PlotWindow::on_action_Copy_SVG_triggered()
{
    storeAndDisableCrosshairsOfAllSubplots();
    QByteArray svgData = plotToSvg();
    restoreCrosshairsOfAllSubplots();

    // Copy to clipboard
    QMimeData *mimeData = new QMimeData();
    mimeData->setData("image/svg+xml", svgData);

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setMimeData(mimeData);
}

void PlotWindow::on_action_Save_as_PNG_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "Save as PNG",
                                                "",
                                                "PNG Image (*.png)");
    if (path.isEmpty()) { return; }

    storeAndDisableCrosshairsOfAllSubplots();
    bool ok = ui->plot->savePng(path, 0, 0, 2.0);
    if (!ok) {
        QMessageBox::critical(this, "Save to PNG failed",
                              "Failed to save to PNG: " + path);
    }
    restoreCrosshairsOfAllSubplots();
}

void PlotWindow::on_action_Save_as_SVG_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "Save as SVG",
                                                "",
                                                "SVG Vector Image (*.svg)");
    if (path.isEmpty()) { return; }

    storeAndDisableCrosshairsOfAllSubplots();
    QByteArray svgData = plotToSvg();
    restoreCrosshairsOfAllSubplots();

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(svgData);
        file.close();
    } else {
        QMessageBox::critical(this, "Save to SVG failed",
                              "Failed to save to SVG: " + path);
    }
}

