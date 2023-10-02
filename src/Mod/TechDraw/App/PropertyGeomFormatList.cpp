/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyGeomFormatList.h"
#include "GeomFormatPy.h"


using namespace App;
using namespace Base;
using namespace TechDraw;

//**************************************************************************
// PropertyGeomFormatList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(TechDraw::PropertyGeomFormatList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyGeomFormatList::PropertyGeomFormatList()
{

}

PropertyGeomFormatList::~PropertyGeomFormatList()
{
    for (std::vector<GeomFormat*>::iterator it = _lValueList.begin(); it != _lValueList.end(); ++it)
        if (*it) delete *it;
}

void PropertyGeomFormatList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyGeomFormatList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyGeomFormatList::setValue(const GeomFormat* lValue)
{
    if (lValue) {
        aboutToSetValue();
        GeomFormat* newVal = lValue->clone();
        for (unsigned int i = 0; i < _lValueList.size(); i++)
            delete _lValueList[i];
        _lValueList.resize(1);
        _lValueList[0] = newVal;
        hasSetValue();
    }
}

void PropertyGeomFormatList::setValues(const std::vector<GeomFormat*>& lValue)
{
    aboutToSetValue();
    std::vector<GeomFormat*> oldVals(_lValueList);
    _lValueList.resize(lValue.size());
    // copy all objects
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i]->clone();
    for (unsigned int i = 0; i < oldVals.size(); i++)
        delete oldVals[i];
    hasSetValue();
}

PyObject *PropertyGeomFormatList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyGeomFormatList::setPyObject(PyObject *value)
{
    // check container of this property to notify about changes
//    Part2DObject* part2d = dynamic_cast<Part2DObject*>(this->getContainer());

    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<GeomFormat*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(GeomFormatPy::Type))) {
                std::string error = std::string("types in list must be 'GeomFormat', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<GeomFormatPy*>(item)->getGeomFormatPtr();
        }

        setValues(values);
//        if (part2d)
//            part2d->acceptGeomFormat();
    }
    else if (PyObject_TypeCheck(value, &(GeomFormatPy::Type))) {
        GeomFormatPy  *pcObject = static_cast<GeomFormatPy*>(value);
        setValue(pcObject->getGeomFormatPtr());
    }
    else {
        std::string error = std::string("type must be 'GeomFormat' or list of 'GeomFormat', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyGeomFormatList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<GeomFormatList count=\"" << getSize() << "\">"
                    << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<GeomFormat  type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << std::endl;
        writer.incInd();
        _lValueList[i]->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</GeomFormat>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</GeomFormatList>" << std::endl;
}

void PropertyGeomFormatList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.clearPartialRestoreObject();
    reader.readElement("GeomFormatList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    std::vector<GeomFormat*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("GeomFormat");
        const char* TypeName = reader.getAttribute("type");
        GeomFormat *newG = static_cast<GeomFormat *>(Base::Type::fromName(TypeName).createInstance());
        newG->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("GeomFormat \"%s\" within a PropertyGeomFormatList was subject to a partial restore.\n", reader.localName());
            if(isOrderRelevant()) {
                // Pushes the best try by the GeomFormat class
                values.push_back(newG);
            }
            else {
                delete newG;
            }
            reader.clearPartialRestoreObject();
        }
        else {
            values.push_back(newG);
        }

        reader.readEndElement("GeomFormat");
    }

    reader.readEndElement("GeomFormatList");

    // assignment
    setValues(values);
}

App::Property *PropertyGeomFormatList::Copy() const
{
    PropertyGeomFormatList *p = new PropertyGeomFormatList();
    p->setValues(_lValueList);
    return p;
}

void PropertyGeomFormatList::Paste(const Property &from)
{
    const PropertyGeomFormatList& FromList = dynamic_cast<const PropertyGeomFormatList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyGeomFormatList::getMemSize() const
{
    int size = sizeof(PropertyGeomFormatList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
