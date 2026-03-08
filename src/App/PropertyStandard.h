// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include <list>
#include <memory>
#include <string>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <Base/Uuid.h>

#include "Property.h"
#include "Enumeration.h"
#include "Material.h"


namespace Base
{
class InputStream;
class OutputStream;
class Writer;
}  // namespace Base


namespace App
{

/** Integer properties
 * This is the father of all properties handling Integers.
 */
class AppExport PropertyInteger: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyInteger();
    ~PropertyInteger() override;

    /** Sets the property
     */
    void setValue(long);

    /** This method returns a string representation of the property
     */
    long getValue() const;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyIntegerItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(long);
    }

    void setPathValue(const App::ObjectIdentifier& path, const boost::any& value) override;
    const boost::any getPathValue(const App::ObjectIdentifier& /*path*/) const override
    {
        return _lValue;
    }

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

protected:
    long _lValue;
};

/** Path properties
 * Properties handling file system paths.
 */
class AppExport PropertyPath: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyPath();
    ~PropertyPath() override;

    /** Sets the property
     */
    void setValue(const std::filesystem::path&);

    /** Sets the property
     */
    void setValue(const char*);

    /** This method returns a string representation of the property
     */
    const std::filesystem::path& getValue() const;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyPathItem";
    }

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
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

protected:
    std::filesystem::path _cValue;
};

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

/** Constraint integer properties
 * This property fulfills the need of a constraint integer. It holds basically a
 * state (integer) and a struct of boundaries. If the boundaries
 * is not set it acts basically like an IntegerProperty and does no checking.
 * The constraints struct can be created on the heap or build in.
 */
class AppExport PropertyIntegerConstraint: public PropertyInteger
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /// Standard constructor
    PropertyIntegerConstraint();

    /// destructor
    ~PropertyIntegerConstraint() override;

    /// Constraint methods
    //@{
    /// the boundary struct
    struct Constraints
    {
        long LowerBound, UpperBound, StepSize;
        Constraints()
            : LowerBound(0)
            , UpperBound(0)
            , StepSize(0)
            , candelete(false)
        {}
        Constraints(long l, long u, long s)
            : LowerBound(l)
            , UpperBound(u)
            , StepSize(s)
            , candelete(false)
        {}
        ~Constraints() = default;
        void setDeletable(bool on)
        {
            candelete = on;
        }
        bool isDeletable() const
        {
            return candelete;
        }

    private:
        bool candelete;
    };
    /** setting the boundaries
     * This sets the constraint struct. It can be dynamically
     * allocated or set as a static in the class the property
     * belongs to:
     * \code
     * const Constraints percent = {0,100,1}
     * \endcode
     */
    void setConstraints(const Constraints* sConstraint);
    /// get the constraint struct
    const Constraints* getConstraints() const;
    //@}

    long getMinimum() const;
    long getMaximum() const;
    long getStepSize() const;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyIntegerConstraintItem";
    }
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

protected:
    const Constraints* _ConstStruct {nullptr};
};

/** Percent property
 * This property is a special integer property and holds only
 * numbers between 0 and 100.
 */

class AppExport PropertyPercent: public PropertyIntegerConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /// Standard constructor
    PropertyPercent();

    /// destructor
    ~PropertyPercent() override;
};

/** Integer list properties
 *
 */
class AppExport PropertyIntegerList: public PropertyListsT<long>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**

     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyIntegerList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyIntegerList() override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyIntegerListItem";
    }

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

protected:
    long getPyValue(PyObject* item) const override;
};

/** Integer list properties
 *
 */
class AppExport PropertyIntegerSet: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**

     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyIntegerSet();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyIntegerSet() override;

    /** Sets the property
     */
    void setValue(long);
    void setValue()
    {
        ;
    }

    void addValue(long value)
    {
        _lValueSet.insert(value);
    }
    void setValues(const std::set<long>& values);

    const std::set<long>& getValues() const
    {
        return _lValueSet;
    }

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
    std::set<long> _lValueSet;
};


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


/** Float properties
 * This is the father of all properties handling floats.
 * Use this type only in rare cases. Mostly you want to
 * use the more specialized types like e.g. PropertyLength.
 * These properties also fulfill the needs of the unit system.
 * See PropertyUnits.h for all properties with units.
 */
class AppExport PropertyFloat: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /** Value Constructor
     *  Construct with explicit Values
     */
    PropertyFloat();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyFloat() override;


    void setValue(double lValue);
    double getValue() const;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyFloatItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(double);
    }

    void setPathValue(const App::ObjectIdentifier& path, const boost::any& value) override;
    const boost::any getPathValue(const App::ObjectIdentifier& path) const override;

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

protected:
    double _dValue;
};

/** Constraint float properties
 * This property fulfills the need of a constraint float. It holds basically a
 * state (float) and a struct of boundaries. If the boundaries
 * is not set it acts basically like a PropertyFloat and does no checking
 * The constraints struct can be created on the heap or built-in.
 */
class AppExport PropertyFloatConstraint: public PropertyFloat
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /** Value Constructor
     *  Construct with explicit Values
     */
    PropertyFloatConstraint();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyFloatConstraint() override;


    /// Constraint methods
    //@{
    /// the boundary struct
    struct Constraints
    {
        double LowerBound, UpperBound, StepSize;
        Constraints()
            : LowerBound(0)
            , UpperBound(0)
            , StepSize(0)
            , candelete(false)
        {}
        Constraints(double l, double u, double s)
            : LowerBound(l)
            , UpperBound(u)
            , StepSize(s)
            , candelete(false)
        {}
        ~Constraints() = default;
        void setDeletable(bool on)
        {
            candelete = on;
        }
        bool isDeletable() const
        {
            return candelete;
        }

    private:
        bool candelete;
    };
    /** setting the boundaries
     * This sets the constraint struct. It can be dynamically
     * allocated or set as an static in the class the property
     * belongs to:
     * \code
     * const Constraints percent = {0.0,100.0,1.0}
     * \endcode
     */
    void setConstraints(const Constraints* sConstrain);
    /// get the constraint struct
    const Constraints* getConstraints() const;
    //@}

    double getMinimum() const;
    double getMaximum() const;
    double getStepSize() const;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyFloatConstraintItem";
    }

    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

protected:
    const Constraints* _ConstStruct {nullptr};
};


/** Precision properties
 * This property fulfills the need of a floating value with many decimal points,
 * e.g. for holding values like Precision::Confusion(). The value has a default
 * constraint for non-negative, but can be overridden
 */
class AppExport PropertyPrecision: public PropertyFloatConstraint
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyPrecision();
    ~PropertyPrecision() override;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyPrecisionItem";
    }
};


class AppExport PropertyFloatList: public PropertyListsT<double>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyFloatList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyFloatList() override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyFloatListItem";
    }

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

protected:
    double getPyValue(PyObject* item) const override;
};


/** String properties
 * This is the father of all properties handling Strings.
 */
class AppExport PropertyString: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyString();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyString() override;

    virtual void setValue(const char* sString);
    void setValue(const std::string& sString);
    const char* getValue() const;
    const std::string& getStrValue() const
    {
        return _cValue;
    }
    bool isEmpty()
    {
        return _cValue.empty();
    }

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyStringItem";
    }
    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

    void setPathValue(const App::ObjectIdentifier& path, const boost::any& value) override;
    const boost::any getPathValue(const App::ObjectIdentifier& path) const override;

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getStrValue() == static_cast<decltype(this)>(&other)->getStrValue();
    }

protected:
    std::string _cValue;
};

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


/** Property handling with font names.
 */
class AppExport PropertyFont: public PropertyString
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyFont();
    ~PropertyFont() override;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyFontItem";
    }

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }
};

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

/** Bool properties
 * This is the father of all properties handling booleans.
 */
class AppExport PropertyBool: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyBool();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyBool() override;

    void setValue(bool lValue);
    bool getValue() const;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyBoolItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(bool);
    }

    void setPathValue(const App::ObjectIdentifier& path, const boost::any& value) override;
    const boost::any getPathValue(const App::ObjectIdentifier& path) const override;

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

private:
    bool _lValue;
};

/** Bool list properties
 *
 */
class AppExport PropertyBoolList: public PropertyListsT<bool, boost::dynamic_bitset<>>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    using inherited = PropertyListsT<bool, boost::dynamic_bitset<>>;

public:
    PropertyBoolList();
    ~PropertyBoolList() override;

    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

protected:
    bool getPyValue(PyObject* py) const override;
};


/** Base::Color properties
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
    void setValue(const Base::Color& col);
    void setValue(float r, float g, float b, float a = 1.0F);
    void setValue(uint32_t rgba);

    /** This method returns a string representation of the property
     */
    const Base::Color& getValue() const;

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
        return sizeof(Base::Color);
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
    Base::Color _cCol;
};

class AppExport PropertyColorList: public PropertyListsT<Base::Color>
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
    Base::Color getPyValue(PyObject* py) const override;

private:
    bool requiresAlphaConversion {false}; // In 1.1 the handling of alpha was inverted
};


/** Material properties
 * This is the father of all properties handling colors.
 */
class AppExport PropertyMaterial: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyMaterial();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyMaterial() override;

    /** Sets the property
     */
    void setValue(const Material& mat);
    void setValue(const Base::Color& col);
    void setValue(float r, float g, float b, float a = 1.0F);
    void setValue(uint32_t rgba);
    void setAmbientColor(const Base::Color& col);
    void setAmbientColor(float r, float g, float b, float a = 1.0F);
    void setAmbientColor(uint32_t rgba);
    void setDiffuseColor(const Base::Color& col);
    void setDiffuseColor(float r, float g, float b, float a = 1.0F);
    void setDiffuseColor(uint32_t rgba);
    void setSpecularColor(const Base::Color& col);
    void setSpecularColor(float r, float g, float b, float a = 1.0F);
    void setSpecularColor(uint32_t rgba);
    void setEmissiveColor(const Base::Color& col);
    void setEmissiveColor(float r, float g, float b, float a = 1.0F);
    void setEmissiveColor(uint32_t rgba);
    void setShininess(float);
    void setTransparency(float);

    /** This method returns a string representation of the property
     */
    const Material& getValue() const;
    const Base::Color& getAmbientColor() const;
    const Base::Color& getDiffuseColor() const;
    const Base::Color& getSpecularColor() const;
    const Base::Color& getEmissiveColor() const;
    double getShininess() const;
    double getTransparency() const;

    PyObject* getPyObject() override;
    void setPyObject(PyObject* py) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    const char* getEditorName() const override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(_cMat);
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
    Material _cMat;
};

/** Material properties
 */
class AppExport PropertyMaterialList: public PropertyListsT<Material>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyMaterialList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyMaterialList() override;

    void setValue();
    void setValue(const std::vector<App::Material>& materials)
    {
        PropertyListsT<Material>::setValue(materials);
    }
    void
    setValues(const std::vector<App::Material>& newValues = std::vector<App::Material>()) override;
    void setValue(const Material& mat);
    void setValue(int index, const Material& mat);

    void setAmbientColor(const Base::Color& col);
    void setAmbientColor(float r, float g, float b, float a = 1.0F);
    void setAmbientColor(uint32_t rgba);
    void setAmbientColor(int index, const Base::Color& col);
    void setAmbientColor(int index, float r, float g, float b, float a = 1.0F);
    void setAmbientColor(int index, uint32_t rgba);

    void setDiffuseColor(const Base::Color& col);
    void setDiffuseColor(float r, float g, float b, float a = 1.0F);
    void setDiffuseColor(uint32_t rgba);
    void setDiffuseColor(int index, const Base::Color& col);
    void setDiffuseColor(int index, float r, float g, float b, float a = 1.0F);
    void setDiffuseColor(int index, uint32_t rgba);
    void setDiffuseColors(const std::vector<Base::Color>& colors);

    void setSpecularColor(const Base::Color& col);
    void setSpecularColor(float r, float g, float b, float a = 1.0F);
    void setSpecularColor(uint32_t rgba);
    void setSpecularColor(int index, const Base::Color& col);
    void setSpecularColor(int index, float r, float g, float b, float a = 1.0F);
    void setSpecularColor(int index, uint32_t rgba);

    void setEmissiveColor(const Base::Color& col);
    void setEmissiveColor(float r, float g, float b, float a = 1.0F);
    void setEmissiveColor(uint32_t rgba);
    void setEmissiveColor(int index, const Base::Color& col);
    void setEmissiveColor(int index, float r, float g, float b, float a = 1.0F);
    void setEmissiveColor(int index, uint32_t rgba);

    void setShininess(float);
    void setShininess(int index, float);

    void setTransparency(float);
    void setTransparency(int index, float);
    void setTransparencies(const std::vector<float>& transparencies);

    const Base::Color& getAmbientColor() const;
    const Base::Color& getAmbientColor(int index) const;

    const Base::Color& getDiffuseColor() const;
    const Base::Color& getDiffuseColor(int index) const;
    std::vector<Base::Color> getDiffuseColors() const;

    const Base::Color& getSpecularColor() const;
    const Base::Color& getSpecularColor(int index) const;

    const Base::Color& getEmissiveColor() const;
    const Base::Color& getEmissiveColor(int index) const;

    float getShininess() const;
    float getShininess(int index) const;

    float getTransparency() const;
    float getTransparency(int index) const;
    std::vector<float> getTransparencies() const;

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    const char* getEditorName() const override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

protected:
    Material getPyValue(PyObject* py) const override;

private:
    enum Format
    {
        Version_0,
        Version_1,
        Version_2,
        Version_3
    };

    void RestoreDocFileV0(uint32_t count, Base::Reader& reader);
    void RestoreDocFileV3(Base::Reader& reader);

    void writeString(Base::OutputStream& str, const std::string& value) const;
    void readString(Base::InputStream& str, std::string& value);

    void verifyIndex(int index) const;
    void setMinimumSizeOne();
    int resizeByOneIfNeeded(int index);
    void convertAlpha(std::vector<App::Material>& materials) const;

    Format formatVersion {Version_0};
    bool requiresAlphaConversion {false};  // In 1.1 the handling of alpha was inverted
};


/** Property for dynamic creation of a FreeCAD persistent object
 *
 * In Python, this property can be assigned a type string to create a dynamic FreeCAD
 * object, and then read back as the Python binding of the newly created object.
 */
class AppExport PropertyPersistentObject: public PropertyString
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    using inherited = PropertyString;

public:
    PyObject* getPyObject() override;
    void setValue(const char* type) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;
    unsigned int getMemSize() const override;

    std::shared_ptr<Base::Persistence> getObject() const
    {
        return _pObject;
    }

protected:
    std::shared_ptr<Base::Persistence> _pObject;
};

}  // namespace App
