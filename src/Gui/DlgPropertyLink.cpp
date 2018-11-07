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
    assert(list.size() >= 5);
#endif
    ui->setupUi(this);
    findObjects(ui->checkObjectType->isChecked(), QString());
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgPropertyLink::~DlgPropertyLink()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgPropertyLink::setSelectionMode(QAbstractItemView::SelectionMode mode)
{
    ui->listWidget->setSelectionMode(mode);
    ui->listWidget->clear();
    findObjects(ui->checkObjectType->isChecked(), ui->searchBox->text());
}

void DlgPropertyLink::accept()
{
    if (ui->listWidget->selectionMode() == QAbstractItemView::SingleSelection) {
        QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
        if (items.isEmpty()) {
            QMessageBox::warning(this, tr("No selection"), tr("Please select an object from the list"));
            return;
        }
    }

    QDialog::accept();
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
        if (list[1].isEmpty())
            list[2] = QString::fromUtf8("");
        return list;
    }
}

QVariantList DlgPropertyLink::propertyLinkList() const
{
    QVariantList varList;
    QList<QListWidgetItem*> items = ui->listWidget->selectedItems();
    if (items.isEmpty()) {
        varList << link;
    }
    else {
        for (QList<QListWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            QStringList list = link;
            list[1] = (*it)->data(Qt::UserRole).toString();
            list[2] = (*it)->text();
            if (list[1].isEmpty())
                list[2] = QString::fromUtf8("");
            varList << list;
        }
    }

    return varList;
}

void DlgPropertyLink::findObjects(bool on, const QString& searchText)
{
    QString docName = link[0]; // document name
    QString objName = link[1]; // internal object name
    QString parName = link[3]; // internal object name of the parent of the link property
    QString proName = link[4]; // property name

    bool isSingleSelection = (ui->listWidget->selectionMode() == QAbstractItemView::SingleSelection);
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
                    Base::Type parType = objType.getParent();
                    if (parType == baseType) {
                        baseType = objType;
                        break;
                    }
                    objType = parType;
                }
            }
        }

        // build list of objects names already in property so we can mark them as selected later on
        std::vector<const char *> selectedNames;

        // build ignore list
        std::vector<App::DocumentObject*> ignoreList;
        App::DocumentObject* par = doc->getObject((const char*)parName.toLatin1());
        App::Property* prop = par->getPropertyByName((const char*)proName.toLatin1());
        if (prop) {
            // for multi-selection we need all objects
            if (isSingleSelection) {
                ignoreList = par->getOutListOfProperty(prop);
            } else {
                // gather names of objects currently in property
                if (prop->getTypeId().isDerivedFrom(App::PropertyLinkList::getClassTypeId())) {
                    const App::PropertyLinkList* propll = static_cast<const App::PropertyLinkList*>(prop);
                    std::vector<App::DocumentObject*> links = propll->getValues();
                    for (std::vector<App::DocumentObject*>::iterator it = links.begin(); it != links.end(); ++it) {
                        selectedNames.push_back((*it)->getNameInDocument());
                    }
                }
            }

            // add the inlist to the ignore list to avoid dependency loops
            std::vector<App::DocumentObject*> inList = par->getInListRecursive();
            ignoreList.insert(ignoreList.end(), inList.begin(), inList.end());
            ignoreList.push_back(par);
        }

        // Add a "None" entry on top
        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        item->setText(tr("None (Remove link)"));
        QByteArray ba("");
        item->setData(Qt::UserRole, ba);

        std::vector<App::DocumentObject*> obj = doc->getObjectsOfType(baseType);
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(*it);
            bool nameOk = true;
            if (!searchText.isEmpty()) { 
                QString label = QString::fromUtf8((*it)->Label.getValue());
                if (!label.contains(searchText,Qt::CaseInsensitive))
                    nameOk = false;
            }
            if (vp && nameOk) {
                // filter out the objects
                if (std::find(ignoreList.begin(), ignoreList.end(), *it) == ignoreList.end()) {
                    QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
                    item->setIcon(vp->getIcon());
                    item->setText(QString::fromUtf8((*it)->Label.getValue()));
                    QByteArray ba((*it)->getNameInDocument());
                    item->setData(Qt::UserRole, ba);
                    // mark items as selected if needed
                    for (std::vector<const char *>::iterator nit = selectedNames.begin(); nit != selectedNames.end(); ++nit) {
                        if (strcmp(*nit,(*it)->getNameInDocument()) == 0) {
                            item->setSelected(true);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void DlgPropertyLink::on_checkObjectType_toggled(bool on)
{
    ui->listWidget->clear();
    findObjects(on, ui->searchBox->text());
}

void DlgPropertyLink::on_searchBox_textChanged(const QString& search)
{
    ui->listWidget->clear();
    bool on = ui->checkObjectType->isChecked();
    findObjects(on, search);
}

#include "moc_DlgPropertyLink.cpp"
