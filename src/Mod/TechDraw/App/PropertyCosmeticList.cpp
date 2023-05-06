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

#include "PropertyCosmeticList.h"
#include "CosmeticEdgePy.h"
#include "Cosmetic.h"
#include "CenterLine.h"


using namespace App;
using namespace Base;
using namespace TechDraw;

TYPESYSTEM_SOURCE(TechDraw::PropertyCosmeticList, App::PropertyLists)

void PropertyCosmeticList::setSize(int newSize)
{
//    for (unsigned int i = newSize; i < _lValueList.size(); i++)
//        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyCosmeticList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}


//_lValueList is not const. so why do we pass a const parameter?
void PropertyCosmeticList::setValue(Cosmetic* lValue)
{
    if (lValue) {
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0] = lValue;
        hasSetValue();
    }
}

void PropertyCosmeticList::setValues(const std::vector<Cosmetic*>& lValue)
{
    aboutToSetValue();
    _lValueList.resize(lValue.size());
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i];
    hasSetValue();
}

PyObject *PropertyCosmeticList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyCosmeticList::setPyObject(PyObject *value)
{
    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<Cosmetic*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(CosmeticEdgePy::Type))) {
                std::string error = std::string("types in list must be 'Cosmetic', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<CosmeticEdgePy*>(item)->getCosmeticEdgePtr();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(CosmeticEdgePy::Type))) {
        CosmeticEdgePy  *pcObject = static_cast<CosmeticEdgePy*>(value);
        setValue(pcObject->getCosmeticEdgePtr());
    }
    else {
        std::string error = std::string("type must be 'Cosmetic' or list of 'Cosmetic', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyCosmeticList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<CosmeticList count=\"" << getSize() << "\">"
                    << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<Cosmetic type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << std::endl;
        writer.incInd();
        
        _lValueList[i]->Save(writer);
    
        writer.decInd();
        writer.Stream() << writer.ind() << "</Cosmetic>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</CosmeticList>" << std::endl;
}

void PropertyCosmeticList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.clearPartialRestoreObject();
    reader.readElement("CosmeticList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    std::vector<Cosmetic*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("Cosmetic");
        const char* TypeName = reader.getAttribute("type");
        // using CosmeticType = Cosmetic*;
        // if (std::strcmp(TypeName, "CosmeticEdge") == 0) {
        //     CosmeticType = CosmeticEdge;
        // }
        // else if (std::strcmp(TypeName, "GeomFormat") == 0) {
        //     CosmeticType = GeomFormat;
        // }
        // else if (std::strcmp(TypeName, "CenterLine") == 0) {
        //     CosmeticType = CenterLine;
        // }
        // else { throw; }
        Cosmetic* newG = static_cast<Cosmetic*>(Base::Type::fromName(TypeName).createInstance());
        newG->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("Cosmetic \"%s\" within a PropertyCosmeticList was subject to a partial restore.\n", reader.localName());
            if(isOrderRelevant()) {
                // Pushes the best try by the Cosmetic class
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

        reader.readEndElement("CosmeticEdge");
    }

    reader.readEndElement("CosmeticList");

    // assignment
    setValues(values);
}

App::Property *PropertyCosmeticList::Copy() const
{
    PropertyCosmeticList *p = new PropertyCosmeticList();
    p->setValues(_lValueList);
    return p;
}

void PropertyCosmeticList::Paste(const Property &from)
{
    const PropertyCosmeticList& FromList = dynamic_cast<const PropertyCosmeticList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyCosmeticList::getMemSize() const
{
    int size = sizeof(PropertyCosmeticList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
