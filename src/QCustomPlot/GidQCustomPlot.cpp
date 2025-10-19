#include "GidQCustomPlot.h"

#include <QSvgGenerator>

GidQCustomPlot::GidQCustomPlot(QWidget *parent) :
    QCustomPlot(parent)
{

}

bool GidQCustomPlot::saveSvg(QBuffer *buffer)
{
    bool success = false;

    QSvgGenerator generator;
    generator.setResolution(logicalDpiX());
    generator.setOutputDevice(buffer);
    generator.setSize(size());
    generator.setViewBox(QRect(0, 0, width(), height()));

    QCPPainter painter;
    if (painter.begin(&generator)) {
        painter.setMode(QCPPainter::pmVectorized);
        painter.setMode(QCPPainter::pmNoCaching);
        painter.setMode(QCPPainter::pmNonCosmetic);
        painter.setWindow(mViewport);
        if (mBackgroundBrush.style() != Qt::NoBrush &&
                mBackgroundBrush.color() != Qt::white &&
                mBackgroundBrush.color() != Qt::transparent &&
                mBackgroundBrush.color().alpha() > 0)
        {
            // Draw background color if not white/transparent
            painter.fillRect(viewport(), mBackgroundBrush);
        }
        draw(&painter);
        painter.end();
        success = true;
    }

    return success;
}
