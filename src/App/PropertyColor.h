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

#ifndef APP_PROPERTYCOLOR_H
#define APP_PROPERTYCOLOR_H

#include <App/Color.h>

#include "Property.h"


namespace App {

/** Color properties
 * This is the father of all properties handling colors.
 */
class AppExport PropertyColor: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
    * A constructor.
    * A more elaborate description of the constructor.
    */
    PropertyColor();

    /**
    * A destructor.
    * A more elaborate description of the destructor.
    */
    ~PropertyColor() override;

    /** Sets the property
    */
    void setValue(const Color& col);
    void setValue(float r, float g, float b, float a = 1.0F);
    void setValue(uint32_t rgba);

    /** This method returns a string representation of the property
    */
    const Color& getValue() const;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyColorItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(Color);
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
    Color _cCol;
};

class AppExport PropertyColorList: public PropertyListsT<Color>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
    * A constructor.
    * A more elaborate description of the constructor.
    */
    PropertyColorList();

    /**
    * A destructor.
    * A more elaborate description of the destructor.
    */
    ~PropertyColorList() override;

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

protected:
    Color getPyValue(PyObject* py) const override;
};

}  // namespace App

#endif
