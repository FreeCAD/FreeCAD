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

#ifndef PropertyTopoShapeList_H
#define PropertyTopoShapeList_H

#include <Mod/Part/PartGlobal.h>

#include <vector>

#include <App/Property.h>


#include  "TopoShape.h"


namespace Base {
class Writer;
class XMLReader;
}

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
    void setValue(TopoShape*);
    void setValues(const std::vector<TopoShape*>&);

    /// index operator
    const TopoShape* operator[] (const int idx) const {
        return _lValueList[idx];
    }

    const std::vector<TopoShape*>& getValues() const {
        return _lValueList;
    }

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save(Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    App::Property *Copy() const override;
    void Paste(const App::Property &from) override;

    unsigned int getMemSize() const override;

private:
    std::vector<TopoShape*> _lValueList;
};

} // namespace Part


#endif // APP_PropertyTopoShapeList_H


