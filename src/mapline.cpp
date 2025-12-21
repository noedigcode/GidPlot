
#include "mapline.h"


MapLine::MapLine(const QGV::GeoPos &pos1, const QGV::GeoPos &pos2) :
    mPos1(pos1),
    mPos2(pos2)
{
    mPen.setCosmetic(true); // Cosmetic: Line will be the same thickness for all zoom levels.
    mPen.setWidth(3);
}

QColor MapLine::color()
{
    return mPen.color();
}

void MapLine::setColor(QColor color)
{
    mPen.setColor(color);
    repaint();
}

void MapLine::setWidth(int width)
{
    mPen.setWidth(width);
    repaint();
}

void MapLine::setPen(QPen pen)
{
    mPen = pen;
    repaint();
}

void MapLine::onProjection(QGVMap* geoMap)
{
    QGVDrawItem::onProjection(geoMap);
    mProjPos1 = geoMap->getProjection()->geoToProj(mPos1);
    mProjPos2 = geoMap->getProjection()->geoToProj(mPos2);
}

void MapLine::onUpdate()
{
    QGVDrawItem::onUpdate();
}

QPainterPath MapLine::projShape() const
{
    QPainterPath path(mProjPos1);
    path.lineTo(mProjPos2);
    return path;
}

void MapLine::projPaint(QPainter* painter)
{
    painter->setPen(mPen);
    painter->drawLine(mProjPos1, mProjPos2);
}

QPointF MapLine::projAnchor() const
{
    return mProjPos1;
}

