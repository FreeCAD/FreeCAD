/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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
# include <sstream>
#endif


#include <App/DocumentObject.h>
#include <App/PropertyContainer.h>
#include <Base/Console.h>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>

#include "PropertyPath.h"
#include "PathPy.h"

using namespace Path;

TYPESYSTEM_SOURCE(Path::PropertyPath, App::Property)

PropertyPath::PropertyPath()
{
}

PropertyPath::~PropertyPath()
{
}

void PropertyPath::setValue(const Toolpath& pa)
{
    aboutToSetValue();
    _Path = pa;
    hasSetValue();
}


const Toolpath &PropertyPath::getValue(void)const
{
    return _Path;
}

PyObject *PropertyPath::getPyObject(void)
{
    return new PathPy(new Toolpath(_Path));
}

void PropertyPath::setPyObject(PyObject *value)
{
    if (PyObject_TypeCheck(value, &(PathPy::Type))) {
        PathPy *pcObject = static_cast<PathPy*>(value);
        setValue(*pcObject->getToolpathPtr());
    }
    else {
        std::string error = std::string("type must be 'Path', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

App::Property *PropertyPath::Copy(void) const
{
    PropertyPath *prop = new PropertyPath();
    prop->_Path = this->_Path;

    return prop;
}

void PropertyPath::Paste(const App::Property &from)
{
    aboutToSetValue();
    _Path = dynamic_cast<const PropertyPath&>(from)._Path;
    hasSetValue();
}

unsigned int PropertyPath::getMemSize (void) const
{
    return _Path.getMemSize();
}

void PropertyPath::Save (Base::Writer &writer) const
{
    _Path.setFileName(getFileName().c_str());
    _Path.Save(writer);
}

void PropertyPath::Restore(Base::XMLReader &reader)
{
#if 0
    reader.readElement("Path");

    std::string file (reader.getAttribute("file") );
    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(),this);
    }

    if (reader.hasAttribute("version")) {
        int version = reader.getAttributeAsInteger("version");
        if (version >= Toolpath::SchemaVersion) {
            reader.readElement("Center");
            double x = reader.getAttributeAsFloat("x");
            double y = reader.getAttributeAsFloat("y");
            double z = reader.getAttributeAsFloat("z");
            Base::Vector3d center(x, y, z);
            _Path.setCenter(center);
        }
    }
#else
    aboutToSetValue();
    _Path.Restore(reader);
    hasSetValue();
#endif
}

void PropertyPath::SaveDocFile (Base::Writer &) const
{
    // does nothing
}

void PropertyPath::RestoreDocFile(Base::Reader &reader)
{
    // Since we are calling  _Path.Restore() above, RestoreDocFile will be called 
    // on _Path instead. I guess the reason for the original 'unnatural'
    // implementation is because 
    //
    // a) Need to call aboutToSetValue(), which is now called inside
    //    PropertyPath::Restore(). Besides, I don't think aboutTo/hasSetValue() does
    //    anything useful during restore, unless someone want to undo from a restore?
    //    Even so, the above call inside Restore() is enough
    //
    // b) Need to to manually set ObjectStatus::Restore because App::Document has
    //    reset that flag when calling RestoreDocFile. However, this is no
    //    longer the case, the flag will remain until calling onDocumentRestored().

#if 1
    (void)reader;
#else
    App::PropertyContainer *container = getContainer();
    App::DocumentObject *obj = nullptr;
    if (container->isDerivedFrom(App::DocumentObject::getClassTypeId())) {
        obj = static_cast<App::DocumentObject*>(container);
    }

    if (obj) {
        obj->setStatus(App::ObjectStatus::Restore, true);
    }

    aboutToSetValue();
    _Path.RestoreDocFile(reader);
    hasSetValue();

    if (obj) {
        obj->setStatus(App::ObjectStatus::Restore, false);
    }
#endif
}



