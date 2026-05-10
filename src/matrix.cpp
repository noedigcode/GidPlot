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

#include "matrix.h"

#include <QVariant>


Matrix::Matrix(int numCols)
    // Add one as first column is used for index
    : mDataCols(numCols + 1), mMetaDataCols(numCols + 1)
{

}

QVector<Matrix::MetaData> Matrix::metadataColumn(int columnIndex)
{
    return mMetaDataCols.value(columnIndex);
}

QVector<double> Matrix::dataColumn(int columnIndex, int startIndex, int length)
{
    const QVector<double> col = mDataCols.value(columnIndex);

    if ((startIndex < 0) || (startIndex >= col.length())) {
        return QVector<double>();
    }

    if ((startIndex == 0) && (length == col.length())) {
        return col;
    } else {
        return col.mid(startIndex, length);
    }
}

int Matrix::errorCount()
{
    return mErrorCount;
}

int Matrix::valueConversionErrorCount()
{
    return mValueConversionErrorCount;
}

int Matrix::excessColsErrorCount()
{
    return mExcessColsErrorCount;
}

int Matrix::insufficientColsErrorCount()
{
    return mInsufficientColsErrorCount;
}

void Matrix::setHeadingsExcludingIndexColumn(QStringList headings)
{
    mHeadings.clear();
    mHeadings.append("(index)");
    mHeadings.append(headings);
}

QStringList Matrix::getHeadingsForExistingColumns()
{
    QStringList ret = mHeadings.mid(0, colCount());
    while (ret.count() < colCount()) {
        ret.append(QString("Column %1").arg(ret.count()));
    }
    return ret;
}

QStringList Matrix::getAllHeadings()
{
    return mHeadings;
}

QString Matrix::heading(int column)
{
    return getHeadingsForExistingColumns().value(column, "");
}

int Matrix::rowCount()
{
    int ret = 0;
    if (mDataCols.count()) {
        ret = mDataCols[0].count();
    }
    return ret;
}

int Matrix::colCount()
{
    return mDataCols.count();
}

void Matrix::addCsvLine(const QByteArray &line)
{
    QList<QByteArray> split = line.split(',');
    QVector<Value> values(split.size());
    for (int i = 0; i < split.size(); ++i) {
        Value& v = values[i];
        bool ok;
        v.originalValue = split[i];
        v.value = v.originalValue.toDouble(&ok);
        if (!ok) {
            // Try bool
            v.value = convertToBool(v.originalValue, false, &ok) ? 1.0 : 0.0;
        }
        v.error = !ok;
    }

    addRow(values);
}

void Matrix::addRow(QVector<double> values)
{
    QVector<Value> values2(values.size());
    for (int i = 0; i < values.size(); ++i) {
        Value& v = values2[i];
        v.value = values[i];
        v.error = false;
    }

    addRow(values2);
}

void Matrix::addRow(QVector<Value> values)
{
    // +1 as first column is index
    int max = qMax(values.count() + 1, colCount());

    int rowCountBeforeAdd = rowCount();

    for (int icol = 0; icol < max; ++icol) {

        MetaData metaData;
        int iValueCol = icol - 1;
        bool backfill = false;
        double backfillValue = 0;

        // Add missing column if necessary
        if (icol == colCount()) {
            mDataCols.append(QVector<double>(rowCountBeforeAdd));
            mMetaDataCols.append(QVector<MetaData>(rowCountBeforeAdd));
            backfill = true;

            metaData.excessColsError = true;
            mExcessColsErrorCount++;
            mErrorCount++;
            metaData.errorStrings.append(
                        QString("Additional column %1 added, not present in previous rows.")
                        .arg(icol));
        }

        if (icol == 0) {

            // First column is index
            mDataCols[icol].append(mDataCols[0].count());

        } else if (iValueCol < values.count()) {

            Value& value = values[iValueCol];
            if (value.error) {
                // Value conversion error
                metaData.valueConversionError = true;
                mValueConversionErrorCount++;
                mErrorCount++;
                // Use previous value if any
                int len = mDataCols[icol].length();
                if (len) {
                    value.value = mDataCols[icol][len-1];
                    metaData.errorStrings.append(
                                QString("Value conversion error. Previous row value used. Original text: %1")
                                .arg(QString::fromUtf8(value.originalValue)));
                } else {
                    metaData.errorStrings.append(
                                QString("Value conversion error. Defaulted to zero. Original text: %1")
                                .arg(QString::fromUtf8(value.originalValue)));
                }
            }
            mDataCols[icol].append(value.value);
            backfillValue = value.value;

        } else {

            // Insufficient columns present in data.
            metaData.insufficientColsError = true;
            mInsufficientColsErrorCount++;
            mErrorCount++;
            // Use value of previous row if any
            double value = 0;
            int len = mDataCols[icol].length();
            if (len) {
                value = mDataCols[icol][len-1];
                metaData.errorStrings.append("No value (insufficient columns in row). Previous row value used.");
            } else {
                metaData.errorStrings.append("No value (insufficient columns in row). Defaulted to zero.");
            }
            mDataCols[icol].append(value);
            backfillValue = value;
        }

        mMetaDataCols[icol].append(metaData);

        if (backfill) {
            for (int irow = 0; irow < rowCountBeforeAdd; irow++) {

                mDataCols[icol][irow] = backfillValue;

                mInsufficientColsErrorCount++;
                mErrorCount++;

                MetaData backfillMetaData;
                backfillMetaData.insufficientColsError = true;
                backfillMetaData.errorStrings.append(
                            QString("No value (insufficient columns in row). Backfilled from row %1.")
                            .arg(rowCount()));

                mMetaDataCols[icol][irow] = backfillMetaData;
            }
        }
    }
}

bool Matrix::colValid(int column)
{
    return ( (column < mDataCols.count()) && (column >= 0) );
}

bool Matrix::convertToBool(const QByteArray& data, bool defaultValue, bool* ok)
{
    bool ret = defaultValue;

    static QList<QByteArray> trues {"true", "1"};
    static QList<QByteArray> falses {"", "false", "0"};

    QByteArray lower = data.trimmed().toLower();
    bool found = false;
    foreach (const QByteArray& t, trues) {
        if (lower == t) {
            ret = true;
            found = true;
        }
    }
    if (!found) {
        foreach (const QByteArray& f, falses) {
            if (lower == f) {
                ret = false;
                found = true;
            }
        }
    }

    if (ok) {
        *ok = found;
    }

    return ret;
}

Matrix::VectorStats Matrix::vectorStats(const QVector<double> &vector)
{
    VectorStats s;
    if (vector.isEmpty()) { return s; }
    s.max = vector[0];
    s.min = vector[0];
    s.monotonicallyIncreasing = true;
    double lastValue = vector[0];
    for (int i = 1; i < vector.count(); i++) {
        double value = vector[i];
        if (value < s.min) { s.min = value; }
        if (value > s.max) { s.max = value; }
        if (value < lastValue) { s.monotonicallyIncreasing = false; }
        lastValue = value;
    }

    return s;
}

bool Matrix::MetaData::hasError()
{
    return valueConversionError
            || excessColsError
            || insufficientColsError;
}

QString Matrix::MetaData::errorString()
{
    return errorStrings.join('\n');
}
