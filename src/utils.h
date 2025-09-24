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

    enum Match {NoMatch, ExactMatch, ContainsMatch};
    static Match looksLikeTimeTitle(QString text)
    {
        // Prefer a title with one of these exact names
        QStringList exact {"time", "t"};
        // Otherwise opt for one that contains one of these
        QStringList contain {"time"};

        text = text.trimmed().toLower();
        foreach (QString x, exact) {
            if (text == x) {
                return ExactMatch;
            }
        }
        foreach (QString c, contain) {
            if (text.contains(c)) {
                return ContainsMatch;
            }
        }
        return NoMatch;
    }

    static QString elidedText(QString text, int maxLength)
    {
        if (text.length() > maxLength) {
            return text.left(maxLength) + "…";
        } else {
            return text;
        }
    }
};

#endif // UTILS_H
