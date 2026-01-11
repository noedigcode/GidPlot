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

#include "csv.h"
#include "graph.h"
#include "plot.h"
#include "mapline.h"
#include "mapMarker.h"

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
    explicit MapPlot(QGVMap *mapWidget, QWidget *parentWidget);

    static MapPlotPtr castFromPlot(PlotPtr plot);

    void plot(CsvPtr csv, int iloncol, int ilatcol, Range range);

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

    struct Crosshair {
        Crosshair() {}
        Crosshair(QGVMap* mapWidget);

        /* setPosition(geoPos, pixelPos) is what is really needed to draw the
         * dot on the map, set the label text and set the label screen
         * coordinates.
         * The other are convenience functions. Use the one corresponding to
         * the info you have available to avoid unnecessary calculations. */
        void setPosition(QGV::GeoPos geoPos, QPoint pixelPos);
        void setPosition(QPointF projPos, QPoint pixelPos);
        void setPosition(QPointF projPos);
        void setPosition(QGV::GeoPos geoPos);

        bool isVisible();
        void setVisible(bool set);
        MapMarker* marker = nullptr;
        QGVWidgetText* label = nullptr;
        QGVMap* mMapWidget = nullptr;
        bool mVisible = false;
    };

    Crosshair* mTrackCrosshair = nullptr;
    Crosshair* mMouseCrosshair = nullptr;

    QPointF pixelPosToCoord(QPoint pos);
    QPoint coordToPixelPos(QPointF coord);

    QPointF latLonToPoint(double lat, double lon);
    QGV::GeoPos pointToGeo(QPointF pos);
    QPointF geoToPoint(QGV::GeoPos pos);

    void setMapTiles(QGVLayerTiles* tiles);
    void removeTiles();

private slots:
    void onMapMouseMove(QPointF projPos);
};


#endif // MAPPLOT_H
