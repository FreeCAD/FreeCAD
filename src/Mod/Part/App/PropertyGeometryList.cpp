/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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
#   include <assert.h>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......

#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Console.h>

#include "Geometry.h"
#include "GeometryPy.h"

#include "PropertyGeometryList.h"
#include "Part2DObject.h"


using namespace App;
using namespace Base;
using namespace std;
using namespace Part;


//**************************************************************************
// PropertyGeometryList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(Part::PropertyGeometryList, App::PropertyLists);

//**************************************************************************
// Construction/Destruction


PropertyGeometryList::PropertyGeometryList()
{

}

PropertyGeometryList::~PropertyGeometryList()
{
    for (std::vector<Geometry*>::iterator it = _lValueList.begin(); it != _lValueList.end(); ++it)
        if (*it) delete *it;
}

void PropertyGeometryList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyGeometryList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyGeometryList::setValue(const Geometry* lValue)
{
    if (lValue) {
        aboutToSetValue();
        Geometry* newVal = lValue->clone();
        for (unsigned int i = 0; i < _lValueList.size(); i++)
            delete _lValueList[i];
        _lValueList.resize(1);
        _lValueList[0] = newVal;
        hasSetValue();
    }
}

void PropertyGeometryList::setValues(const std::vector<Geometry*>& lValue)
{
    aboutToSetValue();
    std::vector<Geometry*> oldVals(_lValueList);
    _lValueList.resize(lValue.size());
    // copy all objects
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i]->clone();
    for (unsigned int i = 0; i < oldVals.size(); i++)
        delete oldVals[i];
    hasSetValue();
}

PyObject *PropertyGeometryList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyGeometryList::setPyObject(PyObject *value)
{
    // check container of this property to notify about changes
    Part2DObject* part2d = dynamic_cast<Part2DObject*>(this->getContainer());

    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<Geometry*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(GeometryPy::Type))) {
                std::string error = std::string("types in list must be 'Geometry', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<GeometryPy*>(item)->getGeometryPtr();
        }

        setValues(values);
        if (part2d)
            part2d->acceptGeometry();
    }
    else if (PyObject_TypeCheck(value, &(GeometryPy::Type))) {
        GeometryPy  *pcObject = static_cast<GeometryPy*>(value);
        setValue(pcObject->getGeometryPtr());
        if (part2d)
            part2d->acceptGeometry();
    }
    else {
        std::string error = std::string("type must be 'Geometry' or list of 'Geometry', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyGeometryList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<GeometryList count=\"" << getSize() <<"\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<Geometry  type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << endl;;
        writer.incInd();
        _lValueList[i]->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</Geometry>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</GeometryList>" << endl ;
}

void PropertyGeometryList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.clearPartialRestoreObject();
    reader.readElement("GeometryList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    std::vector<Geometry*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("Geometry");
        const char* TypeName = reader.getAttribute("type");
        Geometry *newG = (Geometry *)Base::Type::fromName(TypeName).createInstance();
        newG->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("Geometry \"%s\" within a PropertyGeometryList was subject to a partial restore.\n",reader.localName());
            if(isOrderRelevant()) {
                // Pushes the best try by the Geometry class
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

        reader.readEndElement("Geometry");
    }

    reader.readEndElement("GeometryList");

    // assignment
    setValues(values);
}

App::Property *PropertyGeometryList::Copy(void) const
{
    PropertyGeometryList *p = new PropertyGeometryList();
    p->setValues(_lValueList);
    return p;
}

void PropertyGeometryList::Paste(const Property &from)
{
    const PropertyGeometryList& FromList = dynamic_cast<const PropertyGeometryList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyGeometryList::getMemSize(void) const
{
    int size = sizeof(PropertyGeometryList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
