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

#ifndef CSVIMPORTER_H
#define CSVIMPORTER_H

#include "csv.h"

#include <QElapsedTimer>
#include <QFile>
#include <QObject>
#include <QSharedPointer>
#include <QThread>

#include <functional>


class CsvImporter : public QObject
{
    Q_OBJECT
public:
    explicit CsvImporter(QObject *parent = nullptr);

    CsvPtr importCsv(Csv::FileInfo info);
    void removeCsv(CsvPtr csv);

signals:
    void importProgress(CsvPtr csv, QString info);
    void importFinished(CsvPtr csv);

private:
    QList<CsvPtr> csvDataList;
    void emitImportFinishedAndRemoveCsv(CsvPtr csv);

    void runInCsvThread(std::function<void(void)> func);

private slots:
    void doImport(CsvPtr csv);

};

#endif // CSVIMPORTER_H
