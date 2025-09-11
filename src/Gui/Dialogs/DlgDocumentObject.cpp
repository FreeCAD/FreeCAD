// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2014 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                 *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <algorithm>
#include <sstream>
#include <QStyledItemDelegate>
#include <QTreeWidgetItem>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/ObjectIdentifier.h>
#include <App/PropertyPythonObject.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>

#include "Dialogs/DlgDocumentObject.h"
#include "ui_DlgDocumentObject.h"
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

class ItemDelegate: public QStyledItemDelegate
{
public:
    explicit ItemDelegate(QObject* parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    QWidget* createEditor(QWidget* parent,
                          const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override
    {
        if (index.column() != 1) {
            return nullptr;
        }
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
};

/* TRANSLATOR Gui::Dialog::DlgDocumentObject */

DlgDocumentObject::DlgDocumentObject(QWidget* parent)
    : QDialog(parent)
    , SelectionObserver(false, ResolveMode::NoResolve)
    , ui(new Ui_DlgDocumentObject)
{
    // clang-format off
    ui->setupUi(this);
    connect(ui->checkObjectType, &QCheckBox::toggled,
            this, &DlgDocumentObject::onObjectTypeToggled);
    connect(ui->typeTree, &QTreeWidget::itemSelectionChanged,
            this, &DlgDocumentObject::onTypeTreeItemSelectionChanged);
    connect(ui->searchBox, &ExpressionLineEdit::textChanged,
            this, &DlgDocumentObject::onSearchBoxTextChanged);

    ui->typeTree->hide();
    ui->searchBox->installEventFilter(this);
    ui->searchBox->setNoProperty(true);
    ui->searchBox->setExactMatch(Gui::ExpressionParameter::instance()->isExactMatch());

    timer = new QTimer(this);
    timer->setSingleShot(true);

    connect(timer, &QTimer::timeout, this, &DlgDocumentObject::onTimer);

    ui->treeWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->treeWidget->setItemDelegate(new ItemDelegate(this));
    ui->treeWidget->setMouseTracking(true);
    connect(ui->treeWidget, &QTreeWidget::itemEntered,
            this, &DlgDocumentObject::onItemEntered);

    connect(ui->treeWidget, &QTreeWidget::itemExpanded,
            this, &DlgDocumentObject::onItemExpanded);

    connect(ui->treeWidget, &QTreeWidget::itemSelectionChanged, this, &DlgDocumentObject::onItemSelectionChanged);

    connect(ui->searchBox, &QLineEdit::returnPressed, this, &DlgDocumentObject::onItemSearch);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &DlgDocumentObject::onClicked);

    refreshButton = ui->buttonBox->addButton(tr("Reset"), QDialogButtonBox::ActionRole);
    resetButton = ui->buttonBox->addButton(tr("Clear"), QDialogButtonBox::ResetRole);
    // clang-format on
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgDocumentObject::~DlgDocumentObject()
{
    detachObserver();

    // no need to delete child widgets, Qt does it all for us
    delete ui;
}


void DlgDocumentObject::init(App::DocumentObject* owner,
                             bool singleSelect, bool filterOwner,
                             bool filterTypeOwner, bool expandTypeOwner)
{
    ui->treeWidget->blockSignals(true);
    ui->treeWidget->clear();
    ui->treeWidget->blockSignals(false);

    ui->typeTree->blockSignals(true);
    ui->typeTree->clear();
    ui->typeTree->blockSignals(false);

    docItems.clear();
    typeItems.clear();
    itemMap.clear();
    inList.clear();
    selectedTypes.clear();
    currentObj = nullptr;
    searchItem = nullptr;
    subSelections.clear();
    selections.clear();

    this->owner = owner;
    if (!owner || !owner->isAttachedToDocument()) {
        return;
    }
    this->filterOwner = filterOwner;

    ui->searchBox->setDocumentObject(owner);

    std::vector<App::Document*> docs;

    this->singleSelect = singleSelect;
    allowSubObject = false;

    docs = App::GetApplication().getDocuments();

    if (singleSelect) {
        singleParent = true;
        ui->treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    }
    else {
        ui->treeWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    }

    ui->checkSubObject->setVisible(allowSubObject);

    if (!allowSubObject) {
        ui->treeWidget->setColumnCount(1);
    }
    else {
        ui->treeWidget->setColumnCount(2);

        // make sure to show a horizontal scrollbar if needed
        ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    }

    std::set<App::Document*> expandDocs;

    if (expandTypeOwner) {
        Base::Type objType = owner->getTypeId();
        for (auto& doc : docs) {
            for (auto& obj : doc->getObjects()) {
                if (obj->getTypeId() == objType) {
                    expandDocs.insert(doc);
                }
            }
        }
    }
    else {
        expandDocs.insert(owner->getDocument());
    }

    QPixmap docIcon(Gui::BitmapFactory().pixmap("Document"));
    for (auto d : docs) {
        auto item = new QTreeWidgetItem(ui->treeWidget);
        item->setIcon(0, docIcon);
        item->setText(0, QString::fromUtf8(d->Label.getValue()));
        item->setData(0, ObjectNameRole, QByteArray(""));
        item->setData(0, DocNameRole, QByteArray(d->getName()));
        item->setFlags(Qt::ItemIsEnabled);
        item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        if (expandDocs.count(d)) {
            item->setExpanded(true);
        }
        docItems[d] = item;
    }

    if (filterTypeOwner) {
        Base::Type objType = owner->getTypeId();

        if (!objType.isBad()) {
            const char* name = objType.getName();
            auto it = typeItems.find(QByteArray::fromRawData(name, strlen(name) + 1));
            if (it != typeItems.end()) {
                it->second->setSelected(true);
            }
            ui->checkObjectType->setChecked(true);
        }
    }
}

void DlgDocumentObject::onClicked(QAbstractButton* button)
{
    if (button == resetButton) {
        ui->treeWidget->blockSignals(true);
        ui->treeWidget->selectionModel()->clearSelection();
        for (auto item : subSelections) {
            item->setText(1, QString());
        }
        ui->treeWidget->blockSignals(false);
        subSelections.clear();
        Gui::Selection().clearSelection();
    }
    else if (button == refreshButton) {
        init(owner);
    }
}

void DlgDocumentObject::hideEvent(QHideEvent* ev)
{
    detachObserver();
    QDialog::hideEvent(ev);
}

void DlgDocumentObject::closeEvent(QCloseEvent* ev)
{
    detachObserver();
    QDialog::closeEvent(ev);
}

void DlgDocumentObject::attachObserver()
{
    if (isSelectionAttached()) {
        return;
    }

    Gui::Selection().selStackPush();
    attachSelection();

    if (!parentView) {
        for (auto p = parent(); p; p = p->parent()) {
            auto view = qobject_cast<Gui::PropertyView*>(p);
            if (view) {
                parentView = view;
                for (auto& sel : Gui::Selection().getCompleteSelection(ResolveMode::NoResolve)) {
                    savedSelections.emplace_back(sel.DocName, sel.FeatName, sel.SubName);
                }
                break;
            }
        }
    }
    auto view = qobject_cast<Gui::PropertyView*>(parentView.data());
    if (view) {
        view->blockSelection(true);
    }
}

void DlgDocumentObject::showEvent(QShowEvent* ev)
{
    attachObserver();
    QDialog::showEvent(ev);
}

void DlgDocumentObject::onItemEntered(QTreeWidgetItem*)
{
    int timeout = Gui::TreeParams::getPreSelectionDelay() / 2;
    if (timeout < 0) {
        timeout = 1;
    }
    timer->start(timeout);
    Gui::Selection().rmvPreselect();
}

void DlgDocumentObject::leaveEvent(QEvent* ev)
{
    Gui::Selection().rmvPreselect();
    QDialog::leaveEvent(ev);
}

void DlgDocumentObject::detachObserver()
{
    if (isSelectionAttached()) {
        detachSelection();
    }

    auto view = qobject_cast<Gui::PropertyView*>(parentView.data());
    if (view && !savedSelections.empty()) {
        try {
            Gui::Selection().clearSelection();
        }
        catch (Py::Exception& e) {
            e.clear();
        }
        for (auto& sel : savedSelections) {
            if (sel.getSubObject()) {
                Gui::Selection().addSelection(sel.getDocumentName().c_str(),
                                              sel.getObjectName().c_str(),
                                              sel.getSubName().c_str());
            }
        }
        savedSelections.clear();
    }
    if (view) {
        view->blockSelection(false);
    }

    parentView = nullptr;
}

void DlgDocumentObject::onItemSelectionChanged()
{
    auto newSelections = ui->treeWidget->selectedItems();

    if (newSelections.isEmpty() || selections.contains(newSelections.back())) {
        selections = newSelections;
        if (newSelections.isEmpty()) {
            currentObj = nullptr;
        }
        return;
    }

    selections = newSelections;

    auto sobjs = getSubObjectFromItem(newSelections.back());
    App::DocumentObject* obj = !sobjs.empty() ? sobjs.front().getObject() : nullptr;
    if (!obj) {
        Gui::Selection().clearSelection();
        return;
    }

    bool focus = false;
    // Do auto view switch if tree view does not do it
    if (!TreeParams::getSyncView()) {
        focus = ui->treeWidget->hasFocus();
        auto doc = Gui::Application::Instance->getDocument(sobjs.front().getDocumentName().c_str());
        if (doc) {
            auto vp = freecad_cast<Gui::ViewProviderDocumentObject*>(
                doc->getViewProvider(obj));
            if (vp) {
                // If the view provider uses a special window for rendering, switch to it
                MDIView* view = vp->getMDIView();
                if (view) {
                    doc->setActiveWindow(view);
                }
                else {
                    doc->setActiveView(vp, Gui::View3DInventor::getClassTypeId());
                }
            }
        }
    }

    // Sync 3d view selection. To give a better visual feedback, we
    // only keep the latest selection.
    bool blocked = blockSelection(true);
    Gui::Selection().clearSelection();
    for (auto& sobj : sobjs) {
        Gui::Selection().addSelection(sobj.getDocumentName().c_str(),
                                      sobj.getObjectName().c_str(),
                                      sobj.getSubName().c_str());
    }
    blockSelection(blocked);

    // Enforce single parent
    if (singleParent && currentObj && currentObj != obj) {
        ui->treeWidget->blockSignals(true);
        const auto items = ui->treeWidget->selectedItems();
        for (auto item : items) {
            if (item != selections.back()) {
                item->setSelected(false);
            }
        }
        auto last = selections.back();
        selections.clear();
        selections.append(last);
        ui->treeWidget->blockSignals(false);
    }
    currentObj = obj;

    if (focus) {
        // FIXME: does not work, why?
        ui->treeWidget->setFocus();
    }
}

QTreeWidgetItem*
DlgDocumentObject::findItem(App::DocumentObject* obj, const char* subname, bool* pfound)
{
    if (pfound) {
        *pfound = false;
    }

    if (!obj || !obj->isAttachedToDocument()) {
        return nullptr;
    }

    std::vector<App::DocumentObject*> sobjs;
    if (!Base::Tools::isNullOrEmpty(subname)) {
        if (!allowSubObject) {
            obj = obj->getSubObject(subname);
            if (!obj) {
                return nullptr;
            }
        }
        else {
            sobjs = obj->getSubObjectList(subname);
        }
    }

    auto itDoc = docItems.find(obj->getDocument());
    if (itDoc == docItems.end()) {
        return nullptr;
    }
    onItemExpanded(itDoc->second);

    auto it = itemMap.find(obj);
    if (it == itemMap.end() || it->second->isHidden()) {
        return nullptr;
    }

    if (!allowSubObject) {
        if (pfound) {
            *pfound = true;
        }
        return it->second;
    }

    QTreeWidgetItem* item = it->second;

    bool first = true;
    for (auto o : sobjs) {
        if (first) {
            first = false;
            continue;
        }
        onItemExpanded(item);
        bool found = false;
        for (int i = 0, count = item->childCount(); i < count; ++i) {
            auto child = item->child(i);
            if (strcmp(o->getNameInDocument(),
                       child->data(0, ObjectNameRole).toByteArray().constData())
                == 0) {
                item = child;
                found = true;
                break;
            }
        }
        if (!found) {
            return item;
        }
    }
    if (pfound) {
        *pfound = true;
    }
    return item;
}

void DlgDocumentObject::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (msg.Type != SelectionChanges::AddSelection) {
        return;
    }

    bool found = false;
    auto selObj = msg.Object.getObject();

    App::ElementNamePair elementName;
    const char* subname = msg.pSubName;
    if (!ui->checkSubObject->isChecked()) {
        selObj = App::GeoFeature::resolveElement(selObj, subname, elementName);
        if (!selObj) {
            return;
        }
        subname = elementName.oldName.c_str();
    }

    auto item = findItem(selObj, msg.pSubName, &found);
    if (!item) {
        return;
    }

    if (!item->isSelected()) {
        ui->treeWidget->blockSignals(true);
        if (singleSelect || (singleParent && currentObj && currentObj != selObj)) {
            ui->treeWidget->selectionModel()->clearSelection();
        }
        currentObj = selObj;
        item->setSelected(true);
        selections.append(item);
        ui->treeWidget->blockSignals(false);
    }

    ui->treeWidget->scrollToItem(item);
    if (allowSubObject) {
        QString element = QString::fromLatin1(msg.Object.getOldElementName().c_str());
        if (element.size()) {
            QStringList list;
            QString text = item->text(1);
            if (text.size()) {
                list = text.split(QLatin1Char(','));
            }
            if (list.indexOf(element) < 0) {
                list << element;
                item->setText(1, list.join(QLatin1String(",")));
                subSelections.insert(item);
            }
        }
        else if (subSelections.erase(item)) {
            item->setText(1, QString());
        }
    }
}

void DlgDocumentObject::accept()
{
    QDialog::accept();
}

static QTreeWidgetItem*
_getSubObjectFromItem(std::ostringstream& ss, QTreeWidgetItem* item, const char* objName)
{
    auto parent = item->parent();
    assert(parent);
    QByteArray nextName = parent->data(0, Gui::Dialog::DlgDocumentObject::ObjectNameRole).toByteArray();
    if (nextName.isEmpty()) {
        return item;
    }

    item = _getSubObjectFromItem(ss, parent, nextName);
    ss << objName << '.';
    return item;
}

QList<App::SubObjectT> DlgDocumentObject::getSubObjectFromItem(QTreeWidgetItem* item,
                                                               bool needSubName) const
{
    QList<App::SubObjectT> res;

    auto parent = item->parent();
    if (!parent) {
        return res;
    }

    std::ostringstream ss;
    auto parentItem =
        _getSubObjectFromItem(ss, item, item->data(0, ObjectNameRole).toByteArray().constData());

    App::SubObjectT sobj(parentItem->data(0, DocNameRole).toByteArray().constData(),
                         parentItem->data(0, ObjectNameRole).toByteArray().constData(),
                         ss.str().c_str());

    QString elements;
    if (needSubName && allowSubObject) {
        elements = item->text(1);
    }

    if (elements.isEmpty()) {
        res.append(App::SubObjectT());
        res.last() = std::move(sobj);
        return res;
    }

    const auto split = elements.split(QLatin1Char(','));
    for (const QString& element : split) {
        res.append(App::SubObjectT());
        res.last() = App::SubObjectT(sobj.getDocumentName().c_str(),
                                     sobj.getObjectName().c_str(),
                                     (sobj.getSubName() + element.toLatin1().constData()).c_str());
    }
    return res;
}

void DlgDocumentObject::onTimer()
{
    auto item = ui->treeWidget->itemAt(ui->treeWidget->viewport()->mapFromGlobal(QCursor::pos()));
    if (!item) {
        return;
    }
    auto sobjs = getSubObjectFromItem(item);
    if (sobjs.isEmpty()) {
        return;
    }
    const auto& sobj = sobjs.front();
    Gui::Selection().setPreselect(sobj.getDocumentName().c_str(),
                                  sobj.getObjectName().c_str(),
                                  sobj.getSubName().c_str(),
                                  0,
                                  0,
                                  0,
                                  Gui::SelectionChanges::MsgSource::TreeView);
}

QList<App::SubObjectT> DlgDocumentObject::currentSubObjects() const
{
    auto items = ui->treeWidget->selectedItems();
    QList<App::SubObjectT> res;
    for (auto item : items) {
        res.append(getSubObjectFromItem(item));
    }
    return res;
}

void DlgDocumentObject::filterObjects()
{
    for (int i = 0, count = ui->treeWidget->topLevelItemCount(); i < count; ++i) {
        auto item = ui->treeWidget->topLevelItem(i);
        for (int j = 0, c = item->childCount(); j < c; ++j) {
            filterItem(item->child(j));
        }
    }
}

void DlgDocumentObject::filterItem(QTreeWidgetItem* item)
{
    if (filterOwner &&
        strcmp(item->data(0, ObjectNameRole).toByteArray().constData(), owner->getNameInDocument()) == 0 &&
        strcmp(item->data(0, DocNameRole).toByteArray().constData(), owner->getDocument()->getName()) == 0) {
        item->setHidden(true);
        return;
    }
    if (filterType(item)) {
        item->setHidden(true);
        return;
    }
    item->setHidden(false);
    for (int i = 0, count = item->childCount(); i < count; ++i) {
        filterItem(item->child(i));
    }
}

bool DlgDocumentObject::eventFilter(QObject* obj, QEvent* e)
{
    if (obj == ui->searchBox && e->type() == QEvent::KeyPress
        && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
        ui->searchBox->setText(QString());
        return true;
    }
    return QDialog::eventFilter(obj, e);
}

void DlgDocumentObject::onItemSearch()
{
    itemSearch(ui->searchBox->text(), true);
}

void DlgDocumentObject::keyPressEvent(QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Enter || ev->key() == Qt::Key_Return) {
        if (ui->searchBox->hasFocus()) {
            return;
        }
    }
    QDialog::keyPressEvent(ev);
}

void DlgDocumentObject::itemSearch(const QString& text, bool select)
{
    if (searchItem) {
        searchItem->setBackground(0, bgBrush);
    }

    if (!owner) {
        return;
    }

    std::string txt(text.toUtf8().constData());
    try {
        if (txt.empty()) {
            return;
        }
        if (txt.find("<<") == std::string::npos) {
            auto pos = txt.find('.');
            if (pos == std::string::npos) {
                txt += '.';
            }
            else if (pos != txt.size() - 1) {
                txt.insert(pos + 1, "<<");
                if (txt.back() != '.') {
                    txt += '.';
                }
                txt += ">>.";
            }
        }
        else if (txt.back() != '.') {
            txt += '.';
        }
        txt += "_self";
        auto path = App::ObjectIdentifier::parse(owner, txt);
        if (path.getPropertyName() != "_self") {
            return;
        }

        App::DocumentObject* obj = path.getDocumentObject();
        if (!obj) {
            return;
        }

        bool found;
        const char* subname = path.getSubObjectName().c_str();
        QTreeWidgetItem* item = findItem(obj, subname, &found);
        if (!item) {
            return;
        }

        if (select) {
            if (!found) {
                return;
            }
            Gui::Selection().addSelection(obj->getDocument()->getName(),
                                          obj->getNameInDocument(),
                                          subname);
        }
        else {
            Selection().setPreselect(obj->getDocument()->getName(),
                                     obj->getNameInDocument(),
                                     subname,
                                     0,
                                     0,
                                     0,
                                     Gui::SelectionChanges::MsgSource::TreeView);
            searchItem = item;
            ui->treeWidget->scrollToItem(searchItem);
            bgBrush = searchItem->background(0);
            searchItem->setBackground(0, QColor(255, 255, 0, 100));
        }
    }
    catch (...) {
    }
}


QTreeWidgetItem* DlgDocumentObject::createItem(App::DocumentObject* obj, QTreeWidgetItem* parent)
{
    if (!obj || !obj->isAttachedToDocument()) {
        return nullptr;
    }

    if (inList.find(obj) != inList.end()) {
        return nullptr;
    }

    auto vp = freecad_cast<ViewProviderDocumentObject*>(
        Application::Instance->getViewProvider(obj));
    if (!vp) {
        return nullptr;
    }

    QTreeWidgetItem* item;
    if (parent) {
        item = new QTreeWidgetItem(parent);
    }
    else {
        item = new QTreeWidgetItem(ui->treeWidget);
    }
    item->setIcon(0, vp->getIcon());
    item->setText(0, QString::fromUtf8((obj)->Label.getValue()));
    item->setData(0, ObjectNameRole, QByteArray(obj->getNameInDocument()));
    item->setData(0, DocNameRole, QByteArray(obj->getDocument()->getName()));

    if (allowSubObject) {
        item->setChildIndicatorPolicy(!obj->getLinkedObject(true)->getOutList().empty()
                                          ? QTreeWidgetItem::ShowIndicator
                                          : QTreeWidgetItem::DontShowIndicator);
        item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    }

    const char* typeName = obj->getTypeId().getName();
    QByteArray typeData = QByteArray::fromRawData(typeName, strlen(typeName) + 1);
    item->setData(0, TypeNameRole, typeData);

    QByteArray proxyType;
    auto prop =
        freecad_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));
    if (prop) {
        Base::PyGILStateLocker lock;
        Py::Object proxy = prop->getValue();
        if (!proxy.isNone() && !proxy.isString()) {
            const char* name = nullptr;
            if (proxy.hasAttr("__class__")) {
                proxyType = QByteArray(proxy.getAttr("__class__").as_string().c_str());
            }
            else {
                name = proxy.ptr()->ob_type->tp_name;
                proxyType = QByteArray::fromRawData(name, strlen(name) + 1);
            }
            auto it = typeItems.find(proxyType);
            if (it != typeItems.end()) {
                proxyType = it->first;
            }
            else if (name) {
                proxyType = QByteArray(name, proxyType.size());
            }
        }
    }
    item->setData(0, ProxyTypeRole, proxyType);

    filterItem(item);
    return item;
}

QTreeWidgetItem* DlgDocumentObject::createTypeItem(Base::Type type)
{
    if (type.isBad()) {
        return nullptr;
    }

    QTreeWidgetItem* item = nullptr;
    if (!type.isBad() && type != App::DocumentObject::getClassTypeId()) {
        Base::Type parentType = type.getParent();
        if (!parentType.isBad()) {
            const char* name = parentType.getName();
            auto typeData = QByteArray::fromRawData(name, strlen(name) + 1);
            auto& typeItem = typeItems[typeData];
            if (!typeItem) {
                typeItem = createTypeItem(parentType);
                typeItem->setData(0, TypeNameRole, typeData);
            }
            item = typeItem;
        }
    }

    if (!item) {
        item = new QTreeWidgetItem(ui->typeTree);
    }
    else {
        item = new QTreeWidgetItem(item);
    }
    item->setExpanded(true);
    item->setText(0, QString::fromLatin1(type.getName()));
    if (type == App::DocumentObject::getClassTypeId()) {
        item->setFlags(Qt::ItemIsEnabled);
    }
    return item;
}

bool DlgDocumentObject::filterType(QTreeWidgetItem* item)
{
    auto proxyType = item->data(0, ProxyTypeRole).toByteArray();
    QTreeWidgetItem* proxyItem = nullptr;
    if (proxyType.size()) {
        auto& pitem = typeItems[proxyType];
        if (!pitem) {
            pitem = new QTreeWidgetItem(ui->typeTree);
            pitem->setText(0, QString::fromLatin1(proxyType));
            pitem->setIcon(0, item->icon(0));
            pitem->setData(0, TypeNameRole, proxyType);
        }
        proxyItem = pitem;
    }

    auto typeData = item->data(0, TypeNameRole).toByteArray();
    Base::Type type = Base::Type::fromName(typeData.constData());
    if (type.isBad()) {
        return false;
    }

    QTreeWidgetItem*& typeItem = typeItems[typeData];
    if (!typeItem) {
        typeItem = createTypeItem(type);
        typeItem->setData(0, TypeNameRole, typeData);
    }

    if (!proxyType.size()) {
        QIcon icon = typeItem->icon(0);
        if (icon.isNull()) {
            typeItem->setIcon(0, item->icon(0));
        }
    }

    if (!ui->checkObjectType->isChecked() || selectedTypes.empty()) {
        return false;
    }

    if (proxyItem && selectedTypes.count(proxyType)) {
        return false;
    }

    for (auto t = type; !t.isBad() && t != App::DocumentObject::getClassTypeId();
         t = t.getParent()) {
        const char* name = t.getName();
        if (selectedTypes.count(QByteArray::fromRawData(name, strlen(name) + 1))) {
            return false;
        }
    }

    return true;
}

void DlgDocumentObject::onItemExpanded(QTreeWidgetItem* item)
{
    if (item->childCount()) {
        return;
    }

    QByteArray docName = item->data(0, DocNameRole).toByteArray();
    auto doc = App::GetApplication().getDocument(docName);
    if (!doc) {
        return;
    }

    QByteArray objName = item->data(0, ObjectNameRole).toByteArray();
    if (objName.isEmpty()) {
        for (auto obj : doc->getObjects()) {
            auto newItem = createItem(obj, item);
            if (newItem) {
                itemMap[obj] = newItem;
            }
        }
    }
    else if (allowSubObject) {
        auto obj = doc->getObject(objName);
        if (!obj) {
            return;
        }
        std::set<App::DocumentObject*> childSet;
        std::string sub;
        for (auto child : obj->getLinkedObject(true)->getOutList()) {
            if (!childSet.insert(child).second) {
                continue;
            }
            sub = child->getNameInDocument();
            sub += ".";
            if (obj->getSubObject(sub.c_str())) {
                createItem(child, item);
            }
        }
    }
}

void DlgDocumentObject::onObjectTypeToggled(bool on)
{
    ui->typeTree->setVisible(on);
    filterObjects();
}

void DlgDocumentObject::onTypeTreeItemSelectionChanged()
{

    selectedTypes.clear();
    const auto items = ui->typeTree->selectedItems();
    for (auto item : items) {
        selectedTypes.insert(item->data(0, TypeNameRole).toByteArray());
    }

    if (ui->checkObjectType->isChecked()) {
        filterObjects();
    }
}

void DlgDocumentObject::onSearchBoxTextChanged(const QString& text)
{
    itemSearch(text, false);
}

#include "moc_DlgDocumentObject.cpp"
