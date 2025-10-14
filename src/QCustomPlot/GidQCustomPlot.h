#ifndef GIDQCUSTOMPLOT_H
#define GIDQCUSTOMPLOT_H

#include "qcustomplot.h"


class GidQCustomPlot : public QCustomPlot
{
    Q_OBJECT
public:
    GidQCustomPlot(QWidget *parent = nullptr);

    bool saveSvg(QBuffer* buffer);
};

#endif // GIDQCUSTOMPLOT_H
