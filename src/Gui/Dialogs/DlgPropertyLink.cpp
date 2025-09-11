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

#include "Dialogs/DlgPropertyLink.h"
#include "ui_DlgDocumentObject.h"
#include "Document.h"
#include "BitmapFactory.h"
#include "PropertyView.h"
#include "Selection.h"
#include "ViewProviderDocumentObject.h"


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgPropertyLink */

DlgPropertyLink::DlgPropertyLink(QWidget* parent)
    : DlgDocumentObject(parent)
{
}

QList<App::SubObjectT> DlgPropertyLink::getLinksFromProperty(const App::PropertyLinkBase* prop)
{
    QList<App::SubObjectT> res;
    if (!prop) {
        return res;
    }

    std::vector<App::DocumentObject*> objs;
    std::vector<std::string> subs;
    prop->getLinks(objs, true, &subs, false);
    if (subs.empty()) {
        for (auto obj : objs) {
            res.push_back(App::SubObjectT(obj, nullptr));
        }
    }
    else if (objs.size() == 1) {
        for (auto& sub : subs) {
            res.push_back(App::SubObjectT(objs.front(), sub.c_str()));
        }
    }
    else {
        int i = 0;
        for (auto obj : objs) {
            res.push_back(App::SubObjectT(obj, subs[i++].c_str()));
        }
    }
    return res;
}

QString
DlgPropertyLink::formatObject(App::Document* ownerDoc, App::DocumentObject* obj, const char* sub)
{
    if (!obj || !obj->isAttachedToDocument()) {
        return QLatin1String("?");
    }

    const char* objName = obj->getNameInDocument();
    std::string _objName;
    if (ownerDoc && ownerDoc != obj->getDocument()) {
        _objName = obj->getFullName();
        objName = _objName.c_str();
    }

    if (!sub || !sub[0]) {
        if (obj->Label.getStrValue() == obj->getNameInDocument()) {
            return QLatin1String(objName);
        }
        return QStringLiteral("%1 (%2)").arg(QString::fromUtf8(obj->Label.getValue()),
                                                  QLatin1String(objName));
    }

    auto sobj = obj->getSubObject(sub);
    if (!sobj || sobj->Label.getStrValue() == sobj->getNameInDocument()) {
        return QStringLiteral("%1.%2").arg(QLatin1String(objName), QString::fromUtf8(sub));
    }

    return QStringLiteral("%1 (%2.%3)")
        .arg(QString::fromUtf8(sobj->Label.getValue()),
             QLatin1String(objName),
             QString::fromUtf8(sub));
}

static inline bool isLinkSub(const QList<App::SubObjectT>& links)
{
    for (const auto& link : links) {
        if (&link == &links.front()) {
            continue;
        }
        if (link.getDocumentName() != links.front().getDocumentName()
            || link.getObjectName() != links.front().getObjectName()) {
            return false;
        }
    }
    return true;
}

QString DlgPropertyLink::formatLinks(App::Document* ownerDoc, QList<App::SubObjectT> links)
{
    if (!ownerDoc || links.empty()) {
        return {};
    }

    auto obj = links.front().getObject();
    if (!obj) {
        return QLatin1String("?");
    }

    if (links.size() == 1 && links.front().getSubName().empty()) {
        return formatObject(ownerDoc, links.front());
    }

    QStringList list;
    if (isLinkSub(links)) {
        int i = 0;
        for (auto& link : links) {
            list << QString::fromUtf8(link.getSubName().c_str());
            if (++i >= 3) {
                break;
            }
        }
        return QStringLiteral("%1 [%2%3]")
            .arg(formatObject(ownerDoc, obj, nullptr),
                 list.join(QLatin1String(", ")),
                 QLatin1String(links.size() > 3 ? " ..." : ""));
    }

    int i = 0;
    for (auto& link : links) {
        list << formatObject(ownerDoc, link);
        if (++i >= 3) {
            break;
        }
    }
    return QStringLiteral("[%1%2]").arg(list.join(QLatin1String(", ")),
                                             QLatin1String(links.size() > 3 ? " ..." : ""));
}

void DlgPropertyLink::init(const App::DocumentObjectT& prop, bool tryFilter)
{
    setWindowTitle(tr("Link"));
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

    objProp = prop;
    owner = objProp.getObject();
    if (!owner || !owner->isAttachedToDocument()) {
        return;
    }

    ui->searchBox->setDocumentObject(owner);

    auto propLink = freecad_cast<App::PropertyLinkBase*>(objProp.getProperty());
    if (!propLink) {
        return;
    }

    oldLinks = getLinksFromProperty(propLink);

    if (propLink->getScope() != App::LinkScope::Hidden) {
        // populate inList to filter out any objects that contains the owner object
        // of the editing link property
        inList = owner->getInListEx(true);
        inList.insert(owner);
    }

    std::vector<App::Document*> docs;

    singleSelect = false;
    if (propLink->isDerivedFrom<App::PropertyXLinkSub>()
        || propLink->isDerivedFrom<App::PropertyLinkSub>()) {
        allowSubObject = true;
        singleParent = true;
    }
    else if (propLink->isDerivedFrom<App::PropertyLink>()) {
        singleSelect = true;
    }
    else if (propLink->isDerivedFrom<App::PropertyLinkSubList>()) {
        allowSubObject = true;
    }

    if (App::PropertyXLink::supportXLink(propLink)) {
        allowSubObject = true;
        docs = App::GetApplication().getDocuments();
    }
    else {
        docs.push_back(owner->getDocument());
    }

    bool isLinkList = false;
    if (propLink->isDerivedFrom<App::PropertyXLinkList>()
        || propLink->isDerivedFrom<App::PropertyLinkList>()) {
        isLinkList = true;
        allowSubObject = false;
    }

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

    if (oldLinks.empty()) {
        expandDocs.insert(owner->getDocument());
    }
    else {
        for (auto& link : oldLinks) {
            auto doc = link.getDocument();
            if (doc) {
                expandDocs.insert(doc);
            }
        }
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

    if (allowSubObject) {
        if (propLink->testFlag(App::PropertyLinkBase::LinkSyncSubObject)) {
            ui->checkSubObject->setChecked(true);
        }
        else {
            for (auto& link : oldLinks) {
                auto sobj = link.getSubObject();
                if (sobj && sobj != link.getObject()) {
                    ui->checkSubObject->setChecked(true);
                    break;
                }
            }
        }
    }

    if (oldLinks.isEmpty()) {
        return;
    }

    // Try to select items corresponding to the current links inside the
    // property
    {
        QSignalBlocker blockTree(ui->treeWidget);
        QSignalBlocker blockSelectionModel(ui->treeWidget->selectionModel());
        for (auto& link : oldLinks) {
            onSelectionChanged(Gui::SelectionChanges(SelectionChanges::AddSelection,
                                                     link.getDocumentName(),
                                                     link.getObjectName(),
                                                     link.getSubName()));
        }
    }

    // For link list type property, try to auto filter type
    if (tryFilter && isLinkList) {
        Base::Type objType;
        for (const auto& link : std::as_const(oldLinks)) {
            auto obj = link.getSubObject();
            if (!obj) {
                continue;
            }
            if (objType.isBad()) {
                objType = obj->getTypeId();
                continue;
            }
            for (; objType != App::DocumentObject::getClassTypeId();
                 objType = objType.getParent()) {
                if (obj->isDerivedFrom(objType)) {
                    break;
                }
            }
        }

        Base::Type baseType;
        // get only geometric types
        if (objType.isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            baseType = App::GeoFeature::getClassTypeId();
        }
        else {
            baseType = App::DocumentObject::getClassTypeId();
        }

        // get the direct base class of App::DocumentObject which 'obj' is derived from
        while (!objType.isBad()) {
            Base::Type parType = objType.getParent();
            if (parType == baseType) {
                baseType = objType;
                break;
            }
            objType = parType;
        }

        if (!baseType.isBad()) {
            const char* name = baseType.getName();
            auto it = typeItems.find(QByteArray::fromRawData(name, strlen(name) + 1));
            if (it != typeItems.end()) {
                it->second->setSelected(true);
            }
            ui->checkObjectType->setChecked(true);
        }
    }
}

void DlgPropertyLink::onClicked(QAbstractButton* button)
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
        init(objProp);
    }
}

QList<App::SubObjectT> DlgPropertyLink::currentLinks() const
{
    return currentSubObjects();
}

QList<App::SubObjectT> DlgPropertyLink::originalLinks() const
{
    return oldLinks;
}

QString DlgPropertyLink::linksToPython(const QList<App::SubObjectT>& links)
{
    if (links.isEmpty()) {
        return QLatin1String("None");
    }

    if (links.size() == 1) {
        return QString::fromLatin1(links.front().getSubObjectPython(false).c_str());
    }

    std::ostringstream ss;

    if (isLinkSub(links)) {
        ss << '(' << links.front().getObjectPython() << ", [";
        for (const auto& link : links) {
            const auto& sub = link.getSubName();
            if (!sub.empty()) {
                ss << "u'" << Base::Tools::escapedUnicodeFromUtf8(sub.c_str()) << "',";
            }
        }
        ss << "])";
    }
    else {
        ss << '[';
        for (const auto& link : links) {
            ss << link.getSubObjectPython(false) << ',';
        }
        ss << ']';
    }

    return QString::fromLatin1(ss.str().c_str());
}

#include "moc_DlgPropertyLink.cpp"
