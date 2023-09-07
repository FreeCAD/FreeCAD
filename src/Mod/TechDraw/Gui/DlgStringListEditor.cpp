/****************************************************************************
 *   Copyright (c) 2022 Wanderer Fan <wandererfan@gmail.com>                *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QListWidgetItem>
#endif

#include <Base/Tools.h>

#include "DlgStringListEditor.h"
#include "ui_DlgStringListEditor.h"


using namespace TechDrawGui;

/* TRANSLATOR Gui::DlgStringListEditor */

DlgStringListEditor::DlgStringListEditor(const std::vector<std::string> texts, QWidget* parent,
                                         Qt::WindowFlags fl)
    : QDialog(parent, fl),
      ui(new Ui_DlgStringListEditor)
{
    ui->setupUi(this);
    ui->lwTexts->setSortingEnabled(false);

    fillList(texts);

    connect(ui->lwTexts,
            &QListWidget::itemActivated,
            this,
            &DlgStringListEditor::slotItemActivated);
    connect(ui->pbAdd, &QPushButton::clicked, this, &DlgStringListEditor::slotAddItem);
    connect(ui->pbRemove, &QPushButton::clicked, this, &DlgStringListEditor::slotRemoveItem);
    connect(ui->bbButtons, &QDialogButtonBox::accepted, this, &DlgStringListEditor::accept);
    connect(ui->bbButtons, &QDialogButtonBox::rejected, this, &DlgStringListEditor::reject);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgStringListEditor::~DlgStringListEditor()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgStringListEditor::fillList(std::vector<std::string> texts)
{
    QString qText;
    int textCount = texts.size();
    int i = 0;
    for (; i < textCount; i++) {
        qText = Base::Tools::fromStdString(texts[i]);
        QListWidgetItem* item = new QListWidgetItem(qText);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        ui->lwTexts->addItem(item);
    }
    //add a blank line at the end to allow extending the list
    QListWidgetItem* item = new QListWidgetItem(QString::fromUtf8(""));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    ui->lwTexts->addItem(item);
}

void DlgStringListEditor::slotItemActivated(QListWidgetItem* item)
{
    ui->lwTexts->editItem(item);
}

void DlgStringListEditor::slotAddItem()
{
    QString newText = ui->leNewItem->text();
    QListWidgetItem* item = new QListWidgetItem(newText);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    int row = ui->lwTexts->currentRow();
    if (row < 0) {
        //no location set yet, add to end of list
        ui->lwTexts->addItem(item);
    }
    else {
        //insert item at current row and push the rest down 1 position
        ui->lwTexts->insertItem(row, item);
    }
    ui->leNewItem->clear();
    //TODO: how to append to end of list?
}

void DlgStringListEditor::slotRemoveItem()
{
    if (ui->lwTexts->count() < 1) {
        return;
    }
    int row = ui->lwTexts->currentRow();
    if (row >= 0) {
        auto item = ui->lwTexts->takeItem(row);
        delete item;
    }
}

std::vector<std::string> DlgStringListEditor::getTexts() const
{
    std::vector<std::string> outTexts;
    if (ui->lwTexts->count() < 1) {
        return outTexts;
    }

    for (int iRow = 0; iRow < ui->lwTexts->count(); iRow++) {
        QString itemText = ui->lwTexts->item(iRow)->text();
        outTexts.push_back(Base::Tools::toStdString(itemText));
    }
    if (outTexts.back().empty()) {
        outTexts.pop_back();
    }
    return outTexts;
}

void DlgStringListEditor::accept()
{
    QDialog::accept();
}

void DlgStringListEditor::reject()
{
    QDialog::reject();
}

#include "moc_DlgStringListEditor.cpp"
