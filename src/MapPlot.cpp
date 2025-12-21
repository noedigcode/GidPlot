#include "MapPlot.h"

#include <QLayout>

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
    mMapWidget->layout()->setMargin(0);
    setMapOSM();
}

void MapPlot::plot(CsvPtr csv, int iloncol, int ilatcol, Range range)
{
    QVector<double> lats = csv->matrix->data[ilatcol].mid(range.start, range.size());
    QVector<double> lons = csv->matrix->data[iloncol].mid(range.start, range.size());

    if (mFirstPlot) {
        latmin = Matrix::vmin(lats);
        latmax = Matrix::vmax(lats);
        lonmin = Matrix::vmin(lons);
        lonmax = Matrix::vmax(lons);
    } else {
        latmin = qMin(latmin, Matrix::vmin(lats));
        latmax = qMax(latmax, Matrix::vmax(lats));
        lonmin = qMin(lonmin, Matrix::vmin(lons));
        lonmax = qMax(lonmax, Matrix::vmax(lons));
    }

    for (int i = range.start; i < range.end - 1; i++) {
        QGV::GeoPos pos1(lats[i], lons[i]);
        QGV::GeoPos pos2(lats[i+1], lons[i+1]);
        MapLine* line = new MapLine(pos1, pos2);
        line->setColor(Qt::red);
        mMapWidget->addItem(line);
    }

    zoomTo(latmin, lonmin, latmax, lonmax);

    mFirstPlot = false;
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
