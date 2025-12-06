#ifndef MARKEREDITDIALOG_H
#define MARKEREDITDIALOG_H

#include <QDialog>

#include <QMenu>
#include <functional>

namespace Ui {
class MarkerEditDialog;
}

class MarkerEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MarkerEditDialog(QWidget *parent = nullptr);
    ~MarkerEditDialog();

    void edit(QString text, bool showHline, bool showVline, bool showDot,
              std::function<void(void)> callback);

    QString text();
    bool showHline();
    bool showVline();
    bool showDot();

    struct Preset {
        QString name;
        QString text;
    };
    QList<Preset> presets;

private slots:
    void on_pushButton_apply_clicked();
    void on_pushButton_cancel_clicked();

private:
    Ui::MarkerEditDialog *ui;

    QMenu mPresetMenu;
    void setupPresetMenu();

    std::function<void(void)> mCallback = nullptr;
};

#endif // MARKEREDITDIALOG_H
