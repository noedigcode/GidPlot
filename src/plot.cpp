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

#include "plot.h"


Plot::Plot(QWidget *parentWidget)
    : QObject{parentWidget}, mParentWidget(parentWidget)
{
    setupPlotMenu();
    setupCrosshairsDialog();
}

void Plot::storeAndDisableCrosshairs()
{
    mLastPlotCrosshairVisible = plotCrosshairVisible();
    setPlotCrosshairVisible(false);
    mLastMouseCrosshairVisible = mouseCrosshairVisible();
    setMouseCrosshairVisible(false);
}

void Plot::restoreCrosshairs()
{
    setPlotCrosshairVisible(mLastPlotCrosshairVisible);
    setMouseCrosshairVisible(mLastMouseCrosshairVisible);
}

void Plot::expandBounds(QRectF otherDataBounds)
{
    bounds = bounds.united(otherDataBounds);
}

void Plot::setupPlotMenu()
{
    plotMenu.parentWidget = mParentWidget;
    plotMenu.getDataTipGraphCallback = [this]() { return dataTipGraph; };
    plotMenu.getGraphsCallback = [this]() { return graphs; };
    plotMenu.getPlotCrosshairIndexCallback = [=]()
    {
        return mPlotCrosshairIndex;
    };
    connect(&plotMenu, &PlotMenu::dataTipGraphSelected,
            this, &Plot::onActionDataTipGraphSelected);

    connect(plotMenu.actionPlaceMarker, &QAction::triggered,
            this, &Plot::onActionPlaceMarkerTriggered);
    connect(plotMenu.actionMeasure, &QAction::triggered,
            this, &Plot::onActionMeasureTriggered);
    connect(plotMenu.actionShowAll, &QAction::triggered,
            this, &Plot::showAll);
    connect(plotMenu.actionEqualAxes, &QAction::triggered,
            this, &Plot::onActionEqualAxesTriggered);
    connect(plotMenu.actionCrosshairs, &QAction::triggered,
            this, &Plot::showCrosshairsDialog);
    connect(plotMenu.actionLink, &QAction::triggered,
            this, &Plot::linkSettingsTriggered);
}

void Plot::onActionDataTipGraphSelected(GraphPtr graph)
{
    dataTipGraph = graph;
}

void Plot::showCrosshairsDialog()
{
    mCrosshairsDialog.show(crosshairsDialogAboutToShow());
}

void Plot::setupCrosshairsDialog()
{
    connect(&mCrosshairsDialog, &CrosshairsDialog::settingsChanged,
            this, &Plot::crosshairsDialogChanged);
}

Plot::ClosestCoord Plot::findClosestCoord(QPoint mousePos, GraphPtr graph,
                                          ClosestOption closestOption)
{
    ClosestCoord closest;

    int px = mClosestCoordStartSizePx;

    while (!closest.valid) {

        // Create a rectangle around the mouse position.
        // (This starts out small and grows with each iteration of the loop
        //  until a closest coordinate is found.)
        QRect mouseRect(QPoint(), QSize(px, px));
        mouseRect.moveCenter(mousePos);

        // Transform the mouse rectangle from pixel space to plot space,
        // forming a polygon.
        QPolygonF searchPoly({
                                 pixelPosToCoord(mouseRect.topLeft()),
                                 pixelPosToCoord(mouseRect.topRight()),
                                 pixelPosToCoord(mouseRect.bottomRight()),
                                 pixelPosToCoord(mouseRect.bottomLeft())
                             });
        // Get the bounding rectangle of the polygon which will then be
        // used to look up coordinates in the grid hash.
        QRectF searchRect = searchPoly.boundingRect();

        GridHash::Result r = graph->grid.get(searchRect, closestOption);

        // Use a distance limit effectively only accept coordinates in a
        // circle radius around the mouse position. (Otherwise we could
        // potentially leave out closer points depending on the plot
        // transform.)
        double limitDistSqr = px*px;

        double closestDistSqr = 0;
        bool first = true;
        foreach (GridHash::Data d, r.data) {
            QPoint pixelPos = coordToPixelPos(d.coord);
            double distSqr;
            if (closestOption == ClosestXY) {
                distSqr = qPow(mousePos.x() - pixelPos.x(), 2)
                        + qPow(mousePos.y() - pixelPos.y(), 2);
            } else {
                distSqr = qPow(mousePos.x() - pixelPos.x(), 2);
            }
            if ( (first || (distSqr < closestDistSqr)) && (distSqr <= limitDistSqr) ) {
                first = false;
                closestDistSqr = distSqr;
                closest.coord = d.coord;
                closest.dataIndex = d.index;
                closest.valid = true;
            }
        }

        if (r.maxedOut) { break; }

        // Next iteration, if no coord found, try again with larger area
        px *= 2;
    }

    return closest;
}

