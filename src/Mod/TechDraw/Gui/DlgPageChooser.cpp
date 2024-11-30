/****************************************************************************
 *   Copyright (c) 2021 Wanderer Fan <wandererfan@gmail.com>                *
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
# include <QList>
#endif

#include <Base/Console.h> // for FC_LOG_LEVEL_INIT
#include <Base/Tools.h>

#include "DlgPageChooser.h"
#include "ui_DlgPageChooser.h"


FC_LOG_LEVEL_INIT("Gui", true, true)

using namespace TechDrawGui;

/* TRANSLATOR Gui::DlgPageChooser */

DlgPageChooser::DlgPageChooser(
        const std::vector<std::string> labels,
        const std::vector<std::string> names,
        QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), ui(new Ui_DlgPageChooser)
{
    ui->setupUi(this);
    ui->lwPages->setSortingEnabled(true);

    fillList(labels, names);

    connect(ui->bbButtons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->bbButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgPageChooser::~DlgPageChooser()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgPageChooser::fillList(std::vector<std::string> labels, std::vector<std::string> names)
{
    QListWidgetItem* item;
    QString qLabel;
    QString qName;
    QString qText;
    int labelCount = labels.size();
    int i = 0;
    for (; i < labelCount; i++) {
        qLabel = Base::Tools::fromStdString(labels[i]);
        qName = Base::Tools::fromStdString(names[i]);
        qText = QString::fromUtf8("%1 (%2)").arg(qLabel, qName);
        item = new QListWidgetItem(qText, ui->lwPages);
        item->setData(Qt::UserRole, qName);
    }
}

std::string DlgPageChooser::getSelection() const
{
    QList<QListWidgetItem*> sels = ui->lwPages->selectedItems();
    if (!sels.empty()) {
        QListWidgetItem* item = sels.front();
        return item->data(Qt::UserRole).toByteArray().constData();
    }
    return std::string();
}


void DlgPageChooser::accept() {
    QDialog::accept();
}

void DlgPageChooser::reject() {
    QDialog::reject();
}

#include "moc_DlgPageChooser.cpp"
