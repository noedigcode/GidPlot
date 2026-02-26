#pragma once

#include <QGVDrawItem.h>
#include <QString>
#include <QPointF>
#include <QRectF>

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

    /** Pixel offset of the box centre relative to the anchor screen position. */
    void    setBoxOffset(const QPointF& offset);
    QPointF boxOffset() const { return mBoxOffset; }

    // QGVDrawItem interface
    void onProjection(QGVMap* geoMap) override;
    QPainterPath projShape() const override;
    void         projPaint(QPainter* painter) override;

    // Drag interface
    void projOnObjectStartMove(const QPointF& projPos) override;
    void projOnObjectMovePos(const QPointF& projPos) override;
    void projOnObjectStopMove(const QPointF& projPos) override;

private:
    QRectF   boxRect(const QPointF& anchorScreen) const;
    QPointF  anchorToScreen() const;
    static QPointF closestPointOnRect(const QRectF& rect, const QPointF& point);

    void updateGeometry();

    QGV::GeoPos mAnchor;
    QRectF mBoxProjRect;
    QPointF mArrowProjPos;
    QString     mText;
    QPointF     mBoxOffset       {-120.0, -80.0};

    // Drag state
    QPointF     mDragStartOffset;
    QPointF     mDragStartProjPos;

    static constexpr double k_boxWidth   = 120.0;
    static constexpr double k_boxHeight  =  36.0;
    static constexpr double k_padding    =   8.0;
    static constexpr double k_arrowWidth =   1.5;
    static constexpr int    k_fontSize   =   9;
};
