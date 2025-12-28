#ifndef MAPPLOT_H
#define MAPPLOT_H

#include "csv.h"
#include "graph.h"
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

class MapPlot : public QObject
{
    Q_OBJECT
public:
    explicit MapPlot(QGVMap *mapWidget, QObject *parent = nullptr);

    LinkPtr link {new Link()};

    void plot(CsvPtr csv, int iloncol, int ilatcol, Range range);
    void syncDataTip(int index);

    void setMapOSM();
    void zoomTo(double lat1, double lon1, double lat2, double lon2);

    void showAll();

signals:
    void dataTipChanged(int linkGroup, int index);
    void linkSettingsTriggered();

private:
    static QNetworkAccessManager netAccMgr;
    static QNetworkDiskCache netCache;

    int mPenIndex = 0;

    QGVMap* mMapWidget = nullptr;
    QGVLayerTiles* mTilesItem = nullptr;

    void setupLink();

    QList<GraphPtr> mTracks;
    QList<GraphPtr> getAllGraphs();
    GraphPtr dataTipTrack;
    GraphPtr getDataTipGraph();

    double latmin = 0;
    double latmax = 0;
    double lonmin = 0;
    double lonmax = 0;

    void showCrosshairsDialog();

    // -----------------------------------------------------------------------
    // Menus
private:
    PlotMenu plotMenu;
    void setupMenus();
private slots:
    void onActionPlaceMarkerTriggered();
    void onActionMeasureTriggered();

    // -----------------------------------------------------------------------
    // Crosshairs
private:
    void setupCrosshairs();
    MapMarker* mTrackCrosshair = nullptr;
    int mTrackCrosshairIndex = 0;

    struct ClosestCoord {
        bool valid = false;
        double distance = 0;
        double lat = 0;
        double lon = 0;
        int dataIndex;
    };
    ClosestCoord findClosestCoord(QGV::GeoPos pos, GraphPtr graph);

    void setMapTiles(QGVLayerTiles* tiles);
    void removeTiles();

private slots:
    void onMapMouseMove(QPointF projPos);
};

typedef QSharedPointer<MapPlot> MapPlotPtr;

#endif // MAPPLOT_H
