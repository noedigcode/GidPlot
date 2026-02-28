#pragma once

#include <QGVDrawItem.h>
#include <QString>
#include <QPointF>
#include <QRectF>
#include <QPen>

/**
 * @brief A map annotation: a text box with an arrow pointing to a geo coordinate.
 *
 * The anchor point is fixed in map coordinates and moves with the map.
 * The text box is positioned via a pixel offset from the anchor and can
 * be dragged by the user. Arrow thickness and box size never change on zoom.
 *
 * Usage:
 *   QGVAnnotationItem* item = new QGVAnnotationItem();
 *   item->setAnchor(QGV::GeoPos(51.5, -0.1));
 *   item->setText("Tower of London");
 *   item->setBoxOffset(QPointF(-120, -80));
 *   map->addItem(item);
 */
class QGVAnnotationItem : public QGVDrawItem
{
    Q_OBJECT

public:
    QGVAnnotationItem();

    void setAnchor(const QGV::GeoPos& anchor);
    void setText(const QString& text);

    void setBoxOffset(const QPointF& offset);
    QPointF boxOffset() const { return mBoxOffset; }

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
