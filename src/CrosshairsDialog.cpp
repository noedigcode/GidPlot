/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2025 Gideon van der Kolf
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

#include "CrosshairsDialog.h"
#include "ui_CrosshairsDialog.h"

CrosshairsDialog::CrosshairsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CrosshairsDialog)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowStaysOnTopHint, true);
}

CrosshairsDialog::~CrosshairsDialog()
{
    delete ui;
}

void CrosshairsDialog::show(Settings settings)
{
    ui->groupBox_plotCrosshair->setChecked(settings.plotCrosshair);
    ui->checkBox_plotHline->setChecked(settings.plotHline);
    ui->checkBox_plotVline->setChecked(settings.plotVline);
    ui->checkBox_plotDot->setChecked(settings.plotDot);
    ui->groupBox_mouseCrosshair->setChecked(settings.mouseCrosshair);
    ui->checkBox_mouseHline->setChecked(settings.mouseHline);
    ui->checkBox_mouseVline->setChecked(settings.mouseVline);
    ui->checkBox_mouseDot->setChecked(settings.mouseDot);

    QDialog::show();
}

CrosshairsDialog::Settings CrosshairsDialog::settings()
{
    Settings s;
    s.plotCrosshair = ui->groupBox_plotCrosshair->isChecked();
    s.plotHline = ui->checkBox_plotHline->isChecked();
    s.plotVline = ui->checkBox_plotVline->isChecked();
    s.plotDot = ui->checkBox_plotDot->isChecked();
    s.mouseCrosshair = ui->groupBox_mouseCrosshair->isChecked();
    s.mouseHline = ui->checkBox_mouseHline->isChecked();
    s.mouseVline = ui->checkBox_mouseVline->isChecked();
    s.mouseDot = ui->checkBox_mouseDot->isChecked();

    return s;
}

void CrosshairsDialog::on_groupBox_plotCrosshair_toggled(bool /*arg1*/)
{
    emitSettingsChanged();
}

void CrosshairsDialog::emitSettingsChanged()
{
    emit settingsChanged(settings());
}

void CrosshairsDialog::on_checkBox_plotHline_toggled(bool /*checked*/)
{
    emitSettingsChanged();
}

void CrosshairsDialog::on_checkBox_plotVline_toggled(bool /*checked*/)
{
    emitSettingsChanged();
}

void CrosshairsDialog::on_checkBox_plotDot_toggled(bool /*checked*/)
{
    emitSettingsChanged();
}

void CrosshairsDialog::on_groupBox_mouseCrosshair_toggled(bool /*arg1*/)
{
    emitSettingsChanged();
}

void CrosshairsDialog::on_checkBox_mouseHline_toggled(bool /*checked*/)
{
    emitSettingsChanged();
}

void CrosshairsDialog::on_checkBox_mouseVline_toggled(bool /*checked*/)
{
    emitSettingsChanged();
}

void CrosshairsDialog::on_checkBox_mouseDot_toggled(bool /*checked*/)
{
    emitSettingsChanged();
}

void CrosshairsDialog::on_pushButton_ok_clicked()
{
    hide();
}

