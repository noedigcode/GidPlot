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
