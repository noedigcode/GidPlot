/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2026 Gideon van der Kolf
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

#include "graph.h"


QCPAbstractPlottable *Graph::plottable()
{
    QCPAbstractPlottable* ret = nullptr;
    if (curve) {
        ret = static_cast<QCPAbstractPlottable*>(curve);
    } else if (graph) {
        ret = static_cast<QCPAbstractPlottable*>(graph);
    }
    return ret;
}

QRectF Graph::dataBounds()
{
    return QRectF(QPointF(xstats.min, ystats.min),
                  QPointF(xstats.max, ystats.max));
}

bool Graph::isCurve()
{
    return (curve != nullptr);
}

bool Graph::isGraph()
{
    return (graph != nullptr);
}

bool Graph::isTrack()
{
    return (!track.isNull());
}

QString Graph::name()
{
    if (curve) {
        return curve->name();
    } else if (graph) {
        return graph->name();
    } else if (track) {
        return track->name;
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
    } else if (track) {
        return track->lats.count();
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
    } else if (track) {
        return track->lons.at(index);
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
    } else if (track) {
        return track->lats.at(index);
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
    } else if (track) {
        return track->pen.color();
    } else {
        return QColor();
    }
}

QPen Graph::nextPen(int index)
{
    static QList<QPen> pens {
        QPen(Qt::blue),
        QPen(Qt::red),
        QPen(Qt::green),
        QPen(Qt::cyan),
        QPen(Qt::magenta),
        QPen(Qt::darkRed),
    };

    return pens.value(index % pens.count());
}

PlotMenu::PlotMenu(QObject *parent) : QObject(parent)
{
    actionPlaceMarker = menu.addAction(QIcon("://marker"), "Place Marker");
    actionMeasure = menu.addAction(QIcon("://measure"), "Measure");
    actionShowAll = menu.addAction(QIcon("://showall"), "Show All");
    actionEqualAxes = menu.addAction(QIcon("://equalaxes"), "Equal Axes");
    actionEqualAxes->setCheckable(true);

    // Data tip menu
    dataTipMenu.setTitle("Datatip Plot");
    connect(&dataTipMenu, &QMenu::aboutToShow, this, &PlotMenu::onDataTipMenuAboutToShow);
    menu.addMenu(&dataTipMenu);

    // Range menu
    rangeMenu.setTitle("Range");
    connect(&rangeMenu, &QMenu::aboutToShow, this, &PlotMenu::onRangeMenuAboutToShow);
    menu.addMenu(&rangeMenu);

    actionCrosshairs = menu.addAction(QIcon("://crosshair"), "Crosshairs...");
    actionLink = menu.addAction(QIcon("://link"), "Link to Other Plots...");

}

void PlotMenu::setMeasureActionStarted()
{
    actionMeasure->setText("End Measure");
}

void PlotMenu::setMeasureActionEnded()
{
    actionMeasure->setText("Measure");
}

QList<QAction *> PlotMenu::actions()
{
    return menu.actions();
}

GraphPtr PlotMenu::datatipGraph()
{
    GraphPtr ret;
    if (getDataTipGraphCallback) {
        ret = getDataTipGraphCallback();
    }
    return ret;
}

QList<GraphPtr> PlotMenu::allGraphs()
{
    QList<GraphPtr> ret;
    if (getGraphsCallback) {
        ret = getGraphsCallback();
    }
    return ret;
}

int PlotMenu::plotCrosshairIndex()
{
    int ret = 0;
    if (getPlotCrosshairIndexCallback) {
        ret = getPlotCrosshairIndexCallback();
    }
    return ret;
}

void PlotMenu::onDataTipMenuAboutToShow()
{
    dataTipMenu.clear();

    QList<GraphPtr> graphs = allGraphs();

    foreach (GraphPtr g, graphs) {
        QAction* action = dataTipMenu.addAction(g->name(), this,
                                                [this, gwk = g.toWeakRef()]()
        {
            GraphPtr g(gwk);
            if (!g) { return; }
            emit dataTipGraphSelected(g);
        });

        QPixmap pixmap(16, 16);
        pixmap.fill(g->color());
        QIcon icon(pixmap);
        action->setIcon(icon);

        action->setCheckable(true);

        if (datatipGraph() == g) {
            action->setChecked(true);
        }
    }
}

void PlotMenu::onRangeMenuAboutToShow()
{
    rangeMenu.clear();

    GraphPtr dataTipGraph = datatipGraph();
    if (!dataTipGraph) {
        rangeMenu.addAction("No datatip");
    } else {

        QMenu* newRangeMenu = rangeMenu.addMenu("New Range");

        newRangeMenu->addAction("Set start of new range", this, [=]()
        {
            QString name = QInputDialog::getText(
                        parentWidget, "New Range", "Name",
                        QLineEdit::Normal,
                        QString("Range %1")
                        .arg(dataTipGraph->csv->ranges().count() + 1));
            if (name.isEmpty()) { return; }
            RangePtr range(new Range());
            range->name = name;
            // Take start of graph range into account
            range->start = plotCrosshairIndex() + dataTipGraph->range.start;
            range->end = dataTipGraph->range.end + dataTipGraph->range.start;
            dataTipGraph->csv->addRange(range);
        });
        newRangeMenu->addAction("Set end of new range", this, [=]()
        {
            QString name = QInputDialog::getText(
                        parentWidget, "New Range", "Name",
                        QLineEdit::Normal,
                        QString("Range %1")
                        .arg(dataTipGraph->csv->ranges().count() + 1));
            if (name.isEmpty()) { return; }
            RangePtr range(new Range());
            range->name = name;
            // Take start of graph range into account
            range->start = 0 + dataTipGraph->range.start;
            range->end = plotCrosshairIndex() + dataTipGraph->range.start;
            dataTipGraph->csv->addRange(range);
        });

        foreach (RangePtr range, dataTipGraph->csv->ranges()) {
            QMenu* rangeXMenu = rangeMenu.addMenu(range->name);
            RangeWeakPtr rangeWkPtr(range);
            rangeXMenu->addAction("Set start", this, [this, rangeWkPtr]()
            {
                RangePtr range(rangeWkPtr);
                if (!range) { return; }
                // Take start of graph range into account
                range->start = plotCrosshairIndex() + datatipGraph()->range.start;
            });
            rangeXMenu->addAction("Set end", this, [this, rangeWkPtr]()
            {
                RangePtr range(rangeWkPtr);
                if (!range) { return; }
                // Take start of graph range into account
                range->end = plotCrosshairIndex() + datatipGraph()->range.start;
            });
        }
    }
}

int GridHash::maxcolrow()
{
    return n - 1;
}

int GridHash::hash(QPoint point)
{
    return (point.x() + 1) * n + point.y();
}

QPoint GridHash::colrow(QPointF coord)
{
    double max = qMax(bounds.width(), bounds.height());

    int col = (coord.x() - bounds.left()) / max * n;
    int row = (coord.y() - bounds.top()) / max * n;

    return QPoint(col, row);
}

int GridHash::clipcolrow(int value)
{
    return (qMax(qMin(value, maxcolrow()), 0));
}

void GridHash::insert(QPointF coord, int index)
{
    QPoint point = colrow(coord);

    point.setX(clipcolrow(point.x()));
    point.setY(clipcolrow(point.y()));

    map.insert(hash(point), Data{coord, index});
}

GridHash::Result GridHash::get(QRectF rect, ClosestOption option)
{
    Result ret;

    QPoint start = colrow(rect.topLeft());
    QPoint end = colrow(rect.bottomRight());

    int colstart = clipcolrow(start.x());
    int colend = clipcolrow(end.x());
    int rowstart;
    int rowend;
    if (option == ClosestXOnly) {
        rowstart = 0;
        rowend = maxcolrow();
    } else {
        rowstart = clipcolrow(start.y());
        rowend = clipcolrow(end.y());
    }

    for (int col = colstart; col <= colend; col++) {
        for (int row = rowstart; row <= rowend; row++) {
            QList<Data> vals = map.values(hash(QPoint(col, row)));
            ret.data.append(vals);
        }
    }

    bool atmax = true;
    atmax &= (colstart == 0);
    atmax &= (colend == maxcolrow());
    atmax &= (rowstart == 0);
    atmax &= (rowend == maxcolrow());
    ret.maxedOut = atmax;

    return ret;
}
