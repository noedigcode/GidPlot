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

#include "MapPlot.h"

#include "QGVMapQGView.h"

#include <QLayout>
#include <QtMath>

// Static members
QNetworkAccessManager MapPlot::netAccMgr;
QNetworkDiskCache MapPlot::netCache;


MapPlot::MapPlot(QGVMap *mapWidget, QWidget *parentWidget)
    : Plot{parentWidget}, mMapWidget(mapWidget)
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

MapPlotPtr MapPlot::castFromPlot(PlotPtr plot)
{
    return qSharedPointerCast<MapPlot>(plot);
}

void MapPlot::setupLink()
{
    link->supportPosZoom = false;
    link->tag = "Map Plot";
}

void MapPlot::setupMenus()
{
    // TODO
    plotMenu.actionPlaceMarker->setVisible(false);
    qDebug() << "TODO Implement place marker for map plots";

    // TODO
    plotMenu.actionMeasure->setVisible(false);
    qDebug() << "TODO Implement measure for map plots";

    // Equal axes toggling not applicable for map
    plotMenu.actionEqualAxes->setVisible(false);

    // QGVMap displays a context menu of all actions added to it
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
    if (range.size() != csv->matrix->rowCount()) {
        track->name = QString("%1 (%2)").arg(track->name).arg(range.name);
    }

    graph->ystats = Matrix::vstats(track->lats);
    graph->xstats = Matrix::vstats(track->lons);

    // Combine track mins/maxes with overall of all tracks

    expandBounds(graph->dataBounds());

    // Note: we build the grid hash from projected coordinates, as drawn on
    // screen, not geo coordinates, since the grid assumes a flat plane.
    QRectF r = graph->dataBounds();
    QGV::GeoRect gr(pointToGeo(r.topLeft()), pointToGeo(r.bottomRight()));
    QRectF projBounds = mMapWidget->getProjection()->geoToProj(gr);
    graph->grid.bounds = projBounds;
    for (int i = 0; i < track->lats.count(); i++) {
        QPointF projPoint = mMapWidget->getProjection()->geoToProj(
                    QGV::GeoPos(track->lats[i], track->lons[i]));
        graph->grid.insert(projPoint, i);
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

    graphs.append(graph);
    if (!dataTipGraph) {
        dataTipGraph = graph;
        mTrackCrosshair->setVisible(true);
    }

    showAll();
}

void MapPlot::syncAxisRanges(QRectF /*xyrange*/)
{
    // Not applicable for map
}

void MapPlot::syncDataTip(int index)
{
    if (!mTrackCrosshair->isVisible()) { return; }
    if (!dataTipGraph) { return; }

    index -= dataTipGraph->range.start;
    if ((index < 0) || (index >= dataTipGraph->dataCount())) { return; }
    double lat = dataTipGraph->track->lats[index];
    double lon = dataTipGraph->track->lons[index];
    mTrackCrosshair->setPosition(QGV::GeoPos(lat, lon));
}

void MapPlot::setMapOSM()
{
    setMapTiles(new QGVLayerOSM());
}

void MapPlot::zoomTo(double lat1, double lon1, double lat2, double lon2)
{
    zoomTo(QGV::GeoRect(lat1, lon1, lat2, lon2));
}

void MapPlot::zoomTo(QGV::GeoRect geoRect)
{
    QMetaObject::invokeMethod(this, [=]()
    {
        mMapWidget->cameraTo(QGVCameraActions(mMapWidget).scaleTo(geoRect));
    }, Qt::QueuedConnection);
}

void MapPlot::showAll()
{
    zoomTo(QGV::GeoRect(
               pointToGeo(bounds.topLeft()),
               pointToGeo(bounds.bottomRight()) ));
}

bool MapPlot::saveToPng(QString path)
{
    bool ok = toPixmap().save(path, "PNG");

    return ok;
}

QPixmap MapPlot::toPixmap()
{
    QGVMapQGView* geoView = mMapWidget->geoView();

    // Get the size of your QGeoView viewport
    QRect rect = geoView->viewport()->rect();

    // Create a pixmap the same size
    QPixmap pixmap(rect.size());
    QPainter painter(&pixmap);

    // Render the current view onto the pixmap
    geoView->render(&painter, pixmap.rect(), rect);

    return pixmap;
}

bool MapPlot::plotCrosshairVisible()
{
    return mTrackCrosshair->isVisible();
}

void MapPlot::setPlotCrosshairVisible(bool visible)
{
    mTrackCrosshair->setVisible(visible);
}

bool MapPlot::mouseCrosshairVisible()
{
    // TODO Mouse crosshair
    return false;
}

void MapPlot::setMouseCrosshairVisible(bool /*visible*/)
{
    // TODO Mouse crosshair
}

void MapPlot::resized()
{

}

void MapPlot::setupCrosshairs()
{
    mTrackCrosshair = new MapMarker(QGV::GeoPos(), 5);
    mTrackCrosshair->setBrush(QBrush(QColor(255, 0, 0, 100)));
    mTrackCrosshair->setVisible(false);
    mTrackCrosshair->bringToFront();
    mMapWidget->addItem(mTrackCrosshair);
}

CrosshairsDialog::Settings MapPlot::crosshairsDialogAboutToShow()
{
    CrosshairsDialog::Settings s;
    s.plotCrosshair = mTrackCrosshair->isVisible();
    // TODO: Sub-parts of crosshair
    s.plotDot = true;

    // TODO: Mouse crosshair settings

    return s;
}

void MapPlot::crosshairsDialogChanged(CrosshairsDialog::Settings s)
{
    mTrackCrosshair->setVisible(s.plotCrosshair);
    // TODO: Sub-parts of crosshair

    // TODO: Mouse crosshair
}

QPointF MapPlot::pixelPosToCoord(QPoint pos)
{
    // Map pixel position to projection as seen on screen (not geo coordinates)
    return mMapWidget->mapToProj(pos);
}

QPoint MapPlot::coordToPixelPos(QPointF coord)
{
    // Map projected coordinate as drawn on screen (not geo coordinate) to
    // pixel position
    return mMapWidget->mapFromProj(coord);
}

QPointF MapPlot::latLonToPoint(double lat, double lon)
{
    return QPointF(lon, lat);
}

QGV::GeoPos MapPlot::pointToGeo(QPointF pos)
{
    return QGV::GeoPos(pos.y(), pos.x());
}

QPointF MapPlot::geoToPoint(QGV::GeoPos pos)
{
    return latLonToPoint(pos.latitude(), pos.longitude());
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
    QPoint pixelPos = mMapWidget->mapFromProj(projPos);
    if (mTrackCrosshair->isVisible() && dataTipGraph) {
        ClosestCoord closest = findClosestCoord(
                    pixelPos, dataTipGraph,
                    ClosestXY);
        if (closest.valid) {
            QGV::GeoPos closestPos = mMapWidget->getProjection()->projToGeo(closest.coord);
            mTrackCrosshair->setPosition(closestPos);
            emit dataTipChanged(link->group,
                                closest.dataIndex + dataTipGraph->range.start);
            mPlotCrosshairIndex = closest.dataIndex;
        }
    }
}

void MapPlot::onActionEqualAxesTriggered()
{
    // Not applicable for map
}
