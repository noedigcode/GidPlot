#include "PlotMarkerItem.h"

PlotMarkerItem::PlotMarkerItem(QCustomPlot* parentPlot) :
    QCPAbstractItem(parentPlot),
    position(createPosition("position")),
    anchor(createAnchor("anchor", 0))
{
    position->setCoords(0, 0);
}

double PlotMarkerItem::selectTest(const QPointF& /*pos*/, bool /*onlySelectable*/,
                                  QVariant* /*details*/) const
{
    return -1;
}

void PlotMarkerItem::draw(QCPPainter* painter)
{
    painter->setPen(pen);
    painter->setBrush(brush);
    QPointF center(position->pixelPosition());
    double w = size / 2.0;
    QRectF rect(center - QPointF(w, w), center + QPointF(w, w));
    if (clipRect().intersects(rect.toRect())) {
        painter->drawEllipse(rect);
    }
}

QPointF PlotMarkerItem::anchorPixelPosition(int /*anchorId*/) const
{
    return position->pixelPosition();
}
