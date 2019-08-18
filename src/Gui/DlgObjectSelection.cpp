/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
# include <QTreeWidget>
#endif

#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include "DlgObjectSelection.h"
#include "Application.h"
#include "ViewProviderDocumentObject.h"
#include "ui_DlgObjectSelection.h"

FC_LOG_LEVEL_INIT("Gui",true,true);

using namespace Gui;

/* TRANSLATOR Gui::DlgObjectSelection */

DlgObjectSelection::DlgObjectSelection(
        const std::vector<App::DocumentObject*> &objs, QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), ui(new Ui_DlgObjectSelection)
{
    ui->setupUi(this);

    // make sure to show a horizontal scrollbar if needed
#if QT_VERSION >= 0x050000
    ui->depList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->depList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->depList->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->depList->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
#else
    ui->depList->header()->setResizeMode(0, QHeaderView::ResizeToContents);
    ui->depList->header()->setResizeMode(1, QHeaderView::ResizeToContents);
    ui->depList->header()->setResizeMode(2, QHeaderView::ResizeToContents);
    ui->depList->header()->setResizeMode(3, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);
#endif
    ui->depList->header()->setStretchLastSection(false);
    ui->depList->headerItem()->setText(0, tr("Dependency"));
    ui->depList->headerItem()->setText(1, tr("Document"));
    ui->depList->headerItem()->setText(2, tr("Name"));
    ui->depList->headerItem()->setText(3, tr("State"));

    ui->treeWidget->headerItem()->setText(0, tr("Hierarchy"));
    ui->treeWidget->header()->setStretchLastSection(false);

    for(auto obj : App::Document::getDependencyList(objs)) {
        auto &info = objMap[obj];
        info.depItem = new QTreeWidgetItem(ui->depList);
        auto vp = Gui::Application::Instance->getViewProvider(obj);
        if(vp) info.depItem->setIcon(0, vp->getIcon());
        info.depItem->setIcon(0, vp->getIcon());
        info.depItem->setText(0, QString::fromUtf8((obj)->Label.getValue()));
        info.depItem->setText(1, QString::fromUtf8(obj->getDocument()->getName()));
        info.depItem->setText(2, QString::fromLatin1(obj->getNameInDocument()));
        info.depItem->setText(3, tr("Selected"));
        info.depItem->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        info.depItem->setCheckState(0,Qt::Checked);
    }
    for(auto obj : objs) {
        auto &info = objMap[obj];
        info.items.push_back(createItem(obj,0));
        info.items.back()->setCheckState(0,Qt::Checked);
    }

    for(auto &v : objMap) {
        for(auto obj : v.first->getOutListRecursive()) {
            if(obj == v.first)
                continue;
            auto it = objMap.find(obj);
            if(it == objMap.end())
                continue;
            v.second.outList[obj] = &it->second;
        }
        for(auto obj : v.first->getInListRecursive()) {
            if(obj == v.first)
                continue;
            auto it = objMap.find(obj);
            if(it == objMap.end())
                continue;
            v.second.inList[obj] = &it->second;
        }
    }

    connect(ui->treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(onItemChanged(QTreeWidgetItem*,int)));
    connect(ui->depList, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(onItemChanged(QTreeWidgetItem*,int)));
    connect(ui->treeWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));
    connect(ui->depList, SIGNAL(itemSelectionChanged()),
            this, SLOT(onDepSelectionChanged()));
    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgObjectSelection::~DlgObjectSelection()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

QTreeWidgetItem *DlgObjectSelection::createItem(App::DocumentObject *obj, QTreeWidgetItem *parent) {
    QTreeWidgetItem* item;
    if(parent)
        item = new QTreeWidgetItem(parent);
    else
        item = new QTreeWidgetItem(ui->treeWidget);
    auto vp = Gui::Application::Instance->getViewProvider(obj);
    if(vp) item->setIcon(0, vp->getIcon());
    item->setText(0, QString::fromUtf8((obj)->Label.getValue()));
    item->setData(0, Qt::UserRole, QByteArray(obj->getDocument()->getName()));
    item->setData(0, Qt::UserRole+1, QByteArray(obj->getNameInDocument()));
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    std::set<App::DocumentObject *> outSet;
    for(auto o : obj->getOutList()) {
        if(objMap.count(o))
            outSet.insert(o);
    }
    if(outSet.empty())
        return item;
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    if(!parent) {
        bool populate = false;
        for(auto o : outSet) {
            if(objMap[o].items.empty()) {
                populate = true;
                break;
            }
        }
        if(!populate)
            return item;
        for(auto o : outSet) {
            auto &info = objMap[o];
            info.items.push_back(createItem(o,item));
            info.items.back()->setCheckState(0,info.checkState);
        }
    }
    return item;
}

class SignalBlocker {
public:
    SignalBlocker(QTreeWidget *treeWidget)
        :treeWidget(treeWidget)
    {
        treeWidget->blockSignals(true);
    }
    ~SignalBlocker() {
        treeWidget->blockSignals(false);
    }
    QTreeWidget *treeWidget;
};

App::DocumentObject *DlgObjectSelection::objFromItem(QTreeWidgetItem *item) {
    std::string name;
    std::string docName;
    if(item->treeWidget() == ui->treeWidget) {
        docName = item->data(0,Qt::UserRole).toByteArray().constData();
        name = item->data(0,Qt::UserRole+1).toByteArray().constData();
    }else{
        docName = qPrintable(item->text(1));
        name = qPrintable(item->text(2));
    }
    auto doc = App::GetApplication().getDocument(docName.c_str());
    if(!doc) return 0;
    return doc->getObject(name.c_str());
}

void DlgObjectSelection::onItemExpanded(QTreeWidgetItem * item) {
    if(item->childCount()) 
        return;
    auto obj = objFromItem(item);
    if(!obj)
        return;
    SignalBlocker blocker(ui->treeWidget);
    std::set<App::DocumentObject *> outSet;
    for(auto o : obj->getOutList()) {
        if(!objMap.count(obj) || !outSet.insert(o).second)
            continue;
        auto &info = objMap[o];
        info.items.push_back(createItem(o,item));
        info.items.back()->setCheckState(0,info.checkState);
    }
}

void DlgObjectSelection::onItemChanged(QTreeWidgetItem * item, int column) {
    if(column) return;
    auto obj = objFromItem(item);
    if(!obj) return;
    auto state = item->checkState(0);
    auto it = objMap.find(obj);
    if(it == objMap.end() || state == it->second.checkState)
        return;
    SignalBlocker blocker(ui->treeWidget);
    SignalBlocker blocker2(ui->depList);
    auto &info = it->second;
    info.checkState = state;

    if(item == info.depItem) {
        for(auto item : info.items)
            item->setCheckState(0,state);
    }else{
        info.depItem->setCheckState(0,state);
        info.depItem->setText(3,state==Qt::Checked?tr("Selected"):QString());
    }

    if(state == Qt::Unchecked) {
        for(auto &v : info.outList) {
            if(info.inList.count(v.first)) {
                // This indicates a dependency loop. The check here is so that
                // object selection still works despite of the loop
                continue;
            }
            if(v.second->checkState == Qt::Unchecked)
                continue;
            v.second->checkState = Qt::Unchecked;
            v.second->depItem->setText(3,QString());
            v.second->depItem->setCheckState(0,Qt::Unchecked);
            for(auto item : v.second->items)
                item->setCheckState(0,Qt::Unchecked);
        }
        for(auto &v : info.inList) {
            if(v.second->checkState != Qt::Checked)
                continue;
            v.second->checkState = Qt::PartiallyChecked;
            v.second->depItem->setText(3,tr("Partial"));
            v.second->depItem->setCheckState(0,Qt::PartiallyChecked);
            for(auto item : v.second->items)
                item->setCheckState(0,Qt::PartiallyChecked);
        }
        return;
    } else if(state == Qt::Checked) {
        for(auto &v : info.outList) {
            if(info.inList.count(v.first)) {
                // This indicates a dependency loop. The check here is so that
                // object selection still works despite of the loop
                continue;
            }
            if(v.second->checkState == Qt::Checked)
                continue;
            v.second->checkState = Qt::Checked;
            v.second->depItem->setText(3,tr("Selected"));
            v.second->depItem->setCheckState(0,Qt::Checked);
            for(auto item : v.second->items)
                item->setCheckState(0,Qt::Checked);
        }
        bool touched;
        do {
            touched = false;
            for(auto &v : info.inList) {
                if(v.second->checkState != Qt::PartiallyChecked)
                    continue;
                bool partial = false;
                for(auto &vv : v.second->outList) {
                    if(vv.second->checkState != Qt::Checked) {
                        partial = true;
                        break;
                    }
                }
                if(partial)
                    continue;
                touched = true;
                v.second->checkState = Qt::Checked;
                v.second->depItem->setText(3,tr("Selected"));
                v.second->depItem->setCheckState(0,Qt::Checked);
                for(auto item : v.second->items)
                    item->setCheckState(0,Qt::Checked);
            }
        }while(touched);
    }
}

std::vector<App::DocumentObject*> DlgObjectSelection::getSelections() const {
    std::vector<App::DocumentObject*> res;
    for(auto &v : objMap) {
        if(v.second.checkState != Qt::Unchecked)
            res.push_back(v.first);
    }
    return res;
}

void DlgObjectSelection::onItemSelectionChanged() {
    SignalBlocker block2(ui->treeWidget);
    SignalBlocker block(ui->depList);
    QTreeWidgetItem *scroll=0;
    for(auto &v : objMap) {
        auto &info = v.second;
        auto it = sels.find(v.first);
        auto selected = it==sels.end();
        for(auto item : info.items) {
            if(selected == item->isSelected()) {
                for(auto item : info.items)
                    item->setSelected(selected);
                scroll = info.depItem;
                info.depItem->setSelected(selected);
                scroll = info.depItem;
                if(!selected)
                    sels.erase(it);
                else
                    sels.insert(v.first);
                break;
            }
        }
    }
    if(scroll)
        ui->depList->scrollToItem(scroll);
}

void DlgObjectSelection::onDepSelectionChanged() {
    SignalBlocker block2(ui->treeWidget);
    SignalBlocker block(ui->depList);
    QTreeWidgetItem *scroll=0;
    for(auto &v : objMap) {
        auto &info = v.second;
        auto it = sels.find(v.first);
        auto selected = it==sels.end();
        if(info.depItem->isSelected()==selected) {
            for(auto item : info.items) {
                scroll = item;
                item->setSelected(selected);
            }
            if(!selected)
                sels.erase(it);
            else {
                sels.insert(v.first);
                for(auto item : info.items) {
                    for(auto parent=item->parent();parent;parent=parent->parent())
                        parent->setExpanded(true);
                }
            }
        }
    }
    if(scroll)
        ui->treeWidget->scrollToItem(scroll);
}

void DlgObjectSelection::accept() {
    QDialog::accept();
}

void DlgObjectSelection::reject() {
    QDialog::reject();
}

#include "moc_DlgObjectSelection.cpp"
