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

#ifndef CSV_H
#define CSV_H

#include "matrix.h"
#include "Range.h"

class Csv : public QObject
{
    Q_OBJECT
public:

    struct FileInfo
    {
        QString filename;
        QStringList headings;
        int dataStartRow = 0;
    } fileInfo;

    struct ImportInfo
    {
        qint64 timeSecs = 0;
        bool success = false;
        QString info;
    } importInfo;

    QList<RangePtr> ranges();
    void addRange(RangePtr range);

    Range allRange();

    MatrixPtr matrix;

signals:
    void rangeAdded(RangePtr range);

private:
    QList<RangePtr> mRanges;
};

typedef QSharedPointer<Csv> CsvPtr;
typedef QWeakPointer<Csv> CsvWeakPtr;

#endif // CSV_H
