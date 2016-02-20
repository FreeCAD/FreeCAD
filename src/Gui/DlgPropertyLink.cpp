/***************************************************************************
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
# include <sstream>
# include <QListWidgetItem>
# include <QMessageBox>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>

#include "DlgPropertyLink.h"
#include "Application.h"
#include "ViewProvider.h"
#include "ui_DlgPropertyLink.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgPropertyLink */

DlgPropertyLink::DlgPropertyLink(const QStringList& list, QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), link(list), ui(new Ui_DlgPropertyLink)
{
#ifdef FC_DEBUG
    assert(list.size() == 4);
#endif
    ui->setupUi(this);
    findObjects(ui->checkObjectType->isChecked());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgPropertyLink::~DlgPropertyLink()
{
  // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgPropertyLink::accept()
{
    QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::warning(this, tr("No selection"), tr("Please select an object from the list"));
    }
    else {
        QDialog::accept();
    }
}

QStringList DlgPropertyLink::propertyLink() const
{
    QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
    if (items.isEmpty()) {
        return link;
    }
    else {
        QStringList list = link;
        list[1] = items[0]->data(Qt::UserRole).toString();
        list[2] = items[0]->text();
        return list;
    }
}

void DlgPropertyLink::findObjects(bool on)
{
    QString docName = link[0];
    QString objName = link[1];
    QString parName = link[3];

    App::Document* doc = App::GetApplication().getDocument((const char*)docName.toLatin1());
    if (doc) {
        Base::Type baseType = App::DocumentObject::getClassTypeId();
        if (!on) {
            App::DocumentObject* obj = doc->getObject((const char*)objName.toLatin1());
            if (obj) {
                Base::Type objType = obj->getTypeId();
                // get only geometric types
                if (objType.isDerivedFrom(App::GeoFeature::getClassTypeId()))
                    baseType = App::GeoFeature::getClassTypeId();

                // get the direct base class of App::DocumentObject which 'obj' is derived from
                while (!objType.isBad()) {
                    std::string name = objType.getName();
                    Base::Type parType = objType.getParent();
                    if (parType == baseType) {
                        baseType = objType;
                        break;
                    }
                    objType = parType;
                }
            }
        }

        std::vector<App::DocumentObject*> outList;
        App::DocumentObject* par = doc->getObject((const char*)parName.toLatin1());
        if (par) {
            outList = par->getOutList();
            outList.push_back(par);
        }

        std::vector<App::DocumentObject*> obj = doc->getObjectsOfType(baseType);
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(*it);
            if (vp) {
                // filter out the objects
                if (std::find(outList.begin(), outList.end(), *it) == outList.end()) {
                    QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
                    item->setIcon(vp->getIcon());
                    item->setText(QString::fromUtf8((*it)->Label.getValue()));
                    QByteArray ba((*it)->getNameInDocument());
                    item->setData(Qt::UserRole, ba);
                }
            }
        }
    }
}

void DlgPropertyLink::on_checkObjectType_toggled(bool on)
{
    ui->listWidget->clear();
    findObjects(on);
}

#include "moc_DlgPropertyLink.cpp"
