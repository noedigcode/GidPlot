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

#ifndef MARKEREDITDIALOG_H
#define MARKEREDITDIALOG_H

#include <QDialog>

#include <QMenu>
#include <functional>

namespace Ui {
class MarkerEditDialog;
}

class MarkerEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MarkerEditDialog(QWidget *parent = nullptr);
    ~MarkerEditDialog();

    void edit(QString text, bool showHline, bool showVline, bool showDot,
              std::function<void(void)> callback);

    QString text();
    bool showHline();
    bool showVline();
    bool showDot();

    struct Preset {
        QString name;
        QString text;
    };
    QList<Preset> presets;

private slots:
    void on_pushButton_apply_clicked();
    void on_pushButton_cancel_clicked();

private:
    Ui::MarkerEditDialog *ui;

    QMenu mPresetMenu;
    void setupPresetMenu();

    std::function<void(void)> mCallback = nullptr;
};

#endif // MARKEREDITDIALOG_H
