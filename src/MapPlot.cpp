#include "MapPlot.h"

#include <QLayout>
#include <QtMath>

// Static members
QNetworkAccessManager MapPlot::netAccMgr;
QNetworkDiskCache MapPlot::netCache;


MapPlot::MapPlot(QGVMap *mapWidget, QObject *parent)
    : QObject{parent}, mMapWidget(mapWidget)
{
    // Set up network
    if (!QGV::getNetworkManager()) {
        netCache.setCacheDirectory("cacheDir");
        netAccMgr.setCache(&netCache);
        QGV::setNetworkManager(&netAccMgr);
    }

    // Set up map widget
    connect(mMapWidget, &QGVMap::mapMouseMove, this, &MapPlot::onMapMouseMove);
    mMapWidget->setMouseTracking(true);
    mMapWidget->layout()->setMargin(0);
    setMapOSM();

    setupCrosshairs();
    setupLink();
    setupMenus();
}

void MapPlot::setupLink()
{
    link->supportPosZoom = false;
    link->tag = "Map Plot";
}

QList<GraphPtr> MapPlot::getAllGraphs()
{
    return mTracks;
}

GraphPtr MapPlot::getDataTipGraph()
{
    return dataTipTrack;
}

void MapPlot::showCrosshairsDialog()
{
    qDebug() << "TODO showCrosshairsDialog()"; // TODO
}

void MapPlot::setupMenus()
{
    plotMenu.parentWidget = nullptr; qDebug() << "TODO MapPlot set plotMenu.parentWidget"; // TODO
    plotMenu.getDataTipGraphCallback = [=]() { return getDataTipGraph(); };
    plotMenu.getGraphsCallback = [=]() { return getAllGraphs(); };
    plotMenu.getPlotCrosshairIndexCallback = [=]()
    {
        return mTrackCrosshairIndex;
    };

    connect(plotMenu.actionPlaceMarker, &QAction::triggered,
            this, &MapPlot::onActionPlaceMarkerTriggered);
    connect(plotMenu.actionMeasure, &QAction::triggered,
            this, &MapPlot::onActionMeasureTriggered);
    connect(plotMenu.actionShowAll, &QAction::triggered,
            this, &MapPlot::showAll);
    plotMenu.actionEqualAxes->setVisible(false);
    connect(plotMenu.actionCrosshairs, &QAction::triggered,
            this, &MapPlot::showCrosshairsDialog);
    connect(plotMenu.actionLink, &QAction::triggered,
            this, &MapPlot::linkSettingsTriggered);

    mMapWidget->addActions(plotMenu.actions());
}

void MapPlot::onActionPlaceMarkerTriggered()
{
    qDebug() << "TODO onActionPlaceMarkerTriggered()"; // TODO
}

void MapPlot::onActionMeasureTriggered()
{
    qDebug() << "TODO onActionMeasureTriggered()"; // TODO
}

void MapPlot::plot(CsvPtr csv, int iloncol, int ilatcol, Range range)
{
    bool firstPlot = mTracks.isEmpty();

    TrackPtr track(new Track());
    GraphPtr graph(new Graph(track));
    graph->csv = csv;
    graph->iycol = ilatcol;
    graph->ixcol = iloncol;
    graph->range = range;
    track->lats = csv->matrix->data[ilatcol].mid(range.start, range.size());
    track->lons = csv->matrix->data[iloncol].mid(range.start, range.size());
    track->name = QString("%1, %2")
            .arg(csv->matrix->heading(ilatcol))
            .arg(csv->matrix->heading(iloncol));

    graph->ystats = Matrix::vstats(track->lats);
    graph->xstats = Matrix::vstats(track->lons);

    // Combine track mins/maxes with overall of all tracks
    if (firstPlot) {
        latmin = graph->ystats.min;
        latmax = graph->ystats.max;
        lonmin = graph->xstats.min;
        lonmax = graph->xstats.max;
    } else {
        latmin = qMin(latmin, graph->ystats.min);
        latmax = qMax(latmax, graph->ystats.max);
        lonmin = qMin(lonmin, graph->xstats.min);
        lonmax = qMax(lonmax, graph->xstats.max);
    }

    // Create track on map
    track->pen = Graph::nextPen(mPenIndex++);
    for (int i = 0; i < track->lats.count() - 1; i++) {
        QGV::GeoPos pos1(track->lats[i], track->lons[i]);
        QGV::GeoPos pos2(track->lats[i+1], track->lons[i+1]);
        MapLine* line = new MapLine(pos1, pos2);
        line->setColor(track->pen.color());
        track->mapLines.append(line);
        mMapWidget->addItem(line);
    }

    mTracks.append(graph);
    if (!dataTipTrack) {
        dataTipTrack = graph;
        mTrackCrosshair->setVisible(true);
    }

    showAll();
}

void MapPlot::syncDataTip(int index)
{
    if (!mTrackCrosshair->isVisible()) { return; }
    if (!dataTipTrack) { return; }

    index -= dataTipTrack->range.start;
    if ((index < 0) || (index >= dataTipTrack->dataCount())) { return; }
    double lat = dataTipTrack->track->lats[index];
    double lon = dataTipTrack->track->lons[index];
    mTrackCrosshair->setPosition(QGV::GeoPos(lat, lon));
}

void MapPlot::setMapOSM()
{
    setMapTiles(new QGVLayerOSM());
}

void MapPlot::zoomTo(double lat1, double lon1, double lat2, double lon2)
{
    QMetaObject::invokeMethod(this, [=]()
    {
        QGV::GeoRect target(lat1, lon1, lat2, lon2);
        mMapWidget->cameraTo(QGVCameraActions(mMapWidget).scaleTo(target));
    }, Qt::QueuedConnection);
}

void MapPlot::showAll()
{
    zoomTo(latmin, lonmin, latmax, lonmax);
}

void MapPlot::setupCrosshairs()
{
    mTrackCrosshair = new MapMarker(QGV::GeoPos(), 5);
    mTrackCrosshair->setBrush(QBrush(QColor(255, 0, 0, 100)));
    mTrackCrosshair->setVisible(false);
    mTrackCrosshair->bringToFront();
    mMapWidget->addItem(mTrackCrosshair);
}

MapPlot::ClosestCoord MapPlot::findClosestCoord(QGV::GeoPos pos, GraphPtr graph)
{
    ClosestCoord closest;

    if (!graph) {
        closest.valid = false;
        return closest;
    }

    for (int i = 0; i < graph->dataCount(); i++) {

        double lat = graph->track->lats[i];
        double lon = graph->track->lons[i];
        double dist = qSqrt( qPow(pos.latitude() - lat, 2)
                             + qPow(pos.longitude() - lon, 2) );

        if ((i == 0) || (dist < closest.distance)) {
            closest.valid = true;
            closest.distance = dist;
            closest.lat = lat;
            closest.lon = lon;
            closest.dataIndex = i;
            if (dist == 0) {
                break;
            }
        }

    }

    return closest;
}

void MapPlot::setMapTiles(QGVLayerTiles *tiles)
{
    removeTiles();
    mTilesItem = tiles;
    mMapWidget->addItem(mTilesItem);
}

void MapPlot::removeTiles()
{
    if (mTilesItem == nullptr) { return; }

    mMapWidget->removeItem(mTilesItem);
    delete(mTilesItem);
}

void MapPlot::onMapMouseMove(QPointF projPos)
{
    QGV::GeoPos pos = mMapWidget->getProjection()->projToGeo(projPos);
    if (mTrackCrosshair->isVisible() && dataTipTrack) {
        ClosestCoord closest = findClosestCoord(pos, dataTipTrack);
        if (closest.valid) {
            QGV::GeoPos closestPos(closest.lat, closest.lon);
            mTrackCrosshair->setPosition(closestPos);
            emit dataTipChanged(link->group,
                                closest.dataIndex + dataTipTrack->range.start);
            mTrackCrosshairIndex = closest.dataIndex;
        }
    }
}
