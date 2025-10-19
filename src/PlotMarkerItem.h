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

#ifndef PLOTMARKERITEM_H
#define PLOTMARKERITEM_H

#include "QCustomPlot/qcustomplot.h"

#include <QObject>

class PlotMarkerItem : public QCPAbstractItem
{
    Q_OBJECT
public:
    explicit PlotMarkerItem(QCustomPlot* parentPlot);

    QCPItemPosition* const position;
    QCPItemAnchor* const anchor;

    QPen circlePen;
    QBrush circleFillBrush;
    double circleSize = 1;
    bool showCircle = true;

    QPen linePen;
    bool showVerticalLine = false;
    bool showHorizontalLine = false;

    double selectTest(const QPointF& pos, bool onlySelectable,
                      QVariant* details) const override;

protected:
    void draw(QCPPainter* painter) override;
    QPointF anchorPixelPosition(int anchorId) const override;
};

#endif // PLOTMARKERITEM_H
