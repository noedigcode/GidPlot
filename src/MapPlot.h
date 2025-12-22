#ifndef MAPPLOT_H
#define MAPPLOT_H

#include "csv.h"
#include "link.h"
#include "mapline.h"
#include "mapMarker.h"

#include "QGeoView/QGVMap.h"
#include "QGeoView/QGVLayerTiles.h"
#include "QGeoView/QGVLayerOSM.h"

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QObject>

// ===========================================================================

struct Track
{
    CsvPtr csv;
    int iloncol = 0;
    int ilatcol = 0;
    Range range;

    QVector<double> lats;
    QVector<double> lons;

    Matrix::VStats latStats;
    Matrix::VStats lonStats;

    QList<MapLine*> mapLines;
};
typedef QSharedPointer<Track> TrackPtr;

// ===========================================================================

class MapPlot : public QObject
{
    Q_OBJECT
public:
    explicit MapPlot(QGVMap *mapWidget, QObject *parent = nullptr);

    void setupLink();
    LinkPtr link {new Link()};

    void plot(CsvPtr csv, int iloncol, int ilatcol, Range range);
    void syncDataTip(int index);

    void setMapOSM();
    void zoomTo(double lat1, double lon1, double lat2, double lon2);

signals:
    void dataTipChanged(int linkGroup, int index);

private:
    static QNetworkAccessManager netAccMgr;
    static QNetworkDiskCache netCache;

    QGVMap* mMapWidget = nullptr;
    QGVLayerTiles* mTilesItem = nullptr;

    QList<TrackPtr> mTracks;
    TrackPtr dataTipTrack;

    double latmin = 0;
    double latmax = 0;
    double lonmin = 0;
    double lonmax = 0;

    // Crosshairs
    void setupCrosshairs();
    MapMarker* mTrackCrosshair = nullptr;

    struct ClosestCoord {
        bool valid = false;
        double distance = 0;
        double lat = 0;
        double lon = 0;
        int dataIndex;
    };
    ClosestCoord findClosestCoord(QGV::GeoPos pos, TrackPtr track);

    void setMapTiles(QGVLayerTiles* tiles);
    void removeTiles();

private slots:
    void onMapMouseMove(QPointF projPos);
};

typedef QSharedPointer<MapPlot> MapPlotPtr;

#endif // MAPPLOT_H
