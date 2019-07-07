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

#include "Cosmetic.h"
#include "CosmeticVertexPy.h"

#include "PropertyCosmeticVertexList.h"


using namespace App;
using namespace Base;
using namespace std;
using namespace TechDraw;


//**************************************************************************
// PropertyCosmeticVertexList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(TechDraw::PropertyCosmeticVertexList, App::PropertyLists);

//**************************************************************************
// Construction/Destruction


PropertyCosmeticVertexList::PropertyCosmeticVertexList()
{

}

PropertyCosmeticVertexList::~PropertyCosmeticVertexList()
{
    for (std::vector<CosmeticVertex*>::iterator it = _lValueList.begin(); it != _lValueList.end(); ++it)
        if (*it) delete *it;
}

void PropertyCosmeticVertexList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyCosmeticVertexList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyCosmeticVertexList::setValue(const CosmeticVertex* lValue)
{
    if (lValue) {
        aboutToSetValue();
        CosmeticVertex* newVal = lValue->clone();
        for (unsigned int i = 0; i < _lValueList.size(); i++)
            delete _lValueList[i];
        _lValueList.resize(1);
        _lValueList[0] = newVal;
        hasSetValue();
    }
}

void PropertyCosmeticVertexList::setValues(const std::vector<CosmeticVertex*>& lValue)
{
    aboutToSetValue();
    std::vector<CosmeticVertex*> oldVals(_lValueList);
    _lValueList.resize(lValue.size());
    // copy all objects
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i]->clone();
    for (unsigned int i = 0; i < oldVals.size(); i++)
        delete oldVals[i];
    hasSetValue();
}

PyObject *PropertyCosmeticVertexList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyCosmeticVertexList::setPyObject(PyObject *value)
{
    // check container of this property to notify about changes

    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<CosmeticVertex*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(CosmeticVertexPy::Type))) {
                std::string error = std::string("types in list must be 'CosmeticVertex', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<CosmeticVertexPy*>(item)->getCosmeticVertexPtr();
        }

        setValues(values);
//        if (part2d)
//            part2d->acceptCosmeticVertex();
    }
    else if (PyObject_TypeCheck(value, &(CosmeticVertexPy::Type))) {
        CosmeticVertexPy  *pcObject = static_cast<CosmeticVertexPy*>(value);
        setValue(pcObject->getCosmeticVertexPtr());
    }
    else {
        std::string error = std::string("type must be 'CosmeticVertex' or list of 'CosmeticVertex', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyCosmeticVertexList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<CosmeticVertexList count=\"" << getSize() <<"\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<CosmeticVertex  type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << endl;
        writer.incInd();
        _lValueList[i]->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</CosmeticVertex>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</CosmeticVertexList>" << endl ;
}

void PropertyCosmeticVertexList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.clearPartialRestoreObject();
    reader.readElement("CosmeticVertexList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    std::vector<CosmeticVertex*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("CosmeticVertex");
        const char* TypeName = reader.getAttribute("type");
        CosmeticVertex *newG = (CosmeticVertex *)Base::Type::fromName(TypeName).createInstance();
        newG->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("CosmeticVertex \"%s\" within a PropertyCosmeticVertexList was subject to a partial restore.\n",reader.localName());
            if(isOrderRelevant()) {
                // Pushes the best try by the CosmeticVertex class
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

        reader.readEndElement("CosmeticVertex");
    }

    reader.readEndElement("CosmeticVertexList");

    // assignment
    setValues(values);
}

App::Property *PropertyCosmeticVertexList::Copy(void) const
{
    PropertyCosmeticVertexList *p = new PropertyCosmeticVertexList();
    p->setValues(_lValueList);
    return p;
}

void PropertyCosmeticVertexList::Paste(const Property &from)
{
    const PropertyCosmeticVertexList& FromList = dynamic_cast<const PropertyCosmeticVertexList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyCosmeticVertexList::getMemSize(void) const
{
    int size = sizeof(PropertyCosmeticVertexList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
