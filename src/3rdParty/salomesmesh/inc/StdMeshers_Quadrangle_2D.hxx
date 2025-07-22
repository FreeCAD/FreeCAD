// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  File   : StdMeshers_Quadrangle_2D.hxx
//           Moved here from SMESH_Quadrangle_2D.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH

#ifndef _SMESH_QUADRANGLE_2D_HXX_
#define _SMESH_QUADRANGLE_2D_HXX_

#include "SMESH_Algo.hxx"
#include "SMESH_ProxyMesh.hxx"
#include "SMESH_StdMeshers.hxx"
#include "StdMeshers_FaceSide.hxx"
#include "StdMeshers_QuadrangleParams.hxx"

#include <TopoDS_Face.hxx>
#include <Bnd_B2d.hxx>

class SMDS_MeshNode;
class SMESH_Mesh;
class SMESH_MesherHelper;
class SMESH_ProxyMesh;
struct uvPtStruct;


enum TSideID { QUAD_BOTTOM_SIDE=0, QUAD_RIGHT_SIDE, QUAD_TOP_SIDE, QUAD_LEFT_SIDE, NB_QUAD_SIDES };

typedef uvPtStruct UVPtStruct;
struct FaceQuadStruct
{
  struct Side // a side of FaceQuadStruct
  {
    struct Contact // contact of two sides
    {
      int   point; // index of a grid point of this side where two sides meat
      Side* other_side;
      int   other_point;
    };
    StdMeshers_FaceSidePtr grid;
    int                    from, to;     // indices of grid points used by the quad
    int                    di;           // +1 or -1 depending on IsReversed()
    std::set<int>          forced_nodes; // indices of forced grid points
    std::vector<Contact>   contacts;     // contacts with sides of other quads
    int                    nbNodeOut;    // nb of missing nodes on an opposite shorter side

    Side(StdMeshers_FaceSidePtr theGrid = StdMeshers_FaceSidePtr());
    Side& operator=(const Side& otherSide);
    operator StdMeshers_FaceSidePtr() { return grid; }
    operator const StdMeshers_FaceSidePtr() const { return grid; }
    void AddContact( int ip, Side* side, int iop );
    int  ToSideIndex( int quadNodeIndex ) const;
    int  ToQuadIndex( int sideNodeIndex ) const;
    bool IsForced( int nodeIndex ) const;
    bool IsReversed() const { return nbNodeOut ? false : to < from; }
    bool Reverse(bool keepGrid);
    int  NbPoints() const { return Abs( to - from ); }
    double Param( int nodeIndex ) const;
    double Length( int from=-1, int to=-1) const;
    gp_XY Value2d( double x ) const;
    const UVPtStruct& First() const { return GetUVPtStruct()[ from ]; }
    const UVPtStruct& Last()  const {
      return GetUVPtStruct()[ to-nbNodeOut-(IsReversed() ? -1 : +1)];
    }
    // some sortcuts
    const vector<UVPtStruct>& GetUVPtStruct(bool isXConst=0, double constValue=0) const
    { return nbNodeOut ?
        grid->SimulateUVPtStruct( NbPoints()-nbNodeOut-1, isXConst, constValue ) :
        grid->GetUVPtStruct( isXConst, constValue );
    }
  };
  struct SideIterator // iterator on UVPtStruct of a Side
  {
    const UVPtStruct *uvPtr, *uvEnd;
    int               dPtr, counter;
    SideIterator(): uvPtr(0), uvEnd(0), dPtr(0), counter(0) {}
    void Init( const Side& side ) {
      dPtr  = counter = 0;
      uvPtr = uvEnd = 0;
      if ( side.NbPoints() > 0 ) {
        uvPtr = & side.First();
        uvEnd = & side.Last();
        dPtr  = ( uvEnd > uvPtr ) ? +1 : -1;
        uvEnd += dPtr;
      }
    }
    bool More() const { return uvPtr != uvEnd; }
    void Next() { uvPtr += dPtr; ++counter; }
    UVPtStruct& UVPt() const { return (UVPtStruct&) *uvPtr; }
    UVPtStruct& operator[](int i) { return (UVPtStruct&) uvPtr[ i*dPtr]; }
    int  Count() const { return counter; }
  };

  std::vector< Side >      side;
  std::vector< UVPtStruct> uv_grid;
  int                      iSize, jSize;
  TopoDS_Face              face;
  Bnd_B2d                  uv_box;
  std::string              name; // to ease debugging

  FaceQuadStruct ( const TopoDS_Face& F = TopoDS_Face(), const std::string& nm="main" );
  UVPtStruct& UVPt( int i, int j ) { return uv_grid[ i + j * iSize ]; }
  double&        U( int i, int j ) { return UVPt( i, j ).u; }
  double&        V( int i, int j ) { return UVPt( i, j ).v; }
  void  shift    ( size_t nb, bool keepUnitOri, bool keepGrid=false );
  int & nbNodeOut( int iSide ) { return side[ iSide ].nbNodeOut; }
  bool  findCell ( const gp_XY& uv, int & i, int & j );
  bool  isNear   ( const gp_XY& uv, int & i, int & j, int nbLoops=1 );
  bool  isEqual  ( const gp_XY& uv, int   i, int   j );
  void  normPa2IJ( double x, double y, int & i, int & j );
  void  updateUV ( const gp_XY& uv, int   i, int   j, bool isVertical );

  typedef boost::shared_ptr<FaceQuadStruct> Ptr;
};

class STDMESHERS_EXPORT StdMeshers_Quadrangle_2D: public SMESH_2D_Algo
{
 public:
  StdMeshers_Quadrangle_2D(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_Quadrangle_2D();

  virtual bool CheckHypothesis(SMESH_Mesh&         aMesh,
                               const TopoDS_Shape& aShape,
                               Hypothesis_Status&  aStatus);

  virtual bool Compute(SMESH_Mesh&         aMesh,
                       const TopoDS_Shape& aShape);

  virtual bool Evaluate(SMESH_Mesh &         aMesh,
                        const TopoDS_Shape & aShape,
                        MapShapeNbElems&     aResMap);

  FaceQuadStruct::Ptr CheckAnd2Dcompute(SMESH_Mesh&         aMesh,
                                        const TopoDS_Shape& aShape,
                                        const bool          CreateQuadratic);

  FaceQuadStruct::Ptr CheckNbEdges(SMESH_Mesh&         aMesh,
                                   const TopoDS_Shape& aShape,
                                   const bool          considerMesh=false);

  static bool IsApplicable(const TopoDS_Shape & aShape, bool toCheckAll);

 protected:

  bool checkNbEdgesForEvaluate(SMESH_Mesh& aMesh,
                               const TopoDS_Shape & aShape,
                               MapShapeNbElems& aResMap,
                               std::vector<int>& aNbNodes,
                               bool& IsQuadratic);

  bool setNormalizedGrid(FaceQuadStruct::Ptr quad);

  void splitQuadFace(SMESHDS_Mesh *       theMeshDS,
                     const int            theFaceID,
                     const SMDS_MeshNode* theNode1,
                     const SMDS_MeshNode* theNode2,
                     const SMDS_MeshNode* theNode3,
                     const SMDS_MeshNode* theNode4);

  bool computeQuadDominant(SMESH_Mesh&         aMesh,
                           const TopoDS_Face&  aFace);

  bool computeQuadDominant(SMESH_Mesh&         aMesh,
                           const TopoDS_Face&  aFace,
                           FaceQuadStruct::Ptr quad);

  bool computeQuadPref(SMESH_Mesh&         aMesh,
                       const TopoDS_Face&  aFace,
                       FaceQuadStruct::Ptr quad);

  bool computeTriangles(SMESH_Mesh&         aMesh,
                        const TopoDS_Face&  aFace,
                        FaceQuadStruct::Ptr quad);

  bool evaluateQuadPref(SMESH_Mesh&         aMesh,
                        const TopoDS_Shape& aShape,
                        std::vector<int>&   aNbNodes,
                        MapShapeNbElems&    aResMap,
                        bool                isQuadratic);

  bool computeReduced (SMESH_Mesh&         aMesh,
                       const TopoDS_Face&  aFace,
                       FaceQuadStruct::Ptr quad);

  void updateDegenUV(FaceQuadStruct::Ptr quad);

  void smooth (FaceQuadStruct::Ptr quad);

  bool check();

  int getCorners(const TopoDS_Face&          theFace,
                 SMESH_Mesh &                theMesh,
                 std::list<TopoDS_Edge>&     theWire,
                 std::vector<TopoDS_Vertex>& theVertices,
                 int &                       theNbDegenEdges,
                 const bool                  considerMesh);

  bool getEnforcedUV();

  bool addEnforcedNodes();

  int splitQuad(FaceQuadStruct::Ptr quad, int i, int j);

  void shiftQuad(FaceQuadStruct::Ptr& quad, const int num );

  typedef std::map< StdMeshers_FaceSidePtr, std::vector< FaceQuadStruct::Ptr > > TQuadsBySide;
  void updateSideUV( FaceQuadStruct::Side&  side,
                     int                    iForced,
                     const TQuadsBySide&    quads,
                     int *                  iNext=NULL);


 protected: // Fields

  bool myQuadranglePreference;
  bool myTrianglePreference;
  int  myTriaVertexID;
  bool myNeedSmooth, myCheckOri;
  const StdMeshers_QuadrangleParams* myParams;
  StdMeshers_QuadType                myQuadType;

  SMESH_MesherHelper*                myHelper;
  SMESH_ProxyMesh::Ptr               myProxyMesh;
  std::list< FaceQuadStruct::Ptr >   myQuadList;

  struct ForcedPoint
  {
    gp_XY                uv;
    gp_XYZ               xyz;
    TopoDS_Vertex        vertex;
    const SMDS_MeshNode* node;

    double U() const { return uv.X(); }
    double V() const { return uv.Y(); }
    operator const gp_XY& () { return uv; }
  };
  std::vector< ForcedPoint >         myForcedPnts;
};

#endif
