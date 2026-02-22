// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <map>
#include <vector>

#include <App/PropertyGeo.h>

#include <Mod/Part/PartGlobal.h>

#include "TopoShape.h"
#include <TopAbs_ShapeEnum.hxx>


namespace Part
{

class Feature;
/** The part shape property class.
 * @author Werner Mayer
 */
class PartExport PropertyPartShape: public App::PropertyComplexGeoData
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyPartShape();
    ~PropertyPartShape() override;

    /** @name Getter/setter */
    //@{
    /// set the part shape
    void setValue(const TopoShape&);
    /// set the part shape
    void setValue(const TopoDS_Shape&, bool resetElementMap = true);
    /// get the part shape
    const TopoDS_Shape& getValue() const;
    const TopoShape& getShape() const;
    const Data::ComplexGeoData* getComplexData() const override;
    //@}

    /** @name Modification */
    //@{
    /// Set the placement of the geometry
    void setTransform(const Base::Matrix4D& rclTrf) override;
    /// Get the placement of the geometry
    Base::Matrix4D getTransform() const override;
    /// Transform the real shape data
    void transformGeometry(const Base::Matrix4D& rclMat) override;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const override;
    //@}

    /** @name Python interface */
    //@{
    PyObject* getPyObject() override;
    void setPyObject(PyObject* value) override;
    //@}

    /** @name Save/restore */
    //@{
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    virtual void beforeSave() const override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;
    unsigned int getMemSize() const override;
    //@}

    /// Get valid paths for this property; used by auto completer
    void getPaths(std::vector<App::ObjectIdentifier>& paths) const override;

    std::string getElementMapVersion(bool restored) const override;
    void resetElementMapVersion()
    {
        _Ver.clear();
    }

    void afterRestore() override;

    friend class Feature;

private:
    void saveToFile(Base::Writer& writer) const;
    void loadFromFile(Base::Reader& reader);
    void loadFromStream(Base::Reader& reader);

private:
    TopoShape _Shape;
    std::string _Ver;
    mutable int _HasherIndex = 0;
    mutable bool _SaveHasher = false;
};

struct PartExport ShapeHistory
{
    /**
     * @brief MapList: key is index of subshape (of type 'type') in source
     * shape. Value is list of indexes of subshapes in result shape.
     */
    using MapList = std::map<int, std::vector<int>>;
    using List = std::vector<int>;

    TopAbs_ShapeEnum type {TopAbs_SHAPE};
    MapList shapeMap;
    ShapeHistory()
    {}
    /**
     * Build a history of changes
     * MakeShape: The operation that created the changes, e.g. FCBRepAlgoAPI_Common
     * type: The type of object we are interested in, e.g. TopAbs_FACE
     * newS: The new shape that was created by the operation
     * oldS: The original shape prior to the operation
     */
    ShapeHistory(
        BRepBuilderAPI_MakeShape& mkShape,
        TopAbs_ShapeEnum type,
        const TopoDS_Shape& newS,
        const TopoDS_Shape& oldS
    );
    void reset(
        BRepBuilderAPI_MakeShape& mkShape,
        TopAbs_ShapeEnum type,
        const TopoDS_Shape& newS,
        const TopoDS_Shape& oldS
    );
    void join(const ShapeHistory& newH);
};

class PartExport PropertyShapeHistory: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyShapeHistory();
    ~PropertyShapeHistory() override;

    void setSize(int newSize) override
    {
        _lValueList.resize(newSize);
    }
    int getSize() const override
    {
        return _lValueList.size();
    }

    /** Sets the property
     */
    void setValue(const ShapeHistory&);

    void setValues(const std::vector<ShapeHistory>& values);

    const std::vector<ShapeHistory>& getValues() const
    {
        return _lValueList;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return _lValueList.size() * sizeof(ShapeHistory);
    }

private:
    std::vector<ShapeHistory> _lValueList;
};

/** A property class to store hash codes and two radii for the fillet algorithm.
 * @author Werner Mayer
 */
struct PartExport FilletElement
{
    int edgeid;
    double radius1, radius2;

    FilletElement(int id = 0, double r1 = 1.0, double r2 = 1.0)
        : edgeid(id)
        , radius1(r1)
        , radius2(r2)
    {}

    bool operator<(const FilletElement& other) const
    {
        return edgeid < other.edgeid;
    }

    bool operator==(const FilletElement& other) const
    {
        return edgeid == other.edgeid && radius1 == other.radius1 && radius2 == other.radius2;
    }
};

class PartExport PropertyFilletEdges: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyFilletEdges();
    ~PropertyFilletEdges() override;

    void setSize(int newSize) override
    {
        _lValueList.resize(newSize);
    }
    int getSize() const override
    {
        return _lValueList.size();
    }

    /** Sets the property
     */
    void setValue(int id, double r1, double r2);

    void setValues(const std::vector<FilletElement>& values);

    const std::vector<FilletElement>& getValues() const
    {
        return _lValueList;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override
    {
        return _lValueList.size() * sizeof(FilletElement);
    }

private:
    std::vector<FilletElement> _lValueList;
};


class PartExport PropertyShapeCache: public App::Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    virtual App::Property* Copy(void) const override;

    virtual void Paste(const App::Property&) override;

    virtual PyObject* getPyObject() override;

    virtual void setPyObject(PyObject* value) override;

    virtual void Save(Base::Writer& writer) const override;

    virtual void Restore(Base::XMLReader& reader) override;

    static PropertyShapeCache* get(const App::DocumentObject* obj, bool create);
    static bool getShape(const App::DocumentObject* obj, TopoShape& shape, const char* subname = 0);
    static void setShape(const App::DocumentObject* obj, const TopoShape& shape, const char* subname = 0);

private:
    void slotChanged(const App::DocumentObject&, const App::Property& prop);

private:
    std::unordered_map<std::string, TopoShape> cache;
    fastsignals::scoped_connection connChanged;
};

}  // namespace Part
