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
#include "CosmeticEdgePy.h"

#include "PropertyCosmeticEdgeList.h"


using namespace App;
using namespace Base;
using namespace std;
using namespace TechDraw;


//**************************************************************************
// PropertyCosmeticEdgeList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(TechDraw::PropertyCosmeticEdgeList, App::PropertyLists);

//**************************************************************************
// Construction/Destruction


PropertyCosmeticEdgeList::PropertyCosmeticEdgeList()
{

}

PropertyCosmeticEdgeList::~PropertyCosmeticEdgeList()
{
    for (std::vector<CosmeticEdge*>::iterator it = _lValueList.begin(); it != _lValueList.end(); ++it)
        if (*it) delete *it;
}

void PropertyCosmeticEdgeList::setSize(int newSize)
{
    for (unsigned int i = newSize; i < _lValueList.size(); i++)
        delete _lValueList[i];
    _lValueList.resize(newSize);
}

int PropertyCosmeticEdgeList::getSize(void) const
{
    return static_cast<int>(_lValueList.size());
}

void PropertyCosmeticEdgeList::setValue(const CosmeticEdge* lValue)
{
    if (lValue) {
        aboutToSetValue();
        CosmeticEdge* newVal = lValue->clone();
        for (unsigned int i = 0; i < _lValueList.size(); i++)
            delete _lValueList[i];
        _lValueList.resize(1);
        _lValueList[0] = newVal;
        hasSetValue();
    }
}

void PropertyCosmeticEdgeList::setValues(const std::vector<CosmeticEdge*>& lValue)
{
    aboutToSetValue();
    std::vector<CosmeticEdge*> oldVals(_lValueList);
    _lValueList.resize(lValue.size());
    // copy all objects
    for (unsigned int i = 0; i < lValue.size(); i++)
        _lValueList[i] = lValue[i]->clone();
    for (unsigned int i = 0; i < oldVals.size(); i++)
        delete oldVals[i];
    hasSetValue();
}

PyObject *PropertyCosmeticEdgeList::getPyObject(void)
{
    PyObject* list = PyList_New(getSize());
    for (int i = 0; i < getSize(); i++)
        PyList_SetItem( list, i, _lValueList[i]->getPyObject());
    return list;
}

void PropertyCosmeticEdgeList::setPyObject(PyObject *value)
{
    // check container of this property to notify about changes
//    Part2DObject* part2d = dynamic_cast<Part2DObject*>(this->getContainer());

    if (PySequence_Check(value)) {
        Py_ssize_t nSize = PySequence_Size(value);
        std::vector<CosmeticEdge*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            PyObject* item = PySequence_GetItem(value, i);
            if (!PyObject_TypeCheck(item, &(CosmeticEdgePy::Type))) {
                std::string error = std::string("types in list must be 'CosmeticEdge', not ");
                error += item->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = static_cast<CosmeticEdgePy*>(item)->getCosmeticEdgePtr();
        }

        setValues(values);
//        if (part2d)
//            part2d->acceptCosmeticEdge();
    }
    else if (PyObject_TypeCheck(value, &(CosmeticEdgePy::Type))) {
        CosmeticEdgePy  *pcObject = static_cast<CosmeticEdgePy*>(value);
        setValue(pcObject->getCosmeticEdgePtr());
    }
    else {
        std::string error = std::string("type must be 'CosmeticEdge' or list of 'CosmeticEdge', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyCosmeticEdgeList::Save(Writer &writer) const
{
    writer.Stream() << writer.ind() << "<CosmeticEdgeList count=\"" << getSize() <<"\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        writer.Stream() << writer.ind() << "<CosmeticEdge  type=\""
                        << _lValueList[i]->getTypeId().getName() << "\">" << endl;
        writer.incInd();
        _lValueList[i]->Save(writer);
        writer.decInd();
        writer.Stream() << writer.ind() << "</CosmeticEdge>" << endl;
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</CosmeticEdgeList>" << endl ;
}

void PropertyCosmeticEdgeList::Restore(Base::XMLReader &reader)
{
    // read my element
    reader.clearPartialRestoreObject();
    reader.readElement("CosmeticEdgeList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");
    std::vector<CosmeticEdge*> values;
    values.reserve(count);
    for (int i = 0; i < count; i++) {
        reader.readElement("CosmeticEdge");
        const char* TypeName = reader.getAttribute("type");
        CosmeticEdge *newG = (CosmeticEdge *)Base::Type::fromName(TypeName).createInstance();
        newG->Restore(reader);

        if(reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInObject)) {
            Base::Console().Error("CosmeticEdge \"%s\" within a PropertyCosmeticEdgeList was subject to a partial restore.\n",reader.localName());
            if(isOrderRelevant()) {
                // Pushes the best try by the CosmeticEdge class
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

    reader.readEndElement("CosmeticEdgeList");

    // assignment
    setValues(values);
}

App::Property *PropertyCosmeticEdgeList::Copy(void) const
{
    PropertyCosmeticEdgeList *p = new PropertyCosmeticEdgeList();
    p->setValues(_lValueList);
    return p;
}

void PropertyCosmeticEdgeList::Paste(const Property &from)
{
    const PropertyCosmeticEdgeList& FromList = dynamic_cast<const PropertyCosmeticEdgeList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyCosmeticEdgeList::getMemSize(void) const
{
    int size = sizeof(PropertyCosmeticEdgeList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i]->getMemSize();
    return size;
}
