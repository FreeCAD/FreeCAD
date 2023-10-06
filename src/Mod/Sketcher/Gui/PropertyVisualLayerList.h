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

#ifndef SKETCHERGUI_PropertyVisualLayerList_H
#define SKETCHERGUI_PropertyVisualLayerList_H

#include <vector>

#include <App/Property.h>

#include <Mod/Sketcher/SketcherGlobal.h>

#include "VisualLayer.h"


namespace Base
{
class Writer;
}

namespace SketcherGui
{

class SketcherGuiExport PropertyVisualLayerList: public App::PropertyListsT<VisualLayer>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyVisualLayerList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyVisualLayerList() override;

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
    void Restore(Base::DocumentReader& reader,
                 XERCES_CPP_NAMESPACE_QUALIFIER DOMElement* containerEl) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

protected:
    VisualLayer getPyValue(PyObject*) const override;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_PropertyVisualLayerList_H
