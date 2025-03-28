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

#ifndef CSVIMPORTDIALOG_H
#define CSVIMPORTDIALOG_H

#include "csvimporter.h"
#include "utils.h"

#include <QFile>
#include <QMessageBox>
#include <QDialog>

namespace Ui {
class CsvImportDialog;
}

class CsvImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CsvImportDialog(QWidget *parent = 0);
    ~CsvImportDialog();

    void setFile(QString filename);

signals:
    void import(Csv::FileInfo info);

private slots:
    void on_pushButton_import_clicked();

private:
    Ui::CsvImportDialog *ui;
    Csv::FileInfo fileInfo;
    QStringList lines;

    void msgBox(QString msg);
};

#endif // CSVIMPORTDIALOG_H
