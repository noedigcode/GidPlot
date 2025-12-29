#ifndef PLOT_H
#define PLOT_H

#include "link.h"
#include "graph.h"
#include "CrosshairsDialog.h"

#include <QObject>
#include <QWidget>

class Plot : public QObject
{
    Q_OBJECT
public:
    explicit Plot(QWidget *parentWidget = nullptr);
    virtual ~Plot() = default;

    LinkPtr link {new Link()};

    virtual void showAll() = 0;
    virtual bool plotCrosshairVisible() = 0;
    virtual void setPlotCrosshairVisible(bool visible) = 0;
    virtual bool mouseCrosshairVisible() = 0;
    virtual void setMouseCrosshairVisible(bool visible) = 0;

    void storeAndDisableCrosshairs();
    void restoreCrosshairs();

signals:
    void axisRangesChanged(int linkGroup, QRectF xyrange);
    void dataTipChanged(int linkGroup, int index);
    void linkSettingsTriggered();

protected:
    QWidget* mParentWidget = nullptr;

    // -----------------------------------------------------------------------
    // Menus
protected:
    PlotMenu plotMenu;
protected slots:
    virtual void onActionPlaceMarkerTriggered() = 0;
    virtual void onActionMeasureTriggered() = 0;
    virtual void onActionEqualAxesTriggered() = 0;

    // -----------------------------------------------------------------------
    // Crosshairs
protected:
    CrosshairsDialog mCrosshairsDialog;
    int mPlotCrosshairIndex = 0;
    void showCrosshairsDialog();
    virtual CrosshairsDialog::Settings crosshairsDialogAboutToShow() = 0;
    virtual void crosshairsDialogChanged(CrosshairsDialog::Settings s) = 0;

private:
    void setupCrosshairsDialog();
    bool mLastPlotCrosshairVisible = false;
    bool mLastMouseCrosshairVisible = false;

    // -----------------------------------------------------------------------
    // Graphs
protected:
    GraphPtr dataTipGraph;
    QList<GraphPtr> graphs;

private:
    void setupPlotMenu();
};

typedef QSharedPointer<Plot> PlotPtr;

#endif // PLOT_H
