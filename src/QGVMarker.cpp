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

#include "QGVMarker.h"


QGVMarker::QGVMarker(const QGV::GeoPos &pos, const float radius) :
    mPos(pos), mRadius(radius)
{
    setFlags(QGV::ItemFlag::IgnoreAzimuth | QGV::ItemFlag::IgnoreScale);

    mPen.setCosmetic(true); // Cosmetic: Line will be the same thickness for all zoom levels.
    mPen.setWidth(2);
}

void QGVMarker::setPosition(QGV::GeoPos pos)
{
    mPos = pos;

    updateGeometry();

    resetBoundary();
    refresh();
}

void QGVMarker::setWidth(int width)
{
    mPen.setWidth(width);
    repaint();
}

QPen QGVMarker::pen()
{
    return mPen;
}

void QGVMarker::setPen(QPen pen)
{
    mPen = pen;
    repaint();
}

QBrush QGVMarker::brush()
{
    return mBrush;
}

void QGVMarker::setBrush(QBrush brush)
{
    mBrush = brush;
    repaint();
}

void QGVMarker::onProjection(QGVMap* geoMap)
{
    QGVDrawItem::onProjection(geoMap);
    updateGeometry();
}

void QGVMarker::onUpdate()
{
    QGVDrawItem::onUpdate();
}

QPainterPath QGVMarker::projShape() const
{
    QPainterPath path;
    path.addRect(mProjRect);
    return path;
}

void QGVMarker::projPaint(QPainter* painter)
{
    painter->setPen(mPen);
    painter->setBrush(mBrush);
    painter->drawEllipse(mProjRect);
}

QPointF QGVMarker::projAnchor() const
{
    return mProjPos;
}

void QGVMarker::updateGeometry()
{
    mProjPos = getMap()->getProjection()->geoToProj(mPos);
    mProjRect = QRectF(mProjPos - QPointF(mRadius, mRadius),
                       mProjPos + QPointF(mRadius, mRadius));
}

