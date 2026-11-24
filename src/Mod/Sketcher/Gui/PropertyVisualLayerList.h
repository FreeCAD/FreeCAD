// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2023 Abdullah Tahiri
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

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

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

protected:
    VisualLayer getPyValue(PyObject*) const override;
};

}  // namespace SketcherGui
