#ifndef MAPPLOT_H
#define MAPPLOT_H

#include "csv.h"
#include "graph.h"
#include "plot.h"
#include "mapline.h"
#include "mapMarker.h"

#include "QGeoView/QGVMap.h"
#include "QGeoView/QGVLayerTiles.h"
#include "QGeoView/QGVLayerOSM.h"

#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QObject>

// ===========================================================================

class MapPlot : public Plot
{
    Q_OBJECT
public:
    explicit MapPlot(QGVMap *mapWidget, QWidget *parentWidget = nullptr);

    void plot(CsvPtr csv, int iloncol, int ilatcol, Range range);
    void syncDataTip(int index);

    void setMapOSM();
    void zoomTo(double lat1, double lon1, double lat2, double lon2);

    void showAll();

    bool saveToPng(QString path);
    QPixmap toPixmap();

    bool plotCrosshairVisible();
    void setPlotCrosshairVisible(bool visible);
    bool mouseCrosshairVisible();
    void setMouseCrosshairVisible(bool visible);

    void resizeEvent();

private:
    static QNetworkAccessManager netAccMgr;
    static QNetworkDiskCache netCache;

    int mPenIndex = 0;

    QGVMap* mMapWidget = nullptr;
    QGVLayerTiles* mTilesItem = nullptr;

    void setupLink();


    double latmin = 0;
    double latmax = 0;
    double lonmin = 0;
    double lonmax = 0;

    // -----------------------------------------------------------------------
    // Menus
private:
    void setupMenus();
private slots:
    void onActionPlaceMarkerTriggered();
    void onActionMeasureTriggered();
    void onActionEqualAxesTriggered();

    // -----------------------------------------------------------------------
    // Crosshairs
private:
    void setupCrosshairs();
    MapMarker* mTrackCrosshair = nullptr;
    // TODO: Mouse crosshair

    CrosshairsDialog::Settings crosshairsDialogAboutToShow();
    void crosshairsDialogChanged(CrosshairsDialog::Settings s);

    bool lastTrackCrosshairVisible = true;

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
