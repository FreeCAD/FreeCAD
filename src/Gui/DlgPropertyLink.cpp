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
# include <QStyledItemDelegate>
# include <QTreeWidgetItem>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyPythonObject.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>

#include "DlgPropertyLink.h"
#include "ui_DlgPropertyLink.h"
#include "Application.h"
#include "Document.h"
#include "BitmapFactory.h"
#include "PropertyView.h"
#include "Selection.h"
#include "Tree.h"
#include "TreeParams.h"
#include "View3DInventor.h"
#include "ViewProviderDocumentObject.h"


using namespace Gui::Dialog;

class ItemDelegate: public QStyledItemDelegate {
public:
    explicit ItemDelegate(QObject* parent=nullptr): QStyledItemDelegate(parent) {}

    QWidget* createEditor(QWidget *parent,
            const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if(index.column() != 1)
            return nullptr;
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
};

/* TRANSLATOR Gui::Dialog::DlgPropertyLink */

DlgPropertyLink::DlgPropertyLink(QWidget* parent)
  : QDialog(parent), SelectionObserver(false, ResolveMode::NoResolve)
  , ui(new Ui_DlgPropertyLink)
{
    ui->setupUi(this);
    connect(ui->checkObjectType, &QCheckBox::toggled,
            this, &DlgPropertyLink::onObjectTypeToggled);
    connect(ui->typeTree, &QTreeWidget::itemSelectionChanged,
            this, &DlgPropertyLink::onTypeTreeItemSelectionChanged);
    connect(ui->searchBox, &ExpressionLineEdit::textChanged,
            this, &DlgPropertyLink::onSearchBoxTextChanged);

    ui->typeTree->hide();
    ui->searchBox->installEventFilter(this);
    ui->searchBox->setNoProperty(true);
    ui->searchBox->setExactMatch(Gui::ExpressionParameter::instance()->isExactMatch());

    timer = new QTimer(this);
    timer->setSingleShot(true);

    connect(timer, &QTimer::timeout, this, &DlgPropertyLink::onTimer);

    ui->treeWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->treeWidget->setItemDelegate(new ItemDelegate(this));
    ui->treeWidget->setMouseTracking(true);
    connect(ui->treeWidget, &QTreeWidget::itemEntered,
            this, &DlgPropertyLink::onItemEntered);

    connect(ui->treeWidget, &QTreeWidget::itemExpanded,
            this, &DlgPropertyLink::onItemExpanded);

    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &DlgPropertyLink::onItemSelectionChanged);

    connect(ui->searchBox, &QLineEdit::returnPressed, this, &DlgPropertyLink::onItemSearch);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &DlgPropertyLink::onClicked);

    refreshButton = ui->buttonBox->addButton(tr("Reset"), QDialogButtonBox::ActionRole);
    resetButton = ui->buttonBox->addButton(tr("Clear"), QDialogButtonBox::ResetRole);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgPropertyLink::~DlgPropertyLink()
{
    detachObserver();

    // no need to delete child widgets, Qt does it all for us
    delete ui;
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
            res.push_back(App::SubObjectT(obj,nullptr));
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

static inline bool isLinkSub(const QList<App::SubObjectT>& links)
{
    for(const auto &link : links) {
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

QString DlgPropertyLink::formatLinks(App::Document *ownerDoc, QList<App::SubObjectT> links)
{
    if(!ownerDoc || links.empty())
        return {};

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
        return QString::fromLatin1("%1 [%2%3]").arg(formatObject(ownerDoc,obj,nullptr),
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

void DlgPropertyLink::init(const App::DocumentObjectT &prop, bool tryFilter) {
    ui->treeWidget->blockSignals(true);
    ui->treeWidget->clear();
    ui->treeWidget->blockSignals(false);

    ui->typeTree->blockSignals(true);
    ui->typeTree->clear();
    ui->typeTree->blockSignals(false);

    oldLinks.clear();
    docItems.clear();
    typeItems.clear();
    itemMap.clear();
    inList.clear();
    selectedTypes.clear();
    currentObj = nullptr;
    searchItem = nullptr;
    subSelections.clear();
    selections.clear();

    objProp  = prop;
    auto owner = objProp.getObject();
    if(!owner || !owner->getNameInDocument())
        return;

    ui->searchBox->setDocumentObject(owner);

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

    if(App::PropertyXLink::supportXLink(propLink)) {
        allowSubObject = true;
        docs = App::GetApplication().getDocuments();
    } else
        docs.push_back(owner->getDocument());

    bool isLinkList = false;
    if (propLink->isDerivedFrom(App::PropertyXLinkList::getClassTypeId())
            || propLink->isDerivedFrom(App::PropertyLinkList::getClassTypeId()))
    {
        isLinkList = true;
        allowSubObject = false;
    }

    if(singleSelect) {
        singleParent = true;
        ui->treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    } else {
        ui->treeWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    }

    ui->checkSubObject->setVisible(allowSubObject);

    if(!allowSubObject) {
        ui->treeWidget->setColumnCount(1);
    } else {
        ui->treeWidget->setColumnCount(2);

        // make sure to show a horizontal scrollbar if needed
        ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
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

    if(allowSubObject) {
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

    if(oldLinks.isEmpty())
        return;

    // Try to select items corresponding to the current links inside the
    // property
    ui->treeWidget->blockSignals(true);
    for(auto &link : oldLinks) {
        onSelectionChanged(Gui::SelectionChanges(SelectionChanges::AddSelection,
                                                 link.getDocumentName(),
                                                 link.getObjectName(),
                                                 link.getSubName()));
    }
    ui->treeWidget->blockSignals(false);

    // For link list type property, try to auto filter type
    if(tryFilter && isLinkList) {
        Base::Type objType;
        for(const auto& link : qAsConst(oldLinks)) {
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
        ui->treeWidget->blockSignals(true);
        ui->treeWidget->selectionModel()->clearSelection();
        for(auto item : subSelections)
            item->setText(1, QString());
        ui->treeWidget->blockSignals(false);
        subSelections.clear();
        Gui::Selection().clearSelection();
    } else if (button == refreshButton) {
        init(objProp);
    }
}

void DlgPropertyLink::hideEvent(QHideEvent *ev) {
    detachObserver();
    QDialog::hideEvent(ev);
}

void DlgPropertyLink::closeEvent(QCloseEvent *ev) {
    detachObserver();
    QDialog::closeEvent(ev);
}

void DlgPropertyLink::attachObserver() {
    if(isSelectionAttached())
        return;

    Gui::Selection().selStackPush();
    attachSelection();

    if(!parentView) {
        for(auto p=parent(); p; p=p->parent()) {
            auto view = qobject_cast<Gui::PropertyView*>(p);
            if(view) {
                parentView = view;
                for(auto &sel : Gui::Selection().getCompleteSelection(ResolveMode::NoResolve))
                    savedSelections.emplace_back(sel.DocName, sel.FeatName, sel.SubName);
                break;
            }
        }
    }
    auto view = qobject_cast<Gui::PropertyView*>(parentView.data());
    if(view)
        view->blockSelection(true);
}

void DlgPropertyLink::showEvent(QShowEvent *ev) {
    attachObserver();
    QDialog::showEvent(ev);
}

void DlgPropertyLink::onItemEntered(QTreeWidgetItem *) {
    int timeout = Gui::TreeParams::getPreSelectionDelay()/2;
    if(timeout < 0)
        timeout = 1;
    timer->start(timeout);
    Gui::Selection().rmvPreselect();
}

void DlgPropertyLink::leaveEvent(QEvent *ev) {
    Gui::Selection().rmvPreselect();
    QDialog::leaveEvent(ev);
}

void DlgPropertyLink::detachObserver() {
    if(isSelectionAttached())
        detachSelection();

    auto view = qobject_cast<Gui::PropertyView*>(parentView.data());
    if(view && !savedSelections.empty()) {
        try {
            Gui::Selection().clearSelection();
        }
        catch (Py::Exception& e) {
            e.clear();
        }
        for(auto &sel : savedSelections) {
            if(sel.getSubObject())
                Gui::Selection().addSelection(sel.getDocumentName().c_str(),
                                              sel.getObjectName().c_str(),
                                              sel.getSubName().c_str());
        }
        savedSelections.clear();
    }
    if(view)
        view->blockSelection(false);

    parentView = nullptr;
}

void DlgPropertyLink::onItemSelectionChanged()
{
    auto newSelections = ui->treeWidget->selectedItems();

    if(newSelections.isEmpty() || selections.contains(newSelections.back())) {
        selections = newSelections;
        if(newSelections.isEmpty())
            currentObj = nullptr;
        return;
    }

    selections = newSelections;

    auto sobjs = getLinkFromItem(newSelections.back());
    App::DocumentObject *obj = !sobjs.empty()?sobjs.front().getObject():nullptr;
    if(!obj) {
        Gui::Selection().clearSelection();
        return;
    }

    bool focus = false;
    // Do auto view switch if tree view does not do it
    if(!TreeParams::getSyncView()) {
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

    // Sync 3d view selection. To give a better visual feedback, we
    // only keep the latest selection.
    bool blocked = blockSelection(true);
    Gui::Selection().clearSelection();
    for(auto &sobj : sobjs)
        Gui::Selection().addSelection(sobj.getDocumentName().c_str(),
                                      sobj.getObjectName().c_str(),
                                      sobj.getSubName().c_str());
    blockSelection(blocked);

    // Enforce single parent
    if(singleParent && currentObj && currentObj!=obj) {
        ui->treeWidget->blockSignals(true);
        const auto items = ui->treeWidget->selectedItems();
        for(auto item : items) {
            if(item != selections.back())
                item->setSelected(false);
        }
        auto last = selections.back();
        selections.clear();
        selections.append(last);
        ui->treeWidget->blockSignals(false);
    }
    currentObj = obj;

    if(focus) {
        // FIXME: does not work, why?
        ui->treeWidget->setFocus();
    }
}

QTreeWidgetItem *DlgPropertyLink::findItem(
        App::DocumentObject *obj, const char *subname, bool *pfound)
{
    if(pfound)
        *pfound = false;

    if(!obj || !obj->getNameInDocument())
        return nullptr;

    std::vector<App::DocumentObject *> sobjs;
    if(subname && subname[0]) {
        if(!allowSubObject) {
            obj = obj->getSubObject(subname);
            if(!obj)
                return nullptr;
        } else {
            sobjs = obj->getSubObjectList(subname);
        }
    }

    auto itDoc = docItems.find(obj->getDocument());
    if(itDoc == docItems.end())
        return nullptr;
    onItemExpanded(itDoc->second);

    auto it = itemMap.find(obj);
    if(it == itemMap.end() || it->second->isHidden())
        return nullptr;

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

void DlgPropertyLink::onSelectionChanged(const Gui::SelectionChanges& msg)
{
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

    auto item = findItem(selObj, msg.pSubName, &found);
    if(!item || !found)
        return;

    if(!item->isSelected()) {
        ui->treeWidget->blockSignals(true);
        if(singleSelect || (singleParent && currentObj && currentObj!=selObj))
            ui->treeWidget->selectionModel()->clearSelection();
        currentObj = selObj;
        item->setSelected(true);
        selections.append(item);
        ui->treeWidget->blockSignals(false);
    }

    ui->treeWidget->scrollToItem(item);
    if(allowSubObject) {
        QString element = QString::fromLatin1(msg.Object.getOldElementName().c_str());
        if(element.size()) {
            QStringList list;
            QString text = item->text(1);
            if(text.size())
                list = text.split(QLatin1Char(','));
            if(list.indexOf(element)<0) {
                list << element;
                item->setText(1, list.join(QLatin1String(",")));
                subSelections.insert(item);
            }
        } else if (subSelections.erase(item))
            item->setText(1, QString());
    }
}

void DlgPropertyLink::accept()
{
    QDialog::accept();
}

static QTreeWidgetItem *_getLinkFromItem(std::ostringstream &ss, QTreeWidgetItem *item, const char *objName) {
    auto parent = item->parent();
    assert(parent);
    QByteArray nextName = parent->data(0, Qt::UserRole).toByteArray();
    if (nextName.isEmpty())
        return item;

    item = _getLinkFromItem(ss, parent, nextName);
    ss << objName << '.';
    return item;
}

QList<App::SubObjectT>
DlgPropertyLink::getLinkFromItem(QTreeWidgetItem *item, bool needSubName) const
{
    QList<App::SubObjectT> res;

    auto parent = item->parent();
    if(!parent)
        return res;

    std::ostringstream ss;
    auto parentItem = _getLinkFromItem(ss, item,
            item->data(0,Qt::UserRole).toByteArray().constData());

    App::SubObjectT sobj(parentItem->data(0, Qt::UserRole+1).toByteArray().constData(),
                         parentItem->data(0, Qt::UserRole).toByteArray().constData(),
                         ss.str().c_str());

    QString elements;
    if(needSubName && allowSubObject)
        elements = item->text(1);

    if(elements.isEmpty()) {
        res.append(App::SubObjectT());
        res.last() = std::move(sobj);
        return res;
    }

    const auto split = elements.split(QLatin1Char(','));
    for(const QString &element : split) {
        res.append(App::SubObjectT());
        res.last() = App::SubObjectT(sobj.getDocumentName().c_str(),
                                     sobj.getObjectName().c_str(),
                                     (sobj.getSubName() + element.toLatin1().constData()).c_str());
    }
    return res;
}

void DlgPropertyLink::onTimer() {
    auto item = ui->treeWidget->itemAt(
            ui->treeWidget->viewport()->mapFromGlobal(QCursor::pos()));
    if(!item)
        return;
    auto sobjs = getLinkFromItem(item);
    if(sobjs.isEmpty())
        return;
    const auto &sobj = sobjs.front();
    Gui::Selection().setPreselect(sobj.getDocumentName().c_str(),
                                  sobj.getObjectName().c_str(),
                                  sobj.getSubName().c_str(),
                                  0, 0, 0, Gui::SelectionChanges::MsgSource::TreeView);
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

QString DlgPropertyLink::linksToPython(const QList<App::SubObjectT>& links) {
    if(links.isEmpty())
        return QLatin1String("None");

    if(links.size() == 1)
        return QString::fromLatin1(links.front().getSubObjectPython(false).c_str());

    std::ostringstream ss;

    if(isLinkSub(links)) {
        ss << '(' << links.front().getObjectPython() << ", [";
        for(const auto& link : links) {
            const auto &sub = link.getSubName();
            if(!sub.empty())
                ss << "u'" << Base::Tools::escapedUnicodeFromUtf8(sub.c_str()) << "',";
        }
        ss << "])";
    } else {
        ss << '[';
        for(const auto& link : links)
            ss << link.getSubObjectPython(false) << ',';
        ss << ']';
    }

    return QString::fromLatin1(ss.str().c_str());
}

void DlgPropertyLink::filterObjects()
{
    for(int i=0, count=ui->treeWidget->topLevelItemCount(); i<count; ++i) {
        auto item = ui->treeWidget->topLevelItem(i);
        for(int j=0, c=item->childCount(); j<c; ++j)
            filterItem(item->child(j));
    }
}

void DlgPropertyLink::filterItem(QTreeWidgetItem *item) {
    if(filterType(item)) {
        item->setHidden(true);
        return;
    }
    item->setHidden(false);
    for(int i=0, count=item->childCount(); i<count; ++i)
        filterItem(item->child(i));
}

bool DlgPropertyLink::eventFilter(QObject *obj, QEvent *e) {
    if(obj == ui->searchBox
            && e->type() == QEvent::KeyPress
            && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape)
    {
        ui->searchBox->setText(QString());
        return true;
    }
    return QDialog::eventFilter(obj,e);
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
    QDialog::keyPressEvent(ev);
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
            Gui::Selection().addSelection(obj->getDocument()->getName(),
                    obj->getNameInDocument(),subname);
        }else{
            Selection().setPreselect(obj->getDocument()->getName(),
                    obj->getNameInDocument(), subname, 0, 0, 0,
                    Gui::SelectionChanges::MsgSource::TreeView);
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
        return nullptr;

    if(inList.find(obj)!=inList.end())
        return nullptr;

    auto vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(obj));
    if(!vp)
        return nullptr;

    QTreeWidgetItem* item;
    if(parent)
        item = new QTreeWidgetItem(parent);
    else
        item = new QTreeWidgetItem(ui->treeWidget);
    item->setIcon(0, vp->getIcon());
    item->setText(0, QString::fromUtf8((obj)->Label.getValue()));
    item->setData(0, Qt::UserRole, QByteArray(obj->getNameInDocument()));
    item->setData(0, Qt::UserRole+1, QByteArray(obj->getDocument()->getName()));

    if(allowSubObject) {
        item->setChildIndicatorPolicy(!obj->getLinkedObject(true)->getOutList().empty()?
                QTreeWidgetItem::ShowIndicator:QTreeWidgetItem::DontShowIndicator);
        item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
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
            const char *name = nullptr;
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
        return nullptr;

    QTreeWidgetItem *item = nullptr;
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
    QTreeWidgetItem *proxyItem = nullptr;
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

    QByteArray docName = item->data(0, Qt::UserRole+1).toByteArray();
    auto doc = App::GetApplication().getDocument(docName);
    if (!doc)
        return;

    QByteArray objName = item->data(0, Qt::UserRole).toByteArray();
    if (objName.isEmpty()) {
        for(auto obj : doc->getObjects()) {
            auto newItem = createItem(obj,item);
            if(newItem)
                itemMap[obj] = newItem;
        }
    } else if(allowSubObject) {
        auto obj = doc->getObject(objName);
        if(!obj)
            return;
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

void DlgPropertyLink::onObjectTypeToggled(bool on)
{
    ui->typeTree->setVisible(on);
    filterObjects();
}

void DlgPropertyLink::onTypeTreeItemSelectionChanged() {

    selectedTypes.clear();
    const auto items = ui->typeTree->selectedItems();
    for(auto item : items)
        selectedTypes.insert(item->data(0, Qt::UserRole).toByteArray());

    if(ui->checkObjectType->isChecked())
        filterObjects();
}

void DlgPropertyLink::onSearchBoxTextChanged(const QString& text)
{
    itemSearch(text,false);
}

#include "moc_DlgPropertyLink.cpp"
