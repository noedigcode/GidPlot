/******************************************************************************
 *
 * This file is part of GidPlot.
 * Copyright (C) 2025 Gideon van der Kolf
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

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

void LinkDialog::show(QList<PlotWindow *> plotWindows, LinkPtr selectLink)
{
    ui->treeWidget->clear();

    QTreeWidgetItem* itemToSelect = nullptr;

    foreach (PlotWindow* pw, plotWindows) {
        QTreeWidgetItem* parentItem = new QTreeWidgetItem();
        parentItem->setText(0, pw->windowTitle());

        foreach (LinkPtr link, pw->links()) {

            QTreeWidgetItem* child = new QTreeWidgetItem();
            if (link == selectLink) {
                itemToSelect = child;
            }
            child->setText(0, link->tag);
            parentItem->addChild(child);

            QComboBox* cbGroup = new QComboBox();
            cbGroup->addItems(QStringList({"None", "1", "2", "3"}));
            cbGroup->setCurrentIndex(link->group);
            connect(cbGroup, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [linkWkPtr = link.toWeakRef()](int index)
            {
                LinkPtr link(linkWkPtr);
                if (!link) { return; }
                link->group = index;
            });
            ui->treeWidget->setItemWidget(child, 1, cbGroup);

            if (link->supportPosZoom) {

                QCheckBox* cbXpos = new QCheckBox();
                cbXpos->setChecked(link->linkXpos);
                connect(cbXpos, &QCheckBox::toggled,
                        this, [linkWkPtr = link.toWeakRef()](int checked)
                {
                    LinkPtr link(linkWkPtr);
                    if (!link) { return; }
                    link->linkXpos = checked;
                });
                ui->treeWidget->setItemWidget(child, 2, cbXpos);

                QCheckBox* cbYpos = new QCheckBox();
                cbYpos->setChecked(link->linkYpos);
                connect(cbYpos, &QCheckBox::toggled,
                        this, [linkWkPtr = link.toWeakRef()](int checked)
                {
                    LinkPtr link(linkWkPtr);
                    if (!link) { return; }
                    link->linkYpos = checked;
                });
                ui->treeWidget->setItemWidget(child, 3, cbYpos);

                QCheckBox* cbXzoom = new QCheckBox();
                cbXzoom->setChecked(link->linkXzoom);
                connect(cbXzoom, &QCheckBox::toggled,
                        this, [linkWkPtr = link.toWeakRef()](int checked)
                {
                    LinkPtr link(linkWkPtr);
                    if (!link) { return; }
                    link->linkXzoom = checked;
                });
                ui->treeWidget->setItemWidget(child, 4, cbXzoom);

                QCheckBox* cbYzoom = new QCheckBox();
                cbYzoom->setChecked(link->linkYzoom);
                connect(cbYzoom, &QCheckBox::toggled,
                        this, [linkWkPtr = link.toWeakRef()](int checked)
                {
                    LinkPtr link(linkWkPtr);
                    if (!link) { return; }
                    link->linkYzoom = checked;
                });
                ui->treeWidget->setItemWidget(child, 5, cbYzoom);

            }

        }

        ui->treeWidget->addTopLevelItem(parentItem);
        parentItem->setFirstColumnSpanned(true);
        parentItem->setExpanded(true);
    }

    ui->treeWidget->setCurrentItem(itemToSelect);

    QDialog::show();
}
