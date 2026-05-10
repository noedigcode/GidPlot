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
        QStringList errorStrings;
        QString errorString();
    };

    Matrix(int numCols);

    QVector<double> dataColumn(int columnIndex, int startIndex = 0, int length = -1);
    QVector<MetaData> metadataColumn(int columnIndex);

    int errorCount();
    int valueConversionErrorCount();
    int excessColsErrorCount();
    int insufficientColsErrorCount();

    void setHeadingsExcludingIndexColumn(QStringList headings);
    QStringList getHeadingsForExistingColumns();
    QStringList getAllHeadings();
    QString heading(int column);

    int rowCount();
    int colCount();

    void addCsvLine(const QByteArray &line);
    void addRow(QVector<double> values);

    bool colValid(int column);

    bool convertToBool(const QByteArray &data, bool defaultValue, bool* ok = nullptr);

    struct VectorStats {
        double min = 0;
        double max = 0;
        bool monotonicallyIncreasing = false;
    };
    static VectorStats vectorStats(const QVector<double> &vector);

private:
    QStringList mHeadings;
    int mErrorCount = 0;
    int mValueConversionErrorCount = 0;
    int mExcessColsErrorCount = 0;
    int mInsufficientColsErrorCount = 0;

    struct Value {
        double value = 0;
        bool error = true;
        QByteArray originalValue;
    };
    void addRow(QVector<Value> values);

    // Data and meta mDataCols matrices: [col][row]. First column is index
    QVector<QVector<double>> mDataCols;
    QVector<QVector<MetaData>> mMetaDataCols;
};
typedef QSharedPointer<Matrix> MatrixPtr;

#endif // MATRIX_H
