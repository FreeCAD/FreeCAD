/****************************************************************************
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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
# include <boost/algorithm/string/predicate.hpp>
#endif

#include <vector>
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>

#include "PropertyManagerModel.h"
#include "PropertyManagerItem.h"


using namespace Gui;
using namespace Gui::PropertyEditor;

const int ROOT = -1;
const int DOC = 0;
const int OBJ = 1;
const int GROUP = 2;
const int PROPERTY = 3;

FC_LOG_LEVEL_INIT("PropertyManagerModel", true, true)

std::unordered_map<QString, PropertyEditor::Filter> PropertyManagerModel::scopes = {};

PropertyManagerModel::PropertyManagerModel(QObject* parent)
: PropertyModel2(parent)
{
}

PropertyManagerModel::~PropertyManagerModel()
{
}

void PropertyManagerModel::addProperties(QModelIndex & parentIndex, int rowInParent)
{
    QModelIndex objIndex = index(rowInParent, 0, parentIndex);
    auto* parentItem = static_cast<DocumentObjectItem*>(objIndex.internalPointer());
    
    App::DocumentObject* obj = parentItem->getObject();

    std::vector<App::Property*> properties = filter.getPropertiesFiltered(obj);

    if (properties.size() > 0) {

        PropertyEditor::PropertyModel2::PropertyList props;
    
        for (auto prop : properties) {
            std::vector<App::Property*> items(1,prop);
            props.emplace_back(prop->getName(), std::move(items));
                           
        }

        buildUp(objIndex, std::move(props));
    }
}

void PropertyManagerModel::addDocumentObjects(/*DocumentItem* parentItem, */QModelIndex & parentIndex, int rowInParent)
{
    QModelIndex docIndex = index(rowInParent, 0, parentIndex);
    auto* docItem = static_cast<DocumentItem*>(docIndex.internalPointer());
    
    App::Document* doc = docItem->getDocument();
    std::vector<App::DocumentObject*> objs = filter.getObjectsFiltered(doc);
    int nrObjs = static_cast<int>(objs.size());
    
    //beginInsertRows(docIndex, 0, nrObjs - 1);
    for (int row = 0; row < nrObjs; row++) {
        App::DocumentObject* obj = objs[row];
        auto *objItem = new DocumentObjectItem(obj);
        objItem->setParent(docItem);
        QString name = QString::fromLatin1(obj->Label.getValue());
        objItem->setPropertyName(name, name);
        docItem->appendChild(objItem);
        addProperties(docIndex, row);
    }
    //endInsertRows();
}

void PropertyManagerModel::init()
{
    std::vector<App::Document*> docs = filter.getDocumentsFiltered();
    int nrDocs = static_cast<int>(docs.size());

    QModelIndex rootIndex = index(ROOT, 0);

    beginResetModel();
    removeRows(0, rowCount(), index(ROOT, 0));

    //beginInsertRows(rootIndex, 0, nrDocs - 1);
    for (int row = 0; row < nrDocs; row++) {
        auto doc = docs[row];
        auto *di = new DocumentItem(doc);
        di->setParent(rootItem);
        QString name = QString::fromLatin1(doc->Label.getValue());
        di->setPropertyName(name, name);
        rootItem->appendChild(di);
        addDocumentObjects(rootIndex, row);
    }
    //endInsertRows();

    removeItemsWithNoChildren();
    endResetModel();
}

void PropertyManagerModel::removeItemsWithNoChildren() {
    QModelIndex rootIndex = index(ROOT, 0);
    removeItemsWithNoChildren(rootIndex, 0, PROPERTY);
}

void PropertyManagerModel::removeItemsWithNoChildren(QModelIndex& parent, int level, int upToLevel)
{
    if (level < upToLevel) {
        for (int row = 0; row < rowCount(parent); row++) {
            QModelIndex indexChild = index(row, 0, parent);

            removeItemsWithNoChildren(indexChild, ++level, upToLevel);
            if (rowCount(indexChild) == 0) {
                removeRow(row, parent);
                --row;
            }
        }
    }
}


QList<QModelIndex> PropertyManagerModel::childrenAtLevel(int level){
    QList<QModelIndex> children;
    QModelIndex rootIndex = index(ROOT, 0);
    childrenAtLevel(children, rootIndex, level);
    
    return children;
}

void PropertyManagerModel::childrenAtLevel(QList<QModelIndex>& children, QModelIndex& parent, int level){
    for (int row = 0; row < rowCount(parent); ++row) {
        QModelIndex childIndex = index(row, 0, parent);

        if (level == 0) {
            children.append(childIndex);
        } else {
            childrenAtLevel(children, childIndex, level - 1);
        }
    }
}

template <class T>
void PropertyManagerModel::getFilterValues(
        UniqueVector<QString>& filterValues,
        int level,
        std::function<void(T*)> funcAct,
        std::function<QString(T*)> funcProduce)
{
    for (auto index : childrenAtLevel(level)) {
        auto item = static_cast<T*>(index.internalPointer());
        funcAct(item);
        filterValues.push_back(funcProduce(item));
    }
}

template<class T>
static void doNothing(T* item) {};


UniqueVector<QString> PropertyManagerModel::getFilterValuesDocument()
{
    UniqueVector<QString> filterValues = filter.getFilterValuesDocument();

    getFilterValues<DocumentItem>(filterValues, DOC, [&filterValues](DocumentItem* docItem) {
        App::Document* doc = docItem->getDocument();
        App::Document *activeDoc = App::GetApplication().getActiveDocument();
        if (doc == activeDoc) {
            filterValues.push_back(Filter::ACTIVE_DOC);
        }
    }, [](DocumentItem* docItem) {
        App::Document* doc = docItem->getDocument();
        return QString::fromStdString(doc->Label.getStrValue());
    });
        
    return filterValues;
}

UniqueVector<QString> PropertyManagerModel::getFilterValuesTypeObject()
{
    UniqueVector<QString> filterValues = filter.getFilterValuesTypeObject();
    getFilterValues<DocumentObjectItem>(filterValues, OBJ,
                                        &doNothing<DocumentObjectItem>,
                                        [](DocumentObjectItem* objItem) {
                                            App::DocumentObject* obj = objItem->getObject();
                                            return Filter::getType(obj);
                                        });
    return filterValues;
}

UniqueVector<QString> PropertyManagerModel::getFilterValuesNameObject()
{
    UniqueVector<QString> filterValues = filter.getFilterValuesNameObject();

    getFilterValues<DocumentObjectItem>(filterValues, OBJ,
                                        &doNothing<DocumentObjectItem>,
                                        [](DocumentObjectItem* objItem) {
                                            App::DocumentObject* obj = objItem->getObject();
                                            return QString::fromStdString(obj->Label.getStrValue());
                                        });

    return filterValues;
}

void PropertyManagerModel::getGroupNames(UniqueVector<QString> &groupNames)
{
    getFilterValues<PropertySeparatorItem>(groupNames, GROUP, &doNothing<PropertySeparatorItem>,
                                           [](PropertySeparatorItem* groupItem) {
                                               return groupItem->propertyName();
                                           });
}

UniqueVector<QString> PropertyManagerModel::getGroupNames() {
    UniqueVector<QString> groupNames;
    getGroupNames(groupNames);
    return groupNames;
}

UniqueVector<QString> PropertyManagerModel::getFilterValuesGroup()
{
    UniqueVector<QString> filterValues = filter.getFilterValuesGroup();
    getGroupNames(filterValues);
    return filterValues;
}

UniqueVector<QString> PropertyManagerModel::getFilterValuesTypeProperty()
{
    UniqueVector<QString> filterValues = filter.getFilterValuesTypeProperty();
    getFilterValues<PropertyItem>(filterValues, PROPERTY, &doNothing<PropertyItem>,
                                  [](PropertyItem* propItem) {
                                      return Filter::getType(propItem->getFirstProperty());
                                  });
    return filterValues;
}

UniqueVector<QString> PropertyManagerModel::getFilterValuesNameProperty()
{
    UniqueVector<QString> filterValues = filter.getFilterValuesNameProperty();
    getFilterValues<PropertyItem>(filterValues, PROPERTY, &doNothing<PropertyItem>,
                                  [](PropertyItem* propItem) {
                                      return QString::fromStdString(propItem->getFirstProperty()->getName());
                                  });
    return filterValues;
}

void PropertyManagerModel::filterSelected(QString &selectedDocument,
                                          QString &selectedTypeObject, QString &selectedNameObject,
                                          QString &selectedGroup,
                                          QString &selectedTypeProperty, QString &selectedNameProperty)
{
    filter.filter(selectedDocument,
                  selectedTypeObject, selectedNameObject,
                  selectedGroup,
                  selectedTypeProperty, selectedNameProperty);
    init();
}


App::DocumentObject* PropertyManagerModel::getUniqueObject()
{
    QList<QModelIndex> objs = childrenAtLevel(OBJ);
    if (objs.size() == 1) {
        QModelIndex indexObj = objs[0];
        auto objItem = static_cast<DocumentObjectItem*>(indexObj.internalPointer());
        return objItem->getObject();
    }
    return nullptr;
}

QString PropertyManagerModel::getUniqueTypeProperty()
{
    UniqueVector<QString> types;

    for (auto index : childrenAtLevel(PROPERTY)) {
        auto propItem = static_cast<PropertyItem*>(index.internalPointer());
        types.push_back(Filter::getType(propItem->getFirstProperty()));
    }

    if (types.size() == 1) {
        return types[0];
    }
    
    return {};
}


QString PropertyManagerModel::getUniqueGroup()
{
    UniqueVector<QString> groups;
    
    for (auto index : childrenAtLevel(GROUP)) {
        auto groupItem = static_cast<PropertySeparatorItem*>(index.internalPointer());
        groups.push_back(groupItem->propertyName());
    }
    
    if (groups.size() == 1) {
        return groups[0];
    }
    
    return {};
}

void PropertyManagerModel::refresh()
{
    init();
}

bool PropertyManagerModel::scopeExists(QString& name) {
    return scopes.find(name) != scopes.end();
}

void PropertyManagerModel::selectScope(QString& name) {
    filter = scopes[name];
}

void PropertyManagerModel::insertScope(QString& name) {
    scopes[name] = filter;
    filter.setName(name);
}

UniqueVector<QString> PropertyManagerModel::getNamesScope()
{
    UniqueVector<QString> names;
    QString nameFilter = filter.getName();
    if (!nameFilter.isEmpty()) {
        names.push_back(nameFilter);
    }

    for (const auto& pair : scopes) {
        names.push_back(pair.first);
    }

    return names;
}

#include "moc_PropertyManagerModel.cpp"
