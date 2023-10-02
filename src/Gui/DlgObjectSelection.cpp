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
# include <QPushButton>
# include <QTreeWidget>
# include <QCheckBox>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>

#include "DlgObjectSelection.h"
#include "ui_DlgObjectSelection.h"
#include "Application.h"
#include "MainWindow.h"
#include "ViewProviderDocumentObject.h"
#include "MetaTypes.h"
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

static bool inline setCheckState(QTreeWidgetItem *item, Qt::CheckState state, bool forced=true)
{
    if (!forced) {
        if (item->isSelected()) {
            if (state == Qt::Unchecked || item->checkState(0) == Qt::Unchecked)
                return false;
        }
        if (item->checkState(0) == state)
            return false;
    }
    // auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    // FC_MSG(objT.getObjectFullName() << (state == Qt::Unchecked ? " unchecked" :
    //             (state == Qt::Checked ? " checked" : " partial")));
    item->setCheckState(0, state);
    return true;
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
    connect(ui->checkBoxAutoDeps, &QCheckBox::toggled, this, &DlgObjectSelection::onAutoDeps);

    ui->checkBoxShowDeps->setChecked(hGrp->GetBool("ObjectSelectionShowDeps", false));
    QObject::connect(ui->checkBoxShowDeps, &QCheckBox::toggled,
        [this](bool checked) {
            hGrp->SetBool("ObjectSelectionShowDeps", checked);
            onShowDeps();
        });
    QMetaObject::invokeMethod(this, "onShowDeps", Qt::QueuedConnection);

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

    connect(ui->treeWidget, &QTreeWidget::itemExpanded,
            this, &DlgObjectSelection::onItemExpanded);

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
        if (it != filter.end() && *it == obj)
            setItemState(obj, Qt::Unchecked);
    }
    onItemSelectionChanged();

    /**
     * create useOriginalsBtn and add to the button box
     * tried adding to .ui file, but could never get the
     * formatting exactly the way I wanted it. -- <TheMarkster>
     */
    useOriginalsBtn = new QPushButton(tr("&Use Original Selections"));
    useOriginalsBtn->setToolTip(tr("Ignore dependencies and proceed with objects\noriginally selected prior to opening this dialog"));
    ui->buttonBox->addButton(useOriginalsBtn, QDialogButtonBox::ResetRole);

    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &DlgObjectSelection::onObjItemChanged);
    connect(ui->depList, &QTreeWidget::itemChanged, this, &DlgObjectSelection::onDepItemChanged);
    connect(ui->inList, &QTreeWidget::itemChanged, this, &DlgObjectSelection::onDepItemChanged);
    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged,
            this, &DlgObjectSelection::onItemSelectionChanged);
    connect(useOriginalsBtn, &QPushButton::clicked,
            this, &DlgObjectSelection::onUseOriginalsBtnClicked);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DlgObjectSelection::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &DlgObjectSelection::reject);

    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, this, &DlgObjectSelection::checkItemChanged);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgObjectSelection::~DlgObjectSelection()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

QTreeWidgetItem *DlgObjectSelection::getItem(App::DocumentObject *obj,
                                             std::vector<QTreeWidgetItem*> **pitems,
                                             QTreeWidgetItem *parent)
{
    auto &items = itemMap[App::SubObjectT(obj, "")];
    if (pitems)
        *pitems = &items;
    QTreeWidgetItem *item;
    if (!parent) {
        if (!items.empty())
            return items[0];
        item = new QTreeWidgetItem(ui->treeWidget);
        auto vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                Gui::Application::Instance->getViewProvider(obj));
        if (vp) item->setIcon(0, vp->getIcon());
        App::SubObjectT objT(obj, "");
        item->setText(0, QString::fromUtf8((obj)->Label.getValue()));
        if (std::binary_search(initSels.begin(), initSels.end(), obj)) {
            QFont font = item->font(0);
            font.setBold(true);
            font.setItalic(true);
            item->setFont(0, font);
        }
        item->setToolTip(0, QString::fromUtf8(objT.getObjectFullName().c_str()));
        item->setData(0, Qt::UserRole, QVariant::fromValue(objT));
        item->setChildIndicatorPolicy(obj->getOutList().empty() ?
                QTreeWidgetItem::DontShowIndicator : QTreeWidgetItem::ShowIndicator);
    } else if (!items.empty()) {
        item = new QTreeWidgetItem(parent);
        item->setIcon(0, items[0]->icon(0));
        item->setText(0, items[0]->text(0));
        item->setFont(0, items[0]->font(0));
        item->setToolTip(0, items[0]->toolTip(0));
        item->setData(0, Qt::UserRole, items[0]->data(0, Qt::UserRole));
        item->setChildIndicatorPolicy(items[0]->childIndicatorPolicy());
        item->setCheckState(0, items[0]->checkState(0));
    } else
        return nullptr;
    items.push_back(item);
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    return item;
}

void DlgObjectSelection::onItemExpanded(QTreeWidgetItem *item)
{
    if (item->childCount())
        return;
    if (auto obj = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole)).getObject()) {
        QSignalBlocker blocker(ui->treeWidget);
        std::set<App::DocumentObject*> set;
        for (auto child : obj->getOutList()) {
            if (child && set.insert(child).second)
                getItem(child, nullptr, item);
        }
    }
}

void DlgObjectSelection::updateAllItemState()
{
    int count = 0;
    for (const auto &v : itemMap) {
        auto state = v.second[0]->checkState(0);
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
    else if (!count)
        allItem->setCheckState(0, Qt::Unchecked);
}

void DlgObjectSelection::setItemState(App::DocumentObject *obj,
                                      Qt::CheckState state,
                                      bool forced)
{
    std::vector<QTreeWidgetItem*> *items = nullptr;
    auto item = getItem(obj, &items);
    if (!setCheckState(item, state, forced))
        return;

    for (size_t i=1; i<items->size(); ++i)
        setCheckState(items->at(i), state, true);

    std::vector<App::DocumentObject*> objs = {obj};

    if (ui->checkBoxAutoDeps->isChecked() && state == Qt::Checked) {
        // If an object is newly checked, check all its dependencies
        for (auto o : obj->getOutListRecursive()) {
            if (!depSet.count(o) || itemChanged.count(o))
                continue;
            auto itItem = itemMap.find(o);
            if (itItem == itemMap.end() || itItem->second[0]->checkState(0) == state)
                continue;

            for (auto i : itItem->second)
                setCheckState(i, state, true);
            objs.push_back(o);
        }
    }

    for(auto obj : objs) {
        auto it = inMap.find(obj);
        if (it != inMap.end())
            setCheckState(it->second, state);

        auto itDep = depMap.find(obj);
        if (itDep != depMap.end())
            setCheckState(itDep->second, state);

        // If an object toggles state, we need to revisit all its in-list
        // object to update the partial/full checked state.
        for (auto o : obj->getInList()) {
            if (!depSet.count(o) ||itemChanged.count(o))
                continue;
            auto it = itemMap.find(o);
            if (it == itemMap.end() || it->second[0]->checkState(0) == state)
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
                auto s = it->second[0]->checkState(0);
                if (s == Qt::Unchecked)
                    continue;
                if (it->second[0]->checkState(0) == Qt::PartiallyChecked) {
                    selcount = -1;
                    break;
                }
                ++selcount;
            }
            auto state = it->second[0]->checkState(0);
            if (state == Qt::Checked && selcount != count)
                setItemState(o, Qt::PartiallyChecked, true);
            else if (state == Qt::PartiallyChecked && selcount == count)
                setItemState(o, Qt::Checked, true);
        }
    }
}

std::vector<App::DocumentObject*> DlgObjectSelection::getSelections(SelectionOptions options) const {

    if (returnOriginals)
        return initSels;

    std::vector<App::DocumentObject*> res;
    Base::Flags<SelectionOptions> flags(options);
    if (!flags.testFlag(SelectionOptions::Invert)) {
        for (const auto &v : itemMap) {
            if (v.second[0]->checkState(0) == Qt::Unchecked)
                continue;
            if (auto obj = v.first.getObject())
                res.push_back(obj);
        }
    } else {
        for (auto obj : deps) {
            auto it = itemMap.find(obj);
            if (it == itemMap.end() || it->second[0]->checkState(0) == Qt::Unchecked)
                res.push_back(obj);
        }
    }
    if (flags.testFlag(SelectionOptions::Sort))
        std::sort(res.begin(), res.end());
    return res;
}

void DlgObjectSelection::onDepItemChanged(QTreeWidgetItem * depItem, int column) {
    if(column)
        return;
    QSignalBlocker blocker(ui->depList);
    QSignalBlocker blocker2(ui->inList);
    QSignalBlocker blocker3(ui->treeWidget);
    auto state = depItem->checkState(0);
    if (depItem->isSelected()) {
        const auto items = depItem->treeWidget()->selectedItems();
        for (auto item : items) {
            auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
            auto it = itemMap.find(objT);
            if (it == itemMap.end())
                continue;
            setCheckState(item, state);
            for (auto i : it->second)
                setCheckState(i, state);
            itemChanged[objT] = state;
        }
    } else {
        auto objT = qvariant_cast<App::SubObjectT>(depItem->data(0, Qt::UserRole));
        auto it = itemMap.find(objT);
        if (it != itemMap.end()) {
            itemChanged[objT] = state;
            for (auto i : it->second)
                setCheckState(i, state);
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
            for (const auto &v : itemMap) {
                for (auto i : v.second)
                    setCheckState(i, Qt::Unchecked);
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
        // We treat selected item in tree widget specially in case of checking
        // items in depList or inList. To simplify logic, we change selection
        // here if an unselected item has been checked.
        itemChanged[qvariant_cast<App::SubObjectT>(objItem->data(0, Qt::UserRole))] = state;
        onItemSelectionChanged();
    }
    else {
        const auto items = ui->treeWidget->selectedItems();
        for (auto item : items) {
            setCheckState(item, state);
            itemChanged[qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole))] = state;
        }
    }
    timer.start(10);
}


static bool getOutList(App::DocumentObject *obj,
                       std::set<App::DocumentObject*> &visited,
                       std::vector<App::DocumentObject*> &result)
{
    if (!visited.insert(obj).second)
        return false;

    for (auto o : obj->getOutList()) {
        if (getOutList(o, visited, result))
            result.push_back(o);
    }
    return true;
}

void DlgObjectSelection::checkItemChanged() {

    QSignalBlocker blocker(ui->depList);
    QSignalBlocker blocker2(ui->inList);
    QSignalBlocker blocker3(ui->treeWidget);

    std::set<App::DocumentObject*> unchecked;

    for (const auto &v : itemChanged) {
        const auto &objT = v.first;
        Qt::CheckState state = v.second;
        if (auto obj = objT.getObject()) {
            if (state == Qt::Unchecked) {
                // We'll deal with unchecked item later
                if (ui->checkBoxAutoDeps->isChecked())
                    unchecked.insert(obj);
            } else {
                // For checked item, setItemState will auto select its
                // dependency
                setItemState(obj, state, true);
            }
        }
    }

    if (!unchecked.empty()) {
        // When some item is unchecked by the user, we need to re-check the
        // recursive outlist of the initially selected object, excluding all
        // currently unchecked object. And then uncheck any item that does not
        // appear in the returned outlist.

        for (const auto &v : itemMap) {
            auto item = v.second[0];
            if (item->checkState(0) == Qt::Unchecked) {
                if (auto obj = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole)).getObject())
                    unchecked.insert(obj);
            }
        }

        auto outlist = initSels;
        for (auto obj : initSels)
            getOutList(obj, unchecked, outlist);
        std::sort(outlist.begin(), outlist.end());

        for (const auto &v : itemMap) {
            if (itemChanged.count(v.first) == 0 && v.second[0]->checkState(0) == Qt::Unchecked)
                continue;
            if (auto obj = v.first.getObject()) {
                if (!std::binary_search(outlist.begin(), outlist.end(), obj))
                    setItemState(obj, Qt::Unchecked, true);
            }
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
    item->setText(2, QString::fromUtf8(obj->getNameInDocument()));
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    auto it = itemMap.find(obj);
    if (it != itemMap.end())
        setCheckState(item, it->second[0]->checkState(0));
    return item;
}

void DlgObjectSelection::onItemSelectionChanged() {
    ui->depList->clear();
    depMap.clear();
    ui->inList->clear();
    inMap.clear();

    std::vector<App::DocumentObject *> sels;
    const auto items = ui->treeWidget->selectedItems();
    for (auto item : items) {
        if (item == allItem) {
            sels.clear();
            break;
        }
        auto obj = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole)).getObject();
        if (obj)
            sels.push_back(obj);
    }

    std::vector<App::DocumentObject*> _deps;
    if (!sels.empty()) {
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
        auto &objs = !sels.empty() ? _deps : deps;
        for (auto it = objs.rbegin(); it != objs.rend(); ++it)
            createDepItem(ui->depList, *it);
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

void DlgObjectSelection::onUseOriginalsBtnClicked() {
    returnOriginals = true;
    QDialog::accept();
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
        auto item = it->second[0];
        if (item->checkState(0) == Qt::Unchecked)
            continue;
        Qt::CheckState state = Qt::Checked;
        for (auto o : obj->getOutList()) {
            auto it = itemMap.find(o);
            if (it == itemMap.end())
                continue;
            if (it->second[0]->checkState(0) != Qt::Checked) {
                state = Qt::PartiallyChecked;
                break;
            }
        }
        for (auto i : it->second)
            setCheckState(i, state);
    }
    onItemSelectionChanged();
}

void DlgObjectSelection::onShowDeps()
{
    bool checked = ui->checkBoxShowDeps->isChecked();
    auto sizes = ui->vsplitter->sizes();
    if (!checked && sizes[1] > 0)
        sizes[1] = 0;
    else if (checked && (sizes[0] == 0 || sizes[1] == 0))
        sizes[0] = sizes[1] = this->width()/2;
    else
        return;
    ui->vsplitter->setSizes(sizes);
}

#include "moc_DlgObjectSelection.cpp"
