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

#ifndef PART_PROPERTYTOPOSHAPE_H
#define PART_PROPERTYTOPOSHAPE_H

#include <map>
#include <vector>

#include <App/PropertyGeo.h>

#include "TopoShape.h"
#include <TopAbs_ShapeEnum.hxx>


namespace Part
{

/** The part shape property class.
 * @author Werner Mayer
 */
class PartExport PropertyPartShape : public App::PropertyComplexGeoData
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
    void setValue(const TopoDS_Shape&);
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
    void transformGeometry(const Base::Matrix4D &rclMat) override;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const override;
    //@}

    /** @name Python interface */
    //@{
    PyObject* getPyObject() override;
    void setPyObject(PyObject *value) override;
    //@}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    void SaveDocFile (Base::Writer &writer) const override;
    void RestoreDocFile(Base::Reader &reader) override;

    App::Property *Copy() const override;
    void Paste(const App::Property &from) override;
    unsigned int getMemSize () const override;
    //@}

    /// Get valid paths for this property; used by auto completer
    void getPaths(std::vector<App::ObjectIdentifier> & paths) const override;

private:
    void saveToFile(Base::Writer &writer) const;
    void loadFromFile(Base::Reader &reader);
    void loadFromStream(Base::Reader &reader);

private:
    TopoShape _Shape;
};

struct PartExport ShapeHistory {
    /**
    * @brief MapList: key is index of subshape (of type 'type') in source
    * shape. Value is list of indexes of subshapes in result shape.
    */
    using MapList = std::map<int, std::vector<int> >;
    using List = std::vector<int>;

    TopAbs_ShapeEnum type;
    MapList shapeMap;
};

class PartExport PropertyShapeHistory : public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyShapeHistory();
    ~PropertyShapeHistory() override;

    void setSize(int newSize) override {
        _lValueList.resize(newSize);
    }
    int getSize() const override {
        return _lValueList.size();
    }

    /** Sets the property
     */
    void setValue(const ShapeHistory&);

    void setValues (const std::vector<ShapeHistory>& values);

    const std::vector<ShapeHistory> &getValues() const {
        return _lValueList;
    }

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    void SaveDocFile (Base::Writer &writer) const override;
    void RestoreDocFile(Base::Reader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    unsigned int getMemSize () const override {
        return _lValueList.size() * sizeof(ShapeHistory);
    }

private:
    std::vector<ShapeHistory> _lValueList;
};

/** A property class to store hash codes and two radii for the fillet algorithm.
 * @author Werner Mayer
 */
struct PartExport FilletElement {
    int edgeid;
    double radius1, radius2;
};

class PartExport PropertyFilletEdges : public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyFilletEdges();
    ~PropertyFilletEdges() override;

    void setSize(int newSize) override {
        _lValueList.resize(newSize);
    }
    int getSize() const override {
        return _lValueList.size();
    }

    /** Sets the property
     */
    void setValue(int id, double r1, double r2);

    void setValues (const std::vector<FilletElement>& values);

    const std::vector<FilletElement> &getValues() const {
        return _lValueList;
    }

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;

    void Save (Base::Writer &writer) const override;
    void Restore(Base::XMLReader &reader) override;

    void SaveDocFile (Base::Writer &writer) const override;
    void RestoreDocFile(Base::Reader &reader) override;

    Property *Copy() const override;
    void Paste(const Property &from) override;

    unsigned int getMemSize () const override {
        return _lValueList.size() * sizeof(FilletElement);
    }

private:
    std::vector<FilletElement> _lValueList;
};

} //namespace Part


#endif // PART_PROPERTYTOPOSHAPE_H
