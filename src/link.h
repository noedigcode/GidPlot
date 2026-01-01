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

#ifndef LINK_H
#define LINK_H

#include <QSharedPointer>
#include <QString>

class Link
{
public:
    QString tag;
    int group = 0;
    bool supportPosZoom = false;
    bool linkXpos = false;
    bool linkYpos = false;
    bool linkXzoom = false;
    bool linkYzoom = false;

    bool match(int otherGroup);
};

typedef QSharedPointer<Link> LinkPtr;

#endif // LINK_H
