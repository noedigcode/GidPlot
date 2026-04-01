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

/* QGVLine
 *
 * Draws a line on a QGeoView map.
 *
 */

#pragma once

#include <QGVDrawItem.h>

#include <QBrush>
#include <QPainter>
#include <QPen>

class QGVLine : public QGVDrawItem
{
    Q_OBJECT

public:
    explicit QGVLine(const QGV::GeoPos &pos1, const QGV::GeoPos &pos2);
    explicit QGVLine(const QList<QGV::GeoPos> &posList);

    QColor color();
    void setColor(QColor color);
    void setWidth(int width);
    void setPen(QPen pen);

private:
    void setup();

    void onProjection(QGVMap* geoMap) override;
    void onUpdate() override;
    QPainterPath projShape() const override;
    void projPaint(QPainter* painter) override;
    QPointF projAnchor() const override;

    // Start and end coordinates of line
    QList<QGV::GeoPos> mPosList;
    // Projected points of line on to painting area
    QList<QPointF> mProjPosList;

    QPen mPen {Qt::red};
};
