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

#include "mapMarker.h"


MapMarker::MapMarker(const QGV::GeoPos &pos, const float radius) :
    mPos(pos), mRadius(radius)
{
    setFlags(QGV::ItemFlag::IgnoreAzimuth | QGV::ItemFlag::IgnoreScale);

    mPen.setCosmetic(true); // Cosmetic: Line will be the same thickness for all zoom levels.
    mPen.setWidth(2);
}

void MapMarker::setPosition(QGV::GeoPos pos)
{
    mPos = pos;

    updateGeometry();

    resetBoundary();
    refresh();
}

void MapMarker::setWidth(int width)
{
    mPen.setWidth(width);
    repaint();
}

QPen MapMarker::pen()
{
    return mPen;
}

void MapMarker::setPen(QPen pen)
{
    mPen = pen;
    repaint();
}

QBrush MapMarker::brush()
{
    return mBrush;
}

void MapMarker::setBrush(QBrush brush)
{
    mBrush = brush;
    repaint();
}

void MapMarker::onProjection(QGVMap* geoMap)
{
    QGVDrawItem::onProjection(geoMap);
    updateGeometry();
}

void MapMarker::onUpdate()
{
    QGVDrawItem::onUpdate();
}

QPainterPath MapMarker::projShape() const
{
    QPainterPath path;
    path.addRect(mProjRect);
    return path;
}

void MapMarker::projPaint(QPainter* painter)
{
    painter->setPen(mPen);
    painter->setBrush(mBrush);
    painter->drawEllipse(mProjRect);
}

QPointF MapMarker::projAnchor() const
{
    return mProjPos;
}

void MapMarker::updateGeometry()
{
    mProjPos = getMap()->getProjection()->geoToProj(mPos);
    mProjRect = QRectF(mProjPos - QPointF(mRadius, mRadius),
                       mProjPos + QPointF(mRadius, mRadius));
}

