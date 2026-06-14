// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Wandererfan <WandererFan@gmail.com>                *
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

#pragma once

#include <Mod/Part/PartGlobal.h>

#include <vector>

#include <App/Property.h>


#include "TopoShape.h"


namespace Base
{
class Writer;
class XMLReader;
}  // namespace Base

namespace Part
{
class TopoShape;

class PartExport PropertyTopoShapeList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyTopoShapeList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyTopoShapeList() override;

    void setSize(int newSize) override;
    int getSize() const override;

    /** Sets the property
     */
    void setValue();
    void setValue(const TopoShape& ts);
    void setValues(const std::vector<TopoShape>&);
    void clear();

    /// index operator
    const TopoShape& operator[](const int idx) const
    {
        return _lValueList[idx];
    }

    const std::vector<TopoShape>& getValues() const
    {
        return _lValueList;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;

    unsigned int getMemSize() const override;

    void afterRestore() override;

private:
    std::vector<TopoShape> _lValueList;

    // holds the new TopoShapes between their creation in Restore, and the completion of all
    // individual RestoreDocFile calls.  Once the restore is complete, the new TopoShapes are
    // inserted into _lValueList;
    using TopoShapePtr = std::shared_ptr<TopoShape>;
    std::vector<TopoShapePtr> m_restorePointers;
};

}  // namespace Part
