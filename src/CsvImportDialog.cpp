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

#include "CsvImportDialog.h"
#include "ui_CsvImportDialog.h"

#include <QFileInfo>
#include <QScrollBar>

CsvImportDialog::CsvImportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CsvImportDialog)
{
    ui->setupUi(this);

    ui->lineEdit_headings->setFocus();
}

CsvImportDialog::~CsvImportDialog()
{
    delete ui;
}

void CsvImportDialog::setFile(QString filename)
{
    fileInfo.filename = filename;
    ui->lineEdit_filename->setText(filename);
    ui->label_fileSize->setText(QString("File size: %1")
                                    .arg(QLocale().formattedDataSize(
                                        QFileInfo(filename).size())));

    lines.clear();

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // Error opening file
        msgBox("Error opening file " + filename);
        this->hide();
        return;
    } else {

        ui->textBrowser->clear();

        int lineNum = 0;
        int limit = 10;
        while (!file.atEnd()) {
            lineNum++;
            QString line = file.readLine();
            line.replace("\n", "");
            lines.append(line);

            ui->textBrowser->append(QString("%1: %2")
                                    .arg(lineNum)
                                    .arg(line));

            // Detect if first line is text
            if (lineNum == 1) {
                bool letters = false;
                foreach (QChar c, line) {
                    if (c.isLetter()) {
                        letters = true;
                        break;
                    }
                }
                if (letters) {
                    ui->lineEdit_headings->setText("1");
                    ui->spinBox_dataStartRow->setValue(2);
                } else {
                    ui->lineEdit_headings->setText("");
                    ui->spinBox_dataStartRow->setValue(1);
                }
            }

            if (lineNum >= limit) { break; }
        }
    }
    file.close();

    Utils::callLater(this, [=]()
    {
        ui->textBrowser->horizontalScrollBar()->setValue(0);
        ui->textBrowser->verticalScrollBar()->setValue(0);
    });
}

void CsvImportDialog::on_pushButton_import_clicked()
{
    int dataRow = ui->spinBox_dataStartRow->value() - 1;
    if (dataRow >= lines.count()) {
        msgBox("Invalid data start row.");
        return;
    }
    fileInfo.dataStartRow = dataRow;

    int headingsRow = ui->lineEdit_headings->text().toInt();
    if (headingsRow > 0) {
        // > 0 means proper conversion from string
        headingsRow--; // Program counts from zero.
        if (headingsRow >= lines.count()) {
            msgBox("Invalid headings row number.");
            return;
        } else {
            fileInfo.headings = lines[headingsRow].split(",");
        }
    } else {
        // Use specified comma-separated headings
        fileInfo.headings = ui->lineEdit_headings->text().split(",");
    }

    emit import(fileInfo);
    this->hide();
}

void CsvImportDialog::msgBox(QString msg)
{
    QMessageBox mb;
    mb.setText(msg);
    mb.exec();
}
