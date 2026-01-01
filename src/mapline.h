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

/* MapLine
 *
 * Draws a line on a QGeoView map.
 *
 */

#pragma once

#include <QGVDrawItem.h>

#include <QBrush>
#include <QPainter>
#include <QPen>

class MapLine : public QGVDrawItem
{
    Q_OBJECT

public:
    explicit MapLine(const QGV::GeoPos &pos1, const QGV::GeoPos &pos2);

    QColor color();
    void setColor(QColor color);
    void setWidth(int width);
    void setPen(QPen pen);

private:
    void onProjection(QGVMap* geoMap) override;
    void onUpdate() override;
    QPainterPath projShape() const override;
    void projPaint(QPainter* painter) override;
    QPointF projAnchor() const override;

    // Start and end coordinates of line
    QGV::GeoPos mPos1;
    QGV::GeoPos mPos2;
    // Projected points of line on to painting area
    QPointF mProjPos1;
    QPointF mProjPos2;

    QPen mPen {Qt::red};
};
