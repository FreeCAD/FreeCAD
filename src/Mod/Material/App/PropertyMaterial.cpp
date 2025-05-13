/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMetaType>
#include <QUuid>
#endif


#include <App/Application.h>
#include <Base/Writer.h>
#include <Gui/MetaTypes.h>

#include "MaterialManager.h"
#include "MaterialPy.h"
#include "PropertyMaterial.h"

using namespace Materials;

/* TRANSLATOR Material::PropertyMaterial */

TYPESYSTEM_SOURCE(Materials::PropertyMaterial, App::Property)

PropertyMaterial::PropertyMaterial() = default;

PropertyMaterial::~PropertyMaterial() = default;

void PropertyMaterial::setValue(const Material& mat)
{
    aboutToSetValue();
    _material = mat;
    hasSetValue();
}

void PropertyMaterial::setValue(const App::Material& mat)
{
    aboutToSetValue();
    _material = mat;
    hasSetValue();
}

const Material& PropertyMaterial::getValue() const
{
    return _material;
}

PyObject* PropertyMaterial::getPyObject()
{
    return new MaterialPy(new Material(_material));
}

void PropertyMaterial::setPyObject(PyObject* value)
{
    if (PyObject_TypeCheck(value, &(MaterialPy::Type))) {
        setValue(*static_cast<MaterialPy*>(value)->getMaterialPtr());
    }
    else {
        std::string error = std::string("type must be 'Material' not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyMaterial::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<PropertyMaterial uuid=\""
                    << _material.getUUID().toStdString() << "\"/>" << std::endl;
}

void PropertyMaterial::Restore(Base::XMLReader& reader)
{
    // read my Element
    reader.readElement("PropertyMaterial");
    // get the value of my Attribute
    auto uuid = reader.getAttribute<const char*>("uuid");

    setValue(*MaterialManager::getManager().getMaterial(QString::fromLatin1(uuid)));
}

const char* PropertyMaterial::getEditorName() const
{
    if (testStatus(MaterialEdit)) {
        return "";  //"Gui::PropertyEditor::PropertyMaterialItem";
    }
    return "";
}

App::Property* PropertyMaterial::Copy() const
{
    PropertyMaterial* p = new PropertyMaterial();
    p->_material = _material;
    return p;
}

void PropertyMaterial::Paste(const App::Property& from)
{
    aboutToSetValue();
    _material = dynamic_cast<const PropertyMaterial&>(from)._material;
    hasSetValue();
}
