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

#include "PlotMarkerItem.h"

PlotMarkerItem::PlotMarkerItem(QCustomPlot* parentPlot) :
    QCPAbstractItem(parentPlot),
    position(createPosition("position")),
    anchor(createAnchor("anchor", 0))
{
    position->setCoords(0, 0);
}

double PlotMarkerItem::selectTest(const QPointF& /*pos*/, bool /*onlySelectable*/,
                                  QVariant* /*details*/) const
{
    return -1;
}

void PlotMarkerItem::draw(QCPPainter* painter)
{
    painter->setPen(pen);
    painter->setBrush(brush);
    QPointF center(position->pixelPosition());
    QRect clip = clipRect();

    // Draw circle
    if (showCircle) {
        double w = size / 2.0;
        QRectF rect(center - QPointF(w, w), center + QPointF(w, w));
        if (clip.intersects(rect.toRect())) {
            painter->drawEllipse(rect);
        }
    }

    // Horizontal line
    if (horizontalLine) {
        if ((center.y() > clip.top()) && (center.y() < clip.bottom())) {
            painter->drawLine(QLineF(clip.left(), center.y(), clip.right(), center.y()));
        }
    }

    // Vertical line
    if (verticalLine) {
        if ((center.x() > clip.left()) && (center.x() < clip.right())) {
            painter->drawLine(QLineF(center.x(), clip.top(), center.x(), clip.bottom()));
        }
    }
}

QPointF PlotMarkerItem::anchorPixelPosition(int /*anchorId*/) const
{
    return position->pixelPosition();
}
