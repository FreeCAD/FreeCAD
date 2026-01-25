// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>

#include <Mod/Inspection/InspectionGlobal.h>
#include <Mod/Points/App/Points.h>


class TopoDS_Shape;
class BRepExtrema_DistShapeShape;
class gp_Pnt;

namespace MeshCore
{
class MeshKernel;
class MeshGrid;
}  // namespace MeshCore

namespace Mesh
{
class MeshObject;
}
namespace Points
{
class PointsGrid;
}
namespace Part
{
class TopoShape;
}

namespace Inspection
{

/** Delivers the number of points to be checked and returns the appropriate point to an index. */
class InspectionExport InspectActualGeometry
{
public:
    InspectActualGeometry() = default;
    virtual ~InspectActualGeometry() = default;
    /// Number of points to be checked
    virtual unsigned long countPoints() const = 0;
    virtual Base::Vector3f getPoint(unsigned long) const = 0;
};

class InspectionExport InspectActualMesh: public InspectActualGeometry
{
public:
    explicit InspectActualMesh(const Mesh::MeshObject& rMesh);
    ~InspectActualMesh() override;
    unsigned long countPoints() const override;
    Base::Vector3f getPoint(unsigned long) const override;

private:
    const MeshCore::MeshKernel& _mesh;
    bool _bApply;
    Base::Matrix4D _clTrf;
};

class InspectionExport InspectActualPoints: public InspectActualGeometry
{
public:
    explicit InspectActualPoints(const Points::PointKernel&);
    unsigned long countPoints() const override;
    Base::Vector3f getPoint(unsigned long) const override;

private:
    const Points::PointKernel& _rKernel;
};

class InspectionExport InspectActualShape: public InspectActualGeometry
{
public:
    explicit InspectActualShape(const Part::TopoShape&);
    unsigned long countPoints() const override;
    Base::Vector3f getPoint(unsigned long) const override;

private:
    void fetchPoints(double deflection);

private:
    const Part::TopoShape& _rShape;
    std::vector<Base::Vector3d> points;
};

/** Calculates the shortest distance of the underlying geometry to a given point. */
class InspectionExport InspectNominalGeometry
{
public:
    InspectNominalGeometry() = default;
    virtual ~InspectNominalGeometry() = default;
    virtual float getDistance(const Base::Vector3f&) const = 0;
};

class InspectionExport InspectNominalMesh: public InspectNominalGeometry
{
public:
    InspectNominalMesh(const Mesh::MeshObject& rMesh, float offset);
    ~InspectNominalMesh() override;
    float getDistance(const Base::Vector3f&) const override;

private:
    const MeshCore::MeshKernel& _mesh;
    MeshCore::MeshGrid* _pGrid;
    Base::BoundBox3f _box;
    bool _bApply;
    Base::Matrix4D _clTrf;
};

class InspectionExport InspectNominalFastMesh: public InspectNominalGeometry
{
public:
    InspectNominalFastMesh(const Mesh::MeshObject& rMesh, float offset);
    ~InspectNominalFastMesh() override;
    float getDistance(const Base::Vector3f&) const override;

protected:
    const MeshCore::MeshKernel& _mesh;
    MeshCore::MeshGrid* _pGrid;
    Base::BoundBox3f _box;
    unsigned long max_level;
    bool _bApply;
    Base::Matrix4D _clTrf;
};

class InspectionExport InspectNominalPoints: public InspectNominalGeometry
{
public:
    InspectNominalPoints(const Points::PointKernel&, float offset);
    ~InspectNominalPoints() override;
    float getDistance(const Base::Vector3f&) const override;

private:
    const Points::PointKernel& _rKernel;
    Points::PointsGrid* _pGrid;
};

class InspectionExport InspectNominalShape: public InspectNominalGeometry
{
public:
    InspectNominalShape(const TopoDS_Shape&, float offset);
    ~InspectNominalShape() override;
    float getDistance(const Base::Vector3f&) const override;

private:
    bool isInsideSolid(const gp_Pnt&) const;
    bool isBelowFace(const gp_Pnt&) const;

private:
    BRepExtrema_DistShapeShape* distss;
    const TopoDS_Shape& _rShape;
    bool isSolid {false};
};

class InspectionExport PropertyDistanceList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyDistanceList();

    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    ~PropertyDistanceList() override;

    void setSize(int newSize) override;
    int getSize() const override;

    /** Sets the property
     */
    void setValue(float);

    /// index operator
    float operator[](const int idx) const
    {
        return _lValueList.operator[](idx);
    }

    void set1Value(const int idx, float value)
    {
        _lValueList.operator[](idx) = value;
    }
    void setValues(const std::vector<float>& values);

    const std::vector<float>& getValues() const
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
    unsigned int getMemSize() const override;

private:
    std::vector<float> _lValueList;
};

// ----------------------------------------------------------------

/** The inspection feature.
 * \author Werner Mayer
 */
class InspectionExport Feature: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Inspection::Feature);

public:
    /// Constructor
    Feature();
    ~Feature() override;

    /** @name Properties */
    //@{
    App::PropertyFloat SearchRadius;
    App::PropertyFloat Thickness;
    App::PropertyLink Actual;
    App::PropertyLinkList Nominals;
    PropertyDistanceList Distances;
    //@}

    /** @name Actions */
    //@{
    short mustExecute() const override;
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "InspectionGui::ViewProviderInspection";
    }
};

class InspectionExport Group: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(Inspection::Group);

public:
    /// Constructor
    Group();
    ~Group() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "InspectionGui::ViewProviderInspectionGroup";
    }
};

}  // namespace Inspection
