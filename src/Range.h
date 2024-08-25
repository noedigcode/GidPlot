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

#ifndef RANGE_H
#define RANGE_H

#include <QByteArray>
#include <QSharedPointer>
#include <QString>

struct Range
{
    Range();
    Range(int start, int end);
    Range(QString name, int start, int end);

    QString name;
    int start = 0;
    int end = 0;

    int size() const;

    QString toString() const;
    bool fromString(QString s);
};
typedef QSharedPointer<Range> RangePtr;
typedef QWeakPointer<Range> RangeWeakPtr;

#endif // RANGE_H
