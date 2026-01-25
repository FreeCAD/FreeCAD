// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Thomas Anderson <blobfish[at]gmx.com>              *
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
#include <map>
#include <vector>

#include <BRepBuilderAPI_MakeShape.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Standard_Version.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shell.hxx>

#include <TopTools_DataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>

#include <Mod/Part/PartGlobal.h>


namespace ModelRefine
{
using FaceVectorType = std::vector<TopoDS_Face>;
using EdgeVectorType = std::vector<TopoDS_Edge>;
using ShapeVectorType = std::vector<TopoDS_Shape>;
using ShapePairType = std::pair<TopoDS_Shape, TopoDS_Shape>;

void getFaceEdges(const TopoDS_Face& face, EdgeVectorType& edges);
void boundaryEdges(const FaceVectorType& faces, EdgeVectorType& edgesOut);
TopoDS_Shell removeFaces(const TopoDS_Shell& shell, const FaceVectorType& faces);

class FaceTypedBase
{
private:
    FaceTypedBase() = default;

protected:
    FaceTypedBase(const GeomAbs_SurfaceType& typeIn)
    {
        surfaceType = typeIn;
    }

public:
    virtual bool isEqual(const TopoDS_Face& faceOne, const TopoDS_Face& faceTwo) const = 0;
    virtual GeomAbs_SurfaceType getType() const = 0;
    virtual TopoDS_Face buildFace(const FaceVectorType& faces) const = 0;

    static GeomAbs_SurfaceType getFaceType(const TopoDS_Face& faceIn);

protected:
    virtual void boundarySplit(
        const FaceVectorType& facesIn,
        std::vector<EdgeVectorType>& boundariesOut
    ) const;
    GeomAbs_SurfaceType surfaceType;
};

class FaceTypedPlane: public FaceTypedBase
{
private:
    FaceTypedPlane();

public:
    bool isEqual(const TopoDS_Face& faceOne, const TopoDS_Face& faceTwo) const override;
    GeomAbs_SurfaceType getType() const override;
    TopoDS_Face buildFace(const FaceVectorType& faces) const override;
    friend FaceTypedPlane& getPlaneObject();
};
FaceTypedPlane& getPlaneObject();

class FaceTypedCylinder: public FaceTypedBase
{
private:
    FaceTypedCylinder();

public:
    bool isEqual(const TopoDS_Face& faceOne, const TopoDS_Face& faceTwo) const override;
    GeomAbs_SurfaceType getType() const override;
    TopoDS_Face buildFace(const FaceVectorType& faces) const override;
    friend FaceTypedCylinder& getCylinderObject();

protected:
    void boundarySplit(
        const FaceVectorType& facesIn,
        std::vector<EdgeVectorType>& boundariesOut
    ) const override;
};
FaceTypedCylinder& getCylinderObject();

class FaceTypedBSpline: public FaceTypedBase
{
private:
    FaceTypedBSpline();

public:
    bool isEqual(const TopoDS_Face& faceOne, const TopoDS_Face& faceTwo) const override;
    GeomAbs_SurfaceType getType() const override;
    TopoDS_Face buildFace(const FaceVectorType& faces) const override;
    friend FaceTypedBSpline& getBSplineObject();
};
FaceTypedBSpline& getBSplineObject();

class FaceTypeSplitter
{
    using SplitMapType = std::map<GeomAbs_SurfaceType, FaceVectorType>;

public:
    FaceTypeSplitter() = default;
    void addShell(const TopoDS_Shell& shellIn);
    void registerType(const GeomAbs_SurfaceType& type);
    bool hasType(const GeomAbs_SurfaceType& type) const;
    void split();
    const FaceVectorType& getTypedFaceVector(const GeomAbs_SurfaceType& type) const;

private:
    SplitMapType typeMap;
    TopoDS_Shell shell;
};

class FaceAdjacencySplitter
{
public:
    FaceAdjacencySplitter(const TopoDS_Shell& shell);
    void split(const FaceVectorType& facesIn);
    std::size_t getGroupCount() const
    {
        return adjacencyArray.size();
    }
    const FaceVectorType& getGroup(const std::size_t& index) const
    {
        return adjacencyArray[index];
    }

private:
    FaceAdjacencySplitter() = default;
    void recursiveFind(const TopoDS_Face& face, FaceVectorType& outVector);
    std::vector<FaceVectorType> adjacencyArray;
    TopTools_MapOfShape processedMap;
    TopTools_MapOfShape facesInMap;

    TopTools_IndexedDataMapOfShapeListOfShape faceToEdgeMap;
    TopTools_IndexedDataMapOfShapeListOfShape edgeToFaceMap;
};

class FaceEqualitySplitter
{
public:
    FaceEqualitySplitter() = default;
    void split(const FaceVectorType& faces, FaceTypedBase* object);
    std::size_t getGroupCount() const
    {
        return equalityVector.size();
    }
    const FaceVectorType& getGroup(const std::size_t& index) const
    {
        return equalityVector[index];
    }

private:
    std::vector<FaceVectorType> equalityVector;
};

class FaceUniter
{
private:
    FaceUniter() = default;

public:
    FaceUniter(const TopoDS_Shell& shellIn);
    bool process();
    const TopoDS_Shell& getShell() const
    {
        return workShell;
    }
    void fixOrientation(const TopoDS_Shell& shell);
    bool isModified()
    {
        return modifiedSignal;
    }
    const std::vector<ShapePairType>& getModifiedShapes() const
    {
        return modifiedShapes;
    }
    const ShapeVectorType& getDeletedShapes() const
    {
        return deletedShapes;
    }

private:
    TopoDS_Shell workShell;
    std::vector<FaceTypedBase*> typeObjects;
    std::vector<ShapePairType> modifiedShapes;
    ShapeVectorType deletedShapes;
    bool modifiedSignal;
};
}  // namespace ModelRefine

/* excerpt from GeomAbs_SurfaceType.hxx
enum GeomAbs_SurfaceType {
GeomAbs_Plane,
GeomAbs_Cylinder,
GeomAbs_Cone,
GeomAbs_Sphere,
GeomAbs_Torus,
GeomAbs_BezierSurface,
GeomAbs_BSplineSurface,
GeomAbs_SurfaceOfRevolution,
GeomAbs_SurfaceOfExtrusion,
GeomAbs_OffsetSurface,
GeomAbs_OtherSurface
};
*/
namespace Part
{
class PartExport BRepBuilderAPI_RefineModel: public BRepBuilderAPI_MakeShape
{
public:
    BRepBuilderAPI_RefineModel(const TopoDS_Shape&);
#if OCC_VERSION_HEX >= 0x070600
    void Build(const Message_ProgressRange& theRange = Message_ProgressRange()) override;
#else
    void Build() override;
#endif
    const TopTools_ListOfShape& Modified(const TopoDS_Shape& S) override;
    Standard_Boolean IsDeleted(const TopoDS_Shape& S) override;

private:
    void LogModifications(const ModelRefine::FaceUniter& uniter);

protected:
    TopTools_DataMapOfShapeListOfShape myModified;
    TopTools_ListOfShape myEmptyList;
    TopTools_ListOfShape myDeleted;
};
}  // namespace Part
