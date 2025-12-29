#include "plot.h"


Plot::Plot(QWidget *parentWidget)
    : QObject{parentWidget}, mParentWidget(parentWidget)
{
    setupPlotMenu();
    setupCrosshairsDialog();
}

void Plot::storeAndDisableCrosshairs()
{
    mLastPlotCrosshairVisible = plotCrosshairVisible();
    setPlotCrosshairVisible(false);
    mLastMouseCrosshairVisible = mouseCrosshairVisible();
    setMouseCrosshairVisible(false);
}

void Plot::restoreCrosshairs()
{
    setPlotCrosshairVisible(mLastPlotCrosshairVisible);
    setMouseCrosshairVisible(mLastMouseCrosshairVisible);
}

void Plot::setupPlotMenu()
{
    plotMenu.parentWidget = mParentWidget;
    plotMenu.getDataTipGraphCallback = [this]() { return dataTipGraph; };
    plotMenu.getGraphsCallback = [this]() { return graphs; };
    plotMenu.getPlotCrosshairIndexCallback = [=]()
    {
        return mPlotCrosshairIndex;
    };

    connect(plotMenu.actionPlaceMarker, &QAction::triggered,
            this, &Plot::onActionPlaceMarkerTriggered);
    connect(plotMenu.actionMeasure, &QAction::triggered,
            this, &Plot::onActionMeasureTriggered);
    connect(plotMenu.actionShowAll, &QAction::triggered,
            this, &Plot::showAll);
    connect(plotMenu.actionEqualAxes, &QAction::triggered,
            this, &Plot::onActionEqualAxesTriggered);
    connect(plotMenu.actionCrosshairs, &QAction::triggered,
            this, &Plot::showCrosshairsDialog);
    connect(plotMenu.actionLink, &QAction::triggered,
            this, &Plot::linkSettingsTriggered);
}

void Plot::showCrosshairsDialog()
{
    mCrosshairsDialog.show(crosshairsDialogAboutToShow());
}

void Plot::setupCrosshairsDialog()
{
    connect(&mCrosshairsDialog, &CrosshairsDialog::settingsChanged,
            this, &Plot::crosshairsDialogChanged);
}

