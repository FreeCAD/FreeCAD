 /**************************************************************************
 *   Copyright (c) 2021 FreeCAD Developers                                 *
 *   Author: Ajinkya Dahale                                                *
 *   Based on src/Gui/DlgAddProperty.cpp                                   *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMessageBox>
#endif

#include <Gui/Application.h>

#include "DlgActiveBody.h"
#include "ReferenceSelection.h"
#include "Utils.h"

Q_DECLARE_METATYPE(App::DocumentObject*);

using namespace PartDesignGui;

DlgActiveBody::DlgActiveBody(QWidget *parent, App::Document*& doc,
                             const QString& infoText)
    : QDialog(parent),
      ui(new Ui_DlgActiveBody),
      _doc(doc),
      activeBody(nullptr)
{
    ui->setupUi(this);

    QObject::connect(ui->bodySelect, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
                     this, SLOT(accept()));

    if(!infoText.isEmpty()) {
        ui->label->setText(infoText + QString::fromUtf8("\n\n") +
                           QObject::tr("Please select"));
    }

    auto bodies = _doc->getObjectsOfType(PartDesign::Body::getClassTypeId());

    PartDesign::Body* bodyOfActiveObject = nullptr;
    for (const auto &obj :  Gui::Selection().getSelection()) {
        bodyOfActiveObject = PartDesign::Body::findBodyOf(obj.pObject);
        break; // Just get the body for first selected object
    }

    for (const auto &body : bodies) {
        auto item = new QListWidgetItem(QString::fromUtf8(body->Label.getValue()));
        item->setData(Qt::UserRole, QVariant::fromValue(body));
        ui->bodySelect->addItem(item);

        if (body == bodyOfActiveObject) {
            item->setSelected(true);
        }

        // TODO: Any other logic (hover, select effects on view etc.)
    }
}

void DlgActiveBody::accept()
{
    auto selectedItems = ui->bodySelect->selectedItems();
    if (selectedItems.empty())
        return;

    App::DocumentObject* selectedBody =
        selectedItems[0]->data(Qt::UserRole).value<App::DocumentObject*>();
    if (selectedBody)
        activeBody = makeBodyActive(selectedBody, _doc);
    else
        activeBody = makeBody(_doc);

    QDialog::accept();
}

#include "moc_DlgActiveBody.cpp"
