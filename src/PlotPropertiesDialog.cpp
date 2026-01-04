#include "PlotPropertiesDialog.h"
#include "ui_PlotPropertiesDialog.h"

PlotPropertiesDialog::PlotPropertiesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlotPropertiesDialog)
{
    ui->setupUi(this);

    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    connectWidgets();
}

PlotPropertiesDialog::~PlotPropertiesDialog()
{
    delete ui;
}

void PlotPropertiesDialog::setPlotProperties(Plot::Properties p)
{
    blockSignals(true);

    ui->groupBox_plotCrosshair->setChecked(p.plotCrosshair);
    ui->checkBox_plotHline->setChecked(p.plotHline);
    ui->checkBox_plotVline->setChecked(p.plotVline);
    ui->checkBox_plotDot->setChecked(p.plotDot);
    ui->groupBox_mouseCrosshair->setChecked(p.mouseCrosshair);
    ui->checkBox_mouseHline->setChecked(p.mouseHline);
    ui->checkBox_mouseVline->setChecked(p.mouseVline);
    ui->checkBox_mouseDot->setChecked(p.mouseDot);

    ui->checkBox_showTitle->setEnabled(p.supportShowTitle);
    ui->checkBox_showXlabel->setEnabled(p.supportXlabel);
    ui->lineEdit_xlabel->setEnabled(p.supportXlabel);
    ui->checkBox_showYlabel->setEnabled(p.supportYlabel);
    ui->lineEdit_ylabel->setEnabled(p.supportYlabel);

    ui->checkBox_showTitle->setChecked(p.showTitle);
    ui->lineEdit_title->setText(p.title);
    ui->checkBox_showXlabel->setChecked(p.showXlabel);
    ui->lineEdit_xlabel->setText(p.xlabel);
    ui->checkBox_showYlabel->setChecked(p.showYlabel);
    ui->lineEdit_ylabel->setText(p.ylabel);
    ui->checkBox_showLegend->setChecked(p.showLegend);

    blockSignals(false);
}

void PlotPropertiesDialog::showProperties(PlotPtr plot)
{
    prepAndShow(plot);
}

void PlotPropertiesDialog::showCrosshairSettings(PlotPtr plot)
{
    // TODO Crosshair settings is now the same as properties.
    //      Remove crosshair-specific functions
    prepAndShow(plot);
}

void PlotPropertiesDialog::connectWidgets()
{
    QList<QGroupBox*> groupBoxes {
        ui->groupBox_mouseCrosshair,
        ui->groupBox_plotCrosshair,

    };
    foreach (QGroupBox* gb, groupBoxes) {
        connect(gb, &QGroupBox::toggled,
                this, &PlotPropertiesDialog::emitPlotPropertiesChanged);
    }

    QList<QCheckBox*> checkBoxes {
        ui->checkBox_plotDot,
        ui->checkBox_plotHline,
        ui->checkBox_plotVline,
        ui->checkBox_mouseDot,
        ui->checkBox_mouseHline,
        ui->checkBox_mouseVline,
        ui->checkBox_showTitle,
        ui->checkBox_showXlabel,
        ui->checkBox_showYlabel,
        ui->checkBox_showLegend
    };
    foreach (QCheckBox* cb, checkBoxes) {
        connect(cb, &QCheckBox::toggled,
                this, &PlotPropertiesDialog::emitPlotPropertiesChanged);
    }

    QList<QLineEdit*> lineEdits {
        ui->lineEdit_title,
        ui->lineEdit_xlabel,
        ui->lineEdit_ylabel
    };
    foreach (QLineEdit* le, lineEdits) {
        connect(le, &QLineEdit::textEdited,
                this, &PlotPropertiesDialog::emitPlotPropertiesChanged);
    }
}

void PlotPropertiesDialog::emitPlotPropertiesChanged()
{
    emit plotPropertiesChanged(mPlot, getPlotProperties());
}

void PlotPropertiesDialog::setupCurvesTree(PlotPtr plot)
{
    // Setup curves
}

void PlotPropertiesDialog::prepAndShow(PlotPtr plot)
{
    mPlot = plot;
    setPlotProperties(plot->getPlotProperties());
    setupCurvesTree(plot);
    show();
}

Plot::Properties PlotPropertiesDialog::getPlotProperties()
{
    Plot::Properties p;

    p.plotCrosshair = ui->groupBox_plotCrosshair->isChecked();
    p.plotHline = ui->checkBox_plotHline->isChecked();
    p.plotVline = ui->checkBox_plotVline->isChecked();
    p.plotDot = ui->checkBox_plotDot->isChecked();
    p.mouseCrosshair = ui->groupBox_mouseCrosshair->isChecked();
    p.mouseHline = ui->checkBox_mouseHline->isChecked();
    p.mouseVline = ui->checkBox_mouseVline->isChecked();
    p.mouseDot = ui->checkBox_mouseDot->isChecked();

    p.showTitle = ui->checkBox_showTitle->isChecked();
    p.title = ui->lineEdit_title->text();
    p.showXlabel = ui->checkBox_showXlabel->isChecked();
    p.xlabel = ui->lineEdit_xlabel->text();
    p.showYlabel = ui->checkBox_showYlabel->isChecked();
    p.ylabel = ui->lineEdit_ylabel->text();
    p.showLegend = ui->checkBox_showLegend->isChecked();

    return p;
}

