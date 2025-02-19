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

#ifndef APP_PROPERTYUUID_H
#define APP_PROPERTYUUID_H

#include <Base/Uuid.h>

#include "Property.h"

namespace App {

/** UUID properties
 * This property handles unique identifiers
*/
class AppExport PropertyUUID: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
    * A constructor.
    * A more elaborate description of the constructor.
    */
    PropertyUUID();

    /**
    * A destructor.
    * A more elaborate description of the destructor.
    */
    ~PropertyUUID() override;


    void setValue(const Base::Uuid&);
    void setValue(const char* sString);
    void setValue(const std::string& sString);
    const std::string& getValueStr() const;
    const Base::Uuid& getValue() const;

    // virtual const char* getEditorName(void) const { return
    // "Gui::PropertyEditor::PropertyStringItem"; }
    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && _uuid.getValue() == static_cast<decltype(this)>(&other)->_uuid.getValue();
    }

private:
    Base::Uuid _uuid;
};

}  // namespace App

#endif
