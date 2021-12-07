/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
 *   Copyright (c) 2007 Human Rezai <human@mytum.de>                       *
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

#ifndef _SpringbackCorrection_H_
#define _SpringbackCorrection_H_


#include <Mod/Mesh/App/Core/TopoAlgorithm.h>
#include "cutting_tools.h"

/** @brief The main class for the SpringbackCorrection routine

 It takes a mesh and a Topo_Shape and computes
 a deformation of the triangulated Topo_Shape.
 The deformation is based on mirroring the input-mesh on the Topo_Shape.
*/

class MeshFacet;




# define cMin    15.0
# define toolRad  5.0

/** @brief a struct referring to the edges of the input-shape
 
    @param anEdge    edge
    @param aFace     vector of faces limited by edge
    @param pntList   point-vector of the mesh-points of the edge
    @param MaxOffset maximum curvature-radius
    @param MinOffset minimum curvature-radius
*/
struct EdgeStruct
{
    TopoDS_Edge anEdge;
    std::vector<TopoDS_Face> aFace;
    std::vector<gp_Pnt> pntList;
    double MaxOffset;
    double MinOffset;
};

/** @brief a struct referring to the mesh-points of the input-shape 

    @param index   index of the mesh-point
    @param minCurv minimum curvature-radius
    @param maxCurv maximum curvature-radius
    @param error   distance value
    @param pnt     mesh-point
    @param normal  normal-vector
*/
struct MeshPnt
{
    int index;
    double minCurv;
    double maxCurv;
    double error;
    Base::Vector3f pnt;
    Base::Vector3f normal;
};

/** @brief a struct for computing the maximum and minimum curvature-radius
           of a single mesh-point
    @param pnt        mesh-point
    @param distances  distance-vector to all surrounding edges
    @param MinEdgeOff minimum curvature-radii of surrounding edges
    @param MaxEdgeOff maximum curvature-radii of surrounding edges
*/
struct FacePnt
{
    Base::Vector3f pnt;
    std::vector<double> distances;
    std::vector<double> MinEdgeOff;
    std::vector<double> MaxEdgeOff;
};

struct EdgeStruct_Less
{
    bool operator()(const EdgeStruct& _Left, const EdgeStruct& _Right) const
    {

        if (_Left.anEdge.IsSame(_Right.anEdge)) return false;

        return true;
    }
};

struct Edge_Less
{
    bool operator()(const TopoDS_Edge& _Left, const TopoDS_Edge& _Right) const
    {
        return(_Left.HashCode(IntegerLast())<_Right.HashCode(IntegerLast()));
    }
};




struct MeshPntLess
{
    bool operator()(const Base::Vector3f& _Left, const Base::Vector3f& _Right) const
    {
        if ( fabs(_Left.x - _Right.x) > 0.0001 )
            return _Left.x < _Right.x;
        else if ( fabs(_Left.y - _Right.y) > 0.0001)
            return _Left.y < _Right.y;
        else if ( fabs(_Left.z - _Right.z) > 0.0001 )
            return _Left.z < _Right.z;
        return false;

    }
};

class CamExport SpringbackCorrection : public MeshCore::MeshFacetVisitor
{
public:
    /** @brief Constructor */
    SpringbackCorrection();
    /** @brief Constructor
 
        @param aShape input-shape (CAD-Model)
        @param aMesh  input-mesh
    */
    SpringbackCorrection(const TopoDS_Shape& aShape, const MeshCore::MeshKernel& aMesh);
    /** @brief Constructor */
    //SpringbackCorrection(const TopoDS_Shape& aShape, const MeshCore::MeshKernel& aMesh, const MeshCore::MeshKernel& bMesh);

    ~SpringbackCorrection();
    /** @brief loads input-mesh */
    bool Load(const MeshCore::MeshKernel& aMesh);
    /** @brief loads input-shape */
    bool Load(const TopoDS_Shape& aShape);
    /** @brief loads input-shape and -mesh */
    bool Load(const TopoDS_Shape& aShape, const MeshCore::MeshKernel& aMesh);
    /** @brief sets parameter-values to initial state, tessellates the input
               shape and computes a curvature-radius-value at each edge
               contained in the input-shape */ 
    bool Init();
    /** @brief get external setting-parameters */
    bool Init_Setting(struct CuttingToolsSettings& set);

    /** @brief main-function
 
        @param deg_Tol limiting forming-angle
        @param out
        @todo undocumented parameter out
    */
    bool Perform(int deg_Tol, bool out);

    /** @brief sets curvature-radius-value of user-specified edges to zero */
    bool SetFixEdges();
    /** @brief calculates curvature-values as a distance-weighted
               convexcombination */
    bool CalcCurv();

    /** @brief Computes mesh-based curvature-values at each mesh-point

        @param aFace
        @param mesh input-mesh
        @todo undocumented parameter aFace
    */
    std::vector<double> MeshCurvature(const TopoDS_Face& aFace, const MeshCore::MeshKernel& mesh);

    /** @brief computes maximum and minimum curvature-values of the specified
               input-face \p aFace
        @param aFace input-face */
    bool GetCurvature(TopoDS_Face aFace);

    //bool MirrorMesh(std::vector<double> error);
    //double LocalCorrection(std::vector<Base::Vector3f> Neib ,std::vector<Base::Vector3f> Normal,
    //                       bool sign, double minScale);
    //double GlobalCorrection(std::vector<Base::Vector3f> Neib ,std::vector<Base::Vector3f> Normal,
    //                        bool sign, int ind);
    
    //std::vector<Base::Vector3f> FillConvex(std::vector<Base::Vector3f> Neib,std::vector<Base::Vector3f> Normals, int n);
    //bool InsideCheck(Base::Vector3f pnt, Base::Vector3f normal, std::vector<Base::Vector3f> Neib);

    //MeshCore::MeshKernel BuildMesh(Handle_Poly_Triangulation aTri, std::vector<Base::Vector3f> TrPoints);
    /** @brief returns index-value which specifies the boundary-points of the
               input-mesh

        @param mesh     input-mesh
        @param meshPnts mesh-points of input-mesh
    */
    int GetBoundary(const MeshCore::MeshKernel &mesh, MeshCore::MeshPointArray &meshPnts);
    //bool SmoothCurvature();
    /** @brief smooths input-mesh 

        @param mesh           input-mesh
        @param maxTranslation value which stands for the maximum deviation
                              from the initial-mesh
    */
    bool SmoothMesh(MeshCore::MeshKernel &mesh, double maxTranslation);
    /** @brief smooths specified mesh-points of the input-mesh 

        @param mesh           input-mesh
        @param indicies       vector of indicies of the mesh-points for
                              smoothing
        @param maxTranslation value which stands for the maximum deviation
                              from the initial-mesh
    */
    bool SmoothMesh(MeshCore::MeshKernel &mesh, std::vector<int> indicies, double maxTranslation);
    /** @brief computes current angles of all triangles and stores the angle
               degrees in a vector

        @param mesh    input-mesh
        @param deg_tol limiting forming-angle
    */
    //bool GetFaceAng(MeshCore::MeshKernel &mesh, int deg_tol);
    /** @brief computes current angles of all triangles and stores the
               critical-triangle-indicies in a vector

        @param mesh    input-mesh
        @param deg_tol limiting forming-angle
    */
    std::vector<int> FaceCheck(MeshCore::MeshKernel &mesh, int deg_tol);
    /** @brief computes current angles of all triangles and sets the property
               value of the critical triangles to zero

        @param mesh input-mesh
        @param deg_tol limiting forming-angle
    */
    std::vector<int> InitFaceCheck(MeshCore::MeshKernel &mesh, int deg_tol);
    //void ReorderNeighbourList(std::set<MeshCore::MeshPointArray::_TConstIterator> &pnt,
    //                          std::set<MeshCore::MeshFacetArray::_TConstIterator> &face,
    //                          std::vector<unsigned long> &nei,
    //                          unsigned long CurInd);

    /** @brief performs a region-growing-algorithm */
    bool FacetRegionGrowing(MeshCore::MeshKernel &mesh,
                            std::vector<MeshCore::MeshFacet> &arr,
                            MeshCore::MeshFacetArray &mFacets);

    //bool CorrectScale(double max_zVal);
    /** @brief checks the visit-state of specified triangle */ 
    bool Visit(const MeshCore::MeshFacet &rclFacet, const MeshCore::MeshFacet &rclFrom, unsigned long ulFInd, unsigned long ulLevel);
	
    /** @brief builds a mesh-kernel out of one face-triangulation

        @param aFace     face of concern
        @param TFaceMesh MeshKernel where the triangulation will be stored
    */
    bool TransferFaceTriangulationtoFreeCAD(const TopoDS_Face& aFace, MeshCore::MeshKernel& TFaceMesh);
	
    /** @brief computes offset-values for the inner-points of the specified
               region */
    std::vector< std::pair<unsigned long, double> > RegionEvaluate(const MeshCore::MeshKernel &mesh, std::vector<unsigned long> &RegionFacets, std::vector<Base::Vector3f> &normals);
    
    /** @brief input-mesh */
    MeshCore::MeshKernel m_Mesh;
    /** @brief mesh containing the movable regions for the local translation
    */
    MeshCore::MeshKernel m_Mesh_vis;
    /** @brief mesh containing the fix regions for the local translation */
    MeshCore::MeshKernel m_Mesh_vis2;
    /** @brief triangulation of the CAD-shape */
    MeshCore::MeshKernel m_CadMesh;
    /** @brief vector containing translation-vectors at all mesh-points */
    std::vector<Base::Vector3f> m_dist_vec;

private:

    /** @brief initial input-shape (CAD-model) */
    TopoDS_Shape m_Shape;
    /** @brief vector containing the maximum curvature-radius-values at all
               mesh-points */
    std::vector<double> m_CurvPos;
    /** @brief vector containing the minimum curvature-radius-values at all
               mesh-points */
    std::vector<double> m_CurvNeg;
    /** @brief */
    std::vector<double> m_CurvMax;
    /** @brief struct-vector over all edges */
    std::vector<EdgeStruct> m_EdgeStruct;
    /** @brief index-vector for the region-growing-algorithm containing the
               triangles at the current ring-neighbourhood */
    std::vector<unsigned long> m_RingVector;
    /** @brief index-vector for the region-growing-algorithm containing the
               triangles of one computed region */
    std::vector< std::vector<unsigned long> > m_RegionVector;
    /** @brief index-vector for the region-growing-algorithm containing the
               triangles of all computed region */
    std::vector< std::vector<unsigned long> > m_Regions;
    /** @brief */
    //std::vector<MeshCore::MeshFacet> m_RegionBounds;
    /** @brief external setting-parameters */
    CuttingToolsSettings m_set;
    /** @brief index which specifies the current ring-neighbourhood for the
               region-growing-algorithm  */
    int m_RingCurrent;
private:
    /** @brief vector over all input-meshes for the iteration-process */
    std::vector<MeshCore::MeshKernel> m_MeshVec;
    /** @brief copy of the initial input-mesh */
    MeshCore::MeshKernel MeshRef;
    /** @brief */
    //MeshCore::MeshKernel m_Mesh2Fit;
    /** @brief vector over the normal-vectors at all mesh-points */
    std::vector<Base::Vector3f> m_normals;
    /** @brief vector containing the distance-values at all mesh points
               between the initial input-shape (CAD-model) and -mesh */
    std::vector<double> m_error;
    /** @brief vector containing the initial angle-degrees of all triangles */
    std::vector<double> m_FaceAng;
    /** @brief struct-vector over all mesh-points */
    std::vector<MeshPnt> m_MeshStruct;
    /** @brief vector containing the resulting offset-values of all mesh
               points */
    std::vector<double> m_Offset;
public:
    /** @brief map which links mesh-point to mesh-index */
    std::map<Base::Vector3f,MeshPnt,MeshPntLess > MeshMap;
    /** @brief map over all edges */
    std::map<TopoDS_Edge, std::vector<double>, Edge_Less> EdgeMap;

    /** @brief vector containing the user-specified faces which stands fix
               during the springback-correction */
    std::vector<TopoDS_Face> m_FixFaces;
};

#endif
