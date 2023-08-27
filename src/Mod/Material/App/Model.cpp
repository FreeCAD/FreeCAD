/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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
#endif

#include <string>
#include "Exceptions.h"
#include "Model.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelLibrary, Base::BaseClass)

ModelLibrary::ModelLibrary(const QString &libraryName, const QDir &dir, const QString &icon):
    _name(libraryName), _directory(dir), _iconPath(icon)
{}
ModelLibrary::ModelLibrary()
{}

ModelLibrary::~ModelLibrary()
{}

TYPESYSTEM_SOURCE(Materials::ModelProperty, Base::BaseClass)

ModelProperty::ModelProperty()
{

}

ModelProperty::ModelProperty(const QString& name, const QString& type,
                        const QString& units, const QString& url,
                        const QString& description):
    _name(name), _propertyType(type), _units(units), _url(url), _description(description)
{

}

ModelProperty::~ModelProperty()
{

}

ModelProperty& ModelProperty::operator=(const ModelProperty& other)
{
    if (this == &other)
        return *this;

    _name = other._name;
    _propertyType = other._propertyType;
    _units = other._units;
    _url = other._url;
    _description = other._description;
    _inheritance = other._inheritance;
    _columns.clear();
    for (auto it = other._columns.begin(); it != other._columns.end(); it++)
        _columns.push_back(*it);

    return *this;
}

TYPESYSTEM_SOURCE(Materials::Model, Base::BaseClass)

Model::Model()
{}

Model::Model(const ModelLibrary &library, ModelType type, const QString &name, const QDir &directory,
        const QString &uuid, const QString& description, const QString& url,
        const QString& doi):
    _library(library), _type(type), _name(name), _directory(directory), _uuid(uuid), _description(description),
    _url(url), _doi(doi)
{}

Model::~Model()
{}

ModelProperty& Model::operator[] (const QString& key)
{
    try {
        return _properties.at(key);
    } catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

#include "moc_Model.cpp"
