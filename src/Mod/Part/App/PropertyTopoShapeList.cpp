/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

#include <BRepBuilderAPI_Copy.hxx>

#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "TopoShapePy.h"

#include "PropertyTopoShapeList.h"

using namespace App;
using namespace Base;
using namespace std;
using namespace Part;


//**************************************************************************
// PropertyTopoShapeList
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE(Part::PropertyTopoShapeList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyTopoShapeList::PropertyTopoShapeList() = default;

PropertyTopoShapeList::~PropertyTopoShapeList() = default;

void PropertyTopoShapeList::setSize(int newSize)
{
    _lValueList.resize(newSize);
}

int PropertyTopoShapeList::getSize() const
{
    return static_cast<int>(_lValueList.size());
}

// this version of setValue is for the ADD_PROPERTY_TYPE macro to use when creating a
// new PropertyTopoShapeList.  In other list properties that use pointers (instead of
// references) a nullptr is passed to setValue to initialize the empty list, but we can
// not do that with references.
void PropertyTopoShapeList::setValue()
{
    // aboutToSetValue() and hasSetValue() are called in clear() so don't need
    // to be called here.
    clear();
}

void PropertyTopoShapeList::setValue(const TopoShape &ts)
{
    aboutToSetValue();
    _lValueList.resize(1);
    _lValueList[0] = ts;
    hasSetValue();
}

void PropertyTopoShapeList::setValues(const std::vector<TopoShape>& lValue)
{
    aboutToSetValue();
    _lValueList.resize(lValue.size());
    for (unsigned int i = 0; i < lValue.size(); i++) {
        _lValueList[i] = lValue[i];
    }
    hasSetValue();
}

// clean out the list
void PropertyTopoShapeList::clear()
{
    aboutToSetValue();
    _lValueList.clear();
    _lValueList.resize(0);
    hasSetValue();

}

// populate the lists with the TopoShapes that have now finished restoring
void PropertyTopoShapeList::afterRestore()
{
    aboutToSetValue();
    _lValueList.clear();
    for (auto& entry : m_restorePointers) {
        _lValueList.push_back(*entry);
    }
    hasSetValue();

    m_restorePointers.clear();
    App::PropertyLists::afterRestore();
}

PyObject *PropertyTopoShapeList::getPyObject()
{
    Py::List list;
    for (int i = 0; i < getSize(); i++) {
        list.append(Py::asObject(_lValueList[i].getPyObject()));
    }
    return Py::new_reference_to(list);
}

void PropertyTopoShapeList::setPyObject(PyObject *value)
{
    if (PySequence_Check(value)) {
        Py::Sequence sequence(value);
        Py_ssize_t nSize = sequence.size();
        std::vector<TopoShape> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i < nSize; ++i) {
            Py::Object item = sequence.getItem(i);
            if (!PyObject_TypeCheck(item.ptr(), &(TopoShapePy::Type))) {
                std::string error = std::string("types in list must be 'Shape', not ");
                error += item.ptr()->ob_type->tp_name;
                throw Base::TypeError(error);
            }

            values[i] = *static_cast<TopoShapePy*>(item.ptr())->getTopoShapePtr();
        }
        setValues(values);
    }
    else if (PyObject_TypeCheck(value, &(TopoShapePy::Type))) {
        TopoShapePy  *pcObject = static_cast<TopoShapePy*>(value);
        setValue(*pcObject->getTopoShapePtr());
    }
    else {
        std::string error = std::string("type must be 'Shape' or list of 'Shape', not ");
        error += value->ob_type->tp_name;
        throw Base::TypeError(error);
    }
}

void PropertyTopoShapeList::Save(Writer& writer) const
{
    writer.Stream() << writer.ind() << "<ShapeList count=\"" << getSize() << "\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        bool binary = writer.getMode("BinaryBrep");
        writer.Stream() << writer.ind() << "<TopoShape";
        if (!writer.isForceXML()) {
            // See SaveDocFile(), RestoreDocFile()
            //  add a filename to the writer's list.  Each file on the list is eventually
            //  processed by SaveDocFile().
            std::string ext(".");
            ext += std::to_string(i);
            if (binary) {
                ext += ".bin";
            }
            else {
                ext += ".brp";
            }
            writer.Stream() << writer.ind() << " file=\""
                            << writer.addFile(getFileName(ext.c_str()).c_str(), this) << "\"/>\n";
        }
        else if (binary) {
            writer.Stream() << " binary=\"1\">\n";
            _lValueList[i].exportBinary(writer.beginCharStream());
            writer.endCharStream() << writer.ind() << "</TopoShape>\n";
        }
        else {
            writer.Stream() << " brep=\"1\">\n";
            _lValueList[i].exportBrep(writer.beginCharStream() << '\n');
            writer.endCharStream() << '\n' << writer.ind() << "</TopoShape>\n";
        }
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</ShapeList>" << endl;
}

void PropertyTopoShapeList::SaveDocFile(Base::Writer& writer) const
{
    Base::FileInfo finfo(writer.ObjectName);
    bool binary = finfo.hasExtension("bin");
    int index = atoi(Base::FileInfo(finfo.fileNamePure()).extension().c_str());
    if (index < 0 || index >= static_cast<int>(_lValueList.size())) {
        return;
    }

    const TopoShape& shape = _lValueList[index];
    if (binary) {
        shape.exportBinary(writer.Stream());
    }
    else {
        shape.exportBrep(writer.Stream());
    }
}

void PropertyTopoShapeList::Restore(Base::XMLReader& reader)
{
    reader.readElement("ShapeList");
    int count = reader.getAttribute<long>("count");
    m_restorePointers.clear();  // just in case
    m_restorePointers.reserve(count);
    for (int i = 0; i < count; i++) {
        auto newShape = std::make_shared<TopoShape>();
        reader.readElement("TopoShape");
        std::string file(reader.getAttribute<const char*>("file"));
        if (!file.empty()) {
            reader.addFile(file.c_str(), this);
        }
        else if (reader.hasAttribute("binary") && reader.getAttribute<bool>("binary")) {
            newShape->importBinary(reader.beginCharStream());
        }
        else if (reader.hasAttribute("brep") && reader.getAttribute<bool>("brep")) {
            newShape->importBrep(reader.beginCharStream());
        }
        m_restorePointers.push_back(newShape);
    }
    reader.readEndElement("ShapeList");
}

void PropertyTopoShapeList::RestoreDocFile(Base::Reader& reader)
{
    Base::FileInfo finfo(reader.getFileName());
    bool binary = finfo.hasExtension("bin");
    int index = atoi(Base::FileInfo(finfo.fileNamePure()).extension().c_str());
    if (index < 0 || index >= static_cast<int>(m_restorePointers.size())) {
        return;
    }
    if (binary) {
        m_restorePointers[index]->importBinary(reader);
    }
    else {
        m_restorePointers[index]->importBrep(reader);
    }
}

App::Property *PropertyTopoShapeList::Copy() const
{
    PropertyTopoShapeList *p = new PropertyTopoShapeList();
    std::vector<TopoShape> copiedShapes;
    for (auto& shape : _lValueList) {
        BRepBuilderAPI_Copy copy(shape.getShape());
        copiedShapes.emplace_back(copy.Shape());
    }
    p->setValues(copiedShapes);
    return p;
}

void PropertyTopoShapeList::Paste(const Property &from)
{
    const PropertyTopoShapeList& FromList = dynamic_cast<const PropertyTopoShapeList&>(from);
    setValues(FromList._lValueList);
}

unsigned int PropertyTopoShapeList::getMemSize() const
{
    int size = sizeof(PropertyTopoShapeList);
    for (int i = 0; i < getSize(); i++)
        size += _lValueList[i].getMemSize();
    return size;
}

