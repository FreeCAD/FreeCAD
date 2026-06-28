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

#include <Base/BoundBox.h>
#include <Base/Matrix.h>
#include <Base/Placement.h>
#include <Base/Unit.h>
#include <Base/Vector3D.h>

#include "PropertyLinks.h"
#include <FCGlobal.h>


namespace Base
{
class Writer;
}

namespace Data
{
class ComplexGeoData;
}

namespace App
{
class Placement;


/** Vector properties
 * This is the father of all properties handling Integers.
 */
class AppExport PropertyVector: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

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
    ~PropertyVector() override;

    /** Sets the property
     */
    void setValue(const Base::Vector3d& vec);
    void setValue(double x, double y, double z);

    /// Get valid paths for this property; used by auto completer
    void getPaths(std::vector<ObjectIdentifier>& paths) const override;

    /** This method returns a string representation of the property
     */
    const Base::Vector3d& getValue() const;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyVectorItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(Base::Vector3d);
    }

    const boost::any getPathValue(const ObjectIdentifier& path) const override;

    bool getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const override;

    virtual Base::Unit getUnit() const
    {
        return {};
    }

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == dynamic_cast<decltype(this)>(&other)->getValue();
    }

private:
    Base::Vector3d _cVec;
};


class AppExport PropertyVectorDistance: public PropertyVector
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyVectorDistance();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyVectorDistance() override;

    Base::Unit getUnit() const override
    {
        return Base::Unit::Length;
    }

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyVectorDistanceItem";
    }
};

class AppExport PropertyPosition: public PropertyVector
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyPosition();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyPosition() override;

    Base::Unit getUnit() const override
    {
        return Base::Unit::Length;
    }

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyPositionItem";
    }
};

class AppExport PropertyDirection: public PropertyVector
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyDirection();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyDirection() override;

    Base::Unit getUnit() const override
    {
        return Base::Unit::Length;
    }

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyDirectionItem";
    }
};

class AppExport PropertyVectorList: public PropertyListsT<Base::Vector3d>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

    using inherited = PropertyListsT<Base::Vector3d>;

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
    ~PropertyVectorList() override;

    void setValue(double x, double y, double z);
    using inherited::setValue;

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override;
    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyVectorListItem";
    }

protected:
    Base::Vector3d getPyValue(PyObject*) const override;
};

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

/// Property representing a placement
/*!
 * Encapsulates a Base::Placement in a Property
 */
class AppExport PropertyPlacement: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

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
    ~PropertyPlacement() override;

    /** Sets the property
     */
    void setValue(const Base::Placement& pos);

    /** Sets property only if changed
     * @param pos: input placement
     * @param tol: position tolerance
     * @param atol: angular tolerance
     */
    bool setValueIfChanged(const Base::Placement& pos, double tol = 1e-7, double atol = 1e-12);

    /** This method returns a string representation of the property
     */
    const Base::Placement& getValue() const;

    /// Get valid paths for this property; used by auto completer
    void getPaths(std::vector<ObjectIdentifier>& paths) const override;

    void setPathValue(const ObjectIdentifier& path, const boost::any& value) override;

    const boost::any getPathValue(const ObjectIdentifier& path) const override;

    bool getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyPlacementItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(Base::Placement);
    }

    bool isSame(const Property& other) const override
    {
        if (&other == this) {
            return true;
        }
        return getTypeId() == other.getTypeId()
            && getValue() == static_cast<decltype(this)>(&other)->getValue();
    }

    static const Placement Null;

private:
    Base::Placement _cPos;
};

/** the general Link Property
 *  Main Purpose of this property is to Link Objects and Features in a document.
 */
class AppExport PropertyPlacementLink: public PropertyLink
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

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
    ~PropertyPlacementLink() override;

    /** This method returns the linked DocumentObject
     */
    App::Placement* getPlacementObject() const;

    Property* Copy() const override;
    void Paste(const Property& from) override;
};


class AppExport PropertyPlacementList: public PropertyListsT<Base::Placement>
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A property that stores a list of placements
     */
    PropertyPlacementList();

    ~PropertyPlacementList() override;

    PyObject* getPyObject() override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override;

protected:
    Base::Placement getPyValue(PyObject* item) const override;
};


/// Property representing a rotation
/*!
 * Encapsulates a Base::Rotation in a Property
 */
class AppExport PropertyRotation: public Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyRotation();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyRotation() override;

    /** Sets the property
     */
    void setValue(const Base::Rotation& rot);

    /** Sets property only if changed
     * @param pos: input placement
     * @param tol: position tolerance
     * @param atol: angular tolerance
     */
    bool setValueIfChanged(const Base::Rotation& rot, double atol = 1e-12);

    /** This method returns a string representation of the property
     */
    const Base::Rotation& getValue() const;

    /// Get valid paths for this property; used by auto completer
    void getPaths(std::vector<ObjectIdentifier>& paths) const override;

    void setPathValue(const ObjectIdentifier& path, const boost::any& value) override;

    const boost::any getPathValue(const ObjectIdentifier& path) const override;

    bool getPyPathValue(const ObjectIdentifier& path, Py::Object& res) const override;

    const char* getEditorName() const override
    {
        return "Gui::PropertyEditor::PropertyRotationItem";
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return sizeof(Base::Placement);
    }

private:
    Base::Rotation _rot;
};


/** The base class for all basic geometry properties.
 * @author Werner Mayer
 */
class AppExport PropertyGeometry: public App::Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyGeometry();
    ~PropertyGeometry() override;

    /** @name Modification */
    //@{
    /// Set the placement of the geometry
    virtual void setTransform(const Base::Matrix4D& rclTrf) = 0;
    /// Get the placement of the geometry
    virtual Base::Matrix4D getTransform() const = 0;
    /// Applies a transformation on the real geometric data type
    virtual void transformGeometry(const Base::Matrix4D& rclMat) = 0;
    /// Retrieve bounding box information
    virtual Base::BoundBox3d getBoundingBox() const = 0;
    //@}
};

/** The base class for all complex data properties.
 * @author Werner Mayer
 */
class AppExport PropertyComplexGeoData: public App::PropertyGeometry
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyComplexGeoData();
    ~PropertyComplexGeoData() override;

    /** @name Modification */
    //@{
    /// Applies a transformation on the real geometric data type
    void transformGeometry(const Base::Matrix4D& rclMat) override = 0;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    virtual const Data::ComplexGeoData* getComplexData() const = 0;
    Base::BoundBox3d getBoundingBox() const override = 0;
    //@}

    /** Return the element map version
     *
     * @param persisted: if true, return the restored element map version. Or
     * else, return the current element map version
     */
    virtual std::string getElementMapVersion(bool restored = false) const;

    /// Return true to signal element map version change
    virtual bool checkElementMapVersion(const char* ver) const;

    void afterRestore() override;
};

}  // namespace App
