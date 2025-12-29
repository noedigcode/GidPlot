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

class MapPlot;
typedef QSharedPointer<MapPlot> MapPlotPtr;

class MapPlot : public Plot
{
    Q_OBJECT
public:
    explicit MapPlot(QGVMap *mapWidget, QWidget *parentWidget);

    static MapPlotPtr castFromPlot(PlotPtr plot);

    void plot(CsvPtr csv, int iloncol, int ilatcol, Range range);

    void setMapOSM();
    void zoomTo(double lat1, double lon1, double lat2, double lon2);

    bool saveToPng(QString path);
    QPixmap toPixmap();

    bool mouseCrosshairVisible();
    void setMouseCrosshairVisible(bool visible);
    bool plotCrosshairVisible();
    void setPlotCrosshairVisible(bool visible);
    void resized();
    void showAll();
    void syncAxisRanges(QRectF xyrange);
    void syncDataTip(int index);

private:
    static QNetworkAccessManager netAccMgr;
    static QNetworkDiskCache netCache;

    int mPenIndex = 0;

    QGVMap* mMapWidget = nullptr;
    QGVLayerTiles* mTilesItem = nullptr;

    void setupLink();

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


#endif // MAPPLOT_H
