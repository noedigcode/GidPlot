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

#include "crosshair.h"

Crosshair::Crosshair(QObject *parent)
    : QObject{parent}
{}

void Crosshair::init(QCustomPlot* plot)
{
    mPlot = plot;

    mVline = new QCPItemLine(mPlot);
    mVline->setVisible(false);
    mVline->setLayer(mLayerName);

    mHline = new QCPItemLine(mPlot);
    mHline->setVisible(false);
    mHline->setLayer(mLayerName);

    mDot = new QCPItemEllipse(mPlot);
    mDot->setVisible(false);
    mDot->setLayer(mLayerName);

    QCPAxis* xaxis = mPlot->xAxis;
    if (xaxis) {
        connect(xaxis,
                QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                this, &Crosshair::redraw);
    }
    QCPAxis* yaxis = mPlot->yAxis;
    if (!yaxis) {
        connect(yaxis,
                QOverload<const QCPRange &>::of(&QCPAxis::rangeChanged),
                this, &Crosshair::redraw);
    }
}

void Crosshair::enableVline(bool enable)
{
    mEnableVline = enable;
    // If disabling, hide it immediately. If enabling, wait for setCoords().
    if (!enable) {
        mVline->setVisible(false);
    }
}

bool Crosshair::isVlineEnabled()
{
    return mEnableVline;
}

void Crosshair::enableHline(bool enable)
{
    mEnableHline = enable;
    // If disabling, hide it immediately. If enabling, wait for setCoords().
    if (!enable) {
        mHline->setVisible(false);
    }
}

bool Crosshair::isHlineEnabled()
{
    return mEnableHline;
}

void Crosshair::enableDot(bool enable)
{
    mEnableDot = enable;
    // If disabling, hide it immediately. If enabling, wait for setCoords().
    if (!enable) {
        mDot->setVisible(false);
    }
}

bool Crosshair::isDotEnabled()
{
    return mEnableDot;
}

void Crosshair::setDotRadius(int radius)
{
    mDotRadius = radius;
}

int Crosshair::dotRadius()
{
    return mDotRadius;
}

void Crosshair::setCoords(double x, double y)
{
    mXcoord = x;
    mYcoord = y;

    redraw();
}

void Crosshair::setVisible(bool visible)
{
    mVisible = visible;
    // If disabling, hide it immediately. If enabling, wait for setCoords().
    if (!visible) {
        mVline->setVisible(false);
        mHline->setVisible(false);
        mDot->setVisible(false);
    }
}

bool Crosshair::isVisible()
{
    return mVisible;
}

void Crosshair::redraw()
{
    if (!mPlot || !mVline) { return; }
    QCPAxis* xaxis = mPlot->xAxis;
    if (!xaxis) { return; }
    QCPAxis* yaxis = mPlot->yAxis;
    if (!yaxis) { return; }

    double x = mXcoord;
    double y = mYcoord;

    mVline->start->setCoords(x, yaxis->range().upper);
    mVline->end->setCoords(x, yaxis->range().lower);
    mVline->setVisible(mEnableVline && mVisible);

    mHline->start->setCoords(xaxis->range().lower, y);
    mHline->end->setCoords(xaxis->range().upper, y);
    mHline->setVisible(mEnableHline && mVisible);

    int xpx = xaxis->coordToPixel(x);
    int ypx = yaxis->coordToPixel(y);
    mDot->topLeft->setPixelPosition(QPointF(xpx - mDotRadius, ypx - mDotRadius));
    mDot->bottomRight->setPixelPosition(QPointF(xpx + mDotRadius, ypx + mDotRadius));
    mDot->setVisible(mEnableDot && mVisible);
}
