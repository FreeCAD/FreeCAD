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

TYPESYSTEM_SOURCE(Materials::ModelValueProperty, Materials::ModelProperty)

ModelValueProperty::ModelValueProperty()
{
    _valueType = MaterialValue::None;
}

ModelValueProperty::ModelValueProperty(const ModelProperty &property) :
    ModelProperty(property)
{
    setType(getPropertyType());
    for (auto it : property.columns()) {
        ModelValueProperty prop(it);
        addColumn(prop);
    }
}

ModelValueProperty::~ModelValueProperty()
{

}

void ModelValueProperty::setModelUUID(const QString& uuid)
{
    _modelUUID = uuid;
}

const QString ModelValueProperty::getValue(void) const
{
    switch (_valueType){
        case MaterialValue::String:
        case MaterialValue::URL:
            return _valueString;

        case MaterialValue::Boolean:
            return (_valueBoolean) ? QString::fromStdString("true") : QString::fromStdString("false");

        case MaterialValue::Int:
            return QString::number(_valueInt);

        case MaterialValue::Float:
            return QString::number(_valueFloat);

        default:
            break;
    }

    return QString::fromStdString("");
}

void ModelValueProperty::setPropertyType(const QString& type)
{
    ModelProperty::setPropertyType(type);
    setType(type);
}

void ModelValueProperty::setType(const QString& type)
{
    if (type == QString::fromStdString("String"))
    {
       setType(MaterialValue::String);
    } else if (type == QString::fromStdString("Boolean"))
    {
       setType(MaterialValue::Boolean);
    } else if (type == QString::fromStdString("Int"))
    {
       setType(MaterialValue::Int);
    } else if (type == QString::fromStdString("Float"))
    {
       setType(MaterialValue::Float);
    } else if (type == QString::fromStdString("URL"))
    {
       setType(MaterialValue::URL);
    } else if (type == QString::fromStdString("Quantity"))
    {
       setType(MaterialValue::String);
    } else if (type == QString::fromStdString("Color"))
    {
       setType(MaterialValue::String);
    } else if (type == QString::fromStdString("File"))
    {
       setType(MaterialValue::String);
    } else if (type == QString::fromStdString("Image"))
    {
       setType(MaterialValue::String);
    } else {
        // Error. Throw someting
       setType(MaterialValue::None);
    }
}

ModelValueProperty &ModelValueProperty::getColumn(int column)
{
    try {
        return _columns.at(column);
    } catch (std::out_of_range const&) {
        throw PropertyNotFound();
    }
}

void ModelValueProperty::setValue(const QString& value)
{
    if (_valueType == MaterialValue::Boolean)
       setBoolean(value);
    else if (_valueType == MaterialValue::Int)
       setInt(value);
    else if (_valueType == MaterialValue::Float)
       setFloat(value);
    else if (_valueType == MaterialValue::URL)
       setURL(value);
    else
       setString(value);
}

void ModelValueProperty::setString(const QString& value)
{
    // _valueType = MaterialValue::String;
    _valueString = value;
}

void ModelValueProperty::setBoolean(bool value)
{
    // _valueType = MaterialValue::Boolean;
    _valueBoolean = value;
}

void ModelValueProperty::setBoolean(int value)
{
    // _valueType = MaterialValue::Boolean;
    _valueBoolean = (value != 0);
}

void ModelValueProperty::setBoolean(const QString& value)
{
    // _valueType = MaterialValue::Boolean;
    std::string val = value.toStdString();
    if ((val == "true") || (val == "True"))
       _valueBoolean = true;
    else if ((val == "false") || (val == "False"))
       _valueBoolean = true;
    else
        _valueBoolean = std::stoi(val);
}

void ModelValueProperty::setInt(int value)
{
    // _valueType = MaterialValue::Int;
    _valueInt = value;
}

void ModelValueProperty::setInt(const QString& value)
{
    // _valueType = MaterialValue::Int;
    _valueInt = std::stoi(value.toStdString());
}

void ModelValueProperty::setFloat(double value)
{
    // _valueType = MaterialValue::Float;
    _valueFloat = value;
}

void ModelValueProperty::setFloat(const QString& value)
{
    // _valueType = MaterialValue::Float;
    _valueFloat = std::stod(value.toStdString());
}

void ModelValueProperty::setQuantity(const Base::Quantity& value)
{
    // _valueType = MaterialValue::Quantity;
    _valueQuantity = value;
}

void ModelValueProperty::setQuantity(double value, const QString& units)
{
    Q_UNUSED(units);

    // _valueType = MaterialValue::Quantity;
    // _valueQuantity = Base::Quantity(value, QString::fromStdString(units));
    _valueString = QString::number(value);
}

void ModelValueProperty::setQuantity(const QString& value)
{
    Q_UNUSED(value);

    _valueType = MaterialValue::Quantity;
    // _valueQuantity = value;
}

void ModelValueProperty::setURL(const QString& value)
{
    // _valueType = MaterialValue::URL;
    _valueString = value; // Different type but same representation
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
