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

#ifndef APP_PROPERTYENUMERATION_H
#define APP_PROPERTYENUMERATION_H

#include "Enumeration.h"
#include "Property.h"


namespace App {

/// Property wrapper around an Enumeration object.
class AppExport PropertyEnumeration: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /// Standard constructor
    PropertyEnumeration();

    /// Obvious constructor
    explicit PropertyEnumeration(const Enumeration& e);

    /// destructor
    ~PropertyEnumeration() override;

    /// Enumeration methods
    /*!
     * These all function as per documentation in Enumeration
     */
    //@{
    /** setting the enumeration string list
     * The list is a NULL terminated array of pointers to a const char* string
     * \code
     * const char enums[] = {"Black","White","Other",NULL}
     * \endcode
     */
    void setEnums(const char** plEnums);

    /** setting the enumeration string as vector of strings
     * This makes the enumeration custom.
     */
    void setEnums(const std::vector<std::string>& Enums);

    /** set the enum by a string
     * is slower than setValue(long). Use long if possible
     */
    void setValue(const char* value);

    /** set directly the enum value
     * In DEBUG checks for boundaries.
     * Is faster than using setValue(const char*).
     */
    void setValue(long);

    /// Setter using Enumeration
    void setValue(const Enumeration& source);

    /// Returns current value of the enumeration as an integer
    long getValue() const;

    /// checks if the property is set to a certain string value
    bool isValue(const char* value) const;

    /// checks if a string is included in the enumeration
    bool isPartOf(const char* value) const;

    /// get the value as string
    const char* getValueAsString() const;

    /// Returns Enumeration object
    const Enumeration& getEnum() const;

    /// get all possible enum values as vector of strings
    std::vector<std::string> getEnumVector() const;

    /// set enum values as vector of strings
    void setEnumVector(const std::vector<std::string>&);
    /// get the pointer to the enum list
    bool hasEnums() const;

    /// Returns true if the instance is in a usable state
    bool isValid() const;
    //@}

    const char* getEditorName() const override
    {
        return _editorTypeName.c_str();
    }
    void setEditorName(const char* name)
    {
        _editorTypeName = name;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    void setPathValue(const App::ObjectIdentifier& path, const boost::any& value) override;
    virtual bool setPyPathValue(const App::ObjectIdentifier& path, const Py::Object& value);
    const boost::any getPathValue(const App::ObjectIdentifier& /*path*/) const override;
    bool getPyPathValue(const ObjectIdentifier& path, Py::Object& r) const override;

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getEnum() == static_cast<decltype(this)>(&other)->getEnum();
    }

private:
    Enumeration _enum;
    std::string _editorTypeName;
};

}  // namespace App

#endif
