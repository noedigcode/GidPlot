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

#include "csvimporter.h"

#include <QDebug>

CsvImporter::CsvImporter(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<CsvPtr>("CsvPtr");
}

CsvPtr CsvImporter::importCsv(Csv::FileInfo info)
{
    CsvPtr csv(new Csv());
    csv->fileInfo = info;
    csvDataList.append(csv);

    runInCsvThread([this, csvWeakPtr = CsvWeakPtr(csv)]()
    {
        CsvPtr csv2(csvWeakPtr);
        if (!csv2) { return; }
        doImport(csv2);
    });

    return csv;
}

void CsvImporter::removeCsv(CsvPtr csv)
{
    csvDataList.removeAll(csv);
}

void CsvImporter::emitImportFinishedAndRemoveCsv(CsvPtr csv)
{
    csvDataList.removeAll(csv);
    emit importFinished(csv);
}

void CsvImporter::runInCsvThread(std::function<void ()> func)
{
    QMetaObject::invokeMethod(this, func, Qt::QueuedConnection);
}

void CsvImporter::doImport(CsvPtr csv)
{
    QFile file(csv->fileInfo.filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Error opening file
        csv->importInfo.success = false;
        csv->importInfo.info = "Error opening file.";
        emitImportFinishedAndRemoveCsv(csv);
        return;
    }

    QElapsedTimer timer;
    timer.start();

    QElapsedTimer progressFeedbackTimer;
    progressFeedbackTimer.start();
    emit importProgress(csv, "Starting");

    qint64 filesize = file.size();
    qint64 bytesread = 0;


    for (int lineNum = 0; !file.atEnd(); lineNum++) {
        QByteArray line = file.readLine();
        bytesread += line.count();
        if (lineNum < csv->fileInfo.dataStartRow) { continue; }
        if (lineNum == csv->fileInfo.dataStartRow) {
            csv->matrix.reset(new Matrix(line.split(',').count()));
            csv->matrix->setHeadings(csv->fileInfo.headings);
        }
        csv->matrix->addCsvLine(line);

        if (progressFeedbackTimer.elapsed() > 100) {
            emit importProgress(csv,
                                QString("%1 lines (%2 %) (%3 errors) (%4 seconds elapsed)")
                                .arg(lineNum + 1)
                                .arg((bytesread * 100) / filesize)
                                .arg(csv->matrix->errorCount)
                                .arg(timer.elapsed() / 1000));
        }
    }

    file.close();

    csv->importInfo.timeSecs = timer.elapsed() / 1000;

    if (csv->matrix.isNull()) {
        // No data was loaded
        csv->importInfo.success = false;
        csv->importInfo.info = "No data found in file.";
    } else {
        csv->importInfo.success = true;
    }

    emitImportFinishedAndRemoveCsv(csv);
}
