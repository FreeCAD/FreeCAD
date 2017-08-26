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
# include <QTreeWidgetItem>
# include <QMessageBox>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>

#include "DlgPropertyLink.h"
#include "Application.h"
#include "ViewProviderDocumentObject.h"
#include "ui_DlgPropertyLink.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgPropertyLink */

DlgPropertyLink::DlgPropertyLink(const QStringList& list, QWidget* parent, Qt::WindowFlags fl, bool xlink)
  : QDialog(parent, fl), link(list), ui(new Ui_DlgPropertyLink)
{
#ifdef FC_DEBUG
    assert(list.size() >= 5);
#endif
    QString parDoc = link[3];
    QString parName = link[4];
    auto doc = App::GetApplication().getDocument(qPrintable(link[3]));
    if(doc) {
        auto obj = doc->getObject(qPrintable(link[4]));
        if(obj) {
            inList = obj->getInListEx(true);
            inList.insert(obj);
        }
    }

    ui->setupUi(this);
    if(!xlink) 
        ui->comboBox->hide();
    else {
        std::string linkDoc = qPrintable(link[0]);
        for(auto doc : App::GetApplication().getDocuments()) {
            QString name(QString::fromUtf8(doc->getName()));
            ui->comboBox->addItem(name);
            if(linkDoc == doc->getName())
                ui->comboBox->setCurrentIndex(ui->comboBox->count()-1);
        }
    }
    findObjects(ui->checkObjectType->isChecked());

    connect(ui->treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
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
    auto items = ui->treeWidget->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::warning(this, tr("No selection"), tr("Please select an object from the list"));
    }
    else {
        QDialog::accept();
    }
}

QStringList DlgPropertyLink::propertyLink() const
{
    auto items = ui->treeWidget->selectedItems();
    if (items.isEmpty()) {
        return link;
    }
    else {
        QStringList list = link;
        if(link.size()>=6 && items[0]->parent()) {
            QString subname;
            auto parent = items[0];
            for(auto item=parent;;item=parent) {
                parent = item->parent();
                if(!parent) {
                    list[1] = item->data(0,Qt::UserRole).toString();
                    break;
                }
                subname = QString::fromLatin1("%1%2.").
                    arg(subname).arg(item->data(0,Qt::UserRole).toString());
            }
            list[5] = subname;
            if(subname.size())
                list[2] = QString::fromLatin1("%1 (%2.%3)").
                    arg(items[0]->text(0)).arg(list[1]).arg(subname);
            else
                list[2] = items[0]->text(0);
        }else{
            list[1] = items[0]->data(0,Qt::UserRole).toString();
            list[2] = items[0]->text(0);
            if (list[1].isEmpty())
                list[2] = QString::fromUtf8("");
        }
        return list;
    }
}

void DlgPropertyLink::findObjects(bool on)
{
    ui->treeWidget->clear();

    QString docName = link[0];
    QString objName = link[1];

    App::Document* doc = App::GetApplication().getDocument((const char*)docName.toLatin1());
    if (doc) {
        Base::Type baseType = App::DocumentObject::getClassTypeId();
        if (!on) {
            App::DocumentObject* obj = doc->getObject((const char*)objName.toLatin1());
            if (obj && inList.find(obj)==inList.end()) {
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

        // Add a "None" entry on top
        auto* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0,tr("None (Remove link)"));
        QByteArray ba("");
        item->setData(0,Qt::UserRole, ba);

        for(auto obj : doc->getObjectsOfType(baseType))
            createItem(obj,0);
    }
}

void DlgPropertyLink::createItem(App::DocumentObject *obj, QTreeWidgetItem *parent) {
    if(!obj || !obj->getNameInDocument() || inList.find(obj)!=inList.end())
        return;

    auto vp = Gui::Application::Instance->getViewProvider(obj);
    if(!vp) return;

    QString searchText = ui->searchBox->text();
    if (!searchText.isEmpty()) { 
        QString label = QString::fromUtf8((obj)->Label.getValue());
        if (!label.contains(searchText,Qt::CaseInsensitive))
            return;
    }
    QTreeWidgetItem* item;
    if(parent)
        item = new QTreeWidgetItem(parent);
    else
        item = new QTreeWidgetItem(ui->treeWidget);
    item->setIcon(0, vp->getIcon());
    item->setText(0, QString::fromUtf8((obj)->Label.getValue()));
    item->setData(0, Qt::UserRole, QByteArray(obj->getNameInDocument()));
    item->setData(0, Qt::UserRole+1, QByteArray(obj->getDocument()->getName()));
    if(link.size()>=6) {
        item->setChildIndicatorPolicy(obj->hasChildElement()||vp->getChildRoot()?
                QTreeWidgetItem::ShowIndicator:QTreeWidgetItem::DontShowIndicator);
    }
}

void DlgPropertyLink::onItemExpanded(QTreeWidgetItem * item) {
    if(link.size()<6 || item->childCount()) 
        return;

    std::string name(qPrintable(item->data(0, Qt::UserRole).toString()));
    std::string docName(qPrintable(item->data(0, Qt::UserRole+1).toString()));
    auto doc = App::GetApplication().getDocument(docName.c_str());
    if(doc) {
        auto obj = doc->getObject(name.c_str());
        if(!obj) return;
        auto vp = Application::Instance->getViewProvider(obj);
        if(!vp) return;
        for(auto obj : vp->claimChildren())
            createItem(obj,item);
    }
}

void DlgPropertyLink::on_checkObjectType_toggled(bool on)
{
    findObjects(on);
}

void DlgPropertyLink::on_searchBox_textChanged(const QString& /*search*/)
{
    bool on = ui->checkObjectType->isChecked();
    findObjects(on);
}

void DlgPropertyLink::on_comboBox_currentIndexChanged(const QString& text)
{
    link[0] = text;
    bool on = ui->checkObjectType->isChecked();
    findObjects(on);
}


#include "moc_DlgPropertyLink.cpp"
