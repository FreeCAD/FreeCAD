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

#include "PropertyConstraintList.h"
#include "ConstraintPy.h"

using namespace App;
using namespace Base;
using namespace std;
using namespace Sketcher;


//**************************************************************************
// PropertyConstraintList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(Sketcher::PropertyConstraintList, App::PropertyLists);

//**************************************************************************
// Construction/Destruction


PropertyConstraintList::PropertyConstraintList() : validGeometryKeys(0), invalidGeometry(true)
{

}

PropertyConstraintList::~PropertyConstraintList()
{
    for (std::vector<Constraint*>::iterator it = _lValueList.begin(); it != _lValueList.end(); ++it)
        if (*it) delete *it;
}

void PropertyConstraintList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyConstraintList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyConstraintList::setValue(const Constraint* lValue)
{
    if (lValue) {
        aboutToSetValue();
        Constraint* newVal = lValue->clone();
        for (unsigned int i = 0; i < _lValueList.size(); i++)
            delete _lValueList[i];
        _lValueList.resize(1);
        _lValueList[0] = newVal;
        hasSetValue();
    }
}

void PropertyConstraintList::setValues(const std::vector<Constraint*>& lValue)
{
    aboutToSetValue();
    applyValues(lValue);
    hasSetValue();
}

void PropertyConstraintList::applyValues(const std::vector<Constraint*>& lValue)
{
    std::vector<Constraint*> oldVals(_lValueList);
    _lValueList.resize(lValue.size());
    // copy all objects
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i]->clone();
    for (unsigned int i = 0; i < oldVals.size(); i++)
        delete oldVals[i];
}

PyObject *PropertyConstraintList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyConstraintList::setPyObject(PyObject *value)
{
    if (PyList_Check(value)) {
        Py_ssize_t nSize = PyList_Size(value);
        std::vector<Constraint*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PyList_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(ConstraintPy::Type))) {
                std::string error = std::string("types in list must be 'Constraint', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<ConstraintPy*>(item)->getConstraintPtr();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(ConstraintPy::Type))) {
        ConstraintPy *pcObject = static_cast<ConstraintPy*>(value);
        setValue(pcObject->getConstraintPtr());
    }
    else {
        std::string error = std::string("type must be 'Constraint' or list of 'Constraint', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyConstraintList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<ConstraintList count=\"" << getSize() <<"\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++)
        _lValueList[i]->Save(writer);
    writer.decInd();
    writer.Stream() << writer.ind() << "</ConstraintList>" << endl ;
}

void PropertyConstraintList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.readElement("ConstraintList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<Constraint*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        Constraint *newC = new Constraint();
        newC->Restore(reader);
        values.push_back(newC);
    }

    reader.readEndElement("ConstraintList");

    // assignment
    setValues(values);
}

Property *PropertyConstraintList::Copy(void) const
{
    PropertyConstraintList *p = new PropertyConstraintList();
    p->applyValidGeometryKeys(validGeometryKeys);
    p->applyValues(_lValueList);
    return p;
}

void PropertyConstraintList::Paste(const Property &from)
{
    const PropertyConstraintList& FromList = dynamic_cast<const PropertyConstraintList&>(from);
    aboutToSetValue();
    applyValues(FromList._lValueList);
    applyValidGeometryKeys(FromList.validGeometryKeys);
    hasSetValue();
}

unsigned int PropertyConstraintList::getMemSize(void) const
{
    int size = sizeof(PropertyConstraintList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}

void PropertyConstraintList::acceptGeometry(const std::vector<Part::Geometry *> &GeoList)
{
    aboutToSetValue();
    validGeometryKeys.clear();
    validGeometryKeys.reserve(GeoList.size());
    for (std::vector< Part::Geometry * >::const_iterator it=GeoList.begin();
         it != GeoList.end(); ++it)
        validGeometryKeys.push_back((*it)->getTypeId().getKey());
    invalidGeometry = false;
    hasSetValue();
}

void PropertyConstraintList::applyValidGeometryKeys(const std::vector<unsigned int> &keys)
{
    validGeometryKeys = keys;
}

void PropertyConstraintList::checkGeometry(const std::vector<Part::Geometry *> &GeoList)
{
    if (validGeometryKeys.size() != GeoList.size()) {
        invalidGeometry = true;
        return;
    }

    unsigned int i=0;
    for (std::vector< Part::Geometry * >::const_iterator it=GeoList.begin();
         it != GeoList.end(); ++it, i++) {
        if (validGeometryKeys[i] != (*it)->getTypeId().getKey()) {
            invalidGeometry = true;
            return;
        }
    }

    if (invalidGeometry) {
        invalidGeometry = false;
        touch();
    }
}

std::vector<Constraint *> PropertyConstraintList::_emptyValueList(0);
