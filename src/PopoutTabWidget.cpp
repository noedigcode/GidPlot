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

#include "PopoutTabWidget.h"

#include <QEvent>
#include <QScreen>

PopoutTabWidget::PopoutTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    mPopoutButton = new QPushButton("Pop Out", this);
    setCornerWidget(mPopoutButton, Qt::TopRightCorner);
    connect(mPopoutButton, &QPushButton::clicked,
            this, &PopoutTabWidget::popoutCurrentTab);

    connect(this, &PopoutTabWidget::currentChanged, this, [=](int /*index*/)
    {
        updatePopoutButtonVisibility();
    });
}

void PopoutTabWidget::useFirstTabAsPlaceholder(bool set)
{
    if (set) {
        mPlaceholderTab = this->widget(0);
    } else {
        mPlaceholderTab = nullptr;
    }
    updatePopoutButtonVisibility();
}

int PopoutTabWidget::addTab(QWidget *widget, QString title)
{
    return addTab(widget, QIcon(), title);
}

int PopoutTabWidget::addTab(QWidget *widget, const QIcon &icon, QString title)
{
    int tabIndex = QTabWidget::addTab(widget, icon, title);

    // Replace placeholder tab if set and if present
    if (mPlaceholderTab) {
        int index = this->indexOf(mPlaceholderTab);
        if (index >= 0) {
            this->removeTab(0);
        }
    }

    return tabIndex;
}

void PopoutTabWidget::closeAllWindows()
{
    foreach (QMainWindow* window, tabWindows.values()) {
        window->close();
    }
}

int PopoutTabWidget::tabAndWindowCount()
{
    return QTabWidget::count() + tabWindows.count();
}

bool PopoutTabWidget::isPoppedOut(QWidget* widget)
{
    return tabWindows.contains(widget);
}

QMainWindow* PopoutTabWidget::tabWindow(QWidget* widget)
{
    return tabWindows.value(widget, nullptr);
}

void PopoutTabWidget::popoutCurrentTab()
{
    popoutTabWidget(currentWidget());
}

void PopoutTabWidget::popoutTabWidget(QWidget* w)
{
    if (!w) { return; }
    if (this->indexOf(w) == -1) { return; }

    // One can specify this as the new window parent upon creation so it will
    // automatically be destroyed when this object is destroyed. However, then
    // the window doesn't behave like a normal window.
    // Instead, we keep track of the windows which are then closed with the
    // closeAllWindows() function.
    QMainWindow* window = new QMainWindow();
    QIcon icon = QTabWidget::tabIcon(QTabWidget::indexOf(w));
    window->setWindowIcon(icon);
    tabWindows.insert(w, window);

    window->setWindowTitle(tabText(indexOf(w)));
    QSize size = w->size();
    QScreen* screen = window->screen();
    if (screen) {
        size.setWidth(qMin((double)size.width(), screen->size().width() * 0.8));
        size.setHeight(qMin((double)size.height(), screen->size().height() * 0.8));
    }
    window->resize(size);
    window->setCentralWidget(w);
    window->show();
    w->show();

    // Use event filter to detect window close events to pop widget back in
    window->installEventFilter(this);
}

bool PopoutTabWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        QMainWindow* window = qobject_cast<QMainWindow*>(watched);
        if (window) {
            QIcon icon = window->windowIcon();
            QWidget* w = window->takeCentralWidget();
            this->addTab(w, icon, window->windowTitle());
            tabWindows.remove(w);
        }
    }

    return QTabWidget::eventFilter(watched, event);
}

void PopoutTabWidget::updatePopoutButtonVisibility()
{
    if (!mPopoutButton) { return; }
    mPopoutButton->setEnabled(currentWidget() != mPlaceholderTab);
}


