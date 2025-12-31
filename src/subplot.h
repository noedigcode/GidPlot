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

// ===========================================================================

struct Marker
{
    QString datasetName;
    int dataIndex = 0;
    double xCoord = 0;
    double yCoord = 0;
    PlotMarkerItem* plotMarker = nullptr;
    QCPItemText* textItem = nullptr;
    QCPItemLine* arrow = nullptr;
    QString text;
};
typedef QSharedPointer<Marker> MarkerPtr;

// ===========================================================================

struct Measure
{
    MarkerPtr a;
    MarkerPtr b;
    QString tag;
};
typedef QSharedPointer<Measure> MeasurePtr;

// ===========================================================================

class Subplot;
typedef QSharedPointer<Subplot> SubplotPtr;

class Subplot : public Plot
{
    Q_OBJECT
public:
    explicit Subplot(QCPAxisRect* axisRect, QWidget *parentWidget);

    static SubplotPtr newAtBottomOfPlot(QCustomPlot* plot, QWidget* parentWidget);
    static SubplotPtr castFromPlot(PlotPtr plot);

    static QCPLegend* findLegend(QCPAxisRect* axisRect);
    bool inAxisRect(QPoint pos);

    void plot(CsvPtr csv, int ixcol, int iycol, Range range);
    void setXLabel(QString xlabel);
    void setYLabel(QString ylabel);

    bool mouseCrosshairVisible();
    void setMouseCrosshairVisible(bool visible);
    bool plotCrosshairVisible();
    void setPlotCrosshairVisible(bool visible);
    void resized();
    void showAll();
    void syncAxisRanges(QRectF xyrange);
    void syncDataTip(int index);

    void showEvent();
    void keyEvent(QEvent* event);

private:
    QCustomPlot* mPlot = nullptr;
    QCPAxisRect* axisRect = nullptr;
    QCPAxis* xAxis = nullptr;
    QCPAxis* yAxis = nullptr;
    QCPLegend* legend = nullptr;
    void queueReplot();

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

    MarkerEditDialog mMarkerEditDialog;

    CrosshairsDialog::Settings crosshairsDialogAboutToShow();
    void crosshairsDialogChanged(CrosshairsDialog::Settings settings);

    // -----------------------------------------------------------------------
    // Menus
private:
    void setupMenus();
private slots:
    void onActionPlaceMarkerTriggered();
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
    void removeGraph(GraphPtr graph);

    // -------------------------------------------------------------------------
    // Markers

    QList<MarkerPtr> mMarkers; // Markers are kept top (last added) to bottom

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
    void clearCurrentMeasure();
    int mMeasureCounter = 1;

    // -------------------------------------------------------------------------
    // Crosshairs

    void setupCrosshairs();
    enum ClosestOption { ClosestXOnly, ClosestXY } mPlotCrosshairSnap = ClosestXOnly;
    PlotMarkerItem* mPlotCrosshair = nullptr;

    bool mPlotCrosshairVisibilityChangedByUser = false;
    PlotMarkerItem* mMouseCrosshair = nullptr;
    void updateGuiForCrosshairOptions();

    // TODO TESTING
    struct Grid
    {
        QRectF bounds;
        int n = 100;

        int maxcolrow()
        {
            return n - 1;
        }

        struct Data
        {
            QPointF coord;
            int index = 0;
        };

        QMultiMap<int, Data> map;

        int hash(QPoint point)
        {
            return (point.x() + 1) * n + point.y();
        }

        QPoint colrow(QPointF coord)
        {
            double max = qMax(bounds.width(), bounds.height());

            int col = (coord.x() - bounds.left()) / max * n;
            int row = (coord.y() - bounds.top()) / max * n;

            return QPoint(col, row);
        }

        int clipcolrow(int value)
        {
            return (qMax(qMin(value, maxcolrow()), 0));
        }

        void insert(QPointF coord, int index)
        {
            QPoint point = colrow(coord);

            point.setX(clipcolrow(point.x()));
            point.setY(clipcolrow(point.y()));

            map.insert(hash(point), Data{coord, index});
        }

        struct Result
        {
            QList<Data> data;
            bool maxedOut = false;
        };

        Result get(QRectF rect, ClosestOption option)
        {
            Result ret;

            QPoint start = colrow(rect.topLeft());
            QPoint end = colrow(rect.bottomRight());

            int colstart = clipcolrow(start.x());
            int colend = clipcolrow(end.x());
            int rowstart;
            int rowend;
            if (option == ClosestXOnly) {
                rowstart = 0;
                rowend = maxcolrow();
            } else {
                rowstart = clipcolrow(start.y());
                rowend = clipcolrow(end.y());
            }

            for (int col = colstart; col <= colend; col++) {
                for (int row = rowstart; row <= rowend; row++) {
                    QList<Data> vals = map.values(hash(QPoint(col, row)));
                    ret.data.append(vals);
                }
            }

            bool atmax = true;
            atmax &= (colstart == 0);
            atmax &= (colend == maxcolrow());
            atmax &= (rowstart == 0);
            atmax &= (rowend == maxcolrow());
            ret.maxedOut = atmax;

            return ret;
        }

        QList<Data> get(QPointF coord)
        {
            QPoint point = colrow(coord);
            int colstart = clipcolrow(point.x() - 1);
            int colend = clipcolrow(point.x() + 1);
            int rowstart = clipcolrow(point.y() - 1);
            int rowend = clipcolrow(point.y() + 1);

            //qDebug() << "Get" << coord ;

            QList<Data> ret;

            while (ret.isEmpty()) {

                //qDebug() << "    Trying (" << colstart << "," << rowstart << ") to (" << colend << "," << rowend <<")";

                for (int col = colstart; col <= colend; col++) {
                    for (int row = rowstart; row <= rowend; row++) {
                        QList<Data> vals = map.values(hash(QPoint(col, row)));
                        //qDebug() << "    Got" << vals.count() << "values in cell" << col << row;
                        ret.append(vals);
                    }
                }

                if (!ret.isEmpty()) { break; }

                // Expand outwards

                bool atmax = true;
                atmax &= (colstart == 0);
                atmax &= (colend == n - 1);
                atmax &= (rowstart == 0);
                atmax &= (rowend == n - 1);
                if (atmax) {
                    //qDebug() << "    Expanded to max. Stopping.";
                    break;
                }

                //qDebug() << "    Expanding";
                colstart = clipcolrow(colstart - 1);
                colend = clipcolrow(colend + 1);
                rowstart = clipcolrow(rowstart - 1);
                rowend = clipcolrow(rowend + 1);

            }

            //qDebug() << "    Total" << ret.count() << "values around point";

            return ret;
        }
    } grid;

    struct ClosestCoord {
        bool valid = false;
        int distancePixels = 0;
        double xCoord = 0;
        double yCoord = 0;
        int dataIndex = 0;
    };
    ClosestCoord findClosestCoord(QPoint pos, GraphPtr graph,
                                  ClosestOption closestOption);
    QPointF pixelPosToCoord(QPoint pos)
    {
        return QPointF(xAxis->pixelToCoord(pos.x()),
                       yAxis->pixelToCoord(pos.y()));
    }
    QPoint coordToPixelPos(QPointF coord)
    {
        return QPoint(xAxis->coordToPixel(coord.x()),
                      yAxis->coordToPixel(coord.y()));
    }

    bool lastPlotCrosshairVisible = false;
    bool lastMouseCrosshairVisible = false;

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
