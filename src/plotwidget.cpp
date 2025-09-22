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

#include "plotwidget.h"
#include "ui_plotwidget.h"

#include "matrix.h"

#include <QWeakPointer>

PlotWidget::PlotWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlotWidget)
{
    ui->setupUi(this);

    setupMenus();

    connect(ui->plot, &QCustomPlot::plottableClick, this, &PlotWidget::plottableClick);
    connect(ui->plot, &QCustomPlot::mouseMove, this, &PlotWidget::onPlotMouseMove);
    connect(ui->plot, &QCustomPlot::mousePress, this, &PlotWidget::onPlotMousePress);
    connect(ui->plot, &QCustomPlot::mouseRelease, this, &PlotWidget::onPlotMouseRelease);

    ui->plot->setInteraction(QCP::iRangeDrag, true);
    ui->plot->setInteraction(QCP::iRangeZoom, true);
    ui->plot->axisRect()->setRangeZoom(Qt::Horizontal);
    ui->plot->setInteraction(QCP::iSelectPlottables, true);

    ui->plot->installEventFilter(this);

    ui->label_coordinates->clear();

    setupCrosshairs();

    updateGuiForLinkGroup();
}

PlotWidget::~PlotWidget()
{
    delete ui;
}

QCustomPlot *PlotWidget::plotWidget()
{
    return ui->plot;
}

void PlotWidget::plotData(CsvPtr csv, int ixcol, int iycol, Range range)
{
    bool firstPlot = (ui->plot->plottableCount() == 0);

    QVector<double> x = csv->matrix->data[ixcol].mid(range.start, range.size());
    QVector<double> y = csv->matrix->data[iycol].mid(range.start, range.size());

    if (firstPlot) {
        xmin = Matrix::vmin(x);
        xmax = Matrix::vmax(x);
        ymin = Matrix::vmin(y);
        ymax = Matrix::vmax(y);
    } else {
        xmin = qMin(xmin, Matrix::vmin(x));
        xmax = qMax(xmax, Matrix::vmax(x));
        ymin = qMin(ymin, Matrix::vmin(y));
        ymax = qMax(ymax, Matrix::vmax(y));
    }

    // Determine whether x is monotonically inreasing
    bool up = true;
    for (int i = 1; i < x.count(); i++) {
        if (x[i] < x[i-1]) {
            up = false;
            break;
        }
    }

    QPen pen = pens.value(ui->plot->plottableCount() % pens.count());

    GraphPtr graph;
    if (up) {
        // Graph is more efficient but can only be used if x is monotonically
        // increasing
        QCPGraph* qcpgraph = ui->plot->addGraph();
        graph.reset(new Graph(qcpgraph));
        qcpgraph->setData(x, y);
        qcpgraph->setPen(pen);
        qcpgraph->setName(csv->matrix->heading(iycol));
        mPlotCrosshairSnap = SnapXOnly;
        mPlotCrosshair.enableHline(false);
        updateGuiForCrosshairOptions();
    } else {
        // Curve is used otherwise
        QCPCurve* qcpcurve = new QCPCurve(ui->plot->xAxis, ui->plot->yAxis);
        graph.reset(new Graph(qcpcurve));
        qcpcurve->setData(x, y);
        qcpcurve->setPen(pen);
        qcpcurve->setName(csv->matrix->heading(iycol));
        mPlotCrosshairSnap = SnapToClosest;
        setEqualAxesButDontReplot(true);
    }
    graph->csv = csv;
    graph->range = range;
    graphs.append(graph);

    if (graphs.count() == 1) {
        dataTipGraph = graphs.value(0);
    }

    if (ui->plot->plottableCount() > 1) {
        ui->plot->legend->setVisible(true);
    }

    if (firstPlot) {
        // Only show all on first plot as to not mess up view when adding more
        // to existing plot.
        showAll();

        QCPAxis* xaxis = ui->plot->xAxis;
        if (xaxis) {
            connect(xaxis,
                    QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                    this, &PlotWidget::onAxisRangesChanged);
        }
        QCPAxis* yaxis = ui->plot->yAxis;
        if (yaxis) {
            connect(yaxis,
                    QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                    this, &PlotWidget::onAxisRangesChanged);
        }

    } else {
        queueReplot();
    }
}

void PlotWidget::showAll()
{
    QCPAxisRect* ar = ui->plot->axisRect(0);
    if (!ar) { return; }
    QCPAxis* xaxis = ui->plot->xAxis;
    if (!xaxis) { return; }
    QCPAxis* yaxis = ui->plot->yAxis;
    if (!yaxis) { return; }

    QRectF rect(xmin, ymin, xmax - xmin, ymax - ymin);

    // If the plot is a constant horizontal Y line, adjust y axis to display it better.
    if (rect.height() == 0) {
        if (rect.top() == 0) {
            // Y = constant zero. Make y axis 1- to 1.
            rect.setTop(1);
            rect.setBottom(-1);
        } else if (rect.top() > 0) {
            // Y = positive constant. Make y axis 0 to y.
            rect.setBottom(0);
        } else {
            // Y = negative constant. Make y axis y to 0.
            rect.setTop(0);
        }
    }

    // Calculate a padding so lines at the edges of the axis ranges can be seen,
    // e.g. line on x axis (y = 0), or initial line on y axis (x = 0).
    double paddingPx = 2;
    double ypad = rect.height() / ar->height() * paddingPx;
    double xpad = rect.width() / ar->width() * paddingPx;

    rect.setLeft(rect.left() - xpad);
    rect.setRight(rect.right() + xpad);
    rect.setTop(rect.top() - ypad);
    rect.setBottom(rect.bottom() + ypad);

    if (mEqualAxes) {
        updatePlotForEqualAxes(rect);
    } else {
        xaxis->setRange(rect.left(), rect.right());
        yaxis->setRange(rect.top(), rect.bottom());
        queueReplot();
    }
}

void PlotWidget::setEqualAxesButDontReplot(bool fixed)
{
    mEqualAxes = fixed;
    ui->action_Equal_Axes->setChecked(mEqualAxes);

    if (fixed) {
        ui->plot->axisRect()->setRangeZoom(Qt::Vertical | Qt::Horizontal);
    } else {
        ui->plot->axisRect()->setRangeZoom(Qt::Horizontal);
    }
}

void PlotWidget::setEqualAxesAndReplot(bool fixed)
{
    setEqualAxesButDontReplot(fixed);

    if (fixed) {
        QCPAxis* xaxis = ui->plot->xAxis;
        if (!xaxis) { return; }
        QCPAxis* yaxis = ui->plot->yAxis;
        if (!yaxis) { return; }

        QRectF r(xaxis->range().lower, yaxis->range().lower,
                 xaxis->range().size(), yaxis->range().size());
        updatePlotForEqualAxes(r);
    }
}

void PlotWidget::syncAxisRanges(QRectF xyrange)
{
    QCPAxis* xaxis = ui->plot->xAxis;
    if (!xaxis) { return; }
    QCPAxis* yaxis = ui->plot->yAxis;
    if (!yaxis) { return; }

    mRangesSyncedFromOutside = true;

    QRectF r(xaxis->range().lower, yaxis->range().lower,
             xaxis->range().size(), yaxis->range().size());
    QPointF center = r.center();
    if (ui->action_Link_X_Position->isChecked()) {
        center.setX(xyrange.center().x());
    }
    if (ui->action_Link_Y_Position->isChecked()) {
        center.setY(xyrange.center().y());
    }
    if (ui->action_Link_X_Zoom->isChecked()) {
        r.setWidth(xyrange.width());
    }
    if (ui->action_Link_Y_Zoom->isChecked()) {
        r.setHeight(xyrange.height());
    }
    r.moveCenter(center);

    xaxis->setRange(r.left(), r.right());
    yaxis->setRange(r.top(), r.bottom());
    queueReplot();
}

void PlotWidget::syncDataTip(int index)
{
    if (mPlotCrosshair.isVisible()) {
        if (dataTipGraph) {
            index -= dataTipGraph->range.start;
            double x = dataTipGraph->datax(index);
            double y = dataTipGraph->datay(index);
            mPlotCrosshair.setCoords(x, y);
            queueReplot();
        }
    }
}

bool PlotWidget::eventFilter(QObject* /*watched*/, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {

        QKeyEvent* keyEvent = (QKeyEvent*)event;
        switch (keyEvent->key()) {
        case Qt::Key_Shift:
            if (!mEqualAxes) {
                // Holding shift: vertical zoom
                ui->plot->axisRect()->setRangeZoom(Qt::Vertical);
            }
            break;
        case Qt::Key_Control:
            // Holding control: vertical + horizontal zoom
            ui->plot->axisRect()->setRangeZoom(Qt::Vertical | Qt::Horizontal);
            break;
        }

    } else if (event->type() == QEvent::KeyRelease) {

        QKeyEvent* keyEvent = (QKeyEvent*)event;
        switch (keyEvent->key()) {
        case Qt::Key_Shift:
            // Intentional fall-through
        case Qt::Key_Control:
            // Releasing: back to default horizontal zoom
            if (!mEqualAxes) {
                ui->plot->axisRect()->setRangeZoom(Qt::Horizontal);
            }
            break;

        }
    }

    return false;
}

void PlotWidget::resizeEvent(QResizeEvent* /*event*/)
{
    if (mEqualAxes) {
        setEqualAxesAndReplot(true);
    }
}

void PlotWidget::plottableClick(QCPAbstractPlottable* /*plottable*/,
                                int /*dataIndex*/, QMouseEvent* /*event*/)
{

}

void PlotWidget::onPlotMouseMove(QMouseEvent *event)
{
    bool replot = plotMouseMove(event);
    replot |= plotMouseRightDrag(event);

    if (replot) {
        queueReplot();
    }
}

void PlotWidget::onPlotMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        rMouseZoom.start = event->pos() - ui->plot->axisRect()->topLeft();
        rMouseZoom.origXrange = ui->plot->xAxis->range();
        rMouseZoom.origYrange = ui->plot->yAxis->range();
        rMouseZoom.mouseDown = true;
    } else if (event->button() == Qt::LeftButton) {
        lMouseDown = true;
    }
}

/* Handles right mouse button drag on plot. Returns true if plot should be
 * updated (replot). */
bool PlotWidget::plotMouseRightDrag(QMouseEvent* event)
{
    if (!rMouseZoom.mouseDown) { return false; }

    QCPAxisRect* ar = ui->plot->axisRect();
    if (!ar) { return false; }
    QRectF axisRect(ar->rect());

    // Position on axis = mouse position minus axis rect offsets
    QPointF axisPos = event->pos() - axisRect.topLeft();
    // Mouse movement relative to start
    double dx = (axisPos.x() - rMouseZoom.start.x()) / axisRect.width();
    double dy = (axisPos.y() - rMouseZoom.start.y()) / axisRect.height();

    // Start dragging if we have moved further than threshold
    if (!rMouseZoom.isDragging) {
        int px = ui->plot->xAxis->coordToPixel(dx);
        int py = ui->plot->yAxis->coordToPixel(dy);
        if (QPoint(px, py).manhattanLength() > 10) {
            rMouseZoom.isDragging = true;
        }
    }

    bool replot = false;

    if (rMouseZoom.isDragging) {

        // Handle right mouse button dragging zoom

        if (mEqualAxes) {
            dy = -dx;
        }

        // Multiply by factor to make movement more sensitive
        dx *= 10.0;
        dy *= 10.0;

        // Calculate zoomed x range size
        double xsize = rMouseZoom.origXrange.size();
        if (dx > 0) {
            // Right mouse movement --> zoom in --> smaller axis range
            xsize /= 1.0 + qAbs(dx);
        } else {
            // Left mouse movement --> zoom out --> larger axis range
            xsize *= 1.0 + qAbs(dx);
        }

        // x "center factor" used to keep start point at same position
        double xcf = (rMouseZoom.start.x()) / axisRect.width();
        // Apply x zoom
        ui->plot->xAxis->setRange(
                    (rMouseZoom.origXrange.lower + rMouseZoom.origXrange.size() * xcf) - xsize * xcf,
                    (rMouseZoom.origXrange.lower + rMouseZoom.origXrange.size() * xcf) + xsize * (1 - xcf));


        // Calculate zoomed y range size
        double ysize = rMouseZoom.origYrange.size();
        if (dy < 0) {
            // Upward mouse movement --> zoom in --> smaller axis range
            ysize /= 1.0 + qAbs(dy);
        } else {
            // Downward mouse movement --> zoom out --> larger axis range
            ysize *= 1.0 + qAbs(dy);
        }

        // y "center factor" used to keep start point at same position
        double ycf = (axisRect.height() - rMouseZoom.start.y()) / axisRect.height();
        // Apply y zoom
        ui->plot->yAxis->setRange(
                    (rMouseZoom.origYrange.lower + rMouseZoom.origYrange.size() * ycf) - ysize * ycf,
                    (rMouseZoom.origYrange.lower + rMouseZoom.origYrange.size() * ycf) + ysize * (1 - ycf));

        replot = true;
    }

    return replot;
}

/* Handles plot mouse moving. Returns true if plot should be updated (replot). */
bool PlotWidget::plotMouseMove(QMouseEvent* event)
{
    bool posValid = false;
    bool replot = false;

    bool dragging = rMouseZoom.isDragging || lMouseDown;

    QCPAxis* xaxis = ui->plot->xAxis;
    QCPAxis* yaxis = ui->plot->yAxis;
    if (xaxis && yaxis) {

        if ( (xaxis != nullptr) && (yaxis != nullptr) ) {
            double mouseX = xaxis->pixelToCoord(event->x());
            double mouseY = yaxis->pixelToCoord(event->y());
            posValid = true;

            // Only update tracers and coordinates if not dragging as it could
            // slow down dragging
            if (!dragging) {

                QString text;

                if (mPlotCrosshair.isVisible() && dataTipGraph) {

                    int closestDist = 0;
                    double closestX = 0;
                    double closestY = 0;
                    int closestIndex = 0;

                    QElapsedTimer timer;
                    timer.start();

                    for (int i = 0; i < dataTipGraph->dataCount(); ++i) {

                        double xCrd = dataTipGraph->datax(i);
                        int xPixel = xaxis->coordToPixel(xCrd);
                        double yCrd = dataTipGraph->datay(i);

                        int dist = 0;

                        if (mPlotCrosshairSnap == SnapXOnly) {
                            dist = qAbs(event->x() - xPixel);
                        } else if (mPlotCrosshairSnap == SnapToClosest) {
                            int yPixel = yaxis->coordToPixel(yCrd);
                            // Calculate distance in pixels to account for zooms
                            dist = qSqrt( qPow(event->x() - xPixel, 2)
                                          + qPow(event->y() - yPixel, 2) );
                        }

                        if ((i == 0) || (dist < closestDist)) {
                            closestDist = dist;
                            closestX = xCrd;
                            closestY = yCrd;
                            closestIndex = i;
                            if (dist == 0) {
                                break;
                            }
                        }
                    }

                    // Update the plot crosshair
                    // If it takes too long to update, disable the plot crosshair
                    // automatically. However, if the user has enabled it, leave
                    // it alone.
                    if ((timer.elapsed() > 100) && !mPlotCrosshairVisibilityChangedByUser) {

                        mPlotCrosshair.setVisible(false);
                        updateGuiForCrosshairOptions();
                    } else {
                        mPlotCrosshair.setCoords(closestX, closestY);
                        text = QString("Plot: [%1] (%2, %3)")
                                   .arg(closestIndex).arg(closestX).arg(closestY);
                        emit dataTipChanged(closestIndex + dataTipGraph->range.start);
                        replot = true;

                        mPlotCrosshairIndex = closestIndex;
                    }
                }

                // Update mouse crosshair
                if (mMouseCrosshair.isVisible()) {
                    mMouseCrosshair.setCoords(mouseX, mouseY);
                    replot = true;
                }

                // Update coordinates label
                if (!text.isEmpty()) { text += ", "; }
                text += QString("Mouse: (%4, %5)").arg(mouseX).arg(mouseY);

                ui->label_coordinates->setText(text);
            }
        }
    }

    if (!posValid) {
        // Clear coordinates label
        ui->label_coordinates->clear();
    }

    return replot;
}

void PlotWidget::onPlotMouseRelease(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        rMouseZoom.mouseDown = false;
        if (!rMouseZoom.isDragging) {
            // Was not dragging. Trigger mouse right click event
            plotRightClicked(event->pos());
        }
        rMouseZoom.isDragging = false;
    } else if (event->button() == Qt::LeftButton) {
        lMouseDown = false;
    }
}

void PlotWidget::setupCrosshairs()
{
    mPlotCrosshair.init(ui->plot);

    mMouseCrosshair.init(ui->plot);
    mMouseCrosshair.setVisible(false);

    updateGuiForCrosshairOptions();
}

void PlotWidget::updateGuiForCrosshairOptions()
{
    ui->action_Show_Plot_Crosshair->setChecked(mPlotCrosshair.isVisible());
    ui->action_PlotCrosshair_Vertical_Line->setChecked(mPlotCrosshair.isVlineEnabled());
    ui->action_PlotCrosshair_Horizontal_Line->setChecked(mPlotCrosshair.isHlineEnabled());
    ui->action_PlotCrosshair_Dot->setChecked(mPlotCrosshair.isDotEnabled());

    ui->action_Show_Mouse_Crosshair->setChecked(mMouseCrosshair.isVisible());
    ui->action_MouseCrosshair_Vertical_Line->setChecked(mMouseCrosshair.isVlineEnabled());
    ui->action_MouseCrosshair_Horizontal_Line->setChecked(mMouseCrosshair.isHlineEnabled());
    ui->action_MouseCrosshair_Dot->setChecked(mMouseCrosshair.isDotEnabled());

    queueReplot();
}

void PlotWidget::setLinkGroup(int group)
{
    mLinkGroup = group;
    emit linkGroupChanged(group);
    updateGuiForLinkGroup();
}

void PlotWidget::updateGuiForLinkGroup()
{
    ui->action_No_Link->setChecked(mLinkGroup == 0);
    ui->action_Link_to_Group_1->setChecked(mLinkGroup == 1);
    ui->action_Link_to_Group_2->setChecked(mLinkGroup == 2);
    ui->action_Link_to_Group_3->setChecked(mLinkGroup == 3);

    if (mLinkGroup == 0) {
        ui->pushButton_link->setText("Link");
    } else {
        ui->pushButton_link->setText(QString("Link (%1)").arg(mLinkGroup));
    }
}

void PlotWidget::plotRightClicked(const QPoint &pos)
{
    plotContextMenu.popup(ui->plot->mapToGlobal(pos));
}

void PlotWidget::queueReplot()
{
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

void PlotWidget::updatePlotForEqualAxes(QRectF xyrange)
{
    QCPAxisRect* ar = ui->plot->axisRect(0);
    if (!ar) { return; }
    QCPAxis* xaxis = ui->plot->xAxis;
    if (!xaxis) { return; }
    QCPAxis* yaxis = ui->plot->yAxis;
    if (!yaxis) { return; }

    int widthpx = ar->width();
    int heightpx = ar->height();

    double viewRatio = (double)widthpx / (double)heightpx;
    double rangeRatio = xyrange.width() / xyrange.height();
    QPointF center = xyrange.center();

    if (rangeRatio > viewRatio) {
        /*
         *   view area (axisRect)
         *   ________________
         *  |                |
         *  |................|
         *  |:   xyrange    :|
         *  |:..............:|
         *  |                |
         *  |________________|
         *
         * Desired ranges wider than view area
         * Use the full width (x range) and scale the y range
         */
        xyrange.setHeight(xyrange.width() / viewRatio);
    } else {
        /*
         *   view area (axisRect)
         *   ___.........____
         *  |   :       :    |
         *  |   :       :    |
         *  |   :xyrange:    |
         *  |   :       :    |
         *  |   :       :    |
         *  |___:.......:____|
         *
         * Desired ranges narrower than view area
         * Use the full height (y range) and scale the x range
         */
        xyrange.setWidth(xyrange.height() * viewRatio);
    }

    xyrange.moveCenter(center);

    xaxis->setRange(xyrange.left(), xyrange.right());
    yaxis->setRange(xyrange.top(), xyrange.bottom());
    queueReplot();
}

void PlotWidget::setupMenus()
{
    // Range menu

    rangeMenu.setTitle("Range");
    connect(&rangeMenu, &QMenu::aboutToShow, this, [=]()
    {
        rangeMenu.clear();

        if (!dataTipGraph) {
            rangeMenu.addAction("No datatip");
        } else {

            rangeMenu.addAction("Set start of new range", this, [=]()
            {
                QString name = QInputDialog::getText(this, "New Range", "Name",
                                    QLineEdit::Normal,
                                    QString("Range %1").arg(dataTipGraph->csv->ranges.count() + 1));
                if (name.isEmpty()) { return; }
                RangePtr range(new Range());
                range->name = name;
                // Take start of graph range into account
                range->start = mPlotCrosshairIndex + dataTipGraph->range.start;
                range->end = dataTipGraph->range.end + dataTipGraph->range.start;
                dataTipGraph->csv->ranges.append(range);
            });
            rangeMenu.addAction("Set end of new range", this, [=]()
            {
                QString name = QInputDialog::getText(this, "New Range", "Name",
                                    QLineEdit::Normal,
                                    QString("Range %1").arg(dataTipGraph->csv->ranges.count() + 1));
                if (name.isEmpty()) { return; }
                RangePtr range(new Range());
                range->name = name;
                // Take start of graph range into account
                range->start = 0 + dataTipGraph->range.start;
                range->end = mPlotCrosshairIndex + dataTipGraph->range.start;
                dataTipGraph->csv->ranges.append(range);
            });
            foreach (RangePtr range, dataTipGraph->csv->ranges) {
                RangeWeakPtr rangeWkPtr(range);
                rangeMenu.addAction(QString("Set start of %1").arg(range->name),
                                    this, [this, rangeWkPtr]()
                {
                    RangePtr range(rangeWkPtr);
                    if (!range) { return; }
                    // Take start of graph range into account
                    range->start = mPlotCrosshairIndex + dataTipGraph->range.start;
                });
                rangeMenu.addAction(QString("Set end of %1").arg(range->name),
                                    this, [this, rangeWkPtr]()
                {
                    RangePtr range(rangeWkPtr);
                    if (!range) { return; }
                    // Take start of graph range into account
                    range->end = mPlotCrosshairIndex + dataTipGraph->range.start;
                });
            }
        }
    });

    // View menu

    viewMenu.addActions({
        ui->action_Show_All,
        ui->action_Equal_Axes,
        ui->action_Show_Mouse_Crosshair,
        ui->action_Show_Plot_Crosshair
    });
    viewMenu.addMenu("Mouse Crosshair Options")->addActions({
        ui->action_MouseCrosshair_Horizontal_Line,
        ui->action_MouseCrosshair_Vertical_Line,
        ui->action_MouseCrosshair_Dot
    });
    viewMenu.addMenu("Plot Crosshair Options")->addActions({
        ui->action_PlotCrosshair_Horizontal_Line,
        ui->action_PlotCrosshair_Vertical_Line,
        ui->action_PlotCrosshair_Dot
    });
    viewMenu.addMenu(&dataTipMenu);

    // Plot context menu

    plotContextMenu.addActions({
        ui->action_Show_All,
        ui->action_Equal_Axes
    });
    plotContextMenu.addMenu(&dataTipMenu);
    plotContextMenu.addMenu(&rangeMenu);

    // Data tip menu
    dataTipMenu.setTitle("Datatip Plot");
    connect(&dataTipMenu, &QMenu::aboutToShow, this, [=]()
    {
        dataTipMenu.clear();
        foreach (GraphPtr g, graphs) {
            QAction* action = dataTipMenu.addAction(g->name(), this,
                                  [this, gwk = g.toWeakRef()]()
            {
                GraphPtr g2(gwk);
                if (!g2) { return; }
                dataTipGraph = g2;
            });

            QPixmap pixmap(16, 16);
            pixmap.fill(g->color());
            QIcon icon(pixmap);
            action->setIcon(icon);

            action->setCheckable(true);
            if (dataTipGraph == g) {
                action->setChecked(true);
            }
        }
    });

    // Link menu

    linkMenu.addActions({
        ui->action_No_Link,
        ui->action_Link_to_Group_1,
        ui->action_Link_to_Group_2,
        ui->action_Link_to_Group_3
    });
    linkMenu.addSeparator();
    linkMenu.addActions({
        ui->action_Link_X_Zoom,
        ui->action_Link_Y_Zoom,
        ui->action_Link_X_Position,
        ui->action_Link_Y_Position
    });

    // Window menu
    windowMenu.addActions({
        ui->action_Dock_to_Screen_Top,
        ui->action_Dock_to_Screen_Bottom,
        ui->action_Dock_to_Screen_Left,
        ui->action_Dock_to_Screen_Right,
        ui->action_Undocked,
        ui->action_Tab_in_Main_Window
    });

    // Image menu
    imageMenu.addActions({
        ui->action_Copy_Image
    });
}

void PlotWidget::onAxisRangesChanged()
{
    // If range change is due to a sync from another linked plot, ignore it as
    // to not create a possible infinite loop.
    if (mRangesSyncedFromOutside) {
        mRangesSyncedFromOutside = false;
        mRangesChanged = false;
        return;
    }

    // React on ranges changed in a function on the event loop.
    // Use below flag in order to only react once when both x and y axis triggered
    // this slot.
    mRangesChanged = true;
    Utils::callLater(this, [=]()
    {
        if (mRangesChanged) {
            mRangesChanged = false;
            QCPAxis* xaxis = ui->plot->xAxis;
            if (!xaxis) { return; }
            QCPAxis* yaxis = ui->plot->yAxis;
            if (!yaxis) { return; }
            QRectF r(xaxis->range().lower, yaxis->range().lower,
                     xaxis->range().size(), yaxis->range().size());
            emit axisRangesChanged(r);
        }
    });
}

void PlotWidget::on_action_Equal_Axes_triggered()
{
    setEqualAxesAndReplot(ui->action_Equal_Axes->isChecked());
}

void PlotWidget::on_action_Show_All_triggered()
{
    showAll();
}

void PlotWidget::on_action_No_Link_triggered()
{
    setLinkGroup(0);
}

void PlotWidget::on_action_Link_to_Group_1_triggered()
{
    setLinkGroup(1);
}

void PlotWidget::on_action_Link_to_Group_2_triggered()
{
    setLinkGroup(2);
}

void PlotWidget::on_action_Link_to_Group_3_triggered()
{
    setLinkGroup(3);
}

void PlotWidget::on_action_Show_Plot_Crosshair_triggered()
{
    mPlotCrosshairVisibilityChangedByUser = true;
    mPlotCrosshair.setVisible(ui->action_Show_Plot_Crosshair->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWidget::on_action_Show_Mouse_Crosshair_triggered()
{
    mMouseCrosshair.setVisible(ui->action_Show_Mouse_Crosshair->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWidget::on_action_PlotCrosshair_Horizontal_Line_triggered()
{
    mPlotCrosshair.enableHline(ui->action_PlotCrosshair_Horizontal_Line->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWidget::on_action_PlotCrosshair_Vertical_Line_triggered()
{
    mPlotCrosshair.enableVline(ui->action_PlotCrosshair_Vertical_Line->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWidget::on_action_PlotCrosshair_Dot_triggered()
{
    mPlotCrosshair.enableDot(ui->action_PlotCrosshair_Dot->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWidget::on_action_MouseCrosshair_Horizontal_Line_triggered()
{
    mMouseCrosshair.enableHline(ui->action_MouseCrosshair_Horizontal_Line->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWidget::on_action_MouseCrosshair_Vertical_Line_triggered()
{
    mMouseCrosshair.enableVline(ui->action_MouseCrosshair_Vertical_Line->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWidget::on_action_MouseCrosshair_Dot_triggered()
{
    mMouseCrosshair.enableDot(ui->action_MouseCrosshair_Dot->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWidget::on_pushButton_view_clicked()
{
    viewMenu.popup(QCursor::pos());
}

void PlotWidget::on_pushButton_link_clicked()
{
    linkMenu.popup(QCursor::pos());
}

void PlotWidget::on_pushButton_window_clicked()
{
    windowMenu.popup(QCursor::pos());
}

void PlotWidget::on_pushButton_image_clicked()
{
    imageMenu.popup(QCursor::pos());
}

void PlotWidget::on_action_Dock_to_Screen_Top_triggered()
{
    emit dockWindow(DockTop);
}

void PlotWidget::on_action_Dock_to_Screen_Bottom_triggered()
{
    emit dockWindow(DockBottom);
}

void PlotWidget::on_action_Dock_to_Screen_Left_triggered()
{
    emit dockWindow(DockLeft);
}

void PlotWidget::on_action_Dock_to_Screen_Right_triggered()
{
    emit dockWindow(DockRight);
}

void PlotWidget::on_action_Undocked_triggered()
{
    emit dockWindow(DockFloating);
}

void PlotWidget::on_action_Tab_in_Main_Window_triggered()
{
    emit dockWindow(DockTab);
}


bool Graph::isCurve()
{
    return (curve != nullptr);
}

bool Graph::isGraph()
{
    return (graph != nullptr);
}

QString Graph::name()
{
    if (curve) {
        return curve->name();
    } else if (graph) {
        return graph->name();
    } else {
        return "";
    }
}

int Graph::dataCount()
{
    if (curve) {
        return curve->dataCount();
    } else if (graph) {
        return graph->dataCount();
    } else {
        return 0;
    }
}

double Graph::datax(int index)
{
    if (curve) {
        return curve->data()->at(index)->key;
    } else if (graph) {
        return graph->data()->at(index)->key;
    } else {
        return 0;
    }
}

double Graph::datay(int index)
{
    if (curve) {
        return curve->data()->at(index)->value;
    } else if (graph) {
        return graph->data()->at(index)->value;
    } else {
        return 0;
    }
}

QColor Graph::color()
{
    if (curve) {
        return curve->pen().color();
    } else if (graph) {
        return graph->pen().color();
    } else {
        return QColor();
    }
}

void PlotWidget::on_action_Copy_Image_triggered()
{
    QImage image = ui->plot->toPixmap(0, 0, 2.0).toImage();

    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setImage(image);
}

