/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2024 Gideon van der Kolf
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

#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include "csv.h"
#include "crosshair.h"
#include "utils.h"

#include "QCustomPlot/qcustomplot.h"

#include <QWidget>

// =============================================================================

/* Graph provides a unified interface for either a QCPGraph or QCPCurve, either
 * of which can be used to plot data depending on whether it is monotonically
 * increasing or not. */
struct Graph {

    Graph(QCPGraph* graph) : graph(graph) {}
    Graph(QCPCurve* curve) : curve(curve) {}

    QCPCurve* curve = nullptr;
    QCPGraph* graph = nullptr;

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

// =============================================================================

namespace Ui {
class PlotWidget;
}

class PlotWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PlotWidget(QWidget *parent = 0);
    ~PlotWidget();

    enum Dock {
        DockTop, DockBottom, DockLeft, DockRight, DockFloating, DockTab
    };

    QCustomPlot* plotWidget();
    void plotData(CsvPtr csv, int ixcol, int iycol, Range range);
    void showAll();

    void setEqualAxesButDontReplot(bool fixed);
    void setEqualAxesAndReplot(bool fixed);

    void syncAxisRanges(QRectF xyrange);
    void syncDataTip(int index);

signals:
    void linkGroupChanged(int group);
    void axisRangesChanged(QRectF xyrange);
    void dataTipChanged(int index);
    void dockWindow(Dock location);

private slots:
    void plottableClick(QCPAbstractPlottable *plottable, int dataIndex, QMouseEvent *event);
    void onPlotMouseMove(QMouseEvent* event);
    void onPlotMousePress(QMouseEvent* event);
    bool plotMouseRightDrag(QMouseEvent* event);
    bool plotMouseMove(QMouseEvent* event);
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

    void on_pushButton_view_clicked();
    void on_pushButton_link_clicked();
    void on_pushButton_window_clicked();

    void on_action_Dock_to_Screen_Top_triggered();
    void on_action_Dock_to_Screen_Bottom_triggered();
    void on_action_Dock_to_Screen_Left_triggered();
    void on_action_Dock_to_Screen_Right_triggered();
    void on_action_Undocked_triggered();
    void on_action_Tab_in_Main_Window_triggered();

private:
    Ui::PlotWidget *ui;

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

    QMenu rangeMenu;
    QMenu plotContextMenu;
    QMenu viewMenu;
    QMenu linkMenu;
    QMenu windowMenu;
    QMenu dataTipMenu;
    void setupMenus();

    // -------------------------------------------------------------------------

    // Mouse drag zoom
    struct {
        QPoint start;
        QCPRange origXrange;
        QCPRange origYrange;
        bool mouseDown = false;
        bool isDragging = false;
    } rMouseZoom;

    bool lMouseDown = false;

    GraphPtr dataTipGraph;
    QList<GraphPtr> graphs;

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
    Crosshair mMouseCrosshair;
    void updateGuiForCrosshairOptions();

    // -------------------------------------------------------------------------

    int mLinkGroup = 0;
    void setLinkGroup(int group);
    void updateGuiForLinkGroup();

    bool eventFilter(QObject *watched, QEvent *event);
    void resizeEvent(QResizeEvent* event) override;

    bool mRangesChanged = false;
    bool mRangesSyncedFromOutside = false;
};

#endif // PLOTWIDGET_H
