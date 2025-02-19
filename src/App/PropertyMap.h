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

#ifndef APP_PROPERTYMAP_H
#define APP_PROPERTYMAP_H

#include "Property.h"


namespace App {

/** implements a key/value list as property
 *  The key ought to be ASCII the Value should be treated as UTF8 to be saved.
 */
class AppExport PropertyMap: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
    * A constructor.
    * A more elaborate description of the constructor.
    */
    PropertyMap();

    /**
    * A destructor.
    * A more elaborate description of the destructor.
    */
    ~PropertyMap() override;

    virtual int getSize() const;

    /** Sets the property
    */
    void setValue()
    {}
    void setValue(const std::string& key, const std::string& value);
    void setValues(const std::map<std::string, std::string>&);

    /// index operator
    const std::string& operator[](const std::string& key) const;

    void set1Value(const std::string& key, const std::string& value)
    {
        _lValueList.operator[](key) = value;
    }

    const std::map<std::string, std::string>& getValues() const
    {
        return _lValueList;
    }

    // virtual const char* getEditorName(void) const { return
    // "Gui::PropertyEditor::PropertyStringListItem"; }

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
            && getValues() == static_cast<decltype(this)>(&other)->getValues();
    }

private:
    std::map<std::string, std::string> _lValueList;
};

}  // namespace App

#endif