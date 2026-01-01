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

#ifndef PLOT_H
#define PLOT_H

#include "link.h"
#include "graph.h"
#include "CrosshairsDialog.h"

#include <QObject>
#include <QWidget>

class Plot : public QObject
{
    Q_OBJECT
public:
    explicit Plot(QWidget *parentWidget = nullptr);
    virtual ~Plot() = default;

    LinkPtr link {new Link()};

    virtual bool mouseCrosshairVisible() = 0;
    virtual void setMouseCrosshairVisible(bool visible) = 0;
    virtual bool plotCrosshairVisible() = 0;
    virtual void setPlotCrosshairVisible(bool visible) = 0;
    virtual void resized() = 0;
    virtual void showAll() = 0;
    virtual void syncAxisRanges(QRectF xyrange) = 0;
    virtual void syncDataTip(int index) = 0;

    void storeAndDisableCrosshairs();
    void restoreCrosshairs();

signals:
    void axisRangesChanged(int linkGroup, QRectF xyrange);
    void dataTipChanged(int linkGroup, int index);
    void linkSettingsTriggered();

protected:
    QWidget* mParentWidget = nullptr;

    QRectF bounds;
    void expandBounds(QRectF otherDataBounds);

    // -----------------------------------------------------------------------
    // Menus
protected:
    PlotMenu plotMenu;
protected slots:
    virtual void onActionPlaceMarkerTriggered() = 0;
    virtual void onActionMeasureTriggered() = 0;
    virtual void onActionEqualAxesTriggered() = 0;

    // -----------------------------------------------------------------------
    // Crosshairs
protected:
    CrosshairsDialog mCrosshairsDialog;
    int mPlotCrosshairIndex = 0;
    ClosestOption mPlotCrosshairSnap = ClosestXOnly;
    void showCrosshairsDialog();
    virtual CrosshairsDialog::Settings crosshairsDialogAboutToShow() = 0;
    virtual void crosshairsDialogChanged(CrosshairsDialog::Settings s) = 0;

private:
    void setupCrosshairsDialog();
    bool mLastPlotCrosshairVisible = false;
    bool mLastMouseCrosshairVisible = false;

    // -----------------------------------------------------------------------
    // Graphs
protected:
    GraphPtr dataTipGraph;
    QList<GraphPtr> graphs;

    virtual QPointF pixelPosToCoord(QPoint pos) = 0;
    virtual QPoint coordToPixelPos(QPointF coord) = 0;

    struct ClosestCoord {
        bool valid = false;
        QPointF coord;
        int dataIndex = 0;
    };

    int mClosestCoordStartSizePx = 50;
    ClosestCoord findClosestCoord(QPoint mousePos, GraphPtr graph,
                                  ClosestOption closestOption);

private:
    void setupPlotMenu();
private slots:
    void onActionDataTipGraphSelected(GraphPtr graph);
};

typedef QSharedPointer<Plot> PlotPtr;

#endif // PLOT_H
