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

#include "MarkerEditDialog.h"
#include "PlotMarkerItem.h"
#include "csv.h"
#include "utils.h"
#include "subplot.h"

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

    enum Dock {
        DockTop, DockBottom, DockLeft, DockRight, DockFloating, DockTab
    };

    int tag();
    QList<SubplotPtr> subplots();
    QCustomPlot* plotWidget();
    void plotData(CsvPtr csv, int ixcol, int iycol, Range range);
    void plotData(SubplotPtr subplot, CsvPtr csv, int ixcol, int iycol, Range range);
    SubplotPtr addSubplot();
    void setTitle(QString title);
    void setXLabel(QString xlabel);
    void setYLabel(QString ylabel);
    void showAll();

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
    void onPlotItemDoubleClick(QCPAbstractItem *item, QMouseEvent *event);
    void onTitleDoubleClick(QMouseEvent* event);

    void onPlotMouseRelease(QMouseEvent* event);

    void onAxisRangesChanged(SubplotPtr subplot, QRectF xyrange);
    void onDataTipChanged(SubplotPtr subplot, int index);

    void on_action_No_Link_triggered();
    void on_action_Link_to_Group_1_triggered();
    void on_action_Link_to_Group_2_triggered();
    void on_action_Link_to_Group_3_triggered();

    void on_action_Dock_to_Screen_Top_triggered();
    void on_action_Dock_to_Screen_Bottom_triggered();
    void on_action_Dock_to_Screen_Left_triggered();
    void on_action_Dock_to_Screen_Right_triggered();
    void on_action_Undocked_triggered();
    void on_action_Tab_in_Main_Window_triggered();

    void on_action_Resize_Plot_triggered();

private:
    Ui::PlotWindow *ui;

    int mTag = 0;
    QFont mPlotFont;
    void setPlotFont(QFont font);

    QCPTextElement* mPlotTitle = nullptr;

    QByteArray plotToSvg();

    // -------------------------------------------------------------------------
    // Mouse drag zoom
private:
    struct Mouse {
        QPoint lastMovePos;
        Qt::MouseButton button;
        QCPAxisRect* axisRect = nullptr;
        QCPAxis* xAxis = nullptr;
        QCPAxis* yAxis = nullptr;
        QPoint start;
        QCPRange startXrange;
        QCPRange startYrange;
        bool mouseDown = false;
        bool isDragging = false;
    } mouse;

    struct {
        QRectF startRect;
        bool mouseDown = false;
        QCPLegend* legend = nullptr;
    } legendDrag;
    QCPLegend* findLegendInAxisRect(QCPAxisRect* axisRect);
    QCPLegend* findLegendUnderPos(QPoint pos);

    // -------------------------------------------------------------------------

    int mLinkGroup = 0;
    void setLinkGroup(int group);
    void updateGuiForLinkGroup();

    bool eventFilter(QObject *watched, QEvent *event);
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

    // -------------------------------------------------------------------------
    // Subplots

    QList<SubplotPtr> mSubplots;
    void initSubplot(SubplotPtr subplot);
    void storeAndDisableCrosshairsOfAllSubplots();
    void restoreCrosshairsOfAllSubplots();

private slots:
    void on_action_Save_as_PDF_triggered();
    void on_action_Copy_PNG_triggered();
    void on_action_Copy_SVG_triggered();
    void on_action_Save_as_PNG_triggered();
    void on_action_Save_as_SVG_triggered();
    void on_action_Test_1_triggered();
};

#endif // PLOTWINDOW_H
