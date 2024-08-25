/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2024 Gideon van der Kolf
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

#ifndef CROSSHAIR_H
#define CROSSHAIR_H

#include "QCustomPlot/qcustomplot.h"

#include <QObject>

class Crosshair : public QObject
{
    Q_OBJECT
public:
    explicit Crosshair(QObject *parent = nullptr);

    void init(QCustomPlot* plot);

    void enableVline(bool enable);
    bool isVlineEnabled();

    void enableHline(bool enable);
    bool isHlineEnabled();

    void enableDot(bool enable);
    bool isDotEnabled();
    void setDotRadius(int radius);
    int dotRadius();

    void setCoords(double x, double y);

    void setVisible(bool visible);
    bool isVisible();

public slots:
    void redraw();

private:
    QCustomPlot* mPlot = nullptr;
    QString mLayerName = "overlay";

    double mXcoord;
    double mYcoord;

    bool mVisible = true;

    QCPItemEllipse* mDot = nullptr;
    bool mEnableDot = true;
    int mDotRadius = 3;

    QCPItemLine* mVline = nullptr;
    bool mEnableVline = true;
    QCPItemLine* mHline = nullptr;
    bool mEnableHline = true;
};

#endif // CROSSHAIR_H
