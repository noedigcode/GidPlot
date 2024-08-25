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
    : data(numCols + 1), metaData(numCols + 1)
{

}

void Matrix::setHeadings(QStringList headings)
{
    mHeadings.clear();
    mHeadings.append("(index)");
    mHeadings.append(headings);
}

QStringList Matrix::headings()
{
    return mHeadings;
}

QString Matrix::heading(int column)
{
    return mHeadings.value(column, "");
}

int Matrix::headingCount()
{
    return mHeadings.count();
}

int Matrix::rowCount()
{
    int ret = 0;
    if (data.count()) {
        ret = data[0].count();
    }
    return ret;
}

int Matrix::colCount()
{
    return data.count();
}

bool Matrix::addCsvLine(const QByteArray &line)
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

    return addRow(values);
}

bool Matrix::addRow(QVector<double> values)
{
    QVector<Value> values2(values.size());
    for (int i = 0; i < values.size(); ++i) {
        Value& v = values2[i];
        v.value = values[i];
        v.error = false;
    }

    return addRow(values2);
}

bool Matrix::addRow(QVector<Value> values)
{
    bool retError = false;

    // +1 as first column is index
    int n = qMin(values.count() + 1, colCount());

    for (int icol = 0; icol < colCount(); ++icol) {
        bool errorOccurred = false;
        bool valueConversionError = false;
        bool excessColsError = false;
        bool insufficientColsError = false;
        int ivalue = icol - 1;

        if (icol == 0) {
            // First column is index
            data[icol].append(data[0].count());
        } else if (icol < n) {

            Value& value = values[ivalue];
            if (value.error) {
                // On error, use previous value if any
                int len = data[icol].length();
                if (len) {
                    value.value = data[icol][len-1];
                }
            }
            data[icol].append(value.value);

            // Check if value converted properly from string to double
            if (value.error) {
                errorOccurred = true;
                valueConversionError = true;
                valueConversionErrorCount++;
            }
            // Check if there are too many columns and indicate it with error for
            // last data value in row.
            if ( (icol == data.count()-1) && (icol < values.count()-1) ) {
                errorOccurred = true;
                excessColsError = true;
                excessColsErrorCount++;
            }
        } else {
            // icol >= n, meaning insufficient columns present in data.
            // Use previous value if any
            double value = 0;
            int len = data[icol].length();
            if (len) {
                value = data[icol][len-1];
            }
            data[icol].append(value);
            errorOccurred = true;
            insufficientColsError = true;
            insufficientColsErrorCount++;
        }

        Matrix::MetaDataPtr md;
        if (errorOccurred) {
            retError |= errorOccurred;
            errorCount++;
            md.reset(new Matrix::MetaData());
            md->valueConversionError = valueConversionError;
            md->excessColsError = excessColsError;
            md->insufficientColsError = insufficientColsError;
            if (insufficientColsError) {
                md->originalValue = "?";
            } else {
                md->originalValue = values[ivalue].originalValue;
            }
        }
        metaData[icol].append(md);
    }

    return !retError;
}

bool Matrix::colValid(int column)
{
    return ( (column < data.count()) && (column >= 0) );
}

double Matrix::min(int column)
{
    if (!colValid(column)) { return 0; }
    return vmin(data[column]);
}

double Matrix::max(int column)
{
    if (!colValid(column)) { return 0; }
    return vmax(data[column]);
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

double Matrix::vmin(const QVector<double> &v)
{
    if (v.isEmpty()) { return 0; }
    double min = v[0];
    for (int i=1; i < v.count(); i++) {
        if (v[i] < min) { min = v[i]; }
    }
    return min;
}

double Matrix::vmax(const QVector<double> &v)
{
    if (v.isEmpty()) { return 0; }
    double max = v[0];
    for (int i=1; i < v.count(); i++) {
        if (v[i] > max) { max = v[i]; }
    }
    return max;
}

bool Matrix::MetaData::hasError()
{
    return valueConversionError
            || excessColsError
            || insufficientColsError;
}

QString Matrix::MetaData::errorString()
{
    QStringList errors;
    if (valueConversionError) {
        errors.append("Value conversion error");
    }
    if (excessColsError) {
        errors.append("Excess columns");
    }
    if (insufficientColsError) {
        errors.append("No data, insufficient columns");
    }
    return errors.join("; ");
}
