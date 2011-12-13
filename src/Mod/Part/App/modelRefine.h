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


#ifndef MODELREFINE_H
#define MODELREFINE_H

#include <vector>
#include <map>
#include <list>
#include <GeomAbs_SurfaceType.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Edge.hxx>
#include <TopTools_DataMapOfShapeListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>


namespace ModelRefine
{
    typedef std::vector<TopoDS_Face> FaceVectorType;
    typedef std::vector<TopoDS_Edge> EdgeVectorType;

    void getFaceEdges(const TopoDS_Face &face, EdgeVectorType &edges);
    void boundaryEdges(const FaceVectorType &faces, EdgeVectorType &edgesOut);
    TopoDS_Shell removeFaces(const TopoDS_Shell &shell, const FaceVectorType &faces);

    class FaceTypedBase
    {
    private:
        FaceTypedBase(){}
    protected:
        FaceTypedBase(const GeomAbs_SurfaceType &typeIn){surfaceType = typeIn;}
    public:
        virtual bool isEqual(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo) const = 0;
        virtual GeomAbs_SurfaceType getType() const = 0;
        virtual TopoDS_Face buildFace(const FaceVectorType &faces) const = 0;

        static GeomAbs_SurfaceType getFaceType(const TopoDS_Face &faceIn);

    protected:
        GeomAbs_SurfaceType surfaceType;
    };

    class FaceTypedPlane : public FaceTypedBase
    {
    private:
        FaceTypedPlane();
    public:
        virtual bool isEqual(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo) const;
        virtual GeomAbs_SurfaceType getType() const;
        virtual TopoDS_Face buildFace(const FaceVectorType &faces) const;
        friend FaceTypedPlane& getPlaneObject();
    };
    FaceTypedPlane& getPlaneObject();

    class FaceTypedCylinder : public FaceTypedBase
    {
    private:
        FaceTypedCylinder();
    public:
        virtual bool isEqual(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo) const;
        virtual GeomAbs_SurfaceType getType() const;
        virtual TopoDS_Face buildFace(const FaceVectorType &faces) const;
        friend FaceTypedCylinder& getCylinderObject();
    };
    FaceTypedCylinder& getCylinderObject();

    class FaceTypeSplitter
    {
        typedef std::map<GeomAbs_SurfaceType, FaceVectorType> SplitMapType;
    public:
        FaceTypeSplitter(){}
        void addShell(const TopoDS_Shell &shellIn);
        void registerType(const GeomAbs_SurfaceType &type);
        bool hasType(const GeomAbs_SurfaceType &type) const;
        void split();
        const FaceVectorType& getTypedFaceVector(const GeomAbs_SurfaceType &type) const;
    private:
        SplitMapType typeMap;
        TopoDS_Shell shell;
    };

    class FaceAdjacencySplitter
    {
    public:
        FaceAdjacencySplitter(){}
        void split(const FaceVectorType &facesIn);
        int getGroupCount() const {return adjacencyArray.size();}
        const FaceVectorType& getGroup(const std::size_t &index) const {return adjacencyArray[index];}

    private:
        bool hasBeenMapped(const TopoDS_Face &shape);
        void recursiveFind(FaceVectorType &tempSet, const FaceVectorType &facesIn);
        void buildMap(const FaceVectorType &facesIn);
        bool adjacentTest(const TopoDS_Face &faceOne, const TopoDS_Face &faceTwo);
        std::vector<FaceVectorType> adjacencyArray;
        TopTools_DataMapOfShapeListOfShape faceEdgeMap;
        TopTools_MapOfShape processedMap;

    };

    class FaceEqualitySplitter
    {
    public:
        FaceEqualitySplitter(){}
        void split(const FaceVectorType &faces,  FaceTypedBase *object);
        int getGroupCount() const {return equalityVector.size();}
        const FaceVectorType& getGroup(const std::size_t &index) const {return equalityVector[index];}

    private:
        std::vector<FaceVectorType> equalityVector;
    };

    class BoundaryEdgeSplitter
    {
    public:
        BoundaryEdgeSplitter(){}
        void split(const EdgeVectorType &edgesIn);
        const std::vector<EdgeVectorType>& getGroupedEdges(){return groupedEdges;}

    private:
        void splitRecursive(EdgeVectorType &tempEdges, const EdgeVectorType &workEdges);
        void buildMap(const EdgeVectorType &edgesIn);
        bool edgeTest(const TopoDS_Edge &edgeOne, const TopoDS_Edge &edgeTwo);
        TopTools_MapOfShape processed;
        std::vector<EdgeVectorType> groupedEdges;
        TopTools_DataMapOfShapeListOfShape edgeVertexMap;
    };

    class FaceUniter
    {
    private:
        FaceUniter(){}
    public:
        FaceUniter(const TopoDS_Shell &shellIn);
        FaceUniter(const TopoDS_Solid &solidIn);//get first shell
        bool process();
        const TopoDS_Shell& getShell() const {return workShell;}
        bool getSolid(TopoDS_Solid &outSolid) const;//tries to make solid from shell.

    private:
        TopoDS_Shell workShell;
        std::vector<FaceTypedBase *> typeObjects;
    };
}

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

#endif // MODELREFINE_H
