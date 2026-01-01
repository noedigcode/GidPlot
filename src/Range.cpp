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

#include "Range.h"

Range::Range()
{

}

Range::Range(int start, int end) : start(start), end(end)
{

}

Range::Range(QString name, int start, int end) :
    name(name), start(start), end(end)
{

}

int Range::min() const
{
    return qMin(start, end);
}

int Range::max() const
{
    return qMax(start, end);
}

int Range::size() const
{
    return max() - min();
}

bool Range::sameAs(const Range otherRange)
{
    return (min() == otherRange.min())
            && (max() == otherRange.max());
}

QString Range::toRangeString() const
{
    return QString("%1 - %2").arg(start).arg(end);
}

bool Range::fromRangeString(QString s)
{
    static QByteArray allowed {"-,; "};
    s = s.trimmed().simplified();
    foreach (QChar c, allowed) {
        if (s.contains(c)) {
            QStringList terms = s.split(c);
            if (terms.count() < 2) { return false; }
            bool ok = true;
            int a = terms.value(0).toInt(&ok);
            if (!ok) { return false; }
            int b = terms.value(1).toInt(&ok);
            if (!ok) { return false; }
            start = a;
            end = b;
            return true;
        }
    }
    return false;
}
