#include "LinkDialog.h"
#include "ui_LinkDialog.h"

LinkDialog::LinkDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LinkDialog)
{
    ui->setupUi(this);


}

LinkDialog::~LinkDialog()
{
    delete ui;
}

void LinkDialog::show(QList<PlotWindow *> plotWindows, SubplotPtr selectSubplot)
{
    ui->treeWidget->clear();

    QTreeWidgetItem* itemToSelect = nullptr;

    foreach (PlotWindow* pw, plotWindows) {
        QTreeWidgetItem* parentItem = new QTreeWidgetItem();
        parentItem->setText(0, pw->windowTitle());

        foreach (SubplotPtr subplot, pw->subplots()) {
            QTreeWidgetItem* child = new QTreeWidgetItem();
            if (subplot == selectSubplot) {
                itemToSelect = child;
            }
            child->setText(0, subplot->tag);
            parentItem->addChild(child);

            QComboBox* cbGroup = new QComboBox();
            cbGroup->addItems(QStringList({"None", "1", "2", "3"}));
            cbGroup->setCurrentIndex(subplot->linkGroup);
            connect(cbGroup, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [sbWkPtr = subplot.toWeakRef()](int index)
            {
                SubplotPtr s(sbWkPtr);
                if (!s) { return; }
                s->linkGroup = index;
            });
            ui->treeWidget->setItemWidget(child, 1, cbGroup);

            QCheckBox* cbXpos = new QCheckBox();
            cbXpos->setChecked(subplot->linkXpos);
            connect(cbXpos, &QCheckBox::toggled,
                    this, [sbWkPtr = subplot.toWeakRef()](int checked)
            {
                SubplotPtr s(sbWkPtr);
                if (!s) { return; }
                s->linkXpos = checked;
            });
            ui->treeWidget->setItemWidget(child, 2, cbXpos);

            QCheckBox* cbYpos = new QCheckBox();
            cbYpos->setChecked(subplot->linkYpos);
            connect(cbYpos, &QCheckBox::toggled,
                    this, [sbWkPtr = subplot.toWeakRef()](int checked)
            {
                SubplotPtr s(sbWkPtr);
                if (!s) { return; }
                s->linkYpos = checked;
            });
            ui->treeWidget->setItemWidget(child, 3, cbYpos);

            QCheckBox* cbXzoom = new QCheckBox();
            cbXzoom->setChecked(subplot->linkXzoom);
            connect(cbXzoom, &QCheckBox::toggled,
                    this, [sbWkPtr = subplot.toWeakRef()](int checked)
            {
                SubplotPtr s(sbWkPtr);
                if (!s) { return; }
                s->linkXzoom = checked;
            });
            ui->treeWidget->setItemWidget(child, 4, cbXzoom);

            QCheckBox* cbYzoom = new QCheckBox();
            cbYzoom->setChecked(subplot->linkYzoom);
            connect(cbYzoom, &QCheckBox::toggled,
                    this, [sbWkPtr = subplot.toWeakRef()](int checked)
            {
                SubplotPtr s(sbWkPtr);
                if (!s) { return; }
                s->linkYzoom = checked;
            });
            ui->treeWidget->setItemWidget(child, 5, cbYzoom);
        }

        ui->treeWidget->addTopLevelItem(parentItem);
        parentItem->setExpanded(true);
    }

    ui->treeWidget->setCurrentItem(itemToSelect);

    QDialog::show();
}
