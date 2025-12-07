#ifndef CROSSHAIRSDIALOG_H
#define CROSSHAIRSDIALOG_H

#include <QDialog>

namespace Ui {
class CrosshairsDialog;
}

class CrosshairsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CrosshairsDialog(QWidget *parent = nullptr);
    ~CrosshairsDialog();

    struct Settings {
        bool plotCrosshair = false;
        bool plotHline = false;
        bool plotVline = false;
        bool plotDot = false;
        bool mouseCrosshair = false;
        bool mouseHline = false;
        bool mouseVline = false;
        bool mouseDot = false;
    };

    void show(Settings settings);
    Settings settings();

signals:
    void settingsChanged(Settings settings);

private slots:
    void on_groupBox_plotCrosshair_toggled(bool arg1);

    void on_checkBox_plotHline_toggled(bool checked);

    void on_checkBox_plotVline_toggled(bool checked);

    void on_checkBox_plotDot_toggled(bool checked);

    void on_groupBox_mouseCrosshair_toggled(bool arg1);

    void on_checkBox_mouseHline_toggled(bool checked);

    void on_checkBox_mouseVline_toggled(bool checked);

    void on_checkBox_mouseDot_toggled(bool checked);

    void on_pushButton_ok_clicked();

private:
    Ui::CrosshairsDialog *ui;

    void emitSettingsChanged();
};

#endif // CROSSHAIRSDIALOG_H
