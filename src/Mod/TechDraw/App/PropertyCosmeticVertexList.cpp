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

#include "PropertyCosmeticVertexList.h"
#include "CosmeticVertexPy.h"


using namespace App;
using namespace Base;
using namespace TechDraw;

//**************************************************************************
// PropertyCosmeticVertexList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(TechDraw::PropertyCosmeticVertexList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyCosmeticVertexList::PropertyCosmeticVertexList()
{

}

PropertyCosmeticVertexList::~PropertyCosmeticVertexList()
{
}

void PropertyCosmeticVertexList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyCosmeticVertexList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyCosmeticVertexList::setValue(CosmeticVertex* lValue)
{
    if (lValue) {
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0] = lValue;
        hasSetValue();
    }
}

void PropertyCosmeticVertexList::setValues(const std::vector<CosmeticVertex*>& lValue)
{
    aboutToSetValue();
    _lValueList.resize(lValue.size());
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i];
    hasSetValue();
}

PyObject *PropertyCosmeticVertexList::getPyObject()
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
    writer.Stream() << writer.ind() << "<CosmeticVertexList count=\"" << getSize() << "\">"
                    << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<CosmeticVertex  type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << std::endl;
        writer.incInd();
        _lValueList[i]->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</CosmeticVertex>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</CosmeticVertexList>" << std::endl;
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
        CosmeticVertex *newG = static_cast<CosmeticVertex *>(Base::Type::fromName(TypeName).createInstance());
        newG->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("CosmeticVertex \"%s\" within a PropertyCosmeticVertexList was subject to a partial restore.\n", reader.localName());
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

App::Property *PropertyCosmeticVertexList::Copy() const
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

unsigned int PropertyCosmeticVertexList::getMemSize() const
{
    int size = sizeof(PropertyCosmeticVertexList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
