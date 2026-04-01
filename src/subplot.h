/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2025 Gideon van der Kolf
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

#ifndef SUBPLOT_H
#define SUBPLOT_H

#include "csv.h"
#include "graph.h"
#include "plot.h"
#include "PlotMarkerItem.h"
#include "MarkerEditDialog.h"

#include "QCustomPlot/qcustomplot.h"

#include <QObject>
#include <QMultiMap>


class Subplot;
typedef QSharedPointer<Subplot> SubplotPtr;

class Subplot : public Plot
{
    Q_OBJECT
public:

    // -----------------------------------------------------------------------

    struct Marker
    {
        QString datasetName;
        int dataIndex = -1;
        double xCoord = 0;
        double yCoord = 0;
        PlotMarkerItem* plotMarker = nullptr;
        QCPItemText* textItem = nullptr;
        QCPItemLine* arrow = nullptr;
        QString text;
    };
    typedef QSharedPointer<Marker> MarkerPtr;

    // -----------------------------------------------------------------------

    struct Measure
    {
        MarkerPtr a;
        MarkerPtr b;
        QString tag;
    };
    typedef QSharedPointer<Measure> MeasurePtr;

    // -----------------------------------------------------------------------

    explicit Subplot(QCPAxisRect* axisRect, QWidget *parentWidget);

    static SubplotPtr newAtBottomOfPlot(QCustomPlot* plot, QWidget* parentWidget);
    static SubplotPtr castFromPlot(PlotPtr plot);

    static QCPLegend* findLegend(QCPAxisRect* axisRect);
    bool inAxisRect(QPoint pos);

    void plot(CsvPtr csv, int ixcol, int iycol, Range range);
    void setXLabel(QString xlabel, bool visible = true);
    void setXLabelVisible(bool visible);
    void setYLabel(QString ylabel, bool visible = true);
    void setYLabelVisible(bool visible);

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

    void keyEvent(QEvent* event);

private:
    QCustomPlot* mPlot = nullptr;
    QCPAxisRect* axisRect = nullptr;
    QCPAxis* xAxis = nullptr;
    QCPAxis* yAxis = nullptr;
    void queueReplot();

    QString mXlabel;
    bool mXlabelVisible = false;
    QString mYlabel;
    bool mYlabelVisible = false;

    void setupLink();

    // Keep count of current pen index when adding new plots instead of simply
    // using (plottables.count % pens.count), so when a plottable has been
    // removed and a new one is added, it doesn't get the same colour as an
    // existing plottable (unless there are more plottables than pens).
    int mPenIndex = 0;

    bool mEqualAxes = false;
    void updatePlotForEqualAxes(QRectF xyrange);
    void setEqualAxesButDontReplot(bool fixed);
    void setEqualAxesAndReplot(bool fixed);

    bool plotMouseRightDragZoom(QMouseEvent* event);

    bool mRangesChanged = false;
    bool mRangesSyncedFromOutside = false;

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
    // Mouse drag zoom
private:

    struct Mouse {
        QPoint lastMovePos;
        Qt::MouseButton button;
        QPoint start;
        QPoint startRelativeToAxisRect;
        QCPRange startXrange;
        QCPRange startYrange;
        bool mouseDown = false;
        bool isDragging = false;
    } mouse;

    bool plotMouseMove(QMouseEvent* event);

    // -------------------------------------------------------------------------
    // Legend

    QCPLegend* legend = nullptr;
    bool mShowLegend = true;
    bool mAutoShowLegend = true;

    struct LegendMouse {
        bool mouseDown = false;
        QRectF startRect;
    } legendMouse;

    bool legendMouseMove(QMouseEvent* event);

    bool mFirstLegendPlacement = true;
    void updateLegendPlacement();

    // -------------------------------------------------------------------------
    // Graphs / plottables

    QMap<QCPAbstractPlottable*, GraphPtr> plottableGraphMap;
    void setDataTipGraph(GraphPtr graph);

    // -------------------------------------------------------------------------
    // Markers

    QList<MarkerPtr> mMarkers; // Markers are kept top (last added) to bottom

    MarkerEditDialog mMarkerEditDialog;
    void setupMarkerEditDialog();

    MarkerPtr addMarker(QPointF coord);

    struct MarkerMouse {
        bool mouseDown = false;
        MarkerPtr marker;
        QPointF startTextPixelPos;
    } markerMouse;

    bool markerMouseDown(QMouseEvent* mouseEvent);
    bool markerMouseMove(QMouseEvent* mouseEvent);
    void markerMouseUp();

    MarkerPtr findMarkerUnderPos(QPoint pos);

    bool markerRightClick(QPoint pos);
    void updateMarkerArrow(MarkerPtr marker);
    void updateMarkerText(MarkerPtr marker);
    void editMarkerText(MarkerPtr marker);
    void deleteMarker(MarkerPtr marker);

    // -------------------------------------------------------------------------
    // Measures

    QList<MeasurePtr> mMeasures;
    MeasurePtr mCurrentMeasure;
    MarkerPtr newMeasureMarker(double x, double y, QString text);
    void startMeasure(MeasurePtr meas);
    void clearCurrentMeasure();
    int mMeasureCounter = 1;

    // -------------------------------------------------------------------------
    // Crosshairs

    void setupCrosshairs();

    bool mPlotCrosshairVisible = true;
    PlotMarkerItem* mPlotCrosshair = nullptr;
    bool mPlotCrosshairVisibilityChangedByUser = false;

    PlotMarkerItem* mMouseCrosshair = nullptr;

    QPointF pixelPosToCoord(QPoint pos);
    QPoint coordToPixelPos(QPointF coord);

private slots:
    void onPlotMousePress(QMouseEvent* event);
    void onPlotMouseMove(QMouseEvent* event);
    void onPlotMouseRelease(QMouseEvent* event);
    void plotRightClicked(const QPoint &pos);
    void plotLeftClicked(const QPoint &pos);
    void onAxisRangesChanged();
    void onAxisDoubleClick(QCPAxis* axis, QCPAxis::SelectablePart part,
                           QMouseEvent* event);
    void onPlotItemDoubleClick(QCPAbstractItem* item, QMouseEvent* event);
    void onLegendItemRightClicked(QCPPlottableLegendItem* legendItem,
                                  const QPoint &pos);
    void onLegendRightClicked(QCPLegend* legend, const QPoint& pos);
};

#endif // SUBPLOT_H
