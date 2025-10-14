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

    setupMenus();

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

    ui->plot->installEventFilter(this);

    ui->label_coordinates->clear();

    setupCrosshairs();

    updateGuiForLinkGroup();
}

PlotWindow::~PlotWindow()
{
    delete ui;
}

int PlotWindow::tag()
{
    return mTag;
}

QCustomPlot *PlotWindow::plotWidget()
{
    return ui->plot;
}

void PlotWindow::plotData(CsvPtr csv, int ixcol, int iycol, Range range)
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
        mPlotCrosshair->horizontalLine = false;
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
    plottableGraphMap.insert(graph->plottable(), graph);

    if (graphs.count() == 1) {
        dataTipGraph = graphs.value(0);
    }

    if (ui->plot->plottableCount() > 1) {
        ui->plot->legend->setVisible(true);
        updateLegendPlacement();
    }

    if (firstPlot) {
        // Only show all on first plot as to not mess up view when adding more
        // to existing plot.
        showAll();

        QCPAxis* xaxis = ui->plot->xAxis;
        if (xaxis) {
            connect(xaxis,
                    QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                    this, &PlotWindow::onAxisRangesChanged);
        }
        QCPAxis* yaxis = ui->plot->yAxis;
        if (yaxis) {
            connect(yaxis,
                    QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                    this, &PlotWindow::onAxisRangesChanged);
        }

    } else {
        queueReplot();
    }
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
    ui->plot->xAxis->setLabel(xlabel);
    ui->plot->replot();
}

void PlotWindow::setYLabel(QString ylabel)
{
    ui->plot->yAxis->setLabel(ylabel);
    ui->plot->replot();
}

void PlotWindow::showAll()
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

void PlotWindow::setEqualAxesButDontReplot(bool fixed)
{
    mEqualAxes = fixed;
    ui->action_Equal_Axes->setChecked(mEqualAxes);

    if (fixed) {
        ui->plot->axisRect()->setRangeZoom(Qt::Vertical | Qt::Horizontal);
    } else {
        ui->plot->axisRect()->setRangeZoom(Qt::Horizontal);
    }
}

void PlotWindow::setEqualAxesAndReplot(bool fixed)
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

void PlotWindow::syncAxisRanges(QRectF xyrange)
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

void PlotWindow::syncDataTip(int index)
{
    if (mPlotCrosshair->visible()) {
        if (dataTipGraph) {
            index -= dataTipGraph->range.start;
            double x = dataTipGraph->datax(index);
            double y = dataTipGraph->datay(index);
            mPlotCrosshair->position->setCoords(x, y);
            queueReplot();
        }
    }
}

bool PlotWindow::eventFilter(QObject* /*watched*/, QEvent *event)
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

void PlotWindow::resizeEvent(QResizeEvent* /*event*/)
{
    if (mEqualAxes) {
        setEqualAxesAndReplot(true);
    }
    updateLegendPlacement();
}

void PlotWindow::showEvent(QShowEvent* /*event*/)
{
    updateLegendPlacement();
}

void PlotWindow::clearCurrentMeasure()
{
    mCurrentMeasure.reset();
    // Restore measure action text that was set to end measure when started
    ui->action_Measure->setText("Measure");
}

PlotWindow::MarkerPtr PlotWindow::addMarker(QPointF coord)
{
    PlotMarkerItem* dot = new PlotMarkerItem(ui->plot);
    dot->setLayer("markers");
    dot->position->setCoords(coord);
    dot->size = 10;
    dot->brush = QBrush(QColor(255, 0, 0, 100));

    QCPItemText* label = new QCPItemText(ui->plot);
    label->setFont(ui->plot->font());
    label->setLayer("markers");
    label->position->setParentAnchor(dot->anchor);
    // Pixel position is set relative to the anchor assigned above
    label->position->setPixelPosition(label->position->pixelPosition() + QPointF(10, -10));
    label->setPadding(QMargins(2, 2, 2, 2));
    label->setPen(QPen(Qt::black));
    label->setBrush(QBrush(QColor("#f8fabe")));
    label->setPositionAlignment(Qt::AlignLeft | Qt::AlignBottom);

    QCPItemLine* arrow = new QCPItemLine(ui->plot);
    arrow->setLayer("markers");
    arrow->end->setCoords(coord);
    arrow->setHead(QCPLineEnding::esSpikeArrow);

    MarkerPtr marker(new Marker());
    marker->xCoord = coord.x();
    marker->yCoord = coord.y();
    marker->dot = dot;
    marker->textItem = label;
    marker->arrow = arrow;
    marker->text = "$x, $y";

    mMarkers.prepend(marker);
    updateMarkerArrow(marker);
    updateMarkerText(marker);

    ui->plot->replot();

    return marker;
}

PlotWindow::MarkerPtr PlotWindow::findMarkerUnderPos(QPoint pos)
{
    MarkerPtr ret;

    // Markers are kept top to bottom in the list, so iterating will catch the
    // top-most one first
    foreach (MarkerPtr marker, mMarkers) {
        if (marker->textItem) {
            double dist = marker->textItem->selectTest(pos, false);
            if ((dist >= 0) && (dist < ui->plot->selectionTolerance())) {
                ret = marker;
                break;
            }
        }
    }

    return ret;
}

bool PlotWindow::markerMouseDown(QMouseEvent* mouseEvent)
{
    mouseDownMarker = findMarkerUnderPos(mouseEvent->pos());
    if (mouseDownMarker) {
        // Bring marker to the front (setting layer again puts it a the top)
        mouseDownMarker->dot->setLayer("markers");
        mouseDownMarker->textItem->setLayer("markers");
        mouseDownMarker->arrow->setLayer("markers");
        // Move to start of markers list (top to bottom)
        mMarkers.removeAll(mouseDownMarker);
        mMarkers.prepend(mouseDownMarker);
        ui->plot->replot();

        markerTextPixelPosAtMouseDown = mouseDownMarker->textItem->position->pixelPosition();
    }
    return !mouseDownMarker.isNull();
}

bool PlotWindow::markerMouseMove(QMouseEvent* mouseEvent)
{
    if (!mouseDownMarker) { return false; }

    bool replot = false;
    bool dragging = lMouseDownOnMarker;

    if (dragging) {
        QPoint delta = mouseEvent->pos() - lMouseStart;
        mouseDownMarker->textItem->position->setPixelPosition(
                    markerTextPixelPosAtMouseDown + QPointF(delta));
        updateMarkerArrow(mouseDownMarker);
        replot = true;
    }

    return replot;
}

void PlotWindow::markerMouseUp()
{
    mouseDownMarker.reset();
}

bool PlotWindow::markerRightClick(QPoint pos)
{
    MarkerPtr marker = findMarkerUnderPos(pos);
    if (!marker) { return false; }

    QMenu* menu = new QMenu();
    connect(menu, &QMenu::aboutToHide, this, [=]() { menu->deleteLater(); });

    // Use weak pointer to not capture shared pointer in lambdas that might
    // hold on to it.
    QWeakPointer<Marker> mWptr(marker);

    menu->addAction(ui->action_marker_edit_text->icon(), "Edit Text", [=]()
    {
        MarkerPtr m2(mWptr);
        if (!m2) { return; }
        editMarkerText(m2);
    });

    menu->addAction(ui->action_marker_delete->icon(), "Delete Marker", [=]()
    {
        MarkerPtr m2(mWptr);
        if (!m2) { return; }
        deleteMarker(m2);
    });

    menu->popup(ui->plot->mapToGlobal(pos));

    return true;
}

void PlotWindow::updateMarkerArrow(MarkerPtr m)
{
    m->arrow->end->setCoords(m->dot->position->coords());
    QPointF p = m->arrow->end->pixelPosition();
    QRectF r(m->textItem->topLeft->pixelPosition(),
             m->textItem->bottomRight->pixelPosition());

    QCPItemText* text = m->textItem;

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

    m->arrow->start->setParentAnchor(anchor);
}

void PlotWindow::updateMarkerText(MarkerPtr marker)
{
    QString text = marker->text;
    text.replace("$i", QString::number(marker->dataIndex));
    text.replace("$x", QString::number(marker->xCoord));
    text.replace("$y", QString::number(marker->yCoord));
    text.replace("$name", marker->datasetName);
    text.replace("$$", "$");
    marker->textItem->setText(text);
}

void PlotWindow::editMarkerText(MarkerPtr marker)
{
    if (!marker) { return; }

    bool ok;
    QString text = QInputDialog::getMultiLineText(this, "Marker Text",
        "Text ($i = index, $x, $y, $name = plot name)",
        marker->text, &ok);
    if (!ok) { return; }
    marker->text = text;
    updateMarkerText(marker);
    ui->plot->replot();
}

void PlotWindow::deleteMarker(MarkerPtr marker)
{
    if (!marker) { return; }

    ui->plot->removeItem(marker->arrow);
    ui->plot->removeItem(marker->textItem);
    ui->plot->removeItem(marker->dot);
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

    ui->plot->replot();
}

void PlotWindow::plottableClick(QCPAbstractPlottable* /*plottable*/,
                                int /*dataIndex*/, QMouseEvent* /*event*/)
{

}

void PlotWindow::onPlotMouseMove(QMouseEvent *event)
{
    mLastMouseMovePos = event->pos();

    bool replot = false;
    if (lMouseDownOnLegend) {
        replot |= legendMouseMove(event);
    } else if (lMouseDownOnMarker) {
        replot |= markerMouseMove(event);
    } else {
        replot |= plotMouseMove(event);
    }

    replot |= plotMouseRightDrag(event);

    if (replot) {
        queueReplot();
    }
}

void PlotWindow::onPlotMousePress(QMouseEvent* event)
{
    bool markerPressed = markerMouseDown(event);

    if (event->button() == Qt::RightButton) {
        rMouseZoom.start = event->pos() - ui->plot->axisRect()->topLeft();
        rMouseZoom.origXrange = ui->plot->xAxis->range();
        rMouseZoom.origYrange = ui->plot->yAxis->range();
        rMouseZoom.mouseDown = true;
    } else if (event->button() == Qt::LeftButton) {
        lMouseDown = true;
        lMouseStart = event->pos();

        bool used = false;

        if (ui->plot->legend->selectTest(event->pos(), false) >= 0) {
            lMouseDownOnLegend = true;
            ui->plot->setInteraction(QCP::iRangeDrag, false); // Disable plot panning
            mLegendStartRect = ui->plot->axisRect()->insetLayout()->insetRect(0);
            used = true;
        }

        if (!used) {
            if (markerPressed) {
                used = true;
                lMouseDownOnMarker = true;
                ui->plot->setInteraction(QCP::iRangeDrag, false); // Disable plot panning
            }
        }

        if (!used) {
            lMouseDownOnLegend = false;
        }
    }
}

void PlotWindow::onPlotDoubleClick(QMouseEvent* /*event*/)
{

}

void PlotWindow::onAxisDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part,
                                   QMouseEvent* /*event*/)
{
    if (part == QCPAxis::spAxisLabel) {
        bool ok;
        QString text = QInputDialog::getText(this, "Axis Label", "Label",
                                              QLineEdit::Normal,
                                              axis->label(),
                                              &ok);
        if (ok) {
            axis->setLabel(text);
            ui->plot->replot();
        }
    }
}

void PlotWindow::onPlotItemDoubleClick(QCPAbstractItem* item, QMouseEvent* /*event*/)
{
    foreach (MarkerPtr marker, mMarkers) {
        if (marker->textItem == item) {
            editMarkerText(marker);
            break;
        }
    }
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

/* Handles right mouse button drag on plot. Returns true if plot should be
 * updated (replot). */
bool PlotWindow::plotMouseRightDrag(QMouseEvent* event)
{
    if (!rMouseZoom.mouseDown) { return false; }

    QCPAxisRect* ar = ui->plot->axisRect();
    if (!ar) { return false; }
    QRectF axisRect(ar->rect());

    // Position on axis = mouse position minus axis rect offsets
    QPointF axisPos = event->pos() - axisRect.topLeft();
    // Mouse movement relative to start
    double px = axisPos.x() - rMouseZoom.start.x();
    double dx = px / axisRect.width();
    double py = axisPos.y() - rMouseZoom.start.y();
    double dy = py / axisRect.height();

    // Start dragging if we have moved further than threshold
    if (!rMouseZoom.isDragging) {
        if (QPointF(px, py).manhattanLength() > 5.0) {
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
bool PlotWindow::plotMouseMove(QMouseEvent* event)
{
    bool posValid = false;
    bool replot = false;

    lMouseDragging = rMouseZoom.isDragging || lMouseDown;

    QCPAxis* xaxis = ui->plot->xAxis;
    QCPAxis* yaxis = ui->plot->yAxis;

    if (xaxis && yaxis) {

        QRect r = ui->plot->axisRect()->rect();
        bool mouseInAxisRect = r.contains(event->x(), event->y());

        posValid = true;
        double mouseX = xaxis->pixelToCoord(event->x());
        double mouseY = yaxis->pixelToCoord(event->y());

        if (mCurrentMeasure) {
            QPointF delta = QPointF(mouseX, mouseY) -
                            mCurrentMeasure->a->dot->position->coords();
            mCurrentMeasure->b->dot->position->setCoords(mouseX, mouseY);
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

        // - Only update tracers and coordinates if not dragging as it could
        //   slow down dragging
        // - Don't draw crosshairs when mouse is outside of axis rect. This
        //   could lead to the crosshair being directly behind the axis labels,
        //   preventing click events to the labels.
        if (!lMouseDragging && mouseInAxisRect) {

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
                    text = QString("Plot: [%1] (%2, %3)")
                            .arg(closest.dataIndex)
                            .arg(closest.xCoord)
                            .arg(closest.yCoord);
                    emit dataTipChanged(closest.dataIndex
                                        + dataTipGraph->range.start);
                    replot = true;

                    mPlotCrosshairIndex = closest.dataIndex;
                }
            }

            // Update mouse crosshair
            if (mMouseCrosshair->visible()) {
                mMouseCrosshair->position->setCoords(mouseX, mouseY);
                replot = true;
            }

            // Update coordinates label
            if (!text.isEmpty()) { text += ", "; }
            text += QString("Mouse: (%4, %5)").arg(mouseX).arg(mouseY);

            ui->label_coordinates->setText(text);
        }
    }

    if (!posValid) {
        // Clear coordinates label
        ui->label_coordinates->clear();
    }

    return replot;
}

bool PlotWindow::legendMouseMove(QMouseEvent* event)
{
    bool replot = false;
    bool dragging = lMouseDownOnLegend;

    if (dragging) {

        QPoint delta = event->pos() - lMouseStart;

        QSize legendSize = ui->plot->legend->minimumOuterSizeHint();
        QRect axisRectPixels = ui->plot->axisRect()->rect();
        QRect legendRectPixels(
                    mLegendStartRect.x() * axisRectPixels.width(),
                    mLegendStartRect.y() * axisRectPixels.height(),
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
        ui->plot->axisRect()->insetLayout()->setInsetRect(0, legendRectNorm);

        replot = true;
    }

    return replot;
}

void PlotWindow::onLegendItemRightClicked(QCPPlottableLegendItem* legendItem,
                                          const QPoint &pos)
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

    menu->addAction(ui->action_legend_item_rename->icon(), "Rename", this, [=]()
    {
        bool ok;
        QString name = QInputDialog::getText(this, "Curve Name", "Name",
                                             QLineEdit::Normal,
                                             plottable->name(), &ok);
        if (!ok) { return; }
        plottable->setName(name);
        ui->plot->replot();
        updateLegendPlacement();
    });
    menu->addAction(ui->action_legend_item_set_color->icon(), "Set Color", this, [=]()
    {
        QPen pen = plottable->pen();
        QColor color = QColorDialog::getColor(pen.color(),
                                              this);
        if (!color.isValid()) { return; }
        pen.setColor(color);
        plottable->setPen(pen);
        ui->plot->replot();
        updateLegendPlacement();
    });
    menu->addAction(ui->action_legend_item_delete->icon(), "Delete", this, [=]()
    {
        removeGraph(plottableGraphMap.value(plottable));
        ui->plot->replot();
        updateLegendPlacement();
    });
    menu->popup(ui->plot->mapToGlobal(pos));
}

void PlotWindow::onLegendRightClicked(QCPLegend* /*legend*/, const QPoint& /*pos*/)
{
    qDebug() << "Legend right-clicked";
}

void PlotWindow::onPlotMouseRelease(QMouseEvent* event)
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
        if (!lMouseDragging) {
            // Was not dragging. Trigger mouse right click event
            plotLeftClicked(event->pos());
        }
        lMouseDownOnLegend = false;
        lMouseDownOnMarker = false;
        markerMouseUp();
        ui->plot->setInteraction(QCP::iRangeDrag, true); // Re-enable plot panning
    }
}

void PlotWindow::setupCrosshairs()
{
    mPlotCrosshair = new PlotMarkerItem(ui->plot);
    mPlotCrosshair->setLayer("crosshairs");
    mPlotCrosshair->size = 7;
    mPlotCrosshair->verticalLine = true;

    mMouseCrosshair = new PlotMarkerItem(ui->plot);
    mMouseCrosshair->setLayer("crosshairs");
    mMouseCrosshair->size = 7;
    mMouseCrosshair->verticalLine = true;
    mMouseCrosshair->horizontalLine = true;
    mMouseCrosshair->setVisible(false);

    updateGuiForCrosshairOptions();
}

void PlotWindow::updateGuiForCrosshairOptions()
{
    ui->action_Show_Plot_Crosshair->setChecked(mPlotCrosshair->visible());
    ui->action_PlotCrosshair_Vertical_Line->setChecked(mPlotCrosshair->verticalLine);
    ui->action_PlotCrosshair_Horizontal_Line->setChecked(mPlotCrosshair->horizontalLine);
    ui->action_PlotCrosshair_Dot->setChecked(mPlotCrosshair->showCircle);

    ui->action_Show_Mouse_Crosshair->setChecked(mMouseCrosshair->visible());
    ui->action_MouseCrosshair_Vertical_Line->setChecked(mMouseCrosshair->verticalLine);
    ui->action_MouseCrosshair_Horizontal_Line->setChecked(mMouseCrosshair->horizontalLine);
    ui->action_MouseCrosshair_Dot->setChecked(mMouseCrosshair->showCircle);

    queueReplot();
}

PlotWindow::ClosestCoord PlotWindow::findClosestCoord(QPoint pos, GraphPtr graph,
                                                      CrosshairSnap snap)
{
    ClosestCoord closest;

    QCPAxis* xaxis = ui->plot->xAxis;
    QCPAxis* yaxis = ui->plot->yAxis;

    if (!graph || !xaxis || !yaxis) {
        closest.valid = false;
        return closest;
    }

    for (int i = 0; i < graph->dataCount(); ++i) {

        double xCrd = graph->datax(i);
        int xPixel = xaxis->coordToPixel(xCrd);
        double yCrd = graph->datay(i);

        int dist = 0;

        if (snap == SnapXOnly) {
            dist = qAbs(pos.x() - xPixel);
        } else if (snap == SnapToClosest) {
            int yPixel = yaxis->coordToPixel(yCrd);
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

void PlotWindow::storeAndDisableCrosshairs()
{
    lastPlotCrosshairVisible = mPlotCrosshair->visible();
    mPlotCrosshair->setVisible(false);
    lastMouseCrosshairVisible = mMouseCrosshair->visible();
    mMouseCrosshair->setVisible(false);
}

void PlotWindow::restoreCrosshairs()
{
    mPlotCrosshair->setVisible(lastPlotCrosshairVisible);
    mMouseCrosshair->setVisible(lastMouseCrosshairVisible);
}

void PlotWindow::setLinkGroup(int group)
{
    mLinkGroup = group;
    emit linkGroupChanged(group);
    updateGuiForLinkGroup();
}

void PlotWindow::updateGuiForLinkGroup()
{
    ui->action_No_Link->setChecked(mLinkGroup == 0);
    ui->action_Link_to_Group_1->setChecked(mLinkGroup == 1);
    ui->action_Link_to_Group_2->setChecked(mLinkGroup == 2);
    ui->action_Link_to_Group_3->setChecked(mLinkGroup == 3);

    if (mLinkGroup == 0) {
        ui->menuLink->setTitle("Link");
    } else {
        ui->menuLink->setTitle(QString("Link (%1)").arg(mLinkGroup));
    }
}

void PlotWindow::plotRightClicked(const QPoint &pos)
{
    bool used = false;

    QCPLegend* legend = ui->plot->legend;
    if (legend) {

        // Check if plottable item in legend has been right clicked
        for (int i = 0; i < legend->itemCount(); i++) {
            QCPAbstractLegendItem* legendItem = legend->item(i);
            if (legendItem->selectTest(pos, false) >= 0) {
                QCPPlottableLegendItem* plItem = qobject_cast<QCPPlottableLegendItem*>(legendItem);
                if (plItem) {
                    onLegendItemRightClicked(plItem, pos);
                    used = true;
                }
            }
        }

        if (!used) {
            // Check if legend has been right clicked
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
        plotContextMenu.popup(ui->plot->mapToGlobal(pos));
    }
}

void PlotWindow::plotLeftClicked(const QPoint& /*pos*/)
{
    if (mCurrentMeasure) {
        // Stop measuring
        clearCurrentMeasure();
    }
}

void PlotWindow::queueReplot()
{
    ui->plot->replot(QCustomPlot::rpQueuedReplot);
}

void PlotWindow::updatePlotForEqualAxes(QRectF xyrange)
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

QByteArray PlotWindow::plotToSvg()
{
    QByteArray svgData;
    QBuffer buffer(&svgData);
    buffer.open(QIODevice::WriteOnly);

    ui->plot->saveSvg(&buffer);

    buffer.close();

    return svgData;
}

void PlotWindow::setupMenus()
{
    // Range menu

    rangeMenu.setTitle("Range");
    connect(&rangeMenu, &QMenu::aboutToShow, this, &PlotWindow::onRangeMenuAboutToShow);

    // View menu

    ui->menuView->insertMenu(ui->action_datatipPlotMenuPlaceholder, &dataTipMenu);
    ui->menuView->removeAction(ui->action_datatipPlotMenuPlaceholder);

    // Plot context menu

    plotContextMenu.addActions({
        ui->action_Place_Marker,
        ui->action_Measure,
        ui->action_Show_All,
        ui->action_Equal_Axes
    });
    plotContextMenu.addMenu(&dataTipMenu);
    plotContextMenu.addMenu(&rangeMenu);

    // Data tip menu
    dataTipMenu.setTitle("Datatip Plot");
    connect(&dataTipMenu, &QMenu::aboutToShow, this, &PlotWindow::onDataTipMenuAboutToShow);
}

void PlotWindow::onRangeMenuAboutToShow()
{
    rangeMenu.clear();

    if (!dataTipGraph) {
        rangeMenu.addAction("No datatip");
    } else {

        QMenu* newRangeMenu = rangeMenu.addMenu("New Range");

        newRangeMenu->addAction("Set start of new range", this, [=]()
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
        newRangeMenu->addAction("Set end of new range", this, [=]()
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

void PlotWindow::onDataTipMenuAboutToShow()
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

void PlotWindow::updateLegendPlacement()
{
    if (!this->isVisible()) { return; }

    QRect axisRectPixels = ui->plot->axisRect()->rect();
    QSize legendSize = ui->plot->legend->minimumOuterSizeHint();

    QRectF rect = ui->plot->axisRect()->insetLayout()->insetRect(0);
    double x = rect.x() * axisRectPixels.width();
    double y = rect.y() * axisRectPixels.height();
    double w = double(legendSize.width());
    double h = double(legendSize.height());

    if (mFirstLegendPlacement) {
        mFirstLegendPlacement = false;
        // Set free layout so legend can be moved freely
        ui->plot->axisRect()->insetLayout()->setInsetPlacement(0, QCPLayoutInset::ipFree);
        // Zero margins so legend can be moved to the edges
        ui->plot->axisRect()->insetLayout()->setMargins(QMargins(0, 0, 0, 0));
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
    ui->plot->axisRect()->insetLayout()->setInsetRect(0, rect);
    ui->plot->replot();
}

void PlotWindow::removeGraph(GraphPtr graph)
{
    if (!graph) { return; }

    ui->plot->removePlottable(graph->plottable());
    graphs.removeAll(graph);
    plottableGraphMap.remove(graph->plottable());

    if (dataTipGraph == graph) {
        dataTipGraph = graphs.value(0);
    }
}

void PlotWindow::onAxisRangesChanged()
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

void PlotWindow::on_action_Equal_Axes_triggered()
{
    setEqualAxesAndReplot(ui->action_Equal_Axes->isChecked());
}

void PlotWindow::on_action_Show_All_triggered()
{
    showAll();
}

void PlotWindow::on_action_No_Link_triggered()
{
    setLinkGroup(0);
}

void PlotWindow::on_action_Link_to_Group_1_triggered()
{
    setLinkGroup(1);
}

void PlotWindow::on_action_Link_to_Group_2_triggered()
{
    setLinkGroup(2);
}

void PlotWindow::on_action_Link_to_Group_3_triggered()
{
    setLinkGroup(3);
}

void PlotWindow::on_action_Show_Plot_Crosshair_triggered()
{
    mPlotCrosshairVisibilityChangedByUser = true;
    mPlotCrosshair->setVisible(ui->action_Show_Plot_Crosshair->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWindow::on_action_Show_Mouse_Crosshair_triggered()
{
    mMouseCrosshair->setVisible(ui->action_Show_Mouse_Crosshair->isChecked());
    updateGuiForCrosshairOptions();
}

void PlotWindow::on_action_PlotCrosshair_Horizontal_Line_triggered()
{
    mPlotCrosshair->horizontalLine = ui->action_PlotCrosshair_Horizontal_Line->isChecked();
    updateGuiForCrosshairOptions();
}

void PlotWindow::on_action_PlotCrosshair_Vertical_Line_triggered()
{
    mPlotCrosshair->verticalLine = ui->action_PlotCrosshair_Vertical_Line->isChecked();
    updateGuiForCrosshairOptions();
}

void PlotWindow::on_action_PlotCrosshair_Dot_triggered()
{
    mPlotCrosshair->showCircle = ui->action_PlotCrosshair_Dot->isChecked();
    updateGuiForCrosshairOptions();
}

void PlotWindow::on_action_MouseCrosshair_Horizontal_Line_triggered()
{
    mMouseCrosshair->horizontalLine = ui->action_MouseCrosshair_Horizontal_Line->isChecked();
    updateGuiForCrosshairOptions();
}

void PlotWindow::on_action_MouseCrosshair_Vertical_Line_triggered()
{
    mMouseCrosshair->verticalLine = ui->action_MouseCrosshair_Vertical_Line->isChecked();
    updateGuiForCrosshairOptions();
}

void PlotWindow::on_action_MouseCrosshair_Dot_triggered()
{
    mMouseCrosshair->showCircle = ui->action_MouseCrosshair_Dot->isChecked();
    updateGuiForCrosshairOptions();
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


QCPAbstractPlottable* PlotWindow::Graph::plottable()
{
    QCPAbstractPlottable* ret = nullptr;
    if (curve) {
        ret = static_cast<QCPAbstractPlottable*>(curve);
    } else if (graph) {
        ret = static_cast<QCPAbstractPlottable*>(graph);
    }
    return ret;
}

bool PlotWindow::Graph::isCurve()
{
    return (curve != nullptr);
}

bool PlotWindow::Graph::isGraph()
{
    return (graph != nullptr);
}

QString PlotWindow::Graph::name()
{
    if (curve) {
        return curve->name();
    } else if (graph) {
        return graph->name();
    } else {
        return "";
    }
}

int PlotWindow::Graph::dataCount()
{
    if (curve) {
        return curve->dataCount();
    } else if (graph) {
        return graph->dataCount();
    } else {
        return 0;
    }
}

double PlotWindow::Graph::datax(int index)
{
    if (curve) {
        return curve->data()->at(index)->key;
    } else if (graph) {
        return graph->data()->at(index)->key;
    } else {
        return 0;
    }
}

double PlotWindow::Graph::datay(int index)
{
    if (curve) {
        return curve->data()->at(index)->value;
    } else if (graph) {
        return graph->data()->at(index)->value;
    } else {
        return 0;
    }
}

QColor PlotWindow::Graph::color()
{
    if (curve) {
        return curve->pen().color();
    } else if (graph) {
        return graph->pen().color();
    } else {
        return QColor();
    }
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

void PlotWindow::on_action_Place_Marker_triggered()
{
    GraphPtr graph = dataTipGraph;
    if (!graph) { graph = graphs.value(0); }
    if (!graph) { return; }

    QPoint pos = mLastMouseMovePos;
    ClosestCoord closest = findClosestCoord(pos, graph, mPlotCrosshairSnap);
    if (!closest.valid) { return; }

    MarkerPtr marker = addMarker(QPointF(closest.xCoord, closest.yCoord));

    marker->datasetName = graph->name();
    marker->dataIndex = closest.dataIndex;
    marker->text = "Index: $i\n$x, $y";
    updateMarkerText(marker);
    updateMarkerArrow(marker);

    ui->plot->replot();
}

void PlotWindow::on_action_Measure_triggered()
{
    if (!mCurrentMeasure) {
        // Not busy with a measure. Start a new one.
        ui->action_Measure->setText("End Measure");
    } else {
        // Busy with a measure. End it here.
        clearCurrentMeasure();
        return;
    }

    QCPAxis* xaxis = ui->plot->xAxis;
    QCPAxis* yaxis = ui->plot->yAxis;

    QPoint pos = mLastMouseMovePos;
    double x = xaxis->pixelToCoord(pos.x());
    double y = yaxis->pixelToCoord(pos.y());

    MeasurePtr m(new Measure());
    m->tag = QString("Measure %1").arg(mMeasureCounter++);

    QList<MarkerPtr> ab;
    for (int i = 0; i < 2; i++) {

        MarkerPtr a = addMarker(QPointF(x, y));
        a->text = QString("%1 A\n$x, $y").arg(m->tag);
        updateMarkerText(a);
        updateMarkerArrow(a);
        a->dot->showCircle = false;
        a->dot->verticalLine = true;
        a->dot->horizontalLine = true;
        a->dot->pen.setStyle(Qt::DashLine);

        ab.append(a);
    }

    m->a = ab.value(0);
    m->b = ab.value(1);

    mMeasures.append(m);
    mCurrentMeasure = m;

    ui->plot->replot();
}

void PlotWindow::on_action_Save_as_PDF_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "Save as PDF",
                                                "",
                                                "PDF (*.pdf)");
    if (path.isEmpty()) { return; }

    storeAndDisableCrosshairs();
    bool ok = ui->plot->savePdf(path,0, 0, QCP::epNoCosmetic);
    if (!ok) {
        QMessageBox::critical(this, "Save to PDF failed",
                              "Failed to save to PDF: " + path);
    }
    restoreCrosshairs();
}

void PlotWindow::on_action_Copy_PNG_triggered()
{
    storeAndDisableCrosshairs();
    QImage image = ui->plot->toPixmap(0, 0, 2.0).toImage();
    restoreCrosshairs();

    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setImage(image);
}

void PlotWindow::on_action_Copy_SVG_triggered()
{
    storeAndDisableCrosshairs();
    QByteArray svgData = plotToSvg();
    restoreCrosshairs();

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

    storeAndDisableCrosshairs();
    bool ok = ui->plot->savePng(path, 0, 0, 2.0);
    if (!ok) {
        QMessageBox::critical(this, "Save to PNG failed",
                              "Failed to save to PNG: " + path);
    }
    restoreCrosshairs();
}

void PlotWindow::on_action_Save_as_SVG_triggered()
{
    QString path = QFileDialog::getSaveFileName(this, "Save as SVG",
                                                "",
                                                "SVG Vector Image (*.svg)");
    if (path.isEmpty()) { return; }

    storeAndDisableCrosshairs();
    QByteArray svgData = plotToSvg();
    restoreCrosshairs();

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(svgData);
        file.close();
    } else {
        QMessageBox::critical(this, "Save to SVG failed",
                              "Failed to save to SVG: " + path);
    }
}
