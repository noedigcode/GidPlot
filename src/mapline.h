/* MapLine
 *
 * Draws a line on a QGeoView map.
 *
 */

#pragma once

#include <QGVDrawItem.h>

#include <QBrush>
#include <QPainter>
#include <QPen>

class MapLine : public QGVDrawItem
{
    Q_OBJECT

public:
    explicit MapLine(const QGV::GeoPos &pos1, const QGV::GeoPos &pos2);

    QColor color();
    void setColor(QColor color);
    void setWidth(int width);
    void setPen(QPen pen);

private:
    void onProjection(QGVMap* geoMap) override;
    void onUpdate() override;
    QPainterPath projShape() const override;
    void projPaint(QPainter* painter) override;
    QPointF projAnchor() const override;

    // Start and end coordinates of line
    QGV::GeoPos mPos1;
    QGV::GeoPos mPos2;
    // Projected points of line on to painting area
    QPointF mProjPos1;
    QPointF mProjPos2;

    QPen mPen {Qt::red};
};
