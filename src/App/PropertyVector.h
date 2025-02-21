/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef APP_PROPERTYVECTOR_H
#define APP_PROPERTYVECTOR_H

#include <Base/Vector3D.h>
#include <Base/Unit.h>
#include <FCGlobal.h>

#include "Property.h"


namespace Base
{
class Writer;
}

namespace App
{
class Placement;


/** Vector properties
 * This is the father of all properties handling Integers.
 */
class AppExport PropertyVector: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyVector();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyVector() override;

    /** Sets the property
     */
    void setValue(const Base::Vector3d& vec);
    void setValue(double x, double y, double z);

    /// Get valid paths for this property; used by auto completer
    void getPaths(std::vector<ObjectIdentifier>& paths) const override;

    /** This method returns a string representation of the property
     */
    const Base::Vector3d& getValue() const;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyVectorItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(Base::Vector3d);
    }

    const boost::any getPathValue(const ObjectIdentifier& path) const override;

    bool getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const override;

    virtual Base::Unit getUnit() const
    {
        return {};
    }

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == dynamic_cast<decltype(this)>(&other)->getValue();
    }

private:
    Base::Vector3d _cVec;
};


class AppExport PropertyVectorDistance: public PropertyVector
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyVectorDistance();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyVectorDistance() override;

    Base::Unit getUnit() const override
    {
        return Base::Unit::Length;
    }

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyVectorDistanceItem";
    }
};

class AppExport PropertyPosition: public PropertyVector
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyPosition();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyPosition() override;

    Base::Unit getUnit() const override
    {
        return Base::Unit::Length;
    }

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyPositionItem";
    }
};

class AppExport PropertyDirection: public PropertyVector
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyDirection();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyDirection() override;

    Base::Unit getUnit() const override
    {
        return Base::Unit::Length;
    }

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyDirectionItem";
    }
};

}  // namespace App

#endif  // APP_PROPERTYGEO_H
