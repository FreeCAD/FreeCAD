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

#ifndef APP_PROPERTYMATRIX_H
#define APP_PROPERTYMATRIX_H

#include <Base/Matrix.h>

#include "Property.h"

namespace Base {
class Wrtier;
class XMLReader;
}

namespace App {
 
/// Property representing a 4x4 matrix
/*!
 * Encapsulates a Base::Matrix4D in a Property
 */
class AppExport PropertyMatrix: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * Initialises to an identity matrix
     */
    PropertyMatrix();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyMatrix() override;

    /** Sets the property
     */
    void setValue(const Base::Matrix4D& mat);

    /** This method returns a string representation of the property
     */
    const Base::Matrix4D& getValue() const;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyMatrixItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(Base::Matrix4D);
    }

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

private:
    Base::Matrix4D _cMat;
};

}  // namespace App

#endif
