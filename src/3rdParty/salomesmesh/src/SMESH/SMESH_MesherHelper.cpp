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

// File:      SMESH_MesherHelper.cxx
// Created:   15.02.06 15:22:41
// Author:    Sergey KUUL
//
#include "SMESH_MesherHelper.hxx"

#include "SMDS_EdgePosition.hxx"
#include "SMDS_FaceOfNodes.hxx"
#include "SMDS_FacePosition.hxx" 
#include "SMDS_IteratorOnIterators.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMESH_Block.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_MeshAlgos.hxx"
#include "SMESH_ProxyMesh.hxx"
#include "SMESH_subMesh.hxx"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <Geom2d_Curve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Curve.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_Surface.hxx>
#include <ShapeAnalysis.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_MapIteratorOfMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Trsf.hxx>

#include <Standard_Failure.hxx>
#include <Standard_ErrorHandler.hxx>

#include <utilities.h>

#include <limits>

using namespace std;

#define RETURN_BAD_RESULT(msg) { MESSAGE(msg); return false; }

namespace {

  inline SMESH_TNodeXYZ XYZ(const SMDS_MeshNode* n) { return SMESH_TNodeXYZ(n); }

  enum { U_periodic = 1, V_periodic = 2 };
}

//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

SMESH_MesherHelper::SMESH_MesherHelper(SMESH_Mesh& theMesh)
  : myParIndex(0),
    myMesh(&theMesh),
    myShapeID(0),
    myCreateQuadratic(false),
    myCreateBiQuadratic(false),
    myFixNodeParameters(false)
{
  myPar1[0] = myPar2[0] = myPar1[1] = myPar2[1] = 0;
  mySetElemOnShape = ( ! myMesh->HasShapeToMesh() );
}

//=======================================================================
//function : ~SMESH_MesherHelper
//purpose  : 
//=======================================================================

SMESH_MesherHelper::~SMESH_MesherHelper()
{
  {
    TID2ProjectorOnSurf::iterator i_proj = myFace2Projector.begin();
    for ( ; i_proj != myFace2Projector.end(); ++i_proj )
      delete i_proj->second;
  }
  {
    TID2ProjectorOnCurve::iterator i_proj = myEdge2Projector.begin();
    for ( ; i_proj != myEdge2Projector.end(); ++i_proj )
      delete i_proj->second;
  }
}

//=======================================================================
//function : IsQuadraticSubMesh
//purpose  : Check submesh for given shape: if all elements on this shape 
//           are quadratic, quadratic elements will be created.
//           Also fill myTLinkNodeMap
//=======================================================================

bool SMESH_MesherHelper::IsQuadraticSubMesh(const TopoDS_Shape& aSh)
{
  SMESHDS_Mesh* meshDS = GetMeshDS();
  // we can create quadratic elements only if all elements
  // created on sub-shapes of given shape are quadratic
  // also we have to fill myTLinkNodeMap
  myCreateQuadratic = true;
  mySeamShapeIds.clear();
  myDegenShapeIds.clear();
  TopAbs_ShapeEnum subType( aSh.ShapeType()==TopAbs_FACE ? TopAbs_EDGE : TopAbs_FACE );
  if ( aSh.ShapeType()==TopAbs_COMPOUND )
  {
    TopoDS_Iterator subIt( aSh );
    if ( subIt.More() )
      subType = ( subIt.Value().ShapeType()==TopAbs_FACE ) ? TopAbs_EDGE : TopAbs_FACE;
  }
  SMDSAbs_ElementType elemType( subType==TopAbs_FACE ? SMDSAbs_Face : SMDSAbs_Edge );


  int nbOldLinks = myTLinkNodeMap.size();

  if ( !myMesh->HasShapeToMesh() )
  {
    if (( myCreateQuadratic = myMesh->NbFaces( ORDER_QUADRATIC )))
    {
      SMDS_FaceIteratorPtr fIt = meshDS->facesIterator();
      while ( fIt->more() )
        AddTLinks( static_cast< const SMDS_MeshFace* >( fIt->next() ));
    }
  }
  else
  {
    TopExp_Explorer exp( aSh, subType );
    TopTools_MapOfShape checkedSubShapes;
    for (; exp.More() && myCreateQuadratic; exp.Next()) {
      if ( !checkedSubShapes.Add( exp.Current() ))
        continue; // needed if aSh is compound of solids
      if ( SMESHDS_SubMesh * subMesh = meshDS->MeshElements( exp.Current() )) {
        if ( SMDS_ElemIteratorPtr it = subMesh->GetElements() ) {
          while(it->more()) {
            const SMDS_MeshElement* e = it->next();
            if ( e->GetType() != elemType || !e->IsQuadratic() ) {
              myCreateQuadratic = false;
              break;
            }
            else {
              // fill TLinkNodeMap
              switch ( e->NbCornerNodes() ) {
              case 2:
                AddTLinkNode(e->GetNode(0),e->GetNode(1),e->GetNode(2)); break;
              case 3:
                AddTLinkNode(e->GetNode(0),e->GetNode(1),e->GetNode(3));
                AddTLinkNode(e->GetNode(1),e->GetNode(2),e->GetNode(4));
                AddTLinkNode(e->GetNode(2),e->GetNode(0),e->GetNode(5)); break;
              case 4:
                AddTLinkNode(e->GetNode(0),e->GetNode(1),e->GetNode(4));
                AddTLinkNode(e->GetNode(1),e->GetNode(2),e->GetNode(5));
                AddTLinkNode(e->GetNode(2),e->GetNode(3),e->GetNode(6));
                AddTLinkNode(e->GetNode(3),e->GetNode(0),e->GetNode(7));
                break;
              default:
                myCreateQuadratic = false;
                break;
              }
            }
          }
        }
      }
    }
  }

  // if ( nbOldLinks == myTLinkNodeMap.size() ) -- 0023068
  if ( myTLinkNodeMap.empty() )
    myCreateQuadratic = false;

  if ( !myCreateQuadratic )
    myTLinkNodeMap.clear();

  SetSubShape( aSh );

  return myCreateQuadratic;
}

//=======================================================================
//function : SetSubShape
//purpose  : Set geometry to make elements on
//=======================================================================

void SMESH_MesherHelper::SetSubShape(const int aShID)
{
  if ( aShID == myShapeID )
    return;
  if ( aShID > 0 )
    SetSubShape( GetMeshDS()->IndexToShape( aShID ));
  else
    SetSubShape( TopoDS_Shape() );
}

//=======================================================================
//function : SetSubShape
//purpose  : Set geometry to create elements on
//=======================================================================

void SMESH_MesherHelper::SetSubShape(const TopoDS_Shape& aSh)
{
  if ( myShape.IsSame( aSh ))
    return;

  myShape = aSh;
  mySeamShapeIds.clear();
  myDegenShapeIds.clear();

  if ( myShape.IsNull() ) {
    myShapeID  = 0;
    return;
  }
  SMESHDS_Mesh* meshDS = GetMeshDS();
  myShapeID = meshDS->ShapeToIndex(aSh);
  myParIndex = 0;

  // treatment of periodic faces
  for ( TopExp_Explorer eF( aSh, TopAbs_FACE ); eF.More(); eF.Next() )
  {
    const TopoDS_Face& face = TopoDS::Face( eF.Current() );
    BRepAdaptor_Surface surf( face, false );
    if ( surf.IsUPeriodic() || surf.IsUClosed() ) {
      myParIndex |= U_periodic;
      myPar1[0] = surf.FirstUParameter();
      myPar2[0] = surf.LastUParameter();
    }
    if ( surf.IsVPeriodic() || surf.IsVClosed() ) {
      myParIndex |= V_periodic;
      myPar1[1] = surf.FirstVParameter();
      myPar2[1] = surf.LastVParameter();
    }

    gp_Pnt2d uv1, uv2;
    for (TopExp_Explorer exp( face, TopAbs_EDGE ); exp.More(); exp.Next())
    {
      // look for a "seam" edge, a real seam or an edge on period boundary
      TopoDS_Edge edge = TopoDS::Edge( exp.Current() );
      const int edgeID = meshDS->ShapeToIndex( edge );
      if ( myParIndex )
      {
        BRep_Tool::UVPoints( edge, face, uv1, uv2 );
        const double du = Abs( uv1.Coord(1) - uv2.Coord(1) );
        const double dv = Abs( uv1.Coord(2) - uv2.Coord(2) );

        bool isSeam = BRep_Tool::IsClosed( edge, face );
        if ( isSeam ) // real seam - having two pcurves on face
        {
          // pcurve can lie not on pediod boundary (22582, mesh_Quadratic_01/C9)
          if ( du < dv )
          {
            double u1 = uv1.Coord(1);
            edge.Reverse();
            BRep_Tool::UVPoints( edge, face, uv1, uv2 );
            double u2 = uv1.Coord(1);
            myPar1[0] = Min( u1, u2 );
            myPar2[0] = Max( u1, u2 );
          }
          else
          {
            double v1 = uv1.Coord(2);
            edge.Reverse();
            BRep_Tool::UVPoints( edge, face, uv1, uv2 );
            double v2 = uv1.Coord(2);
            myPar1[1] = Min( v1, v2 );
            myPar2[1] = Max( v1, v2 );
          }
        }
        else //if ( !isSeam )
        {
          // one pcurve but on period boundary (22772, mesh_Quadratic_01/D1)
          if      (( myParIndex & U_periodic ) && du < Precision::PConfusion() )
          {
            isSeam = ( Abs( uv1.Coord(1) - myPar1[0] ) < Precision::PConfusion() ||
                       Abs( uv1.Coord(1) - myPar2[0] ) < Precision::PConfusion() );
          }
          else if (( myParIndex & V_periodic ) && dv < Precision::PConfusion() )
          {
            isSeam = ( Abs( uv1.Coord(2) - myPar1[1] ) < Precision::PConfusion() ||
                       Abs( uv1.Coord(2) - myPar2[1] ) < Precision::PConfusion() );
          }
          if ( isSeam ) // vertices are on period boundary, check a middle point (23032)
          {
            double f,l, r = 0.2345;
            Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface( edge, face, f, l );
            uv2 = C2d->Value( f * r + l * ( 1.-r ));
            if ( du < Precision::PConfusion() )
              isSeam = ( Abs( uv1.Coord(1) - uv2.Coord(1) ) < Precision::PConfusion() );
            else
              isSeam = ( Abs( uv1.Coord(2) - uv2.Coord(2) ) < Precision::PConfusion() );
          }
        }
        if ( isSeam )
        {
          // store seam shape indices, negative if shape encounters twice ('real seam')
          mySeamShapeIds.insert( IsSeamShape( edgeID ) ? -edgeID : edgeID );
          for ( TopExp_Explorer v( edge, TopAbs_VERTEX ); v.More(); v.Next() ) {
            int vertexID = meshDS->ShapeToIndex( v.Current() );
            mySeamShapeIds.insert( IsSeamShape( vertexID ) ? -vertexID : vertexID );
          }
        }
      }
      // look for a degenerated edge
      if ( SMESH_Algo::isDegenerated( edge )) {
        myDegenShapeIds.insert( edgeID );
        for ( TopExp_Explorer v( edge, TopAbs_VERTEX ); v.More(); v.Next() )
          myDegenShapeIds.insert( meshDS->ShapeToIndex( v.Current() ));
      }
      if ( !BRep_Tool::SameParameter( edge ) ||
           !BRep_Tool::SameRange( edge ))
      {
        setPosOnShapeValidity( edgeID, false );
      }
    }
  }
}

//=======================================================================
//function : GetNodeUVneedInFaceNode
//purpose  : Check if inFaceNode argument is necessary for call GetNodeUV(F,..)
//           Return true if the face is periodic.
//           If F is Null, answer about sub-shape set through IsQuadraticSubMesh() or
//           * SetSubShape()
//=======================================================================

bool SMESH_MesherHelper::GetNodeUVneedInFaceNode(const TopoDS_Face& F) const
{
  if ( F.IsNull() ) return !mySeamShapeIds.empty();

  if ( !F.IsNull() && !myShape.IsNull() && myShape.IsSame( F ))
    return !mySeamShapeIds.empty();

  TopLoc_Location loc;
  Handle(Geom_Surface) aSurface = BRep_Tool::Surface( F,loc );
  if ( !aSurface.IsNull() )
    return ( aSurface->IsUPeriodic() || aSurface->IsVPeriodic() );

  return false;
}

//=======================================================================
//function : IsMedium
//purpose  : 
//=======================================================================

bool SMESH_MesherHelper::IsMedium(const SMDS_MeshNode*      node,
                                  const SMDSAbs_ElementType typeToCheck)
{
  return SMESH_MeshEditor::IsMedium( node, typeToCheck );
}

//=======================================================================
//function : GetSubShapeByNode
//purpose  : Return support shape of a node
//=======================================================================

TopoDS_Shape SMESH_MesherHelper::GetSubShapeByNode(const SMDS_MeshNode* node,
                                                   const SMESHDS_Mesh*  meshDS)
{
  int shapeID = node ? node->getshapeId() : 0;
  if ( 0 < shapeID && shapeID <= meshDS->MaxShapeIndex() )
    return meshDS->IndexToShape( shapeID );
  else
    return TopoDS_Shape();
}


//=======================================================================
//function : AddTLinkNode
//purpose  : add a link in my data structure
//=======================================================================

void SMESH_MesherHelper::AddTLinkNode(const SMDS_MeshNode* n1,
                                      const SMDS_MeshNode* n2,
                                      const SMDS_MeshNode* n12)
{
  // add new record to map
  SMESH_TLink link( n1, n2 );
  myTLinkNodeMap.insert( make_pair(link,n12));
}

//================================================================================
/*!
 * \brief Add quadratic links of edge to own data structure
 */
//================================================================================

bool SMESH_MesherHelper::AddTLinks(const SMDS_MeshEdge* edge)
{
  if ( edge && edge->IsQuadratic() )
    AddTLinkNode(edge->GetNode(0), edge->GetNode(1), edge->GetNode(2));
  else
    return false;
  return true;
}

//================================================================================
/*!
 * \brief Add quadratic links of face to own data structure
 */
//================================================================================

bool SMESH_MesherHelper::AddTLinks(const SMDS_MeshFace* f)
{
  bool isQuad = true;
  if ( !f->IsPoly() )
    switch ( f->NbNodes() ) {
    case 7:
      // myMapWithCentralNode.insert
      //   ( make_pair( TBiQuad( f->GetNode(0),f->GetNode(1),f->GetNode(2) ),
      //                f->GetNode(6)));
      // break; -- add medium nodes as well
    case 6:
      AddTLinkNode(f->GetNode(0),f->GetNode(1),f->GetNode(3));
      AddTLinkNode(f->GetNode(1),f->GetNode(2),f->GetNode(4));
      AddTLinkNode(f->GetNode(2),f->GetNode(0),f->GetNode(5)); break;

    case 9:
      // myMapWithCentralNode.insert
      //   ( make_pair( TBiQuad( f->GetNode(0),f->GetNode(1),f->GetNode(2),f->GetNode(3) ),
      //                f->GetNode(8)));
      // break; -- add medium nodes as well
    case 8:
      AddTLinkNode(f->GetNode(0),f->GetNode(1),f->GetNode(4));
      AddTLinkNode(f->GetNode(1),f->GetNode(2),f->GetNode(5));
      AddTLinkNode(f->GetNode(2),f->GetNode(3),f->GetNode(6));
      AddTLinkNode(f->GetNode(3),f->GetNode(0),f->GetNode(7)); break;
    default:;
      isQuad = false;
    }
  return isQuad;
}

//================================================================================
/*!
 * \brief Add quadratic links of volume to own data structure
 */
//================================================================================

bool SMESH_MesherHelper::AddTLinks(const SMDS_MeshVolume* volume)
{
  if ( volume->IsQuadratic() )
  {
    SMDS_VolumeTool vTool( volume );
    const SMDS_MeshNode** nodes = vTool.GetNodes();
    set<int> addedLinks;
    for ( int iF = 1; iF < vTool.NbFaces(); ++iF )
    {
      const int nbN = vTool.NbFaceNodes( iF );
      const int* iNodes = vTool.GetFaceNodesIndices( iF );
      for ( int i = 0; i < nbN; )
      {
        int iN1  = iNodes[i++];
        int iN12 = iNodes[i++];
        int iN2  = iNodes[i];
        if ( iN1 > iN2 ) std::swap( iN1, iN2 );
        int linkID = iN1 * vTool.NbNodes() + iN2;
        pair< set<int>::iterator, bool > it_isNew = addedLinks.insert( linkID );
        if ( it_isNew.second )
          AddTLinkNode( nodes[iN1], nodes[iN2], nodes[iN12] );
        else
          addedLinks.erase( it_isNew.first ); // each link encounters only twice
      }
      if ( vTool.NbNodes() == 27 )
      {
        const SMDS_MeshNode* nFCenter = nodes[ vTool.GetCenterNodeIndex( iF )];
        if ( nFCenter->GetPosition()->GetTypeOfPosition() == SMDS_TOP_3DSPACE )
          myMapWithCentralNode.insert
            ( make_pair( TBiQuad( nodes[ iNodes[0]], nodes[ iNodes[1]],
                                  nodes[ iNodes[2]], nodes[ iNodes[3]] ),
                         nFCenter ));
      }
    }
    return true;
  }
  return false;
}

//================================================================================
/*!
 * \brief Return true if position of nodes on the shape hasn't yet been checked or
 * the positions proved to be invalid
 */
//================================================================================

bool SMESH_MesherHelper::toCheckPosOnShape(int shapeID ) const
{
  map< int,bool >::const_iterator id_ok = myNodePosShapesValidity.find( shapeID );
  return ( id_ok == myNodePosShapesValidity.end() || !id_ok->second );
}

//================================================================================
/*!
 * \brief Set validity of positions of nodes on the shape.
 * Once set, validity is not changed
 */
//================================================================================

void SMESH_MesherHelper::setPosOnShapeValidity(int shapeID, bool ok ) const
{
  std::map< int,bool >::iterator sh_ok = 
    ((SMESH_MesherHelper*)this)->myNodePosShapesValidity.insert( make_pair( shapeID, ok)).first;
  if ( !ok )
    sh_ok->second = ok;
}

//=======================================================================
//function : ToFixNodeParameters
//purpose  : Enables fixing node parameters on EDGEs and FACEs in 
//           GetNodeU(...,check=true), GetNodeUV(...,check=true), CheckNodeUV() and
//           CheckNodeU() in case if a node lies on a shape set via SetSubShape().
//           Default is False
//=======================================================================

void SMESH_MesherHelper::ToFixNodeParameters(bool toFix)
{
  myFixNodeParameters = toFix;
}


//=======================================================================
//function : getUVOnSeam
//purpose  : Select UV on either of 2 pcurves of a seam edge, closest to the given UV
//=======================================================================

gp_Pnt2d SMESH_MesherHelper::getUVOnSeam( const gp_Pnt2d& uv1, const gp_Pnt2d& uv2 ) const
{
  gp_Pnt2d result = uv1;
  for ( int i = U_periodic; i <= V_periodic ; ++i )
  {
    if ( myParIndex & i )
    {
      double p1 = uv1.Coord( i );
      double dp1 = Abs( p1-myPar1[i-1]), dp2 = Abs( p1-myPar2[i-1]);
      if ( myParIndex == i ||
           dp1 < ( myPar2[i-1] - myPar1[i-1] ) / 100. ||
           dp2 < ( myPar2[i-1] - myPar1[i-1] ) / 100. )
      {
        double p2 = uv2.Coord( i );
        double p1Alt = ( dp1 < dp2 ) ? myPar2[i-1] : myPar1[i-1];
        if ( Abs( p2 - p1 ) > Abs( p2 - p1Alt ))
          result.SetCoord( i, p1Alt );
      }
    }
  }
  return result;
}

//=======================================================================
//function : GetNodeUV
//purpose  : Return node UV on face
//=======================================================================

gp_XY SMESH_MesherHelper::GetNodeUV(const TopoDS_Face&   F,
                                    const SMDS_MeshNode* n,
                                    const SMDS_MeshNode* n2,
                                    bool*                check) const
{
  gp_Pnt2d uv( Precision::Infinite(), Precision::Infinite() );

  const SMDS_PositionPtr Pos = n->GetPosition();
  bool uvOK = false;
  if ( Pos->GetTypeOfPosition() == SMDS_TOP_FACE )
  {
    // node has position on face
    const SMDS_FacePosition* fpos = static_cast<const SMDS_FacePosition*>( Pos );
    uv.SetCoord( fpos->GetUParameter(), fpos->GetVParameter() );
    if ( check )
      uvOK = CheckNodeUV( F, n, uv.ChangeCoord(), 2.*getFaceMaxTol( F )); // 2. from 22830
  }
  else if ( Pos->GetTypeOfPosition() == SMDS_TOP_EDGE )
  {
    // node has position on EDGE => it is needed to find
    // corresponding EDGE from FACE, get pcurve for this
    // EDGE and retrieve value from this pcurve
    const SMDS_EdgePosition* epos = static_cast<const SMDS_EdgePosition*>( Pos );
    const int              edgeID = n->getshapeId();
    const TopoDS_Edge& E = TopoDS::Edge( GetMeshDS()->IndexToShape( edgeID ));
    double f, l, u = epos->GetUParameter();
    Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface( E, F, f, l );
    bool validU = ( !C2d.IsNull() && ( f < u ) && ( u < l ));
    if ( validU ) uv = C2d->Value( u );
    else          uv.SetCoord( Precision::Infinite(),0.);
    if ( check || !validU )
      uvOK = CheckNodeUV( F, n, uv.ChangeCoord(), 2.*getFaceMaxTol( F ),/*force=*/ !validU );

    // for a node on a seam EDGE select one of UVs on 2 pcurves
    if ( n2 && IsSeamShape( edgeID ))
    {
      uv = getUVOnSeam( uv, GetNodeUV( F, n2, 0, check ));
    }
    else
    { // adjust uv to period
      TopLoc_Location loc;
      Handle(Geom_Surface) S = BRep_Tool::Surface(F,loc);
      Standard_Boolean isUPeriodic = S->IsUPeriodic();
      Standard_Boolean isVPeriodic = S->IsVPeriodic();
      gp_Pnt2d newUV = uv;
      if ( isUPeriodic || isVPeriodic ) {
        Standard_Real UF,UL,VF,VL;
        S->Bounds(UF,UL,VF,VL);
        if ( isUPeriodic ) newUV.SetX( uv.X() + ShapeAnalysis::AdjustToPeriod(uv.X(),UF,UL));
        if ( isVPeriodic ) newUV.SetY( uv.Y() + ShapeAnalysis::AdjustToPeriod(uv.Y(),VF,VL));

        if ( n2 )
        {
          gp_Pnt2d uv2 = GetNodeUV( F, n2, 0, check );
          if ( isUPeriodic && Abs( uv.X()-uv2.X() ) < Abs( newUV.X()-uv2.X() ))
            newUV.SetX( uv.X() );
          if ( isVPeriodic && Abs( uv.Y()-uv2.Y() ) < Abs( newUV.Y()-uv2.Y() ))
            newUV.SetY( uv.Y() );
        }
      }
      uv = newUV;
    }
  }
  else if ( Pos->GetTypeOfPosition() == SMDS_TOP_VERTEX )
  {
    if ( int vertexID = n->getshapeId() ) {
      const TopoDS_Vertex& V = TopoDS::Vertex(GetMeshDS()->IndexToShape(vertexID));
      try {
        uv = BRep_Tool::Parameters( V, F );
        uvOK = true;
      }
      catch (Standard_Failure& exc) {
      }
      if ( !uvOK )
      {
        if ( !IsSubShape( V, F ))
        {
          MESSAGE ( "SMESH_MesherHelper::GetNodeUV(); Vertex " << vertexID
                    << " not in face " << GetMeshDS()->ShapeToIndex( F ) );
          // get UV of a vertex closest to the node
          double dist = 1e100;
          gp_Pnt pn = XYZ( n );
          for ( TopExp_Explorer vert( F,TopAbs_VERTEX ); !uvOK && vert.More(); vert.Next() ) {
            TopoDS_Vertex curV = TopoDS::Vertex( vert.Current() );
            gp_Pnt p = BRep_Tool::Pnt( curV );
            double curDist = p.SquareDistance( pn );
            if ( curDist < dist ) {
              dist = curDist;
              uv = BRep_Tool::Parameters( curV, F );
              uvOK = ( dist < DBL_MIN );
            }
          }
        }
        else
        {
          uvOK = false;
          TopTools_ListIteratorOfListOfShape it( myMesh->GetAncestors( V ));
          for ( ; it.More(); it.Next() ) {
            if ( it.Value().ShapeType() == TopAbs_EDGE ) {
              const TopoDS_Edge & edge = TopoDS::Edge( it.Value() );
              double f,l;
              Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface(edge, F, f, l);
              if ( !C2d.IsNull() ) {
                double u = ( V == IthVertex( 0, edge )) ?  f : l;
                uv = C2d->Value( u );
                uvOK = true;
                break;
              }
            }
          }
          if ( !uvOK && V.Orientation() == TopAbs_INTERNAL )
          {
            Handle(ShapeAnalysis_Surface) projector = GetSurface( F );
            if ( n2 ) uv = GetNodeUV( F, n2 );
            if ( Precision::IsInfinite( uv.X() ))
              uv = projector->NextValueOfUV( uv, BRep_Tool::Pnt( V ), BRep_Tool::Tolerance( F ));
            else
              uv = projector->ValueOfUV( BRep_Tool::Pnt( V ), BRep_Tool::Tolerance( F ));
            uvOK = ( projector->Gap() < getFaceMaxTol( F ));
          }
        }
      }
      if ( n2 && IsSeamShape( vertexID ))
      {
        bool isSeam = ( myShape.IsSame( F ));
        if ( !isSeam ) {
          SMESH_MesherHelper h( *myMesh );
          h.SetSubShape( F );
          isSeam = IsSeamShape( vertexID );
        }

        if ( isSeam )
          uv = getUVOnSeam( uv, GetNodeUV( F, n2, 0 ));
      }
    }
  }
  else
  {
    uvOK = CheckNodeUV( F, n, uv.ChangeCoord(), 2.*getFaceMaxTol( F ));
  }

  if ( check && !uvOK )
    *check = uvOK;

  return uv.XY();
}

//=======================================================================
//function : CheckNodeUV
//purpose  : Check and fix node UV on a face
//=======================================================================

bool SMESH_MesherHelper::CheckNodeUV(const TopoDS_Face&   F,
                                     const SMDS_MeshNode* n,
                                     gp_XY&               uv,
                                     const double         tol,
                                     const bool           force,
                                     double               distXYZ[4]) const
{
  int  shapeID = n->getshapeId();
  bool infinit;
  if (( infinit = ( Precision::IsInfinite( uv.X() ) || Precision::IsInfinite( uv.Y() ))) ||
      ( force ) ||
      ( uv.X() == 0. && uv.Y() == 0. ) ||
      ( toCheckPosOnShape( shapeID )))
  {
    // check that uv is correct
    TopLoc_Location loc;
    Handle(Geom_Surface) surface = BRep_Tool::Surface( F,loc );
    gp_Pnt nodePnt = XYZ( n ), surfPnt(0,0,0);
    double dist = 0;
    if ( !loc.IsIdentity() ) nodePnt.Transform( loc.Transformation().Inverted() );
    if ( infinit ||
         (dist = nodePnt.Distance( surfPnt = surface->Value( uv.X(), uv.Y() ))) > tol )
    {
      setPosOnShapeValidity( shapeID, false );
      if ( !infinit && distXYZ ) {
        surfPnt.Transform( loc );
        distXYZ[0] = dist;
        distXYZ[1] = surfPnt.X(); distXYZ[2] = surfPnt.Y(); distXYZ[3]=surfPnt.Z();
      }
      // uv incorrect, project the node to surface
      GeomAPI_ProjectPointOnSurf& projector = GetProjector( F, loc, tol );
      projector.Perform( nodePnt );
      if ( !projector.IsDone() || projector.NbPoints() < 1 )
      {
        MESSAGE( "SMESH_MesherHelper::CheckNodeUV() failed to project" );
        return false;
      }
      Standard_Real U,V;
      projector.LowerDistanceParameters(U,V);
      uv.SetCoord( U,V );
      surfPnt = surface->Value( U, V );
      dist = nodePnt.Distance( surfPnt );
      if ( distXYZ ) {
        surfPnt.Transform( loc );
        distXYZ[0] = dist;
        distXYZ[1] = surfPnt.X(); distXYZ[2] = surfPnt.Y(); distXYZ[3]=surfPnt.Z();
      }
      if ( dist > tol )
      {
        MESSAGE( "SMESH_MesherHelper::CheckNodeUV(), invalid projection" );
        return false;
      }
      // store the fixed UV on the face
      if ( myShape.IsSame(F) && shapeID == myShapeID && myFixNodeParameters )
        const_cast<SMDS_MeshNode*>(n)->SetPosition
          ( SMDS_PositionPtr( new SMDS_FacePosition( U, V )));
    }
    else if ( myShape.IsSame(F) && uv.Modulus() > numeric_limits<double>::min() )
    {
      setPosOnShapeValidity( shapeID, true );
    }
  }
  return true;
}

//=======================================================================
//function : GetProjector
//purpose  : Return projector intitialized by given face without location, which is returned
//=======================================================================

GeomAPI_ProjectPointOnSurf& SMESH_MesherHelper::GetProjector(const TopoDS_Face& F,
                                                             TopLoc_Location&   loc,
                                                             double             tol ) const
{
  Handle(Geom_Surface) surface = BRep_Tool::Surface( F,loc );
  int faceID = GetMeshDS()->ShapeToIndex( F );
  TID2ProjectorOnSurf& i2proj = const_cast< TID2ProjectorOnSurf&>( myFace2Projector );
  TID2ProjectorOnSurf::iterator i_proj = i2proj.find( faceID );
  if ( i_proj == i2proj.end() )
  {
    if ( tol == 0 ) tol = BRep_Tool::Tolerance( F );
    double U1, U2, V1, V2;
    surface->Bounds(U1, U2, V1, V2);
    GeomAPI_ProjectPointOnSurf* proj = new GeomAPI_ProjectPointOnSurf();
    proj->Init( surface, U1, U2, V1, V2, tol );
    i_proj = i2proj.insert( make_pair( faceID, proj )).first;
  }
  return *( i_proj->second );
}

//=======================================================================
//function : GetSurface
//purpose  : Return a cached ShapeAnalysis_Surface of a FACE
//=======================================================================

Handle(ShapeAnalysis_Surface) SMESH_MesherHelper::GetSurface(const TopoDS_Face& F ) const
{
  Handle(Geom_Surface) surface = BRep_Tool::Surface( F );
  int faceID = GetMeshDS()->ShapeToIndex( F );
  TID2Surface::iterator i_surf = myFace2Surface.find( faceID );
  if ( i_surf == myFace2Surface.end() && faceID )
  {
    Handle(ShapeAnalysis_Surface) surf( new ShapeAnalysis_Surface( surface ));
    i_surf = myFace2Surface.insert( make_pair( faceID, surf )).first;
  }
  return i_surf->second;
}

namespace
{
  gp_XY AverageUV(const gp_XY& uv1, const gp_XY& uv2) { return ( uv1 + uv2 ) / 2.; }
  gp_XY_FunPtr(Added); // define gp_XY_Added pointer to function calling gp_XY::Added(gp_XY)
  gp_XY_FunPtr(Subtracted); 
}

//=======================================================================
//function : ApplyIn2D
//purpose  : Perform given operation on two 2d points in parameric space of given surface.
//           It takes into account period of the surface. Use gp_XY_FunPtr macro
//           to easily define pointer to function of gp_XY class.
//=======================================================================

gp_XY SMESH_MesherHelper::ApplyIn2D(Handle(Geom_Surface) surface,
                                    const gp_XY&         uv1,
                                    const gp_XY&         uv2,
                                    xyFunPtr             fun,
                                    const bool           resultInPeriod)
{
  if ( surface->IsKind(STANDARD_TYPE(Geom_RectangularTrimmedSurface )))
    surface = Handle(Geom_RectangularTrimmedSurface)::DownCast( surface )->BasisSurface();
  Standard_Boolean isUPeriodic = surface.IsNull() ? false : surface->IsUPeriodic();
  Standard_Boolean isVPeriodic = surface.IsNull() ? false : surface->IsVPeriodic();
  if ( !isUPeriodic && !isVPeriodic )
    return fun(uv1,uv2);

  // move uv2 not far than half-period from uv1
  double u2 = 
    uv2.X()+(isUPeriodic ? ShapeAnalysis::AdjustByPeriod(uv2.X(),uv1.X(),surface->UPeriod()) :0);
  double v2 = 
    uv2.Y()+(isVPeriodic ? ShapeAnalysis::AdjustByPeriod(uv2.Y(),uv1.Y(),surface->VPeriod()) :0);

  // execute operation
  gp_XY res = fun( uv1, gp_XY(u2,v2) );

  // move result within period
  if ( resultInPeriod )
  {
    Standard_Real UF,UL,VF,VL;
    surface->Bounds(UF,UL,VF,VL);
    if ( isUPeriodic )
      res.SetX( res.X() + ShapeAnalysis::AdjustToPeriod(res.X(),UF,UL));
    if ( isVPeriodic )
      res.SetY( res.Y() + ShapeAnalysis::AdjustToPeriod(res.Y(),VF,VL));
  }

  return res;
}

//=======================================================================
//function : AdjustByPeriod
//purpose  : Move node positions on a FACE within surface period
//=======================================================================

void SMESH_MesherHelper::AdjustByPeriod( const TopoDS_Face& face, gp_XY uv[], const int nbUV )
{
  SMESH_MesherHelper h( *myMesh ), *ph = face.IsSame( myShape ) ? this : &h;
  ph->SetSubShape( face );

  for ( int iCoo = U_periodic; iCoo <= V_periodic; ++iCoo )
    if ( ph->GetPeriodicIndex() & iCoo )
    {
      const double period = ( ph->myPar2[iCoo-1] - ph->myPar1[iCoo-1] );
      const double xRef = uv[0].Coord( iCoo );
      for ( int i = 1; i < nbUV; ++i )
      {
        double x = uv[i].Coord( iCoo );
        double dx = ShapeAnalysis::AdjustByPeriod( x, xRef, period );
        uv[i].SetCoord( iCoo, x + dx );
      }
    }
}

//=======================================================================
//function : GetMiddleUV
//purpose  : Return middle UV taking in account surface period
//=======================================================================

gp_XY SMESH_MesherHelper::GetMiddleUV(const Handle(Geom_Surface)& surface,
                                      const gp_XY&                p1,
                                      const gp_XY&                p2)
{
  // NOTE:
  // the proper place of getting basic surface seems to be in ApplyIn2D()
  // but we put it here to decrease a risk of regressions just before releasing a version
  // Handle(Geom_Surface) surf = surface;
  // while ( !surf.IsNull() && surf->IsKind(STANDARD_TYPE(Geom_RectangularTrimmedSurface )))
  //   surf = Handle(Geom_RectangularTrimmedSurface)::DownCast( surf )->BasisSurface();

  return ApplyIn2D( surface, p1, p2, & AverageUV );
}

//=======================================================================
//function : GetCenterUV
//purpose  : Return UV for the central node of a biquadratic triangle
//=======================================================================

gp_XY SMESH_MesherHelper::GetCenterUV(const gp_XY& uv1,
                                      const gp_XY& uv2, 
                                      const gp_XY& uv3, 
                                      const gp_XY& uv12,
                                      const gp_XY& uv23,
                                      const gp_XY& uv31,
                                      bool *       isBadTria/*=0*/)
{
  bool badTria;
  gp_XY uvAvg = ( uv12 + uv23 + uv31 ) / 3.;

  if      (( badTria = (( uvAvg - uv1 ) * ( uvAvg - uv23 ) > 0 )))
    uvAvg = ( uv1 + uv23 ) / 2.;
  else if (( badTria = (( uvAvg - uv2 ) * ( uvAvg - uv31 ) > 0 )))
    uvAvg = ( uv2 + uv31 ) / 2.;
  else if (( badTria = (( uvAvg - uv3 ) * ( uvAvg - uv12 ) > 0 )))
    uvAvg = ( uv3 + uv12 ) / 2.;

  if ( isBadTria )
    *isBadTria = badTria;
  return uvAvg;
}

//=======================================================================
//function : GetNodeU
//purpose  : Return node U on edge
//=======================================================================

double SMESH_MesherHelper::GetNodeU(const TopoDS_Edge&   E,
                                    const SMDS_MeshNode* n,
                                    const SMDS_MeshNode* inEdgeNode,
                                    bool*                check) const
{
  double param = Precision::Infinite();

  const SMDS_PositionPtr pos = n->GetPosition();
  if ( pos->GetTypeOfPosition()==SMDS_TOP_EDGE )
  {
    const SMDS_EdgePosition* epos = static_cast<const SMDS_EdgePosition*>( pos );
    param =  epos->GetUParameter();
  }
  else if( pos->GetTypeOfPosition() == SMDS_TOP_VERTEX )
  {
    if ( inEdgeNode && TopExp::FirstVertex( E ).IsSame( TopExp::LastVertex( E ))) // issue 0020128
    {
      Standard_Real f,l;
      BRep_Tool::Range( E, f,l );
      double uInEdge = GetNodeU( E, inEdgeNode );
      param = ( fabs( uInEdge - f ) < fabs( l - uInEdge )) ? f : l;
    }
    else
    {
      SMESHDS_Mesh * meshDS = GetMeshDS();
      int vertexID = n->getshapeId();
      const TopoDS_Vertex& V = TopoDS::Vertex(meshDS->IndexToShape(vertexID));
      param =  BRep_Tool::Parameter( V, E );
    }
  }
  if ( check )
  {
    double tol = BRep_Tool::Tolerance( E );
    double f,l;  BRep_Tool::Range( E, f,l );
    bool force = ( param < f-tol || param > l+tol );
    if ( !force && pos->GetTypeOfPosition()==SMDS_TOP_EDGE )
      force = ( GetMeshDS()->ShapeToIndex( E ) != n->getshapeId() );

    *check = CheckNodeU( E, n, param, 2*tol, force );
  }
  return param;
}

//=======================================================================
//function : CheckNodeU
//purpose  : Check and fix node U on an edge
//           Return false if U is bad and could not be fixed
//=======================================================================

bool SMESH_MesherHelper::CheckNodeU(const TopoDS_Edge&   E,
                                    const SMDS_MeshNode* n,
                                    double&              u,
                                    const double         tol,
                                    const bool           force,
                                    double               distXYZ[4]) const
{
  int  shapeID = n->getshapeId();
  bool infinit;
  if (( infinit = Precision::IsInfinite( u )) ||
      ( force ) ||
      ( u == 0. ) ||
      ( toCheckPosOnShape( shapeID )))
  {
    TopLoc_Location loc; double f,l;
    Handle(Geom_Curve) curve = BRep_Tool::Curve( E,loc,f,l );
    if ( curve.IsNull() ) // degenerated edge
    {
      if ( u+tol < f || u-tol > l )
      {
        double r = Max( 0.5, 1 - tol*n->GetID()); // to get a unique u on edge
        u =  f*r + l*(1-r);
      }
    }
    else
    {
      gp_Pnt nodePnt = SMESH_TNodeXYZ( n );
      if ( !loc.IsIdentity() ) nodePnt.Transform( loc.Transformation().Inverted() );
      gp_Pnt curvPnt;
      double dist = 2*tol;
      if ( !infinit )
      {
        curvPnt = curve->Value( u );
        dist    = nodePnt.Distance( curvPnt );
        if ( distXYZ ) {
          curvPnt.Transform( loc );
          distXYZ[0] = dist;
          distXYZ[1] = curvPnt.X(); distXYZ[2] = curvPnt.Y(); distXYZ[3]=curvPnt.Z();
        }
      }
      if ( dist > tol )
      {
        setPosOnShapeValidity( shapeID, false );
        // u incorrect, project the node to the curve
        int edgeID = GetMeshDS()->ShapeToIndex( E );
        TID2ProjectorOnCurve& i2proj = const_cast< TID2ProjectorOnCurve&>( myEdge2Projector );
        TID2ProjectorOnCurve::iterator i_proj =
          i2proj.insert( make_pair( edgeID, (GeomAPI_ProjectPointOnCurve*) 0 )).first;
        if ( !i_proj->second  )
        {
          i_proj->second = new GeomAPI_ProjectPointOnCurve();
          i_proj->second->Init( curve, f, l );
        }
        GeomAPI_ProjectPointOnCurve* projector = i_proj->second;
        projector->Perform( nodePnt );
        if ( projector->NbPoints() < 1 )
        {
          MESSAGE( "SMESH_MesherHelper::CheckNodeU() failed to project" );
          return false;
        }
        Standard_Real U = projector->LowerDistanceParameter();
        u = double( U );
        MESSAGE(" f " << f << " l " << l << " u " << u);
        curvPnt = curve->Value( u );
        dist = nodePnt.Distance( curvPnt );
        if ( distXYZ ) {
          curvPnt.Transform( loc );
          distXYZ[0] = dist;
          distXYZ[1] = curvPnt.X(); distXYZ[2] = curvPnt.Y(); distXYZ[3]=curvPnt.Z();
        }
        if ( dist > tol )
        {
          MESSAGE( "SMESH_MesherHelper::CheckNodeU(), invalid projection" );
          MESSAGE("distance " << dist << " " << tol );
          return false;
        }
        // store the fixed U on the edge
        if ( myShape.IsSame(E) && shapeID == myShapeID && myFixNodeParameters )
          const_cast<SMDS_MeshNode*>(n)->SetPosition
            ( SMDS_PositionPtr( new SMDS_EdgePosition( U )));
      }
      else if ( fabs( u ) > numeric_limits<double>::min() )
      {
        setPosOnShapeValidity( shapeID, true );
      }
      if (( u < f-tol || u > l+tol ) && force )
      {
        MESSAGE("u < f-tol || u > l+tol  ; u " << u << " f " << f << " l " << l);
        // node is on vertex but is set on periodic but trimmed edge (issue 0020890)
        try
        {
          // do not use IsPeriodic() as Geom_TrimmedCurve::IsPeriodic () returns false
          double period = curve->Period();
          u = ( u < f ) ? u + period : u - period;
        }
        catch (Standard_Failure& exc)
        {
          return false;
        }
      }
    }
  }
  return true;
}

//=======================================================================
//function : GetMediumPos
//purpose  : Return index and type of the shape  (EDGE or FACE only) to
//           set a medium node on
//param    : useCurSubShape - if true, returns the shape set via SetSubShape()
//           if any
//param    : expectedSupport - shape type corresponding to element being created,
//                             e.g TopAbs_EDGE if SMDSAbs_Edge is created
//                             basing on \a n1 and \a n2
// Calling GetMediumPos() with useCurSubShape=true is OK only for the
// case where the lower dim mesh is already constructed and converted to quadratic,
// else, nodes on EDGEs are assigned to FACE, for example.
//=======================================================================

std::pair<int, TopAbs_ShapeEnum>
SMESH_MesherHelper::GetMediumPos(const SMDS_MeshNode* n1,
                                 const SMDS_MeshNode* n2,
                                 const bool           useCurSubShape,
                                 TopAbs_ShapeEnum     expectedSupport)
{
  if ( useCurSubShape && !myShape.IsNull() )
    return std::make_pair( myShapeID, myShape.ShapeType() );

  TopAbs_ShapeEnum shapeType = TopAbs_SHAPE;
  int              shapeID = -1;
  TopoDS_Shape     shape;

  if (( myShapeID == n1->getshapeId() || myShapeID == n2->getshapeId() ) && myShapeID > 0 )
  {
    shapeType = myShape.ShapeType();
    shapeID   = myShapeID;
  }
  else if ( n1->getshapeId() == n2->getshapeId() )
  {
    shapeID = n2->getshapeId();
    shape = GetSubShapeByNode( n1, GetMeshDS() );
  }
  else // 2 different shapes
  {
    const SMDS_TypeOfPosition Pos1 = n1->GetPosition()->GetTypeOfPosition();
    const SMDS_TypeOfPosition Pos2 = n2->GetPosition()->GetTypeOfPosition();

    if ( Pos1 == SMDS_TOP_3DSPACE || Pos2 == SMDS_TOP_3DSPACE )
    {
      // in SOLID
    }
    else if ( Pos1 == SMDS_TOP_FACE || Pos2 == SMDS_TOP_FACE )
    {
      // in FACE or SOLID
      if ( Pos1 != SMDS_TOP_FACE || Pos2 != SMDS_TOP_FACE ) // not 2 FACEs
      {
        if ( Pos1 != SMDS_TOP_FACE ) std::swap( n1,n2 );
        TopoDS_Shape F = GetSubShapeByNode( n1, GetMeshDS() );
        TopoDS_Shape S = GetSubShapeByNode( n2, GetMeshDS() );
        if ( IsSubShape( S, F ))
        {
          shapeType = TopAbs_FACE;
          shapeID   = n1->getshapeId();
        }
      }
    }
    else if ( Pos1 == SMDS_TOP_EDGE && Pos2 == SMDS_TOP_EDGE )
    {
      TopoDS_Shape E1 = GetSubShapeByNode( n1, GetMeshDS() );
      TopoDS_Shape E2 = GetSubShapeByNode( n2, GetMeshDS() );
      shape = GetCommonAncestor( E1, E2, *myMesh, TopAbs_FACE );
    }
    else if ( Pos1 == SMDS_TOP_VERTEX && Pos2 == SMDS_TOP_VERTEX )
    {
      TopoDS_Shape V1 = GetSubShapeByNode( n1, GetMeshDS() );
      TopoDS_Shape V2 = GetSubShapeByNode( n2, GetMeshDS() );
      shape = GetCommonAncestor( V1, V2, *myMesh, TopAbs_EDGE );
      if ( shape.IsNull() ) shape = GetCommonAncestor( V1, V2, *myMesh, TopAbs_FACE );
    }
    else // on VERTEX and EDGE
    {
      if ( Pos1 != SMDS_TOP_VERTEX ) std::swap( n1,n2 );
      TopoDS_Shape V = GetSubShapeByNode( n1, GetMeshDS() );
      TopoDS_Shape E = GetSubShapeByNode( n2, GetMeshDS() );
      if ( IsSubShape( V, E ))
        shape = E;
      else
        shape = GetCommonAncestor( V, E, *myMesh, TopAbs_FACE );
    }
  }

  if ( !shape.IsNull() )
  {
    if ( shapeID < 1 )
      shapeID = GetMeshDS()->ShapeToIndex( shape );
    shapeType = shape.ShapeType(); // EDGE or FACE

    if ( expectedSupport < shapeType &&
         expectedSupport != TopAbs_SHAPE &&
         !myShape.IsNull() &&
         myShape.ShapeType() == expectedSupport )
    {
      // e.g. a side of triangle connects nodes on the same EDGE but does not
      // lie on this EDGE (an arc with a coarse mesh)
      // =>  shapeType == TopAbs_EDGE, expectedSupport == TopAbs_FACE;
      // hope that myShape is a right shape, return it if the found shape
      // has converted elements of corresponding dim (segments in our example)
      int nbConvertedElems = 0;
      SMDSAbs_ElementType type = ( shapeType == TopAbs_FACE ? SMDSAbs_Face : SMDSAbs_Edge );
      for ( int iN = 0; iN < 2; ++iN )
      {
        const SMDS_MeshNode* n = iN ? n2 : n1;
        SMDS_ElemIteratorPtr it = n->GetInverseElementIterator( type );
        while ( it->more() )
        {
          const SMDS_MeshElement* elem = it->next();
          if ( elem->getshapeId() == shapeID &&
               elem->IsQuadratic() )
          {
            ++nbConvertedElems;
            break;
          }
        }
      }
      if ( nbConvertedElems == 2 )
      {
        shapeType = myShape.ShapeType();
        shapeID   = myShapeID;
      }
    }
  }
  return make_pair( shapeID, shapeType );
}

//=======================================================================
//function : GetCentralNode
//purpose  : Return existing or create a new central node for a quardilateral
//           quadratic face given its 8 nodes.
//@param   : force3d - true means node creation in between the given nodes,
//           else node position is found on a geometrical face if any.
//=======================================================================

const SMDS_MeshNode* SMESH_MesherHelper::GetCentralNode(const SMDS_MeshNode* n1,
                                                        const SMDS_MeshNode* n2,
                                                        const SMDS_MeshNode* n3,
                                                        const SMDS_MeshNode* n4,
                                                        const SMDS_MeshNode* n12,
                                                        const SMDS_MeshNode* n23,
                                                        const SMDS_MeshNode* n34,
                                                        const SMDS_MeshNode* n41,
                                                        bool                 force3d)
{
  SMDS_MeshNode *centralNode = 0; // central node to return

  // Find an existing central node

  TBiQuad keyOfMap(n1,n2,n3,n4);
  std::map<TBiQuad, const SMDS_MeshNode* >::iterator itMapCentralNode;
  itMapCentralNode = myMapWithCentralNode.find( keyOfMap );
  if ( itMapCentralNode != myMapWithCentralNode.end() ) 
  {
    return (*itMapCentralNode).second;
  }

  // Get type of shape for the new central node

  TopAbs_ShapeEnum shapeType = TopAbs_SHAPE;
  int              solidID = -1;
  int              faceID = -1;
  TopoDS_Shape     shape;
  TopTools_ListIteratorOfListOfShape it;

  std::map< int, int > faceId2nbNodes;
  std::map< int, int > ::iterator itMapWithIdFace;
  
  SMESHDS_Mesh* meshDS = GetMeshDS();
  
  // check if a face lies on a FACE, i.e. its all corner nodes lie either on the FACE or
  // on sub-shapes of the FACE
  if ( GetMesh()->HasShapeToMesh() )
  {
    const SMDS_MeshNode* nodes[] = { n1, n2, n3, n4 };
    for(int i = 0; i < 4; i++)
    {
      shape = GetSubShapeByNode( nodes[i], meshDS );
      if ( shape.IsNull() ) break;
      if ( shape.ShapeType() == TopAbs_SOLID )
      {
        solidID   = nodes[i]->getshapeId();
        shapeType = TopAbs_SOLID;
        break;
      }
      if ( shape.ShapeType() == TopAbs_FACE )
      {
        faceID          = nodes[i]->getshapeId();
        itMapWithIdFace = faceId2nbNodes.insert( std::make_pair( faceID, 0 ) ).first;
        itMapWithIdFace->second++;
      }
      else
      {
        PShapeIteratorPtr it = GetAncestors( shape, *GetMesh(), TopAbs_FACE );
        while ( const TopoDS_Shape* face = it->next() )
        {
          faceID = meshDS->ShapeToIndex( *face );
          itMapWithIdFace = faceId2nbNodes.insert( std::make_pair( faceID, 0 )).first;
          itMapWithIdFace->second++;
        }
      }
    }
  }
  if ( solidID < 1 && !faceId2nbNodes.empty() ) // SOLID not found
  {
    // find ID of the FACE the four corner nodes belong to
    itMapWithIdFace = faceId2nbNodes.find( myShapeID ); // IPAL52698
    if ( itMapWithIdFace != faceId2nbNodes.end() &&
         itMapWithIdFace->second == 4 )
    {
      shapeType = TopAbs_FACE;
      faceID = myShapeID;
    }
    else
    {
      itMapWithIdFace = faceId2nbNodes.begin();
      for ( ; itMapWithIdFace != faceId2nbNodes.end(); ++itMapWithIdFace)
      {
        if ( itMapWithIdFace->second == 4 )
        {
          shapeType = TopAbs_FACE;
          faceID = (*itMapWithIdFace).first;
          break;
        }
      }
    }
  }

  TopoDS_Face F;
  if ( shapeType == TopAbs_FACE )
  {
    F = TopoDS::Face( meshDS->IndexToShape( faceID ));
  }

  // Create a node

  gp_XY  uvAvg;
  gp_Pnt P;
  bool toCheck = true;
  if ( !F.IsNull() && !force3d )
  {
    Handle(ShapeAnalysis_Surface) surface = GetSurface( F );
    if ( HasDegeneratedEdges() || surface->HasSingularities( 1e-7 ))
    {
      gp_Pnt center = calcTFI (0.5, 0.5, // IPAL0052863
                               SMESH_TNodeXYZ(n1),  SMESH_TNodeXYZ(n2),
                               SMESH_TNodeXYZ(n3),  SMESH_TNodeXYZ(n4),
                               SMESH_TNodeXYZ(n12), SMESH_TNodeXYZ(n23),
                               SMESH_TNodeXYZ(n34), SMESH_TNodeXYZ(n41));
      gp_Pnt2d uv12 = GetNodeUV( F, n12, n3, &toCheck );
      uvAvg = surface->NextValueOfUV( uv12, center, BRep_Tool::Tolerance( F )).XY();
    }
    else
    {
      gp_XY uv[8] = {
        GetNodeUV( F,n1,  n3, &toCheck ),
        GetNodeUV( F,n2,  n4, &toCheck ),
        GetNodeUV( F,n3,  n1, &toCheck ),
        GetNodeUV( F,n4,  n2, &toCheck ),
        GetNodeUV( F,n12, n3 ),
        GetNodeUV( F,n23, n4 ),
        GetNodeUV( F,n34, n2 ),
        GetNodeUV( F,n41, n2 )
      };
      AdjustByPeriod( F, uv, 8 ); // put uv[] within a period (IPAL52698)

      uvAvg = calcTFI (0.5, 0.5, uv[0],uv[1],uv[2],uv[3], uv[4],uv[5],uv[6],uv[7] );
    }
    P = surface->Value( uvAvg );
    centralNode = meshDS->AddNode( P.X(), P.Y(), P.Z() );
    // if ( mySetElemOnShape ) node is not elem!
    meshDS->SetNodeOnFace( centralNode, faceID, uvAvg.X(), uvAvg.Y() );
  }
  else // ( force3d || F.IsNull() )
  {
    P = calcTFI (0.5, 0.5,
                 SMESH_TNodeXYZ(n1),  SMESH_TNodeXYZ(n2),
                 SMESH_TNodeXYZ(n3),  SMESH_TNodeXYZ(n4),
                 SMESH_TNodeXYZ(n12), SMESH_TNodeXYZ(n23),
                 SMESH_TNodeXYZ(n34), SMESH_TNodeXYZ(n41));
    centralNode = meshDS->AddNode( P.X(), P.Y(), P.Z() );

    if ( !F.IsNull() ) // force3d
    {
      uvAvg = (GetNodeUV(F,n1,n3,&toCheck) +
               GetNodeUV(F,n2,n4,&toCheck) +
               GetNodeUV(F,n3,n1,&toCheck) +
               GetNodeUV(F,n4,n2,&toCheck)) / 4;
      //CheckNodeUV( F, centralNode, uvAvg, 2*BRep_Tool::Tolerance( F ), /*force=*/true);
      meshDS->SetNodeOnFace( centralNode, faceID, uvAvg.X(), uvAvg.Y() );
    }
    else if ( solidID > 0 )
    {
      meshDS->SetNodeInVolume( centralNode, solidID );
    }
    else if ( myShapeID > 0 && mySetElemOnShape )
    {
      meshDS->SetMeshElementOnShape( centralNode, myShapeID );
    }
  }
  myMapWithCentralNode.insert( std::make_pair( keyOfMap, centralNode ) );
  return centralNode;
}

//=======================================================================
//function : GetCentralNode
//purpose  : Return existing or create a new central node for a
//           quadratic triangle given its 6 nodes.
//@param   : force3d - true means node creation in between the given nodes,
//           else node position is found on a geometrical face if any.
//=======================================================================

const SMDS_MeshNode* SMESH_MesherHelper::GetCentralNode(const SMDS_MeshNode* n1,
                                                        const SMDS_MeshNode* n2,
                                                        const SMDS_MeshNode* n3,
                                                        const SMDS_MeshNode* n12,
                                                        const SMDS_MeshNode* n23,
                                                        const SMDS_MeshNode* n31,
                                                        bool                 force3d)
{
  SMDS_MeshNode *centralNode = 0; // central node to return

  // Find an existing central node

  TBiQuad keyOfMap(n1,n2,n3);
  std::map<TBiQuad, const SMDS_MeshNode* >::iterator itMapCentralNode;
  itMapCentralNode = myMapWithCentralNode.find( keyOfMap );
  if ( itMapCentralNode != myMapWithCentralNode.end() ) 
  {
    return (*itMapCentralNode).second;
  }

  // Get type of shape for the new central node

  TopAbs_ShapeEnum shapeType = TopAbs_SHAPE;
  int              solidID = -1;
  int              faceID = -1;
  TopoDS_Shape     shape;
  TopTools_ListIteratorOfListOfShape it;

  std::map< int, int > faceId2nbNodes;
  std::map< int, int > ::iterator itMapWithIdFace;
  
  SMESHDS_Mesh* meshDS = GetMeshDS();
  
  // check if a face lies on a FACE, i.e. its all corner nodes lie either on the FACE or
  // on sub-shapes of the FACE
  if ( GetMesh()->HasShapeToMesh() )
  {
    const SMDS_MeshNode* nodes[] = { n1, n2, n3 };
    for(int i = 0; i < 3; i++)
    {
      shape = GetSubShapeByNode( nodes[i], meshDS );
      if ( shape.IsNull() ) break;
      if ( shape.ShapeType() == TopAbs_SOLID )
      {
        solidID   = nodes[i]->getshapeId();
        shapeType = TopAbs_SOLID;
        break;
      }
      if ( shape.ShapeType() == TopAbs_FACE )
      {
        faceID          = nodes[i]->getshapeId();
        itMapWithIdFace = faceId2nbNodes.insert( std::make_pair( faceID, 0 ) ).first;
        itMapWithIdFace->second++;
      }
      else
      {
        PShapeIteratorPtr it = GetAncestors(shape, *GetMesh(), TopAbs_FACE );
        while ( const TopoDS_Shape* face = it->next() )
        {
          faceID = meshDS->ShapeToIndex( *face );
          itMapWithIdFace = faceId2nbNodes.insert( std::make_pair( faceID, 0 ) ).first;
          itMapWithIdFace->second++;
        }
      }
    }
  }
  if ( solidID < 1 && !faceId2nbNodes.empty() ) // SOLID not found
  {
    // find ID of the FACE the four corner nodes belong to
    itMapWithIdFace = faceId2nbNodes.find( myShapeID ); // IPAL52698
    if ( itMapWithIdFace != faceId2nbNodes.end() &&
         itMapWithIdFace->second == 4 )
    {
      shapeType = TopAbs_FACE;
      faceID = myShapeID;
    }
    else
    {
      itMapWithIdFace = faceId2nbNodes.begin();
      for ( ; itMapWithIdFace != faceId2nbNodes.end(); ++itMapWithIdFace)
      {
        if ( itMapWithIdFace->second == 3 )
        {
          shapeType = TopAbs_FACE;
          faceID = (*itMapWithIdFace).first;
          break;
        }
      }
    }
  }

  TopoDS_Face F;
  gp_XY       uvAvg;

  if ( shapeType == TopAbs_FACE )
  {
    F = TopoDS::Face( meshDS->IndexToShape( faceID ));
    bool checkOK = true, badTria = false;
    gp_XY uv[6] = {
      GetNodeUV( F, n1, n23, &checkOK ),
      GetNodeUV( F, n2, n31, &checkOK ),
      GetNodeUV( F, n3, n12, &checkOK ),
      GetNodeUV( F, n12, n3, &checkOK ),
      GetNodeUV( F, n23, n1, &checkOK ),
      GetNodeUV( F, n31, n2, &checkOK )
    };
    AdjustByPeriod( F, uv, 6 ); // put uv[] within a period (IPAL52698)

    uvAvg = GetCenterUV( uv[0],uv[1],uv[2], uv[3],uv[4],uv[5], &badTria );

    if ( badTria || !checkOK )
      force3d = true;
  }

  // Create a central node

  gp_Pnt P;
  if ( !F.IsNull() && !force3d )
  {
    TopLoc_Location        loc;
    Handle( Geom_Surface ) S = BRep_Tool::Surface( F, loc );
    P = S->Value( uvAvg.X(), uvAvg.Y() ).Transformed( loc );
    centralNode = meshDS->AddNode( P.X(), P.Y(), P.Z() );
    // if ( mySetElemOnShape ) node is not elem!
    meshDS->SetNodeOnFace( centralNode, faceID, uvAvg.X(), uvAvg.Y() );
  }
  else // ( force3d || F.IsNull() )
  {
    P = ( SMESH_TNodeXYZ( n12 ) +
          SMESH_TNodeXYZ( n23 ) +
          SMESH_TNodeXYZ( n31 ) ) / 3;
    centralNode = meshDS->AddNode( P.X(), P.Y(), P.Z() );

    if ( !F.IsNull() ) // force3d
    {
      meshDS->SetNodeOnFace( centralNode, faceID, uvAvg.X(), uvAvg.Y() );
    }
    else if ( solidID > 0 )
    {
      meshDS->SetNodeInVolume( centralNode, solidID );
    }
    else if ( myShapeID > 0 && mySetElemOnShape )
    {
      meshDS->SetMeshElementOnShape( centralNode, myShapeID );
    }
  }
  myMapWithCentralNode.insert( std::make_pair( keyOfMap, centralNode ) );
  return centralNode;
}

//=======================================================================
//function : GetMediumNode
//purpose  : Return existing or create a new medium node between given ones
//=======================================================================

const SMDS_MeshNode* SMESH_MesherHelper::GetMediumNode(const SMDS_MeshNode* n1,
                                                       const SMDS_MeshNode* n2,
                                                       bool                 force3d,
                                                       TopAbs_ShapeEnum     expectedSupport)
{
  // Find existing node

  SMESH_TLink link(n1,n2);
  ItTLinkNode itLN = myTLinkNodeMap.find( link );
  if ( itLN != myTLinkNodeMap.end() ) {
    return (*itLN).second;
  }

  // Create medium node

  SMDS_MeshNode* n12;
  SMESHDS_Mesh* meshDS = GetMeshDS();

  if ( IsSeamShape( n1->getshapeId() ))
    // to get a correct UV of a node on seam, the second node must have checked UV
    std::swap( n1, n2 );

  // get type of shape for the new medium node
  int faceID = -1, edgeID = -1;
  TopoDS_Edge E; double u [2];
  TopoDS_Face F; gp_XY  uv[2];
  bool uvOK[2] = { true, true };
  const bool useCurSubShape = ( !myShape.IsNull() && myShape.ShapeType() == TopAbs_EDGE );

  pair<int, TopAbs_ShapeEnum> pos = GetMediumPos( n1, n2, useCurSubShape, expectedSupport );

  // get positions of the given nodes on shapes
  if ( pos.second == TopAbs_FACE )
  {
    F = TopoDS::Face(meshDS->IndexToShape( faceID = pos.first ));
    uv[0] = GetNodeUV(F,n1,n2, force3d ? 0 : &uvOK[0]);
    if (( !force3d ) &&
        ( HasDegeneratedEdges() || GetSurface( F )->HasSingularities( 1e-7 )))
    {
      // IPAL52850 (degen VERTEX not at singularity)
      // project middle point to a surface
      SMESH_TNodeXYZ p1( n1 ), p2( n2 );
      gp_Pnt pMid = 0.5 * ( p1 + p2 );
      Handle(ShapeAnalysis_Surface) projector = GetSurface( F );
      gp_Pnt2d uvMid;
      if ( uvOK[0] )
        uvMid = projector->NextValueOfUV( uv[0], pMid, BRep_Tool::Tolerance( F ));
      else
        uvMid = projector->ValueOfUV( pMid, getFaceMaxTol( F ));
      if ( projector->Gap() * projector->Gap() < ( p1 - p2 ).SquareModulus() / 4 )
      {
        gp_Pnt pProj = projector->Value( uvMid );
        n12  = meshDS->AddNode( pProj.X(), pProj.Y(), pProj.Z() );
        meshDS->SetNodeOnFace( n12, faceID, uvMid.X(), uvMid.Y() );
        myTLinkNodeMap.insert( make_pair ( link, n12 ));
        return n12;
      }
    }
    uv[1] = GetNodeUV(F,n2,n1, force3d ? 0 : &uvOK[1]);
  }
  else if ( pos.second == TopAbs_EDGE )
  {
    const SMDS_PositionPtr Pos1 = n1->GetPosition();
    const SMDS_PositionPtr Pos2 = n2->GetPosition();
    if ( Pos1->GetTypeOfPosition()==SMDS_TOP_EDGE &&
         Pos2->GetTypeOfPosition()==SMDS_TOP_EDGE &&
         n1->getshapeId() != n2->getshapeId() )
    {
      // issue 0021006
      return getMediumNodeOnComposedWire(n1,n2,force3d);
    }
    E = TopoDS::Edge(meshDS->IndexToShape( edgeID = pos.first ));
    try {
      u[0] = GetNodeU(E,n1,n2, force3d ? 0 : &uvOK[0]);
      u[1] = GetNodeU(E,n2,n1, force3d ? 0 : &uvOK[1]);
    }
    catch ( Standard_Failure& f )
    {
      // issue 22502 / a node is on VERTEX not belonging to E
      // issue 22568 / both nodes are on non-connected VERTEXes
      return getMediumNodeOnComposedWire(n1,n2,force3d);
    }
  }

  if ( !force3d & uvOK[0] && uvOK[1] )
  {
    // we try to create medium node using UV parameters of
    // nodes, else - medium between corresponding 3d points
    if( ! F.IsNull() )
    {
      //if ( uvOK[0] && uvOK[1] )
      {
        if ( IsDegenShape( n1->getshapeId() )) {
          if ( myParIndex & U_periodic ) uv[0].SetCoord( 1, uv[1].Coord( 1 ));
          else                           uv[0].SetCoord( 2, uv[1].Coord( 2 ));
        }
        else if ( IsDegenShape( n2->getshapeId() )) {
          if ( myParIndex & U_periodic ) uv[1].SetCoord( 1, uv[0].Coord( 1 ));
          else                           uv[1].SetCoord( 2, uv[0].Coord( 2 ));
        }
        TopLoc_Location loc;
        Handle(Geom_Surface) S = BRep_Tool::Surface(F,loc);
        gp_XY UV = GetMiddleUV( S, uv[0], uv[1] );
        gp_Pnt P = S->Value( UV.X(), UV.Y() ).Transformed(loc);
        n12 = meshDS->AddNode(P.X(), P.Y(), P.Z());
        // if ( mySetElemOnShape ) node is not elem!
        meshDS->SetNodeOnFace(n12, faceID, UV.X(), UV.Y());
        myTLinkNodeMap.insert(make_pair(link,n12));
        return n12;
      }
    }
    else if ( !E.IsNull() )
    {
      double f,l;
      Handle(Geom_Curve) C = BRep_Tool::Curve(E, f, l);
      if(!C.IsNull())
      {
        Standard_Boolean isPeriodic = C->IsPeriodic();
        double U;
        if(isPeriodic) {
          Standard_Real Period = C->Period();
          Standard_Real p = u[1]+ShapeAnalysis::AdjustByPeriod(u[1],u[0],Period);
          Standard_Real pmid = (u[0]+p)/2.;
          U = pmid+ShapeAnalysis::AdjustToPeriod(pmid,C->FirstParameter(),C->LastParameter());
        }
        else
          U = (u[0]+u[1])/2.;

        gp_Pnt P = C->Value( U );
        n12 = meshDS->AddNode(P.X(), P.Y(), P.Z());
        //if ( mySetElemOnShape ) node is not elem!
        meshDS->SetNodeOnEdge(n12, edgeID, U);
        myTLinkNodeMap.insert(make_pair(link,n12));
        return n12;
      }
    }
  }

  // 3d variant
  double x = ( n1->X() + n2->X() )/2.;
  double y = ( n1->Y() + n2->Y() )/2.;
  double z = ( n1->Z() + n2->Z() )/2.;
  n12 = meshDS->AddNode(x,y,z);

  //if ( mySetElemOnShape ) node is not elem!
  {
    if ( !F.IsNull() )
    {
      gp_XY UV = ( uv[0] + uv[1] ) / 2.;
      CheckNodeUV( F, n12, UV, 2 * BRep_Tool::Tolerance( F ), /*force=*/true);
      meshDS->SetNodeOnFace(n12, faceID, UV.X(), UV.Y() );
    }
    else if ( !E.IsNull() )
    {
      double U = ( u[0] + u[1] ) / 2.;
      CheckNodeU( E, n12, U, 2 * BRep_Tool::Tolerance( E ), /*force=*/true);
      meshDS->SetNodeOnEdge(n12, edgeID, U);
    }
    else if ( myShapeID > 0 && mySetElemOnShape )
    {
      meshDS->SetMeshElementOnShape(n12, myShapeID);
    }
  }

  myTLinkNodeMap.insert( make_pair( link, n12 ));
  return n12;
}

//================================================================================
/*!
 * \brief Makes a medium node if nodes reside different edges
 */
//================================================================================

const SMDS_MeshNode* SMESH_MesherHelper::getMediumNodeOnComposedWire(const SMDS_MeshNode* n1,
                                                                     const SMDS_MeshNode* n2,
                                                                     bool                 force3d)
{
  SMESH_TNodeXYZ p1( n1 ), p2( n2 );
  gp_Pnt      middle = 0.5 * p1 + 0.5 * p2;
  SMDS_MeshNode* n12 = AddNode( middle.X(), middle.Y(), middle.Z() );

  // To find position on edge and 3D position for n12,
  // project <middle> to 2 edges and select projection most close to <middle>

  TopoDS_Edge bestEdge;
  double u = 0, distMiddleProj = Precision::Infinite(), distXYZ[4], f,l;

  // get shapes under the nodes
  TopoDS_Shape shape[2];
  int nbShapes = 0;
  for ( int is2nd = 0; is2nd < 2; ++is2nd )
  {
    const SMDS_MeshNode* n = is2nd ? n2 : n1;
    TopoDS_Shape S = GetSubShapeByNode( n, GetMeshDS() );
    if ( !S.IsNull() )
      shape[ nbShapes++ ] = S;
  }
  // get EDGEs
  vector< TopoDS_Shape > edges;
  for ( int iS = 0; iS < nbShapes; ++iS )
  {
    switch ( shape[iS].ShapeType() ) {
    case TopAbs_EDGE:
    {
      edges.push_back( shape[iS] );
      break;
    }
    case TopAbs_VERTEX:
    {
      TopoDS_Shape edge;
      if ( nbShapes == 2 && iS==0 && shape[1-iS].ShapeType() == TopAbs_VERTEX )
        edge = GetCommonAncestor( shape[iS], shape[1-iS], *myMesh, TopAbs_EDGE );

      if ( edge.IsNull() )
      {
        PShapeIteratorPtr eIt = GetAncestors( shape[iS], *myMesh, TopAbs_EDGE );
        while( const TopoDS_Shape* e = eIt->next() )
          edges.push_back( *e );
      }
      break;
    }
    case TopAbs_FACE:
    {
      if ( nbShapes == 1 || shape[1-iS].ShapeType() < TopAbs_EDGE )
        for ( TopExp_Explorer e( shape[iS], TopAbs_EDGE ); e.More(); e.Next() )
          edges.push_back( e.Current() );
      break;
    }
    default:
      continue;
    }
  }
  // project to get U of projection and distance from middle to projection
  for ( size_t iE = 0; iE < edges.size(); ++iE )
  {
    const TopoDS_Edge& edge = TopoDS::Edge( edges[ iE ]);
    distXYZ[0] = distMiddleProj;
    double testU = 0;
    CheckNodeU( edge, n12, testU, 2 * BRep_Tool::Tolerance(edge), /*force=*/true, distXYZ );
    if ( distXYZ[0] < distMiddleProj )
    {
      distMiddleProj = distXYZ[0];
      u = testU;
      bestEdge = edge;
    }
  }
  // {
  //   // both projections failed; set n12 on the edge of n1 with U of a common vertex
  //   TopoDS_Vertex vCommon;
  //   if ( TopExp::CommonVertex( edges[0], edges[1], vCommon ))
  //     u = BRep_Tool::Parameter( vCommon, edges[0] );
  //   else
  //   {
  //     double f,l, u0 = GetNodeU( edges[0], n1 );
  //     BRep_Tool::Range( edges[0],f,l );
  //     u = ( fabs(u0-f) < fabs(u0-l) ) ? f : l;
  //   }
  //   iOkEdge = 0;
  //   distMiddleProj = 0;
  // }

  if ( !bestEdge.IsNull() )
  {
    // move n12 to position of a successfull projection
    //double tol = BRep_Tool::Tolerance(edges[ iOkEdge ]);
    if ( !force3d /*&& distMiddleProj > 2*tol*/ )
    {
      TopLoc_Location loc;
      Handle(Geom_Curve) curve = BRep_Tool::Curve( bestEdge,loc,f,l );
      gp_Pnt p = curve->Value( u ).Transformed( loc );
      GetMeshDS()->MoveNode( n12, p.X(), p.Y(), p.Z() );
    }
    //if ( mySetElemOnShape ) node is not elem!
    {
      int edgeID = GetMeshDS()->ShapeToIndex( bestEdge );
      if ( edgeID != n12->getshapeId() )
        GetMeshDS()->UnSetNodeOnShape( n12 );
      GetMeshDS()->SetNodeOnEdge(n12, edgeID, u);
    }
  }
  myTLinkNodeMap.insert( make_pair( SMESH_TLink(n1,n2), n12 ));

  return n12;
}

//=======================================================================
//function : AddNode
//purpose  : Creates a node
//=======================================================================

SMDS_MeshNode* SMESH_MesherHelper::AddNode(double x, double y, double z, int ID,
                                           double u, double v)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshNode* node = 0;
  if ( ID )
    node = meshDS->AddNodeWithID( x, y, z, ID );
  else
    node = meshDS->AddNode( x, y, z );
  if ( mySetElemOnShape && myShapeID > 0 ) { // node is not elem ?
    switch ( myShape.ShapeType() ) {
    case TopAbs_SOLID:  meshDS->SetNodeInVolume( node, myShapeID);       break;
    case TopAbs_SHELL:  meshDS->SetNodeInVolume( node, myShapeID);       break;
    case TopAbs_FACE:   meshDS->SetNodeOnFace(   node, myShapeID, u, v); break;
    case TopAbs_EDGE:   meshDS->SetNodeOnEdge(   node, myShapeID, u);    break;
    case TopAbs_VERTEX: meshDS->SetNodeOnVertex( node, myShapeID);       break;
    default: ;
    }
  }
  return node;
}

//=======================================================================
//function : AddEdge
//purpose  : Creates quadratic or linear edge
//=======================================================================

SMDS_MeshEdge* SMESH_MesherHelper::AddEdge(const SMDS_MeshNode* n1,
                                           const SMDS_MeshNode* n2,
                                           const int            id,
                                           const bool           force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  
  SMDS_MeshEdge* edge = 0;
  if (myCreateQuadratic) {
    const SMDS_MeshNode* n12 = GetMediumNode(n1,n2,force3d);
    if(id)
      edge = meshDS->AddEdgeWithID(n1, n2, n12, id);
    else
      edge = meshDS->AddEdge(n1, n2, n12);
  }
  else {
    if(id)
      edge = meshDS->AddEdgeWithID(n1, n2, id);
    else
      edge = meshDS->AddEdge(n1, n2);
  }

  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( edge, myShapeID );

  return edge;
}

//=======================================================================
//function : AddFace
//purpose  : Creates quadratic or linear triangle
//=======================================================================

SMDS_MeshFace* SMESH_MesherHelper::AddFace(const SMDS_MeshNode* n1,
                                           const SMDS_MeshNode* n2,
                                           const SMDS_MeshNode* n3,
                                           const int id,
                                           const bool force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshFace* elem = 0;

  if( n1==n2 || n2==n3 || n3==n1 )
    return elem;

  if(!myCreateQuadratic) {
    if(id)
      elem = meshDS->AddFaceWithID(n1, n2, n3, id);
    else
      elem = meshDS->AddFace(n1, n2, n3);
  }
  else {
    const SMDS_MeshNode* n12 = GetMediumNode( n1, n2, force3d, TopAbs_FACE );
    const SMDS_MeshNode* n23 = GetMediumNode( n2, n3, force3d, TopAbs_FACE );
    const SMDS_MeshNode* n31 = GetMediumNode( n3, n1, force3d, TopAbs_FACE );
    if(myCreateBiQuadratic)
    {
     const SMDS_MeshNode* nCenter = GetCentralNode(n1, n2, n3, n12, n23, n31, force3d);
     if(id)
       elem = meshDS->AddFaceWithID(n1, n2, n3, n12, n23, n31, nCenter, id);
     else
       elem = meshDS->AddFace(n1, n2, n3, n12, n23, n31, nCenter);
    }
    else
    {
      if(id)
        elem = meshDS->AddFaceWithID(n1, n2, n3, n12, n23, n31, id);
      else
        elem = meshDS->AddFace(n1, n2, n3, n12, n23, n31);
    }
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
//function : AddFace
//purpose  : Creates bi-quadratic, quadratic or linear quadrangle
//=======================================================================

SMDS_MeshFace* SMESH_MesherHelper::AddFace(const SMDS_MeshNode* n1,
                                           const SMDS_MeshNode* n2,
                                           const SMDS_MeshNode* n3,
                                           const SMDS_MeshNode* n4,
                                           const int            id,
                                           const bool           force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshFace* elem = 0;

  if( n1==n2 ) {
    return AddFace(n1,n3,n4,id,force3d);
  }
  if( n1==n3 ) {
    return AddFace(n1,n2,n4,id,force3d);
  }
  if( n1==n4 ) {
    return AddFace(n1,n2,n3,id,force3d);
  }
  if( n2==n3 ) {
    return AddFace(n1,n2,n4,id,force3d);
  }
  if( n2==n4 ) {
    return AddFace(n1,n2,n3,id,force3d);
  }
  if( n3==n4 ) {
    return AddFace(n1,n2,n3,id,force3d);
  }

  if(!myCreateQuadratic) {
    if(id)
      elem = meshDS->AddFaceWithID(n1, n2, n3, n4, id);
    else
      elem = meshDS->AddFace(n1, n2, n3, n4);
  }
  else {
    const SMDS_MeshNode* n12 = GetMediumNode( n1, n2, force3d, TopAbs_FACE );
    const SMDS_MeshNode* n23 = GetMediumNode( n2, n3, force3d, TopAbs_FACE );
    const SMDS_MeshNode* n34 = GetMediumNode( n3, n4, force3d, TopAbs_FACE );
    const SMDS_MeshNode* n41 = GetMediumNode( n4, n1, force3d, TopAbs_FACE );
    if(myCreateBiQuadratic)
    {
     const SMDS_MeshNode* nCenter = GetCentralNode(n1, n2, n3, n4, n12, n23, n34, n41, force3d);
     if(id)
       elem = meshDS->AddFaceWithID(n1, n2, n3, n4, n12, n23, n34, n41, nCenter, id);
     else
       elem = meshDS->AddFace(n1, n2, n3, n4, n12, n23, n34, n41, nCenter);
    }
    else
    {
      if(id)
        elem = meshDS->AddFaceWithID(n1, n2, n3, n4, n12, n23, n34, n41, id);
      else
        elem = meshDS->AddFace(n1, n2, n3, n4, n12, n23, n34, n41);
    }
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
//function : AddPolygonalFace
//purpose  : Creates polygon, with additional nodes in quadratic mesh
//=======================================================================

SMDS_MeshFace* SMESH_MesherHelper::AddPolygonalFace (const vector<const SMDS_MeshNode*>& nodes,
                                                     const int                           id,
                                                     const bool                          force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshFace* elem = 0;

  if(!myCreateQuadratic)
  {
    if(id)
      elem = meshDS->AddPolygonalFaceWithID(nodes, id);
    else
      elem = meshDS->AddPolygonalFace(nodes);
  }
  else
  {
    vector<const SMDS_MeshNode*> newNodes( nodes.size() * 2 );
    newNodes = nodes;
    for ( int i = 0; i < nodes.size(); ++i )
    {
      const SMDS_MeshNode* n1 = nodes[i];
      const SMDS_MeshNode* n2 = nodes[(i+1)%nodes.size()];
      const SMDS_MeshNode* n12 = GetMediumNode( n1, n2, force3d, TopAbs_FACE );
      newNodes.push_back( n12 );
    }
    if(id)
      elem = meshDS->AddQuadPolygonalFaceWithID(newNodes, id);
    else
      elem = meshDS->AddQuadPolygonalFace(newNodes);
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
//function : AddVolume
//purpose  : Creates quadratic or linear prism
//=======================================================================

SMDS_MeshVolume* SMESH_MesherHelper::AddVolume(const SMDS_MeshNode* n1,
                                               const SMDS_MeshNode* n2,
                                               const SMDS_MeshNode* n3,
                                               const SMDS_MeshNode* n4,
                                               const SMDS_MeshNode* n5,
                                               const SMDS_MeshNode* n6,
                                               const int id,
                                               const bool force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshVolume* elem = 0;
  if(!myCreateQuadratic) {
    if(id)
      elem = meshDS->AddVolumeWithID(n1, n2, n3, n4, n5, n6, id);
    else
      elem = meshDS->AddVolume(n1, n2, n3, n4, n5, n6);
  }
  else {
    const SMDS_MeshNode* n12 = GetMediumNode( n1, n2, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n23 = GetMediumNode( n2, n3, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n31 = GetMediumNode( n3, n1, force3d, TopAbs_SOLID );

    const SMDS_MeshNode* n45 = GetMediumNode( n4, n5, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n56 = GetMediumNode( n5, n6, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n64 = GetMediumNode( n6, n4, force3d, TopAbs_SOLID );

    const SMDS_MeshNode* n14 = GetMediumNode( n1, n4, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n25 = GetMediumNode( n2, n5, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n36 = GetMediumNode( n3, n6, force3d, TopAbs_SOLID );

    if(id)
      elem = meshDS->AddVolumeWithID(n1, n2, n3, n4, n5, n6,
                                     n12, n23, n31, n45, n56, n64, n14, n25, n36, id);
    else
      elem = meshDS->AddVolume(n1, n2, n3, n4, n5, n6,
                               n12, n23, n31, n45, n56, n64, n14, n25, n36);
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
//function : AddVolume
//purpose  : Creates quadratic or linear tetrahedron
//=======================================================================

SMDS_MeshVolume* SMESH_MesherHelper::AddVolume(const SMDS_MeshNode* n1,
                                               const SMDS_MeshNode* n2,
                                               const SMDS_MeshNode* n3,
                                               const SMDS_MeshNode* n4,
                                               const int id,
                                               const bool force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshVolume* elem = 0;
  if(!myCreateQuadratic) {
    if(id)
      elem = meshDS->AddVolumeWithID(n1, n2, n3, n4, id);
    else
      elem = meshDS->AddVolume(n1, n2, n3, n4);
  }
  else {
    const SMDS_MeshNode* n12 = GetMediumNode( n1, n2, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n23 = GetMediumNode( n2, n3, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n31 = GetMediumNode( n3, n1, force3d, TopAbs_SOLID );

    const SMDS_MeshNode* n14 = GetMediumNode( n1, n4, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n24 = GetMediumNode( n2, n4, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n34 = GetMediumNode( n3, n4, force3d, TopAbs_SOLID );

    if(id)
      elem = meshDS->AddVolumeWithID(n1, n2, n3, n4, n12, n23, n31, n14, n24, n34, id);
    else
      elem = meshDS->AddVolume(n1, n2, n3, n4, n12, n23, n31, n14, n24, n34);
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
//function : AddVolume
//purpose  : Creates quadratic or linear pyramid
//=======================================================================

SMDS_MeshVolume* SMESH_MesherHelper::AddVolume(const SMDS_MeshNode* n1,
                                               const SMDS_MeshNode* n2,
                                               const SMDS_MeshNode* n3,
                                               const SMDS_MeshNode* n4,
                                               const SMDS_MeshNode* n5,
                                               const int id,
                                               const bool force3d)
{
  SMDS_MeshVolume* elem = 0;
  if(!myCreateQuadratic) {
    if(id)
      elem = GetMeshDS()->AddVolumeWithID(n1, n2, n3, n4, n5, id);
    else
      elem = GetMeshDS()->AddVolume(n1, n2, n3, n4, n5);
  }
  else {
    const SMDS_MeshNode* n12 = GetMediumNode( n1, n2, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n23 = GetMediumNode( n2, n3, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n34 = GetMediumNode( n3, n4, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n41 = GetMediumNode( n4, n1, force3d, TopAbs_SOLID );

    const SMDS_MeshNode* n15 = GetMediumNode( n1, n5, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n25 = GetMediumNode( n2, n5, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n35 = GetMediumNode( n3, n5, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n45 = GetMediumNode( n4, n5, force3d, TopAbs_SOLID );

    if(id)
      elem = GetMeshDS()->AddVolumeWithID ( n1,  n2,  n3,  n4,  n5,
                                            n12, n23, n34, n41,
                                            n15, n25, n35, n45,
                                            id);
    else
      elem = GetMeshDS()->AddVolume( n1,  n2,  n3,  n4,  n5,
                                     n12, n23, n34, n41,
                                     n15, n25, n35, n45);
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    GetMeshDS()->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
//function : AddVolume
//purpose  : Creates tri-quadratic, quadratic or linear hexahedron
//=======================================================================

SMDS_MeshVolume* SMESH_MesherHelper::AddVolume(const SMDS_MeshNode* n1,
                                               const SMDS_MeshNode* n2,
                                               const SMDS_MeshNode* n3,
                                               const SMDS_MeshNode* n4,
                                               const SMDS_MeshNode* n5,
                                               const SMDS_MeshNode* n6,
                                               const SMDS_MeshNode* n7,
                                               const SMDS_MeshNode* n8,
                                               const int id,
                                               const bool force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshVolume* elem = 0;
  if(!myCreateQuadratic) {
    if(id)
      elem = meshDS->AddVolumeWithID(n1, n2, n3, n4, n5, n6, n7, n8, id);
    else
      elem = meshDS->AddVolume(n1, n2, n3, n4, n5, n6, n7, n8);
  }
  else {
    const SMDS_MeshNode* n12 = GetMediumNode( n1, n2, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n23 = GetMediumNode( n2, n3, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n34 = GetMediumNode( n3, n4, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n41 = GetMediumNode( n4, n1, force3d, TopAbs_SOLID );

    const SMDS_MeshNode* n56 = GetMediumNode( n5, n6, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n67 = GetMediumNode( n6, n7, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n78 = GetMediumNode( n7, n8, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n85 = GetMediumNode( n8, n5, force3d, TopAbs_SOLID );

    const SMDS_MeshNode* n15 = GetMediumNode( n1, n5, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n26 = GetMediumNode( n2, n6, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n37 = GetMediumNode( n3, n7, force3d, TopAbs_SOLID );
    const SMDS_MeshNode* n48 = GetMediumNode( n4, n8, force3d, TopAbs_SOLID );
    if ( myCreateBiQuadratic )
    {
      const SMDS_MeshNode* n1234 = GetCentralNode( n1,n2,n3,n4,n12,n23,n34,n41,force3d );
      const SMDS_MeshNode* n1256 = GetCentralNode( n1,n2,n5,n6,n12,n26,n56,n15,force3d );
      const SMDS_MeshNode* n2367 = GetCentralNode( n2,n3,n6,n7,n23,n37,n67,n26,force3d );
      const SMDS_MeshNode* n3478 = GetCentralNode( n3,n4,n7,n8,n34,n48,n78,n37,force3d );
      const SMDS_MeshNode* n1458 = GetCentralNode( n1,n4,n5,n8,n41,n48,n15,n85,force3d );
      const SMDS_MeshNode* n5678 = GetCentralNode( n5,n6,n7,n8,n56,n67,n78,n85,force3d );

      vector<gp_XYZ> pointsOnShapes( SMESH_Block::ID_Shell );

      pointsOnShapes[ SMESH_Block::ID_V000 ] = SMESH_TNodeXYZ( n4 );
      pointsOnShapes[ SMESH_Block::ID_V100 ] = SMESH_TNodeXYZ( n8 );
      pointsOnShapes[ SMESH_Block::ID_V010 ] = SMESH_TNodeXYZ( n3 );
      pointsOnShapes[ SMESH_Block::ID_V110 ] = SMESH_TNodeXYZ( n7 );
      pointsOnShapes[ SMESH_Block::ID_V001 ] = SMESH_TNodeXYZ( n1 );
      pointsOnShapes[ SMESH_Block::ID_V101 ] = SMESH_TNodeXYZ( n5 );
      pointsOnShapes[ SMESH_Block::ID_V011 ] = SMESH_TNodeXYZ( n2 );
      pointsOnShapes[ SMESH_Block::ID_V111 ] = SMESH_TNodeXYZ( n6 );

      pointsOnShapes[ SMESH_Block::ID_Ex00 ] = SMESH_TNodeXYZ( n48 );
      pointsOnShapes[ SMESH_Block::ID_Ex10 ] = SMESH_TNodeXYZ( n37 );
      pointsOnShapes[ SMESH_Block::ID_E0y0 ] = SMESH_TNodeXYZ( n15 );
      pointsOnShapes[ SMESH_Block::ID_E1y0 ] = SMESH_TNodeXYZ( n26 );
      pointsOnShapes[ SMESH_Block::ID_Ex01 ] = SMESH_TNodeXYZ( n34 );
      pointsOnShapes[ SMESH_Block::ID_Ex11 ] = SMESH_TNodeXYZ( n78 );
      pointsOnShapes[ SMESH_Block::ID_E0y1 ] = SMESH_TNodeXYZ( n12 );
      pointsOnShapes[ SMESH_Block::ID_E1y1 ] = SMESH_TNodeXYZ( n56 );
      pointsOnShapes[ SMESH_Block::ID_E00z ] = SMESH_TNodeXYZ( n41 );
      pointsOnShapes[ SMESH_Block::ID_E10z ] = SMESH_TNodeXYZ( n85 );
      pointsOnShapes[ SMESH_Block::ID_E01z ] = SMESH_TNodeXYZ( n23 );
      pointsOnShapes[ SMESH_Block::ID_E11z ] = SMESH_TNodeXYZ( n67 );

      pointsOnShapes[ SMESH_Block::ID_Fxy0 ] = SMESH_TNodeXYZ( n3478 );
      pointsOnShapes[ SMESH_Block::ID_Fxy1 ] = SMESH_TNodeXYZ( n1256 );
      pointsOnShapes[ SMESH_Block::ID_Fx0z ] = SMESH_TNodeXYZ( n1458 );
      pointsOnShapes[ SMESH_Block::ID_Fx1z ] = SMESH_TNodeXYZ( n2367 );
      pointsOnShapes[ SMESH_Block::ID_F0yz ] = SMESH_TNodeXYZ( n1234 );
      pointsOnShapes[ SMESH_Block::ID_F1yz ] = SMESH_TNodeXYZ( n5678 );

      gp_XYZ centerCube(0.5, 0.5, 0.5);
      gp_XYZ nCenterElem;
      SMESH_Block::ShellPoint( centerCube, pointsOnShapes, nCenterElem );
      const SMDS_MeshNode* nCenter =
        meshDS->AddNode( nCenterElem.X(), nCenterElem.Y(), nCenterElem.Z() );
      meshDS->SetNodeInVolume( nCenter, myShapeID );

      if(id)
        elem = meshDS->AddVolumeWithID(n1, n2, n3, n4, n5, n6, n7, n8,
                                       n12, n23, n34, n41, n56, n67,
                                       n78, n85, n15, n26, n37, n48,
                                       n1234, n1256, n2367, n3478, n1458, n5678, nCenter, id);
      else
        elem = meshDS->AddVolume(n1, n2, n3, n4, n5, n6, n7, n8,
                                 n12, n23, n34, n41, n56, n67,
                                 n78, n85, n15, n26, n37, n48,
                                 n1234, n1256, n2367, n3478, n1458, n5678, nCenter);
    }
    else
    {
      if(id)
        elem = meshDS->AddVolumeWithID(n1, n2, n3, n4, n5, n6, n7, n8,
                                       n12, n23, n34, n41, n56, n67,
                                       n78, n85, n15, n26, n37, n48, id);
      else
        elem = meshDS->AddVolume(n1, n2, n3, n4, n5, n6, n7, n8,
                                 n12, n23, n34, n41, n56, n67,
                                 n78, n85, n15, n26, n37, n48);
    }
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
//function : AddVolume
//purpose  : Creates LINEAR!!!!!!!!! octahedron
//=======================================================================

SMDS_MeshVolume* SMESH_MesherHelper::AddVolume(const SMDS_MeshNode* n1,
                                               const SMDS_MeshNode* n2,
                                               const SMDS_MeshNode* n3,
                                               const SMDS_MeshNode* n4,
                                               const SMDS_MeshNode* n5,
                                               const SMDS_MeshNode* n6,
                                               const SMDS_MeshNode* n7,
                                               const SMDS_MeshNode* n8,
                                               const SMDS_MeshNode* n9,
                                               const SMDS_MeshNode* n10,
                                               const SMDS_MeshNode* n11,
                                               const SMDS_MeshNode* n12,
                                               const int id, 
                                               bool force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshVolume* elem = 0;
  if(id)
    elem = meshDS->AddVolumeWithID(n1,n2,n3,n4,n5,n6,n7,n8,n9,n10,n11,n12,id);
  else
    elem = meshDS->AddVolume(n1,n2,n3,n4,n5,n6,n7,n8,n9,n10,n11,n12);
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );
  return elem;
}

//=======================================================================
//function : AddPolyhedralVolume
//purpose  : Creates polyhedron. In quadratic mesh, adds medium nodes
//=======================================================================

SMDS_MeshVolume*
SMESH_MesherHelper::AddPolyhedralVolume (const std::vector<const SMDS_MeshNode*>& nodes,
                                         const std::vector<int>&                  quantities,
                                         const int                                id,
                                         const bool                               force3d)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshVolume* elem = 0;
  if(!myCreateQuadratic)
  {
    if(id)
      elem = meshDS->AddPolyhedralVolumeWithID(nodes, quantities, id);
    else
      elem = meshDS->AddPolyhedralVolume(nodes, quantities);
  }
  else
  {
    vector<const SMDS_MeshNode*> newNodes;
    vector<int> newQuantities;
    for ( int iFace=0, iN=0; iFace < quantities.size(); ++iFace)
    {
      int nbNodesInFace = quantities[iFace];
      newQuantities.push_back(0);
      for ( int i = 0; i < nbNodesInFace; ++i )
      {
        const SMDS_MeshNode* n1 = nodes[ iN + i ];
        newNodes.push_back( n1 );
        newQuantities.back()++;
        
        const SMDS_MeshNode* n2 = nodes[ iN + ( i+1==nbNodesInFace ? 0 : i+1 )];
//         if ( n1->GetPosition()->GetTypeOfPosition() != SMDS_TOP_3DSPACE &&
//              n2->GetPosition()->GetTypeOfPosition() != SMDS_TOP_3DSPACE )
        {
          const SMDS_MeshNode* n12 = GetMediumNode( n1, n2, force3d, TopAbs_SOLID );
          newNodes.push_back( n12 );
          newQuantities.back()++;
        }
      }
      iN += nbNodesInFace;
    }
    if(id)
      elem = meshDS->AddPolyhedralVolumeWithID( newNodes, newQuantities, id );
    else
      elem = meshDS->AddPolyhedralVolume( newNodes, newQuantities );
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

namespace
{
  //================================================================================
  /*!
   * \brief Check if a node belongs to any face of sub-mesh
   */
  //================================================================================

  bool isNodeInSubMesh( const SMDS_MeshNode* n, const SMESHDS_SubMesh* sm )
  {
    SMDS_ElemIteratorPtr fIt = n->GetInverseElementIterator( SMDSAbs_Face );
    while ( fIt->more() )
      if ( sm->Contains( fIt->next() ))
        return true;
    return false;
  }
}

//=======================================================================
//function : IsSameElemGeometry
//purpose  : Returns true if all elements of a sub-mesh are of same shape
//=======================================================================

bool SMESH_MesherHelper::IsSameElemGeometry(const SMESHDS_SubMesh* smDS,
                                            SMDSAbs_GeometryType   shape,
                                            const bool             nullSubMeshRes)
{
  if ( !smDS ) return nullSubMeshRes;

  SMDS_ElemIteratorPtr elemIt = smDS->GetElements();
  while ( elemIt->more() ) {
    const SMDS_MeshElement* e = elemIt->next();
    if ( e->GetGeomType() != shape )
      return false;
  }
  return true;
}

//=======================================================================
//function : LoadNodeColumns
//purpose  : Load nodes bound to face into a map of node columns
//=======================================================================

bool SMESH_MesherHelper::LoadNodeColumns(TParam2ColumnMap & theParam2ColumnMap,
                                         const TopoDS_Face& theFace,
                                         const TopoDS_Edge& theBaseEdge,
                                         SMESHDS_Mesh*      theMesh,
                                         SMESH_ProxyMesh*   theProxyMesh)
{
  return LoadNodeColumns(theParam2ColumnMap,
                         theFace,
                         std::list<TopoDS_Edge>(1,theBaseEdge),
                         theMesh,
                         theProxyMesh);
}

//=======================================================================
//function : LoadNodeColumns
//purpose  : Load nodes bound to face into a map of node columns
//=======================================================================

bool SMESH_MesherHelper::LoadNodeColumns(TParam2ColumnMap &            theParam2ColumnMap,
                                         const TopoDS_Face&            theFace,
                                         const std::list<TopoDS_Edge>& theBaseSide,
                                         SMESHDS_Mesh*                 theMesh,
                                         SMESH_ProxyMesh*              theProxyMesh)
{
  // get a right sub-mesh of theFace

  const SMESHDS_SubMesh* faceSubMesh = 0;
  if ( theProxyMesh )
  {
    faceSubMesh = theProxyMesh->GetSubMesh( theFace );
    if ( !faceSubMesh ||
         faceSubMesh->NbElements() == 0 ||
         theProxyMesh->IsTemporary( faceSubMesh->GetElements()->next() ))
    {
      // can use a proxy sub-mesh with not temporary elements only
      faceSubMesh = 0;
      theProxyMesh = 0;
    }
  }
  if ( !faceSubMesh )
    faceSubMesh = theMesh->MeshElements( theFace );
  if ( !faceSubMesh || faceSubMesh->NbElements() == 0 )
    return false;

  if ( theParam2ColumnMap.empty() )
  {
    // get data of edges for normalization of params
    vector< double > length;
    double fullLen = 0;
    list<TopoDS_Edge>::const_iterator edge;
    {
      for ( edge = theBaseSide.begin(); edge != theBaseSide.end(); ++edge )
      {
        double len = std::max( 1e-10, SMESH_Algo::EdgeLength( *edge ));
        fullLen += len;
        length.push_back( len );
      }
    }

    // get nodes on theBaseEdge sorted by param on edge and initialize theParam2ColumnMap with them
    edge = theBaseSide.begin();
    for ( int iE = 0; edge != theBaseSide.end(); ++edge, ++iE )
    {
      map< double, const SMDS_MeshNode*> sortedBaseNN;
      SMESH_Algo::GetSortedNodesOnEdge( theMesh, *edge,/*noMedium=*/true, sortedBaseNN );

      map< double, const SMDS_MeshNode*>::iterator u_n;
      // pb with mesh_Projection_2D_00/A1 fixed by adding expectedSupport arg to GetMediumPos()
      // so the following solution is commented (hope forever :)
      //
      // SMESH_Algo::GetSortedNodesOnEdge( theMesh, *edge,/*noMedium=*/true, sortedBaseNN,
      // // SMDSAbs_Edge here is needed to be coherent with
      // // StdMeshers_FaceSide used by Quadrangle to get nodes
      // // on EDGE; else pb in mesh_Projection_2D_00/A1 where a
      // // medium node on EDGE is medium in a triangle but not
      // // in a segment
      // SMDSAbs_Edge );
      // if ( faceSubMesh->GetElements()->next()->IsQuadratic() )
      //   // filter off nodes medium in faces on theFace (same pb with mesh_Projection_2D_00/A1)
      //   for ( u_n = sortedBaseNN.begin(); u_n != sortedBaseNN.end() ;     )
      //   {
      //     const SMDS_MeshNode* node = u_n->second;
      //     SMDS_ElemIteratorPtr faceIt = node->GetInverseElementIterator( SMDSAbs_Face );
      //     if ( faceIt->more() && node ) {
      //       const SMDS_MeshElement* face = faceIt->next();
      //       if ( faceSubMesh->Contains( face ) && face->IsMediumNode( node ))
      //         node = 0;
      //     }
      //     if ( !node )
      //       sortedBaseNN.erase( u_n++ );
      //     else
      //       ++u_n;
      //   }
      if ( sortedBaseNN.empty() ) continue;

      u_n = sortedBaseNN.begin();
      if ( theProxyMesh ) // from sortedBaseNN remove nodes not shared by faces of faceSubMesh
      {
        const SMDS_MeshNode* n1 = (++sortedBaseNN.begin())->second;
        const SMDS_MeshNode* n2 = (++sortedBaseNN.rbegin())->second;
        bool allNodesAreProxy = ( n1 != theProxyMesh->GetProxyNode( n1 ) &&
                                  n2 != theProxyMesh->GetProxyNode( n2 ));
        if ( allNodesAreProxy )
          for ( u_n = sortedBaseNN.begin(); u_n != sortedBaseNN.end(); u_n++ )
            u_n->second = theProxyMesh->GetProxyNode( u_n->second );

        if ( u_n = sortedBaseNN.begin(), !isNodeInSubMesh( u_n->second, faceSubMesh ))
        {
          while ( ++u_n != sortedBaseNN.end() && !isNodeInSubMesh( u_n->second, faceSubMesh ));
          sortedBaseNN.erase( sortedBaseNN.begin(), u_n );
        }
        if ( !sortedBaseNN.empty() )
          if ( u_n = --sortedBaseNN.end(), !isNodeInSubMesh( u_n->second, faceSubMesh ))
          {
            while ( u_n != sortedBaseNN.begin() && !isNodeInSubMesh( (--u_n)->second, faceSubMesh ));
            sortedBaseNN.erase( ++u_n, sortedBaseNN.end() );
          }
        if ( sortedBaseNN.empty() ) continue;
      }

      double f, l;
      BRep_Tool::Range( *edge, f, l );
      if ( edge->Orientation() == TopAbs_REVERSED ) std::swap( f, l );
      const double coeff = 1. / ( l - f ) * length[iE] / fullLen;
      const double prevPar = theParam2ColumnMap.empty() ? 0 : theParam2ColumnMap.rbegin()->first;
      for ( u_n = sortedBaseNN.begin(); u_n != sortedBaseNN.end(); u_n++ )
      {
        double par = prevPar + coeff * ( u_n->first - f );
        TParam2ColumnMap::iterator u2nn =
          theParam2ColumnMap.insert( theParam2ColumnMap.end(), make_pair( par, TNodeColumn()));
        u2nn->second.push_back( u_n->second );
      }
    }
    if ( theParam2ColumnMap.size() < 2 )
      return false;
  }

  // nb rows of nodes
  int prevNbRows     = theParam2ColumnMap.begin()->second.size(); // current, at least 1 here
  int expectedNbRows = faceSubMesh->NbElements() / ( theParam2ColumnMap.size()-1 ); // to be added

  // fill theParam2ColumnMap column by column by passing from nodes on
  // theBaseEdge up via mesh faces on theFace

  TParam2ColumnMap::iterator par_nVec_1, par_nVec_2;
  par_nVec_2 = theParam2ColumnMap.begin();
  par_nVec_1 = par_nVec_2++;
  TIDSortedElemSet emptySet, avoidSet;
  for ( ; par_nVec_2 != theParam2ColumnMap.end(); ++par_nVec_1, ++par_nVec_2 )
  {
    vector<const SMDS_MeshNode*>& nCol1 = par_nVec_1->second;
    vector<const SMDS_MeshNode*>& nCol2 = par_nVec_2->second;
    nCol1.resize( prevNbRows + expectedNbRows );
    nCol2.resize( prevNbRows + expectedNbRows );

    int i1, i2, foundNbRows = 0;
    const SMDS_MeshNode *n1 = nCol1[ prevNbRows-1 ];
    const SMDS_MeshNode *n2 = nCol2[ prevNbRows-1 ];
    // find face sharing node n1 and n2 and belonging to faceSubMesh
    while ( const SMDS_MeshElement* face =
            SMESH_MeshAlgos::FindFaceInSet( n1, n2, emptySet, avoidSet, &i1, &i2))
    {
      if ( faceSubMesh->Contains( face ))
      {
        int nbNodes = face->NbCornerNodes();
        if ( nbNodes != 4 )
          return false;
        if ( foundNbRows + 1 > expectedNbRows )
          return false;
        n1 = face->GetNode( (i2+2) % 4 ); // opposite corner of quadrangle face
        n2 = face->GetNode( (i1+2) % 4 );
        nCol1[ prevNbRows + foundNbRows] = n1;
        nCol2[ prevNbRows + foundNbRows] = n2;
        ++foundNbRows;
      }
      avoidSet.insert( face );
    }
    if ( foundNbRows != expectedNbRows )
      return false;
    avoidSet.clear();
  }
  return ( theParam2ColumnMap.size() > 1 &&
           theParam2ColumnMap.begin()->second.size() == prevNbRows + expectedNbRows );
}

namespace
{
  //================================================================================
  /*!
   * \brief Return true if a node is at a corner of a 2D structured mesh of FACE
   */
  //================================================================================

  bool isCornerOfStructure( const SMDS_MeshNode*   n,
                            const SMESHDS_SubMesh* faceSM,
                            SMESH_MesherHelper&    faceAnalyser )
  {
    int nbFacesInSM = 0;
    if ( n ) {
      SMDS_ElemIteratorPtr fIt = n->GetInverseElementIterator( SMDSAbs_Face );
      while ( fIt->more() )
        nbFacesInSM += faceSM->Contains( fIt->next() );
    }
    if ( nbFacesInSM == 1 )
      return true;

    if ( nbFacesInSM == 2 && n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX )
    {
      return faceAnalyser.IsRealSeam( n->getshapeId() );
    }
    return false;
  }
}

//=======================================================================
//function : IsStructured
//purpose  : Return true if 2D mesh on FACE is a structured rectangle
//=======================================================================

bool SMESH_MesherHelper::IsStructured( SMESH_subMesh* faceSM )
{
  SMESHDS_SubMesh* fSM = faceSM->GetSubMeshDS();
  if ( !fSM || fSM->NbElements() == 0 )
    return false;

  list< TopoDS_Edge > edges;
  list< int > nbEdgesInWires;
  int nbWires = SMESH_Block::GetOrderedEdges( TopoDS::Face( faceSM->GetSubShape() ),
                                              edges, nbEdgesInWires );
  if ( nbWires != 1 /*|| nbEdgesInWires.front() != 4*/ ) // allow composite sides
    return false;

  // algo: find corners of a structure and then analyze nb of faces and
  // length of structure sides

  SMESHDS_Mesh* meshDS = faceSM->GetFather()->GetMeshDS();
  SMESH_MesherHelper faceAnalyser( *faceSM->GetFather() );
  faceAnalyser.SetSubShape( faceSM->GetSubShape() );

  // rotate edges to get the first node being at corner
  // (in principle it's not necessary because so far none SALOME algo can make
  //  such a structured mesh that all corner nodes are not on VERTEXes)
  bool isCorner     = false;
  int nbRemainEdges = nbEdgesInWires.front();
  do {
    TopoDS_Vertex V = IthVertex( 0, edges.front() );
    isCorner = isCornerOfStructure( SMESH_Algo::VertexNode( V, meshDS ),
                                    fSM, faceAnalyser);
    if ( !isCorner ) {
      edges.splice( edges.end(), edges, edges.begin() );
      --nbRemainEdges;
    }
  }
  while ( !isCorner && nbRemainEdges > 0 );

  if ( !isCorner )
    return false;

  // get all nodes from EDGEs
  list< const SMDS_MeshNode* > nodes;
  list< TopoDS_Edge >::iterator edge = edges.begin();
  for ( ; edge != edges.end(); ++edge )
  {
    map< double, const SMDS_MeshNode* > u2Nodes;
    if ( !SMESH_Algo::GetSortedNodesOnEdge( meshDS, *edge,
                                            /*skipMedium=*/true, u2Nodes ))
      return false;

    list< const SMDS_MeshNode* > edgeNodes;
    map< double, const SMDS_MeshNode* >::iterator u2n = u2Nodes.begin();
    for ( ; u2n != u2Nodes.end(); ++u2n )
      edgeNodes.push_back( u2n->second );
    if ( edge->Orientation() == TopAbs_REVERSED )
      edgeNodes.reverse();

    if ( !nodes.empty() && nodes.back() == edgeNodes.front() )
      edgeNodes.pop_front();
    nodes.splice( nodes.end(), edgeNodes, edgeNodes.begin(), edgeNodes.end() );
  }

  // get length of structured sides
  vector<int> nbEdgesInSide;
  int nbEdges = 0;
  list< const SMDS_MeshNode* >::iterator n = ++nodes.begin();
  for ( ; n != nodes.end(); ++n )
  {
    ++nbEdges;
    if ( isCornerOfStructure( *n, fSM, faceAnalyser )) {
      nbEdgesInSide.push_back( nbEdges );
      nbEdges = 0;
    }
  }

  // checks
  if ( nbEdgesInSide.size() != 4 )
    return false;
  if ( nbEdgesInSide[0] != nbEdgesInSide[2] )
    return false;
  if ( nbEdgesInSide[1] != nbEdgesInSide[3] )
    return false;
  if ( nbEdgesInSide[0] * nbEdgesInSide[1] != fSM->NbElements() )
    return false;

  return true;
}

//=======================================================================
//function : IsDistorted2D
//purpose  : Return true if 2D mesh on FACE is ditorted
//=======================================================================

bool SMESH_MesherHelper::IsDistorted2D( SMESH_subMesh* faceSM,
                                        bool           checkUV)
{
  if ( !faceSM || faceSM->GetSubShape().ShapeType() != TopAbs_FACE )
    return false;

  bool haveBadFaces = false;

  SMESH_MesherHelper helper( *faceSM->GetFather() );
  helper.SetSubShape( faceSM->GetSubShape() );

  const TopoDS_Face&  F = TopoDS::Face( faceSM->GetSubShape() );
  SMESHDS_SubMesh* smDS = helper.GetMeshDS()->MeshElements( F );
  if ( !smDS || smDS->NbElements() == 0 ) return false;

  SMDS_ElemIteratorPtr faceIt = smDS->GetElements();
  double prevArea = 0;
  vector< const SMDS_MeshNode* > nodes;
  vector< gp_XY >                uv;
  bool* toCheckUV = checkUV ? & checkUV : 0;
  while ( faceIt->more() && !haveBadFaces )
  {
    const SMDS_MeshElement* face = faceIt->next();

    // get nodes
    nodes.resize( face->NbCornerNodes() );
    SMDS_MeshElement::iterator n = face->begin_nodes();
    for ( size_t i = 0; i < nodes.size(); ++n, ++i )
      nodes[ i ] = *n;

    // avoid elems on degenarate shapes as UV on them can be wrong
    if ( helper.HasDegeneratedEdges() )
    {
      bool isOnDegen = false;
      for ( size_t i = 0; ( i < nodes.size() && !isOnDegen ); ++i )
        isOnDegen = helper.IsDegenShape( nodes[ i ]->getshapeId() );
      if ( isOnDegen )
        continue;
    }
    // prepare to getting UVs
    const SMDS_MeshNode* inFaceNode = 0;
    if ( helper.HasSeam() ) {
      for ( size_t i = 0; ( i < nodes.size() && !inFaceNode ); ++i )
        if ( !helper.IsSeamShape( nodes[ i ]->getshapeId() ))
          inFaceNode = nodes[ i ];
      if ( !inFaceNode )
        continue;
    }
    // get UVs
    uv.resize( nodes.size() );
    for ( size_t i = 0; i < nodes.size(); ++i )
      uv[ i ] = helper.GetNodeUV( F, nodes[ i ], inFaceNode, toCheckUV );

    // compare orientation of triangles
    double faceArea = 0;
    for ( int iT = 0, nbT = nodes.size()-2; iT < nbT; ++iT )
    {
      gp_XY v1 = uv[ iT+1 ] - uv[ 0 ];
      gp_XY v2 = uv[ iT+2 ] - uv[ 0 ];
      faceArea += v2 ^ v1;
    }
    haveBadFaces = ( faceArea * prevArea < 0 );
    prevArea = faceArea;
  }

  return haveBadFaces;
}

//================================================================================
/*!
 * \brief Find out elements orientation on a geometrical face
 * \param theFace - The face correctly oriented in the shape being meshed
 * \retval bool - true if the face normal and the normal of first element
 *                in the correspoding submesh point in different directions
 */
//================================================================================

bool SMESH_MesherHelper::IsReversedSubMesh (const TopoDS_Face& theFace)
{
  if ( theFace.IsNull() )
    return false;

  // find out orientation of a meshed face
  int faceID = GetMeshDS()->ShapeToIndex( theFace );
  TopoDS_Shape aMeshedFace = GetMeshDS()->IndexToShape( faceID );
  bool isReversed = ( theFace.Orientation() != aMeshedFace.Orientation() );

  const SMESHDS_SubMesh * aSubMeshDSFace = GetMeshDS()->MeshElements( faceID );
  if ( !aSubMeshDSFace )
    return isReversed;

  // find an element on a bounday of theFace
  SMDS_ElemIteratorPtr iteratorElem = aSubMeshDSFace->GetElements();
  const SMDS_MeshNode* nn[2];
  while ( iteratorElem->more() ) // loop on elements on theFace
  {
    const SMDS_MeshElement* elem = iteratorElem->next();
    if ( ! elem ) continue;

    // look for 2 nodes on EDGE
    int nbNodes = elem->NbCornerNodes();
    nn[0] = elem->GetNode( nbNodes-1 );
    for ( int iN = 0; iN < nbNodes; ++iN )
    {
      nn[1] = elem->GetNode( iN );
      if ( nn[0]->GetPosition()->GetDim() < 2 &&
           nn[1]->GetPosition()->GetDim() < 2 )
      {
        TopoDS_Shape s0 = GetSubShapeByNode( nn[0], GetMeshDS() );
        TopoDS_Shape s1 = GetSubShapeByNode( nn[1], GetMeshDS() );
        TopoDS_Shape  E = GetCommonAncestor( s0, s1, *myMesh, TopAbs_EDGE );
        if ( !E.IsNull() && !s0.IsSame( s1 ))
        {
          // is E seam edge?
          int nb = 0;
          for ( TopExp_Explorer exp( theFace, TopAbs_EDGE ); exp.More(); exp.Next() )
            if ( E.IsSame( exp.Current() )) {
              ++nb;
              E = exp.Current(); // to know orientation
            }
          if ( nb == 1 )
          {
            bool ok = true;
            double u0 = GetNodeU( TopoDS::Edge( E ), nn[0], nn[1], &ok );
            double u1 = GetNodeU( TopoDS::Edge( E ), nn[1], nn[0], &ok );
            if ( ok )
            {
              isReversed = ( u0 > u1 );
              if ( E.Orientation() == TopAbs_REVERSED )
                isReversed = !isReversed;
              return isReversed;
            }
          }
        }
      }
      nn[0] = nn[1];
    }
  }

  // find an element with a good normal
  gp_Vec Ne;
  bool normalOK = false;
  gp_XY uv;
  iteratorElem = aSubMeshDSFace->GetElements();
  while ( !normalOK && iteratorElem->more() ) // loop on elements on theFace
  {
    const SMDS_MeshElement* elem = iteratorElem->next();
    if ( ! SMESH_MeshAlgos::FaceNormal( elem, const_cast<gp_XYZ&>( Ne.XYZ() ), /*normalized=*/0 ))
      continue;
    normalOK = true;

    // get UV of a node inside theFACE
    SMDS_ElemIteratorPtr nodesIt = elem->nodesIterator();
    const SMDS_MeshNode* nInFace = 0;
    int iPosDim = SMDS_TOP_VERTEX;
    while ( nodesIt->more() ) // loop on nodes
    {
      const SMDS_MeshNode* n = static_cast<const SMDS_MeshNode*>( nodesIt->next() );
      if ( n->GetPosition()->GetTypeOfPosition() >= iPosDim )
      {
        nInFace = n;
        iPosDim = n->GetPosition()->GetTypeOfPosition();
      }
    }
    uv = GetNodeUV( theFace, nInFace, 0, &normalOK );
  }
  if ( !normalOK )
    return isReversed;

  // face normal at node position
  TopLoc_Location loc;
  Handle(Geom_Surface) surf = BRep_Tool::Surface( theFace, loc );
  // if ( surf.IsNull() || surf->Continuity() < GeomAbs_C1 )
  // some surfaces not detected as GeomAbs_C1 are nevertheless correct for meshing
  if ( surf.IsNull() || surf->Continuity() < GeomAbs_C0 )
    return isReversed;

  gp_Vec d1u, d1v; gp_Pnt p;
  surf->D1( uv.X(), uv.Y(), p, d1u, d1v );
  gp_Vec Nf = (d1u ^ d1v).Transformed( loc );

  if ( theFace.Orientation() == TopAbs_REVERSED )
    Nf.Reverse();

  return Ne * Nf < 0.;
}

//=======================================================================
//function : Count
//purpose  : Count nb of sub-shapes
//=======================================================================

int SMESH_MesherHelper::Count(const TopoDS_Shape&    shape,
                              const TopAbs_ShapeEnum type,
                              const bool             ignoreSame)
{
  if ( ignoreSame ) {
    TopTools_IndexedMapOfShape map;
    TopExp::MapShapes( shape, type, map );
    return map.Extent();
  }
  else {
    int nb = 0;
    for ( TopExp_Explorer exp( shape, type ); exp.More(); exp.Next() )
      ++nb;
    return nb;
  }
}

//=======================================================================
//function : NbAncestors
//purpose  : Return number of unique ancestors of the shape
//=======================================================================

int SMESH_MesherHelper::NbAncestors(const TopoDS_Shape& shape,
                                    const SMESH_Mesh&   mesh,
                                    TopAbs_ShapeEnum    ancestorType/*=TopAbs_SHAPE*/)
{
  TopTools_MapOfShape ancestors;
  TopTools_ListIteratorOfListOfShape ansIt( mesh.GetAncestors(shape) );
  for ( ; ansIt.More(); ansIt.Next() ) {
    if ( ancestorType == TopAbs_SHAPE || ansIt.Value().ShapeType() == ancestorType )
      ancestors.Add( ansIt.Value() );
  }
  return ancestors.Extent();
}

//=======================================================================
//function : GetSubShapeOri
//purpose  : Return orientation of sub-shape in the main shape
//=======================================================================

TopAbs_Orientation SMESH_MesherHelper::GetSubShapeOri(const TopoDS_Shape& shape,
                                                      const TopoDS_Shape& subShape)
{
  TopAbs_Orientation ori = TopAbs_Orientation(-1);
  if ( !shape.IsNull() && !subShape.IsNull() )
  {
    TopExp_Explorer e( shape, subShape.ShapeType() );
    if ( shape.Orientation() >= TopAbs_INTERNAL ) // TopAbs_INTERNAL or TopAbs_EXTERNAL
      e.Init( shape.Oriented(TopAbs_FORWARD), subShape.ShapeType() );
    for ( ; e.More(); e.Next())
      if ( subShape.IsSame( e.Current() ))
        break;
    if ( e.More() )
      ori = e.Current().Orientation();
  }
  return ori;
}

//=======================================================================
//function : IsSubShape
//purpose  : 
//=======================================================================

bool SMESH_MesherHelper::IsSubShape( const TopoDS_Shape& shape,
                                     const TopoDS_Shape& mainShape )
{
  if ( !shape.IsNull() && !mainShape.IsNull() )
  {
    for ( TopExp_Explorer exp( mainShape, shape.ShapeType());
          exp.More();
          exp.Next() )
      if ( shape.IsSame( exp.Current() ))
        return true;
  }
  SCRUTE((shape.IsNull()));
  SCRUTE((mainShape.IsNull()));
  return false;
}

//=======================================================================
//function : IsSubShape
//purpose  : 
//=======================================================================

bool SMESH_MesherHelper::IsSubShape( const TopoDS_Shape& shape, SMESH_Mesh* aMesh )
{
  if ( shape.IsNull() || !aMesh )
    return false;
  return
    aMesh->GetMeshDS()->ShapeToIndex( shape ) ||
    // PAL16202
    (shape.ShapeType() == TopAbs_COMPOUND && aMesh->GetMeshDS()->IsGroupOfSubShapes( shape ));
}

//=======================================================================
//function : IsBlock
//purpose  : 
//=======================================================================

bool SMESH_MesherHelper::IsBlock( const TopoDS_Shape& shape )
{
  if ( shape.IsNull() )
    return false;

  TopoDS_Shell shell;
  TopExp_Explorer exp( shape, TopAbs_SHELL );
  if ( !exp.More() ) return false;
  shell = TopoDS::Shell( exp.Current() );
  if ( exp.Next(), exp.More() ) return false;

  TopoDS_Vertex v;
  TopTools_IndexedMapOfOrientedShape map;
  return SMESH_Block::FindBlockShapes( shell, v, v, map );
}


//================================================================================
/*!
 * \brief Return maximal tolerance of shape
 */
//================================================================================

double SMESH_MesherHelper::MaxTolerance( const TopoDS_Shape& shape )
{
  double tol = Precision::Confusion();
  TopExp_Explorer exp;
  for ( exp.Init( shape, TopAbs_FACE ); exp.More(); exp.Next() )
    tol = Max( tol, BRep_Tool::Tolerance( TopoDS::Face( exp.Current())));
  for ( exp.Init( shape, TopAbs_EDGE ); exp.More(); exp.Next() )
    tol = Max( tol, BRep_Tool::Tolerance( TopoDS::Edge( exp.Current())));
  for ( exp.Init( shape, TopAbs_VERTEX ); exp.More(); exp.Next() )
    tol = Max( tol, BRep_Tool::Tolerance( TopoDS::Vertex( exp.Current())));

  return tol;
}

//================================================================================
/*!
 * \brief Return MaxTolerance( face ), probably cached
 */
//================================================================================

double SMESH_MesherHelper::getFaceMaxTol( const TopoDS_Shape& face ) const
{
  int faceID = GetMeshDS()->ShapeToIndex( face );

  SMESH_MesherHelper* me = const_cast< SMESH_MesherHelper* >( this );
  double & tol = me->myFaceMaxTol.insert( make_pair( faceID, -1. )).first->second;
  if ( tol < 0 )
    tol = MaxTolerance( face );

  return tol;
}

//================================================================================
/*!
 * \brief Return an angle between two EDGEs sharing a common VERTEX with reference
 *        of the FACE normal
 *  \return double - the angle (between -Pi and Pi), negative if the angle is concave,
 *                   1e100 in case of failure
 *  \waring Care about order of the EDGEs and their orientation to be as they are
 *          within the FACE! Don't pass degenerated EDGEs neither!
 */
//================================================================================

double SMESH_MesherHelper::GetAngle( const TopoDS_Edge &   theE1,
                                     const TopoDS_Edge &   theE2,
                                     const TopoDS_Face &   theFace,
                                     const TopoDS_Vertex & theCommonV,
                                     gp_Vec*               theFaceNormal)
{
  double angle = 1e100;
  try
  {
    double f,l;
    Handle(Geom_Curve)     c1 = BRep_Tool::Curve( theE1, f,l );
    Handle(Geom_Curve)     c2 = BRep_Tool::Curve( theE2, f,l );
    Handle(Geom2d_Curve) c2d1 = BRep_Tool::CurveOnSurface( theE1, theFace, f,l );
    Handle(Geom_Surface) surf = BRep_Tool::Surface( theFace );
    double                 p1 = BRep_Tool::Parameter( theCommonV, theE1 );
    double                 p2 = BRep_Tool::Parameter( theCommonV, theE2 );
    if ( c1.IsNull() || c2.IsNull() )
      return angle;
    gp_XY uv = c2d1->Value( p1 ).XY();
    gp_Vec du, dv; gp_Pnt p;
    surf->D1( uv.X(), uv.Y(), p, du, dv );
    gp_Vec vec1, vec2, vecRef = du ^ dv;
    int  nbLoops = 0;
    double p1tmp = p1;
    while ( vecRef.SquareMagnitude() < 1e-25 )
    {
      double dp = ( l - f ) / 1000.;
      p1tmp += dp * (( Abs( p1 - f ) > Abs( p1 - l )) ? -1. : +1.);
      uv = c2d1->Value( p1tmp ).XY();
      surf->D1( uv.X(), uv.Y(), p, du, dv );
      vecRef = du ^ dv;
      if ( ++nbLoops > 10 )
      {
#ifdef _DEBUG_
        cout << "SMESH_MesherHelper::GetAngle(): Captured in a sigularity" << endl;
#endif
        return angle;
      }
    }
    if ( theFace.Orientation() == TopAbs_REVERSED )
      vecRef.Reverse();
    if ( theFaceNormal ) *theFaceNormal = vecRef;

    c1->D1( p1, p, vec1 );
    c2->D1( p2, p, vec2 );
    // TopoDS_Face F = theFace;
    // if ( F.Orientation() == TopAbs_INTERNAL )
    //   F.Orientation( TopAbs_FORWARD );
    if ( theE1.Orientation() /*GetSubShapeOri( F, theE1 )*/ == TopAbs_REVERSED )
      vec1.Reverse();
    if ( theE2.Orientation() /*GetSubShapeOri( F, theE2 )*/ == TopAbs_REVERSED )
      vec2.Reverse();
    angle = vec1.AngleWithRef( vec2, vecRef );

    if ( Abs ( angle ) >= 0.99 * M_PI )
    {
      BRep_Tool::Range( theE1, f, l );
      p1 += 1e-7 * ( p1-f < l-p1 ? +1. : -1. );
      c1->D1( p1, p, vec1 );
      if ( theE1.Orientation() == TopAbs_REVERSED )
        vec1.Reverse();
      BRep_Tool::Range( theE2, f, l );
      p2 += 1e-7 * ( p2-f < l-p2 ? +1. : -1. );
      c2->D1( p2, p, vec2 );
      if ( theE2.Orientation() == TopAbs_REVERSED )
        vec2.Reverse();
      angle = vec1.AngleWithRef( vec2, vecRef );
    }
  }
  catch (...)
  {
  }
  return angle;
}

//================================================================================
/*!
 * \brief Check if the first and last vertices of an edge are the same
 * \param anEdge - the edge to check
 * \retval bool - true if same
 */
//================================================================================

bool SMESH_MesherHelper::IsClosedEdge( const TopoDS_Edge& anEdge )
{
  if ( anEdge.Orientation() >= TopAbs_INTERNAL )
    return IsClosedEdge( TopoDS::Edge( anEdge.Oriented( TopAbs_FORWARD )));
  return TopExp::FirstVertex( anEdge ).IsSame( TopExp::LastVertex( anEdge ));
}

//================================================================================
/*!
 * \brief Wrapper over TopExp::FirstVertex() and TopExp::LastVertex() fixing them
 *  in the case of INTERNAL edge
 */
//================================================================================

TopoDS_Vertex SMESH_MesherHelper::IthVertex( const bool  is2nd,
                                             TopoDS_Edge anEdge,
                                             const bool  CumOri )
{
  if ( anEdge.Orientation() >= TopAbs_INTERNAL )
    anEdge.Orientation( TopAbs_FORWARD );

  const TopAbs_Orientation tgtOri = is2nd ? TopAbs_REVERSED : TopAbs_FORWARD;
  TopoDS_Iterator vIt( anEdge, CumOri );
  while ( vIt.More() && vIt.Value().Orientation() != tgtOri )
    vIt.Next();

  return ( vIt.More() ? TopoDS::Vertex(vIt.Value()) : TopoDS_Vertex() );
}

//================================================================================
/*!
 * \brief Return type of shape contained in a group 
 *  \param group - a shape of type TopAbs_COMPOUND
 *  \param avoidCompound - not to return TopAbs_COMPOUND
 */
//================================================================================

TopAbs_ShapeEnum SMESH_MesherHelper::GetGroupType(const TopoDS_Shape& group,
                                                  const bool          avoidCompound)
{
  if ( !group.IsNull() )
  {
    if ( group.ShapeType() != TopAbs_COMPOUND )
      return group.ShapeType();

    // iterate on a compound
    TopoDS_Iterator it( group );
    if ( it.More() )
      return avoidCompound ? GetGroupType( it.Value() ) : it.Value().ShapeType();
  }
  return TopAbs_SHAPE;
}

//================================================================================
/*!
 * \brief Returns a shape, to which a hypothesis used to mesh a given shape is assigned
 *  \param [in] hyp - the hypothesis
 *  \param [in] shape - the shape, for meshing which the \a hyp is used
 *  \param [in] mesh - the mesh
 *  \return TopoDS_Shape - the shape the \a hyp is assigned to
 */
//================================================================================

TopoDS_Shape SMESH_MesherHelper::GetShapeOfHypothesis( const SMESHDS_Hypothesis * hyp,
                                                       const TopoDS_Shape&        shape,
                                                       SMESH_Mesh*                mesh)
{
  const SMESH_Hypothesis* h = static_cast<const SMESH_Hypothesis*>( hyp );
  SMESH_HypoFilter hypFilter( SMESH_HypoFilter::Is( h ));

  TopoDS_Shape shapeOfHyp;
  mesh->GetHypothesis( shape, hypFilter, /*checkAncestors=*/true, &shapeOfHyp );
  return shapeOfHyp;
}

//=======================================================================
//function : IsQuadraticMesh
//purpose  : Check mesh without geometry for: if all elements on this shape are quadratic,
//           quadratic elements will be created.
//           Used then generated 3D mesh without geometry.
//=======================================================================

SMESH_MesherHelper:: MType SMESH_MesherHelper::IsQuadraticMesh()
{
  int NbAllEdgsAndFaces=0;
  int NbQuadFacesAndEdgs=0;
  int NbFacesAndEdges=0;
  //All faces and edges
  NbAllEdgsAndFaces = myMesh->NbEdges() + myMesh->NbFaces();
  if ( NbAllEdgsAndFaces == 0 )
    return SMESH_MesherHelper::LINEAR;
  
  //Quadratic faces and edges
  NbQuadFacesAndEdgs = myMesh->NbEdges(ORDER_QUADRATIC) + myMesh->NbFaces(ORDER_QUADRATIC);

  //Linear faces and edges
  NbFacesAndEdges = myMesh->NbEdges(ORDER_LINEAR) + myMesh->NbFaces(ORDER_LINEAR);
  
  if (NbAllEdgsAndFaces == NbQuadFacesAndEdgs) {
    //Quadratic mesh
    return SMESH_MesherHelper::QUADRATIC;
  }
  else if (NbAllEdgsAndFaces == NbFacesAndEdges) {
    //Linear mesh
    return SMESH_MesherHelper::LINEAR;
  }
  else
    //Mesh with both type of elements
    return SMESH_MesherHelper::COMP;
}

//=======================================================================
//function : GetOtherParam
//purpose  : Return an alternative parameter for a node on seam
//=======================================================================

double SMESH_MesherHelper::GetOtherParam(const double param) const
{
  int i = myParIndex & U_periodic ? 0 : 1;
  return fabs(param-myPar1[i]) < fabs(param-myPar2[i]) ? myPar2[i] : myPar1[i];
}

namespace {

  //=======================================================================
  /*!
   * \brief Iterator on ancestors of the given type
   */
  //=======================================================================

  struct TAncestorsIterator : public SMDS_Iterator<const TopoDS_Shape*>
  {
    TopTools_ListIteratorOfListOfShape _ancIter;
    TopAbs_ShapeEnum                   _type;
    TopTools_MapOfShape                _encountered;
    TAncestorsIterator( const TopTools_ListOfShape& ancestors, TopAbs_ShapeEnum type)
      : _ancIter( ancestors ), _type( type )
    {
      if ( _ancIter.More() ) {
        if ( _ancIter.Value().ShapeType() != _type ) next();
        else _encountered.Add( _ancIter.Value() );
      }
    }
    virtual bool more()
    {
      return _ancIter.More();
    }
    virtual const TopoDS_Shape* next()
    {
      const TopoDS_Shape* s = _ancIter.More() ? & _ancIter.Value() : 0;
      if ( _ancIter.More() )
        for ( _ancIter.Next();  _ancIter.More(); _ancIter.Next())
          if ( _ancIter.Value().ShapeType() == _type && _encountered.Add( _ancIter.Value() ))
            break;
      return s;
    }
  };

} // namespace

//=======================================================================
/*!
 * \brief Return iterator on ancestors of the given type
 */
//=======================================================================

PShapeIteratorPtr SMESH_MesherHelper::GetAncestors(const TopoDS_Shape& shape,
                                                   const SMESH_Mesh&   mesh,
                                                   TopAbs_ShapeEnum    ancestorType)
{
  return PShapeIteratorPtr( new TAncestorsIterator( mesh.GetAncestors(shape), ancestorType));
}

//=======================================================================
//function : GetCommonAncestor
//purpose  : Find a common ancestors of two shapes of the given type
//=======================================================================

TopoDS_Shape SMESH_MesherHelper::GetCommonAncestor(const TopoDS_Shape& shape1,
                                                   const TopoDS_Shape& shape2,
                                                   const SMESH_Mesh&   mesh,
                                                   TopAbs_ShapeEnum    ancestorType)
{
  TopoDS_Shape commonAnc;
  if ( !shape1.IsNull() && !shape2.IsNull() )
  {
    if ( shape1.ShapeType() == ancestorType && IsSubShape( shape2, shape1 ))
      return shape1;
    if ( shape2.ShapeType() == ancestorType && IsSubShape( shape1, shape2 ))
      return shape2;

    PShapeIteratorPtr ancIt = GetAncestors( shape1, mesh, ancestorType );
    while ( const TopoDS_Shape* anc = ancIt->next() )
      if ( IsSubShape( shape2, *anc ))
      {
        commonAnc = *anc;
        break;
      }
  }
  return commonAnc;
}

//#include <Perf_Meter.hxx>

//=======================================================================
namespace { // Structures used by FixQuadraticElements()
//=======================================================================

#define __DMP__(txt) \
  // cout << txt
#define MSG(txt) __DMP__(txt<<endl)
#define MSGBEG(txt) __DMP__(txt)

  //const double straightTol2 = 1e-33; // to detect straing links
  bool isStraightLink(double linkLen2, double middleNodeMove2)
  {
    // straight if <node move> < 1/15 * <link length>
    return middleNodeMove2 < 1/15./15. * linkLen2;
  }

  struct QFace;
  // ---------------------------------------
  /*!
   * \brief Quadratic link knowing its faces
   */
  struct QLink: public SMESH_TLink
  {
    const SMDS_MeshNode*          _mediumNode;
    mutable vector<const QFace* > _faces;
    mutable gp_Vec                _nodeMove;
    mutable int                   _nbMoves;
    mutable bool                  _is2dFixed; // is moved along surface or in 3D

    QLink(const SMDS_MeshNode* n1, const SMDS_MeshNode* n2, const SMDS_MeshNode* nm):
      SMESH_TLink( n1,n2 ), _mediumNode(nm), _nodeMove(0,0,0), _nbMoves(0) {
      _faces.reserve(4);
      _nodeMove = MediumPnt() - MiddlePnt();
      _is2dFixed = ( MediumPos() != SMDS_TOP_FACE );
    }
    void SetContinuesFaces() const;
    const QFace* GetContinuesFace( const QFace* face ) const;
    bool   OnBoundary() const;
    gp_XYZ MiddlePnt() const { return ( XYZ( node1() ) + XYZ( node2() )) / 2.; }
    gp_XYZ MediumPnt() const { return XYZ( _mediumNode ); }

    SMDS_TypeOfPosition  MediumPos() const
    { return _mediumNode->GetPosition()->GetTypeOfPosition(); }
    SMDS_TypeOfPosition  EndPos(bool isSecond) const
    { return (isSecond ? node2() : node1())->GetPosition()->GetTypeOfPosition(); }
    const SMDS_MeshNode* EndPosNode(SMDS_TypeOfPosition pos) const
    { return EndPos(0) == pos ? node1() : EndPos(1) == pos ? node2() : 0; }

    void Move(const gp_Vec& move, bool sum=false, bool is2dFixed=false) const
    { _nodeMove += move; _nbMoves += sum ? (_nbMoves==0) : 1; _is2dFixed |= is2dFixed; }
    gp_XYZ Move() const { return _nodeMove.XYZ() / _nbMoves; }
    bool IsMoved() const { return (_nbMoves > 0 /*&& !IsStraight()*/); }
    bool IsFixedOnSurface() const { return _is2dFixed; }
    bool IsStraight() const
    { return isStraightLink( (XYZ(node1())-XYZ(node2())).SquareModulus(),
                             _nodeMove.SquareMagnitude());
    }
    bool operator<(const QLink& other) const {
      return (node1()->GetID() == other.node1()->GetID() ?
              node2()->GetID() < other.node2()->GetID() :
              node1()->GetID() < other.node1()->GetID());
    }
//     struct PtrComparator {
//       bool operator() (const QLink* l1, const QLink* l2 ) const { return *l1 < *l2; }
//     };
  };
  // ---------------------------------------------------------
  /*!
   * \brief Link in the chain of links; it connects two faces
   */
  struct TChainLink
  {
    const QLink*         _qlink;
    mutable const QFace* _qfaces[2];

    TChainLink(const QLink* qlink=0):_qlink(qlink) {
      _qfaces[0] = _qfaces[1] = 0;
    }
    void SetFace(const QFace* face) const { int iF = _qfaces[0] ? 1 : 0; _qfaces[iF]=face; }

    bool IsBoundary() const { return !_qfaces[1]; }

    void RemoveFace( const QFace* face ) const
    { _qfaces[(face == _qfaces[1])] = 0; if (!_qfaces[0]) std::swap(_qfaces[0],_qfaces[1]); }

    const QFace* NextFace( const QFace* f ) const
    { return _qfaces[0]==f ? _qfaces[1] : _qfaces[0]; }

    const SMDS_MeshNode* NextNode( const SMDS_MeshNode* n ) const
    { return n == _qlink->node1() ? _qlink->node2() : _qlink->node1(); }

    bool operator<(const TChainLink& other) const { return *_qlink < *other._qlink; }

    operator bool() const { return (_qlink); }

    const QLink* operator->() const { return _qlink; }

    gp_Vec Normal() const;

    bool IsStraight() const;
  };
  // --------------------------------------------------------------------
  typedef list< TChainLink > TChain;
  typedef set < TChainLink > TLinkSet;
  typedef TLinkSet::const_iterator TLinkInSet;

  const int theFirstStep = 5;

  enum { ERR_OK, ERR_TRI, ERR_PRISM, ERR_UNKNOWN }; // errors of QFace::GetLinkChain()
  // --------------------------------------------------------------------
  /*!
   * \brief Quadratic face shared by two volumes and bound by QLinks
   */
  struct QFace: public TIDSortedNodeSet
  {
    mutable const SMDS_MeshElement* _volumes[2];
    mutable vector< const QLink* >  _sides;
    mutable bool                    _sideIsAdded[4]; // added in chain of links
    gp_Vec                          _normal;
#ifdef _DEBUG_
    mutable const SMDS_MeshElement* _face;
#endif

    QFace( const vector< const QLink*>& links, const SMDS_MeshElement* face=0 );

    void SetVolume(const SMDS_MeshElement* v) const { _volumes[ _volumes[0] ? 1 : 0 ] = v; }

    int NbVolumes() const { return !_volumes[0] ? 0 : !_volumes[1] ? 1 : 2; }

    void AddSelfToLinks() const {
      for ( int i = 0; i < _sides.size(); ++i )
        _sides[i]->_faces.push_back( this );
    }
    int LinkIndex( const QLink* side ) const {
      for (int i=0; i<_sides.size(); ++i ) if ( _sides[i] == side ) return i;
      return -1;
    }
    bool GetLinkChain( int iSide, TChain& chain, SMDS_TypeOfPosition pos, int& err) const;

    bool GetLinkChain( TChainLink& link, TChain& chain, SMDS_TypeOfPosition pos, int& err) const
    {
      int i = LinkIndex( link._qlink );
      if ( i < 0 ) return true;
      _sideIsAdded[i] = true;
      link.SetFace( this );
      // continue from opposite link
      return GetLinkChain( (i+2)%_sides.size(), chain, pos, err );
    }
    bool IsBoundary() const { return !_volumes[1]; }

    bool Contains( const SMDS_MeshNode* node ) const { return count(node); }

    //bool IsSpoiled(const QLink* bentLink ) const;

    TLinkInSet GetBoundaryLink( const TLinkSet&      links,
                                const TChainLink&    avoidLink,
                                TLinkInSet *         notBoundaryLink = 0,
                                const SMDS_MeshNode* nodeToContain = 0,
                                bool *               isAdjacentUsed = 0,
                                int                  nbRecursionsLeft = -1) const;

    TLinkInSet GetLinkByNode( const TLinkSet&      links,
                              const TChainLink&    avoidLink,
                              const SMDS_MeshNode* nodeToContain) const;

    const SMDS_MeshNode* GetNodeInFace() const {
      for ( int iL = 0; iL < _sides.size(); ++iL )
        if ( _sides[iL]->MediumPos() == SMDS_TOP_FACE ) return _sides[iL]->_mediumNode;
      return 0;
    }

    gp_Vec LinkNorm(const int i, SMESH_MesherHelper* theFaceHelper=0) const;

    double MoveByBoundary( const TChainLink&   theLink,
                           const gp_Vec&       theRefVec,
                           const TLinkSet&     theLinks,
                           SMESH_MesherHelper* theFaceHelper=0,
                           const double        thePrevLen=0,
                           const int           theStep=theFirstStep,
                           gp_Vec*             theLinkNorm=0,
                           double              theSign=1.0) const;
  };

  //================================================================================
  /*!
   * \brief Dump QLink and QFace
   */
  inline ostream& operator << (ostream& out, const QLink& l)
  {
    out <<"QLink nodes: "
        << l.node1()->GetID() << " - "
        << l._mediumNode->GetID() << " - "
        << l.node2()->GetID() << endl;
    return out;
  }
  inline ostream& operator << (ostream& out, const QFace& f)
  {
    out <<"QFace nodes: "/*<< &f << "  "*/;
    for ( TIDSortedNodeSet::const_iterator n = f.begin(); n != f.end(); ++n )
      out << (*n)->GetID() << " ";
    out << " \tvolumes: "
        << (f._volumes[0] ? f._volumes[0]->GetID() : 0) << " "
        << (f._volumes[1] ? f._volumes[1]->GetID() : 0);
    out << "  \tNormal: "<< f._normal.X() <<", "<<f._normal.Y() <<", "<<f._normal.Z() << endl;
    return out;
  }

  //================================================================================
  /*!
   * \brief Construct QFace from QLinks 
   */
  //================================================================================

  QFace::QFace( const vector< const QLink*>& links, const SMDS_MeshElement* face )
  {
    _volumes[0] = _volumes[1] = 0;
    _sides = links;
    _sideIsAdded[0]=_sideIsAdded[1]=_sideIsAdded[2]=_sideIsAdded[3]=false;
    _normal.SetCoord(0,0,0);
    for ( int i = 1; i < _sides.size(); ++i ) {
      const QLink *l1 = _sides[i-1], *l2 = _sides[i];
      insert( l1->node1() ); insert( l1->node2() );
      // compute normal
      gp_Vec v1( XYZ( l1->node2()), XYZ( l1->node1()));
      gp_Vec v2( XYZ( l2->node1()), XYZ( l2->node2()));
      if ( l1->node1() != l2->node1() && l1->node2() != l2->node2() )
        v1.Reverse(); 
      _normal += v1 ^ v2;
    }
    double normSqSize = _normal.SquareMagnitude();
    if ( normSqSize > numeric_limits<double>::min() )
      _normal /= sqrt( normSqSize );
    else
      _normal.SetCoord(1e-33,0,0);

#ifdef _DEBUG_
    _face = face;
#endif
  }
  //================================================================================
  /*!
   * \brief Make up a chain of links
   *  \param iSide - link to add first
   *  \param chain - chain to fill in
   *  \param pos   - position of medium nodes the links should have
   *  \param error - out, specifies what is wrong
   *  \retval bool - false if valid chain can't be built; "valid" means that links
   *                 of the chain belongs to rectangles bounding hexahedrons
   */
  //================================================================================

  bool QFace::GetLinkChain( int iSide, TChain& chain, SMDS_TypeOfPosition pos, int& error) const
  {
    if ( iSide >= _sides.size() ) // wrong argument iSide
      return false;
    if ( _sideIsAdded[ iSide ]) // already in chain
      return true;

    if ( _sides.size() != 4 ) { // triangle - visit all my continous faces
      MSGBEG( *this );
      TLinkSet links;
      list< const QFace* > faces( 1, this );
      while ( !faces.empty() ) {
        const QFace* face = faces.front();
        for ( int i = 0; i < face->_sides.size(); ++i ) {
          if ( !face->_sideIsAdded[i] && face->_sides[i] ) {
            face->_sideIsAdded[i] = true;
            // find a face side in the chain
            TLinkInSet chLink = links.insert( TChainLink(face->_sides[i])).first;
//             TChain::iterator chLink = chain.begin();
//             for ( ; chLink != chain.end(); ++chLink )
//               if ( chLink->_qlink == face->_sides[i] )
//                 break;
//             if ( chLink == chain.end() )
//               chLink = chain.insert( chain.begin(), TChainLink(face->_sides[i]));
            // add a face to a chained link and put a continues face in the queue
            chLink->SetFace( face );
            if ( face->_sides[i]->MediumPos() == pos )
              if ( const QFace* contFace = face->_sides[i]->GetContinuesFace( face ))
                if ( contFace->_sides.size() == 3 )
                  faces.push_back( contFace );
          }
        }
        faces.pop_front();
      }
      if ( error < ERR_TRI )
        error = ERR_TRI;
      chain.insert( chain.end(), links.begin(),links.end() );
      return false;
    }
    _sideIsAdded[iSide] = true; // not to add this link to chain again
    const QLink* link = _sides[iSide];
    if ( !link)
      return true;

    // add link into chain
    TChain::iterator chLink = chain.insert( chain.begin(), TChainLink(link));
    chLink->SetFace( this );
    MSGBEG( *this );

    // propagate from a quadrangle to neighbour faces
    if ( link->MediumPos() >= pos ) {
      int nbLinkFaces = link->_faces.size();
      if ( nbLinkFaces == 4 || (/*nbLinkFaces < 4 && */link->OnBoundary())) {
        // hexahedral mesh or boundary quadrangles - goto a continous face
        if ( const QFace* f = link->GetContinuesFace( this ))
          if ( f->_sides.size() == 4 )
            return f->GetLinkChain( *chLink, chain, pos, error );
      }
      else {
        TChainLink chLink(link); // side face of prismatic mesh - visit all faces of iSide
        for ( int i = 0; i < nbLinkFaces; ++i )
          if ( link->_faces[i] )
            link->_faces[i]->GetLinkChain( chLink, chain, pos, error );
        if ( error < ERR_PRISM )
          error = ERR_PRISM;
        return false;
      }
    }
    return true;
  }

  //================================================================================
  /*!
   * \brief Return a boundary link of the triangle face
   *  \param links - set of all links
   *  \param avoidLink - link not to return
   *  \param notBoundaryLink - out, neither the returned link nor avoidLink
   *  \param nodeToContain - node the returned link must contain; if provided, search
   *                         also performed on adjacent faces
   *  \param isAdjacentUsed - returns true if link is found in adjacent faces
   *  \param nbRecursionsLeft - to limit recursion
   */
  //================================================================================

  TLinkInSet QFace::GetBoundaryLink( const TLinkSet&      links,
                                     const TChainLink&    avoidLink,
                                     TLinkInSet *         notBoundaryLink,
                                     const SMDS_MeshNode* nodeToContain,
                                     bool *               isAdjacentUsed,
                                     int                  nbRecursionsLeft) const
  {
    TLinkInSet linksEnd = links.end(), boundaryLink = linksEnd;

    typedef list< pair< const QFace*, TLinkInSet > > TFaceLinkList;
    TFaceLinkList adjacentFaces;

    for ( int iL = 0; iL < _sides.size(); ++iL )
    {
      if ( avoidLink._qlink == _sides[iL] )
        continue;
      TLinkInSet link = links.find( _sides[iL] );
      if ( link == linksEnd ) continue;
      if ( (*link)->MediumPos() > SMDS_TOP_FACE )
        continue; // We work on faces here, don't go inside a solid

      // check link
      if ( link->IsBoundary() ) {
        if ( !nodeToContain ||
             (*link)->node1() == nodeToContain ||
             (*link)->node2() == nodeToContain )
        {
          boundaryLink = link;
          if ( !notBoundaryLink ) break;
        }
      }
      else if ( notBoundaryLink ) {
        *notBoundaryLink = link;
        if ( boundaryLink != linksEnd ) break;
      }

      if ( boundaryLink == linksEnd && nodeToContain ) // collect adjacent faces
        if ( const QFace* adj = link->NextFace( this ))
          if ( adj->Contains( nodeToContain ))
            adjacentFaces.push_back( make_pair( adj, link ));
    }

    if ( isAdjacentUsed ) *isAdjacentUsed = false;
    if ( boundaryLink == linksEnd && nodeToContain && nbRecursionsLeft) // check adjacent faces
    {
      if ( nbRecursionsLeft < 0 )
        nbRecursionsLeft = nodeToContain->NbInverseElements();
      TFaceLinkList::iterator adj = adjacentFaces.begin();
      for ( ; boundaryLink == linksEnd && adj != adjacentFaces.end(); ++adj )
        boundaryLink = adj->first->GetBoundaryLink( links, *(adj->second), 0, nodeToContain,
                                                    isAdjacentUsed, nbRecursionsLeft-1);
      if ( isAdjacentUsed ) *isAdjacentUsed = true;
    }
    return boundaryLink;
  }
  //================================================================================
  /*!
   * \brief Return a link ending at the given node but not avoidLink
   */
  //================================================================================

  TLinkInSet QFace::GetLinkByNode( const TLinkSet&      links,
                                   const TChainLink&    avoidLink,
                                   const SMDS_MeshNode* nodeToContain) const
  {
    for ( int i = 0; i < _sides.size(); ++i )
      if ( avoidLink._qlink != _sides[i] &&
           (_sides[i]->node1() == nodeToContain || _sides[i]->node2() == nodeToContain ))
        return links.find( _sides[ i ]);
    return links.end();
  }

  //================================================================================
  /*!
   * \brief Return normal to the i-th side pointing outside the face
   */
  //================================================================================

  gp_Vec QFace::LinkNorm(const int i, SMESH_MesherHelper* /*uvHelper*/) const
  {
    gp_Vec   norm = _normal ^ gp_Vec( XYZ(_sides[i]->node1()), XYZ(_sides[i]->node2()));
    gp_XYZ    pIn = ( _sides[ (i+1)%3 ]->MiddlePnt() +
                      _sides[ (i+2)%3 ]->MiddlePnt() ) / 2.;
    gp_Vec vecOut = ( _sides[i]->MiddlePnt() - pIn );

    if ( norm * vecOut < 0 )
      norm.Reverse();
    double mag2 = norm.SquareMagnitude();
    if ( mag2 > numeric_limits<double>::min() )
      norm /= sqrt( mag2 );
    return norm;
  }
  //================================================================================
  /*!
   * \brief Move medium node of theLink according to its distance from boundary
   *  \param theLink - link to fix
   *  \param theRefVec - movement of boundary
   *  \param theLinks - all adjacent links of continous triangles
   *  \param theFaceHelper - helper is not used so far
   *  \param thePrevLen - distance from the boundary
   *  \param theStep - number of steps till movement propagation limit
   *  \param theLinkNorm - out normal to theLink
   *  \param theSign - 1 or -1 depending on movement of boundary
   *  \retval double - distance from boundary to propagation limit or other boundary
   */
  //================================================================================

  double QFace::MoveByBoundary( const TChainLink&   theLink,
                                const gp_Vec&       theRefVec,
                                const TLinkSet&     theLinks,
                                SMESH_MesherHelper* theFaceHelper,
                                const double        thePrevLen,
                                const int           theStep,
                                gp_Vec*             theLinkNorm,
                                double              theSign) const
  {
    if ( !theStep )
      return thePrevLen; // propagation limit reached

    int iL; // index of theLink
    for ( iL = 0; iL < _sides.size(); ++iL )
      if ( theLink._qlink == _sides[ iL ])
        break;

    MSG(string(theStep,'.')<<" Ref( "<<theRefVec.X()<<","<<theRefVec.Y()<<","<<theRefVec.Z()<<" )"
        <<" thePrevLen " << thePrevLen);
    MSG(string(theStep,'.')<<" "<<*theLink._qlink);

    gp_Vec linkNorm = -LinkNorm( iL/*, theFaceHelper*/ ); // normal to theLink
    double refProj = theRefVec * linkNorm; // project movement vector to normal of theLink
    if ( theStep == theFirstStep )
      theSign = refProj < 0. ? -1. : 1.;
    else if ( theSign * refProj < 0.4 * theRefVec.Magnitude())
      return thePrevLen; // to propagate movement forward only, not in side dir or backward

    int iL1 = (iL + 1) % 3, iL2 = (iL + 2) % 3; // indices of the two other links of triangle
    TLinkInSet link1 = theLinks.find( _sides[iL1] );
    TLinkInSet link2 = theLinks.find( _sides[iL2] );

    const QFace *f1 = 0, *f2 = 0;  // adjacent faces
    bool isBndLink1 = true, isBndLink2 = true;
    if ( link1 != theLinks.end() && link2 != theLinks.end() )
    {
      f1 = link1->NextFace( this );
      f2 = link2->NextFace( this );

      isBndLink1 = ( theLink->MediumPos() > (*link1)->MediumPos() );
      isBndLink2 = ( theLink->MediumPos() > (*link2)->MediumPos() );
      if ( theStep == theFirstStep ) // (issue 22541) quad-dominant mesh
      {
        if ( !isBndLink1 && !f1 )
          f1 = (*link1)->GetContinuesFace( this ); // get a quadrangle face
        if ( !isBndLink2 && !f2 )
          f2 = (*link2)->GetContinuesFace( this );
      }
    }
    else if ( _sides.size() < 4 )
      return thePrevLen;      

    // propagate to adjacent faces till limit step or boundary
    double len1 = thePrevLen + (theLink->MiddlePnt() - _sides[iL1]->MiddlePnt()).Modulus();
    double len2 = thePrevLen + (theLink->MiddlePnt() - _sides[iL2]->MiddlePnt()).Modulus();
    gp_Vec linkDir1(0,0,0); // initialize to avoid valgrind error ("Conditional jump...")
    gp_Vec linkDir2(0,0,0);
    try {
      OCC_CATCH_SIGNALS;
      if ( f1 && !isBndLink1 )
        len1 = f1->MoveByBoundary
          ( *link1, theRefVec, theLinks, theFaceHelper, len1, theStep-1, &linkDir1, theSign);
      else
        linkDir1 = LinkNorm( iL1/*, theFaceHelper*/ );
    } catch (...) {
      MSG( " --------------- EXCEPTION");
      return thePrevLen;
    }
    try {
      OCC_CATCH_SIGNALS;
      if ( f2 && !isBndLink2 )
        len2 = f2->MoveByBoundary
          ( *link2, theRefVec, theLinks, theFaceHelper, len2, theStep-1, &linkDir2, theSign);
      else
        linkDir2 = LinkNorm( iL2/*, theFaceHelper*/ );
    } catch (...) {
      MSG( " --------------- EXCEPTION");
      return thePrevLen;
    }

    double fullLen = 0;
    if ( theStep != theFirstStep )
    {
      // choose chain length by direction of propagation most codirected with theRefVec
      bool choose1 = ( theRefVec * linkDir1 * theSign > theRefVec * linkDir2 * theSign );
      fullLen = choose1 ? len1 : len2;
      double r = thePrevLen / fullLen;

      gp_Vec move = linkNorm * refProj * ( 1 - r );
      theLink->Move( move, /*sum=*/true );

      MSG(string(theStep,'.')<<" Move "<< theLink->_mediumNode->GetID()<<
          " by " << refProj * ( 1 - r ) << " following " <<
          (choose1 ? *link1->_qlink : *link2->_qlink)); // warning: link1 can be invalid

      if ( theLinkNorm ) *theLinkNorm = linkNorm;
    }
    return fullLen;
  }

  //================================================================================
  /*!
   * \brief Checks if the face is distorted due to bentLink
   */
  //================================================================================
#if 0
  bool QFace::IsSpoiled(const QLink* bentLink ) const
  {
    // code is valid for convex faces only
    gp_XYZ gc(0,0,0);
    for ( TIDSortedNodeSet::const_iterator n = begin(); n!=end(); ++n)
      gc += XYZ( *n ) / size();
    for (unsigned i = 0; i < _sides.size(); ++i )
    {
      if ( _sides[i] == bentLink ) continue;
      gp_Vec linkNorm = _normal ^ gp_Vec( XYZ(_sides[i]->node1()), XYZ(_sides[i]->node2()));
      gp_Vec vecOut( gc, _sides[i]->MiddlePnt() );
      if ( linkNorm * vecOut < 0 )
        linkNorm.Reverse();
      double mag2 = linkNorm.SquareMagnitude();
      if ( mag2 > numeric_limits<double>::min() )
        linkNorm /= sqrt( mag2 );
      gp_Vec vecBent    ( _sides[i]->MiddlePnt(), bentLink->MediumPnt());
      gp_Vec vecStraight( _sides[i]->MiddlePnt(), bentLink->MiddlePnt());
      if ( vecBent * linkNorm > -0.1*vecStraight.Magnitude() )
        return true;
    }
    return false;
    
  }
#endif
  //================================================================================
  /*!
   * \brief Find pairs of continues faces 
   */
  //================================================================================

  void QLink::SetContinuesFaces() const
  {
    //       x0         x - QLink, [-|] - QFace, v - volume
    //   v0  |   v1   
    //       |          Between _faces of link x2 two vertical faces are continues
    // x1----x2-----x3  and two horizontal faces are continues. We set vertical faces
    //       |          to _faces[0] and _faces[1] and horizontal faces to
    //   v2  |   v3     _faces[2] and _faces[3] (or vise versa).
    //       x4

    if ( _faces.empty() )
      return;
    int iFaceCont = -1, nbBoundary = 0, iBoundary[2]={-1,-1};
    if ( _faces[0]->IsBoundary() )
      iBoundary[ nbBoundary++ ] = 0;
    for ( int iF = 1; iFaceCont < 0 && iF < _faces.size(); ++iF )
    {
      // look for a face bounding none of volumes bound by _faces[0]
      bool sameVol = false;
      int nbVol = _faces[iF]->NbVolumes();
      for ( int iV = 0; !sameVol && iV < nbVol; ++iV )
        sameVol = ( _faces[iF]->_volumes[iV] == _faces[0]->_volumes[0] ||
                    _faces[iF]->_volumes[iV] == _faces[0]->_volumes[1]);
      if ( !sameVol )
        iFaceCont = iF;
      if ( _faces[iF]->IsBoundary() )
        iBoundary[ nbBoundary++ ] = iF;
    }
    // Set continues faces: arrange _faces to have
    // _faces[0] continues to _faces[1]
    // _faces[2] continues to _faces[3]
    if ( nbBoundary == 2 ) // bnd faces are continues
    {
      if (( iBoundary[0] < 2 ) != ( iBoundary[1] < 2 ))
      {
        int iNear0 = iBoundary[0] < 2 ? 1-iBoundary[0] : 5-iBoundary[0];
        std::swap( _faces[ iBoundary[1] ], _faces[iNear0] );
      }
    }
    else if ( iFaceCont > 0 ) // continues faces found
    {
      if ( iFaceCont != 1 )
        std::swap( _faces[1], _faces[iFaceCont] );
    }
    else if ( _faces.size() > 1 ) // not found, set NULL by the first face
    {
      _faces.insert( ++_faces.begin(), (QFace*) 0 );
    }
  }
  //================================================================================
  /*!
   * \brief Return a face continues to the given one
   */
  //================================================================================

  const QFace* QLink::GetContinuesFace( const QFace* face ) const
  {
    for ( int i = 0; i < _faces.size(); ++i ) {
      if ( _faces[i] == face ) {
        int iF = i < 2 ? 1-i : 5-i;
        return iF < _faces.size() ? _faces[iF] : 0;
      }
    }
    return 0;
  }
  //================================================================================
  /*!
   * \brief True if link is on mesh boundary
   */
  //================================================================================

  bool QLink::OnBoundary() const
  {
    for ( int i = 0; i < _faces.size(); ++i )
      if (_faces[i] && _faces[i]->IsBoundary()) return true;
    return false;
  }
  //================================================================================
  /*!
   * \brief Return normal of link of the chain
   */
  //================================================================================

  gp_Vec TChainLink::Normal() const {
    gp_Vec norm;
    if (_qfaces[0]) norm  = _qfaces[0]->_normal;
    if (_qfaces[1]) norm += _qfaces[1]->_normal;
    return norm;
  }
  //================================================================================
  /*!
   * \brief Test link curvature taking into account size of faces
   */
  //================================================================================

  bool TChainLink::IsStraight() const
  {
    bool isStraight = _qlink->IsStraight();
    if ( isStraight && _qfaces[0] && !_qfaces[1] )
    {
      int i = _qfaces[0]->LinkIndex( _qlink );
      int iOpp = ( i + 2 ) % _qfaces[0]->_sides.size();
      gp_XYZ mid1 = _qlink->MiddlePnt();
      gp_XYZ mid2 = _qfaces[0]->_sides[ iOpp ]->MiddlePnt();
      double faceSize2 = (mid1-mid2).SquareModulus();
      isStraight = _qlink->_nodeMove.SquareMagnitude() < 1/10./10. * faceSize2;
    }
    return isStraight;
  }
  
  //================================================================================
  /*!
   * \brief Move medium nodes of vertical links of pentahedrons adjacent by side faces
   */
  //================================================================================

  void fixPrism( TChain& allLinks )
  {
    // separate boundary links from internal ones
    typedef set<const QLink* /*, QLink::PtrComparator */> QLinkSet;
    QLinkSet interLinks, bndLinks1, bndLink2;

    bool isCurved = false;
    for ( TChain::iterator lnk = allLinks.begin(); lnk != allLinks.end(); ++lnk ) {
      if ( (*lnk)->OnBoundary() )
        bndLinks1.insert( lnk->_qlink );
      else
        interLinks.insert( lnk->_qlink );
      isCurved = isCurved || !lnk->IsStraight();
    }
    if ( !isCurved )
      return; // no need to move

    QLinkSet *curBndLinks = &bndLinks1, *newBndLinks = &bndLink2;

    while ( !interLinks.empty() && !curBndLinks->empty() )
    {
      // propagate movement from boundary links to connected internal links
      QLinkSet::iterator bnd = curBndLinks->begin(), bndEnd = curBndLinks->end();
      for ( ; bnd != bndEnd; ++bnd )
      {
        const QLink* bndLink = *bnd;
        for ( int i = 0; i < bndLink->_faces.size(); ++i ) // loop on faces of bndLink
        {
          const QFace* face = bndLink->_faces[i]; // quadrange lateral face of a prism
          if ( !face ) continue;
          // find and move internal link opposite to bndLink within the face
          int interInd = ( face->LinkIndex( bndLink ) + 2 ) % face->_sides.size();
          const QLink* interLink = face->_sides[ interInd ];
          QLinkSet::iterator pInterLink = interLinks.find( interLink );
          if ( pInterLink == interLinks.end() ) continue; // not internal link
          interLink->Move( bndLink->_nodeMove );
          // treated internal links become new boundary ones
          interLinks.erase( pInterLink );
          newBndLinks->insert( interLink );
        }
      }
      curBndLinks->clear();
      std::swap( curBndLinks, newBndLinks );
    }
  }

  //================================================================================
  /*!
   * \brief Fix links of continues triangles near curved boundary
   */
  //================================================================================

  void fixTriaNearBoundary( TChain & allLinks, SMESH_MesherHelper& /*helper*/)
  {
    if ( allLinks.empty() ) return;

    TLinkSet linkSet( allLinks.begin(), allLinks.end());
    TLinkInSet linkIt = linkSet.begin(), linksEnd = linkSet.end();

    for ( linkIt = linkSet.begin(); linkIt != linksEnd; ++linkIt)
    {
      if ( linkIt->IsBoundary() && !linkIt->IsStraight() && linkIt->_qfaces[0])
      {
        // move iff a boundary link is bent towards inside of a face (issue 0021084)
        const QFace* face = linkIt->_qfaces[0];
        gp_XYZ pIn = ( face->_sides[0]->MiddlePnt() +
                       face->_sides[1]->MiddlePnt() +
                       face->_sides[2]->MiddlePnt() ) / 3.;
        gp_XYZ insideDir( pIn - (*linkIt)->MiddlePnt());
        bool linkBentInside = ((*linkIt)->_nodeMove.Dot( insideDir ) > 0 );
        //if ( face->IsSpoiled( linkIt->_qlink ))
        if ( linkBentInside )
          face->MoveByBoundary( *linkIt, (*linkIt)->_nodeMove, linkSet );
      }
    }
  }

  //================================================================================
  /*!
   * \brief Detect rectangular structure of links and build chains from them
   */
  //================================================================================

  enum TSplitTriaResult {
    _OK, _NO_CORNERS, _FEW_ROWS, _MANY_ROWS, _NO_SIDELINK, _BAD_MIDQUAD, _NOT_RECT,
    _NO_MIDQUAD, _NO_UPTRIA, _BAD_SET_SIZE, _BAD_CORNER, _BAD_START, _NO_BOTLINK, _TWISTED_CHAIN };

  TSplitTriaResult splitTrianglesIntoChains( TChain &            allLinks,
                                             vector< TChain> &   resultChains,
                                             SMDS_TypeOfPosition pos )
  {
    // put links in the set and evalute number of result chains by number of boundary links
    TLinkSet linkSet;
    int nbBndLinks = 0;
    for ( TChain::iterator lnk = allLinks.begin(); lnk != allLinks.end(); ++lnk ) {
      linkSet.insert( *lnk );
      nbBndLinks += lnk->IsBoundary();
    }
    resultChains.clear();
    resultChains.reserve( nbBndLinks / 2 );

    TLinkInSet linkIt, linksEnd = linkSet.end();

    // find a boundary link with corner node; corner node has position pos-2
    // i.e. SMDS_TOP_VERTEX for links on faces and SMDS_TOP_EDGE for
    // links in volume
    SMDS_TypeOfPosition cornerPos = SMDS_TypeOfPosition(pos-2);
    const SMDS_MeshNode* corner = 0;
    for ( linkIt = linkSet.begin(); linkIt != linksEnd; ++linkIt )
      if ( linkIt->IsBoundary() && (corner = (*linkIt)->EndPosNode(cornerPos)))
        break;
    if ( !corner)
      return _NO_CORNERS;

    TLinkInSet           startLink = linkIt;
    const SMDS_MeshNode* startCorner = corner;
    vector< TChain* >    rowChains;
    int iCol = 0;

    while ( startLink != linksEnd) // loop on columns
    {
      // We suppose we have a rectangular structure like shown here. We have found a
      //               corner of the rectangle (startCorner) and a boundary link sharing  
      //    |/  |/  |  the startCorner (startLink). We are going to loop on rows of the   
      //  --o---o---o  structure making several chains at once. One chain (columnChain)   
      //    |\  |  /|  starts at startLink and continues upward (we look at the structure 
      //  \ | \ | / |  from such point that startLink is on the bottom of the structure). 
      //   \|  \|/  |  While going upward we also fill horizontal chains (rowChains) we   
      //  --o---o---o  encounter.                                                         
      //   /|\  |\  |
      //  / | \ | \ |  startCorner
      //    |  \|  \|,'
      //  --o---o---o
      //          `.startLink

      if ( resultChains.size() == nbBndLinks / 2 )
        return _NOT_RECT;
      resultChains.push_back( TChain() );
      TChain& columnChain = resultChains.back();

      TLinkInSet botLink = startLink; // current horizontal link to go up from
      corner = startCorner; // current corner the botLink ends at
      int iRow = 0;
      while ( botLink != linksEnd ) // loop on rows
      {
        // add botLink to the columnChain
        columnChain.push_back( *botLink );

        const QFace* botTria = botLink->_qfaces[0]; // bottom triangle bound by botLink
        if ( !botTria )
        { // the column ends
          if ( botLink == startLink )
            return _TWISTED_CHAIN; // issue 0020951
          linkSet.erase( botLink );
          if ( iRow != rowChains.size() )
            return _FEW_ROWS; // different nb of rows in columns
          break;
        }
        // find the link dividing the quadrangle (midQuadLink) and vertical boundary
        // link ending at <corner> (sideLink); there are two cases:
        // 1) midQuadLink does not end at <corner>, then we easily find it by botTria,
        //   since midQuadLink is not at boundary while sideLink is.
        // 2) midQuadLink ends at <corner>
        bool isCase2;
        TLinkInSet midQuadLink = linksEnd;
        TLinkInSet sideLink = botTria->GetBoundaryLink( linkSet, *botLink, &midQuadLink,
                                                        corner, &isCase2 );
        if ( isCase2 ) { // find midQuadLink among links of botTria
          midQuadLink = botTria->GetLinkByNode( linkSet, *botLink, corner );
          if ( midQuadLink->IsBoundary() )
            return _BAD_MIDQUAD;
        }
        if ( sideLink == linksEnd || midQuadLink == linksEnd || sideLink == midQuadLink )
          return sideLink == linksEnd ? _NO_SIDELINK : _NO_MIDQUAD;

        // fill chains
        columnChain.push_back( *midQuadLink );
        if ( iRow >= rowChains.size() ) {
          if ( iCol > 0 )
            return _MANY_ROWS; // different nb of rows in columns
          if ( resultChains.size() == nbBndLinks / 2 )
            return _NOT_RECT;
          resultChains.push_back( TChain() );
          rowChains.push_back( & resultChains.back() );
        }
        rowChains[iRow]->push_back( *sideLink );
        rowChains[iRow]->push_back( *midQuadLink );

        const QFace* upTria = midQuadLink->NextFace( botTria ); // upper tria of the rectangle
        if ( !upTria)
          return _NO_UPTRIA;
        if ( iRow == 0 ) {
          // prepare startCorner and startLink for the next column
          startCorner = startLink->NextNode( startCorner );
          if (isCase2)
            startLink = botTria->GetBoundaryLink( linkSet, *botLink, 0, startCorner );
          else
            startLink = upTria->GetBoundaryLink( linkSet, *midQuadLink, 0, startCorner );
          // check if no more columns remains
          if ( startLink != linksEnd ) {
            const SMDS_MeshNode* botNode = startLink->NextNode( startCorner );
            if ( (isCase2 ? botTria : upTria)->Contains( botNode ))
              startLink = linksEnd; // startLink bounds upTria or botTria
            else if ( startLink == botLink || startLink == midQuadLink || startLink == sideLink )
              return _BAD_START;
          }
        }
        // find bottom link and corner for the next row
        corner = sideLink->NextNode( corner );
        // next bottom link ends at the new corner
        linkSet.erase( botLink );
        botLink = upTria->GetLinkByNode( linkSet, (isCase2 ? *sideLink : *midQuadLink), corner );
        if ( botLink == linksEnd || botLink == midQuadLink || botLink == sideLink)
          return _NO_BOTLINK;
        if ( midQuadLink == startLink || sideLink == startLink )
          return _TWISTED_CHAIN; // issue 0020951
        linkSet.erase( midQuadLink );
        linkSet.erase( sideLink );

        // make faces neighboring the found ones be boundary
        if ( startLink != linksEnd ) {
          const QFace* tria = isCase2 ? botTria : upTria;
          for ( int iL = 0; iL < 3; ++iL ) {
            linkIt = linkSet.find( tria->_sides[iL] );
            if ( linkIt != linksEnd )
              linkIt->RemoveFace( tria );
          }
        }
        if ( botLink->_qfaces[0] == upTria || botLink->_qfaces[1] == upTria )
          botLink->RemoveFace( upTria ); // make next botTria first in vector

        iRow++;
      } // loop on rows

      iCol++;
    }
    // In the linkSet, there must remain the last links of rowChains; add them
    if ( linkSet.size() != rowChains.size() )
      return _BAD_SET_SIZE;
    for ( int iRow = 0; iRow < rowChains.size(); ++iRow ) {
      // find the link (startLink) ending at startCorner
      corner = 0;
      for ( startLink = linkSet.begin(); startLink != linksEnd; ++startLink ) {
        if ( (*startLink)->node1() == startCorner ) {
          corner = (*startLink)->node2(); break;
        }
        else if ( (*startLink)->node2() == startCorner) {
          corner = (*startLink)->node1(); break;
        }
      }
      if ( startLink == linksEnd )
        return _BAD_CORNER;
      rowChains[ iRow ]->push_back( *startLink );
      linkSet.erase( startLink );
      startCorner = corner;
    }

    return _OK;
  }

  //================================================================================
  /*!
   * \brief Place medium nodes at the link middle for elements whose corner nodes
   *        are out of geometrical boundary to prevent distorting elements.
   *        Issue 0020982, note 0013990
   */
  //================================================================================

  void force3DOutOfBoundary( SMESH_MesherHelper&    theHelper,
                             SMESH_ComputeErrorPtr& theError)
  {
    SMESHDS_Mesh* meshDS = theHelper.GetMeshDS();
    TopoDS_Shape  shape = theHelper.GetSubShape().Oriented( TopAbs_FORWARD );
    if ( shape.IsNull() ) return;

    if ( !theError ) theError = SMESH_ComputeError::New();

    gp_XYZ faceNorm;

    if ( shape.ShapeType() == TopAbs_FACE ) // 2D
    {
      if ( theHelper.GetMesh()->NbTriangles( ORDER_QUADRATIC ) < 1 ) return;

      SMESHDS_SubMesh* faceSM = meshDS->MeshElements( shape );
      if ( !faceSM ) return;

      const TopoDS_Face&      face = TopoDS::Face( shape );
      Handle(Geom_Surface) surface = BRep_Tool::Surface( face );

      TopExp_Explorer edgeIt( face, TopAbs_EDGE );
      for ( ; edgeIt.More(); edgeIt.Next() ) // loop on EDGEs of a FACE
      {
        // check if the EDGE needs checking
        const TopoDS_Edge& edge = TopoDS::Edge( edgeIt.Current() );
        if ( SMESH_Algo::isDegenerated( edge ) )
          continue;
        if ( theHelper.IsRealSeam( edge ) &&
             edge.Orientation() == TopAbs_REVERSED )
          continue;

        SMESHDS_SubMesh* edgeSM = meshDS->MeshElements( edge );
        if ( !edgeSM ) continue;

        double f,l;
        Handle(Geom2d_Curve) pcurve  = BRep_Tool::CurveOnSurface( edge, face, f, l );
        BRepAdaptor_Curve    curve3D( edge );
        switch ( curve3D.GetType() ) {
        case GeomAbs_Line: continue;
        case GeomAbs_Circle:
        case GeomAbs_Ellipse:
        case GeomAbs_Hyperbola:
        case GeomAbs_Parabola:
          try
          {
            gp_Vec D1, D2, Du1, Dv1; gp_Pnt p;
            curve3D.D2( 0.5 * ( f + l ), p, D1, D2 );
            gp_Pnt2d uv = pcurve->Value( 0.5 * ( f + l ) );
            surface->D1( uv.X(), uv.Y(), p, Du1, Dv1 );
            gp_Vec fNorm = Du1 ^ Dv1;
            if ( fNorm.IsParallel( D2, M_PI * 25./180. ))
              continue; // face is normal to the curve3D

            gp_Vec curvNorm = fNorm ^ D1;
            if ( edge.Orientation() == TopAbs_REVERSED ) curvNorm.Reverse();
            if ( curvNorm * D2 > 0 )
              continue; // convex edge
          }
          catch ( Standard_Failure &)
          {
            continue;
          }
        }
        // get nodes shared by faces that may be distorted
        SMDS_NodeIteratorPtr nodeIt;
        if ( edgeSM->NbNodes() > 0 ) {
          nodeIt = edgeSM->GetNodes();
        }
        else {
          SMESHDS_SubMesh* vertexSM = meshDS->MeshElements( theHelper.IthVertex( 0, edge ));
          if ( !vertexSM )
            vertexSM = meshDS->MeshElements( theHelper.IthVertex( 1, edge ));
          if ( !vertexSM ) continue;
          nodeIt = vertexSM->GetNodes();
        }

        // find suspicious faces
        TIDSortedElemSet checkedFaces;
        vector< const SMDS_MeshNode* > nOnEdge( 2 );
        const SMDS_MeshNode*           nOnFace;
        while ( nodeIt->more() )
        {
          const SMDS_MeshNode* n      = nodeIt->next();
          SMDS_ElemIteratorPtr faceIt = n->GetInverseElementIterator( SMDSAbs_Face );
          while ( faceIt->more() )
          {
            const SMDS_MeshElement* f = faceIt->next();
            if ( !faceSM->Contains( f ) ||
                 f->NbNodes() < 6       || // check quadratic triangles only
                 !checkedFaces.insert( f ).second )
              continue;

            // get nodes on EDGE and on FACE of a suspicious face
            nOnEdge.clear(); nOnFace = 0;
            SMDS_MeshElement::iterator triNode = f->begin_nodes();
            for ( int nbN = 0; nbN < 3; ++triNode, ++nbN )
            {
              n = *triNode;
              if ( n->GetPosition()->GetDim() == 2 )
                nOnFace = n;
              else
                nOnEdge.push_back( n );
            }

            // check if nOnFace is inside the FACE
            if ( nOnFace && nOnEdge.size() == 2 )
            {
              theHelper.AddTLinks( static_cast< const SMDS_MeshFace* > ( f ));
              if ( !SMESH_MeshAlgos::FaceNormal( f, faceNorm, /*normalized=*/false ))
                continue;
              gp_XYZ edgeDir  = SMESH_TNodeXYZ( nOnEdge[0] ) - SMESH_TNodeXYZ( nOnEdge[1] );
              gp_XYZ edgeNorm = faceNorm ^ edgeDir;
              n = theHelper.GetMediumNode( nOnEdge[0], nOnEdge[1], true ); // find n, not create 
              gp_XYZ pN0     = SMESH_TNodeXYZ( nOnEdge[0] );
              gp_XYZ pMedium = SMESH_TNodeXYZ( n );                   // on-edge node location
              gp_XYZ pFaceN  = SMESH_TNodeXYZ( nOnFace );             // on-face node location
              double hMedium = edgeNorm * gp_Vec( pN0, pMedium ).XYZ();
              double hFace   = edgeNorm * gp_Vec( pN0, pFaceN ).XYZ();
              if ( Abs( hMedium ) > Abs( hFace * 0.6 ))
              {
                // nOnFace is out of FACE, move a medium on-edge node to the middle
                gp_XYZ pMid3D = 0.5 * ( pN0 + SMESH_TNodeXYZ( nOnEdge[1] ));
                meshDS->MoveNode( n, pMid3D.X(), pMid3D.Y(), pMid3D.Z() );
                MSG( "move OUT of face " << n );
                theError->myBadElements.push_back( f );
              }
            }
          }
        }
      }
      if ( !theError->myBadElements.empty() )
        theError->myName = EDITERR_NO_MEDIUM_ON_GEOM;
      return;

    } // 2D ==============================================================================

    if ( shape.ShapeType() == TopAbs_SOLID ) // 3D
    {
      if ( theHelper.GetMesh()->NbTetras  ( ORDER_QUADRATIC ) < 1 &&
           theHelper.GetMesh()->NbPyramids( ORDER_QUADRATIC ) < 1 ) return;

      SMESHDS_SubMesh* solidSM = meshDS->MeshElements( shape );
      if ( !solidSM ) return;

      // check if the SOLID is bound by concave FACEs
      vector< TopoDS_Face > concaveFaces;
      TopExp_Explorer faceIt( shape, TopAbs_FACE );
      for ( ; faceIt.More(); faceIt.Next() ) // loop on FACEs of a SOLID
      {
        const TopoDS_Face&  face = TopoDS::Face( faceIt.Current() );
        if ( !meshDS->MeshElements( face )) continue;

        BRepAdaptor_Surface surface( face );
        switch ( surface.GetType() ) {
        case GeomAbs_Plane: continue;
        case GeomAbs_Cylinder:
        case GeomAbs_Cone:
        case GeomAbs_Sphere:
          try
          {
            double u = 0.5 * ( surface.FirstUParameter() + surface.LastUParameter() );
            double v = 0.5 * ( surface.FirstVParameter() + surface.LastVParameter() );
            gp_Vec Du1, Dv1, Du2, Dv2, Duv2; gp_Pnt p;
            surface.D2( u,v, p, Du1, Dv1, Du2, Dv2, Duv2 );
            gp_Vec fNorm = Du1 ^ Dv1;
            if ( face.Orientation() == TopAbs_REVERSED ) fNorm.Reverse();
            bool concaveU = ( fNorm * Du2 > 1e-100 );
            bool concaveV = ( fNorm * Dv2 > 1e-100 );
            if ( concaveU || concaveV )
              concaveFaces.push_back( face );
          }
          catch ( Standard_Failure &)
          {
            concaveFaces.push_back( face );
          }
        }
      }
      if ( concaveFaces.empty() )
        return;

      // fix 2D mesh on the SOLID
      for ( faceIt.ReInit(); faceIt.More(); faceIt.Next() ) // loop on FACEs of a SOLID
      {
        SMESH_MesherHelper faceHelper( *theHelper.GetMesh() );
        faceHelper.SetSubShape( faceIt.Current() );
        force3DOutOfBoundary( faceHelper, theError );
      }

      // get an iterator over faces on concaveFaces
      vector< SMDS_ElemIteratorPtr > faceIterVec( concaveFaces.size() );
      for ( size_t i = 0; i < concaveFaces.size(); ++i )
        faceIterVec[i] = meshDS->MeshElements( concaveFaces[i] )->GetElements();
      typedef SMDS_IteratorOnIterators
        < const SMDS_MeshElement*, vector< SMDS_ElemIteratorPtr > > TIterOnIter;
      SMDS_ElemIteratorPtr faceIter( new TIterOnIter( faceIterVec ));

      // a seacher to check if a volume is close to a concave face
      std::unique_ptr< SMESH_ElementSearcher > faceSearcher
        ( SMESH_MeshAlgos::GetElementSearcher( *theHelper.GetMeshDS(), faceIter ));

      // classifier
      //BRepClass3d_SolidClassifier solidClassifier( shape );

      TIDSortedElemSet checkedVols, movedNodes;
      //for ( faceIt.ReInit(); faceIt.More(); faceIt.Next() ) // loop on FACEs of a SOLID
      for ( size_t iF = 0; iF < concaveFaces.size(); ++iF ) // loop on concave FACEs
      {
        //const TopoDS_Shape& face = faceIt.Current();
        const TopoDS_Shape& face = concaveFaces[ iF ];
        SMESHDS_SubMesh*  faceSM = meshDS->MeshElements( face );
        if ( !faceSM ) continue;

        // get nodes shared by volumes (tet and pyra) on the FACE that may be distorted
        SMDS_NodeIteratorPtr nodeIt;
        if ( faceSM->NbNodes() > 0 ) {
          nodeIt = faceSM->GetNodes();
        }
        else {
          TopExp_Explorer vertex( face, TopAbs_VERTEX );
          SMESHDS_SubMesh* vertexSM = meshDS->MeshElements( vertex.Current() );
          if ( !vertexSM ) continue;
          nodeIt = vertexSM->GetNodes();
        }
        // get ids of sub-shapes of the FACE
        set< int > subIDs;
        SMESH_subMeshIteratorPtr smIt =
          theHelper.GetMesh()->GetSubMesh( face )->getDependsOnIterator(/*includeSelf=*/true);
        while ( smIt->more() )
          subIDs.insert( smIt->next()->GetId() );

        // find suspicious volumes adjacent to the FACE
        vector< const SMDS_MeshNode* >    nOnFace( 4 );
        const SMDS_MeshNode*              nInSolid;
        while ( nodeIt->more() )
        {
          const SMDS_MeshNode* n     = nodeIt->next();
          SMDS_ElemIteratorPtr volIt = n->GetInverseElementIterator( SMDSAbs_Volume );
          while ( volIt->more() )
          {
            const SMDS_MeshElement* vol = volIt->next();
            int nbN = vol->NbCornerNodes();
            if ( ( nbN != 4 && nbN != 5 )  ||
                 !solidSM->Contains( vol ) ||
                 !checkedVols.insert( vol ).second )
              continue;

            // get nodes on FACE and in SOLID of a suspicious volume
            nOnFace.clear(); nInSolid = 0;
            SMDS_MeshElement::iterator volNode = vol->begin_nodes();
            for ( int nb = nbN; nb > 0; ++volNode, --nb )
            {
              n = *volNode;
              if ( n->GetPosition()->GetDim() == 3 )
                nInSolid = n;
              else if ( subIDs.count( n->getshapeId() ))
                nOnFace.push_back( n );
              else
                nInSolid = n;
            }
            if ( !nInSolid || nOnFace.size() != nbN - 1 )
              continue;

            // get size of the vol
            SMESH_TNodeXYZ pInSolid( nInSolid ), pOnFace0( nOnFace[0] );
            double volLength = pInSolid.SquareDistance( nOnFace[0] );
            for ( size_t i = 1; i < nOnFace.size(); ++i )
            {
              volLength = Max( volLength, pOnFace0.SquareDistance( nOnFace[i] ));
            }

            // check if vol is close to concaveFaces
            const SMDS_MeshElement* closeFace =
              faceSearcher->FindClosestTo( pInSolid, SMDSAbs_Face );
            if ( !closeFace ||
                 pInSolid.SquareDistance( closeFace->GetNode(0) ) > 4 * volLength )
              continue;

            // check if vol is distorted, i.e. a medium node is much closer
            // to nInSolid than the link middle
            bool isDistorted = false;
            SMDS_FaceOfNodes onFaceTria( nOnFace[0], nOnFace[1], nOnFace[2] );
            if ( !SMESH_MeshAlgos::FaceNormal( &onFaceTria, faceNorm, /*normalized=*/false ))
              continue;
            theHelper.AddTLinks( static_cast< const SMDS_MeshVolume* > ( vol ));
            vector< pair< SMESH_TLink, const SMDS_MeshNode* > > links;
            for ( size_t i = 0; i < nOnFace.size(); ++i ) // loop on links between nOnFace
              for ( size_t j = i+1; j < nOnFace.size(); ++j )
              {
                SMESH_TLink link( nOnFace[i], nOnFace[j] );
                TLinkNodeMap::const_iterator linkIt =
                  theHelper.GetTLinkNodeMap().find( link );
                if ( linkIt != theHelper.GetTLinkNodeMap().end() )
                {
                  links.push_back( make_pair( linkIt->first, linkIt->second  ));
                  if ( !isDistorted ) {
                    // compare projections of nInSolid and nMedium to face normal
                    gp_Pnt pMedium = SMESH_TNodeXYZ( linkIt->second );
                    double hMedium = faceNorm * gp_Vec( pOnFace0, pMedium ).XYZ();
                    double hVol    = faceNorm * gp_Vec( pOnFace0, pInSolid ).XYZ();
                    isDistorted = ( Abs( hMedium ) > Abs( hVol * 0.5 ));
                  }
                }
              }
            // move medium nodes to link middle
            if ( isDistorted )
            {
              for ( size_t i = 0; i < links.size(); ++i )
              {
                const SMDS_MeshNode* nMedium = links[i].second;
                if ( movedNodes.insert( nMedium ).second )
                {
                  gp_Pnt pMid3D = 0.5 * ( SMESH_TNodeXYZ( links[i].first.node1() ) +
                                          SMESH_TNodeXYZ( links[i].first.node2() ));
                  meshDS->MoveNode( nMedium, pMid3D.X(), pMid3D.Y(), pMid3D.Z() );
                  MSG( "move OUT of solid " << nMedium );
                }
              }
              theError->myBadElements.push_back( vol );
            }
          } // loop on volumes sharing a node on FACE
        } // loop on nodes on FACE
      }  // loop on FACEs of a SOLID

      if ( !theError->myBadElements.empty() )
        theError->myName = EDITERR_NO_MEDIUM_ON_GEOM;
    } // 3D case
  }

} //namespace

//=======================================================================
/*!
 * \brief Move medium nodes of faces and volumes to fix distorted elements
 * \param error - container of fixed distorted elements
 * \param volumeOnly - to fix nodes on faces or not, if the shape is solid
 * 
 * Issue 0020307: EDF 992 SMESH : Linea/Quadratic with Medium Node on Geometry
 */
//=======================================================================

void SMESH_MesherHelper::FixQuadraticElements(SMESH_ComputeErrorPtr& compError,
                                              bool                   volumeOnly)
{
  // setenv NO_FixQuadraticElements to know if FixQuadraticElements() is guilty of bad conversion
  if ( getenv("NO_FixQuadraticElements") )
    return;

  // 0. Apply algorithm to SOLIDs or FACEs
  // ----------------------------------------------
  if ( myShape.IsNull() ) {
    if ( !myMesh->HasShapeToMesh() ) return;
    SetSubShape( myMesh->GetShapeToMesh() );

#ifdef _DEBUG_
    int nbSolids = 0;
    TopTools_IndexedMapOfShape solids;
    TopExp::MapShapes(myShape,TopAbs_SOLID,solids);
    nbSolids = solids.Extent();
#endif
    TopTools_MapOfShape faces; // faces not in solid or in not meshed solid
    for ( TopExp_Explorer f(myShape,TopAbs_FACE,TopAbs_SOLID); f.More(); f.Next() ) {
      faces.Add( f.Current() ); // not in solid
    }
    for ( TopExp_Explorer s(myShape,TopAbs_SOLID); s.More(); s.Next() ) {
      if ( myMesh->GetSubMesh( s.Current() )->IsEmpty() ) { // get faces of solid
        for ( TopExp_Explorer f( s.Current(), TopAbs_FACE); f.More(); f.Next() )
          faces.Add( f.Current() ); // in not meshed solid
      }
      else { // fix nodes in the solid and its faces
#ifdef _DEBUG_
        MSG("FIX SOLID " << nbSolids-- << " #" << GetMeshDS()->ShapeToIndex(s.Current()));
#endif
        SMESH_MesherHelper h(*myMesh);
        h.SetSubShape( s.Current() );
        h.ToFixNodeParameters(true);
        h.FixQuadraticElements( compError, false );
      }
    }
    // fix nodes on geom faces
#ifdef _DEBUG_
    int nbfaces = faces.Extent(); /*avoid "unused varianbles": */ nbfaces++, nbfaces--; 
#endif
    for ( TopTools_MapIteratorOfMapOfShape fIt( faces ); fIt.More(); fIt.Next() ) {
      MSG("FIX FACE " << nbfaces-- << " #" << GetMeshDS()->ShapeToIndex(fIt.Key()));
      SMESH_MesherHelper h(*myMesh);
      h.SetSubShape( fIt.Key() );
      h.ToFixNodeParameters(true);
      h.FixQuadraticElements( compError, true);
    }
    //perf_print_all_meters(1);
    if ( compError && compError->myName == EDITERR_NO_MEDIUM_ON_GEOM )
      compError->myComment = "during conversion to quadratic, "
        "some medium nodes were not placed on geometry to avoid distorting elements";
    return;
  }

  // 1. Find out type of elements and get iterator on them
  // ---------------------------------------------------

  SMDS_ElemIteratorPtr elemIt;
  SMDSAbs_ElementType elemType = SMDSAbs_All;

  SMESH_subMesh* submesh = myMesh->GetSubMeshContaining( myShapeID );
  if ( !submesh )
    return;
  if ( SMESHDS_SubMesh* smDS = submesh->GetSubMeshDS() ) {
    elemIt = smDS->GetElements();
    if ( elemIt->more() ) {
      elemType = elemIt->next()->GetType();
      elemIt = smDS->GetElements();
    }
  }
  if ( !elemIt || !elemIt->more() || elemType < SMDSAbs_Face )
    return;

  // 2. Fill in auxiliary data structures
  // ----------------------------------

  set< QLink > links;
  set< QFace > faces;
  set< QLink >::iterator pLink;
  set< QFace >::iterator pFace;

  bool isCurved = false;
  //bool hasRectFaces = false;
  //set<int> nbElemNodeSet;
  SMDS_VolumeTool volTool;

  TIDSortedNodeSet apexOfPyramid;
  const int apexIndex = 4;

  // Issue 0020982
  // Move medium nodes to the link middle for elements whose corner nodes
  // are out of geometrical boundary to fix distorted elements.
  force3DOutOfBoundary( *this, compError );

  if ( elemType == SMDSAbs_Volume )
  {
    while ( elemIt->more() ) // loop on volumes
    {
      const SMDS_MeshElement* vol = elemIt->next();
      if ( !vol->IsQuadratic() || !volTool.Set( vol ))
        return;
      double volMinSize2 = -1.;
      for ( int iF = 0; iF < volTool.NbFaces(); ++iF ) // loop on faces of volume
      {
        int nbN = volTool.NbFaceNodes( iF );
        //nbElemNodeSet.insert( nbN );
        const SMDS_MeshNode** faceNodes = volTool.GetFaceNodes( iF );
        vector< const QLink* > faceLinks( nbN/2 );
        for ( int iN = 0; iN < nbN; iN += 2 ) // loop on links of a face
        {
          // store QLink
          QLink link( faceNodes[iN], faceNodes[iN+2], faceNodes[iN+1] );
          pLink = links.insert( link ).first;
          faceLinks[ iN/2 ] = & *pLink;

          if ( link.MediumPos() == SMDS_TOP_3DSPACE )
          {
            if ( !link.IsStraight() )
              return; // already fixed
          }
          else if ( !isCurved )
          {
            if ( volMinSize2 < 0 ) volMinSize2 = volTool.MinLinearSize2();
            isCurved = !isStraightLink( volMinSize2, link._nodeMove.SquareMagnitude() );
          }
        }
        // store QFace
        pFace = faces.insert( QFace( faceLinks )).first;
        if ( pFace->NbVolumes() == 0 )
          pFace->AddSelfToLinks();
        pFace->SetVolume( vol );
//         hasRectFaces = hasRectFaces ||
//           ( volTool.GetVolumeType() == SMDS_VolumeTool::QUAD_HEXA ||
//             volTool.GetVolumeType() == SMDS_VolumeTool::QUAD_PENTA );
#ifdef _DEBUG_
        if ( nbN == 6 )
          pFace->_face = GetMeshDS()->FindFace(faceNodes[0],faceNodes[2],faceNodes[4]);
        else
          pFace->_face = GetMeshDS()->FindFace(faceNodes[0],faceNodes[2],
                                               faceNodes[4],faceNodes[6] );
#endif
      }
      // collect pyramid apexes for further correction
      if ( vol->NbCornerNodes() == 5 )
        apexOfPyramid.insert( vol->GetNode( apexIndex ));
    }
    set< QLink >::iterator pLink = links.begin();
    for ( ; pLink != links.end(); ++pLink )
      pLink->SetContinuesFaces();
  }
  else
  {
    while ( elemIt->more() ) // loop on faces
    {
      const SMDS_MeshElement* face = elemIt->next();
      if ( !face->IsQuadratic() )
        continue;
      //nbElemNodeSet.insert( face->NbNodes() );
      int nbN = face->NbNodes()/2;
      vector< const QLink* > faceLinks( nbN );
      for ( int iN = 0; iN < nbN; ++iN ) // loop on links of a face
      {
        // store QLink
        QLink link( face->GetNode(iN), face->GetNode((iN+1)%nbN), face->GetNode(iN+nbN) );
        pLink = links.insert( link ).first;
        faceLinks[ iN ] = & *pLink;
        if ( !isCurved &&
             link.node1()->GetPosition()->GetTypeOfPosition() < 2 &&
             link.node2()->GetPosition()->GetTypeOfPosition() < 2 )
          isCurved = !link.IsStraight();
      }
      // store QFace
      pFace = faces.insert( QFace( faceLinks )).first;
      pFace->AddSelfToLinks();
      //hasRectFaces = ( hasRectFaces || nbN == 4 );
    }
  }
  if ( !isCurved )
    return; // no curved edges of faces

  // 3. Compute displacement of medium nodes
  // ---------------------------------------

  SMESH_MesherHelper faceHlp(*myMesh);

  // two loops on QFaces: the first is to treat boundary links, the second is for internal ones.
  TopLoc_Location loc;
  bool checkUV;
  // not to treat boundary of volumic sub-mesh.
  int isInside = ( elemType == SMDSAbs_Volume && volumeOnly ) ? 1 : 0;
  for ( ; isInside < 2; ++isInside )
  {
    MSG( "--------------- LOOP (inside=" << isInside << ") ------------------");
    SMDS_TypeOfPosition pos = isInside ? SMDS_TOP_3DSPACE : SMDS_TOP_FACE;
    SMDS_TypeOfPosition bndPos = isInside ? SMDS_TOP_FACE : SMDS_TOP_EDGE;

    for ( pFace = faces.begin(); pFace != faces.end(); ++pFace ) {
      if ( bool(isInside) == pFace->IsBoundary() )
        continue;
      for ( int dir = 0; dir < 2; ++dir ) // 2 directions of propagation from the quadrangle
      {
        MSG( "CHAIN");
        // make chain of links connected via continues faces
        int error = ERR_OK;
        TChain rawChain;
        if ( !pFace->GetLinkChain( dir, rawChain, pos, error) && error ==ERR_UNKNOWN ) continue;
        rawChain.reverse();
        if ( !pFace->GetLinkChain( dir+2, rawChain, pos, error ) && error ==ERR_UNKNOWN ) continue;

        vector< TChain > chains;
        if ( error == ERR_OK ) { // chain contains continues rectangles
          chains.resize(1);
          chains[0].splice( chains[0].begin(), rawChain );
        }
        else if ( error == ERR_TRI ) {  // chain contains continues triangles
          TSplitTriaResult res = splitTrianglesIntoChains( rawChain, chains, pos );
          if ( res != _OK ) { // not 'quadrangles split into triangles' in chain
            fixTriaNearBoundary( rawChain, *this );
            break;
          }
        }
        else if ( error == ERR_PRISM ) { // quadrangle side faces of prisms
          fixPrism( rawChain );
          break;
        }
        else {
          continue;
        }
        for ( int iC = 0; iC < chains.size(); ++iC )
        {
          TChain& chain = chains[iC];
          if ( chain.empty() ) continue;
          if ( chain.front().IsStraight() && chain.back().IsStraight() ) {
            MSG("3D straight - ignore");
            continue;
          }
          if ( chain.front()->MediumPos() > bndPos ||
               chain.back() ->MediumPos() > bndPos ) {
            MSG("Internal chain - ignore");
            continue;
          }
          // mesure chain length and compute link position along the chain
          double chainLen = 0;
          vector< double > linkPos;
          TChain savedChain; // backup
          MSGBEG( "Link medium nodes: ");
          TChain::iterator link0 = chain.begin(), link1 = chain.begin(), link2;
          for ( ++link1; link1 != chain.end(); ++link1, ++link0 ) {
            MSGBEG( (*link0)->_mediumNode->GetID() << "-" <<(*link1)->_mediumNode->GetID()<<" ");
            double len = ((*link0)->MiddlePnt() - (*link1)->MiddlePnt()).Modulus();
            while ( len < numeric_limits<double>::min() ) { // remove degenerated link
              if ( savedChain.empty() ) savedChain = chain;
              link1 = chain.erase( link1 );
              if ( link1 == chain.end() )
                break;
              len = ((*link0)->MiddlePnt() - (*link1)->MiddlePnt()).Modulus();
            }
            chainLen += len;
            linkPos.push_back( chainLen );
          }
          MSG("");
          if ( linkPos.size() <= 2 && savedChain.size() > 2 ) {
            //continue;
            linkPos.clear();
            chainLen = 0;
            chain = savedChain;
            for ( link1 = chain.begin(); link1 != chain.end(); ++link1 ) {
              chainLen += 1;
              linkPos.push_back( chainLen );
            }
          }
          gp_Vec move0 = chain.front()->_nodeMove;
          gp_Vec move1 = chain.back ()->_nodeMove;

          TopoDS_Face face;
          if ( !isInside )
          {
            // compute node displacement of end links of chain in parametric space of FACE
            TChainLink& linkOnFace = *(++chain.begin());
            const SMDS_MeshNode* nodeOnFace = linkOnFace->_mediumNode;
            TopoDS_Shape f = GetSubShapeByNode( nodeOnFace, GetMeshDS() );
            if ( !f.IsNull() && f.ShapeType() == TopAbs_FACE )
            {
              face = TopoDS::Face( f );
              faceHlp.SetSubShape( face );
              Handle(Geom_Surface) surf = BRep_Tool::Surface(face,loc);
              //bool isStraight[2]; // commented for issue 0023118
              for ( int is1 = 0; is1 < 2; ++is1 ) // move0 or move1
              {
                TChainLink& link = is1 ? chain.back() : chain.front();
                gp_XY uvm  = faceHlp.GetNodeUV( face, link->_mediumNode, nodeOnFace, &checkUV );
                gp_XY uv1  = faceHlp.GetNodeUV( face, link->node1(),     nodeOnFace, &checkUV );
                gp_XY uv2  = faceHlp.GetNodeUV( face, link->node2(),     nodeOnFace, &checkUV );
                gp_XY uv12 = faceHlp.GetMiddleUV( surf, uv1, uv2 );
                // uvMove = uvm - uv12
                gp_XY uvMove = ApplyIn2D(surf, uvm, uv12, gp_XY_Subtracted, /*inPeriod=*/false);
                ( is1 ? move1 : move0 ).SetCoord( uvMove.X(), uvMove.Y(), 0 );
                if ( !is1 ) // correct nodeOnFace for move1 (issue 0020919)
                  nodeOnFace = (*(++chain.rbegin()))->_mediumNode;
                // isStraight[is1] = isStraightLink( (uv2-uv1).SquareModulus(),
                //                                   10 * uvMove.SquareModulus());
              }
              // if ( isStraight[0] && isStraight[1] ) {
              //   MSG("2D straight - ignore");
              //   continue; // straight - no need to move nodes of internal links
              // }

              // check if a chain is already fixed
              gp_XY uvm  = faceHlp.GetNodeUV( face, linkOnFace->_mediumNode, 0, &checkUV );
              gp_XY uv1  = faceHlp.GetNodeUV( face, linkOnFace->node1(), nodeOnFace, &checkUV );
              gp_XY uv2  = faceHlp.GetNodeUV( face, linkOnFace->node2(), nodeOnFace, &checkUV );
              gp_XY uv12 = faceHlp.GetMiddleUV( surf, uv1, uv2 );
              if (( uvm - uv12 ).SquareModulus() > 1e-10 )
              {
                MSG("Already fixed - ignore");
                continue;
              }
            }
          }
          gp_Trsf trsf;
          if ( isInside || face.IsNull() )
          {
            // compute node displacement of end links in their local coord systems
            {
              TChainLink& ln0 = chain.front(), ln1 = *(++chain.begin());
              trsf.SetTransformation( gp_Ax3( gp::Origin(), ln0.Normal(),
                                              gp_Vec( ln0->MiddlePnt(), ln1->MiddlePnt() )));
              move0.Transform(trsf);
            }
            {
              TChainLink& ln0 = *(++chain.rbegin()), ln1 = chain.back();
              trsf.SetTransformation( gp_Ax3( gp::Origin(), ln1.Normal(),
                                              gp_Vec( ln0->MiddlePnt(), ln1->MiddlePnt() )));
              move1.Transform(trsf);
            }
          }
          // compute displacement of medium nodes
          link2 = chain.begin();
          link0 = link2++;
          link1 = link2++;
          for ( int i = 0; link2 != chain.end(); ++link0, ++link1, ++link2, ++i )
          {
            double r = linkPos[i] / chainLen;
            // displacement in local coord system
            gp_Vec move = (1. - r) * move0 + r * move1;
            if ( isInside || face.IsNull()) {
              // transform to global
              gp_Vec x01( (*link0)->MiddlePnt(), (*link1)->MiddlePnt() );
              gp_Vec x12( (*link1)->MiddlePnt(), (*link2)->MiddlePnt() );
              try {
                gp_Vec x = x01.Normalized() + x12.Normalized();
                trsf.SetTransformation( gp_Ax3( gp::Origin(), link1->Normal(), x), gp_Ax3() );
              } catch ( Standard_Failure &) {
                trsf.Invert();
              }
              move.Transform(trsf);
              (*link1)->Move( move, /*sum=*/false, /*is2dFixed=*/false );
            }
            else {
              // compute 3D displacement by 2D one
              Handle(Geom_Surface) s = BRep_Tool::Surface(face,loc);
              gp_XY oldUV   = faceHlp.GetNodeUV( face, (*link1)->_mediumNode, 0, &checkUV );
              gp_XY newUV   = ApplyIn2D( s, oldUV, gp_XY( move.X(),move.Y()), gp_XY_Added );
              gp_Pnt newPnt = s->Value( newUV.X(), newUV.Y());
              move = gp_Vec( XYZ((*link1)->_mediumNode), newPnt.Transformed(loc) );
              if ( SMDS_FacePosition* nPos =
                   dynamic_cast< SMDS_FacePosition* >((*link1)->_mediumNode->GetPosition()))
                nPos->SetParameters( newUV.X(), newUV.Y() );
#ifdef _DEBUG_
              if ( (XYZ((*link1)->node1()) - XYZ((*link1)->node2())).SquareModulus() <
                   move.SquareMagnitude())
              {
                gp_XY uv0 = faceHlp.GetNodeUV( face, (*link0)->_mediumNode, 0, &checkUV );
                gp_XY uv2 = faceHlp.GetNodeUV( face, (*link2)->_mediumNode, 0, &checkUV );
                MSG( "TOO LONG MOVE \t" <<
                     "uv0: "<<uv0.X()<<", "<<uv0.Y()<<" \t" <<
                     "uv2: "<<uv2.X()<<", "<<uv2.Y()<<" \t" <<
                     "uvOld: "<<oldUV.X()<<", "<<oldUV.Y()<<" \t" <<
                     "newUV: "<<newUV.X()<<", "<<newUV.Y()<<" \t");
              }
#endif
              (*link1)->Move( move, /*sum=*/false, /*is2dFixed=*/true );
            }
            MSG( "Move " << (*link1)->_mediumNode->GetID() << " following "
                 << chain.front()->_mediumNode->GetID() <<"-"
                 << chain.back ()->_mediumNode->GetID() <<
                 " by " << move.Magnitude());
          }
        } // loop on chains of links
      } // loop on 2 directions of propagation from quadrangle
    } // loop on faces
  } // fix faces and/or volumes

  // 4. Move nodes
  // -------------

  TIDSortedElemSet biQuadQuas, biQuadTris, triQuadHexa;
  const SMDS_MeshElement *biQuadQua, *triQuadHex;
  const bool toFixCentralNodes = ( myMesh->NbBiQuadQuadrangles() +
                                   myMesh->NbBiQuadTriangles() +
                                   myMesh->NbTriQuadraticHexas() );
  double distXYZ[4];
  faceHlp.ToFixNodeParameters( true );

  for ( pLink = links.begin(); pLink != links.end(); ++pLink ) {
    if ( pLink->IsMoved() )
    {
      gp_Pnt p = pLink->MiddlePnt() + pLink->Move();

      // put on surface nodes on FACE but moved in 3D (23050)
      if ( !pLink->IsFixedOnSurface() )
      {
        faceHlp.SetSubShape( pLink->_mediumNode->getshapeId() );
        if ( faceHlp.GetSubShape().ShapeType() == TopAbs_FACE )
        {
          const_cast<SMDS_MeshNode*>( pLink->_mediumNode )->setXYZ( p.X(), p.Y(), p.Z());
          p.Coord( distXYZ[1], distXYZ[2], distXYZ[3] );
          gp_XY uv( Precision::Infinite(), 0 );
          if ( faceHlp.CheckNodeUV( TopoDS::Face( faceHlp.GetSubShape() ), pLink->_mediumNode,
                                    uv, /*tol=*/pLink->Move().Modulus(), /*force=*/true, distXYZ ))
            p.SetCoord( distXYZ[1], distXYZ[2], distXYZ[3] );
        }
      }
      GetMeshDS()->MoveNode( pLink->_mediumNode, p.X(), p.Y(), p.Z());

      // collect bi-quadratic elements
      if ( toFixCentralNodes )
      {
        biQuadQua = triQuadHex = 0;
        SMDS_ElemIteratorPtr eIt = pLink->_mediumNode->GetInverseElementIterator();
        while ( eIt->more() )
        {
          const SMDS_MeshElement* e = eIt->next();
          switch( e->GetEntityType() ) {
          case SMDSEntity_BiQuad_Quadrangle: biQuadQuas.insert( e ); break;
          case SMDSEntity_BiQuad_Triangle:   biQuadTris.insert( e ); break;
          case SMDSEntity_TriQuad_Hexa:      triQuadHexa.insert( e ); break;
          default:;
          }
        }
      }
    }
  }
  // Fix positions of central nodes of bi-tri-quadratic elements

  // treat bi-quad quadrangles
  {
    vector< const SMDS_MeshNode* > nodes( 9 );
    gp_XY uv[ 9 ];
    TIDSortedElemSet::iterator quadIt = biQuadQuas.begin();
    for ( ; quadIt != biQuadQuas.end(); ++quadIt )
    {
      const SMDS_MeshElement* quad = *quadIt;
      // nodes
      nodes.clear();
      nodes.assign( quad->begin_nodes(), quad->end_nodes() );
      // FACE
      TopoDS_Shape S = GetSubShapeByNode( nodes.back(), GetMeshDS() );
      if ( S.IsNull() || S.ShapeType() != TopAbs_FACE ) continue;
      const TopoDS_Face& F = TopoDS::Face( S );
      Handle( Geom_Surface ) surf = BRep_Tool::Surface( F, loc );
      const double tol = BRep_Tool::Tolerance( F );
      // UV
      for ( int i = 0; i < 8; ++i )
      {
        uv[ i ] = GetNodeUV( F, nodes[i], nodes[8], &checkUV );
        // as this method is used after mesh generation, UV of nodes is not
        // updated according to bending links, so we update 
        if ( i > 3 && nodes[i]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE )
          CheckNodeUV( F, nodes[i], uv[ i ], 2*tol, /*force=*/true );
      }
      AdjustByPeriod( F, uv, 8 ); // put uv[] within a period (IPAL52698)
      // move the central node
      gp_XY uvCent = calcTFI (0.5, 0.5, uv[0],uv[1],uv[2],uv[3],uv[4],uv[5],uv[6],uv[7] );
      gp_Pnt p = surf->Value( uvCent.X(), uvCent.Y() ).Transformed( loc );
      GetMeshDS()->MoveNode( nodes[8], p.X(), p.Y(), p.Z());
    }
  }

  // treat bi-quad triangles
  {
    vector< const SMDS_MeshNode* > nodes;
    gp_XY uv[ 6 ];
    TIDSortedElemSet::iterator triIt = biQuadTris.begin();
    for ( ; triIt != biQuadTris.end(); ++triIt )
    {
      const SMDS_MeshElement* tria = *triIt;
      // FACE
      const TopoDS_Shape& S = GetMeshDS()->IndexToShape( tria->getshapeId() );
      if ( S.IsNull() || S.ShapeType() != TopAbs_FACE ) continue;
      const TopoDS_Face& F = TopoDS::Face( S );
      Handle( Geom_Surface ) surf = BRep_Tool::Surface( F, loc );
      const double tol = BRep_Tool::Tolerance( F );

      // nodes
      nodes.assign( tria->begin_nodes(), tria->end_nodes() );
      // UV
      bool uvOK = true, badTria = false;
      for ( int i = 0; i < 6; ++i )
      {
        uv[ i ] = GetNodeUV( F, nodes[i], nodes[(i+1)%3], &uvOK );
        // as this method is used after mesh generation, UV of nodes is not
        // updated according to bending links, so we update 
        if ( nodes[i]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_FACE )
          CheckNodeUV( F, nodes[i], uv[ i ], 2*tol, /*force=*/true );
      }

      // move the central node
      gp_Pnt p;
      if ( !uvOK || badTria )
      {
        p = ( SMESH_TNodeXYZ( nodes[3] ) +
              SMESH_TNodeXYZ( nodes[4] ) +
              SMESH_TNodeXYZ( nodes[5] )) / 3;
      }
      else
      {
        AdjustByPeriod( F, uv, 6 ); // put uv[] within a period (IPAL52698)
        gp_XY uvCent = GetCenterUV( uv[0], uv[1], uv[2], uv[3], uv[4], uv[5], &badTria );
        p = surf->Value( uvCent.X(), uvCent.Y() ).Transformed( loc );
      }
      GetMeshDS()->MoveNode( tria->GetNode(6), p.X(), p.Y(), p.Z() );
    }
  }

  // treat tri-quadratic hexahedra
  {
    SMDS_VolumeTool volExp;
    TIDSortedElemSet::iterator hexIt = triQuadHexa.begin();
    for ( ; hexIt != triQuadHexa.end(); ++hexIt )
    {
      volExp.Set( *hexIt, /*ignoreCentralNodes=*/false );

      // fix nodes central in sides
      for ( int iQuad = 0; iQuad < volExp.NbFaces(); ++iQuad )
      {
        const SMDS_MeshNode** quadNodes = volExp.GetFaceNodes( iQuad );
        if ( quadNodes[8]->GetPosition()->GetTypeOfPosition() == SMDS_TOP_3DSPACE )
        {
          gp_XYZ p = calcTFI( 0.5, 0.5,
                              SMESH_TNodeXYZ( quadNodes[0] ), SMESH_TNodeXYZ( quadNodes[2] ),
                              SMESH_TNodeXYZ( quadNodes[4] ), SMESH_TNodeXYZ( quadNodes[6] ),
                              SMESH_TNodeXYZ( quadNodes[1] ), SMESH_TNodeXYZ( quadNodes[3] ),
                              SMESH_TNodeXYZ( quadNodes[5] ), SMESH_TNodeXYZ( quadNodes[7] ));
          GetMeshDS()->MoveNode( quadNodes[8], p.X(), p.Y(), p.Z());
        }
      }

      // fix the volume central node
      vector<gp_XYZ> pointsOnShapes( SMESH_Block::ID_Shell );
      const SMDS_MeshNode** hexNodes = volExp.GetNodes();

      pointsOnShapes[ SMESH_Block::ID_V000 ] = SMESH_TNodeXYZ( hexNodes[ 0 ] );
      pointsOnShapes[ SMESH_Block::ID_V100 ] = SMESH_TNodeXYZ( hexNodes[ 3 ] );
      pointsOnShapes[ SMESH_Block::ID_V010 ] = SMESH_TNodeXYZ( hexNodes[ 1 ] );
      pointsOnShapes[ SMESH_Block::ID_V110 ] = SMESH_TNodeXYZ( hexNodes[ 2 ] );
      pointsOnShapes[ SMESH_Block::ID_V001 ] = SMESH_TNodeXYZ( hexNodes[ 4 ] );
      pointsOnShapes[ SMESH_Block::ID_V101 ] = SMESH_TNodeXYZ( hexNodes[ 7 ] );
      pointsOnShapes[ SMESH_Block::ID_V011 ] = SMESH_TNodeXYZ( hexNodes[ 5 ] );
      pointsOnShapes[ SMESH_Block::ID_V111 ] = SMESH_TNodeXYZ( hexNodes[ 6 ] );

      pointsOnShapes[ SMESH_Block::ID_Ex00 ] = SMESH_TNodeXYZ( hexNodes[ 11 ] );
      pointsOnShapes[ SMESH_Block::ID_Ex10 ] = SMESH_TNodeXYZ( hexNodes[  9 ] );
      pointsOnShapes[ SMESH_Block::ID_E0y0 ] = SMESH_TNodeXYZ( hexNodes[  8 ] );
      pointsOnShapes[ SMESH_Block::ID_E1y0 ] = SMESH_TNodeXYZ( hexNodes[ 10 ] );
      pointsOnShapes[ SMESH_Block::ID_Ex01 ] = SMESH_TNodeXYZ( hexNodes[ 15 ] );
      pointsOnShapes[ SMESH_Block::ID_Ex11 ] = SMESH_TNodeXYZ( hexNodes[ 13 ] );
      pointsOnShapes[ SMESH_Block::ID_E0y1 ] = SMESH_TNodeXYZ( hexNodes[ 12 ] );
      pointsOnShapes[ SMESH_Block::ID_E1y1 ] = SMESH_TNodeXYZ( hexNodes[ 14 ] );
      pointsOnShapes[ SMESH_Block::ID_E00z ] = SMESH_TNodeXYZ( hexNodes[ 16 ] );    
      pointsOnShapes[ SMESH_Block::ID_E10z ] = SMESH_TNodeXYZ( hexNodes[ 19 ] );    
      pointsOnShapes[ SMESH_Block::ID_E01z ] = SMESH_TNodeXYZ( hexNodes[ 17 ] );    
      pointsOnShapes[ SMESH_Block::ID_E11z ] = SMESH_TNodeXYZ( hexNodes[ 18 ] );

      pointsOnShapes[ SMESH_Block::ID_Fxy0 ] = SMESH_TNodeXYZ( hexNodes[ 20 ] );
      pointsOnShapes[ SMESH_Block::ID_Fxy1 ] = SMESH_TNodeXYZ( hexNodes[ 25 ] );
      pointsOnShapes[ SMESH_Block::ID_Fx0z ] = SMESH_TNodeXYZ( hexNodes[ 21 ] );   
      pointsOnShapes[ SMESH_Block::ID_Fx1z ] = SMESH_TNodeXYZ( hexNodes[ 23 ] );   
      pointsOnShapes[ SMESH_Block::ID_F0yz ] = SMESH_TNodeXYZ( hexNodes[ 24 ] );    
      pointsOnShapes[ SMESH_Block::ID_F1yz ] = SMESH_TNodeXYZ( hexNodes[ 22 ] );

      gp_XYZ nCenterParams(0.5, 0.5, 0.5), nCenterCoords;
      SMESH_Block::ShellPoint( nCenterParams, pointsOnShapes, nCenterCoords );
      GetMeshDS()->MoveNode( hexNodes[26],
                             nCenterCoords.X(), nCenterCoords.Y(), nCenterCoords.Z());
    }
  }
}
