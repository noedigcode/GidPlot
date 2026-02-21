#ifndef QGVCROSSHAIRWIDGET_H
#define QGVCROSSHAIRWIDGET_H

#include "QGVWidget.h"

class QGVCrosshairWidget : public QGVWidget
{
    Q_OBJECT
public:
    QGVCrosshairWidget();

    void setPos(QPoint pos);

    bool horizontalLineVisible();
    void setHorizontalLineVisible(bool visible);
    bool verticalLineVisible();
    void setVerticalLineVisible(bool visible);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint mPos;
    bool mHorizontalLineVisible = true;
    bool mVerticalLineVisible = true;
};

#endif // QGVCROSSHAIRWIDGET_H
