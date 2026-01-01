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

#include "mapline.h"


MapLine::MapLine(const QGV::GeoPos &pos1, const QGV::GeoPos &pos2) :
    mPos1(pos1),
    mPos2(pos2)
{
    mPen.setCosmetic(true); // Cosmetic: Line will be the same thickness for all zoom levels.
    mPen.setWidth(3);
}

QColor MapLine::color()
{
    return mPen.color();
}

void MapLine::setColor(QColor color)
{
    mPen.setColor(color);
    repaint();
}

void MapLine::setWidth(int width)
{
    mPen.setWidth(width);
    repaint();
}

void MapLine::setPen(QPen pen)
{
    mPen = pen;
    repaint();
}

void MapLine::onProjection(QGVMap* geoMap)
{
    QGVDrawItem::onProjection(geoMap);
    mProjPos1 = geoMap->getProjection()->geoToProj(mPos1);
    mProjPos2 = geoMap->getProjection()->geoToProj(mPos2);
}

void MapLine::onUpdate()
{
    QGVDrawItem::onUpdate();
}

QPainterPath MapLine::projShape() const
{
    QPainterPath path(mProjPos1);
    path.lineTo(mProjPos2);
    return path;
}

void MapLine::projPaint(QPainter* painter)
{
    painter->setPen(mPen);
    painter->drawLine(mProjPos1, mProjPos2);
}

QPointF MapLine::projAnchor() const
{
    return mProjPos1;
}

