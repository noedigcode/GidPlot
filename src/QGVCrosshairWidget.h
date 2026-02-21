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

#ifndef QGVCROSSHAIRWIDGET_H
#define QGVCROSSHAIRWIDGET_H

#include "QGVWidget.h"

class QGVCrosshairWidget : public QGVWidget
{
    Q_OBJECT
public:
    QGVCrosshairWidget();

    void setPos(QPoint pos);

    bool horizontalLineVisible();
    void setHorizontalLineVisible(bool visible);
    bool verticalLineVisible();
    void setVerticalLineVisible(bool visible);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint mPos;
    bool mHorizontalLineVisible = true;
    bool mVerticalLineVisible = true;
};

#endif // QGVCROSSHAIRWIDGET_H
