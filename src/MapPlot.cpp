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

    // Set up legend
    mLegend.reset(new QGVLegendWidget(mMapWidget));
    mLegend->setVisible(false);

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

void MapPlot::setDataTipGraph(GraphPtr graph)
{
    dataTipGraph = graph;
    // Refresh plot crosshair visibility (in case graph is null)
    setPlotCrosshairVisible(mPlotCrosshairVisible);
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
        QGVLine* line = new QGVLine(pos1, pos2);
        line->setColor(track->pen.color());
        track->mapLines.append(line);
        mMapWidget->addItem(line);
    }

    mGraphs.append(graph);
    if (!dataTipGraph) {
        setDataTipGraph(graph);
    }

    // Add to legend
    mLegend->addEntry(track->pen.color(), track->name);
    if (mGraphs.count() == 1) {
        mLegend->snapToCorner(Qt::TopRightCorner);
    }
    if (mAutoShowLegend) {
        mShowLegend = (mGraphs.count() > 1);
    }
    mLegend->setVisible(mShowLegend && !mGraphs.isEmpty());

    showAll();

    // Call resized() to update crosshairs widget size. Queue it to give MapPlot
    // a chance to be set up and displayed.
    QMetaObject::invokeMethod(this, [=]() { resized(); }, Qt::QueuedConnection);
}

void MapPlot::syncAxisRanges(QRectF /*xyrange*/)
{
    // Not applicable for map
}

void MapPlot::syncDataTip(int index)
{
    if (!plotCrosshairVisible()) { return; }
    if (!dataTipGraph) { return; }

    index -= dataTipGraph->range.start;
    if ((index < 0) || (index >= dataTipGraph->dataCount())) { return; }
    double lat = dataTipGraph->track->lats[index];
    double lon = dataTipGraph->track->lons[index];
    mPlotCrosshair->setPosition(QGV::GeoPos(lat, lon));
}

void MapPlot::setMapOSM()
{
    setMapTiles(new QGVLayerOSM()); // Ownership is taken of layer and it will
                                    // be deleted when a new layer is set.
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

Plot::Properties MapPlot::getPlotProperties()
{
    Properties p;

    p.plotCrosshair = plotCrosshairVisible();
    p.plotDot = mPlotCrosshair->isMarkerVisible();
    p.plotHline = mPlotCrosshair->isHlineVisible();
    p.plotVline = mPlotCrosshair->isVlineVisible();

    p.mouseCrosshair = mouseCrosshairVisible();
    p.mouseDot = mMouseCrosshair->isMarkerVisible();
    p.mouseHline = mMouseCrosshair->isHlineVisible();
    p.mouseVline = mMouseCrosshair->isVlineVisible();

    p.supportShowTitle = false;
    p.supportXlabel = false;
    p.supportYlabel = false;

    p.title = this->title();

    p.showLegend = mShowLegend;

    return p;
}

void MapPlot::setPlotProperties(Properties p)
{
    setPlotCrosshairVisible(p.plotCrosshair);
    mPlotCrosshair->setMarkerVisible(p.plotDot);
    mPlotCrosshair->setHlineVisible(p.plotHline);
    mPlotCrosshair->setVlineVisible(p.plotVline);

    setMouseCrosshairVisible(p.mouseCrosshair);
    mMouseCrosshair->setMarkerVisible(p.mouseDot);
    mMouseCrosshair->setHlineVisible(p.mouseHline);
    mMouseCrosshair->setVlineVisible(p.mouseVline);

    this->setTitle(p.title);

    if (p.showLegend != mShowLegend) {
        mAutoShowLegend = false;
        mShowLegend = p.showLegend;
        mLegend->setVisible(mShowLegend && !mGraphs.isEmpty());
    }
}

void MapPlot::renameGraph(GraphPtr graph, QString name)
{
    graph->track->name = name;
    mLegend->setEntryName(mGraphs.indexOf(graph), name);
}

void MapPlot::setGraphColor(GraphPtr graph, QColor color)
{
    graph->track->pen.setColor(color);

    foreach (QGVLine* ml, graph->track->mapLines) {
        ml->setColor(graph->track->pen.color());
    }

    mLegend->setEntryColor(mGraphs.indexOf(graph), color);
}

void MapPlot::removeGraph(GraphPtr graph)
{
    if (!graph) { return; }

    // Remove from legend
    mLegend->removeEntry(mGraphs.indexOf(graph));

    // Remove from map
    while (!graph->track->mapLines.isEmpty()) {
        QGVLine* ml = graph->track->mapLines.takeFirst();
        mMapWidget->removeItem(ml);
        delete ml;
    }

    // Remove from data structures
    mGraphs.removeAll(graph);

    // Remove from datatip
    if (dataTipGraph == graph) {
        setDataTipGraph(mGraphs.value(0));
    }

    // Recalculate overall min/max of remaining tracks
    bounds = QRectF();
    foreach (GraphPtr graph, mGraphs) {
        expandBounds(graph->dataBounds());
    }

    if (mAutoShowLegend) {
        mShowLegend = (mGraphs.count() > 1);
    }
    mLegend->setVisible(mShowLegend && !mGraphs.isEmpty());
}

bool MapPlot::plotCrosshairVisible()
{
    return mPlotCrosshairVisible;
}

void MapPlot::setPlotCrosshairVisible(bool visible)
{
    mPlotCrosshairVisible = visible;
    mPlotCrosshair->setVisible(visible && !dataTipGraph.isNull());
}

bool MapPlot::mouseCrosshairVisible()
{
    return mMouseCrosshair->isVisible();
}

void MapPlot::setMouseCrosshairVisible(bool visible)
{
    mMouseCrosshair->setVisible(visible);
}

void MapPlot::resized()
{
    mPlotCrosshair->lines->resize(mMapWidget->size());
    mMouseCrosshair->lines->resize(mMapWidget->size());
    mLegend->updatePlacement();
}

void MapPlot::showEvent()
{
    resized();
}

void MapPlot::setupCrosshairs()
{
    mPlotCrosshair = new Crosshair(mMapWidget);
    mMouseCrosshair = new Crosshair(mMapWidget);
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
    QPoint mousePixelPos = mMapWidget->mapFromProj(projPos);

    if (mPlotCrosshairVisible && dataTipGraph) {

        ClosestCoord closestProjPos = findClosestCoord(mousePixelPos, dataTipGraph,
                                                       ClosestXY);
        if (closestProjPos.valid) {

            mPlotCrosshair->setPosition(closestProjPos.coord);

            emit dataTipChanged(link->group,
                                closestProjPos.dataIndex + dataTipGraph->range.start);
            mPlotCrosshairIndex = closestProjPos.dataIndex;
        }
    }

    if (mMouseCrosshair->isVisible()) {
        mMouseCrosshair->setPosition(projPos, mousePixelPos);
    }
}

void MapPlot::onActionEqualAxesTriggered()
{
    // Not applicable for map
}

MapPlot::Crosshair::Crosshair(QGVMap *mapWidget)
{
    mMapWidget = mapWidget;

    marker = new QGVMarker(QGV::GeoPos(), 5);
    marker->setBrush(QBrush(QColor(255, 0, 0, 100)));
    marker->setVisible(mVisible);
    marker->bringToFront();
    mMapWidget->addItem(marker);

    label = new QGVWidgetText();
    label->setStyleSheet(
                "padding: 3px;"
                "background-color: rgba(255,255,255,150);");
    label->setVisible(mVisible);
    mMapWidget->addWidget(label);

    lines = new QGVCrosshairWidget();
    lines->move(0, 0);
    lines->resize(mMapWidget->size());
    lines->setVisible(mVisible);
    mMapWidget->addWidget(lines);
}

void MapPlot::Crosshair::setPosition(QGV::GeoPos geoPos, QPoint pixelPos)
{
    marker->setPosition(geoPos);

    label->setText(QString("%1, %2")
                   .arg(QString::number(geoPos.latitude(), 'f', 7))
                   .arg(QString::number(geoPos.longitude(), 'f', 7)));

    static const int offset = 5;
    label->move(QPoint(pixelPos.x() + offset,
                       pixelPos.y() - label->height() - offset));

    lines->setPos(pixelPos);
}

void MapPlot::Crosshair::setPosition(QPointF projPos, QPoint pixelPos)
{
    QGV::GeoPos geoPos = mMapWidget->getProjection()->projToGeo(projPos);
    setPosition(geoPos, pixelPos);
}

void MapPlot::Crosshair::setPosition(QPointF projPos)
{
    QPoint pixelPos = mMapWidget->mapFromProj(projPos);
    setPosition(projPos, pixelPos);
}

void MapPlot::Crosshair::setPosition(QGV::GeoPos geoPos)
{
    QPointF projPos = mMapWidget->getProjection()->geoToProj(geoPos);
    QPoint pixelPos = mMapWidget->mapFromProj(projPos);
    setPosition(geoPos, pixelPos);
}

bool MapPlot::Crosshair::isVisible()
{
    return mVisible;
}

void MapPlot::Crosshair::setVisible(bool set)
{
    mVisible = set;

    marker->setVisible(mShowMarker & set);
    label->setVisible(mShowLabel & set);
    lines->setVisible(set);
}

void MapPlot::Crosshair::setMarkerVisible(bool visible)
{
    mShowMarker = visible;
    marker->setVisible(mShowMarker & mVisible);
}

bool MapPlot::Crosshair::isMarkerVisible()
{
    return mShowMarker;
}

void MapPlot::Crosshair::setLabelVisible(bool visible)
{
    mShowLabel = visible;
    label->setVisible(mShowLabel & mVisible);
}

void MapPlot::Crosshair::setHlineVisible(bool visible)
{
    lines->setHorizontalLineVisible(visible);
}

bool MapPlot::Crosshair::isHlineVisible()
{
    return lines->horizontalLineVisible();
}

void MapPlot::Crosshair::setVlineVisible(bool visible)
{
    lines->setVerticalLineVisible(visible);
}

bool MapPlot::Crosshair::isVlineVisible()
{
    return lines->verticalLineVisible();
}
