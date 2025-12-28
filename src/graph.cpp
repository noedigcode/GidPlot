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
                        .arg(dataTipGraph->csv->ranges.count() + 1));
            if (name.isEmpty()) { return; }
            RangePtr range(new Range());
            range->name = name;
            // Take start of graph range into account
            range->start = plotCrosshairIndex() + dataTipGraph->range.start;
            range->end = dataTipGraph->range.end + dataTipGraph->range.start;
            dataTipGraph->csv->ranges.append(range);
        });
        newRangeMenu->addAction("Set end of new range", this, [=]()
        {
            QString name = QInputDialog::getText(
                        parentWidget, "New Range", "Name",
                        QLineEdit::Normal,
                        QString("Range %1")
                        .arg(dataTipGraph->csv->ranges.count() + 1));
            if (name.isEmpty()) { return; }
            RangePtr range(new Range());
            range->name = name;
            // Take start of graph range into account
            range->start = 0 + dataTipGraph->range.start;
            range->end = plotCrosshairIndex() + dataTipGraph->range.start;
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
