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


#ifndef APP_PropertyConstraintList_H
#define APP_PropertyConstraintList_H

// Std. configurations


#include <vector>
#include <string>
#include <App/Property.h>
#include <Mod/Part/App/Geometry.h>
#include "Constraint.h"

namespace Base {
class Writer;
}

namespace Sketcher
{
class Constraint;

class SketcherExport PropertyConstraintList : public App::PropertyLists
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyConstraintList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyConstraintList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    /** Sets the property
     */
    void setValue(const Constraint*);
    void setValues(const std::vector<Constraint*>&);

    /// index operator
    const Constraint *operator[] (const int idx) const {
        return invalidGeometry ? 0 : _lValueList[idx];
    }

    const std::vector<Constraint*> &getValues(void) const {
        return invalidGeometry ? _emptyValueList : _lValueList;
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save(Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const App::Property &from);

    virtual unsigned int getMemSize(void) const;

    void acceptGeometry(const std::vector<Part::Geometry *> &GeoList);
    void checkGeometry(const std::vector<Part::Geometry *> &GeoList);

private:
    std::vector<Constraint *> _lValueList;

    std::vector<unsigned int> validGeometryKeys;
    bool invalidGeometry;

    void applyValues(const std::vector<Constraint*>&);
    void applyValidGeometryKeys(const std::vector<unsigned int> &keys);

    static std::vector<Constraint *> _emptyValueList;
};

} // namespace Sketcher


#endif // APP_PropertyConstraintList_H
