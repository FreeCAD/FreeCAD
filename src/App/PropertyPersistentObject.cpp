/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyPersistentObject.h"


namespace App {

TYPESYSTEM_SOURCE(App::PropertyPersistentObject, App::PropertyString)

PyObject* PropertyPersistentObject::getPyObject()
{
    if (_pObject) {
        return _pObject->getPyObject();
    }
    return inherited::getPyObject();
}

void PropertyPersistentObject::Save(Base::Writer& writer) const
{
    inherited::Save(writer);
    writer.Stream() << writer.ind() << "<PersistentObject>" << std::endl;
    if (_pObject) {
        writer.incInd();
        _pObject->Save(writer);
        writer.decInd();
    }
    writer.Stream() << writer.ind() << "</PersistentObject>" << std::endl;
}

void PropertyPersistentObject::Restore(Base::XMLReader& reader)
{
    inherited::Restore(reader);
    reader.readElement("PersistentObject");
    if (_pObject) {
        _pObject->Restore(reader);
    }
    reader.readEndElement("PersistentObject");
}

Property* PropertyPersistentObject::Copy() const
{
    auto* p = new PropertyPersistentObject();
    p->_cValue = _cValue;
    p->_pObject = _pObject;
    return p;
}

void PropertyPersistentObject::Paste(const Property& from)
{
    const auto& prop = dynamic_cast<const PropertyPersistentObject&>(from);
    if (_cValue != prop._cValue || _pObject != prop._pObject) {
        aboutToSetValue();
        _cValue = prop._cValue;
        _pObject = prop._pObject;
        hasSetValue();
    }
}

unsigned int PropertyPersistentObject::getMemSize() const
{
    auto size = inherited::getMemSize();
    if (_pObject) {
        size += _pObject->getMemSize();
    }
    return size;
}

void PropertyPersistentObject::setValue(const char* type)
{
    if (!type) {
        type = "";
    }
    if (type[0]) {
        Base::Type::importModule(type);
        Base::Type t = Base::Type::fromName(type);
        if (t.isBad()) {
            throw Base::TypeError("Invalid type");
        }
        if (!t.isDerivedFrom(Persistence::getClassTypeId())) {
            throw Base::TypeError("Type must be derived from Base::Persistence");
        }
        if (_pObject && _pObject->getTypeId() == t) {
            return;
        }
    }
    aboutToSetValue();
    _pObject.reset();
    _cValue = type;
    if (type[0]) {
        _pObject.reset(static_cast<Base::Persistence*>(Base::Type::createInstanceByName(type)));
    }
    hasSetValue();
}

}  // namespace App
