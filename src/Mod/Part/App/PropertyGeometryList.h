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

#ifndef APP_PropertyGeometryList_H
#define APP_PropertyGeometryList_H

#include <vector>

#include <App/Property.h>

#include "Geometry.h"


namespace Base {
class Writer;
}

namespace Part
{
class Geometry;

class PartExport PropertyGeometryList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyGeometryList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyGeometryList() override;

    void setSize(int newSize) override;
    int getSize() const override;

    /** Sets the property
     */
    void setValue(const Geometry*);
    void setValues(const std::vector<Geometry*>&);
    void setValues(std::vector<Geometry*>&&);

    /// index operator
    const Geometry *operator[] (const int idx) const {
        return _lValueList[idx];
    }

    const std::vector<Geometry*> &getValues() const {
        return _lValueList;
    }

    void set1Value(int idx, std::unique_ptr<Geometry> &&);

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save(Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    App::Property *Copy() const override;
    void Paste(const App::Property &from) override;

    unsigned int getMemSize() const override;

private:
    void trySaveGeometry(Geometry * geom, Base::Writer &writer) const;
    void tryRestoreGeometry(Geometry * geom, Base::XMLReader &reader);

private:
    std::vector<Geometry*> _lValueList;
};

} // namespace Part


#endif // APP_PropertyGeometryList_H
