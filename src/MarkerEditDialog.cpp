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

#include "MarkerEditDialog.h"
#include "ui_MarkerEditDialog.h"

MarkerEditDialog::MarkerEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MarkerEditDialog)
{
    ui->setupUi(this);

    setupPresetMenu();
}

MarkerEditDialog::~MarkerEditDialog()
{
    delete ui;
}

void MarkerEditDialog::edit(QString text, bool showHline, bool showVline,
                            bool showDot, std::function<void ()> callback)
{
    ui->plainTextEdit_labelText->setPlainText(text);
    ui->checkBox_horizontalLine->setChecked(showHline);
    ui->checkBox_verticalLine->setChecked(showVline);
    ui->checkBox_dot->setChecked(showDot);
    mCallback = callback;
    show();
}

QString MarkerEditDialog::text()
{
    return ui->plainTextEdit_labelText->toPlainText();
}

bool MarkerEditDialog::showHline()
{
    return ui->checkBox_horizontalLine->isChecked();
}

bool MarkerEditDialog::showVline()
{
    return ui->checkBox_verticalLine->isChecked();
}

bool MarkerEditDialog::showDot()
{
    return ui->checkBox_dot->isChecked();
}

void MarkerEditDialog::on_pushButton_apply_clicked()
{
    if (mCallback) {
        mCallback();
    }
    mCallback = nullptr;
    hide();
}

void MarkerEditDialog::on_pushButton_cancel_clicked()
{
    mCallback = nullptr;
    hide();
}

void MarkerEditDialog::setupPresetMenu()
{
    presets << Preset{"X, Y labeled on separate lines",
                      "X: $x\nY: $y"}

            << Preset{"X, Y, index labeled on separate lines",
                      "X: $x\nY: $y\nIndex: $i"}

            << Preset{"X, Y, values only",
                      "$x, $y"};

    foreach (Preset preset, presets) {
        mPresetMenu.addAction(preset.name, this, [=]()
        {
            ui->plainTextEdit_labelText->setPlainText(preset.text);
        });
    }

    ui->toolButton_presets->setMenu(&mPresetMenu);
}

