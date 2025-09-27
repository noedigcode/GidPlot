#ifndef PLOTMARKERITEM_H
#define PLOTMARKERITEM_H

#include "QCustomPlot/qcustomplot.h"

#include <QObject>

class PlotMarkerItem : public QCPAbstractItem
{
    Q_OBJECT
public:
    explicit PlotMarkerItem(QCustomPlot* parentPlot);

    QCPItemPosition* const position;
    QCPItemAnchor* const anchor;
    QPen pen;
    QBrush brush;
    double size;
    bool showCircle = true;
    bool verticalLine = false;
    bool horizontalLine = false;

    double selectTest(const QPointF& pos, bool onlySelectable,
                      QVariant* details) const override;

protected:
    void draw(QCPPainter* painter) override;
    QPointF anchorPixelPosition(int anchorId) const override;
};

#endif // PLOTMARKERITEM_H
