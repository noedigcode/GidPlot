#pragma once

#include <QGVDrawItem.h>

#include <QBrush>
#include <QPainter>
#include <QPen>

class MapMarker : public QGVDrawItem
{
    Q_OBJECT

public:
    explicit MapMarker(const QGV::GeoPos &pos, const float radius);

    void setPosition(QGV::GeoPos pos);

    void setWidth(int width);
    QPen pen();
    void setPen(QPen pen);
    QBrush brush();
    void setBrush(QBrush brush);

private:
    void onProjection(QGVMap* geoMap) override;
    void onUpdate() override;
    QPainterPath projShape() const override;
    void projPaint(QPainter* painter) override;
    QPointF projAnchor() const override;

    void updateGeometry();

    QPen mPen {Qt::black};
    QBrush mBrush {QColor(255, 255, 255, 150)};
    QGV::GeoPos mPos;
    float mRadius = 5;

    // Projected points onto painting area
    QPointF mProjPos;
    QRectF mProjRect;
};
