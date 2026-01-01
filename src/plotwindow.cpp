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

QList<LinkPtr> PlotWindow::links()
{
    QList<LinkPtr> ret;

    foreach (SubplotPtr subplot, mSubplots) {
        ret.append(subplot->link);
    }
    if (mMapPlot) {
        ret.append(mMapPlot->link);
    }

    return ret;
}

QList<SubplotPtr> PlotWindow::subplots()
{
    return mSubplots;
}

MapPlotPtr PlotWindow::mapPlot()
{
    return mMapPlot;
}

QSize PlotWindow::plotWidgetSize()
{
    if (mMapWidget) {
        return mMapWidget->size();
    } else {
        return ui->plot->size();
    }
}

void PlotWindow::plotData(CsvPtr csv, int ixcol, int iycol, Range range)
{
    SubplotPtr subplot = mSubplots.value(0);
    if (!subplot) {
        subplot.reset(new Subplot(ui->plot->axisRect(), this));
        initSubplot(subplot);
    }

    plotData(subplot, csv, ixcol, iycol, range);
}

void PlotWindow::plotData(SubplotPtr subplot, CsvPtr csv, int ixcol, int iycol, Range range)
{
    if (!subplot) { return; }

    subplot->plot(csv, ixcol, iycol, range);

    setupGuiForNormalPlot();
}

void PlotWindow::plotMap(CsvPtr csv, int ixcol, int iycol, Range range)
{
    if (!mMapWidget) {
        mMapWidget = new QGVMap();
        mMapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        QHBoxLayout* layout = static_cast<QHBoxLayout*>(ui->centralwidget->layout());
        layout->removeWidget(ui->plot);
        ui->plot->hide();
        layout->insertWidget(0, mMapWidget, 1);
    }

    if (!mMapPlot) {
        mMapPlot.reset(new MapPlot(mMapWidget, this));

        initPlot(mMapPlot);
        mAllPlots.append(mMapPlot);
    }

    mMapPlot->plot(csv, ixcol, iycol, range);

    setupGuiForMap();
}

SubplotPtr PlotWindow::addSubplot()
{
    SubplotPtr subplot = Subplot::newAtBottomOfPlot(ui->plot, this);
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
        if (subplot->link->match(linkGroup)) {
            subplot->syncAxisRanges(xyrange);
        }
    }
}

void PlotWindow::syncDataTip(int linkGroup, int index)
{
    foreach (SubplotPtr subplot, mSubplots) {
        if (subplot->link->match(linkGroup)) {
            subplot->syncDataTip(index);
        }
    }
    if (mMapPlot) {
        if (mMapPlot->link->match(linkGroup)) {
            mMapPlot->syncDataTip(index);
        }
    }
}

void PlotWindow::setupGuiForNormalPlot()
{
    // Mouse/keyboard control info
    ui->frame_info_ctrlWheelZoom->setVisible(true);
    ui->frame_info_ldragPan->setVisible(true);
    ui->frame_info_rclickMenu->setVisible(true);
    ui->frame_info_rdragZoom->setVisible(true);
    ui->frame_info_shiftWheelVzoom->setVisible(true);
    ui->frame_info_wheelHzoom->setVisible(true);
    ui->frame_info_wheelZoom->setVisible(false);
}

void PlotWindow::setupGuiForMap()
{
    // Mouse/keyboard control info
    ui->frame_info_ctrlWheelZoom->setVisible(false);
    ui->frame_info_ldragPan->setVisible(true);
    ui->frame_info_rclickMenu->setVisible(true);
    ui->frame_info_rdragZoom->setVisible(false);
    ui->frame_info_shiftWheelVzoom->setVisible(false);
    ui->frame_info_wheelHzoom->setVisible(false);
    ui->frame_info_wheelZoom->setVisible(true);

    // Image menu items
    ui->action_Copy_SVG->setVisible(false);
    ui->action_Save_as_PDF->setVisible(false);
    ui->action_Save_as_SVG->setVisible(false);
}

bool PlotWindow::eventFilter(QObject* /*watched*/, QEvent *event)
{
    // TODO: consider implementing in Subplot
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
    foreach (PlotPtr p, mAllPlots) {
        p->resized();
    }
}

void PlotWindow::showEvent(QShowEvent* /*event*/)
{
    foreach (SubplotPtr subplot, mSubplots) {
        subplot->showEvent();
    }
}

void PlotWindow::initPlot(PlotPtr plot)
{
    connect(plot.data(), &Plot::axisRangesChanged,
            this, [this, pWkPtr = plot.toWeakRef()]
            (int linkGroup, QRectF xyrange)
    {
        PlotPtr p(pWkPtr);
        if (!p) { return; }
        onAxisRangesChanged(p, linkGroup, xyrange);
    });

    connect(plot.data(), &Plot::dataTipChanged,
            this, [this, pWkPtr = plot.toWeakRef()]
            (int linkGroup, int index)
    {
        PlotPtr p(pWkPtr);
        if (!p) { return; }
        onDataTipChanged(p, linkGroup, index);
    });

    connect(plot.data(), &Plot::linkSettingsTriggered,
            this, [this, pWkPtr = plot.toWeakRef()]()
    {
        PlotPtr p(pWkPtr);
        if (!p) { return; }
        emit linkSettingsTriggered(p->link);
    });
}

void PlotWindow::initSubplot(SubplotPtr subplot)
{
    initPlot(subplot);

    mSubplots.append(subplot);
    mAllPlots.append(subplot);

    subplot->link->tag = QString("Subplot %1").arg(mSubplots.count());
}

void PlotWindow::storeAndDisableCrosshairsOfAllSubplots()
{
    foreach (SubplotPtr subplot, mSubplots) {
        subplot->storeAndDisableCrosshairs();
    }
    if (mMapPlot) {
        mMapPlot->storeAndDisableCrosshairs();
    }
}

void PlotWindow::restoreCrosshairsOfAllSubplots()
{
    foreach (SubplotPtr subplot, mSubplots) {
        subplot->restoreCrosshairs();
    }
    if (mMapPlot) {
        mMapPlot->restoreCrosshairs();
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

QString PlotWindow::sanitizeFilename(QString filename)
{
    static QByteArray invalid = "<:\"/\\|?*";

    foreach (QChar c, invalid) {
        filename.replace(c, "");
    }

    return filename;
}

QString PlotWindow::filenameForThisPlot()
{
    return sanitizeFilename(this->windowTitle());
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

void PlotWindow::onAxisRangesChanged(PlotPtr plot, int linkGroup, QRectF xyrange)
{
    foreach (PlotPtr p, mAllPlots) {
        if (p == plot) { continue; }
        if (p->link->match(linkGroup)) {
            p->syncAxisRanges(xyrange);
        }
    }
    emit axisRangesChanged(linkGroup, xyrange);
}

void PlotWindow::onDataTipChanged(PlotPtr plot, int linkGroup, int index)
{
    foreach (PlotPtr p, mAllPlots) {
        if (p == plot) { continue; }
        if (p->link->match(linkGroup)) {
            p->syncDataTip(index);
        }
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
                                filenameForThisPlot() + ".pdf",
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

    QImage image;
    if (mMapPlot) {
        image = mMapPlot->toPixmap().toImage();
    } else {
        image = ui->plot->toPixmap(0, 0, 2.0).toImage();
    }

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
                                                filenameForThisPlot() + ".png",
                                                "PNG Image (*.png)");
    if (path.isEmpty()) { return; }

    storeAndDisableCrosshairsOfAllSubplots();

    bool ok;

    if (mMapPlot) {

        ok = mMapPlot->saveToPng(path);

    } else {

        ok = ui->plot->savePng(path, 0, 0, 2.0);

    }

    if (!ok) {
        QMessageBox::critical(this, "Save to PNG failed",
                              "Failed to save to PNG: " + path);
    }

    restoreCrosshairsOfAllSubplots();
}

void PlotWindow::on_action_Save_as_SVG_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "Save as SVG",
                                                filenameForThisPlot() + ".svg",
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


