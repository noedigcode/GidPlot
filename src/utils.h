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

#ifndef UTILS_H
#define UTILS_H

#include <QTimer>

#include <functional>

class Utils
{
public:

    static void callLater(QObject* context, std::function<void(void)> func)
    {
        // A timer with 0 timeout will queue the function at the end of the event
        // loop.
        QTimer::singleShot(0, context, func);
    }
};

#endif // UTILS_H
