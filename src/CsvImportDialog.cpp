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
    fileInfo = Csv::FileInfo();

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
            QByteArray line = file.readLine();
            line.replace("\n", "");
            lines.append(line);

            ui->textBrowser->append(QString("%1: %2")
                                    .arg(lineNum)
                                    .arg(QString(line)));

            // Try to guess some properties from the first line
            if (lineNum == 1) {

                // Determine whether line is text or data
                bool isText = false;
                foreach (QChar c, line) {
                    if (c.isLetter()) {
                        isText = true;
                        break;
                    }
                }
                if (isText) {
                    // Set headings text box to first row number
                    ui->lineEdit_headings->setText("1");
                    // Assume data starts at row 2
                    ui->spinBox_dataStartRow->setValue(2);
                } else {
                    // Clear headings text box
                    ui->lineEdit_headings->setText("");
                    // Assume data starts at row 1
                    ui->spinBox_dataStartRow->setValue(1);
                }

                // Try to guess separator
                if (line.contains('\t')) {
                    ui->radioButton_separatorTab->setChecked(true);
                } else if (line.contains(',')) {
                    ui->radioButton_separatorComma->setChecked(true);
                } else if (line.contains(' ')) {
                    ui->radioButton_separatorSpace->setChecked(true);
                    ui->checkBox_combineSeparators->setChecked(true);
                } else {
                    // Fall back on comma
                    ui->radioButton_separatorComma->setChecked(true);
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

    if (ui->radioButton_separatorComma->isChecked()) {
        fileInfo.separator = ',';
    } else if (ui->radioButton_separatorTab->isChecked()) {
        fileInfo.separator = '\t';
    } else if (ui->radioButton_separatorSpace->isChecked()) {
        fileInfo.separator = ' ';
    }
    fileInfo.combineSeparators = ui->checkBox_combineSeparators->isChecked();

    // Determine headings based on headings text box content
    QString headingText = ui->lineEdit_headings->text().trimmed();
    if (!headingText.isEmpty()) {
        int headingsRow = ui->lineEdit_headings->text().toInt();
        if (headingsRow > 0) {
            // > 0 means proper conversion from string, i.e. a heading row has
            // been specified.
            headingsRow--; // Program counts from zero.
            if (headingsRow >= lines.count()) {
                msgBox("Invalid headings row number.");
                return;
            } else {
                QByteArrayList headings = Csv::separateLine(
                            lines.value(headingsRow), fileInfo);
                foreach (QByteArray heading, headings) {
                    fileInfo.headings.append(QString(heading).trimmed());
                }
            }
        } else {
            // Use comma-separated headings specified in text box
            foreach (QString heading, ui->lineEdit_headings->text().split(",")) {
                fileInfo.headings.append(heading.trimmed());
            }
        }
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
