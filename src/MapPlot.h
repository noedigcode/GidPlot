#ifndef MAPPLOT_H
#define MAPPLOT_H

#include "csv.h"
#include "mapline.h"

#include "QGeoView/QGVMap.h"
#include "QGeoView/QGVLayerTiles.h"
#include "QGeoView/QGVLayerOSM.h"

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QObject>

class MapPlot : public QObject
{
    Q_OBJECT
public:
    explicit MapPlot(QGVMap *mapWidget, QObject *parent = nullptr);

    void plot(CsvPtr csv, int iloncol, int ilatcol, Range range);

    void setMapOSM();
    void zoomTo(double lat1, double lon1, double lat2, double lon2);

signals:

private:
    static QNetworkAccessManager netAccMgr;
    static QNetworkDiskCache netCache;

    QGVMap* mMapWidget = nullptr;
    QGVLayerTiles* mTilesItem = nullptr;

    bool mFirstPlot = true;
    double latmin = 0;
    double latmax = 0;
    double lonmin = 0;
    double lonmax = 0;


    void setMapTiles(QGVLayerTiles* tiles);
    void removeTiles();
};

#endif // MAPPLOT_H
