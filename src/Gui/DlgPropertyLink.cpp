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
# include <QPushButton>
# include <QVBoxLayout>
#endif

#include <QStyledItemDelegate>

#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/DocumentObserver.h>

#include "Document.h"
#include "View3DInventor.h"
#include "Tree.h"
#include "Selection.h"
#include "PropertyView.h"
#include "BitmapFactory.h"
#include "DlgPropertyLink.h"
#include "Application.h"
#include "ViewProviderDocumentObject.h"
#include "MetaTypes.h"
#include "ui_DlgPropertyLink.h"

using namespace Gui::Dialog;

class ItemDelegate: public QStyledItemDelegate {
public:
    ItemDelegate(QObject* parent=0): QStyledItemDelegate(parent) {}

    virtual QWidget* createEditor(QWidget *parent,
            const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        if(index.column() != 1)
            return nullptr;
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
};

/* TRANSLATOR Gui::Dialog::DlgPropertyLink */

DlgPropertyLink::DlgPropertyLink(QWidget* parent, int flags)
  : QWidget(parent)
  , ui(new Ui_DlgPropertyLink)
  , flags(flags)
{
    ui->setupUi(this);

    ui->typeTree->hide();
    if(flags & NoTypeFilter) {
        QSignalBlocker blocker(ui->checkObjectType);
        ui->checkObjectType->setChecked(true);
        ui->checkObjectType->hide();
    }

    if(flags & NoSearchBox) {
        ui->searchBox->hide();
        ui->labelSearch->hide();
    } else {
        ui->searchBox->installEventFilter(this);
        ui->searchBox->setNoProperty(true);
        connect(ui->searchBox, SIGNAL(returnPressed()), this, SLOT(onItemSearch()));
    }

    ui->checkSubObject->setChecked(false);
    if(flags & NoSyncSubObject)
        ui->checkSubObject->hide();
    else if(flags & AlwaysSyncSubObject) {
        ui->checkSubObject->hide();
        ui->checkSubObject->setChecked(true);
    }

    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));

    ui->treeWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->treeWidget->setItemDelegate(new ItemDelegate(this));
    ui->treeWidget->setMouseTracking(true);
    connect(ui->treeWidget, SIGNAL(itemEntered(QTreeWidgetItem*, int)),
            this, SLOT(onItemEntered(QTreeWidgetItem*)));

    ui->treeWidget->viewport()->installEventFilter(this);

    connect(ui->treeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)),
            this, SLOT(onItemExpanded(QTreeWidgetItem*)));

    connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(ui->treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));

    if(flags & NoButton)
        ui->buttonBox->hide();
    else {
        connect(ui->buttonBox, SIGNAL(accepted()), this, SIGNAL(accepted()));
        connect(ui->buttonBox, SIGNAL(rejected()), this, SIGNAL(rejected()));
        connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClicked(QAbstractButton*)));
        refreshButton = ui->buttonBox->addButton(tr("Reset"), QDialogButtonBox::ActionRole);
        resetButton = ui->buttonBox->addButton(tr("Clear"), QDialogButtonBox::ResetRole);
    }
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgPropertyLink::~DlgPropertyLink()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

QTreeWidget *DlgPropertyLink::treeWidget() {
    return ui->treeWidget;
}

QList<App::SubObjectT> DlgPropertyLink::getLinksFromProperty(const App::PropertyLinkBase *prop)
{
    QList<App::SubObjectT> res;
    if(!prop)
        return res;

    std::vector<App::DocumentObject*> objs;
    std::vector<std::string> subs;
    prop->getLinks(objs,true,&subs,false);
    if(subs.empty()) {
        for(auto obj : objs) 
            res.push_back(App::SubObjectT(obj,0));
    } else if (objs.size()==1) {
        for(auto &sub : subs) 
            res.push_back(App::SubObjectT(objs.front(),sub.c_str()));
    } else {
        int i=0;
        for(auto obj : objs) 
            res.push_back(App::SubObjectT(obj,subs[i++].c_str()));
    }
    return res;
}

QString DlgPropertyLink::formatObject(App::Document *ownerDoc, App::DocumentObject *obj, const char *sub)
{
    if(!obj || !obj->getNameInDocument())
        return QLatin1String("?");

    const char *objName = obj->getNameInDocument();
    std::string _objName;
    if(ownerDoc && ownerDoc!=obj->getDocument()) {
        _objName = obj->getFullName();
        objName = _objName.c_str();
    }

    if(!sub || !sub[0]) {
        if(obj->Label.getStrValue() == obj->getNameInDocument())
            return QLatin1String(objName);
        return QString::fromLatin1("%1 (%2)").arg(QLatin1String(objName),
                                                  QString::fromUtf8(obj->Label.getValue()));
    }

    auto sobj = obj->getSubObject(sub);
    if(!sobj || sobj->Label.getStrValue() == sobj->getNameInDocument())
        return QString::fromLatin1("%1.%2").arg(QLatin1String(objName),
                                                QString::fromUtf8(sub));

    return QString::fromLatin1("%1.%2 (%3)").arg(QLatin1String(objName),
                                                 QString::fromUtf8(sub),
                                                 QString::fromUtf8(sobj->Label.getValue()));
}

static inline bool isLinkSub(QList<App::SubObjectT> links)
{
    for(auto &link : links) {
        if(&link == &links.front())
            continue;
        if(link.getDocumentName() != links.front().getDocumentName()
                || link.getObjectName() != links.front().getObjectName())
        {
            return false;
        }
    }
    return true;
}

void DlgPropertyLink::setItemLabel(QTreeWidgetItem *item, std::size_t idx) {
    if(singleSelect)
        return;

    QString label = item->data(0, Qt::UserRole+4).toString();
    if(!idx)
        item->setText(0, label);
    else
        item->setText(0, QString::fromLatin1("%1: %2").arg(idx).arg(label));
}

QString DlgPropertyLink::formatLinks(App::Document *ownerDoc, QList<App::SubObjectT> links)
{
    if(!ownerDoc || links.empty())
        return QString();

    auto obj = links.front().getObject();
    if(!obj)
        return QLatin1String("?");

    if(links.size() == 1 && links.front().getSubName().empty())
        return formatObject(ownerDoc, links.front());

    QStringList list;
    if(isLinkSub(links)) {
        int i = 0;
        for(auto &link : links) {
            list << QString::fromUtf8(link.getSubName().c_str());
            if( ++i >= 3)
                break;
        }
        return QString::fromLatin1("%1 [%2%3]").arg(formatObject(ownerDoc,obj,0),
                                                    list.join(QLatin1String(", ")),
                                                    QLatin1String(links.size()>3?" ...":""));
    }

    int i = 0;
    for(auto &link : links) {
        list << formatObject(ownerDoc,link);
        if( ++i >= 3)
            break;
    }
    return QString::fromLatin1("[%1%2]").arg(list.join(QLatin1String(", ")),
                                             QLatin1String(links.size()>3?" ...":""));
}

void DlgPropertyLink::setTypeFilter(std::set<QByteArray> &&filter)
{
    selectedTypes = std::move(filter);
}

void DlgPropertyLink::setTypeFilter(const std::vector<Base::Type> &types)
{
    selectedTypes.clear();
    for(auto &type : types) {
        const char *name = type.getName();
        selectedTypes.insert(QByteArray::fromRawData(name,strlen(name)+1));
    }
}

void DlgPropertyLink::setTypeFilter(Base::Type type)
{
    selectedTypes.clear();
    const char *name = type.getName();
    selectedTypes.insert(QByteArray::fromRawData(name,strlen(name)+1));
}

void DlgPropertyLink::setContext(App::SubObjectT &&ctx)
{
    selContext = std::move(ctx);
}

void DlgPropertyLink::setInitObjects(std::vector<App::DocumentObjectT> &&objs)
{
    initObjs = std::move(objs);
}

static std::pair<App::DocumentObject *, const char *> resolveContext(
        const App::SubObjectT &ctx, std::string &subname, const App::SubObjectT &sobjT)
{
    auto obj = ctx.getObject();
    if (!obj) {
        subname = sobjT.getSubName();
        return {sobjT.getObject(), subname.c_str()};
    }
    subname = ctx.getSubName();
    subname += sobjT.getObjectName();
    subname += ".";
    auto sobj = sobjT.getObject();
    if (!sobj)
        return {nullptr, nullptr};
    if(obj->getSubObject(subname.c_str()) == sobj) {
        subname += sobjT.getSubName();
        return {obj, subname.c_str()};
    }

    subname = sobjT.getSubName();
    return {sobj, subname.c_str()};
}

void DlgPropertyLink::init(const App::DocumentObjectT &prop, bool tryFilter)
{
    {
        QSignalBlocker blocker(ui->typeTree);
        ui->typeTree->clear();
    }

    {
        QSignalBlocker blocker(ui->treeWidget);
        ui->treeWidget->clear();
    }
    oldLinks.clear();
    docItems.clear();
    typeItems.clear();
    itemMap.clear();
    inList.clear();
    if(tryFilter)
        selectedTypes.clear();
    currentObj = nullptr;
    searchItem = nullptr;
    elementSels.clear();
    selections.clear();

    objProp  = prop;
    auto owner = objProp.getObject();
    if(!owner || !owner->getNameInDocument())
        return;

    ui->searchBox->setDocumentObject(owner);

    if (selContext.getObjectName().empty()) {
        for(auto &sel : Gui::Selection().getSelectionT("*", false)) {
            if (sel.getSubObject() == owner) {
                if (sel.getObject() != owner) 
                    selContext = sel.getParent();
                break;
            }
        }
    }

    auto propLink = Base::freecad_dynamic_cast<App::PropertyLinkBase>(objProp.getProperty());
    if(!propLink)
        return;

    oldLinks = getLinksFromProperty(propLink);

    if(propLink->getScope() != App::LinkScope::Hidden) {
        // populate inList to filter out any objects that contains the owner object
        // of the editing link property
        inList = owner->getInListEx(true);
        inList.insert(owner);
    }

    std::vector<App::Document*> docs;

    singleSelect = false;
    if(propLink->isDerivedFrom(App::PropertyXLinkSub::getClassTypeId())
            || propLink->isDerivedFrom(App::PropertyLinkSub::getClassTypeId()))
    {
        allowSubObject = true;
        singleParent = true;
    } else if (propLink->isDerivedFrom(App::PropertyLink::getClassTypeId())) {
        singleSelect = true;
    } else if (propLink->isDerivedFrom(App::PropertyLinkSubList::getClassTypeId())) {
        allowSubObject = true;
    }

    std::vector<App::DocumentObject*> objs;

    isXLink = false;

    if(App::PropertyXLink::supportXLink(propLink)) {
        allowSubObject = true;
        docs = App::GetApplication().getDocuments();
        isXLink = true;
    } else if (initObjs.empty()) {
        objs = owner->getDocument()->getObjects();
    } else {
        objs.reserve(initObjs.size());
        for(auto &o : initObjs) 
            objs.push_back(o.getObject());
    }

    bool isLinkList = false;
    if (propLink->isDerivedFrom(App::PropertyXLinkList::getClassTypeId())
            || propLink->isDerivedFrom(App::PropertyLinkList::getClassTypeId()))
    {
        isLinkList = true;
        allowSubObject = false;
    } 

    if(flags & NoSubObject)
        allowSubObject = false;

    if(singleSelect) {
        singleParent = true; 
        ui->treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    } else {
        ui->treeWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    }

    if(!(flags & (NoSyncSubObject|AlwaysSyncSubObject)))
        ui->checkSubObject->setVisible(allowSubObject);

    if(!allowSubObject && !(flags & AllowSubElement)) {
        ui->treeWidget->setHeaderHidden(true);
        ui->treeWidget->setColumnCount(1);
        ui->treeWidget->setSelectionBehavior(QTreeWidget::SelectRows);
    } else {
        ui->treeWidget->setSelectionBehavior(QTreeWidget::SelectItems);
        ui->treeWidget->setHeaderHidden(false);
        ui->treeWidget->headerItem()->setText(0, tr("Object"));
        ui->treeWidget->headerItem()->setText(1, tr("Element"));
        ui->treeWidget->setColumnCount(2);

        // make sure to show a horizontal scrollbar if needed
#if QT_VERSION >= 0x050000
        ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
#else
        ui->treeWidget->header()->setResizeMode(0, QHeaderView::ResizeToContents);
#endif
    }
        
    std::set<App::Document*> expandDocs;

    if(oldLinks.empty()) {
        expandDocs.insert(owner->getDocument());
    } else {
        for(auto &link : oldLinks) {
            auto doc = link.getDocument();
            if(doc)
                expandDocs.insert(doc);
        }
    }

    if(objs.size()) {
        for(auto obj : objs) {
            if(!obj || itemMap.count(obj))
                continue;
            auto item = createItem(obj,nullptr);
            if(item)
                itemMap[obj] = item;
        }
    } else {
        QPixmap docIcon(Gui::BitmapFactory().pixmap("Document"));
        for(auto d : docs) {
            auto item = new QTreeWidgetItem(ui->treeWidget);
            item->setIcon(0, docIcon);
            item->setText(0, QString::fromUtf8(d->Label.getValue()));
            item->setData(0, Qt::UserRole, QByteArray(""));
            item->setData(0, Qt::UserRole+1, QByteArray(d->getName()));
            item->setFlags(Qt::ItemIsEnabled);
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            if(expandDocs.count(d))
                item->setExpanded(true);
            docItems[d] = item;
        }
    }

    if(oldLinks.isEmpty())
        return;

    if(allowSubObject && !(flags & (NoSyncSubObject|AlwaysSyncSubObject))) {
        if (propLink->testFlag(App::PropertyLinkBase::LinkSyncSubObject))
            ui->checkSubObject->setChecked(true);
        else {
            for(auto &link : oldLinks) {
                auto sobj = link.getSubObject();
                if(sobj && sobj!=link.getObject()) {
                    ui->checkSubObject->setChecked(true);
                    break;
                }
            }
        }
    }

    // Try to select items corresponding to the current links inside the
    // property
    std::string subname;
    for(auto &link : oldLinks) {
        auto res = resolveContext(selContext, subname, link);
        if(!res.first)
            continue;
        selectionChanged(Gui::SelectionChanges(SelectionChanges::AddSelection,
                                               res.first->getDocument()->getName(),
                                               res.first->getNameInDocument(),
                                               res.second));
    }

    // For link list type property, try to auto filter type
    if(tryFilter && isLinkList) {
        Base::Type objType;
        for(auto link : oldLinks) {
            auto obj = link.getSubObject();
            if(!obj)
                continue;
            if(objType.isBad()) {
                objType = obj->getTypeId();
                continue;
            }
            for(;objType != App::DocumentObject::getClassTypeId();
                 objType = objType.getParent())
            {
                if(obj->isDerivedFrom(objType))
                    break;
            }
        }

        Base::Type baseType;
        // get only geometric types
        if (objType.isDerivedFrom(App::GeoFeature::getClassTypeId()))
            baseType = App::GeoFeature::getClassTypeId();
        else
            baseType = App::DocumentObject::getClassTypeId();

        // get the direct base class of App::DocumentObject which 'obj' is derived from
        while (!objType.isBad()) {
            Base::Type parType = objType.getParent();
            if (parType == baseType) {
                baseType = objType;
                break;
            }
            objType = parType;
        }

        if(!baseType.isBad()) {
            const char *name = baseType.getName();
            auto it = typeItems.find(QByteArray::fromRawData(name,strlen(name)+1));
            if(it != typeItems.end())
                it->second->setSelected(true);
            ui->checkObjectType->setChecked(true);
        }
    }
}

void DlgPropertyLink::onClicked(QAbstractButton *button) {
    if(button == resetButton) {
        QSignalBlocker blocker(ui->treeWidget);
        clearSelection(nullptr);
        Gui::Selection().clearSelection();
    } else if (button == refreshButton) {
        init(objProp);
    }
}

void DlgPropertyLink::attachObserver(Gui::SelectionObserver *observer) {
    if(observer->isConnectionAttached())
        return;

    Gui::Selection().selStackPush();
    observer->attachSelection();

    if(!parentView) {
        for(auto p=parent(); p; p=p->parent()) {
            auto view = qobject_cast<Gui::PropertyView*>(p);
            if(view) {
                parentView = view;
                for(auto &sel : Gui::Selection().getCompleteSelection(0))
                    savedSelections.emplace_back(sel.DocName, sel.FeatName, sel.SubName);
                break;
            }
        }
    }
    auto view = qobject_cast<Gui::PropertyView*>(parentView.data());
    if(view)
        view->blockConnection(true);
}

void DlgPropertyLink::onItemEntered(QTreeWidgetItem *) {
    if(!timer->isActive() && enterTime.elapsed() < Gui::TreeParams::Instance()->PreSelectionDelay()) {
        onTimer();
        return;
    }
    int timeout = Gui::TreeParams::Instance()->PreSelectionDelay()/2;
    if(timeout < 0)
        timeout = 1;
    timer->start(timeout);
}

void DlgPropertyLink::leaveEvent(QEvent *ev) {
    timer->stop();
    Gui::Selection().rmvPreselect();
    QWidget::leaveEvent(ev);
}

void DlgPropertyLink::detachObserver(Gui::SelectionObserver *observer) {
    if(observer->isConnectionAttached())
        observer->detachSelection();

    auto view = qobject_cast<Gui::PropertyView*>(parentView.data());
    if(view && savedSelections.size()) {
        Gui::Selection().clearSelection();
        for(auto &sel : savedSelections) {
            if(sel.getSubObject())
                Gui::Selection().addSelection(sel.getDocumentName().c_str(),
                                              sel.getObjectName().c_str(),
                                              sel.getSubName().c_str());
        }
        savedSelections.clear();
    }
    if(view)
        view->blockConnection(false);

    parentView = nullptr;
}

void DlgPropertyLink::onCurrentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem*)
{
    if(!item)
        return;
    int column = ui->treeWidget->currentColumn();

    auto sobjs = getLinkFromItem(item, column==1);
    App::DocumentObject *obj = sobjs.size()?sobjs.front().getObject():nullptr;
    if(!obj) {
        Gui::Selection().clearSelection();
        return;
    }

    bool focus = false;
    // Do auto view switch if tree view does not do it
    if(!TreeParams::Instance()->SyncView() && selContext.getObjectName().empty()) {
        focus = ui->treeWidget->hasFocus();
        auto doc = Gui::Application::Instance->getDocument(sobjs.front().getDocumentName().c_str());
        if(doc) {
            auto vp = Base::freecad_dynamic_cast<Gui::ViewProviderDocumentObject>(
                    doc->getViewProvider(obj));
            if(vp) {
                doc->setActiveView(vp, Gui::View3DInventor::getClassTypeId());
            }
        }
    }

    // Sync 3d view selection. Note that we selecting the current item in 3D
    // view, however the current item may not be selected in the tree widget.
    // This is by design, because we want to give user visual feedback of the
    // item so that they can make a decision to change the selection, which
    // may trigger expensive recomputes
    {
        Base::StateLocker locker(busy);
        Gui::Selection().clearSelection();

        std::string subname;
        for(auto &sobj : sobjs) {
            auto res = resolveContext(selContext, subname, sobj);
            if(!res.first) continue;
            Gui::Selection().addSelection(res.first->getDocument()->getName(),
                    res.first->getNameInDocument(), res.second);
        }
    }

    if(focus) {
        // FIXME: does not work, why?
        ui->treeWidget->setFocus();
    }
}

void DlgPropertyLink::onItemSelectionChanged()
{
    checkItemSelection();
}

void DlgPropertyLink::checkItemSelection()
{
    auto newSelections = ui->treeWidget->selectedItems();
    if(newSelections == selections)
        return;

    int column = ui->treeWidget->currentColumn();
    auto item = ui->treeWidget->currentItem();

    App::DocumentObject *obj = nullptr;
    if(item->isSelected()) {
        auto sobjs = getLinkFromItem(item, column==1);
        obj = sobjs.size()?sobjs.front().getObject():nullptr;
        if(obj && singleParent && currentObj!=obj) {
            // Enforce single parent 
            QSignalBlocker blocker(ui->treeWidget);
            for(auto sel : newSelections) {
                if(sel != item) 
                    sel->setSelected(false);
            }
            newSelections.clear();
            newSelections.append(item);
        }
    }

    // Clear elements on unselected items
    for(auto it=elementSels.begin(); it!=elementSels.end();) {
        auto sitem = *it;
        if(!sitem->isSelected()) {
            sitem->setText(1, QString());
            it = elementSels.erase(it);
        } else
            ++it;
    }

    for(auto item : selections) {
        if(!item->isSelected())
            setItemLabel(item);
    }
    std::size_t i = 0;
    for(auto item : newSelections)
        setItemLabel(item, ++i);

    currentObj = obj;
    selections = newSelections;
    linkChanged();
}

QTreeWidgetItem *DlgPropertyLink::findItem(
        App::DocumentObject *obj, const char *subname, bool *pfound)
{
    if(pfound)
        *pfound = false;

    if(!obj || !obj->getNameInDocument() || !obj->getSubObject(subname))
        return 0;

    std::vector<App::DocumentObject *> sobjs;
    if(subname && subname[0]) {
        if(!allowSubObject) {
            obj = obj->getSubObject(subname);
            if(!obj)
                return 0;
            sobjs.push_back(obj);
        } else {
            bool checking = true;
            for(auto sobj : obj->getSubObjectList(subname)) {
                if(checking && inList.count(sobj))
                    continue;
                checking = false;
                sobjs.push_back(sobj);
            }
        }
    } else
        sobjs.push_back(obj);

    if(sobjs.empty())
        return 0;

    if(docItems.size()) {
        auto itDoc = docItems.find(sobjs.front()->getDocument());
        if(itDoc == docItems.end())
            return 0;
        onItemExpanded(itDoc->second);
    }

    auto it = itemMap.find(sobjs.front());
    if(it == itemMap.end() || it->second->isHidden())
        return 0;

    if(!allowSubObject) {
        if(pfound)
            *pfound = true;
        return it->second;
    }

    QTreeWidgetItem *item = it->second;

    bool first = true;
    for(auto o : sobjs) {
        if(first) {
            first = false;
            continue;
        }
        onItemExpanded(item);
        bool found = false;
        for(int i=0,count=item->childCount();i<count;++i) {
            auto child = item->child(i);
            if(child->isHidden())
                break;
            if(strcmp(o->getNameInDocument(),
                        child->data(0, Qt::UserRole).toByteArray().constData())==0)
            {
                item = child;
                found = true;
                break;
            }
        }
        if(!found)
            return item;
    }
    if(pfound)
        *pfound = true;
    return item;
}

void DlgPropertyLink::selectionChanged(const Gui::SelectionChanges& msg)
{
    if(busy)
        return;

    if(msg.pOriginalMsg) {
        selectionChanged(*msg.pOriginalMsg);
        return;
    }

    if (msg.Type != SelectionChanges::AddSelection)
        return;

    bool found = false;
    auto selObj = msg.Object.getObject();

    std::pair<std::string,std::string> elementName;
    const char *subname = msg.pSubName;
    if(!ui->checkSubObject->isChecked()) {
        selObj = App::GeoFeature::resolveElement(selObj,subname,elementName);
        if(!selObj)
            return;
        subname = elementName.second.c_str();
    }

    auto item = findItem(selObj, subname, &found);
    if(!item || !found)
        return;

    std::string element = msg.Object.getOldElementName();
    if(elementFilter && elementFilter(msg.Object,element))
        return;

    if(!item->isSelected()) {
        QSignalBlocker blocker(ui->treeWidget);
        if(singleSelect || (singleParent && currentObj && currentObj!=selObj)) {
            clearSelection(item);
        }
        currentObj = selObj;
        ui->treeWidget->setCurrentItem(item,0);
        selections.append(item);
        setItemLabel(item, selections.size());
    }

    ui->treeWidget->scrollToItem(item);
    if(allowSubObject || (flags & AllowSubElement)) {
        if(element.size()) {
            QString e = QString::fromLatin1(element.c_str());
            QStringList list;
            QString text = item->text(1);
            if(text.size())
                list = text.split(QLatin1Char(','));
            if(list.indexOf(e)<0) {
                list << e;
                item->setText(1, list.join(QLatin1String(",")));
                elementSels.insert(item);
                linkChanged();
            }
        } else if (elementSels.erase(item)) {
            item->setText(1, QString());
            linkChanged();
        }
    }
}

void DlgPropertyLink::clearSelection(QTreeWidgetItem *itemKeep) {
    ui->treeWidget->selectionModel()->clearSelection();
    for(auto it=elementSels.begin(); it!=elementSels.end();) {
        if(*it!=itemKeep) {
            (*it)->setText(1,QString());
            it = elementSels.erase(it);
        } else
            ++it;
    }
    for(auto item : selections)
        setItemLabel(item, item==itemKeep?1:0);
    selections.clear();
    if(itemKeep)
        selections.append(itemKeep);
}

static QTreeWidgetItem *_getLinkFromItem(std::ostringstream &ss, QTreeWidgetItem *item, const char *objName) {
    auto parent = item->parent();
    if(!parent)
        return item;
    const char *nextName = parent->data(0, Qt::UserRole).toByteArray().constData();
    if(!nextName[0])
        return item;

    item = _getLinkFromItem(ss, parent, nextName);
    ss << objName << '.';
    return item;
}

static App::SubObjectT subObjectFromItem(QTreeWidgetItem *item)
{
    App::SubObjectT sobj;
    auto parent = item->parent();
    if(!parent) {
        return App::SubObjectT(item->data(0, Qt::UserRole+1).toByteArray().constData(),
                               item->data(0, Qt::UserRole).toByteArray().constData(), 0);
    }
    
    std::ostringstream ss;
    auto parentItem = _getLinkFromItem(ss, item,
            item->data(0,Qt::UserRole).toByteArray().constData());

    return App::SubObjectT(parentItem->data(0, Qt::UserRole+1).toByteArray().constData(),
                           parentItem->data(0, Qt::UserRole).toByteArray().constData(),
                           ss.str().c_str());
}

QList<App::SubObjectT>
DlgPropertyLink::getLinkFromItem(QTreeWidgetItem *item, bool needElement) const
{
    QList<App::SubObjectT> res;

    auto sobj = subObjectFromItem(item);
    if(sobj.getObjectName().empty())
        return res;

    QString elements;
    if(needElement && (allowSubObject || (flags & AllowSubElement)))
        elements = item->text(1);

    if(elements.isEmpty()) {
        res.append(App::SubObjectT());
        res.last() = std::move(sobj);
        return res;
    }

    for(const QString &element : elements.split(QLatin1Char(','))) {
        res.append(App::SubObjectT());
        res.last() = App::SubObjectT(sobj.getDocumentName().c_str(),
                                     sobj.getObjectName().c_str(),
                                     (sobj.getSubName() + element.toLatin1().constData()).c_str());
    }
    return res;
}

void DlgPropertyLink::onTimer() {
    auto pos = ui->treeWidget->viewport()->mapFromGlobal(QCursor::pos());
    auto item = ui->treeWidget->itemAt(pos);
    if(!item) 
        return;
    bool needElement = true;
    if(ui->treeWidget->columnCount() > 1) {
        int hpos = ui->treeWidget->header()->sectionPosition(1);
        auto rect = ui->treeWidget->visualItemRect(item);
        needElement = pos.x() >= rect.left() + hpos;
    }
    auto sobjs = getLinkFromItem(item, needElement);
    if(sobjs.isEmpty())
        return;
    const auto &sobj = sobjs.front();

    std::string subname;
    auto res = resolveContext(selContext, subname, sobj);
    if(res.first) {
        Gui::Selection().setPreselect(res.first->getDocument()->getName(),
                                      res.first->getNameInDocument(),
                                      res.second,
                                      0,0,0,2);
    }
    enterTime.start();
}

QList<App::SubObjectT> DlgPropertyLink::currentLinks() const
{
    auto items = ui->treeWidget->selectedItems();
    QList<App::SubObjectT> res;
    for(auto item : items) 
        res.append(getLinkFromItem(item));
    return res;
}

QList<App::SubObjectT> DlgPropertyLink::originalLinks() const
{
    return oldLinks;
}

QString DlgPropertyLink::linksToPython(QList<App::SubObjectT> links) {
    if(links.isEmpty())
        return QLatin1String("None");

    if(links.size() == 1)
        return QString::fromLatin1(links.front().getSubObjectPython(false).c_str());

    std::ostringstream ss;

    if(isLinkSub(links)) {
        ss << '(' << links.front().getObjectPython() << ", [";
        for(auto link : links) {
            const auto &sub = link.getSubName();
            if(sub.size())
                ss << "u'" << Base::Tools::escapedUnicodeFromUtf8(sub.c_str()) << "',";
        }
        ss << "])";
    } else {
        ss << '[';
        for(auto link : links)
            ss << link.getSubObjectPython(false) << ',';
        ss << ']';
    }

    return QString::fromLatin1(ss.str().c_str());
}

void DlgPropertyLink::filterObjects()
{
    for(int i=0, count=ui->treeWidget->topLevelItemCount(); i<count; ++i) {
        auto item = ui->treeWidget->topLevelItem(i);
        if(!isXLink) {
            filterItem(item);
            continue;
        }
        for(int j=0, c=item->childCount(); j<c; ++j)
            filterItem(item->child(j));
    }
}

void DlgPropertyLink::filterItem(QTreeWidgetItem *item)
{
    if(filterType(item)) {
        item->setHidden(true);
        return;
    }
    if(objFilter && objFilter(subObjectFromItem(item))) {
        item->setHidden(true);
        return;
    }
    item->setHidden(false);
    for(int i=0, count=item->childCount(); i<count; ++i)
        filterItem(item->child(i));
}

bool DlgPropertyLink::eventFilter(QObject *obj, QEvent *e) {
    if(obj == ui->searchBox) {
        if(e->type() == QEvent::KeyPress
            && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape)
        {
            ui->searchBox->setText(QString());
            return true;
        }
    } else if (obj == ui->treeWidget->viewport()) {
        if(e->type() == QEvent::MouseButtonPress) {
            // In case there are two columns, we block item selection via
            // clicking the second column. Instead, we use the click to set the
            // current item only, and by handling the currentItemChanged()
            // event, we'll set the selection in 3D view. This allows user the
            // chance to bring the item on top for visual inspection without
            // changing the item selection, which may trigger linkChanged()
            // event and potentially expensive recomputation.  
            if(ui->treeWidget->columnCount() <= 1)
                return false;
            auto ke = static_cast<QMouseEvent*>(e);
            auto pos = ke->pos();
            auto item = ui->treeWidget->itemAt(pos);
            if(!item) 
                return false;
            int hpos = ui->treeWidget->header()->sectionPosition(1);
            auto rect = ui->treeWidget->visualItemRect(item);
            if(pos.x() >= rect.left() + hpos) {
                if(ui->treeWidget->currentItem() == item && ui->treeWidget->currentColumn() == 1) {
                    // already the current item, currentItemChanged() won't be triggered,,
                    // so call the slot manually to make the 3D selection
                    onCurrentItemChanged(item, nullptr);
                } else {
                    ui->treeWidget->setCurrentItem(item,1,QItemSelectionModel::Deselect);
                }
                return true;
            }
            return false;
        }
    }
    return QWidget::eventFilter(obj,e);
}

void DlgPropertyLink::onItemSearch() {
    itemSearch(ui->searchBox->text(), true);
}

void DlgPropertyLink::keyPressEvent(QKeyEvent *ev)
{
    if(ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return) {
        if(ui->searchBox->hasFocus())
            return;
    }
    QWidget::keyPressEvent(ev);
}

void DlgPropertyLink::itemSearch(const QString &text, bool select) {
    if(searchItem)
        searchItem->setBackground(0, bgBrush);

    auto owner = objProp.getObject();
    if(!owner)
        return;

    std::string txt(text.toUtf8().constData());
    try {
        if(txt.empty())
            return;
        if(txt.find("<<") == std::string::npos) {
            auto pos = txt.find('.');
            if(pos==std::string::npos)
                txt += '.';
            else if(pos!=txt.size()-1) {
                txt.insert(pos+1,"<<");
                if(txt.back()!='.')
                    txt += '.';
                txt += ">>.";
            }
        }else if(txt.back() != '.')
            txt += '.';
        txt += "_self";
        auto path = App::ObjectIdentifier::parse(owner,txt);
        if(path.getPropertyName() != "_self")
            return;

        App::DocumentObject *obj = path.getDocumentObject();
        if(!obj) 
            return;

        bool found;
        const char *subname = path.getSubObjectName().c_str();
        QTreeWidgetItem *item = findItem(obj, subname, &found);
        if(!item)
            return;

        if(select) {
            if(!found)
                return;
            std::string s;
            auto res = resolveContext(selContext, s, App::SubObjectT(obj,subname));
            if(res.first)
                Gui::Selection().addSelection(res.first->getDocument()->getName(),
                        res.first->getNameInDocument(),res.second);
        }else{
            std::string s;
            auto res = resolveContext(selContext, s, App::SubObjectT(obj,subname));
            if(res.first)
                Selection().setPreselect(res.first->getDocument()->getName(),
                    res.first->getNameInDocument(), res.second, 0,0,0,2);
            searchItem = item;
            ui->treeWidget->scrollToItem(searchItem);
            bgBrush = searchItem->background(0);
            searchItem->setBackground(0, QColor(255, 255, 0, 100));
        }
    } catch(...)
    {
    }
}

QTreeWidgetItem *DlgPropertyLink::createItem(
        App::DocumentObject *obj, QTreeWidgetItem *parent)
{
    if(!obj || !obj->getNameInDocument())
        return 0;

    if(inList.count(obj))
        return 0;

    auto vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(obj));
    if(!vp) 
        return 0;

    QTreeWidgetItem* item;
    if(parent)
        item = new QTreeWidgetItem(parent);
    else
        item = new QTreeWidgetItem(ui->treeWidget);
    item->setIcon(0, vp->getIcon());
    QString label = QString::fromUtf8((obj)->Label.getValue());
    item->setText(0, label);
    item->setData(0, Qt::UserRole+4, label);
    item->setData(0, Qt::UserRole, QByteArray(obj->getNameInDocument()));
    item->setData(0, Qt::UserRole+1, QByteArray(obj->getDocument()->getName()));

    if(allowSubObject || (flags & AllowSubElement)) {
        if(allowSubObject)
            item->setChildIndicatorPolicy(obj->getLinkedObject(true)->getOutList().size()?
                    QTreeWidgetItem::ShowIndicator:QTreeWidgetItem::DontShowIndicator);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }

    const char *typeName = obj->getTypeId().getName();
    QByteArray typeData = QByteArray::fromRawData(typeName, strlen(typeName)+1);
    item->setData(0, Qt::UserRole+2, typeData);

    QByteArray proxyType;
    auto prop = Base::freecad_dynamic_cast<App::PropertyPythonObject>(
            obj->getPropertyByName("Proxy"));
    if(prop) {
        Base::PyGILStateLocker lock;
        Py::Object proxy = prop->getValue();
        if(!proxy.isNone() && !proxy.isString()) {
            const char *name = 0;
            if (proxy.hasAttr("__class__")) 
                proxyType = QByteArray(proxy.getAttr("__class__").as_string().c_str());
            else {
                name = proxy.ptr()->ob_type->tp_name;
                proxyType = QByteArray::fromRawData(name, strlen(name)+1);
            }
            auto it = typeItems.find(proxyType);
            if(it != typeItems.end())
                proxyType = it->first;
            else if (name)
                proxyType = QByteArray(name, proxyType.size());
        }
    }
    item->setData(0, Qt::UserRole+3, proxyType);

    filterItem(item);
    return item;
}

QTreeWidgetItem *DlgPropertyLink::createTypeItem(Base::Type type) {
    if(type.isBad())
        return 0;

    QTreeWidgetItem *item = 0;
    if(!type.isBad() && type!=App::DocumentObject::getClassTypeId()) {
        Base::Type parentType = type.getParent();
        if(!parentType.isBad()) {
            const char *name = parentType.getName();
            auto typeData = QByteArray::fromRawData(name,strlen(name)+1);
            auto &typeItem = typeItems[typeData];
            if(!typeItem) {
                typeItem = createTypeItem(parentType);
                typeItem->setData(0, Qt::UserRole, typeData);
            }
            item = typeItem;
        }
    }

    if(!item)
        item = new QTreeWidgetItem(ui->typeTree);
    else
        item = new QTreeWidgetItem(item);
    item->setExpanded(true);
    item->setText(0, QString::fromLatin1(type.getName()));
    if(type == App::DocumentObject::getClassTypeId())
        item->setFlags(Qt::ItemIsEnabled);
    return item;
}

bool DlgPropertyLink::filterType(QTreeWidgetItem *item) {
    auto proxyType = item->data(0, Qt::UserRole+3).toByteArray();
    QTreeWidgetItem *proxyItem = 0;
    if(proxyType.size()) {
        auto &pitem = typeItems[proxyType];
        if(!pitem) {
            pitem = new QTreeWidgetItem(ui->typeTree);
            pitem->setText(0,QString::fromLatin1(proxyType));
            pitem->setIcon(0,item->icon(0));
            pitem->setData(0,Qt::UserRole,proxyType);
        }
        proxyItem = pitem;
    }

    auto typeData = item->data(0, Qt::UserRole+2).toByteArray();
    Base::Type type = Base::Type::fromName(typeData.constData());
    if(type.isBad())
        return false;

    QTreeWidgetItem *&typeItem = typeItems[typeData];
    if(!typeItem)  {
        typeItem = createTypeItem(type);
        typeItem->setData(0, Qt::UserRole, typeData);
    }

    if(!proxyType.size()) {
        QIcon icon = typeItem->icon(0);
        if(icon.isNull())
            typeItem->setIcon(0, item->icon(0));
    }

    if(!ui->checkObjectType->isChecked() || selectedTypes.empty())
        return false;

    if(proxyItem && selectedTypes.count(proxyType))
        return false;

    for(auto t=type; !t.isBad() && t!=App::DocumentObject::getClassTypeId(); t=t.getParent()) {
        const char *name = t.getName();
        if(selectedTypes.count(QByteArray::fromRawData(name, strlen(name)+1)))
            return false;
    }

    return true;
}

void DlgPropertyLink::onItemExpanded(QTreeWidgetItem * item) {
    if(item->childCount()) 
        return;

    const char *docName = item->data(0, Qt::UserRole+1).toByteArray().constData();
    auto doc = App::GetApplication().getDocument(docName);
    if(!doc)
        return;

    const char *objName = item->data(0, Qt::UserRole).toByteArray().constData();
    if(!objName[0]) {
        for(auto obj : doc->getObjects()) {
            auto newItem = createItem(obj,item);
            if(newItem)
                itemMap[obj] = newItem;
        }
    } else if(allowSubObject) {
        auto obj = doc->getObject(objName);
        if(!obj) return;
        std::set<App::DocumentObject*> childSet;
        std::string sub;
        for(auto child : obj->getLinkedObject(true)->getOutList()) {
            if(!childSet.insert(child).second)
                continue;
            sub = child->getNameInDocument();
            sub += ".";
            if(obj->getSubObject(sub.c_str()))
                createItem(child,item);
        }
    }
}

void DlgPropertyLink::on_checkObjectType_toggled(bool on)
{
    ui->typeTree->setVisible(on);
    filterObjects();
}

void DlgPropertyLink::on_typeTree_itemSelectionChanged() {

    selectedTypes.clear();
    for(auto item : ui->typeTree->selectedItems())
        selectedTypes.insert(item->data(0, Qt::UserRole).toByteArray());

    if(ui->checkObjectType->isChecked())
        filterObjects();
}

void DlgPropertyLink::on_searchBox_textChanged(const QString& text)
{
    itemSearch(text,false);
}

///////////////////////////////////////////////////////////////////////////////

PropertyLinkEditor::PropertyLinkEditor(QWidget *parent)
    :QDialog(parent), SelectionObserver(false)
{
    proxy = new DlgPropertyLink(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(proxy);
    connect(proxy, SIGNAL(accepted()), this, SLOT(accept()));
    connect(proxy, SIGNAL(rejected()), this, SLOT(reject()));
}

PropertyLinkEditor::~PropertyLinkEditor()
{
    proxy->detachObserver(this);
}

void PropertyLinkEditor::showEvent(QShowEvent *ev)
{
    proxy->attachObserver(this);
    QDialog::showEvent(ev);
}

void PropertyLinkEditor::hideEvent(QHideEvent *ev)
{
    proxy->detachObserver(this);
    QDialog::hideEvent(ev);
}

void PropertyLinkEditor::closeEvent(QCloseEvent *ev)
{
    proxy->detachObserver(this);
    QDialog::closeEvent(ev);
}

void PropertyLinkEditor::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    proxy->selectionChanged(msg);
}

#include "moc_DlgPropertyLink.cpp"
