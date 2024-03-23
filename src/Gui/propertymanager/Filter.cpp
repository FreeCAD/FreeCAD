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

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>

#include "Filter.h"


using namespace Gui;
using namespace Gui::PropertyEditor;

FC_LOG_LEVEL_INIT("Filter", true, true)

const QString Filter::STAR = QString::fromLatin1("\"*\"");
const QString Filter::ACTIVE_DOC = QString::fromLatin1("\"Active document\"");
const QString Filter::BASE_GROUP = QString::fromLatin1("Base");
//const QString Filter::ACTIVE_OBJ = QString::fromLatin1("\"Active object\"");

Filter::Filter() : filterDocument(ACTIVE_DOC),
                   filterTypeObject(STAR), filterNameObject(STAR),
                   filterGroup(STAR),
                   filterTypeProp(STAR), filterNameProp(STAR)
{
}

UniqueVector<QString> Filter::getFilterValuesDocument()
{
    return {filterDocument, STAR};
}

QString Filter::getType(Base::Persistence* obj)
{
    return QString::fromStdString(obj->getTypeId().getName());
}

UniqueVector<QString> Filter::getFilterValuesTypeObject()
{
    return {filterTypeObject, STAR};
}

UniqueVector<QString> Filter::getFilterValuesNameObject()
{
    return {filterNameObject, STAR};
}

UniqueVector<QString> Filter::getFilterValuesGroup()
{
    return {filterGroup, STAR, BASE_GROUP};
}

UniqueVector<QString> Filter::getFilterValuesTypeProperty()
{
    return {filterTypeProp, STAR};
}

UniqueVector<QString> Filter::getFilterValuesNameProperty()
{
    return {filterNameProp, STAR};
}

void Filter::filter(QString &selectedDocument,
                    QString &selectedTypeObject, QString &selectedNameObject,
                    QString &selectedGroup,
                    QString &selectedTypeProperty,
                    QString &selectedNameProperty)
{
    filterDocument = selectedDocument;
    filterTypeObject = selectedTypeObject;
    filterNameObject = selectedNameObject;
    filterGroup = selectedGroup;
    filterTypeProp = selectedTypeProperty;
    filterNameProp = selectedNameProperty;
}

std::vector<App::Document*> Filter::getDocumentsFiltered()
{
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    std::vector<App::Document*> docsFiltered;
    for (auto doc : docs) {
        QString label = QString::fromStdString(doc->Label.getStrValue());
        if (filterDocument == STAR ||
            filterDocument == label ||
            (filterDocument == ACTIVE_DOC && 
             doc == App::GetApplication().getActiveDocument())) {
            docsFiltered.push_back(doc);
        }
    }

    return docsFiltered;
}

std::vector<App::DocumentObject*> Filter::getObjectsFiltered(App::Document* doc)
{
    std::vector<App::DocumentObject*> objs = doc->getObjects();
    std::vector<App::DocumentObject*> objsFiltered;
    for (auto obj : objs) {
        QString label = QString::fromStdString(obj->Label.getStrValue());
        QString type = getType(obj);
        if ((filterTypeObject == STAR || filterTypeObject == type) &&
            (filterNameObject == STAR || filterNameObject == label)) {
                /* (filterNameObject == ACTIVE_OBJ &&
                   obj == doc->getActiveObject())*/
            objsFiltered.push_back(obj);
        }
    }

    return objsFiltered;
}

std::vector<App::Property*> Filter::getPropertiesFiltered(App::DocumentObject* obj)
{
    std::vector<App::Property*> properties;
    obj->getPropertyList(properties);
    std::vector<App::Property*> propertiesFiltered;

    for (auto prop : properties) {
        QString group = QString::fromUtf8(prop->getGroup());
        QString type = getType(prop);
        QString name = QString::fromUtf8(prop->getName());
        if ((filterGroup == STAR    || filterGroup == group) &&
            (filterTypeProp == STAR || filterTypeProp == type) &&
            (filterNameProp == STAR || filterNameProp == name)) {
            propertiesFiltered.push_back(prop);
        }
    }

    return propertiesFiltered;
}

QString Filter::getName()
{
    return name;
}

void Filter::setName(QString& name)
{
    this->name = name;
}
