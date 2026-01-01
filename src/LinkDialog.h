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

#ifndef LINKDIALOG_H
#define LINKDIALOG_H

#include "plotwindow.h"

#include <QDialog>

namespace Ui {
class LinkDialog;
}

class LinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LinkDialog(QWidget *parent = nullptr);
    ~LinkDialog();

    void show(QList<PlotWindow*> plotWindows, LinkPtr selectLink);

private:
    Ui::LinkDialog *ui;

};

#endif // LINKDIALOG_H
