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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <vector>
#include <App/Property.h>


namespace Base {
class Writer;
}

namespace TechDraw
{
class CosmeticEdge;

class TechDrawExport PropertyCosmeticEdgeList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyCosmeticEdgeList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyCosmeticEdgeList() override;

    void setSize(int newSize) override;
    int getSize(void) const override;

    /** Sets the property
     */
/*    void setValue(const CosmeticEdge*);*/
    void setValue(CosmeticEdge*);
    void setValues(const std::vector<CosmeticEdge*>&);

    /// index operator
    const CosmeticEdge *operator[] (const int idx) const {
        return _lValueList[idx];
    }

    const std::vector<CosmeticEdge*> &getValues() const {
        return _lValueList;
    }

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save(Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    App::Property *Copy() const override;
    void Paste(const App::Property &from) override;

    unsigned int getMemSize(void) const override;

private:
    std::vector<CosmeticEdge*> _lValueList;
};

} // namespace TechDraw