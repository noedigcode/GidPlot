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

#include "subplot.h"

#include "utils.h"


Subplot::Subplot(QCPAxisRect *axisRect, QWidget *parentWidget)
    : QObject{parentWidget}, parentWidget(parentWidget), axisRect(axisRect)
{
    plot = axisRect->parentPlot();
    xAxis = axisRect->axis(QCPAxis::atBottom);
    yAxis = axisRect->axis(QCPAxis::atLeft);

    // We will manually add plottables to legends per subplot
    plot->setAutoAddPlottableToLegend(false);

    if (axisRect->insetLayout()->elements(false).contains(plot->legend)) {
        // Use already existing default plot legend for this subplot (axisrect)
        legend = plot->legend;
    } else {
        // Create new legend for this subplot (axisrect)
        legend = new QCPLegend();
        legend->setVisible(false);
        axisRect->insetLayout()->addElement(legend, Qt::AlignRight|Qt::AlignTop);
        axisRect->insetLayout()->setMargins(QMargins(12, 12, 12, 12));
        legend->setLayer("legend");
    }

    connect(plot, &QCustomPlot::mouseMove, this, &Subplot::onPlotMouseMove);
    connect(plot, &QCustomPlot::mousePress, this, &Subplot::onPlotMousePress);
    connect(plot, &QCustomPlot::mouseRelease, this, &Subplot::onPlotMouseRelease);
    connect(plot, &QCustomPlot::axisDoubleClick, this, &Subplot::onAxisDoubleClick);
    connect(plot, &QCustomPlot::itemDoubleClick, this, &Subplot::onPlotItemDoubleClick);

    setupCrosshairs();
    setupCrosshairsDialog();
    setupMenus();
    setupLink();
}

void Subplot::setupLink()
{
    link->supportPosZoom = true;
}

QCPLegend *Subplot::findLegend(QCPAxisRect *axisRect)
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

bool Subplot::inAxisRect(QPoint pos)
{
    return (axisRect && (axisRect->selectTest(pos, false) >= 0));
}

void Subplot::onPlotMousePress(QMouseEvent *event)
{
    if (!inAxisRect(event->pos())) { return; }

    mouse.mouseDown = true;
    mouse.button = event->button();
    mouse.start = event->pos();
    mouse.startRelativeToAxisRect = event->pos() - axisRect->topLeft();
    mouse.startXrange = xAxis->range();
    mouse.startYrange = yAxis->range();

    legendMouse.mouseDown = false;
    markerMouse.mouseDown = false;

    // Call markerMouseDown irrelevant of whether we are going to store
    // its pressed for a possible drag action.
    bool markerPressed = markerMouseDown(event);

    if (event->button() == Qt::LeftButton) {
        if (legend && legend->selectTest(event->pos(), false) >= 0) {
            legendMouse.mouseDown = true;
            plot->setInteraction(QCP::iRangeDrag, false); // Disable plot panning
            legendMouse.startRect = axisRect->insetLayout()->insetRect(0);
        } else if (markerPressed) {
            markerMouse.mouseDown = true;
            plot->setInteraction(QCP::iRangeDrag, false); // Disable plot panning
        }
    }
}

void Subplot::onPlotItemDoubleClick(QCPAbstractItem *item, QMouseEvent* event)
{
    if (!inAxisRect(event->pos())) { return; }

    foreach (MarkerPtr marker, mMarkers) {
        if (marker->textItem == item) {
            editMarkerText(marker);
            break;
        }
    }
}

void Subplot::onLegendItemRightClicked(QCPPlottableLegendItem *legendItem, const QPoint &pos)
{
    QCPAbstractPlottable* plottable = legendItem->plottable();

    QMenu* menu = new QMenu();
    connect(menu, &QMenu::aboutToHide, this, [=]() {
        menu->deleteLater();
    });

    QPixmap pixmap(16, 16);
    pixmap.fill(plottable->pen().color());
    QIcon icon(pixmap);
    menu->addAction(icon, plottable->name());

    menu->addAction(QIcon("://edit"), "Rename", this, [=]()
    {
        bool ok;
        QString name = QInputDialog::getText(parentWidget, "Curve Name", "Name",
                                             QLineEdit::Normal,
                                             plottable->name(), &ok);
        if (!ok) { return; }
        plottable->setName(name);
        plot->replot();
        updateLegendPlacement();
    });
    menu->addAction(QIcon("://color"), "Set Color", this, [=]()
    {
        QPen pen = plottable->pen();
        QColor color = QColorDialog::getColor(pen.color(), parentWidget);
        if (!color.isValid()) { return; }
        pen.setColor(color);
        plottable->setPen(pen);
        plot->replot();
        updateLegendPlacement();
    });
    menu->addAction(QIcon("://delete"), "Delete", this, [=]()
    {
        removeGraph(plottableGraphMap.value(plottable));
        plot->replot();
        updateLegendPlacement();
    });
    menu->popup(plot->mapToGlobal(pos));
}

void Subplot::onLegendRightClicked(QCPLegend* /*legend*/, const QPoint& /*pos*/)
{
    qDebug() << "Legend right-clicked";
}

void Subplot::plotData(CsvPtr csv, int ixcol, int iycol, Range range)
{
    bool firstPlot = (axisRect->plottables().size() == 0);

    QVector<double> x = csv->matrix->data[ixcol].mid(range.start, range.size());
    QVector<double> y = csv->matrix->data[iycol].mid(range.start, range.size());

    Matrix::VStats xstats = Matrix::vstats(x);
    Matrix::VStats ystats = Matrix::vstats(y);

    QPen pen = Graph::nextPen(mPenIndex++);

    GraphPtr graph;
    if (xstats.monotonicallyIncreasing) {
        // Graph is more efficient but can only be used if x is monotonically
        // increasing
        QCPGraph* qcpgraph = plot->addGraph(xAxis, yAxis);
        graph.reset(new Graph(qcpgraph));
        qcpgraph->setData(x, y);
        qcpgraph->setPen(pen);
        qcpgraph->setName(csv->matrix->heading(iycol));
        mPlotCrosshairSnap = SnapXOnly;
        mPlotCrosshair->showHorizontalLine = false;
        updateGuiForCrosshairOptions();
    } else {
        // Curve is used otherwise
        QCPCurve* qcpcurve = new QCPCurve(xAxis, yAxis);
        graph.reset(new Graph(qcpcurve));
        qcpcurve->setData(x, y);
        qcpcurve->setPen(pen);
        qcpcurve->setName(csv->matrix->heading(iycol));
        mPlotCrosshairSnap = SnapToClosest;
        setEqualAxesButDontReplot(true);
    }
    graph->csv = csv;
    graph->range = range;
    graph->xstats = xstats;
    graph->ystats = ystats;
    graphs.append(graph);
    plottableGraphMap.insert(graph->plottable(), graph);

    // Record min/max for all graphs
    if (firstPlot) {
        xmin = xstats.min;
        xmax = xstats.max;
        ymin = ystats.min;
        ymax = ystats.max;
    } else {
        xmin = qMin(xmin, xstats.min);
        xmax = qMax(xmax, xstats.max);
        ymin = qMin(ymin, ystats.min);
        ymax = qMax(ymax, ystats.max);
    }

    legend->addItem(new QCPPlottableLegendItem(legend, graph->plottable()));

    if (graphs.count() == 1) {
        dataTipGraph = graphs.value(0);
    }

    // Show legend if there is more than one curve
    if (axisRect->plottables().count() > 1) {
        legend->setVisible(true);
        updateLegendPlacement();
    }

    if (firstPlot) {
        // Only show all on first plot as to not mess up view when adding more
        // to existing plot.

        // Queue show all to give subplot a chance to be set up and displayed.
        QMetaObject::invokeMethod(this, [=]() { showAll(); }, Qt::QueuedConnection);

        connect(xAxis,
                QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                this, &Subplot::onAxisRangesChanged);
        connect(yAxis,
                QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                this, &Subplot::onAxisRangesChanged);

    } else {
        queueReplot();
    }
}

void Subplot::setXLabel(QString xlabel)
{
    xAxis->setLabel(xlabel);
    plot->replot();
}

void Subplot::setYLabel(QString ylabel)
{
    yAxis->setLabel(ylabel);
    plot->replot();
}

void Subplot::queueReplot()
{
    plot->replot(QCustomPlot::rpQueuedReplot);
}

void Subplot::syncAxisRanges(QRectF xyrange)
{
    mRangesSyncedFromOutside = true;

    QRectF r(xAxis->range().lower, yAxis->range().lower,
             xAxis->range().size(), yAxis->range().size());
    QPointF center = r.center();
    if (link->linkXpos) {
        center.setX(xyrange.center().x());
    }
    if (link->linkYpos) {
        center.setY(xyrange.center().y());
    }
    if (link->linkXzoom) {
        r.setWidth(xyrange.width());
    }
    if (link->linkYzoom) {
        r.setHeight(xyrange.height());
    }
    r.moveCenter(center);

    xAxis->setRange(r.left(), r.right());
    yAxis->setRange(r.top(), r.bottom());

    queueReplot();
}

void Subplot::syncDataTip(int index)
{
    if (!mPlotCrosshair->visible()) { return; }
    if (!dataTipGraph) { return; }

    index -= dataTipGraph->range.start;
    double x = dataTipGraph->datax(index);
    double y = dataTipGraph->datay(index);
    mPlotCrosshair->position->setCoords(x, y);
    mPlotCrosshair->text = QString("%1, %2 [%3]")
            .arg(x)
            .arg(y)
            .arg(index);
    queueReplot();
}

void Subplot::resizeEvent()
{
    if (mEqualAxes) {
        setEqualAxesAndReplot(true);
    }
    updateLegendPlacement();
}

void Subplot::showEvent()
{
    updateLegendPlacement();
}

void Subplot::keyEvent(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {

        QKeyEvent* keyEvent = (QKeyEvent*)event;
        switch (keyEvent->key()) {
        case Qt::Key_Shift:
            if (!mEqualAxes) {
                // Holding shift: vertical zoom
                axisRect->setRangeZoom(Qt::Vertical);
            }
            break;
        case Qt::Key_Control:
            // Holding control: vertical + horizontal zoom
            axisRect->setRangeZoom(Qt::Vertical | Qt::Horizontal);
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
                axisRect->setRangeZoom(Qt::Horizontal);
            }
            break;

        }
    }
}

void Subplot::showAll()
{
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
    double ypad = rect.height() / axisRect->height() * paddingPx;
    double xpad = rect.width() / axisRect->width() * paddingPx;

    rect.setLeft(rect.left() - xpad);
    rect.setRight(rect.right() + xpad);
    rect.setTop(rect.top() - ypad);
    rect.setBottom(rect.bottom() + ypad);

    if (mEqualAxes) {
        updatePlotForEqualAxes(rect);
    } else {
        xAxis->setRange(rect.left(), rect.right());
        yAxis->setRange(rect.top(), rect.bottom());
        queueReplot();
    }
}

void Subplot::setEqualAxesButDontReplot(bool fixed)
{
    mEqualAxes = fixed;
    actionEqualAxes->setChecked(mEqualAxes);

    if (fixed) {
        axisRect->setRangeZoom(Qt::Vertical | Qt::Horizontal);
    } else {
        axisRect->setRangeZoom(Qt::Horizontal);
    }
}

void Subplot::setEqualAxesAndReplot(bool fixed)
{
    setEqualAxesButDontReplot(fixed);

    if (fixed) {
        QRectF r(xAxis->range().lower, yAxis->range().lower,
                 xAxis->range().size(), yAxis->range().size());
        updatePlotForEqualAxes(r);
    }
}

void Subplot::updatePlotForEqualAxes(QRectF xyrange)
{
    int widthpx = axisRect->width();
    int heightpx = axisRect->height();

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

    xAxis->setRange(xyrange.left(), xyrange.right());
    yAxis->setRange(xyrange.top(), xyrange.bottom());
    queueReplot();
}

bool Subplot::plotMouseRightDragZoom(QMouseEvent *event)
{
    /* Handles right mouse button drag zoom on plot. Returns true if plot
     * should be updated (replot). */

    if (!mouse.isDragging) { return false; }
    if (mouse.button != Qt::RightButton) { return false; }

    QPointF axisPos = event->pos() - axisRect->topLeft();
    // Mouse movement relative to start
    double px = axisPos.x() - mouse.startRelativeToAxisRect.x();
    double dx = px / (float)axisRect->width();
    double py = axisPos.y() - mouse.startRelativeToAxisRect.y();
    double dy = py / (float)axisRect->height();

    if (mEqualAxes) {
        dy = -dx;
    }

    // Multiply by factor to make movement more sensitive
    dx *= 10.0;
    dy *= 10.0;

    // Calculate zoomed x range size
    double xsize = mouse.startXrange.size();
    if (dx > 0) {
        // Right mouse movement --> zoom in --> smaller axis range
        xsize /= 1.0 + qAbs(dx);
    } else {
        // Left mouse movement --> zoom out --> larger axis range
        xsize *= 1.0 + qAbs(dx);
    }

    // x "center factor" used to keep start point at same position
    double xcf = (mouse.startRelativeToAxisRect.x()) / (float)axisRect->width();
    // Apply x zoom
    xAxis->setRange(
                (mouse.startXrange.lower + mouse.startXrange.size() * xcf) - xsize * xcf,
                (mouse.startXrange.lower + mouse.startXrange.size() * xcf) + xsize * (1 - xcf));

    // Calculate zoomed y range size
    double ysize = mouse.startYrange.size();
    if (dy < 0) {
        // Upward mouse movement --> zoom in --> smaller axis range
        ysize /= 1.0 + qAbs(dy);
    } else {
        // Downward mouse movement --> zoom out --> larger axis range
        ysize *= 1.0 + qAbs(dy);
    }

    // y "center factor" used to keep start point at same position
    double ycf = (axisRect->height() - mouse.startRelativeToAxisRect.y()) / (float)axisRect->height();
    // Apply y zoom
    yAxis->setRange(
                (mouse.startYrange.lower + mouse.startYrange.size() * ycf) - ysize * ycf,
                (mouse.startYrange.lower + mouse.startYrange.size() * ycf) + ysize * (1 - ycf));

    return true;
}

void Subplot::setupCrosshairsDialog()
{
    connect(&mCrosshairsDialog, &CrosshairsDialog::settingsChanged,
            this, &Subplot::onCrosshairsDialogChanged);
}

void Subplot::showCrosshairsDialog()
{
    CrosshairsDialog::Settings s;
    s.plotCrosshair = mPlotCrosshair->visible();
    s.plotHline = mPlotCrosshair->showHorizontalLine;
    s.plotVline = mPlotCrosshair->showVerticalLine;
    s.plotDot = mPlotCrosshair->showCircle;
    s.mouseCrosshair = mMouseCrosshair->visible();
    s.mouseHline = mMouseCrosshair->showHorizontalLine;
    s.mouseVline = mMouseCrosshair->showVerticalLine;
    s.mouseDot = mMouseCrosshair->showCircle;

    mCrosshairsDialog.show(s);
}

void Subplot::onCrosshairsDialogChanged(CrosshairsDialog::Settings s)
{
    if (s.plotCrosshair != mPlotCrosshair->visible()) {
        mPlotCrosshairVisibilityChangedByUser = true;
        mPlotCrosshair->setVisible(s.plotCrosshair);
    }
    mPlotCrosshair->showHorizontalLine = s.plotHline;
    mPlotCrosshair->showVerticalLine = s.plotVline;
    mPlotCrosshair->showCircle = s.plotDot;
    mMouseCrosshair->setVisible(s.mouseCrosshair);
    mMouseCrosshair->showHorizontalLine = s.mouseHline;
    mMouseCrosshair->showVerticalLine = s.mouseVline;
    mMouseCrosshair->showCircle = s.mouseDot;

    updateGuiForCrosshairOptions();
}

void Subplot::setupMenus()
{
    // Plot context menu

    plotContextMenu.addAction(QIcon("://marker"), "Place Marker", this,
                              &Subplot::onActionPlaceMarkerTriggered);
    actionMeasure = plotContextMenu.addAction(QIcon("://measure"), "Measure", this,
                              &Subplot::onActionMeasureTriggered);
    plotContextMenu.addAction(QIcon("://showall"), "Show All", this,
                              [=]() { showAll(); });
    actionEqualAxes = plotContextMenu.addAction(QIcon("://equalaxes"), "Equal Axes", this,
                              &Subplot::onActionEqualAxesTriggered);
    actionEqualAxes->setCheckable(true);

    // Data tip menu
    dataTipMenu.setTitle("Datatip Plot");
    connect(&dataTipMenu, &QMenu::aboutToShow, this, &Subplot::onDataTipMenuAboutToShow);
    plotContextMenu.addMenu(&dataTipMenu);

    // Range menu
    rangeMenu.setTitle("Range");
    connect(&rangeMenu, &QMenu::aboutToShow, this, &Subplot::onRangeMenuAboutToShow);
    plotContextMenu.addMenu(&rangeMenu);

    plotContextMenu.addAction(QIcon("://crosshair"), "Crosshairs...",
                              this, [=]() { showCrosshairsDialog(); });

    plotContextMenu.addAction(QIcon("://link"), "Link to Other Plots...",
                                    this, &Subplot::linkSettingsTriggered);
}

void Subplot::onRangeMenuAboutToShow()
{
    rangeMenu.clear();

    if (!dataTipGraph) {
        rangeMenu.addAction("No datatip");
    } else {

        QMenu* newRangeMenu = rangeMenu.addMenu("New Range");

        newRangeMenu->addAction("Set start of new range", this, [=]()
        {
            QString name = QInputDialog::getText(parentWidget, "New Range", "Name",
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
        newRangeMenu->addAction("Set end of new range", this, [=]()
        {
            QString name = QInputDialog::getText(parentWidget, "New Range", "Name",
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
            QMenu* rangeXMenu = rangeMenu.addMenu(range->name);
            RangeWeakPtr rangeWkPtr(range);
            rangeXMenu->addAction("Set start", this, [this, rangeWkPtr]()
            {
                RangePtr range(rangeWkPtr);
                if (!range) { return; }
                // Take start of graph range into account
                range->start = mPlotCrosshairIndex + dataTipGraph->range.start;
            });
            rangeXMenu->addAction("Set end", this, [this, rangeWkPtr]()
            {
                RangePtr range(rangeWkPtr);
                if (!range) { return; }
                // Take start of graph range into account
                range->end = mPlotCrosshairIndex + dataTipGraph->range.start;
            });
        }
    }
}

void Subplot::onDataTipMenuAboutToShow()
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
}

void Subplot::onActionPlaceMarkerTriggered()
{
    GraphPtr graph = dataTipGraph;
    if (!graph) { graph = graphs.value(0); }
    if (!graph) { return; }

    QPoint pos = mouse.lastMovePos;
    ClosestCoord closest = findClosestCoord(pos, graph, mPlotCrosshairSnap);
    if (!closest.valid) { return; }

    MarkerPtr marker = addMarker(QPointF(closest.xCoord, closest.yCoord));

    marker->datasetName = graph->name();
    marker->dataIndex = closest.dataIndex;
    marker->text = "X: $x\nY: $y\nIndex: $i";
    updateMarkerText(marker);
    updateMarkerArrow(marker);

    plot->replot();
}

void Subplot::onActionMeasureTriggered()
{
    if (!mCurrentMeasure) {
        // Not busy with a measure. Start a new one.
        actionMeasure->setText("End Measure");
    } else {
        // Busy with a measure. End it here.
        clearCurrentMeasure();
        return;
    }

    QPoint pos = mouse.lastMovePos;
    double x = xAxis->pixelToCoord(pos.x());
    double y = yAxis->pixelToCoord(pos.y());

    MeasurePtr m(new Measure());
    m->tag = QString("Measure %1").arg(mMeasureCounter++);

    QList<MarkerPtr> ab;
    for (int i = 0; i < 2; i++) {

        MarkerPtr a = addMarker(QPointF(x, y));
        a->text = QString("%1 A\n$x, $y").arg(m->tag);
        updateMarkerText(a);
        updateMarkerArrow(a);
        a->plotMarker->showCircle = false;
        a->plotMarker->showVerticalLine = true;
        a->plotMarker->showHorizontalLine = true;

        ab.append(a);
    }

    m->a = ab.value(0);
    m->b = ab.value(1);

    mMeasures.append(m);
    mCurrentMeasure = m;

    plot->replot();
}

void Subplot::onActionEqualAxesTriggered()
{
    setEqualAxesAndReplot(actionEqualAxes->isChecked());
}

bool Subplot::plotMouseMove(QMouseEvent *event)
{
    bool replot = false;

    if (inAxisRect(event->pos()) && xAxis && yAxis) {

        double mouseX = xAxis->pixelToCoord(event->x());
        double mouseY = yAxis->pixelToCoord(event->y());

        if (mCurrentMeasure) {
            QPointF delta = QPointF(mouseX, mouseY) -
                    mCurrentMeasure->a->plotMarker->position->coords();
            mCurrentMeasure->b->plotMarker->position->setCoords(mouseX, mouseY);
            mCurrentMeasure->b->xCoord = mouseX;
            mCurrentMeasure->b->yCoord = mouseY;
            mCurrentMeasure->b->text = QString("%1 B\n$x, $y\ndx: %2, dy: %3")
                    .arg(mCurrentMeasure->tag)
                    .arg(delta.x())
                    .arg(delta.y());
            updateMarkerText(mCurrentMeasure->b);
            updateMarkerArrow(mCurrentMeasure->b);

            replot = true;
        }

        // - Only update crosshairs if not dragging as it could slow down dragging
        // - Use event buttons and not recorded mouse down button, as button
        //   could have been pressed down on another subplot and move over
        //   this one.

        if (event->buttons() == Qt::NoButton) {

            QString text;

            if (mPlotCrosshair->visible() && dataTipGraph) {

                QElapsedTimer timer;
                timer.start();

                ClosestCoord closest = findClosestCoord(event->pos(),
                                                        dataTipGraph,
                                                        mPlotCrosshairSnap);

                // Update the plot crosshair
                // If it takes too long to update, disable the plot crosshair
                // automatically. However, if the user has enabled it, leave
                // it alone.
                if ((timer.elapsed() > 100) && !mPlotCrosshairVisibilityChangedByUser) {
                    mPlotCrosshair->setVisible(false);
                    updateGuiForCrosshairOptions();
                } else if (closest.valid) {
                    mPlotCrosshair->position->setCoords(closest.xCoord, closest.yCoord);
                    mPlotCrosshair->text = QString("%1, %2 [%3]")
                            .arg(closest.xCoord)
                            .arg(closest.yCoord)
                            .arg(closest.dataIndex);
                    text = QString("Plot: [%1] (%2, %3)")
                            .arg(closest.dataIndex)
                            .arg(closest.xCoord)
                            .arg(closest.yCoord);
                    emit dataTipChanged(link->group,
                                closest.dataIndex + dataTipGraph->range.start);
                    replot = true;

                    mPlotCrosshairIndex = closest.dataIndex;
                }
            }

            // Update mouse crosshair
            if (mMouseCrosshair->visible()) {
                mMouseCrosshair->position->setCoords(mouseX, mouseY);
                mMouseCrosshair->text = QString("%1, %2").arg(mouseX).arg(mouseY);
                replot = true;
            }
        }
    }

    return replot;
}

bool Subplot::legendMouseMove(QMouseEvent *event)
{
    if (!axisRect) { return false; }
    if (!legend) { return false; }

    bool replot = false;

    if (legendMouse.mouseDown) {

        QPoint delta = event->pos() - mouse.start;

        QSize legendSize = legend->minimumOuterSizeHint();
        QRect axisRectPixels = axisRect->rect();
        QRect legendRectPixels(
                    legendMouse.startRect.x() * axisRectPixels.width(),
                    legendMouse.startRect.y() * axisRectPixels.height(),
                    legendSize.width(),
                    legendSize.height());
        legendRectPixels.translate(delta);

        int movex = 0;
        // Keep right edge inside plot
        if (legendRectPixels.right() > axisRectPixels.width()) {
            movex = -(legendRectPixels.right() - axisRectPixels.width());
        }
        // Keep left edge inside plot (overrides right)
        if (legendRectPixels.left() < 0) {
            movex = -legendRectPixels.left();
        }
        int movey = 0;
        // Keep bottom edge inside plot
        if (legendRectPixels.bottom() > axisRectPixels.height()) {
            movey = -(legendRectPixels.bottom() - axisRectPixels.height());
        }
        // Keep top edge inside plot (overrides bottom)
        if (legendRectPixels.top() < 0) {
            movey = -legendRectPixels.top();
        }
        legendRectPixels.translate(movex, movey);

        // Normalise
        QRectF legendRectNorm(
                    (double)(legendRectPixels.left()) / axisRectPixels.width(),
                    (double)(legendRectPixels.top()) / axisRectPixels.height(),
                    (double)(legendRectPixels.width()) / axisRectPixels.width(),
                    (double)(legendRectPixels.height()) / axisRectPixels.height());

        // Note that QCustomPlot::axisRect()->insetLayout() insetPlacement must
        // be QCPLayoutInset::ipFree for below to take effect.
        // This is set elsewhere.
        axisRect->insetLayout()->setInsetRect(0, legendRectNorm);

        replot = true;
    }

    return replot;
}

void Subplot::updateLegendPlacement()
{
    if (!plot->isVisible()) { return; }

    QRect axisRectPixels = axisRect->rect();
    QSize legendSize = legend->minimumOuterSizeHint();

    QRectF rect = axisRect->insetLayout()->insetRect(0);
    double x = rect.x() * axisRectPixels.width();
    double y = rect.y() * axisRectPixels.height();
    double w = double(legendSize.width());
    double h = double(legendSize.height());

    if (mFirstLegendPlacement) {
        mFirstLegendPlacement = false;
        // Set free layout so legend can be moved freely
        axisRect->insetLayout()->setInsetPlacement(0, QCPLayoutInset::ipFree);
        // Zero margins so legend can be moved to the edges
        axisRect->insetLayout()->setMargins(QMargins(0, 0, 0, 0));
        x = axisRectPixels.width() - legendSize.width() - 10;
        y = 10;
    } else {
        // Ensure the legend is within view
        if ((x + w) > axisRectPixels.width()) {
            x = axisRectPixels.width() - w;
        }
        if (x < 0) { x = 0; }
        if ((y + h) > axisRectPixels.height()) {
            y = axisRectPixels.height() - h;
        }
        if (y < 0) { y = 0; }
    }

    double xNorm = x / axisRectPixels.width();
    double yNorm = y / axisRectPixels.height();
    double wNorm = w / axisRectPixels.width();
    double hNorm = h / axisRectPixels.height();

    rect.setX(xNorm);
    rect.setY(yNorm);
    rect.setWidth(wNorm);
    rect.setHeight(hNorm);
    axisRect->insetLayout()->setInsetRect(0, rect);
    plot->replot();
}

void Subplot::removeGraph(GraphPtr graph)
{
    if (!graph) { return; }

    QCPPlottableLegendItem* legendItem = legend->itemWithPlottable(graph->plottable());
    if (legendItem) {
        legend->remove(legendItem);
    }
    plot->removePlottable(graph->plottable());
    graphs.removeAll(graph);
    plottableGraphMap.remove(graph->plottable());

    if (dataTipGraph == graph) {
        dataTipGraph = graphs.value(0);
    }

    // Recalculate overall min/max for remaining graphs
    bool first = true;
    foreach (GraphPtr graph, graphs) {
        if (first) {
            first = false;
            xmin = graph->xstats.min;
            xmax = graph->xstats.max;
            ymin = graph->ystats.min;
            ymax = graph->ystats.max;
        } else {
            xmin = qMin(xmin, graph->xstats.min);
            xmax = qMax(xmax, graph->xstats.max);
            ymin = qMin(ymin, graph->ystats.min);
            ymax = qMax(ymax, graph->ystats.max);
        }
    }
}

MarkerPtr Subplot::addMarker(QPointF coord)
{
    PlotMarkerItem* dot = new PlotMarkerItem(plot, axisRect, xAxis, yAxis);
    dot->setLayer("markers");
    dot->position->setCoords(coord);
    dot->circleSize = 10;
    dot->circleFillBrush = QBrush(QColor(255, 0, 0, 100));
    dot->linePen.setStyle(Qt::DashLine);

    QCPItemText* label = new QCPItemText(plot);
    label->setClipAxisRect(axisRect);
    label->position->setAxes(xAxis, yAxis);
    label->setFont(plot->font());
    label->setLayer("marker-labels");
    label->position->setParentAnchor(dot->anchor);
    // Pixel position is set relative to the anchor assigned above
    label->position->setPixelPosition(label->position->pixelPosition() + QPointF(10, -10));
    label->setPadding(QMargins(2, 2, 2, 2));
    label->setPen(QPen(Qt::black));
    label->setBrush(QBrush(QColor("#f8fabe")));
    label->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);

    QCPItemLine* arrow = new QCPItemLine(plot);
    arrow->setClipAxisRect(axisRect);
    foreach (QCPItemPosition* position, arrow->positions()) {
        position->setAxes(xAxis, yAxis);
    }
    arrow->setLayer("markers");
    arrow->end->setCoords(coord);
    arrow->setHead(QCPLineEnding::esSpikeArrow);

    MarkerPtr marker(new Marker());
    marker->xCoord = coord.x();
    marker->yCoord = coord.y();
    marker->plotMarker = dot;
    marker->textItem = label;
    marker->arrow = arrow;
    marker->text = "$x, $y";

    mMarkers.prepend(marker);
    updateMarkerText(marker);
    updateMarkerArrow(marker);

    plot->replot();

    return marker;
}

bool Subplot::markerMouseDown(QMouseEvent *mouseEvent)
{
    MarkerPtr marker = findMarkerUnderPos(mouseEvent->pos());
    markerMouse.marker = marker;

    if (marker) {
        // Bring marker to the front (setting layer again puts it a the top)
        marker->plotMarker->setLayer(marker->plotMarker->layer());
        marker->textItem->setLayer(marker->textItem->layer());
        marker->arrow->setLayer(marker->arrow->layer());
        // Move to start of markers list (top to bottom)
        mMarkers.removeAll(marker);
        mMarkers.prepend(marker);
        plot->replot();

        markerMouse.startTextPixelPos = marker->textItem->position->pixelPosition();
    }
    return !marker.isNull();
}

bool Subplot::markerMouseMove(QMouseEvent *mouseEvent)
{
    if (!markerMouse.marker) { return false; }

    bool replot = false;

    if (markerMouse.mouseDown) {
        QPoint delta = mouseEvent->pos() - mouse.start;
        markerMouse.marker->textItem->position->setPixelPosition(
                    markerMouse.startTextPixelPos + QPointF(delta));
        updateMarkerArrow(markerMouse.marker);
        replot = true;
    }

    return replot;
}

void Subplot::markerMouseUp()
{
    markerMouse.marker.reset();
    markerMouse.mouseDown = false;
}

MarkerPtr Subplot::findMarkerUnderPos(QPoint pos)
{
    MarkerPtr ret;

    // Markers are kept top to bottom in the list, so iterating will catch the
    // top-most one first
    foreach (MarkerPtr marker, mMarkers) {
        if (marker->textItem) {
            double dist = marker->textItem->selectTest(pos, false);
            if ((dist >= 0) && (dist < plot->selectionTolerance())) {
                ret = marker;
                break;
            }
        }
    }

    return ret;
}

bool Subplot::markerRightClick(QPoint pos)
{
    MarkerPtr marker = markerMouse.marker;
    if (!marker) { return false; }

    QMenu* menu = new QMenu();
    connect(menu, &QMenu::aboutToHide, this, [=]() { menu->deleteLater(); });

    // Use weak pointer to not capture shared pointer in lambdas that might
    // hold on to it.
    QWeakPointer<Marker> mWptr(markerMouse.marker);

    menu->addAction(QIcon("://edit"), "Edit Text",
                    this, [this, mWptr = marker.toWeakRef()]()
    {
        MarkerPtr m(mWptr);
        if (!m) { return; }
        editMarkerText(m);
    });

    menu->addAction(QIcon("://delete"), "Delete Marker",
                    this, [this, mWptr = marker.toWeakRef()]()
    {
        MarkerPtr m(mWptr);
        if (!m) { return; }
        deleteMarker(m);
    });

    menu->popup(plot->mapToGlobal(pos));

    return true;
}

void Subplot::updateMarkerArrow(MarkerPtr marker)
{
    marker->arrow->end->setCoords(marker->plotMarker->position->coords());
    QPointF p = marker->arrow->end->pixelPosition();
    QRectF r(marker->textItem->topLeft->pixelPosition(),
             marker->textItem->bottomRight->pixelPosition());

    QCPItemText* text = marker->textItem;

    QCPItemAnchor* anchor = nullptr;
    if (p.x() < r.left()) {
        if (p.y() > r.bottom()) {
            anchor = text->bottomLeft;
        } else if (p.y() < r.top()) {
            anchor = text->topLeft;
        } else {
            anchor = text->left;
        }
    } else if (p.x() > r.right()) {
        if (p.y() > r.bottom()) {
            anchor = text->bottomRight;
        } else if (p.y() < r.top()) {
            anchor = text->topRight;
        } else {
            anchor = text->right;
        }
    } else if (p.y() < r.top()) {
        anchor = text->top;
    } else {
        anchor = text->bottom;
    }

    marker->arrow->start->setParentAnchor(anchor);
}

void Subplot::updateMarkerText(MarkerPtr marker)
{
    QString text = marker->text;
    text.replace("$i", QString::number(marker->dataIndex));
    text.replace("$x", QString::number(marker->xCoord));
    text.replace("$y", QString::number(marker->yCoord));
    text.replace("$name", marker->datasetName);
    text.replace("$$", "$");
    marker->textItem->setText(text);
}

void Subplot::editMarkerText(MarkerPtr marker)
{
    if (!marker) { return; }

    mMarkerEditDialog.edit(marker->text,
                           marker->plotMarker->showHorizontalLine,
                           marker->plotMarker->showVerticalLine,
                           marker->plotMarker->showCircle,
                           [this, markerWkPtr = marker.toWeakRef()]()
    {
        MarkerPtr m(markerWkPtr);
        if (!m) { return; }

        m->text = mMarkerEditDialog.text();
        m->plotMarker->showHorizontalLine = mMarkerEditDialog.showHline();
        m->plotMarker->showVerticalLine = mMarkerEditDialog.showVline();
        m->plotMarker->showCircle = mMarkerEditDialog.showDot();

        updateMarkerText(m);
        updateMarkerArrow(m);
        plot->replot();
    });
}

void Subplot::deleteMarker(MarkerPtr marker)
{
    if (!marker) { return; }

    plot->removeItem(marker->arrow);
    plot->removeItem(marker->textItem);
    plot->removeItem(marker->plotMarker);
    mMarkers.removeAll(marker);

    // Remove related measure
    foreach (MeasurePtr m, mMeasures) {
        if ((m->a == marker) || (m->b == marker)) {
            mMeasures.removeAll(m);
            if (mCurrentMeasure == m) {
                clearCurrentMeasure();
            }
        }
    }

    plot->replot();
}

void Subplot::clearCurrentMeasure()
{
    mCurrentMeasure.reset();
    // Restore measure action text that was set to end measure when started
    actionMeasure->setText("Measure");
}

void Subplot::setupCrosshairs()
{
    mPlotCrosshair = new PlotMarkerItem(plot, axisRect, xAxis, yAxis);
    mPlotCrosshair->setClipAxisRect(axisRect);
    mPlotCrosshair->setLayer("crosshairs");
    mPlotCrosshair->circleSize = 7;
    mPlotCrosshair->showVerticalLine = true;

    mMouseCrosshair = new PlotMarkerItem(plot, axisRect, xAxis, yAxis);
    mMouseCrosshair->setClipAxisRect(axisRect);
    mMouseCrosshair->setLayer("crosshairs");
    mMouseCrosshair->circleSize = 7;
    mMouseCrosshair->showVerticalLine = true;
    mMouseCrosshair->showHorizontalLine = true;
    mMouseCrosshair->setVisible(false);

    updateGuiForCrosshairOptions();
}

void Subplot::updateGuiForCrosshairOptions()
{
    queueReplot();
}

void Subplot::storeAndDisableCrosshairs()
{
    lastPlotCrosshairVisible = mPlotCrosshair->visible();
    mPlotCrosshair->setVisible(false);
    lastMouseCrosshairVisible = mMouseCrosshair->visible();
    mMouseCrosshair->setVisible(false);
}

void Subplot::restoreCrosshairs()
{
    mPlotCrosshair->setVisible(lastPlotCrosshairVisible);
    mMouseCrosshair->setVisible(lastMouseCrosshairVisible);
}

void Subplot::onPlotMouseMove(QMouseEvent *event)
{
    // Note: Do not limit to axisRect as mouse may be outside of window but
    // busy dragging.

    mouse.lastMovePos = event->pos();

    // Start dragging if we have moved further than threshold
    if (mouse.mouseDown && !mouse.isDragging) {
        double px = event->pos().x() - mouse.start.x();
        double py = event->pos().y() - mouse.start.y();
        if (QPointF(px, py).manhattanLength() > 5.0) {
            mouse.isDragging = true;
        }
    }

    bool replot = false;
    if (legendMouse.mouseDown) {
        replot |= legendMouseMove(event);
    } else if (markerMouse.mouseDown) {
        replot |= markerMouseMove(event);
    } else {
        replot |= plotMouseMove(event);
    }

    replot |= plotMouseRightDragZoom(event);

    if (replot) {
        queueReplot();
    }
}

void Subplot::onPlotMouseRelease(QMouseEvent *event)
{
    mouse.mouseDown = false;

    if (event->button() == Qt::RightButton) {
        if (!mouse.isDragging) {
            // Was not dragging. Trigger mouse right click event
            plotRightClicked(event->pos());
        }
    } else if (event->button() == Qt::LeftButton) {
        if (!mouse.isDragging) {
            // Was not dragging. Trigger mouse click event
            plotLeftClicked(event->pos());
        }
        plot->setInteraction(QCP::iRangeDrag, true); // Re-enable plot panning
    }

    legendMouse.mouseDown = false;
    markerMouseUp();
    mouse.mouseDown = false;
    mouse.isDragging = false;
}

void Subplot::plotRightClicked(const QPoint &pos)
{
    if (!inAxisRect(pos)) { return; }

    bool used = false;

    if (legend) {

        // Check if plottable item in legend has been right clicked
        for (int i = 0; i < legend->itemCount(); i++) {
            QCPAbstractLegendItem* legendItem = legend->item(i);
            if (legendItem && legendItem->selectTest(pos, false) >= 0) {
                QCPPlottableLegendItem* plItem = qobject_cast<QCPPlottableLegendItem*>(legendItem);
                if (plItem) {
                    onLegendItemRightClicked(plItem, pos);
                    used = true;
                }
            }
        }

        if (!used) {
            // Plottable item was not clicked. // Check if legend has been right clicked
            if (legend->selectTest(pos, false) >= 0) {
                onLegendRightClicked(legend, pos);
                used = true;
            }
        }
    }

    if (!used) {
        used = markerRightClick(pos);
    }

    if (!used) {
        // Plot area
        plotContextMenu.popup(plot->mapToGlobal(pos));
    }
}

void Subplot::plotLeftClicked(const QPoint& /*pos*/)
{
    // Clear current measure whether this click is inside this axisRect or not
    if (mCurrentMeasure) {
        // Stop measuring
        clearCurrentMeasure();
    }
}

void Subplot::onAxisRangesChanged()
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
            QRectF r(xAxis->range().lower, yAxis->range().lower,
                     xAxis->range().size(), yAxis->range().size());
            emit axisRangesChanged(link->group, r);
        }
    });
}

void Subplot::onAxisDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part,
                                QMouseEvent* /*event*/)
{
    bool ourAxis = (axis == xAxis) || (axis == yAxis);
    if (!ourAxis) { return; }

    if (part == QCPAxis::spAxisLabel) {
        bool ok;
        QString text = QInputDialog::getText(parentWidget, "Axis Label", "Label",
                                              QLineEdit::Normal,
                                              axis->label(),
                                              &ok);
        if (ok) {
            axis->setLabel(text);
            plot->replot();
        }
    }
}

Subplot::ClosestCoord Subplot::findClosestCoord(QPoint pos, GraphPtr graph, CrosshairSnap snap)
{
    ClosestCoord closest;

    if (!graph || !xAxis || !yAxis) {
        closest.valid = false;
        return closest;
    }

    for (int i = 0; i < graph->dataCount(); ++i) {

        double xCrd = graph->datax(i);
        int xPixel = xAxis->coordToPixel(xCrd);
        double yCrd = graph->datay(i);

        int dist = 0;

        if (snap == SnapXOnly) {
            dist = qAbs(pos.x() - xPixel);
        } else if (snap == SnapToClosest) {
            int yPixel = yAxis->coordToPixel(yCrd);
            // Calculate distance in pixels to account for zooms
            dist = qSqrt( qPow(pos.x() - xPixel, 2)
                          + qPow(pos.y() - yPixel, 2) );
        }

        if ((i == 0) || (dist < closest.distancePixels)) {
            closest.valid = true;
            closest.distancePixels = dist;
            closest.xCoord = xCrd;
            closest.yCoord = yCrd;
            closest.dataIndex = i;
            if (dist == 0) {
                // We're not gonna find anything closer
                break;
            }
        }
    }

    return closest;
}


