/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#include <string>
#include <list>
#include <vector>
#include <boost/filesystem/path.hpp>

#include <Base/Uuid.h>
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
    ~PropertyInteger();

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

protected:
    long _lValue;
};

/** Path properties
 * This properties handling file system paths.
 */
class AppExport PropertyPath: public Property
{
    TYPESYSTEM_HEADER();

public:
  
    PropertyPath();
    ~PropertyPath();

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

/** Enum properties
 * This property fullfill the need of enumarations. It holds basicly a 
 * state (integer) and a list of valid state names. If the valid state
 * list is not set it act basicly like a IntegerProperty and do no checking.
 * If the list is set it checks on the range and if you set the state with
 * a string if its included in the enumarations.
 * In DEBUG the boundaries get checked, otherwise the caller of setValue()
 * has the responsebility to check the correctnes.
 * This mean if you set by setValue(const char*) with an not included value
 * and not using isPartOf() before,
 * in DEBUG you get an assert() in release its set to 0.
 */
class AppExport PropertyEnumeration: public PropertyInteger
{
    TYPESYSTEM_HEADER();

public:
    /// Standard constructor
    PropertyEnumeration();
    
    /// destructor
    ~PropertyEnumeration();

    /// Enumeration methods 
    //@{
    /** setting the enumaration string list
     * The list is a NULL terminated array of pointers to a const char* string
     * \code
     * const char enums[] = {"Black","White","Other",NULL}
     * \endcode
     */
    void setEnums(const char** plEnums);
    /** set the enum by a string
     * is slower the setValue(long). Use long if possible
     */
    void setValue(const char* value);
    /** set directly the enum value
     * In DEBUG checks for boundaries.
     * Is faster then using setValue(const char*).
     */
    void setValue(long);
    /// checks if the property is set to a certain string value
    bool isValue(const char* value) const;
    /// checks if a string is included in the enumeration
    bool isPartOf(const char* value) const;
    /// get the value as string
    const char* getValueAsString(void) const;
    /// get all possible enum values as vector of strings
    std::vector<std::string> getEnumVector(void) const;
    /// set all enum values as vector of strings
    void setEnumVector(const std::vector<std::string>&);
    /// get the pointer to the enum list
    const char** getEnums(void) const;
    //@}

    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyEnumItem"; }
    
    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

private:
    bool _CustomEnum;
    const char** _EnumArray;
};

/** Constraint integer properties
 * This property fullfill the need of constraint integer. It holds basicly a 
 * state (integer) and a struct of boundaries. If the boundaries
 * is not set it act basicly like a IntegerProperty and do no checking.
 * The constraints struct can be created on the heap or build in.
 */
class AppExport PropertyIntegerConstraint: public PropertyInteger
{
    TYPESYSTEM_HEADER();

public:
    /// Standard constructor
    PropertyIntegerConstraint();
    
    /// destructor
    ~PropertyIntegerConstraint();

    /// Constraint methods 
    //@{
    /// the boundary struct
    struct Constraints {
        long LowerBound, UpperBound, StepSize;
    };
    /** setting the boundaries
     * This sets the constraint struct. It can be dynamcly 
     * allocated or set as an static in the class the property
     * blongs to:
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
 * This property is a special interger property and holds only
 * numbers between 0 and 100.
 */

class AppExport PropertyPercent: public PropertyIntegerConstraint
{
    TYPESYSTEM_HEADER();

public:
    /// Standard constructor
    PropertyPercent();
    
    /// destructor
    ~PropertyPercent();
};

/** Integer list properties
 * 
 */
class AppExport PropertyIntegerList: public PropertyLists
{
    TYPESYSTEM_HEADER();

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
    ~PropertyIntegerList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    /** Sets the property 
     */
    void setValue(long);
  
    /// index operator
    long operator[] (const int idx) const {return _lValueList.operator[] (idx);} 
  
    void  set1Value (const int idx, long value){_lValueList.operator[] (idx) = value;}
    void setValues (const std::vector<long>& values);

    const std::vector<long> &getValues(void) const{return _lValueList;}

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);
    
    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);
    
    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    virtual unsigned int getMemSize (void) const;

private:
    std::vector<long> _lValueList;
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
    ~PropertyIntegerSet();

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
 *  The key ought to be ASCII the Value should be treated as UTF8 to be save.
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
    ~PropertyMap();

    virtual int getSize(void) const;
    
    /** Sets the property 
     */
    void setValue(void){};
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
 * use the more specialized types like e.g. PropertyLenth.
 * These properties fulfill also the needs of the unit system.
 * See PropertyUnits.h for all properties with units.
 */
class AppExport PropertyFloat: public Property
{
    TYPESYSTEM_HEADER();

public:
    /** Value Constructor
     *  Construct with explicite Values
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
    
protected:
    double _dValue;
};

/** Constraint float properties
 * This property fullfill the need of constraint float. It holds basicly a 
 * state (float) and a struct of boundaries. If the boundaries
 * is not set it act basicly like a IntegerProperty and do no checking.
 * The constraints struct can be created on the heap or build in.
 */
class AppExport PropertyFloatConstraint: public PropertyFloat
{
    TYPESYSTEM_HEADER();

public:

    /** Value Constructor
     *  Construct with explicite Values
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
    };
    /** setting the boundaries
     * This sets the constraint struct. It can be dynamcly 
     * allocated or set as an static in the class the property
     * blongs to:
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

class AppExport PropertyFloatList: public PropertyLists
{
    TYPESYSTEM_HEADER();

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
    
    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    /** Sets the property 
     */
    void setValue(double);

    void setValue (void){};
    
    /// index operator
    double operator[] (const int idx) const {return _lValueList.operator[] (idx);} 
    
    
    void set1Value (const int idx, double value){_lValueList.operator[] (idx) = value;}
    void setValues (const std::vector<double>& values);
    
    const std::vector<double> &getValues(void) const{return _lValueList;}
    
    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);
    
    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);
    
    virtual void SaveDocFile (Base::Writer &writer) const;
    virtual void RestoreDocFile(Base::Reader &reader);
    
    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    virtual unsigned int getMemSize (void) const;

private:
    std::vector<double> _lValueList;
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

    void setValue(const char* sString);
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

private:
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

class AppExport PropertyStringList: public PropertyLists
{
    TYPESYSTEM_HEADER();

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
    ~PropertyStringList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;
    
    /** Sets the property 
     */
    void setValue(const std::string&);
    void setValues(const std::vector<std::string>&);
    void setValues(const std::list<std::string>&);
    
    /// index operator
    const std::string& operator[] (const int idx) const {return _lValueList.operator[] (idx);} 
    
    void  set1Value (const int idx, const std::string& value){_lValueList.operator[] (idx) = value;}
    
    const std::vector<std::string> &getValues(void) const{return _lValueList;}
    
    virtual const char* getEditorName(void) const { return "Gui::PropertyEditor::PropertyStringListItem"; }
    
    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);
    
    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);
    
    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    
    virtual unsigned int getMemSize (void) const;
    

private:
    std::vector<std::string> _lValueList;
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
    
private:
    bool _lValue;
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
    ~PropertyColor();

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

class AppExport PropertyColorList: public PropertyLists
{
    TYPESYSTEM_HEADER();

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
    ~PropertyColorList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;
    
    /** Sets the property 
     */
    void setValue(const Color&);
  
    /// index operator
    const Color& operator[] (const int idx) const {return _lValueList.operator[] (idx);} 
    
    void  set1Value (const int idx, const Color& value){_lValueList.operator[] (idx) = value;}
    
    void setValues (const std::vector<Color>& values);
    const std::vector<Color> &getValues(void) const{return _lValueList;}
    
    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);
    
    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);
    
    virtual void SaveDocFile (Base::Writer &writer) const;
    virtual void RestoreDocFile(Base::Reader &reader);
    
    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    virtual unsigned int getMemSize (void) const;
    
private:
    std::vector<Color> _lValueList;
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
    ~PropertyMaterial();
    
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
    
    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    
    virtual unsigned int getMemSize (void) const{return sizeof(_cMat);}
    
private:
    Material _cMat;
};



} // namespace App

#endif // APP_PROPERTYSTANDARD_H
