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
    QRect clip = clipRect();

    // Draw circle
    if (showCircle) {
        double w = size / 2.0;
        QRectF rect(center - QPointF(w, w), center + QPointF(w, w));
        if (clip.intersects(rect.toRect())) {
            painter->drawEllipse(rect);
        }
    }

    // Horizontal line
    if (horizontalLine) {
        if ((center.y() > clip.top()) && (center.y() < clip.bottom())) {
            painter->drawLine(QLineF(clip.left(), center.y(), clip.right(), center.y()));
        }
    }

    // Vertical line
    if (verticalLine) {
        if ((center.x() > clip.left()) && (center.x() < clip.right())) {
            painter->drawLine(QLineF(center.x(), clip.top(), center.x(), clip.bottom()));
        }
    }
}

QPointF PlotMarkerItem::anchorPixelPosition(int /*anchorId*/) const
{
    return position->pixelPosition();
}
