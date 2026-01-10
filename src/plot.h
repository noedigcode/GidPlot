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

#include <QObject>
#include <QWidget>

class Plot : public QObject
{
    Q_OBJECT
public:
    explicit Plot(QWidget *parentWidget = nullptr);
    virtual ~Plot() = default;

    struct Properties
    {
        bool plotCrosshair = false;
        bool plotHline = false;
        bool plotVline = false;
        bool plotDot = false;
        bool mouseCrosshair = false;
        bool mouseHline = false;
        bool mouseVline = false;
        bool mouseDot = false;

        bool supportShowTitle = true;
        bool supportXlabel = true;
        bool supportYlabel = true;

        bool showTitle = true;
        bool showXlabel = true;
        bool showYlabel = true;
        QString title;
        QString xlabel;
        QString ylabel;
        bool showLegend = false;
    };
    virtual Properties getPlotProperties() = 0;
    virtual void setPlotProperties(Properties p) = 0;

    LinkPtr link {new Link()};

    QList<GraphPtr> graphs();
    virtual void renameGraph(GraphPtr graph, QString name) = 0;
    virtual void setGraphColor(GraphPtr graph, QColor color) = 0;
    virtual void removeGraph(GraphPtr graph) = 0;

    QString title();
    void setTitle(QString title, bool visible = true);

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
    void requestShowPlotProperties();
    void titleChanged(QString title, bool visible);

protected:
    QWidget* mParentWidget = nullptr;

    QString mTitle;
    bool mTitleVisible = true;

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
    int mPlotCrosshairIndex = 0;
    ClosestOption mPlotCrosshairSnap = ClosestXOnly;

private:
    bool mLastPlotCrosshairVisible = false;
    bool mLastMouseCrosshairVisible = false;

    // -----------------------------------------------------------------------
    // Graphs
protected:
    GraphPtr dataTipGraph;
    QList<GraphPtr> mGraphs;

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
