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
}

void MapPlot::setupLink()
{
    link->supportPosZoom = false;
}

void MapPlot::plot(CsvPtr csv, int iloncol, int ilatcol, Range range)
{
    bool firstPlot = mTracks.isEmpty();

    TrackPtr track(new Track());
    track->csv = csv;
    track->ilatcol = ilatcol;
    track->iloncol = iloncol;
    track->range = range;
    track->lats = csv->matrix->data[ilatcol].mid(range.start, range.size());
    track->lons = csv->matrix->data[iloncol].mid(range.start, range.size());

    track->latStats = Matrix::vstats(track->lats);
    track->lonStats = Matrix::vstats(track->lons);

    // Combine track mins/maxes with overall of all tracks
    if (firstPlot) {
        latmin = track->latStats.min;
        latmax = track->latStats.max;
        lonmin = track->lonStats.min;
        lonmax = track->lonStats.max;
    } else {
        latmin = qMin(latmin, track->latStats.min);
        latmax = qMax(latmax, track->latStats.max);
        lonmin = qMin(lonmin, track->lonStats.min);
        lonmax = qMax(lonmax, track->lonStats.max);
    }

    // Create track on map
    for (int i = range.start; i < range.end - 1; i++) {
        QGV::GeoPos pos1(track->lats[i], track->lons[i]);
        QGV::GeoPos pos2(track->lats[i+1], track->lons[i+1]);
        MapLine* line = new MapLine(pos1, pos2);
        line->setColor(Qt::red);
        track->mapLines.append(line);
        mMapWidget->addItem(line);
    }

    mTracks.append(track);
    if (!dataTipTrack) {
        dataTipTrack = track;
        mTrackCrosshair->setVisible(true);
    }

    zoomTo(latmin, lonmin, latmax, lonmax);
}

void MapPlot::syncDataTip(int index)
{
    if (!mTrackCrosshair->isVisible()) { return; }
    if (!dataTipTrack) { return; }

    index -= dataTipTrack->range.start;
    if ((index < 0) || (index >= dataTipTrack->lats.count())) { return; }
    double lat = dataTipTrack->lats[index];
    double lon = dataTipTrack->lons[index];
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

void MapPlot::setupCrosshairs()
{
    mTrackCrosshair = new MapMarker(QGV::GeoPos(), 5);
    mTrackCrosshair->setBrush(QBrush(QColor(255, 0, 0, 100)));
    mTrackCrosshair->setVisible(false);
    mMapWidget->addItem(mTrackCrosshair);
}

MapPlot::ClosestCoord MapPlot::findClosestCoord(QGV::GeoPos pos, TrackPtr track)
{
    ClosestCoord closest;

    if (!track) {
        closest.valid = false;
        return closest;
    }

    for (int i = 0; i < track->lats.count(); i++) {

        double lat = track->lats[i];
        double lon = track->lons[i];
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
        }
    }
}
