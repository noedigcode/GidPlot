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

    const int mCurveTreeColName = 1;
    const int mCurveTreeColColor = 0;

    QMap<QTreeWidgetItem*, GraphPtr> mTreeItemGraphMap;
    void setItemColorIcon(QTreeWidgetItem* item, QColor color);

private slots:
    void on_treeWidget_curves_itemChanged(QTreeWidgetItem *item, int column);
    void on_treeWidget_curves_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_toolButton_curve_remove_clicked();
};

#endif // PLOTPROPERTIESDIALOG_H
