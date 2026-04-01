/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2026 Gideon van der Kolf
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef MAPPLOT_H
#define MAPPLOT_H

#include "MarkerEditDialog.h"
#include "QGVAnnotationItem.h"
#include "QGVCrosshairWidget.h"
#include "QGVLegendWidget.h"
#include "QGVLine.h"
#include "QGVMarker.h"
#include "csv.h"
#include "graph.h"
#include "plot.h"

#include "QGeoView/QGVMap.h"
#include "QGeoView/QGVLayerTiles.h"
#include "QGeoView/QGVLayerOSM.h"
#include "QGeoView/QGVWidgetText.h"

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

    // -----------------------------------------------------------------------

    struct Marker
    {
        QString datasetName;
        int dataIndex = -1;
        QGV::GeoPos pos;
        QGVAnnotationItem* mapAnnotation = nullptr;
        QString text;
    };
    typedef QSharedPointer<Marker> MarkerPtr;

    // -----------------------------------------------------------------------

    explicit MapPlot(QGVMap *mapWidget, QWidget *parentWidget);

    static MapPlotPtr castFromPlot(PlotPtr plot);

    void plot(CsvPtr csv, int iloncol, int ilatcol, Range range);
    void showEmpty();

    void setMapOSM();
    void zoomTo(double lat1, double lon1, double lat2, double lon2);
    void zoomTo(QGV::GeoRect geoRect);

    bool saveToPng(QString path);
    QPixmap toPixmap();

    Properties getPlotProperties();
    void setPlotProperties(Properties p);

    void renameGraph(GraphPtr graph, QString name);
    void setGraphColor(GraphPtr graph, QColor color);
    void removeGraph(GraphPtr graph);

    bool mouseCrosshairVisible();
    void setMouseCrosshairVisible(bool visible);
    bool plotCrosshairVisible();
    void setPlotCrosshairVisible(bool visible);
    void resized();
    void showEvent();
    void showAll();
    void syncAxisRanges(QRectF xyrange);
    void syncDataTip(int index);

private:
    static QNetworkAccessManager netAccMgr;
    static QNetworkDiskCache netCache;

    int mPenIndex = 0;

    QGVMap* mMapWidget = nullptr;
    QGVLayerTiles* mTilesItem = nullptr;
    void setMapTiles(QGVLayerTiles* tiles);
    void removeTiles();

    void setupLink();

    void setDataTipGraph(GraphPtr graph);

    // -----------------------------------------------------------------------
    // Legend

    QScopedPointer<QGVLegendWidget> mLegend;
    bool mShowLegend = true;
    bool mAutoShowLegend = true;

    // -----------------------------------------------------------------------
    // Menus
private:
    void setupMenus();
private slots:
    void onActionCopyCurveCoordinateTriggered();
    void onActionCopyCurveIndexTriggered();
    void onActionCopyMouseCoordinateTriggered();
    void onActionPlaceMarkerOnCurveTriggered();
    void onActionPlaceMarkerAtMouseTriggered();
    void onActionPasteMarkerTriggered();
    void onActionMeasureTriggered();
    void onActionEqualAxesTriggered();

    // -----------------------------------------------------------------------
    // Crosshairs
private:
    void setupCrosshairs();

    struct Crosshair {
        Crosshair() {}
        Crosshair(QGVMap* mapWidget);

        /* setPosition(geoPos, pixelPos) is what is really needed to draw the
         * dot on the map, set the label text and set the label screen
         * coordinates.
         * The others are convenience functions. Use the one corresponding to
         * the info you have available to avoid unnecessary calculations. */
        void setPosition(QGV::GeoPos geoPos, QPoint pixelPos, int index = -1);
        void setPosition(QPointF projPos, QPoint pixelPos, int index = -1);
        void setPosition(QPointF projPos, int index = -1);
        void setPosition(QGV::GeoPos geoPos, int index = -1);

        bool isVisible();
        void setVisible(bool set);

        QGVMarker* marker = nullptr;
        void setMarkerVisible(bool visible);
        bool isMarkerVisible();

        QGVWidgetText* label = nullptr;
        void setLabelVisible(bool visible);
        bool isLabelVisible();

        QGVCrosshairWidget* lines = nullptr;
        void setHlineVisible(bool visible);
        bool isHlineVisible();
        void setVlineVisible(bool visible);
        bool isVlineVisible();

        QGVMap* mMapWidget = nullptr;

    private:
        bool mVisible = false;
        bool mShowMarker = true;
        bool mShowLabel = true;
    };

    bool mPlotCrosshairVisible = true;
    Crosshair* mPlotCrosshair = nullptr;
    Crosshair* mMouseCrosshair = nullptr;

    // -----------------------------------------------------------------------
    // Helpers

    QPointF pixelPosToCoord(QPoint pos);
    QPoint coordToPixelPos(QPointF coord);

    QPointF latLonToPoint(double lat, double lon);
    QGV::GeoPos pointToGeo(QPointF pos);
    QPointF geoToPoint(QGV::GeoPos pos);

    double geoDistance(QGV::GeoPos a, QGV::GeoPos b);
    double geoHeading(QGV::GeoPos a, QGV::GeoPos b);

    static QString formatLatLon(double value);

    // -----------------------------------------------------------------------
    // Mouse

    struct Mouse {
        QPointF lastMoveProjPos;
    } mouse;

private slots:    
    void onMapMouseMove(QPointF projPos);
    void onMapMouseClick(QPointF projPos);

    // -----------------------------------------------------------------------
    // Markers
private:
    QList<MarkerPtr> mMarkers;
    MarkerEditDialog mMarkerEditDialog;
    void setupMarkerEditDialog();
    MarkerPtr addMarker(QGV::GeoPos geoPos);
    void updateMarkerText(MarkerPtr marker);
    void editMarkerText(MarkerPtr marker);
    void deleteMarker(MarkerPtr marker);
    void onMarkerRightClick(MarkerPtr marker, QPoint pixelPos);

    // -----------------------------------------------------------------------
    // Measures
private:
    struct Measure
    {
        MarkerPtr a;
        MarkerPtr b;
        QString tag;
    };
    typedef QSharedPointer<Measure> MeasurePtr;

    QList<MeasurePtr> mMeasures;
    MeasurePtr mCurrentMeasure;
    void startMeasure(MeasurePtr meas);
    void clearCurrentMeasure();
    int mMeasureCounter = 1;
};


#endif // MAPPLOT_H
