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

#ifndef GRAPH_H
#define GRAPH_H

#include "csv.h"
#include "mapline.h"

#include "QCustomPlot/qcustomplot.h"

#include <QObject>
#include <QSharedPointer>

#include <functional>


/* Graph provides a unified interface for either a QCPGraph, QCPCurve, or
 * a map Track. QCPGraph and QCPCurve are plotted in subplots, using QCustomPlot,
 * depending on whether it is monotonically increasing (QCPGraph) or not
 * (QCPCurve). A Track is plotted on a QGeoView map. */

// ===========================================================================

enum ClosestOption { ClosestXOnly, ClosestXY };

// ===========================================================================

struct Track
{
    QVector<double> lats;
    QVector<double> lons;

    QString name;
    QPen pen;

    QList<MapLine*> mapLines;

};
typedef QSharedPointer<Track> TrackPtr;

// ===========================================================================

struct GridHash
{
    QRectF bounds;
    int n = 100;

    int maxcolrow();

    struct Data
    {
        QPointF coord;
        int index = 0;
    };

    QMultiMap<int, Data> map;

    int hash(QPoint point);
    QPoint colrow(QPointF coord);
    int clipcolrow(int value);
    void insert(QPointF coord, int index);

    struct Result
    {
        QList<Data> data;
        bool maxedOut = false;
    };

    Result get(QRectF rect, ClosestOption option);
};

// ===========================================================================

class Graph
{
public:
    Graph(QCPGraph* graph) : graph(graph) {}
    Graph(QCPCurve* curve) : curve(curve) {}
    Graph(TrackPtr track) : track(track) {}

    QCPCurve* curve = nullptr;
    QCPGraph* graph = nullptr;
    QCPAbstractPlottable* plottable();

    TrackPtr track;

    CsvPtr csv;
    Range range;
    int ixcol = 0;
    int iycol = 0;

    Matrix::VStats xstats;
    Matrix::VStats ystats;
    QRectF dataBounds();

    GridHash grid;

    bool isCurve();
    bool isGraph();
    bool isTrack();
    QString name();
    int dataCount();
    double datax(int index);
    double datay(int index);
    QColor color();

    static QPen nextPen(int index);
};

typedef QSharedPointer<Graph> GraphPtr;

// ===========================================================================

class PlotMenu : public QObject
{
    Q_OBJECT
public:
    PlotMenu(QObject* parent = nullptr);

    QWidget* parentWidget = nullptr;
    std::function<GraphPtr()> getDataTipGraphCallback;
    std::function<QList<GraphPtr>()> getGraphsCallback;
    std::function<int()> getPlotCrosshairIndexCallback;
    void setMeasureActionStarted();
    void setMeasureActionEnded();

    QMenu menu;

    QAction* actionCrosshairs;
    QAction* actionEqualAxes;
    QAction* actionLink;
    QAction* actionMeasure;
    QAction* actionPlaceMarker;
    QAction* actionShowAll;
    QMenu dataTipMenu;
    QMenu rangeMenu;

    QList<QAction*> actions();

signals:
    void dataTipGraphSelected(GraphPtr graph);

private:
    GraphPtr datatipGraph();
    QList<GraphPtr> allGraphs();
    int plotCrosshairIndex();

private slots:
    void onDataTipMenuAboutToShow();
    void onRangeMenuAboutToShow();
};


#endif // GRAPH_H
