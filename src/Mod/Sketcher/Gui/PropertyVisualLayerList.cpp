/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com      *
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
#include <Base/DocumentReader.h>
#include <Base/Interpreter.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "PropertyVisualLayerList.h"


using namespace App;
using namespace Base;
using namespace std;
using namespace SketcherGui;


TYPESYSTEM_SOURCE(SketcherGui::PropertyVisualLayerList, App::PropertyLists)

//**************************************************************************
// Construction/Destruction

PropertyVisualLayerList::PropertyVisualLayerList() = default;

PropertyVisualLayerList::~PropertyVisualLayerList() = default;

//**************************************************************************
// Base class implementer

PyObject* PropertyVisualLayerList::getPyObject()
{
    THROWM(Base::NotImplementedError, "PropertyVisualLayerList has no python counterpart");
}

VisualLayer PropertyVisualLayerList::getPyValue(PyObject* item) const
{

    (void)item;
    THROWM(Base::NotImplementedError, "PropertyVisualLayerList has no python counterpart");
}

void PropertyVisualLayerList::Save(Base::Writer& writer) const
{
    writer.Stream() << writer.ind() << "<VisualLayerList count=\"" << getSize() << "\">" << endl;
    writer.incInd();
    for (int i = 0; i < getSize(); i++) {
        _lValueList[i].Save(writer);
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</VisualLayerList>" << endl;
}

void PropertyVisualLayerList::Restore(Base::XMLReader& reader)
{
    reader.readElement("VisualLayerList");
    // get the value of my attribute
    int count = reader.getAttributeAsInteger("count");

    std::vector<VisualLayer> layers;
    layers.reserve(count);

    for (int i = 0; i < count; i++) {
        VisualLayer visualLayer;
        visualLayer.Restore(reader);
        layers.push_back(std::move(visualLayer));
    }

    reader.readEndElement("VisualLayerList");

    setValues(std::move(layers));
}

void PropertyVisualLayerList::Restore(Base::DocumentReader& reader,
                                      XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* containerEl)
{
    auto Prop_VisualLayerListDOM =
        reader.FindElementByField(containerEl, "Property", "name", "VisualLayerList");
    if (Prop_VisualLayerListDOM) {
        auto VisualLayerListDOM = reader.FindElement(Prop_VisualLayerListDOM, "VisualLayerList");
        const char* count_cstr = reader.GetAttribute(VisualLayerListDOM, "count");
        if (count_cstr) {
            int count = reader.ContentToInt(count_cstr);
            std::vector<VisualLayer> layers;
            layers.reserve(count);

            auto prev_VisualLayerDOM = reader.FindElement(VisualLayerListDOM, "VisualLayer");
            VisualLayer visualLayer;
            visualLayer.Restore(reader, prev_VisualLayerDOM);
            layers.push_back(std::move(visualLayer));
            for (int i = 1; i < count; i++) {
                auto VisualLayerDOM_i = reader.FindNextElement(prev_VisualLayerDOM, "VisualLayer");
                VisualLayer visualLayer;
                visualLayer.Restore(reader, VisualLayerDOM_i);
                layers.push_back(std::move(visualLayer));
                prev_VisualLayerDOM = VisualLayerDOM_i;
            }

            setValues(std::move(layers));
        }
    }
}

Property* PropertyVisualLayerList::Copy() const
{
    PropertyVisualLayerList* p = new PropertyVisualLayerList();
    p->_lValueList = _lValueList;
    return p;
}

void PropertyVisualLayerList::Paste(const Property& from)
{
    setValues(dynamic_cast<const PropertyVisualLayerList&>(from)._lValueList);
}

unsigned int PropertyVisualLayerList::getMemSize() const
{
    return static_cast<unsigned int>(_lValueList.size() * sizeof(VisualLayer));
}
