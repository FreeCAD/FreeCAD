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

#include "PropertyCenterLineList.h"
#include "CenterLinePy.h"


using namespace App;
using namespace Base;
using namespace TechDraw;

//**************************************************************************
// PropertyCenterLineList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(TechDraw::PropertyCenterLineList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction


PropertyCenterLineList::PropertyCenterLineList()
{

}

PropertyCenterLineList::~PropertyCenterLineList()
{
}

void PropertyCenterLineList::setSize(int newSize)
{
//    for (unsigned int i = newSize; i < _lValueList.size(); i++)
//        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyCenterLineList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyCenterLineList::setValue(CenterLine* lValue)
{
    if (lValue) {
        aboutToSetValue();
        _lValueList.resize(1);
        _lValueList[0] = lValue;
        hasSetValue();
    }
}

void PropertyCenterLineList::setValues(const std::vector<CenterLine*>& lValue)
{
    aboutToSetValue();
    _lValueList.resize(lValue.size());
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i];
    hasSetValue();
}

PyObject *PropertyCenterLineList::getPyObject()
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyCenterLineList::setPyObject(PyObject *value)
{
    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<CenterLine*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(CenterLinePy::Type))) {
                std::string error = std::string("types in list must be 'CenterLine', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<CenterLinePy*>(item)->getCenterLinePtr();
        }

        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(CenterLinePy::Type))) {
        CenterLinePy  *pcObject = static_cast<CenterLinePy*>(value);
        setValue(pcObject->getCenterLinePtr());
    }
    else {
        std::string error = std::string("type must be 'CenterLine' or list of 'CenterLine', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyCenterLineList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<CenterLineList count=\"" << getSize() << "\">"
                    << std::endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<CenterLine  type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << std::endl;
        writer.incInd();
        _lValueList[i]->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</CenterLine>" << std::endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</CenterLineList>" << std::endl;
}

void PropertyCenterLineList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.clearPartialRestoreObject();
    reader.readElement("CenterLineList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    std::vector<CenterLine*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("CenterLine");
        const char* TypeName = reader.getAttribute("type");
        CenterLine *newG = static_cast<CenterLine *>(Base::Type::fromName(TypeName).createInstance());
        newG->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("CenterLine \"%s\" within a PropertyCenterLineList was subject to a partial restore.\n", reader.localName());
            if(isOrderRelevant()) {
                // Pushes the best try by the CenterLine class
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

        reader.readEndElement("CenterLine");
    }

    reader.readEndElement("CenterLineList");

    // assignment
    setValues(values);
}

App::Property *PropertyCenterLineList::Copy() const
{
    PropertyCenterLineList *p = new PropertyCenterLineList();
    p->setValues(_lValueList);
    return p;
}

void PropertyCenterLineList::Paste(const Property &from)
{
    const PropertyCenterLineList& FromList = dynamic_cast<const PropertyCenterLineList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyCenterLineList::getMemSize() const
{
    int size = sizeof(PropertyCenterLineList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
