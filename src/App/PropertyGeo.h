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


#ifndef APP_PROPERTYGEO_H
#define APP_PROPERTYGEO_H

// Std. configurations

#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Base/BoundBox.h>
#include <Base/Placement.h>

#include "Property.h"
#include "PropertyLinks.h"
#include "ComplexGeoData.h"

namespace Base {
class Writer;
}

namespace Data {
class ComplexGeoData;
}

namespace App
{
class Feature;
class Placement;



/** Vector properties
 * This is the father of all properties handling Integers.
 */
class AppExport PropertyVector: public Property
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyVector();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyVector();

    /** Sets the property
     */
    void setValue(const Base::Vector3d &vec);
    void setValue(double x, double y, double z);

    /** This method returns a string representation of the property
     */
    const Base::Vector3d &getValue(void) const;
    const char* getEditorName(void) const {
        return "Gui::PropertyEditor::PropertyVectorItem";
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const {
        return sizeof(Base::Vector3d);
    }

private:
    Base::Vector3d _cVec;
};


class AppExport PropertyVectorList: public PropertyLists
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyVectorList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyVectorList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    /** Sets the property
     */
    void setValue(const Base::Vector3d&);
    void setValue(double x, double y, double z);

    /// index operator
    const Base::Vector3d& operator[] (const int idx) const {
        return _lValueList.operator[] (idx);
    }

    void set1Value (const int idx, const Base::Vector3d& value) {
        _lValueList.operator[] (idx) = value;
    }

    void setValues (const std::vector<Base::Vector3d>& values);

    void setValue (void){};

    const std::vector<Base::Vector3d> &getValues(void) const {
        return _lValueList;
    }

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
    std::vector<Base::Vector3d> _lValueList;
};

/** Vector properties
 * This is the father of all properties handling Integers.
 */
class AppExport PropertyMatrix: public Property
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyMatrix();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyMatrix();

    /** Sets the property
     */
    void setValue(const Base::Matrix4D &mat);

    /** This method returns a string representation of the property
     */
    const Base::Matrix4D &getValue(void) const;
    const char* getEditorName(void) const {
        return "Gui::PropertyEditor::PropertyMatrixItem";
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const {
        return sizeof(Base::Matrix4D);
    }

private:
    Base::Matrix4D _cMat;
};

/** Vector properties
 * This is the father of all properties handling Integers.
 */
class AppExport PropertyPlacement: public Property
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyPlacement();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyPlacement();

    /** Sets the property
     */
    void setValue(const Base::Placement &pos);

    /** This method returns a string representation of the property
     */
    const Base::Placement &getValue(void) const;
    const char* getEditorName(void) const {
        return "Gui::PropertyEditor::PropertyPlacementItem";
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);

    virtual unsigned int getMemSize (void) const {
        return sizeof(Base::Placement);
    }

    static const Placement Null;

private:
    Base::Placement _cPos;
};

/** the general Link Poperty
 *  Main Purpose of this property is to Link Objects and Feautures in a document.
 */
class AppExport PropertyPlacementLink : public PropertyLink
{
    TYPESYSTEM_HEADER();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyPlacementLink();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyPlacementLink();

    /** This method returns the linked DocumentObject
     */
    App::Placement * getPlacementObject(void) const;

    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
};

/** The base class for all basic geometry properties.
 * @author Werner Mayer
 */
class AppExport PropertyGeometry : public App::Property
{
    TYPESYSTEM_HEADER();

public:
    PropertyGeometry();
    ~PropertyGeometry();

    /** @name Modification */
    //@{
    /// Applies a transformation on the real geometric data type
    virtual void transformGeometry(const Base::Matrix4D &rclMat) = 0;
    /// Retrieve bounding box information
    virtual Base::BoundBox3d getBoundingBox() const = 0;
    //@}
};

/** The base class for all complex data properties.
 * @author Werner Mayer
 */
class AppExport PropertyComplexGeoData : public App::PropertyGeometry
{
    TYPESYSTEM_HEADER();

public:
    PropertyComplexGeoData();
    ~PropertyComplexGeoData();

    /** @name Modification */
    //@{
    /// Applies a transformation on the real geometric data type
    virtual void transformGeometry(const Base::Matrix4D &rclMat) = 0;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    virtual const Data::ComplexGeoData* getComplexData() const = 0;
    virtual Base::BoundBox3d getBoundingBox() const = 0;
    virtual void getFaces(std::vector<Base::Vector3d> &Points,
        std::vector<Data::ComplexGeoData::Facet> &Topo,
        float Accuracy, uint16_t flags=0) const  = 0;
    //@}
};

} // namespace App


#endif // APP_PROPERTYGEO_H
