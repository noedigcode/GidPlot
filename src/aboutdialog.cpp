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

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include "QCustomPlot/qcustomplot.h"

AboutDialog::AboutDialog(QString settingsPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    setWindowTitle(QString("About %1").arg(APP_NAME));

    QString text = ui->label_settingsPath->text();
    text.replace("%SETTINGS_PATH%", settingsPath);
    ui->label_settingsPath->setText(text);

    text = ui->label_appname->text();
    text.replace("%APP_NAME%", APP_NAME);
    ui->label_appname->setText(text);

    text = ui->label_appInfo->text();
    text.replace("%APP_VERSION%", APP_VERSION);
    text.replace("%APP_YEAR_FROM%", APP_YEAR_FROM);
    text.replace("%APP_YEAR%", APP_YEAR);
    text.replace("%QT_VERSION%", QT_VERSION_STR);
    text.replace("%QCUSTOMPLOT_VERSION%", QCUSTOMPLOT_VERSION_STR);
    ui->label_appInfo->setText(text);

    QString changelog = "Could not load changelog";
    QFile f("://changelog");
    if (f.open(QIODevice::ReadOnly)) {
        changelog = f.readAll();
    }
    ui->textBrowser->setMarkdown(changelog);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_pushButton_Ok_clicked()
{
    this->hide();
}
