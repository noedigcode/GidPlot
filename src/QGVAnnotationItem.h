/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2026 Gideon van der Kolf
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

#pragma once

#include <QGVDrawItem.h>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QPen>

/* A map annotation: a text box with an arrow pointing to a geo coordinate.
 *
 * The anchor point is fixed in map coordinates and moves with the map.
 * The text box is positioned via a pixel offset from the anchor and can
 * be dragged by the user.
 */
class QGVAnnotationItem : public QGVDrawItem
{
    Q_OBJECT

public:
    QGVAnnotationItem();

    void setAnchor(const QGV::GeoPos& anchor);
    void setText(const QString& text);
    QString text();
    void setBoxOffset(const QPointF& offset);

    // QGVDrawItem interface
    QPointF projAnchor() const override;
    void onProjection(QGVMap* geoMap) override;
    void onCamera(const QGVCameraState& oldState, const QGVCameraState& newState) override;
    QPainterPath projShape() const override;
    void projPaint(QPainter* painter) override;

    // Drag interface
    void projOnMouseClick(const QPointF& projPos) override;
    void projOnMouseRightClick(const QPointF& projPos) override;
    void projOnMouseDoubleClick(const QPointF& projPos) override;
    void projOnObjectStartMove(const QPointF& mouseProjPos) override;
    void projOnObjectMovePos(const QPointF& mouseProjPos) override;
    void projOnObjectStopMove(const QPointF& projPos) override;

signals:
    void rightClicked(QPoint pixelPos);
    void doubleClicked(QPoint pixelPos);

private:
    struct TextBoxRects {
        QRectF textRect;
        QRectF boxRect;
    };
    TextBoxRects getTextBoxRects(const QPointF &anchorProjPos) const;
    static QPointF closestPointOnRect(const QRectF& rect, const QPointF& point);

    void updateAnchor();
    void updateGeometry();

    QGV::GeoPos mAnchorGeoPos;
    QPointF mAnchorProjPos;
    TextBoxRects mTextBoxProjRects;
    QPointF mArrowTailProjPos;
    QRectF mDotProjRect;
    QString mText;

    QPointF mBoxOffset {10.0, -10.0};
    QBrush mBoxFillBrush {QColor("#f8fabe")};
    QPen mBoxPen {Qt::black};
    QBrush mDotFillBrush {QColor(255, 0, 0, 100)};
    QPen mDotPen;
    int mDotSize = 10;

    // Drag state
    bool mDragValid = false;
    QPointF mDragStartOffset;
    QPointF mDragStartProjPos;
    QPoint mDragStartPixelPos;

    double mTextPadding = 2.0;
    Qt::Alignment mTextAlignment = Qt::AlignCenter;
    QFont mFont;
    double mArrowLineWidth = 1;
    double mArrowHeadLen = 10;
    double mArrowHeadWidthHalf = 4;
    int mFontSize = 9;
};
