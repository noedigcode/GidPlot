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

#ifndef MATRIX_H
#define MATRIX_H

#include <QPoint>
#include <QSharedPointer>
#include <QStringList>
#include <QVector>

class Matrix
{
public:
    struct MetaData
    {
        bool valueConversionError = false;
        bool excessColsError = false;
        bool insufficientColsError = false;
        bool hasError();
        QString errorString();
        QString originalValue;
    };
    typedef QSharedPointer<MetaData> MetaDataPtr;

    Matrix(int numCols);

    // Data and meta data matrices: [col][row]. First column is index
    QVector<QVector<double>> data;
    QVector<QVector<MetaDataPtr>> metaData;

    int errorCount = 0;
    int valueConversionErrorCount = 0;
    int excessColsErrorCount = 0;
    int insufficientColsErrorCount = 0;

    void setHeadings(QStringList headings);
    QStringList headings();
    QString heading(int column);
    int headingCount();

    int rowCount();
    int colCount();

    bool addCsvLine(const QByteArray &line);
    bool addRow(QVector<double> values);
    struct Value {
        double value = 0;
        bool error;
        QByteArray originalValue;
    };
    bool addRow(QVector<Value> values);

    bool colValid(int column);
    double min(int column);
    double max(int column);

    bool convertToBool(const QByteArray &data, bool defaultValue, bool* ok = nullptr);

    static double vmin(const QVector<double> &v);
    static double vmax(const QVector<double> &v);

private:
    QStringList mHeadings;
};
typedef QSharedPointer<Matrix> MatrixPtr;

#endif // MATRIX_H
