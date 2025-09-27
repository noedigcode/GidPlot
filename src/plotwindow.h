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

#ifndef PLOTWINDOW_H
#define PLOTWINDOW_H

#include "csv.h"
#include "crosshair.h"
#include "utils.h"

#include "QCustomPlot/qcustomplot.h"

#include <QMainWindow>

namespace Ui {
class PlotWindow;
}

class PlotWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlotWindow(int tag = 0, QWidget *parent = nullptr);
    ~PlotWindow();

    // =========================================================================

    /* Graph provides a unified interface for either a QCPGraph or QCPCurve,
     * either of which can be used to plot data depending on whether it is
     * monotonically increasing (QCPGraph) or not (QCPCurve). */
    struct Graph {

        Graph(QCPGraph* graph) : graph(graph) {}
        Graph(QCPCurve* curve) : curve(curve) {}

        QCPCurve* curve = nullptr;
        QCPGraph* graph = nullptr;
        QCPAbstractPlottable* plottable();

        CsvPtr csv;
        Range range;

        bool isCurve();
        bool isGraph();
        QString name();
        int dataCount();
        double datax(int index);
        double datay(int index);
        QColor color();
    };
    typedef QSharedPointer<Graph> GraphPtr;

    // =========================================================================

    enum Dock {
        DockTop, DockBottom, DockLeft, DockRight, DockFloating, DockTab
    };

    int tag();
    QCustomPlot* plotWidget();
    void plotData(CsvPtr csv, int ixcol, int iycol, Range range);
    void setTitle(QString title);
    void setXLabel(QString xlabel);
    void setYLabel(QString ylabel);
    void showAll();

    void setEqualAxesButDontReplot(bool fixed);
    void setEqualAxesAndReplot(bool fixed);

    void syncAxisRanges(QRectF xyrange);
    void syncDataTip(int index);

signals:
    void linkGroupChanged(int group);
    void axisRangesChanged(QRectF xyrange);
    void dataTipChanged(int index);
    void requestWindowDock(Dock location);
    void requestWindowResize(int width, int height);
    void titleSet(QString title);

private slots:
    void plottableClick(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    void onPlotMouseMove(QMouseEvent* event);
    void onPlotMousePress(QMouseEvent* event);
    void onPlotDoubleClick(QMouseEvent* event);
    void onAxisDoubleClick(QCPAxis *axis, QCPAxis::SelectablePart part, QMouseEvent *event);
    void onTitleDoubleClick(QMouseEvent* event);
    bool plotMouseRightDrag(QMouseEvent* event);
    bool plotMouseMove(QMouseEvent* event);
    bool legendMouseMove(QMouseEvent* event);
    void onLegendItemRightClicked(QCPPlottableLegendItem* legendItem, const QPoint& pos);
    void onLegendRightClicked(QCPLegend* legend, const QPoint& pos);
    void onPlotMouseRelease(QMouseEvent* event);
    void plotRightClicked(const QPoint &pos);

    void onAxisRangesChanged();

    void on_action_Equal_Axes_triggered();
    void on_action_Show_All_triggered();

    void on_action_No_Link_triggered();
    void on_action_Link_to_Group_1_triggered();
    void on_action_Link_to_Group_2_triggered();
    void on_action_Link_to_Group_3_triggered();

    void on_action_Show_Plot_Crosshair_triggered();
    void on_action_Show_Mouse_Crosshair_triggered();
    void on_action_PlotCrosshair_Horizontal_Line_triggered();
    void on_action_PlotCrosshair_Vertical_Line_triggered();
    void on_action_PlotCrosshair_Dot_triggered();

    void on_action_MouseCrosshair_Horizontal_Line_triggered();
    void on_action_MouseCrosshair_Vertical_Line_triggered();
    void on_action_MouseCrosshair_Dot_triggered();

    void on_action_Dock_to_Screen_Top_triggered();
    void on_action_Dock_to_Screen_Bottom_triggered();
    void on_action_Dock_to_Screen_Left_triggered();
    void on_action_Dock_to_Screen_Right_triggered();
    void on_action_Undocked_triggered();
    void on_action_Tab_in_Main_Window_triggered();

    void on_action_Copy_Image_triggered();
    void on_action_Resize_Plot_triggered();

private:
    Ui::PlotWindow *ui;
    int mTag = 0;
    QCPTextElement* mPlotTitle = nullptr;

    void queueReplot();

    QList<QPen> pens {
        QPen(Qt::blue),
        QPen(Qt::red),
        QPen(Qt::green),
        QPen(Qt::cyan),
        QPen(Qt::magenta),
        QPen(Qt::darkRed),
    };

    bool mEqualAxes = false;
    void updatePlotForEqualAxes(QRectF xyrange);

    // -------------------------------------------------------------------------
    // Menus
private:
    QMenu rangeMenu;
    QMenu plotContextMenu;
    QMenu dataTipMenu;
    void setupMenus();
private slots:
    void onRangeMenuAboutToShow();
    void onDataTipMenuAboutToShow();

    // -------------------------------------------------------------------------
    // Mouse drag zoom
private:
    QPoint mLastMouseMovePos;

    struct {
        QPoint start;
        QCPRange origXrange;
        QCPRange origYrange;
        bool mouseDown = false;
        bool isDragging = false;
    } rMouseZoom;

    bool lMouseDown = false;
    QPoint lMouseStart;

    bool lMouseDownOnLegend = false;
    QRectF mLegendStartRect;

    bool lMouseDownOnMarker = false;

    bool mFirstLegendPlacement = true;
    void updateLegendPlacement();

    GraphPtr dataTipGraph;
    QList<GraphPtr> graphs;
    QMap<QCPAbstractPlottable*, GraphPtr> plottableGraphMap;
    void removeGraph(GraphPtr graph);

    // -------------------------------------------------------------------------

    double xmin = 0;
    double xmax = 0;
    double ymin = 0;
    double ymax = 0;

    // -------------------------------------------------------------------------
    // Crosshairs

    void setupCrosshairs();
    enum CrosshairSnap { SnapXOnly, SnapToClosest } mPlotCrosshairSnap = SnapXOnly;
    Crosshair mPlotCrosshair;
    int mPlotCrosshairIndex = 0;
    bool mPlotCrosshairVisibilityChangedByUser = false;
    Crosshair mMouseCrosshair;
    void updateGuiForCrosshairOptions();

    struct ClosestCoord {
        bool valid = false;
        int distancePixels = 0;
        double xCoord = 0;
        double yCoord = 0;
        int dataIndex = 0;
    };
    ClosestCoord findClosestCoord(QPoint pos, GraphPtr graph, CrosshairSnap snap);

    // -------------------------------------------------------------------------

    int mLinkGroup = 0;
    void setLinkGroup(int group);
    void updateGuiForLinkGroup();

    bool eventFilter(QObject *watched, QEvent *event);
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

    bool mRangesChanged = false;
    bool mRangesSyncedFromOutside = false;

    // -------------------------------------------------------------------------
    // Markers

    struct Marker
    {
        int dataIndex = 0;
        double xCoord = 0;
        double yCoord = 0;
        QCPItemTracer* tracer = nullptr;
        QCPItemText* textItem = nullptr;
        QCPItemLine* arrow = nullptr;
        QString text;
    };
    typedef QSharedPointer<Marker> MarkerPtr;

    QList<MarkerPtr> mMarkers;

    MarkerPtr findMarkerUnderPos(QPoint pos);
    MarkerPtr mouseDownMarker;
    QPointF markerTextPixelPosAtMouseDown;
    bool markerMouseDown(QMouseEvent* mouseEvent);
    bool markerMouseMove(QMouseEvent* mouseEvent);
    void markerMouseUp();
    bool markerRightClick(QPoint pos);
    void updateMarkerArrow(MarkerPtr marker);
    void updateMarkerText(MarkerPtr marker);

private slots:
    void on_action_Place_Marker_triggered();
};

#endif // PLOTWINDOW_H
