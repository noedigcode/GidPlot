#include "MarkerEditDialog.h"
#include "ui_MarkerEditDialog.h"

MarkerEditDialog::MarkerEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MarkerEditDialog)
{
    ui->setupUi(this);

    setupPresetMenu();
}

MarkerEditDialog::~MarkerEditDialog()
{
    delete ui;
}

void MarkerEditDialog::edit(QString text, bool showHline, bool showVline,
                            bool showDot, std::function<void ()> callback)
{
    ui->plainTextEdit_labelText->setPlainText(text);
    ui->checkBox_horizontalLine->setChecked(showHline);
    ui->checkBox_verticalLine->setChecked(showVline);
    ui->checkBox_dot->setChecked(showDot);
    mCallback = callback;
    show();
}

QString MarkerEditDialog::text()
{
    return ui->plainTextEdit_labelText->toPlainText();
}

bool MarkerEditDialog::showHline()
{
    return ui->checkBox_horizontalLine->isChecked();
}

bool MarkerEditDialog::showVline()
{
    return ui->checkBox_verticalLine->isChecked();
}

bool MarkerEditDialog::showDot()
{
    return ui->checkBox_dot->isChecked();
}

void MarkerEditDialog::on_pushButton_apply_clicked()
{
    if (mCallback) {
        mCallback();
    }
    mCallback = nullptr;
    hide();
}

void MarkerEditDialog::on_pushButton_cancel_clicked()
{
    mCallback = nullptr;
    hide();
}

void MarkerEditDialog::setupPresetMenu()
{
    presets << Preset{"X, Y labeled on separate lines",
                      "X: $x\nY: $y"}

            << Preset{"X, Y, index labeled on separate lines",
                      "X: $x\nY: $y\nIndex: $i"}

            << Preset{"X, Y, values only",
                      "$x, $y"};

    foreach (Preset preset, presets) {
        mPresetMenu.addAction(preset.name, this, [=]()
        {
            ui->plainTextEdit_labelText->setPlainText(preset.text);
        });
    }

    ui->toolButton_presets->setMenu(&mPresetMenu);
}

