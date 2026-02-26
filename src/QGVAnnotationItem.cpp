#include "QGVAnnotationItem.h"

#include <QGVMap.h>
#include <QPainter>
#include <QPainterPath>
#include <QFontMetricsF>
#include <QtMath>

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
QGVAnnotationItem::QGVAnnotationItem()
    : QGVDrawItem()
{
    setFlag(QGV::ItemFlag::Movable, true);
    setFlag(QGV::ItemFlag::IgnoreScale, true);
    setFlag(QGV::ItemFlag::IgnoreAzimuth, true);
}

// ─────────────────────────────────────────────
//  Public API
// ─────────────────────────────────────────────
void QGVAnnotationItem::setAnchor(const QGV::GeoPos& anchor)
{
    mAnchor = anchor;
    resetBoundary();
}

void QGVAnnotationItem::setText(const QString& text)
{
    mText = text;
    repaint();
}

void QGVAnnotationItem::setBoxOffset(const QPointF& offset)
{
    mBoxOffset = offset;
    resetBoundary();
}

void QGVAnnotationItem::onProjection(QGVMap *geoMap)
{
    QGVDrawItem::onProjection(geoMap);
    updateGeometry();
}

// ─────────────────────────────────────────────
//  QGVDrawItem interface
// ─────────────────────────────────────────────
QPainterPath QGVAnnotationItem::projShape() const
{
    if (!getMap())
        return {};

    QPointF anchorScene = getMap()->getProjection()->geoToProj(mAnchor);

    // Use a generous margin in scene coords to cover the box at any zoom level.
    const double margin = qMax(qAbs(mBoxOffset.x()), qAbs(mBoxOffset.y()))
                        + k_boxWidth + k_boxHeight + 20.0;

    QPainterPath path;
    path.addRect(anchorScene.x() - margin, anchorScene.y() - margin,
                 margin * 2.0, margin * 2.0);
    return path;
}

void QGVAnnotationItem::projPaint(QPainter* painter)
{
    if (!getMap()) { return; }

    QPointF anchorScreen = mArrowProjPos; //anchorToScreen();
    QRectF  box          = boxRect(anchorScreen);
    QPointF arrowTail    = closestPointOnRect(box, anchorScreen);

//    painter->save();

    // Reset transform so everything is drawn in screen pixels, never scaling with zoom.
//    painter->setTransform(QTransform());

    // --- Arrow line ---
    painter->setPen(QPen(Qt::black, k_arrowWidth, Qt::SolidLine, Qt::RoundCap));
    painter->drawLine(anchorScreen, arrowTail);

    // --- Arrowhead ---
    {
        QPointF dir = anchorScreen - arrowTail;
        double  len = qSqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (len > 1e-3) {
            dir /= len;
            QPointF perp(-dir.y(), dir.x());
            const double headLen  = 8.0;
            const double headHalf = 4.0;
            QPolygonF head;
            head << anchorScreen
                 << anchorScreen - dir * headLen + perp * headHalf
                 << anchorScreen - dir * headLen - perp * headHalf;
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::black);
            painter->drawPolygon(head);
        }
    }

    // --- Box ---
    painter->setPen(QPen(Qt::black, 1));
    painter->setBrush(Qt::white);
    painter->drawRect(box);

    // --- Text ---
    QFont font = painter->font();
    font.setPointSize(k_fontSize);
    painter->setFont(font);
    painter->setPen(Qt::black);
    painter->drawText(box.adjusted(k_padding, 0, -k_padding, 0),
                      Qt::AlignVCenter | Qt::AlignLeft | Qt::TextWordWrap,
                      mText);

    // --- Anchor dot ---
    painter->setPen(Qt::NoPen);
    painter->setBrush(Qt::black);
    painter->drawEllipse(anchorScreen, 3.0, 3.0);

//    painter->restore();
}

// ─────────────────────────────────────────────
//  Drag handlers
// ─────────────────────────────────────────────
void QGVAnnotationItem::projOnObjectStartMove(const QPointF& projPos)
{
    mDragStartOffset   = mBoxOffset;
    mDragStartProjPos  = projPos;
}

void QGVAnnotationItem::projOnObjectMovePos(const QPointF& projPos)
{
    if (!getMap())
        return;

    // projPos is in scene coordinates; convert delta to screen pixels.
    QPointF startScreen = getMap()->mapFromProj(mDragStartProjPos);
    QPointF currScreen  = getMap()->mapFromProj(projPos);
    QPointF delta       = currScreen - startScreen;

    mBoxOffset = mDragStartOffset + delta;
    resetBoundary();
}

void QGVAnnotationItem::projOnObjectStopMove(const QPointF& /*projPos*/)
{
    // Nothing extra needed; mBoxOffset is already up to date.
}

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
QPointF QGVAnnotationItem::anchorToScreen() const
{
    return getMap()->mapFromProj(
        getMap()->getProjection()->geoToProj(mAnchor));
}

QRectF QGVAnnotationItem::boxRect(const QPointF& anchorScreen) const
{
    QPointF centre = anchorScreen + mBoxOffset;
    return QRectF(centre.x() - k_boxWidth  / 2.0,
                  centre.y() - k_boxHeight / 2.0,
                  k_boxWidth,
                  k_boxHeight);
}

QPointF QGVAnnotationItem::closestPointOnRect(const QRectF& rect, const QPointF& point)
{
    double cx = qBound(rect.left(), point.x(), rect.right());
    double cy = qBound(rect.top(),  point.y(), rect.bottom());

    if (rect.contains(point)) {
        double dLeft   = point.x() - rect.left();
        double dRight  = rect.right()  - point.x();
        double dTop    = point.y() - rect.top();
        double dBottom = rect.bottom() - point.y();
        double minD    = qMin(qMin(dLeft, dRight), qMin(dTop, dBottom));
        if      (minD == dLeft)   cx = rect.left();
        else if (minD == dRight)  cx = rect.right();
        else if (minD == dTop)    cy = rect.top();
        else                      cy = rect.bottom();
    }

    return {cx, cy};
}

void QGVAnnotationItem::updateGeometry()
{
    mArrowProjPos = getMap()->getProjection()->geoToProj(mAnchor);
}
