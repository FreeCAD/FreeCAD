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

#ifndef APP_PROPERTYSTRINGLIST_H
#define APP_PROPERTYSTRINGLIST_H

#include "Property.h"


namespace App {

class AppExport PropertyStringList: public PropertyListsT<std::string>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    using inherited = PropertyListsT<std::string>;

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyStringList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyStringList() override;

    void setValues(const std::list<std::string>&);
    using inherited::setValues;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyStringListItem";
    }

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override;

protected:
    std::string getPyValue(PyObject* item) const override;
};

}  // namespace App

#endif
