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

#include "QGVCrosshairWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

QGVCrosshairWidget::QGVCrosshairWidget()
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void QGVCrosshairWidget::setPos(QPoint pos)
{
    mPos = pos;
    update();
}

bool QGVCrosshairWidget::horizontalLineVisible()
{
    return mHorizontalLineVisible;
}

void QGVCrosshairWidget::setHorizontalLineVisible(bool visible)
{
    mHorizontalLineVisible = visible;
    update();
}

bool QGVCrosshairWidget::verticalLineVisible()
{
    return mVerticalLineVisible;
}

void QGVCrosshairWidget::setVerticalLineVisible(bool visible)
{
    mVerticalLineVisible = visible;
    update();
}

void QGVCrosshairWidget::paintEvent(QPaintEvent* /*event*/)
{
    if (size().isEmpty()) { return; }

    QPainter painter(this);

    if (mHorizontalLineVisible) {
        painter.drawLine(0, mPos.y(), width(), mPos.y());
    }
    if (mVerticalLineVisible) {
        painter.drawLine(mPos.x(), 0, mPos.x(), height());
    }
}
