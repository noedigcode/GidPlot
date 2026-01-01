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

#pragma once

#include <QGVDrawItem.h>

#include <QBrush>
#include <QPainter>
#include <QPen>

class MapMarker : public QGVDrawItem
{
    Q_OBJECT

public:
    explicit MapMarker(const QGV::GeoPos &pos, const float radius);

    void setPosition(QGV::GeoPos pos);

    void setWidth(int width);
    QPen pen();
    void setPen(QPen pen);
    QBrush brush();
    void setBrush(QBrush brush);

private:
    void onProjection(QGVMap* geoMap) override;
    void onUpdate() override;
    QPainterPath projShape() const override;
    void projPaint(QPainter* painter) override;
    QPointF projAnchor() const override;

    void updateGeometry();

    QPen mPen {Qt::black};
    QBrush mBrush {QColor(255, 255, 255, 150)};
    QGV::GeoPos mPos;
    float mRadius = 5;

    // Projected points onto painting area
    QPointF mProjPos;
    QRectF mProjRect;
};
