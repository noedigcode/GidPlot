#ifndef PLOTPROPERTIESDIALOG_H
#define PLOTPROPERTIESDIALOG_H

#include "plot.h"

#include <QDialog>


namespace Ui {
class PlotPropertiesDialog;
}

class PlotPropertiesDialog : public QDialog
{
    Q_OBJECT

public:

    explicit PlotPropertiesDialog(QWidget *parent = nullptr);
    ~PlotPropertiesDialog();

    void setPlotProperties(Plot::Properties p);
    Plot::Properties getPlotProperties();

    void showProperties(PlotPtr plot);

signals:
    void plotPropertiesChanged(PlotPtr plot, Plot::Properties p);

private:
    Ui::PlotPropertiesDialog *ui;

    PlotPtr mPlot;

    bool mBusySettingProperties = false;

    void connectWidgets();
    void emitPlotPropertiesChanged();
    void setupCurvesTree(PlotPtr plot);

    void prepAndShow(PlotPtr plot);
};

#endif // PLOTPROPERTIESDIALOG_H
