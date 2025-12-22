
#include "mapMarker.h"


MapMarker::MapMarker(const QGV::GeoPos &pos, const float radius) :
    mPos(pos), mRadius(radius)
{
    setFlags(QGV::ItemFlag::IgnoreAzimuth | QGV::ItemFlag::IgnoreScale);

    mPen.setCosmetic(true); // Cosmetic: Line will be the same thickness for all zoom levels.
    mPen.setWidth(2);
}

void MapMarker::setPosition(QGV::GeoPos pos)
{
    mPos = pos;

    updateGeometry();

    resetBoundary();
    refresh();
}

void MapMarker::setWidth(int width)
{
    mPen.setWidth(width);
    repaint();
}

QPen MapMarker::pen()
{
    return mPen;
}

void MapMarker::setPen(QPen pen)
{
    mPen = pen;
    repaint();
}

QBrush MapMarker::brush()
{
    return mBrush;
}

void MapMarker::setBrush(QBrush brush)
{
    mBrush = brush;
    repaint();
}

void MapMarker::onProjection(QGVMap* geoMap)
{
    QGVDrawItem::onProjection(geoMap);
    updateGeometry();
}

void MapMarker::onUpdate()
{
    QGVDrawItem::onUpdate();
}

QPainterPath MapMarker::projShape() const
{
    QPainterPath path;
    path.addRect(mProjRect);
    return path;
}

void MapMarker::projPaint(QPainter* painter)
{
    painter->setPen(mPen);
    painter->setBrush(mBrush);
    painter->drawEllipse(mProjRect);
}

QPointF MapMarker::projAnchor() const
{
    return mProjPos;
}

void MapMarker::updateGeometry()
{
    mProjPos = getMap()->getProjection()->geoToProj(mPos);
    mProjRect = QRectF(mProjPos - QPointF(mRadius, mRadius),
                       mProjPos + QPointF(mRadius, mRadius));
}

