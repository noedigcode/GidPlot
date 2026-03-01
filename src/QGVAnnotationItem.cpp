#include "QGVAnnotationItem.h"

#include "QGVMap.h"

#include <QPainter>
#include <QPainterPath>
#include <QFontMetricsF>
#include <QtMath>

QGVAnnotationItem::QGVAnnotationItem()
    : QGVDrawItem()
{
    setFlag(QGV::ItemFlag::Movable, true);
    setFlag(QGV::ItemFlag::Clickable, true);
    setFlag(QGV::ItemFlag::IgnoreScale, true);
    setFlag(QGV::ItemFlag::IgnoreAzimuth, true);
}

void QGVAnnotationItem::setAnchor(const QGV::GeoPos& anchor)
{
    mAnchorGeoPos = anchor;
    updateAnchor();
    updateGeometry();

    QGVDrawItem::resetBoundary();
    QGVDrawItem::refresh();
}

void QGVAnnotationItem::setText(const QString& text)
{
    mText = text;

    updateGeometry();
    QGVDrawItem::resetBoundary();
}

QString QGVAnnotationItem::text()
{
    return mText;
}

void QGVAnnotationItem::setBoxOffset(const QPointF& offset)
{
    mBoxOffset = offset;
    QGVDrawItem::resetBoundary();
}

QPointF QGVAnnotationItem::projAnchor() const
{
    // This is the anchor from which the object is drawn, which is important
    // to specify for this object as scale is being ignored, but we are drawing
    // with an offset to the anchor position.
    return mAnchorProjPos;
}

void QGVAnnotationItem::onProjection(QGVMap *geoMap)
{
    QGVDrawItem::onProjection(geoMap);

    updateAnchor();
    updateGeometry();
}

void QGVAnnotationItem::onCamera(const QGVCameraState &oldState, const QGVCameraState &newState)
{
    updateAnchor();
    updateGeometry();

    QGVDrawItem::onCamera(oldState, newState);
}

QPainterPath QGVAnnotationItem::projShape() const
{
    if (!getMap()) { return {}; }

    // Create a shape/path that represents the actual object shape for
    // determining draw update area and selection hit testing.

    QPainterPath path;

    // Text box
    path.addRect(mTextBoxProjRects.boxRect);

    // Arrow line
    QPainterPath linePath;
    linePath.moveTo(mArrowTailProjPos);
    linePath.lineTo(mAnchorProjPos);

    QPainterPathStroker stroker;
    stroker.setWidth(2.0);
    path.addPath(stroker.createStroke(linePath));

    // Anchor dot
    path.addRect(mDotProjRect);

    return path;
}

void QGVAnnotationItem::projPaint(QPainter* painter)
{
    if (!getMap()) { return; }

    // Anchor dot
    painter->setPen(mDotPen);
    painter->setBrush(mDotFillBrush);
    painter->drawEllipse(mDotProjRect);

    // Arrow Line
    painter->setPen(QPen(Qt::black, mArrowLineWidth, Qt::SolidLine, Qt::RoundCap));
    painter->drawLine(mArrowTailProjPos, mAnchorProjPos);

    // Arrow Head
    {
        QPointF dir = mAnchorProjPos - mArrowTailProjPos;
        double  len = qSqrt(dir.x() * dir.x() + dir.y() * dir.y());
        if (len > 1e-3) {
            dir /= len;
            QPointF perp(-dir.y(), dir.x());
            QPolygonF head;
            head << mAnchorProjPos
                 << mAnchorProjPos - dir * mArrowHeadLen + perp * mArrowHeadWidthHalf
                 << mAnchorProjPos - dir * mArrowHeadLen - perp * mArrowHeadWidthHalf;
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::black);
            painter->drawPolygon(head);
        }
    }

    // Box
    painter->setPen(mBoxPen);
    painter->setBrush(mBoxFillBrush);
    painter->drawRect(mTextBoxProjRects.boxRect);

    // Text
    painter->setFont(mFont);
    painter->setPen(Qt::black);
    painter->drawText(mTextBoxProjRects.textRect, Qt::TextDontClip|mTextAlignment, mText);
}

void QGVAnnotationItem::projOnMouseClick(const QPointF& /*projPos*/)
{

}

void QGVAnnotationItem::projOnMouseRightClick(const QPointF &projPos)
{
    emit rightClicked(getMap()->mapFromProj(projPos));
}

void QGVAnnotationItem::projOnMouseDoubleClick(const QPointF &projPos)
{
    emit doubleClicked(getMap()->mapFromProj(projPos));
}

void QGVAnnotationItem::projOnObjectStartMove(const QPointF& mouseProjPos)
{
    mDragStartOffset   = mBoxOffset;
    mDragStartProjPos  = mouseProjPos;
    QPoint mousePixelPos = getMap()->mapFromProj(mouseProjPos);
    mDragStartPixelPos = mousePixelPos;

    // Only allow move when dragging from inside the text box, not on the
    // arrow line or dot.
    // Must use the screen pixel values, as this object ignores scaling and
    // could be at different project coordinates based on zoom.
    QPoint anchorPixelPos = getMap()->mapFromProj(mAnchorProjPos);
    QRectF pixelBoxRect = getTextBoxRects(anchorPixelPos).boxRect;
    mDragValid = pixelBoxRect.contains(mousePixelPos);
}

void QGVAnnotationItem::projOnObjectMovePos(const QPointF& mouseProjPos)
{
    if (!getMap()) { return; }

    if (!mDragValid) { return; }

    QPoint mousePixelPos = getMap()->mapFromProj(mouseProjPos);
    QPointF delta = mousePixelPos - mDragStartPixelPos;

    mBoxOffset = mDragStartOffset + delta;
    updateGeometry();

    QGVDrawItem::resetBoundary();
    QGVDrawItem::repaint();
}

void QGVAnnotationItem::projOnObjectStopMove(const QPointF& /*projPos*/)
{

}

QGVAnnotationItem::TextBoxRects QGVAnnotationItem::getTextBoxRects(const QPointF& anchorProjPos) const
{
    TextBoxRects ret;

    QFontMetricsF fm(mFont);

    ret.textRect = fm.boundingRect(QRectF(),
                Qt::TextDontClip|mTextAlignment, mText);

    // Offset from anchor is upwards and right to box bottom left, to match
    // behaviour of other plots.
    QPointF topLeft = anchorProjPos + mBoxOffset
            + QPointF(mTextPadding, -mTextPadding)
            - ret.textRect.topLeft()
            + QPointF(0, -ret.textRect.height());
    ret.textRect.translate(topLeft);

    ret.boxRect = ret.textRect.adjusted(-mTextPadding, -mTextPadding,
                                        mTextPadding, mTextPadding);

    return ret;
}

QPointF QGVAnnotationItem::closestPointOnRect(const QRectF& rect, const QPointF& point)
{
    QPointF anchor;

    if (point.x() < rect.left()) {
        if (point.y() > rect.bottom()) {
            anchor = rect.bottomLeft();
        } else if (point.y() < rect.top()) {
            anchor = rect.topLeft();
        } else {
            // Center left
            anchor = QPointF(rect.left(), rect.center().y());
        }
    } else if (point.x() > rect.right()) {
        if (point.y() > rect.bottom()) {
            anchor = rect.bottomRight();
        } else if (point.y() < rect.top()) {
            anchor = rect.topRight();
        } else {
            // Center right
            anchor = QPointF(rect.right(), rect.center().y());
        }
    } else if (point.y() < rect.top()) {
        // Center top
        anchor = QPointF(rect.center().x(), rect.top());
    } else {
        // Center bottom
        anchor = QPointF(rect.center().x(), rect.bottom());
    }

    return anchor;
}

void QGVAnnotationItem::updateAnchor()
{
    if (!getMap()) { return; }
    mAnchorProjPos = getMap()->getProjection()->geoToProj(mAnchorGeoPos);
}

void QGVAnnotationItem::updateGeometry()
{
    mTextBoxProjRects = getTextBoxRects(mAnchorProjPos);
    mArrowTailProjPos = closestPointOnRect(mTextBoxProjRects.boxRect, mAnchorProjPos);
    mDotProjRect = QRectF(mAnchorProjPos.x() - mDotSize / 2.0,
                          mAnchorProjPos.y() - mDotSize / 2.0,
                          mDotSize,
                          mDotSize);
}
