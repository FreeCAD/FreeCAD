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

#ifndef APP_PROPERTYROTATION_H
#define APP_PROPERTYROTATION_H

#include <vector>
#include <boost/any/fwd.hpp>

#include <App/Property.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>


namespace Py {
class Object;
}

namespace Base {
class XMLReader;
class Writer;
}

namespace App {
class ObjectIdentifier;

/// Property representing a rotation
/*!
 * Encapsulates a Base::Rotation in a Property
 */
class AppExport PropertyRotation: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyRotation();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyRotation() override;

    /** Sets the property
     */
    void setValue(const Base::Rotation& rot);

    /** Sets property only if changed
     * @param pos: input placement
     * @param tol: position tolerance
     * @param atol: angular tolerance
     */
    bool setValueIfChanged(const Base::Rotation& rot, double atol = 1e-12);

    /** This method returns a string representation of the property
     */
    const Base::Rotation& getValue() const;

    /// Get valid paths for this property; used by auto completer
    void getPaths(std::vector<ObjectIdentifier>& paths) const override;

    void setPathValue(const ObjectIdentifier& path, const boost::any& value) override;

    const boost::any getPathValue(const ObjectIdentifier& path) const override;

    bool getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyRotationItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(Base::Placement);
    }

private:
    Base::Rotation _rot;
};

}  // namespace App

#endif
 