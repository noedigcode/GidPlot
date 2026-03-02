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
    connect(mMapWidget, &QGVMap::mapMouseClicked, this, &MapPlot::onMapMouseClick);
    mMapWidget->setMouseTracking(true);
    mMapWidget->layout()->setMargin(0);
    setMapOSM();

    // Set up legend
    mLegend.reset(new QGVLegendWidget(mMapWidget));
    mLegend->setVisible(false);

    setupCrosshairs();
    setupLink();
    setupMenus();
    setupMarkerEditDialog();
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
    // Equal axes toggling not applicable to map
    plotMenu.actionEqualAxes->setVisible(false);

    // QGVMap displays a context menu of all actions added to it
    mMapWidget->addActions(plotMenu.actions());
}

void MapPlot::onActionCopyCurveCoordinateTriggered()
{
    GraphPtr graph = dataTipGraph;
    if (!graph) { graph = mGraphs.value(0); }
    if (!graph) { return; }

    QPoint mousePixelPos = mMapWidget->mapFromProj(mouse.lastMoveProjPos);
    ClosestCoord closestProjPos = findClosestCoord(mousePixelPos, graph, ClosestXY);
    if (!closestProjPos.valid) { return; }

    QGV::GeoPos geoPos = mMapWidget->getProjection()->projToGeo(closestProjPos.coord);

    QGuiApplication::clipboard()->setText(
                QString("%1, %2")
                .arg(MapPlot::formatLatLon(geoPos.latitude()))
                .arg(MapPlot::formatLatLon(geoPos.longitude())));
}

void MapPlot::onActionCopyCurveIndexTriggered()
{
    GraphPtr graph = dataTipGraph;
    if (!graph) { graph = mGraphs.value(0); }
    if (!graph) { return; }

    QPoint mousePixelPos = mMapWidget->mapFromProj(mouse.lastMoveProjPos);
    ClosestCoord closestProjPos = findClosestCoord(mousePixelPos, graph, ClosestXY);
    if (!closestProjPos.valid) { return; }

    QGuiApplication::clipboard()->setText(QString::number(closestProjPos.dataIndex));
}

void MapPlot::onActionCopyMouseCoordinateTriggered()
{
    GraphPtr graph = dataTipGraph;
    if (!graph) { graph = mGraphs.value(0); }
    if (!graph) { return; }

    QPoint mousePixelPos = mMapWidget->mapFromProj(mouse.lastMoveProjPos);
    QPointF projPos = pixelPosToCoord(mousePixelPos);
    QGV::GeoPos geoPos = mMapWidget->getProjection()->projToGeo(projPos);

    QGuiApplication::clipboard()->setText(
                QString("%1, %2")
                .arg(MapPlot::formatLatLon(geoPos.latitude()))
                .arg(MapPlot::formatLatLon(geoPos.longitude())));
}

void MapPlot::onActionPlaceMarkerOnCurveTriggered()
{
    GraphPtr graph = dataTipGraph;
    if (!graph) { graph = mGraphs.value(0); }
    if (!graph) { return; }

    QPoint mousePixelPos = mMapWidget->mapFromProj(mouse.lastMoveProjPos);
    ClosestCoord closestProjPos = findClosestCoord(mousePixelPos, graph, ClosestXY);
    if (!closestProjPos.valid) { return; }
    QGV::GeoPos geoPos = mMapWidget->getProjection()->projToGeo(closestProjPos.coord);

    MarkerPtr marker = addMarker(geoPos);

    marker->datasetName = graph->name();
    marker->dataIndex = closestProjPos.dataIndex;
    marker->text = "Lat: $lat\nLon: $lon\nIndex: $i";
    updateMarkerText(marker);
}

void MapPlot::onActionPlaceMarkerAtMouseTriggered()
{
    GraphPtr graph = dataTipGraph;
    if (!graph) { graph = mGraphs.value(0); }
    if (!graph) { return; }

    QPoint mousePixelPos = mMapWidget->mapFromProj(mouse.lastMoveProjPos);
    QPointF projPos = pixelPosToCoord(mousePixelPos);
    QGV::GeoPos geoPos = mMapWidget->getProjection()->projToGeo(projPos);

    MarkerPtr marker = addMarker(geoPos);

    marker->text = "Lat: $lat\nLon: $lon";
    updateMarkerText(marker);
}

void MapPlot::onActionPasteMarkerTriggered()
{
    GraphPtr graph = dataTipGraph;
    if (!graph) { graph = mGraphs.value(0); }
    if (!graph) { return; }

    QGV::GeoPos geoPos;
    int index = Plot::copiedMarkerData.dataIndex - dataTipGraph->range.start;
    if ((index >= 0) && (index < dataTipGraph->dataCount())) {
        // Valid index
        double lat = dataTipGraph->track->lats[index];
        double lon = dataTipGraph->track->lons[index];
        geoPos = QGV::GeoPos(lat, lon);
    } else {
        // Invalid index. See if it makes sense pasting the marker using coordinates.
        QRectF rect = Plot::bounds;
        double padx = bounds.width() * 0.5;
        double pady = bounds.height() * 0.5;
        rect.adjust(-padx, -pady, padx, pady);
        bool doPaste = true;
        if (!rect.contains(QPointF(Plot::copiedMarkerData.sourceX,
                                     Plot::copiedMarkerData.sourceY))) {
            // Copied marker coordinates are far out of our bounds.
            // Confirm with user.
            int button = QMessageBox::question(nullptr, "Paste Marker",
                QString("Are you sure you want to paste the marker?\n"
                "The copied marker has no data index and it may fall far outside "
                "of this plot's bounds.\n"
                "Source dataset: %1, x: %2, y: %3;\n")
                .arg(Plot::copiedMarkerData.sourceTitle)
                .arg(Plot::copiedMarkerData.sourceXaxis)
                .arg(Plot::copiedMarkerData.sourceYaxis));
            if (button != QMessageBox::Yes) {
                doPaste = false;
            }
        }
        if (doPaste) {
            geoPos = QGV::GeoPos(Plot::copiedMarkerData.sourceY,
                                 Plot::copiedMarkerData.sourceX);
        } else {
            return;
        }
    }

    MarkerPtr marker = addMarker(geoPos);

    marker->datasetName = graph->name();
    marker->dataIndex = Plot::copiedMarkerData.dataIndex;
    marker->text = Plot::copiedMarkerData.text;
    updateMarkerText(marker);
}

void MapPlot::onActionMeasureTriggered()
{
    if (!mCurrentMeasure) {
        // Not busy with a measure. Start a new one.
        Plot::plotMenu.setMeasureActionStarted();
    } else {
        // Busy with a measure. End it here.
        clearCurrentMeasure();
        return;
    }

    QPointF projPos = mouse.lastMoveProjPos;
    QGV::GeoPos geoPos = mMapWidget->getProjection()->projToGeo(projPos);

    MeasurePtr m = MeasurePtr::create();
    m->tag = QString("Measure %1").arg(mMeasureCounter++);

    QList<MarkerPtr> ab;
    for (int i = 0; i < 2; i++) {

        MarkerPtr a = addMarker(geoPos);
        a->text = QString("%1 A\n$lat, $lon").arg(m->tag);
        updateMarkerText(a);

        ab.append(a);
    }

    m->a = ab.value(0);
    m->b = ab.value(1);

    mMeasures.append(m);
    mCurrentMeasure = m;
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
    int lineCount = 0;
    for (int i = 0; i < track->lats.count() - 1; i++) {
        QGV::GeoPos pos1(track->lats[i], track->lons[i]);
        QGV::GeoPos pos2(track->lats[i+1], track->lons[i+1]);
        if (pos1 == pos2) { continue; }
        lineCount++;
        QGVLine* line = new QGVLine(pos1, pos2);
        line->setColor(track->pen.color());
        track->mapLines.append(line);
        mMapWidget->addItem(line);
    }
    qDebug() << "Map plot: Data length: " << track->lats.count() << "Lines drawn: " << lineCount;

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
    mPlotCrosshair->setPosition(QGV::GeoPos(lat, lon), index);
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

double MapPlot::geoDistance(QGV::GeoPos a, QGV::GeoPos b)
{
    // Haversine formula

    double alatrad = qDegreesToRadians(a.latitude());
    double blatrad = qDegreesToRadians(b.latitude());
    double dlatrad = qDegreesToRadians(b.latitude() - a.latitude());
    double dlonrad = qDegreesToRadians(b.longitude() - a.longitude());

    static const double EARTH_R = 6378137;

    return EARTH_R * 2 *
            qAsin(
                qSqrt(
                    qPow(qSin(dlatrad/2),2)
                    + qCos(alatrad) * qCos(blatrad) * qPow(qSin(dlonrad/2), 2)
                )
            );
}

double MapPlot::geoHeading(QGV::GeoPos a, QGV::GeoPos b)
{
    double alonrad = qDegreesToRadians(a.longitude());
    double alatrad = qDegreesToRadians(a.latitude());
    double blonrad = qDegreesToRadians(b.longitude());
    double blatrad = qDegreesToRadians(b.latitude());

    double headingRad = qAtan2((blonrad - alonrad) * qCos(blatrad),
                               (blatrad - alatrad));

    return qRadiansToDegrees(headingRad);
}

QString MapPlot::formatLatLon(double value)
{
    return QString::number(value, 'f', 7);
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
    mouse.lastMoveProjPos = projPos;
    QPoint mousePixelPos = mMapWidget->mapFromProj(projPos);

    // Plot crosshair
    if (mPlotCrosshairVisible && dataTipGraph) {

        ClosestCoord closestProjPos = findClosestCoord(mousePixelPos, dataTipGraph,
                                                       ClosestXY);
        if (closestProjPos.valid) {

            mPlotCrosshair->setPosition(closestProjPos.coord, closestProjPos.dataIndex);

            emit dataTipChanged(link->group,
                                closestProjPos.dataIndex + dataTipGraph->range.start);
            mPlotCrosshairIndex = closestProjPos.dataIndex;
        }
    }

    // Mouse crosshair
    if (mMouseCrosshair->isVisible()) {
        mMouseCrosshair->setPosition(projPos, mousePixelPos);
    }

    // Measure
    if (mCurrentMeasure) {
        QGV::GeoPos geoPos = mMapWidget->getProjection()->projToGeo(mouse.lastMoveProjPos);
        double dist = geoDistance(mCurrentMeasure->a->pos, geoPos);
        double heading = geoHeading(mCurrentMeasure->a->pos, geoPos);

        mCurrentMeasure->b->pos = geoPos;
        mCurrentMeasure->b->mapAnnotation->setAnchor(geoPos);
        mCurrentMeasure->b->text =
                QString("%1 B\n$lat, $lon\nDistance: %2 m\nHeading: %3 deg")
                .arg(mCurrentMeasure->tag)
                .arg(dist)
                .arg(heading);
        updateMarkerText(mCurrentMeasure->b);
    }
}

void MapPlot::onMapMouseClick(QPointF /*projPos*/)
{
    if (mCurrentMeasure) {
        // Stop measuring
        clearCurrentMeasure();
    }
}

void MapPlot::setupMarkerEditDialog()
{
    mMarkerEditDialog.setMapPlotMode();
}

void MapPlot::updateMarkerText(MarkerPtr marker)
{
    QString text = marker->text;
    text.replace("$i", QString::number(marker->dataIndex));
    text.replace("$x", formatLatLon(marker->pos.longitude()));
    text.replace("$y", formatLatLon(marker->pos.latitude()));
    text.replace("$lat", formatLatLon(marker->pos.latitude()));
    text.replace("$lon", formatLatLon(marker->pos.longitude()));
    text.replace("$name", marker->datasetName);
    text.replace("$$", "$");
    marker->mapAnnotation->setText(text);
}

void MapPlot::editMarkerText(MarkerPtr marker)
{
    if (!marker) { return; }

    mMarkerEditDialog.edit(marker->text,
                           false,
                           false,
                           false,
                           [this, markerWkPtr = marker.toWeakRef()]()
    {
        MarkerPtr m(markerWkPtr);
        if (!m) { return; }

        m->text = mMarkerEditDialog.text();

        updateMarkerText(m);
    });
}

void MapPlot::deleteMarker(MarkerPtr marker)
{
    if (!marker) { return; }

    mMapWidget->removeItem(marker->mapAnnotation);
    mMarkers.removeAll(marker);

    // Remove related measure
    foreach (MeasurePtr m, mMeasures) {
        if ((m->a == marker) || (m->b == marker)) {
            mMeasures.removeAll(m);
            if (mCurrentMeasure == m) {
                clearCurrentMeasure();
            }
        }
    }
}

void MapPlot::onMarkerRightClick(MarkerPtr marker, QPoint pixelPos)
{
    QMenu* menu = new QMenu();
    connect(menu, &QMenu::aboutToHide, this, [=]() { menu->deleteLater(); });

    // Use weak pointer to not capture shared pointer in lambdas that might
    // hold on to it.
    QWeakPointer<Marker> mWptr(marker);

    menu->addAction(QIcon("://edit"), "Edit",
                    this, [this, mWptr]()
    {
        MarkerPtr m(mWptr);
        if (!m) { return; }
        editMarkerText(m);
    });

    menu->addAction(QIcon("://copy"), "Copy Text/Marker",
                    this, [this, mWptr]()
    {
        MarkerPtr m(mWptr);
        if (!m) { return; }

        // Copy text, as displayed, to OS clipboard
        QGuiApplication::clipboard()->setText(m->mapAnnotation->text());

        // Store marker data for other plots to use
        Plot::copiedMarkerData.text = m->text;
        Plot::copiedMarkerData.dataIndex = m->dataIndex + dataTipGraph->range.start;
        Plot::copiedMarkerData.valid = true;
        Plot::copiedMarkerData.sourceX = m->pos.longitude();
        Plot::copiedMarkerData.sourceY = m->pos.latitude();
        Plot::copiedMarkerData.sourceXaxis = "Longitude";
        Plot::copiedMarkerData.sourceYaxis = "Latitude";
        Plot::copiedMarkerData.sourceTitle = Plot::title();
    });

    menu->addAction(QIcon("://coordinate"), "Copy Coordinate",
                    this, [this, mWptr]()
    {
        MarkerPtr m(mWptr);
        if (!m) { return; }

        // Copy coordinate to OS clipboard
        QGuiApplication::clipboard()->setText(
                    QString("%1, %2")
                    .arg(MapPlot::formatLatLon(m->pos.latitude()))
                    .arg(MapPlot::formatLatLon(m->pos.longitude())));
    });

    menu->addSeparator();

    menu->addAction(QIcon("://delete"), "Delete Marker",
                    this, [this, mWptr]()
    {
        MarkerPtr m(mWptr);
        if (!m) { return; }
        deleteMarker(m);
    });

    menu->popup(mMapWidget->mapToGlobal(pixelPos));
}

void MapPlot::clearCurrentMeasure()
{
    mCurrentMeasure.reset();
    // Restore measure action text that was set to end measure when started
    Plot::plotMenu.setMeasureActionEnded();
}

MapPlot::MarkerPtr MapPlot::addMarker(QGV::GeoPos geoPos)
{
    MarkerPtr marker = MarkerPtr::create();

    marker->pos = geoPos;

    marker->mapAnnotation = new QGVAnnotationItem();
    marker->mapAnnotation->setAnchor(geoPos);
    marker->mapAnnotation->setText("New Marker");

    connect(marker->mapAnnotation, &QGVAnnotationItem::rightClicked,
            this, [this, mWptr = marker.toWeakRef()](QPoint pixelPos)
    {
        MarkerPtr m(mWptr);
        if (!m) { return; }
        onMarkerRightClick(m, pixelPos);
    });
    connect(marker->mapAnnotation, &QGVAnnotationItem::doubleClicked,
            this, [this, mWptr = marker.toWeakRef()](QPoint /*pixelPos*/)
    {
        MarkerPtr m(mWptr);
        if (!m) { return; }
        editMarkerText(m);
    });

    mMapWidget->addItem(marker->mapAnnotation);
    mMarkers.append(marker);

    return marker;
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

void MapPlot::Crosshair::setPosition(QGV::GeoPos geoPos, QPoint pixelPos, int index)
{
    marker->setPosition(geoPos);

    QString text = QString("%1, %2")
            .arg(MapPlot::formatLatLon(geoPos.latitude()))
            .arg(MapPlot::formatLatLon(geoPos.longitude()));
    if (index >= 0) {
        text = QString("%1 [%2]").arg(text).arg(index);
    }
    label->setText(text);

    static const int offset = 5;
    label->move(QPoint(pixelPos.x() + offset,
                       pixelPos.y() - label->height() - offset));

    lines->setPos(pixelPos);
}

void MapPlot::Crosshair::setPosition(QPointF projPos, QPoint pixelPos, int index)
{
    QGV::GeoPos geoPos = mMapWidget->getProjection()->projToGeo(projPos);
    setPosition(geoPos, pixelPos, index);
}

void MapPlot::Crosshair::setPosition(QPointF projPos, int index)
{
    QPoint pixelPos = mMapWidget->mapFromProj(projPos);
    setPosition(projPos, pixelPos, index);
}

void MapPlot::Crosshair::setPosition(QGV::GeoPos geoPos, int index)
{
    QPointF projPos = mMapWidget->getProjection()->geoToProj(geoPos);
    QPoint pixelPos = mMapWidget->mapFromProj(projPos);
    setPosition(geoPos, pixelPos, index);
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
