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

#include "csv.h"


QList<RangePtr> Csv::ranges()
{
    return mRanges;
}

void Csv::addRange(RangePtr range)
{
    mRanges.append(range);
    emit rangeAdded(range);
}

Range Csv::allRange()
{
    return Range("All", 0, matrix->rowCount());
}

QByteArrayList Csv::separateLine(const QByteArray &line, FileInfo fileInfo)
{
    if (fileInfo.combineSeparators) {
        QByteArray line2;
        bool lastWasSep = false;
        for (int i = 0; i < line.count(); i++) {
            char c = line.at(i);
            if (c == '\n') { break; } // Do not add newlines to output
            bool isSep = (c == fileInfo.separator);
            if (i == 0) {
                // Skip leading separator
                if (!isSep) {
                    line2.append(c);
                }
            } else {
                bool repeatedSep = isSep && lastWasSep;
                if (!repeatedSep) {
                    line2.append(c);
                }
            }
            lastWasSep = isSep;
        }
        // Remove trailing separator
        if (line2.endsWith(fileInfo.separator)) {
            line2.remove(line2.count() - 1, 1);
        }
        return line2.split(fileInfo.separator);
    } else {
        return line.split(fileInfo.separator);
    }
}
