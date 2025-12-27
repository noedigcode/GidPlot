#ifndef GRAPH_H
#define GRAPH_H

#include "csv.h"
#include "mapline.h"

#include "QCustomPlot/qcustomplot.h"

#include <QSharedPointer>


/* Graph provides a unified interface for either a QCPGraph, QCPCurve, or
 * a map Track. QCPGraph and QCPCurve are plotted in subplots, using QCustomPlot,
 * depending on whether it is monotonically increasing (QCPGraph) or not
 * (QCPCurve). A Track is plotted on a QGeoView map. */

struct Track
{
    QVector<double> lats;
    QVector<double> lons;

    QString name; // TODO set
    QPen pen; // TODO set and use

    QList<MapLine*> mapLines;

};
typedef QSharedPointer<Track> TrackPtr;

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

#endif // GRAPH_H
