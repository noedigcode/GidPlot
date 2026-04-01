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

#include "QGVLine.h"


QGVLine::QGVLine(const QGV::GeoPos &pos1, const QGV::GeoPos &pos2)
{
    mPosList << pos1 << pos2;
    setup();
}

QGVLine::QGVLine(const QList<QGV::GeoPos> &posList)
{
    mPosList = posList;
    setup();
}

QColor QGVLine::color()
{
    return mPen.color();
}

void QGVLine::setColor(QColor color)
{
    mPen.setColor(color);
    repaint();
}

void QGVLine::setWidth(int width)
{
    mPen.setWidth(width);
    repaint();
}

void QGVLine::setPen(QPen pen)
{
    mPen = pen;
    repaint();
}

void QGVLine::setup()
{
    mPen.setCosmetic(true); // Cosmetic: Line will be the same thickness for all zoom levels.
    mPen.setWidth(3);
}

void QGVLine::onProjection(QGVMap* geoMap)
{
    QGVDrawItem::onProjection(geoMap);
    mProjPosList.clear();
    foreach (QGV::GeoPos pos, mPosList) {
        mProjPosList.append(geoMap->getProjection()->geoToProj(pos));
    }
}

void QGVLine::onUpdate()
{
    QGVDrawItem::onUpdate();
}

QPainterPath QGVLine::projShape() const
{
    QPainterPath path;
    for (int i = 0; i < mProjPosList.count(); i++) {
        const QPointF& p = mProjPosList.at(i);
        if (i == 0) {
            path.moveTo(p);
        } else {
            path.lineTo(p);
        }
    }

    return path;
}

void QGVLine::projPaint(QPainter* painter)
{
    if (mProjPosList.count() < 2) { return; }
    painter->setPen(mPen);
    QPointF p1 = mProjPosList.at(0);
    for (int i = 1; i < mProjPosList.count(); i++) {
        QPointF p2 = mProjPosList.at(i);
        painter->drawLine(p1, p2);
        p1 = p2;
    }
}

QPointF QGVLine::projAnchor() const
{
    return mProjPosList.value(0);
}

