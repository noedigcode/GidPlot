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

PlotMarkerItem::PlotMarkerItem(QCustomPlot* parentPlot, QCPAxisRect *axisRect,
                               QCPAxis* xAxis, QCPAxis* yAxis) :
    QCPAbstractItem(parentPlot),
    position(createPosition("position")),
    anchor(createAnchor("anchor", 0))
{
    this->setClipAxisRect(axisRect);
    position->setAxisRect(axisRect);
    position->setAxes(xAxis, yAxis);
    position->setCoords(0, 0);
}

double PlotMarkerItem::selectTest(const QPointF& /*pos*/, bool /*onlySelectable*/,
                                  QVariant* /*details*/) const
{
    return -1;
}

void PlotMarkerItem::draw(QCPPainter* painter)
{
    QPointF center(position->pixelPosition());
    QRect clip = clipRect();

    // Draw text
    if (!text.isEmpty()) {

        QFont font = parentPlot()->font();
        font.setPointSize(10);
        painter->setFont(font);

        int xoffset = 5;
        int yoffset = 5;
        int pad = 3;
        QPoint textPos(center.x() + xoffset + pad, center.y() - yoffset - pad);

        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(text);
        QRectF textRect(textPos.x() - pad,
                        textPos.y() - fm.ascent(),
                        textWidth + 2*pad,
                        fm.height() + 2*pad);
        painter->fillRect(textRect, QBrush(QColor(255, 255, 255, 150)));

        painter->setPen(QPen(Qt::black));
        painter->drawText(textPos, text);

    }

    // Draw circle
    if (showCircle) {
        double w = circleSize / 2.0;
        QRectF rect(center - QPointF(w, w), center + QPointF(w, w));
        if (clip.intersects(rect.toRect())) {
            painter->setPen(circlePen);
            painter->setBrush(circleFillBrush);
            painter->drawEllipse(rect);
        }
    }

    // Horizontal line
    if (showHorizontalLine) {
        if ((center.y() > clip.top()) && (center.y() < clip.bottom())) {
            painter->setPen(linePen);
            painter->drawLine(QLineF(clip.left(), center.y(), clip.right(), center.y()));
        }
    }

    // Vertical line
    if (showVerticalLine) {
        if ((center.x() > clip.left()) && (center.x() < clip.right())) {
            painter->setPen(linePen);
            painter->drawLine(QLineF(center.x(), clip.top(), center.x(), clip.bottom()));
        }
    }
}

QPointF PlotMarkerItem::anchorPixelPosition(int /*anchorId*/) const
{
    return position->pixelPosition();
}
