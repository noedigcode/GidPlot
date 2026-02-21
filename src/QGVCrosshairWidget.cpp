#include "QGVCrosshairWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

QGVCrosshairWidget::QGVCrosshairWidget()
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void QGVCrosshairWidget::setPos(QPoint pos)
{
    mPos = pos;
    update();
}

bool QGVCrosshairWidget::horizontalLineVisible()
{
    return mHorizontalLineVisible;
}

void QGVCrosshairWidget::setHorizontalLineVisible(bool visible)
{
    mHorizontalLineVisible = visible;
    update();
}

bool QGVCrosshairWidget::verticalLineVisible()
{
    return mVerticalLineVisible;
}

void QGVCrosshairWidget::setVerticalLineVisible(bool visible)
{
    mVerticalLineVisible = visible;
    update();
}

void QGVCrosshairWidget::paintEvent(QPaintEvent* /*event*/)
{
    if (size().isEmpty()) { return; }

    QPainter painter(this);

    if (mHorizontalLineVisible) {
        painter.drawLine(0, mPos.y(), width(), mPos.y());
    }
    if (mVerticalLineVisible) {
        painter.drawLine(mPos.x(), 0, mPos.x(), height());
    }
}
