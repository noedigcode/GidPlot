#ifndef LINKDIALOG_H
#define LINKDIALOG_H

#include "plotwindow.h"

#include <QDialog>

namespace Ui {
class LinkDialog;
}

class LinkDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LinkDialog(QWidget *parent = nullptr);
    ~LinkDialog();

    void show(QList<PlotWindow*> plotWindows, SubplotPtr selectSubplot);

private:
    Ui::LinkDialog *ui;

};

#endif // LINKDIALOG_H
