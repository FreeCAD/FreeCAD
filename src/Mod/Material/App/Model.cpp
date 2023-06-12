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
#include "Model.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelLibrary, Base::BaseClass)

ModelLibrary::ModelLibrary(const std::string &libraryName, const QDir &dir, const std::string &icon):
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

ModelProperty::ModelProperty(const std::string& name, const std::string& type,
                        const std::string& units, const std::string& url,
                        const std::string& description):
    _name(name), _type(type), _units(units), _url(url), _description(description)
{

}

ModelProperty::~ModelProperty()
{

}

TYPESYSTEM_SOURCE(Materials::ModelValueProperty, Materials::ModelProperty)

ModelValueProperty::ModelValueProperty()
{

}

ModelValueProperty::ModelValueProperty(const ModelProperty &property) :
    ModelProperty(property)
{

}

ModelValueProperty::~ModelValueProperty()
{

}

void ModelValueProperty::setModelUUID(const std::string& uuid)
{
    _modelUUID = uuid;
}

void ModelValueProperty::setString(const std::string& value)
{
    _valueType = ValueType::String;
    _valueString = value;
}

void ModelValueProperty::setInt(int value)
{
    _valueType = ValueType::Int;
    _valueInt = value;
}

void ModelValueProperty::setInt(const std::string& value)
{
    _valueType = ValueType::Int;
    _valueInt = std::stoi(value);
}

void ModelValueProperty::setFloat(double value)
{
    _valueType = ValueType::Float;
    _valueFloat = value;
}

void ModelValueProperty::setFloat(const std::string& value)
{
    _valueType = ValueType::Float;
    _valueFloat = std::stod(value);
}

void ModelValueProperty::setQuantity(const Base::Quantity& value)
{
    _valueType = ValueType::Quantity;
    _valueQuantity = value;
}

void ModelValueProperty::setQuantity(double value, const std::string& units)
{
    _valueType = ValueType::Quantity;
    _valueQuantity = Base::Quantity(value, QString::fromStdString(units));
}

void ModelValueProperty::setQuantity(const std::string& value)
{
    _valueType = ValueType::Quantity;
    // _valueQuantity = value;
}


TYPESYSTEM_SOURCE(Materials::Model, Base::BaseClass)

Model::Model()
{}

Model::Model(const ModelLibrary &library, ModelType type, const std::string &name, const QDir &directory, 
        const std::string &uuid, const std::string& description, const std::string& url,
        const std::string& doi):
    _library(library), _type(type), _name(name), _directory(directory), _uuid(uuid), _description(description),
    _url(url), _doi(doi)
{}

Model::~Model()
{}

#include "moc_Model.cpp"
