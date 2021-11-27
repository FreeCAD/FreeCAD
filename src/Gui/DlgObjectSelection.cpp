/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
# include <QCheckBox>
# include <QDesktopWidget>
# include <QResizeEvent>
#endif

#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include "DlgObjectSelection.h"
#include "Application.h"
#include "MainWindow.h"
#include "ViewProviderDocumentObject.h"
#include "MetaTypes.h"
#include "ui_DlgObjectSelection.h"
#include "ViewParams.h"

FC_LOG_LEVEL_INIT("Gui",true,true)

using namespace Gui;

/* TRANSLATOR Gui::DlgObjectSelection */

DlgObjectSelection::DlgObjectSelection(
        const std::vector<App::DocumentObject*> &objs, QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl)
{
    init(objs, {});
}

DlgObjectSelection::DlgObjectSelection(
        const std::vector<App::DocumentObject*> &objs,
        const std::vector<App::DocumentObject*> &excludes,
        QWidget* parent,
        Qt::WindowFlags fl)
  : QDialog(parent, fl)
{
    init(objs, excludes);
}

// This function is here for debugging purpose
static void inline setCheckState(QTreeWidgetItem *item, Qt::CheckState state)
{
    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    item->setCheckState(0, state);
}

void DlgObjectSelection::init(const std::vector<App::DocumentObject*> &objs,
                              const std::vector<App::DocumentObject*> &excludes)

{
    initSels = objs;
    std::sort(initSels.begin(), initSels.end());

    deps = App::Document::getDependencyList(objs, App::Document::DepSort);
    depSet.insert(deps.begin(), deps.end());

    ui = new Ui_DlgObjectSelection;
    ui->setupUi(this);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General");
    ui->checkBoxAutoDeps->setChecked(hGrp->GetBool("ObjectSelectionAutoDeps", true));
    connect(ui->checkBoxAutoDeps, SIGNAL(toggled(bool)), this, SLOT(onAutoDeps(bool)));

    // make sure to show a horizontal scrollbar if needed
    ui->depList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->depList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->depList->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->inList->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->inList->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->inList->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    ui->depList->headerItem()->setText(0, tr("Depending on"));
    ui->depList->headerItem()->setText(1, tr("Document"));
    ui->depList->headerItem()->setText(2, tr("Name"));

    ui->inList->headerItem()->setText(0, tr("Depended by"));
    ui->inList->headerItem()->setText(1, tr("Document"));
    ui->inList->headerItem()->setText(2, tr("Name"));

    ui->treeWidget->headerItem()->setText(0, tr("Selections"));
    ui->treeWidget->header()->setStretchLastSection(false);


    allItem = new QTreeWidgetItem(ui->treeWidget);
    allItem->setText(0, QStringLiteral("<%1>").arg(tr("All")));
    QFont font = allItem->font(0);
    font.setBold(true);
    allItem->setFont(0, font);
    allItem->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    allItem->setCheckState(0, Qt::Checked);

    for(auto obj : initSels)
        getItem(obj)->setCheckState(0, Qt::Checked);

    for(auto obj : deps)
        getItem(obj)->setCheckState(0, Qt::Checked);

    auto filter = excludes;
    std::sort(filter.begin(), filter.end());
    for (auto obj : deps) {
        auto it = std::lower_bound(filter.begin(), filter.end(), obj);
        if (it != filter.end() && *it == obj) {
            std::set<App::DocumentObject*> set;
            setItemState(set, obj, Qt::Unchecked);
        }
    }
    onItemSelectionChanged();

    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(onObjItemChanged(QTreeWidgetItem*,int)));
    connect(ui->depList, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(onDepItemChanged(QTreeWidgetItem*,int)));
    connect(ui->inList, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
            this, SLOT(onDepItemChanged(QTreeWidgetItem*,int)));
    connect(ui->treeWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(onItemSelectionChanged()));

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(checkItemChanged()));
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgObjectSelection::~DlgObjectSelection()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

QTreeWidgetItem *DlgObjectSelection::getItem(App::DocumentObject *obj) {
    auto &item = itemMap[obj];
    if (item)
        return item;
    item = new QTreeWidgetItem(ui->treeWidget);
    auto vp = Gui::Application::Instance->getViewProvider(obj);
    if(vp) item->setIcon(0, vp->getIcon());
    App::SubObjectT objT(obj);
    item->setText(0, QString::fromUtf8((obj)->Label.getValue()));
    if (std::binary_search(initSels.begin(), initSels.end(), obj)) {
        QFont font = item->font(0);
        font.setBold(true);
        font.setItalic(true);
        item->setFont(0, font);
    }
    item->setToolTip(0, QString::fromUtf8(objT.getObjectFullName().c_str()));
    item->setData(0, Qt::UserRole, QVariant::fromValue(objT));
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    return item;
}

void DlgObjectSelection::updateAllItemState()
{
    int count = 0;
    for (auto &v : itemMap) {
        auto state = v.second->checkState(0);
        if (state == Qt::Unchecked) {
            if (count) {
                allItem->setCheckState(0, Qt::PartiallyChecked);
                return;
            }
        } else {
            if (state == Qt::PartiallyChecked) {
                allItem->setCheckState(0, Qt::PartiallyChecked);
                return;
            }
            ++count;
        }
    }
    if (count && count == (int)itemMap.size())
        allItem->setCheckState(0, Qt::Checked);
}

void DlgObjectSelection::setItemState(std::set<App::DocumentObject*> &set,
                                      App::DocumentObject *obj,
                                      Qt::CheckState state,
                                      bool forced)
{
    if (!set.insert(obj).second)
        return;

    auto item = getItem(obj);

    if (!forced) {
        if (item->isSelected()) {
            if (state == Qt::Unchecked || item->checkState(0) == Qt::Unchecked)
                return;
        }
        if (item->checkState(0) == state)
            return;
    }
    setCheckState(item, state);

    if (!ui->checkBoxAutoDeps->isChecked())
        return;

    auto itDep = depMap.find(obj);
    if (itDep != depMap.end())
        setCheckState(itDep->second, state);

    if (state != Qt::PartiallyChecked) {
        for (auto o : obj->getOutList()) {
            if (depSet.count(o))
                setItemState(set, o, state);
        }
    }

    auto it = inMap.find(obj);
    if (it != inMap.end())
        setCheckState(it->second, state);

    // If an object toggles state, we need to revisit all its in list object
    for (auto o : obj->getInList())
        set.erase(o);

    for (auto o : obj->getInList()) {
        if (!depSet.count(o) || set.count(o) || itemChanged.count(o))
            continue;
        int count = 0;
        int selcount = 0;
        for (auto sibling : o->getOutList()) {
            if (!depSet.count(sibling))
                continue;
            ++count;
            auto it = itemMap.find(sibling);
            if (it == itemMap.end())
                continue;
            auto s = it->second->checkState(0);
            if (s == Qt::Unchecked)
                continue;
            if (it->second->checkState(0) == Qt::PartiallyChecked) {
                selcount = -1;
                break;
            }
            ++selcount;
        }
        auto it = itemMap.find(o);
        if (it != itemMap.end()) {
            auto state = it->second->checkState(0);
            if (state == Qt::Checked && selcount != count)
                setItemState(set, o, Qt::PartiallyChecked);
            else if (state == Qt::PartiallyChecked && selcount == count)
                setItemState(set, o, Qt::Checked);
        }
    }
}

std::vector<App::DocumentObject*> DlgObjectSelection::getSelections(bool invert, bool sort) const {
    std::vector<App::DocumentObject*> res;
    if (!invert) {
        for (auto &v : itemMap) {
            if (v.second->checkState(0) == Qt::Unchecked)
                continue;
            if (auto obj = v.first.getObject())
                res.push_back(obj);
        }
    } else {
        for (auto obj : deps) {
            auto it = itemMap.find(obj);
            if (it == itemMap.end() || it->second->checkState(0) == Qt::Unchecked)
                res.push_back(obj);
        }
    }
    if (sort)
        std::sort(res.begin(), res.end());
    return res;
}

void DlgObjectSelection::onDepItemChanged(QTreeWidgetItem * depItem, int column) {
    if(column) return;
    QSignalBlocker blocker(ui->depList);
    QSignalBlocker blocker2(ui->inList);
    QSignalBlocker blocker3(ui->treeWidget);
    auto state = depItem->checkState(0);
    if (depItem->isSelected()) {
        for (auto item : depItem->treeWidget()->selectedItems()) {
            auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
            auto it = itemMap.find(objT);
            if (it == itemMap.end())
                continue;
            setCheckState(item, state);
            setCheckState(it->second, state);
            itemChanged[objT] = state;
        }
    } else {
        auto objT = qvariant_cast<App::SubObjectT>(depItem->data(0, Qt::UserRole));
        auto it = itemMap.find(objT);
        if (it != itemMap.end()) {
            itemChanged[objT] = state;
            setCheckState(it->second, state);
        }
    }
    timer.start(10);
}

void DlgObjectSelection::onObjItemChanged(QTreeWidgetItem * objItem, int column) {
    if(column != 0)
        return;

    QSignalBlocker blocker3(ui->treeWidget);
    auto state = objItem->checkState(0);
    if (objItem == allItem) {
        if (state == Qt::PartiallyChecked)
            return;
        ui->treeWidget->selectionModel()->clearSelection();
        itemChanged.clear();
        timer.stop();
        onItemSelectionChanged();
        if (state == Qt::Unchecked) {
            for (auto &v : itemMap) {
                setCheckState(v.second, Qt::Unchecked);
                auto it = depMap.find(v.first);
                if (it != depMap.end())
                    setCheckState(it->second, Qt::Unchecked);
                it = inMap.find(v.first);
                if (it != inMap.end())
                    setCheckState(it->second, Qt::Unchecked);
            }
        } else {
            for (auto obj : initSels)
                setCheckState(getItem(obj), Qt::Checked);
            for (auto obj : deps) {
                setCheckState(getItem(obj), Qt::Checked);
                auto it = depMap.find(obj);
                if (it != depMap.end())
                    setCheckState(it->second, Qt::Checked);
                it = inMap.find(obj);
                if (it != inMap.end())
                    setCheckState(it->second, Qt::Checked);
            }
        }
        return;
    }

    if (!objItem->isSelected()) {
        ui->treeWidget->selectionModel()->clearSelection();
        objItem->setSelected(true);
        // We treat select item in tree widget specially in case of checking
        // items in depList or inList. To simplify logic, we change selection
        // here if an unselected item has been checked.
        itemChanged[qvariant_cast<App::SubObjectT>(objItem->data(0, Qt::UserRole))] = state;
        onItemSelectionChanged();
    }
    else {
        for (auto item : ui->treeWidget->selectedItems()) {
            setCheckState(item, state);
            itemChanged[qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole))] = state;
        }
    }
    timer.start(10);
}

void DlgObjectSelection::checkItemChanged() {

    QSignalBlocker blocker(ui->depList);
    QSignalBlocker blocker2(ui->inList);
    QSignalBlocker blocker3(ui->treeWidget);

    for (auto &v : itemChanged) {
        const auto &objT = v.first;
        Qt::CheckState state = v.second;
        if (auto obj = objT.getObject()) {
            std::set<App::DocumentObject*> set;
            setItemState(set, obj, state, true);
        }
    }
    itemChanged.clear();
    updateAllItemState();
}

QTreeWidgetItem *DlgObjectSelection::createDepItem(QTreeWidget *parent, App::DocumentObject *obj)
{
    auto item = new QTreeWidgetItem(parent);
    if (parent == ui->depList)
        depMap[obj] = item;
    else
        inMap[obj] = item;
    App::SubObjectT objT(obj);
    auto vp = Gui::Application::Instance->getViewProvider(obj);
    if(vp) item->setIcon(0, vp->getIcon());
    item->setData(0, Qt::UserRole, QVariant::fromValue(objT));
    item->setToolTip(0, QString::fromUtf8(objT.getObjectFullName().c_str()));
    item->setText(0, QString::fromUtf8((obj)->Label.getValue()));
    if (std::binary_search(initSels.begin(), initSels.end(), obj)) {
        QFont font = item->font(0);
        font.setBold(true);
        font.setItalic(true);
        item->setFont(0, font);
    }
    item->setText(1, QString::fromUtf8(obj->getDocument()->getName()));
    item->setText(2, QString::fromLatin1(obj->getNameInDocument()));
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    auto it = itemMap.find(obj);
    if (it != itemMap.end())
        setCheckState(item, it->second->checkState(0));
    return item;
}

void DlgObjectSelection::onItemSelectionChanged() {
    ui->depList->clear();
    depMap.clear();
    ui->inList->clear();
    inMap.clear();

    std::vector<App::DocumentObject *> sels;
    for (auto item : ui->treeWidget->selectedItems()) {
        if (item == allItem) {
            sels.clear();
            break;
        }
        auto obj = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole)).getObject();
        if (obj)
            sels.push_back(obj);
    }

    std::vector<App::DocumentObject*> _deps;
    if (sels.size()) {
        std::sort(sels.begin(), sels.end());
        for (auto dep : App::Document::getDependencyList(sels, App::Document::DepSort)) {
            if (!std::binary_search(sels.begin(), sels.end(), dep))
                _deps.push_back(dep);
        }
    }

    bool enabled = ui->depList->isSortingEnabled();
    if (enabled)
        ui->depList->setSortingEnabled(false);

    bool enabled2 = ui->inList->isSortingEnabled();
    if (enabled2)
        ui->inList->setSortingEnabled(false);

    {
        QSignalBlocker blocker(ui->depList);
        for (auto obj : sels.size() ? _deps : deps)
            createDepItem(ui->depList, obj);
    }

    std::set<App::DocumentObject*> inlist;
    for (auto obj : sels)
        obj->getInListEx(inlist, true);
    for (auto it = inlist.begin(); it != inlist.end();) {
        if (!depSet.count(*it) || std::binary_search(sels.begin(), sels.end(), *it))
            it = inlist.erase(it);
        else
            ++it;
    }
    {
        QSignalBlocker blocker2(ui->inList);
        for (auto obj : inlist)
            createDepItem(ui->inList, obj);
    }
        
    if (enabled)
        ui->depList->setSortingEnabled(true);
    if (enabled2)
        ui->inList->setSortingEnabled(true);
}

void DlgObjectSelection::accept() {
    QDialog::accept();
}

void DlgObjectSelection::reject() {
    QDialog::reject();
}

void DlgObjectSelection::addCheckBox(QCheckBox *box) {
    ui->horizontalLayout->insertWidget(0, box);
}

void DlgObjectSelection::setMessage(const QString &msg) {
    ui->label->setText(msg);
}

void DlgObjectSelection::showEvent(QShowEvent *e) {
    if (!geometryRestored) {
        geometryRestored = true;
        std::string geometry = hGrp->GetASCII("ObjectSelectionGeometry", "");
        std::istringstream iss(geometry);
        int x,y,w,h;
        if (iss >> x >> y >> w >> h) {
            if (ViewParams::getCheckWidgetPlacementOnRestore()) {
                QRect rect = QApplication::desktop()->availableGeometry(getMainWindow());
                x = std::max<int>(rect.left(), std::min<int>(rect.left()+rect.width()/2, x));
                y = std::max<int>(rect.top(), std::min<int>(rect.top()+rect.height()/2, y));
                w = std::min<int>(rect.width(), w);
                h = std::min<int>(rect.height(), h);
            }
            this->move(x, y);
            this->resize(w, h);
        }
    }
    QDialog::showEvent(e);
}

void DlgObjectSelection::hideEvent(QHideEvent *ev)
{
    saveGeometry();
    QDialog::hideEvent(ev);
}

void DlgObjectSelection::saveGeometry()
{
    std::ostringstream oss;
    oss << savedPos.x() << " " << savedPos.y() << " "
        << savedSize.width() << " " << savedSize.height();
    hGrp->SetASCII("ObjectSelectionGeometry", oss.str().c_str());
}

void DlgObjectSelection::resizeEvent(QResizeEvent* ev)
{
    savedSize = ev->size();
    QDialog::resizeEvent(ev);
}

void DlgObjectSelection::onAutoDeps(bool checked)
{
    hGrp->SetBool("ObjectSelectionAutoDeps", checked);
    if (!checked)
        return;

    QSignalBlocker blocker(ui->treeWidget);
    for (auto obj : deps) {
        auto it = itemMap.find(obj);
        if (it == itemMap.end())
            continue;
        auto item = it->second;
        if (item->checkState(0) == Qt::Unchecked)
            continue;
        Qt::CheckState state = Qt::Checked;
        for (auto o : obj->getOutList()) {
            auto it = itemMap.find(o);
            if (it == itemMap.end())
                continue;
            if (it->second->checkState(0) != Qt::Checked) {
                state = Qt::PartiallyChecked;
                break;
            }
        }
        setCheckState(item, state);
    }
    onItemSelectionChanged();
}

void DlgObjectSelection::moveEvent(QMoveEvent *ev)
{
    savedPos = this->pos();
    QDialog::moveEvent(ev);
}

void DlgObjectSelection::closeEvent(QCloseEvent *e)
{
    saveGeometry();
    QDialog::closeEvent(e);
}

#include "moc_DlgObjectSelection.cpp"
