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

#ifndef POPOUTTABWIDGET_H
#define POPOUTTABWIDGET_H

#include <QTabWidget>
#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QMainWindow>
#include <QMap>

class PopoutTabWidget : public QTabWidget
{
    Q_OBJECT
public:
    PopoutTabWidget(QWidget* parent = nullptr);

    void useFirstTabAsPlaceholder(bool set);
    int addTab(QWidget *widget, QString title);
    int addTab(QWidget* widget, const QIcon &icon, QString title);
    void closeAllWindows();
    int tabAndWindowCount();
    bool isPoppedOut(QWidget* widget);
    QMainWindow* tabWindow(QWidget* widget);

public slots:
    void popoutCurrentTab();
    void popoutTabWidget(QWidget* w);

private:
    bool eventFilter(QObject *watched, QEvent *event);

    QWidget* mPlaceholderTab = nullptr;
    QPushButton* mPopoutButton = nullptr;

    void updatePopoutButtonVisibility();

    QMap<QWidget*, QMainWindow*> tabWindows;
};

#endif // POPOUTTABWIDGET_H
