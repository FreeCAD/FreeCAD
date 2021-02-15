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


#ifndef APP_PROPERTYSTANDARD_H
#define APP_PROPERTYSTANDARD_H

// Std. configurations


#include <memory>
#include <string>
#include <list>
#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <boost/filesystem/path.hpp>

#include <Base/Uuid.h>
#include "Enumeration.h"
#include "Property.h"
#include "Material.h"

namespace Base {
class Writer;
}


namespace App
{

/** Integer properties
 * This is the father of all properties handling Integers.
 */
class AppExport PropertyInteger: public Property
{
    TYPESYSTEM_HEADER();

public:
    PropertyInteger();
    virtual ~PropertyInteger();

    /** Sets the property
     */
    void setValue(long);

    /** This method returns a string representation of the property
     */
    long getValue(void) const;
    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyIntegerItem"; }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const{return sizeof(long);}

    virtual void setPathValue(const App::ObjectIdentifier & path, const boost::any & value);
    virtual const boost::any getPathValue(const App::ObjectIdentifier & /*path*/) const { return _lValue; }

protected:
    long _lValue;
};

/** Path properties
 * Properties handling file system paths.
 */
class AppExport PropertyPath: public Property
{
    TYPESYSTEM_HEADER();

public:

    PropertyPath();
    virtual ~PropertyPath();

    /** Sets the property
     */
    void setValue(const boost::filesystem::path &);

    /** Sets the property
     */
    void setValue(const char *);

    /** This method returns a string representation of the property
     */
    boost::filesystem::path getValue(void) const;

    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyPathItem"; }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const;

protected:
    boost::filesystem::path _cValue;
};

/// Property wrapper around an Enumeration object.
class AppExport PropertyEnumeration: public Property
{
    TYPESYSTEM_HEADER();

public:
    /// Standard constructor
    PropertyEnumeration();

    /// Obvious constructor
    PropertyEnumeration(const Enumeration &e);

    /// destructor
    virtual ~PropertyEnumeration();

    /// Enumeration methods
    /*!
     * These all function as per documentation in Enumeration
     */
    //@{
    /** setting the enumaration string list
     * The list is a NULL terminated array of pointers to a const char* string
     * \code
     * const char enums[] = {"Black","White","Other",NULL}
     * \endcode
     */
    void setEnums(const char** plEnums);

    /** setting the enumaration string as vector of strings
     * This makes the enumeration custom.
     */
    void setEnums(const std::vector<std::string> &Enums);

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
    void setValue(const Enumeration &source);

    /// Returns current value of the enumeration as an integer
    long getValue(void) const;

    /// checks if the property is set to a certain string value
    bool isValue(const char* value) const;

    /// checks if a string is included in the enumeration
    bool isPartOf(const char* value) const;

    /// get the value as string
    const char * getValueAsString(void) const;

    /// Returns Enumeration object
    Enumeration getEnum(void) const;

    /// get all possible enum values as vector of strings
    std::vector<std::string> getEnumVector(void) const;

    /// get the pointer to the enum list
    const char ** getEnums(void) const;

    /// Returns true if the instance is in a usable state
    bool isValid(void) const;
    //@}

    const char* getEditorName(void) const { return _editorTypeName.c_str(); }
    void setEditorName(const char* name) { _editorTypeName = name; }

    virtual PyObject * getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save(Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property * Copy(void) const;
    virtual void Paste(const Property &from);

    virtual void setPathValue(const App::ObjectIdentifier & path, const boost::any & value);
    virtual const boost::any getPathValue(const App::ObjectIdentifier & /*path*/) const { return _enum; }

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
    TYPESYSTEM_HEADER();

public:
    /// Standard constructor
    PropertyIntegerConstraint();

    /// destructor
    virtual ~PropertyIntegerConstraint();

    /// Constraint methods
    //@{
    /// the boundary struct
    struct Constraints {
        long LowerBound, UpperBound, StepSize;
        Constraints()
            : LowerBound(0)
            , UpperBound(0)
            , StepSize(0)
            , candelete(false)
        {
        }
        Constraints(long l, long u, long s)
            : LowerBound(l)
            , UpperBound(u)
            , StepSize(s)
            , candelete(false)
        {
        }
        ~Constraints()
        {
        }
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
    const Constraints*  getConstraints(void) const;
    //@}

    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyIntegerConstraintItem"; }
    virtual void setPyObject(PyObject *);

protected:
    const Constraints* _ConstStruct;
};

/** Percent property
 * This property is a special integer property and holds only
 * numbers between 0 and 100.
 */

class AppExport PropertyPercent: public PropertyIntegerConstraint
{
    TYPESYSTEM_HEADER();

public:
    /// Standard constructor
    PropertyPercent();

    /// destructor
    virtual ~PropertyPercent();
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
    virtual ~PropertyIntegerList();

    virtual const char* getEditorName(void) const override
    { return "Gui::PropertyEditor::PropertyIntegerListItem"; }

    virtual PyObject *getPyObject(void) override;

    virtual void Save (Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;

    virtual Property *Copy(void) const override;
    virtual void Paste(const Property &from) override;
    virtual unsigned int getMemSize (void) const override;

protected:
    long getPyValue(PyObject *item) const override;
};

/** Integer list properties
 *
 */
class AppExport PropertyIntegerSet: public Property
{
    TYPESYSTEM_HEADER();

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
    virtual ~PropertyIntegerSet();

    /** Sets the property
     */
    void setValue(long);
    void setValue(void){;}

    void addValue (long value){_lValueSet.insert(value);}
    void setValues (const std::set<long>& values);

    const std::set<long> &getValues(void) const{return _lValueSet;}

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    virtual unsigned int getMemSize (void) const;

private:
    std::set<long> _lValueSet;
};


/** implements a key/value list as property
 *  The key ought to be ASCII the Value should be treated as UTF8 to be saved.
 */
class AppExport PropertyMap: public Property
{
    TYPESYSTEM_HEADER();

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
    virtual ~PropertyMap();

    virtual int getSize(void) const;

    /** Sets the property
     */
    void setValue(void){}
    void setValue(const std::string& key,const std::string& value);
    void setValues(const std::map<std::string,std::string>&);

    /// index operator
    const std::string& operator[] (const std::string& key) const ;

    void  set1Value (const std::string& key, const std::string& value){_lValueList.operator[] (key) = value;}

    const std::map<std::string,std::string> &getValues(void) const{return _lValueList;}

    //virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyStringListItem"; }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const;


private:
    std::map<std::string,std::string> _lValueList;
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
    TYPESYSTEM_HEADER();

public:
    /** Value Constructor
     *  Construct with explicit Values
     */
    PropertyFloat(void);

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyFloat();


    void setValue(double lValue);
    double getValue(void) const;

    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyFloatItem"; }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const{return sizeof(double);}

    void setPathValue(const App::ObjectIdentifier &path, const boost::any &value);
    const boost::any getPathValue(const App::ObjectIdentifier &path) const;

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
    TYPESYSTEM_HEADER();

public:

    /** Value Constructor
     *  Construct with explicit Values
     */
    PropertyFloatConstraint(void);

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyFloatConstraint();


    /// Constraint methods
    //@{
    /// the boundary struct
    struct Constraints {
        double LowerBound, UpperBound, StepSize;
        Constraints()
            : LowerBound(0)
            , UpperBound(0)
            , StepSize(0)
            , candelete(false)
        {
        }
        Constraints(double l, double u, double s)
            : LowerBound(l)
            , UpperBound(u)
            , StepSize(s)
            , candelete(false)
        {
        }
        ~Constraints()
        {
        }
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
    const Constraints*  getConstraints(void) const;
    //@}

    virtual const char* getEditorName(void) const
    { return "Gui::PropertyEditor::PropertyFloatConstraintItem"; }

    virtual void setPyObject(PyObject *);

protected:
    const Constraints* _ConstStruct;
};


/** Precision properties
 * This property fulfills the need of a floating value with many decimal points,
 * e.g. for holding values like Precision::Confusion(). The value has a default
 * constraint for non-negative, but can be overridden
 */
class AppExport PropertyPrecision: public PropertyFloatConstraint
{
    TYPESYSTEM_HEADER();
public:
    PropertyPrecision(void);
    virtual ~PropertyPrecision();
    virtual const char* getEditorName(void) const
    { return "Gui::PropertyEditor::PropertyPrecisionItem"; }
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
    virtual ~PropertyFloatList();

    virtual const char* getEditorName(void) const override
    { return "Gui::PropertyEditor::PropertyFloatListItem"; }

    virtual PyObject *getPyObject(void) override;

    virtual void Save (Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;

    virtual void SaveDocFile (Base::Writer &writer) const override;
    virtual void RestoreDocFile(Base::Reader &reader) override;

    virtual Property *Copy(void) const override;
    virtual void Paste(const Property &from) override;
    virtual unsigned int getMemSize (void) const override;

protected:
    double getPyValue(PyObject *item) const override;
};


/** String properties
 * This is the father of all properties handling Strings.
 */
class AppExport PropertyString: public Property
{
    TYPESYSTEM_HEADER();

public:

    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyString(void);

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyString();

    virtual void setValue(const char* sString);
    void setValue(const std::string &sString);
    const char* getValue(void) const;
    const std::string& getStrValue(void) const
    { return _cValue; }
    bool isEmpty(void){return _cValue.empty();}

    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyStringItem"; }
    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    virtual unsigned int getMemSize (void) const;

    void setPathValue(const App::ObjectIdentifier &path, const boost::any &value);
    const boost::any getPathValue(const App::ObjectIdentifier &path) const;

protected:
    std::string _cValue;
};

/** UUID properties
 * This property handles unique identifieers
 */
class AppExport PropertyUUID: public Property
{
    TYPESYSTEM_HEADER();

public:

    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyUUID(void);

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyUUID();


    void setValue(const Base::Uuid &);
    void setValue(const char* sString);
    void setValue(const std::string &sString);
    const std::string& getValueStr(void) const;
    const Base::Uuid& getValue(void) const;

    //virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyStringItem"; }
    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    virtual unsigned int getMemSize (void) const;

private:
    Base::Uuid _uuid;
};


/** Property handling with font names.
 */
class AppExport PropertyFont : public PropertyString
{
    TYPESYSTEM_HEADER();

public:
    PropertyFont(void);
    virtual ~PropertyFont();
    virtual const char* getEditorName(void) const
    { return "Gui::PropertyEditor::PropertyFontItem"; }
};

class AppExport PropertyStringList: public PropertyListsT<std::string>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    typedef PropertyListsT<std::string> inherited;

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
    virtual ~PropertyStringList();

    void setValues(const std::list<std::string>&);
    using inherited::setValues;

    virtual const char* getEditorName(void) const override
    { return "Gui::PropertyEditor::PropertyStringListItem"; }

    virtual PyObject *getPyObject(void) override;

    virtual void Save (Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;

    virtual Property *Copy(void) const override;
    virtual void Paste(const Property &from) override;

    virtual unsigned int getMemSize (void) const override;

protected:
    std::string getPyValue(PyObject *item) const override;
};

/** Bool properties
 * This is the father of all properties handling booleans.
 */
class AppExport PropertyBool : public Property
{
    TYPESYSTEM_HEADER();

public:

    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyBool(void);

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyBool();

    void setValue(bool lValue);
    bool getValue(void) const;

    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyBoolItem"; }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const{return sizeof(bool);}

    void setPathValue(const App::ObjectIdentifier &path, const boost::any &value);
    const boost::any getPathValue(const App::ObjectIdentifier &path) const;

private:
    bool _lValue;
};

/** Bool list properties
 *
 */
class AppExport PropertyBoolList : public PropertyListsT<bool,boost::dynamic_bitset<> >
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    typedef PropertyListsT<bool,boost::dynamic_bitset<> > inherited;

public:
    PropertyBoolList();
    virtual ~PropertyBoolList();

    virtual PyObject *getPyObject(void) override;
    virtual void setPyObject(PyObject *) override;

    virtual void Save (Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;

    virtual Property *Copy(void) const override;
    virtual void Paste(const Property &from) override;
    virtual unsigned int getMemSize (void) const override;

protected:
    bool getPyValue(PyObject *) const override;
};


/** Color properties
 * This is the father of all properties handling colors.
 */
class AppExport PropertyColor : public Property
{
    TYPESYSTEM_HEADER();

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
    virtual ~PropertyColor();

    /** Sets the property
     */
    void setValue(const Color &col);
    void setValue(float r, float g, float b, float a=0.0f);
    void setValue(uint32_t rgba);

    /** This method returns a string representation of the property
     */
    const Color &getValue(void) const;

    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyColorItem"; }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const{return sizeof(Color);}


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
    virtual ~PropertyColorList();

    virtual PyObject *getPyObject(void) override;

    virtual void Save (Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;

    virtual void SaveDocFile (Base::Writer &writer) const override;
    virtual void RestoreDocFile(Base::Reader &reader) override;

    virtual Property *Copy(void) const override;
    virtual void Paste(const Property &from) override;
    virtual unsigned int getMemSize (void) const override;

protected:
    Color getPyValue(PyObject *) const override;
};

/** Material properties
 * This is the father of all properties handling colors.
 */
class AppExport PropertyMaterial : public Property
{
    TYPESYSTEM_HEADER();

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
    virtual ~PropertyMaterial();

    /** Sets the property
     */
    void setValue(const Material &mat);
    void setAmbientColor(const Color& col);
    void setDiffuseColor(const Color& col);
    void setSpecularColor(const Color& col);
    void setEmissiveColor(const Color& col);
    void setShininess(float);
    void setTransparency(float);

    /** This method returns a string representation of the property
     */
    const Material &getValue(void) const;

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual const char* getEditorName(void) const;

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const{return sizeof(_cMat);}

private:
    Material _cMat;
};

/** Material properties
*/
class AppExport PropertyMaterialList : public PropertyListsT<Material>
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
    virtual ~PropertyMaterialList();

    virtual PyObject *getPyObject(void) override;

    virtual void Save(Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;

    virtual void SaveDocFile(Base::Writer &writer) const override;
    virtual void RestoreDocFile(Base::Reader &reader) override;

    virtual const char* getEditorName(void) const override;

    virtual Property *Copy(void) const override;
    virtual void Paste(const Property &from) override;
    virtual unsigned int getMemSize(void) const override;

protected:
    Material getPyValue(PyObject *) const override;
};


/** Property for dynamic creation of a FreeCAD persistent object
 *
 * In Python, this property can be assigned a type string to create a dynamic FreeCAD
 * object, and then read back as the Python binding of the newly created object.
 */
class AppExport PropertyPersistentObject: public PropertyString {
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
    typedef PropertyString inherited;
public:
    virtual PyObject *getPyObject(void) override;
    virtual void setValue(const char* type) override;

    virtual void Save (Base::Writer &writer) const override;
    virtual void Restore(Base::XMLReader &reader) override;

    virtual Property *Copy(void) const override;
    virtual void Paste(const Property &from) override;
    virtual unsigned int getMemSize (void) const override;

    std::shared_ptr<Base::Persistence> getObject() const {
        return _pObject;
    }

protected:
    std::shared_ptr<Base::Persistence> _pObject;
};

} // namespace App

#endif // APP_PROPERTYSTANDARD_H
