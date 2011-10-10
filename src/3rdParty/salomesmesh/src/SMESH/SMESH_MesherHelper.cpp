//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
// File:      SMESH_MesherHelper.cxx
// Created:   15.02.06 15:22:41
// Author:    Sergey KUUL
//
#include "SMESH_MesherHelper.hxx"

#include "SMDS_FacePosition.hxx" 
#include "SMDS_EdgePosition.hxx"
#include "SMDS_VolumeTool.hxx"
#include "SMESH_subMesh.hxx"

#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Tool.hxx>
#include <Geom2d_Curve.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Geom_Curve.hxx>
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

#define RETURN_BAD_RESULT(msg) { MESSAGE(msg); return false; }

namespace {

  gp_XYZ XYZ(const SMDS_MeshNode* n) { return gp_XYZ(n->X(), n->Y(), n->Z()); }

}

//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

SMESH_MesherHelper::SMESH_MesherHelper(SMESH_Mesh& theMesh)
  : myMesh(&theMesh), myShapeID(0), myCreateQuadratic(false)
{
  mySetElemOnShape = ( ! myMesh->HasShapeToMesh() );
}

//=======================================================================
//function : CheckShape
//purpose  : 
//=======================================================================

bool SMESH_MesherHelper::IsQuadraticSubMesh(const TopoDS_Shape& aSh)
{
  SMESHDS_Mesh* meshDS = GetMeshDS();
  // we can create quadratic elements only if all elements
  // created on subshapes of given shape are quadratic
  // also we have to fill myTLinkNodeMap
  myCreateQuadratic = true;
  mySeamShapeIds.clear();
  myDegenShapeIds.clear();
  TopAbs_ShapeEnum subType( aSh.ShapeType()==TopAbs_FACE ? TopAbs_EDGE : TopAbs_FACE );
  SMDSAbs_ElementType elemType( subType==TopAbs_FACE ? SMDSAbs_Face : SMDSAbs_Edge );

  int nbOldLinks = myTLinkNodeMap.size();

  TopExp_Explorer exp( aSh, subType );
  for (; exp.More() && myCreateQuadratic; exp.Next()) {
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
            switch ( e->NbNodes() ) {
            case 3:
              AddTLinkNode(e->GetNode(0),e->GetNode(1),e->GetNode(2)); break;
            case 6:
              AddTLinkNode(e->GetNode(0),e->GetNode(1),e->GetNode(3));
              AddTLinkNode(e->GetNode(1),e->GetNode(2),e->GetNode(4));
              AddTLinkNode(e->GetNode(2),e->GetNode(0),e->GetNode(5)); break;
            case 8:
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

  if ( nbOldLinks == myTLinkNodeMap.size() )
    myCreateQuadratic = false;

  if(!myCreateQuadratic) {
    myTLinkNodeMap.clear();
  }
  SetSubShape( aSh );

  return myCreateQuadratic;
}

//================================================================================
/*!
 * \brief Set geomerty to make elements on
  * \param aSh - geomertic shape
 */
//================================================================================

void SMESH_MesherHelper::SetSubShape(const int aShID)
{
  if ( aShID == myShapeID )
    return;
  if ( aShID > 1 )
    SetSubShape( GetMeshDS()->IndexToShape( aShID ));
  else
    SetSubShape( TopoDS_Shape() );
}

//================================================================================
/*!
 * \brief Set geomerty to make elements on
  * \param aSh - geomertic shape
 */
//================================================================================

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

  // treatment of periodic faces
  for ( TopExp_Explorer eF( aSh, TopAbs_FACE ); eF.More(); eF.Next() )
  {
    const TopoDS_Face& face = TopoDS::Face( eF.Current() );
    BRepAdaptor_Surface surface( face );
    if ( surface.IsUPeriodic() || surface.IsVPeriodic() )
    {
      for (TopExp_Explorer exp( face, TopAbs_EDGE ); exp.More(); exp.Next())
      {
        // look for a seam edge
        const TopoDS_Edge& edge = TopoDS::Edge( exp.Current() );
        if ( BRep_Tool::IsClosed( edge, face )) {
          // initialize myPar1, myPar2 and myParIndex
          if ( mySeamShapeIds.empty() ) {
            gp_Pnt2d uv1, uv2;
            BRep_Tool::UVPoints( edge, face, uv1, uv2 );
            if ( Abs( uv1.Coord(1) - uv2.Coord(1) ) < Abs( uv1.Coord(2) - uv2.Coord(2) ))
            {
              myParIndex = 1; // U periodic
              myPar1 = surface.FirstUParameter();
              myPar2 = surface.LastUParameter();
            }
            else {
              myParIndex = 2;  // V periodic
              myPar1 = surface.FirstVParameter();
              myPar2 = surface.LastVParameter();
            }
          }
          // store seam shape indices, negative if shape encounters twice
          int edgeID = meshDS->ShapeToIndex( edge );
          mySeamShapeIds.insert( IsSeamShape( edgeID ) ? -edgeID : edgeID );
          for ( TopExp_Explorer v( edge, TopAbs_VERTEX ); v.More(); v.Next() ) {
            int vertexID = meshDS->ShapeToIndex( v.Current() );
            mySeamShapeIds.insert( IsSeamShape( vertexID ) ? -vertexID : vertexID );
          }
        }

        // look for a degenerated edge
        if ( BRep_Tool::Degenerated( edge )) {
          myDegenShapeIds.insert( meshDS->ShapeToIndex( edge ));
          for ( TopExp_Explorer v( edge, TopAbs_VERTEX ); v.More(); v.Next() )
            myDegenShapeIds.insert( meshDS->ShapeToIndex( v.Current() ));
        }
      }
    }
  }
}

//================================================================================
  /*!
   * \brief Check if inFaceNode argument is necessary for call GetNodeUV(F,..)
    * \param F - the face
    * \retval bool - return true if the face is periodic
   */
//================================================================================

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
/*!
 * \brief Return support shape of a node
 * \param node - the node
 * \param meshDS - mesh DS
 * \retval TopoDS_Shape - found support shape
 */
//=======================================================================

TopoDS_Shape SMESH_MesherHelper::GetSubShapeByNode(const SMDS_MeshNode* node,
                                                   SMESHDS_Mesh*        meshDS)
{
  int shapeID = node->GetPosition()->GetShapeId();
  if ( 0 < shapeID && shapeID <= meshDS->MaxShapeIndex() )
    return meshDS->IndexToShape( shapeID );
  else
    return TopoDS_Shape();
}


//=======================================================================
//function : AddTLinkNode
//purpose  : 
//=======================================================================
/*!
 * Auxilary function for filling myTLinkNodeMap
 */
void SMESH_MesherHelper::AddTLinkNode(const SMDS_MeshNode* n1,
                                      const SMDS_MeshNode* n2,
                                      const SMDS_MeshNode* n12)
{
  // add new record to map
  SMESH_TLink link( n1, n2 );
  myTLinkNodeMap.insert( make_pair(link,n12));
}

//=======================================================================
/*!
 * \brief Select UV on either of 2 pcurves of a seam edge, closest to the given UV
 * \param uv1 - UV on the seam
 * \param uv2 - UV within a face
 * \retval gp_Pnt2d - selected UV
 */
//=======================================================================

gp_Pnt2d SMESH_MesherHelper::GetUVOnSeam( const gp_Pnt2d& uv1, const gp_Pnt2d& uv2 ) const
{
  double p1 = uv1.Coord( myParIndex );
  double p2 = uv2.Coord( myParIndex );
  double p3 = ( Abs( p1 - myPar1 ) < Abs( p1 - myPar2 )) ? myPar2 : myPar1;
  if ( Abs( p2 - p1 ) > Abs( p2 - p3 ))
    p1 = p3;
  gp_Pnt2d result = uv1;
  result.SetCoord( myParIndex, p1 );
  return result;
}

//=======================================================================
/*!
 * \brief Return node UV on face
 * \param F - the face
 * \param n - the node
 * \param n2 - a node of element being created located inside a face
 * \param check - optional flag returing false if found UV are invalid
 * \retval gp_XY - resulting UV
 */
//=======================================================================

gp_XY SMESH_MesherHelper::GetNodeUV(const TopoDS_Face&   F,
                                    const SMDS_MeshNode* n,
                                    const SMDS_MeshNode* n2,
                                    bool*                check) const
{
  gp_Pnt2d uv( 1e100, 1e100 );
  const SMDS_PositionPtr Pos = n->GetPosition();
  bool uvOK = false;
  if(Pos->GetTypeOfPosition()==SMDS_TOP_FACE)
  {
    // node has position on face
    const SMDS_FacePosition* fpos =
      static_cast<const SMDS_FacePosition*>(n->GetPosition().get());
    uv.SetCoord(fpos->GetUParameter(),fpos->GetVParameter());
    uvOK = CheckNodeUV( F, n, uv.ChangeCoord(), BRep_Tool::Tolerance( F ));
  }
  else if(Pos->GetTypeOfPosition()==SMDS_TOP_EDGE)
  {
    // node has position on edge => it is needed to find
    // corresponding edge from face, get pcurve for this
    // edge and retrieve value from this pcurve
    const SMDS_EdgePosition* epos =
      static_cast<const SMDS_EdgePosition*>(n->GetPosition().get());
    int edgeID = Pos->GetShapeId();
    TopoDS_Edge E = TopoDS::Edge(GetMeshDS()->IndexToShape(edgeID));
    double f, l, u = epos->GetUParameter();
    Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface(E, F, f, l);
    if ( f < u && u < l )
      uv = C2d->Value( u );
    else
      uv.SetCoord(0.,0.);
    uvOK = CheckNodeUV( F, n, uv.ChangeCoord(), BRep_Tool::Tolerance( E ));

    // for a node on a seam edge select one of UVs on 2 pcurves
    if ( n2 && IsSeamShape( edgeID ) )
    {
      uv = GetUVOnSeam( uv, GetNodeUV( F, n2, 0 ));
    }
    else
    { // adjust uv to period
      TopLoc_Location loc;
      Handle(Geom_Surface) S = BRep_Tool::Surface(F,loc);
      Standard_Boolean isUPeriodic = S->IsUPeriodic();
      Standard_Boolean isVPeriodic = S->IsVPeriodic();
      if ( isUPeriodic || isVPeriodic ) {
        Standard_Real UF,UL,VF,VL;
        S->Bounds(UF,UL,VF,VL);
        if(isUPeriodic)
          uv.SetX( uv.X() + ShapeAnalysis::AdjustToPeriod(uv.X(),UF,UL));
        if(isVPeriodic)
          uv.SetY( uv.Y() + ShapeAnalysis::AdjustToPeriod(uv.Y(),VF,VL));
      }
    }
  }
  else if(Pos->GetTypeOfPosition()==SMDS_TOP_VERTEX)
  {
    if ( int vertexID = n->GetPosition()->GetShapeId() ) {
      const TopoDS_Vertex& V = TopoDS::Vertex(GetMeshDS()->IndexToShape(vertexID));
      try {
        uv = BRep_Tool::Parameters( V, F );
        uvOK = true;
      }
      catch (Standard_Failure& exc) {
      }
      if ( !uvOK ) {
        for ( TopExp_Explorer vert(F,TopAbs_VERTEX); !uvOK && vert.More(); vert.Next() )
          uvOK = ( V == vert.Current() );
        if ( !uvOK ) {
#ifdef _DEBUG_
          MESSAGE ( "SMESH_MesherHelper::GetNodeUV(); Vertex " << vertexID
               << " not in face " << GetMeshDS()->ShapeToIndex( F ) );
#endif
          // get UV of a vertex closest to the node
          double dist = 1e100;
          gp_Pnt pn = XYZ( n );
          for ( TopExp_Explorer vert(F,TopAbs_VERTEX); !uvOK && vert.More(); vert.Next() ) {
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
        else {
          TopTools_ListIteratorOfListOfShape it( myMesh->GetAncestors( V ));
          for ( ; it.More(); it.Next() ) {
            if ( it.Value().ShapeType() == TopAbs_EDGE ) {
              const TopoDS_Edge & edge = TopoDS::Edge( it.Value() );
              double f,l;
              Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface(edge, F, f, l);
              if ( !C2d.IsNull() ) {
                double u = ( V == TopExp::FirstVertex( edge ) ) ?  f : l;
                uv = C2d->Value( u );
                break;
              }
            }
          }
        }
      }
      if ( n2 && IsSeamShape( vertexID ) )
        uv = GetUVOnSeam( uv, GetNodeUV( F, n2, 0 ));
    }
  }

  if ( check )
    *check = uvOK;

  return uv.XY();
}

//=======================================================================
/*!
 * \brief Check and fix node UV on a face
 *  \retval bool - false if UV is bad and could not be fixed
 */
//=======================================================================

bool SMESH_MesherHelper::CheckNodeUV(const TopoDS_Face&   F,
                                     const SMDS_MeshNode* n,
                                     gp_XY&               uv,
                                     const double         tol) const
{
  if ( !myOkNodePosShapes.count( n->GetPosition()->GetShapeId() ))
  {
    // check that uv is correct
    TopLoc_Location loc;
    Handle(Geom_Surface) surface = BRep_Tool::Surface( F,loc );
    gp_Pnt nodePnt = XYZ( n );
    if ( !loc.IsIdentity() ) nodePnt.Transform( loc.Transformation().Inverted() );
    if ( nodePnt.Distance( surface->Value( uv.X(), uv.Y() )) > tol )
    {
      // uv incorrect, project the node to surface
      GeomAPI_ProjectPointOnSurf projector( nodePnt, surface, tol );
      if ( !projector.IsDone() || projector.NbPoints() < 1 )
      {
        MESSAGE( "SMESH_MesherHelper::CheckNodeUV() failed to project" );
        return false;
      }
      Quantity_Parameter U,V;
      projector.LowerDistanceParameters(U,V);
      if ( nodePnt.Distance( surface->Value( U, V )) > tol )
      {
        MESSAGE( "SMESH_MesherHelper::CheckNodeUV(), invalid projection" );
        return false;
      }
      uv.SetCoord( U,V );
    }
    else if ( uv.Modulus() > numeric_limits<double>::min() )
    {
      ((SMESH_MesherHelper*) this)->myOkNodePosShapes.insert( n->GetPosition()->GetShapeId() );
    }
  }
  return true;
}

//=======================================================================
/*!
 * \brief Return middle UV taking in account surface period
 */
//=======================================================================

gp_XY SMESH_MesherHelper::GetMiddleUV(const Handle(Geom_Surface)& surface,
                                      const gp_XY&                p1,
                                      const gp_XY&                p2)
{
  if ( surface.IsNull() )
    return 0.5 * ( p1 + p2 );
  //checking if surface is periodic
  Standard_Real UF,UL,VF,VL;
  surface->Bounds(UF,UL,VF,VL);

  Standard_Real u,v;
  Standard_Boolean isUPeriodic = surface->IsUPeriodic();
  if(isUPeriodic) {
    Standard_Real UPeriod = surface->UPeriod();
    Standard_Real p2x = p2.X()+ShapeAnalysis::AdjustByPeriod(p2.X(),p1.X(),UPeriod);
    Standard_Real pmid = (p1.X()+p2x)/2.;
    u = pmid+ShapeAnalysis::AdjustToPeriod(pmid,UF,UL);
  }
  else {
    u= (p1.X()+p2.X())/2.;
  }
  Standard_Boolean isVPeriodic = surface->IsVPeriodic();
  if(isVPeriodic) {
    Standard_Real VPeriod = surface->VPeriod();
    Standard_Real p2y = p2.Y()+ShapeAnalysis::AdjustByPeriod(p2.Y(),p1.Y(),VPeriod);
    Standard_Real pmid = (p1.Y()+p2y)/2.;
    v = pmid+ShapeAnalysis::AdjustToPeriod(pmid,VF,VL);
  }
  else {
    v = (p1.Y()+p2.Y())/2.;
  }
  return gp_XY( u,v );
}

//=======================================================================
/*!
 * \brief Return node U on edge
 * \param E - the Edge
 * \param n - the node
 * \retval double - resulting U
 * 
 * Auxilary function called form GetMediumNode()
 */
//=======================================================================

double SMESH_MesherHelper::GetNodeU(const TopoDS_Edge&   E,
                                    const SMDS_MeshNode* n,
                                    bool*                check)
{
  double param = 0;
  const SMDS_PositionPtr Pos = n->GetPosition();
  if(Pos->GetTypeOfPosition()==SMDS_TOP_EDGE) {
    const SMDS_EdgePosition* epos =
      static_cast<const SMDS_EdgePosition*>(n->GetPosition().get());
    param =  epos->GetUParameter();
  }
  else if(Pos->GetTypeOfPosition()==SMDS_TOP_VERTEX) {
    SMESHDS_Mesh * meshDS = GetMeshDS();
    int vertexID = n->GetPosition()->GetShapeId();
    const TopoDS_Vertex& V = TopoDS::Vertex(meshDS->IndexToShape(vertexID));
    param =  BRep_Tool::Parameter( V, E );
  }
  return param;
}

//================================================================================
/*!
 * \brief Return existing or create new medium nodes between given ones
 *  \param force3d - if true, new node is the middle of n1 and n2,
 *                   else is located on geom face or geom edge
 */
//================================================================================

const SMDS_MeshNode* SMESH_MesherHelper::GetMediumNode(const SMDS_MeshNode* n1,
                                                       const SMDS_MeshNode* n2,
                                                       bool force3d)
{
  SMESH_TLink link(n1,n2);
  ItTLinkNode itLN = myTLinkNodeMap.find( link );
  if ( itLN != myTLinkNodeMap.end() ) {
    return (*itLN).second;
  }
  // create medium node
  SMDS_MeshNode* n12;
  SMESHDS_Mesh* meshDS = GetMeshDS();
  int faceID = -1, edgeID = -1;
  const SMDS_PositionPtr Pos1 = n1->GetPosition();
  const SMDS_PositionPtr Pos2 = n2->GetPosition();

  if( myShape.IsNull() )
  {
    if( Pos1->GetTypeOfPosition()==SMDS_TOP_FACE ) {
      faceID = Pos1->GetShapeId();
    }
    else if( Pos2->GetTypeOfPosition()==SMDS_TOP_FACE ) {
      faceID = Pos2->GetShapeId();
    }

    if( Pos1->GetTypeOfPosition()==SMDS_TOP_EDGE ) {
      edgeID = Pos1->GetShapeId();
    }
    if( Pos2->GetTypeOfPosition()==SMDS_TOP_EDGE ) {
      edgeID = Pos2->GetShapeId();
    }
  }
  if(!force3d)
  {
    // we try to create medium node using UV parameters of
    // nodes, else - medium between corresponding 3d points

    TopAbs_ShapeEnum shapeType = myShape.IsNull() ? TopAbs_SHAPE : myShape.ShapeType();
    if(faceID>0 || shapeType == TopAbs_FACE) {
      // obtaining a face and 2d points for nodes
      TopoDS_Face F;
      if( myShape.IsNull() )
        F = TopoDS::Face(meshDS->IndexToShape(faceID));
      else {
        F = TopoDS::Face(myShape);
        faceID = myShapeID;
      }
      bool uvOK1, uvOK2;
      gp_XY p1 = GetNodeUV(F,n1,n2, &uvOK1);
      gp_XY p2 = GetNodeUV(F,n2,n1, &uvOK2);

      if ( uvOK1 && uvOK2 )
      {
        if ( IsDegenShape( Pos1->GetShapeId() ))
          p1.SetCoord( myParIndex, p2.Coord( myParIndex ));
        else if ( IsDegenShape( Pos2->GetShapeId() ))
          p2.SetCoord( myParIndex, p1.Coord( myParIndex ));

        TopLoc_Location loc;
        Handle(Geom_Surface) S = BRep_Tool::Surface(F,loc);
        gp_XY uv = GetMiddleUV( S, p1, p2 );
        gp_Pnt P = S->Value( uv.X(), uv.Y() ).Transformed(loc);
        n12 = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(n12, faceID, uv.X(), uv.Y());
        myTLinkNodeMap.insert(make_pair(link,n12));
        return n12;
      }
    }
    if (edgeID>0 || shapeType == TopAbs_EDGE) {

      TopoDS_Edge E;
      if( myShape.IsNull() )
        E = TopoDS::Edge(meshDS->IndexToShape(edgeID));
      else {
        E = TopoDS::Edge(myShape);
        edgeID = myShapeID;
      }

      double p1 = GetNodeU(E,n1);
      double p2 = GetNodeU(E,n2);

      double f,l;
      Handle(Geom_Curve) C = BRep_Tool::Curve(E, f, l);
      if(!C.IsNull()) {

        Standard_Boolean isPeriodic = C->IsPeriodic();
        double u;
        if(isPeriodic) {
          Standard_Real Period = C->Period();
          Standard_Real p = p2+ShapeAnalysis::AdjustByPeriod(p2,p1,Period);
          Standard_Real pmid = (p1+p)/2.;
          u = pmid+ShapeAnalysis::AdjustToPeriod(pmid,C->FirstParameter(),C->LastParameter());
        }
        else
          u = (p1+p2)/2.;

        gp_Pnt P = C->Value( u );
        n12 = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnEdge(n12, edgeID, u);
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
  if(edgeID>0)
    meshDS->SetNodeOnEdge(n12, edgeID);
  else if(faceID>0)
    meshDS->SetNodeOnFace(n12, faceID);
  else
    meshDS->SetNodeInVolume(n12, myShapeID);
  myTLinkNodeMap.insert( make_pair( link, n12 ));
  return n12;
}

//=======================================================================
/*!
 * Creates a node
 */
//=======================================================================

SMDS_MeshNode* SMESH_MesherHelper::AddNode(double x, double y, double z, int ID)
{
  SMESHDS_Mesh * meshDS = GetMeshDS();
  SMDS_MeshNode* node = 0;
  if ( ID )
    node = meshDS->AddNodeWithID( x, y, z, ID );
  else
    node = meshDS->AddNode( x, y, z );
  if ( mySetElemOnShape && myShapeID > 0 ) {
    switch ( myShape.ShapeType() ) {
    case TopAbs_SOLID:  meshDS->SetNodeInVolume( node, myShapeID); break;
    case TopAbs_SHELL:  meshDS->SetNodeInVolume( node, myShapeID); break;
    case TopAbs_FACE:   meshDS->SetNodeOnFace(   node, myShapeID); break;
    case TopAbs_EDGE:   meshDS->SetNodeOnEdge(   node, myShapeID); break;
    case TopAbs_VERTEX: meshDS->SetNodeOnVertex( node, myShapeID); break;
    default: ;
    }
  }
  return node;
}

//=======================================================================
/*!
 * Creates quadratic or linear edge
 */
//=======================================================================

SMDS_MeshEdge* SMESH_MesherHelper::AddEdge(const SMDS_MeshNode* n1,
                                                const SMDS_MeshNode* n2,
                                                const int id,
                                                const bool force3d)
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
/*!
 * Creates quadratic or linear triangle
 */
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
    const SMDS_MeshNode* n12 = GetMediumNode(n1,n2,force3d);
    const SMDS_MeshNode* n23 = GetMediumNode(n2,n3,force3d);
    const SMDS_MeshNode* n31 = GetMediumNode(n3,n1,force3d);

    if(id)
      elem = meshDS->AddFaceWithID(n1, n2, n3, n12, n23, n31, id);
    else
      elem = meshDS->AddFace(n1, n2, n3, n12, n23, n31);
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
/*!
 * Creates quadratic or linear quadrangle
 */
//=======================================================================

SMDS_MeshFace* SMESH_MesherHelper::AddFace(const SMDS_MeshNode* n1,
                                           const SMDS_MeshNode* n2,
                                           const SMDS_MeshNode* n3,
                                           const SMDS_MeshNode* n4,
                                           const int id,
                                           const bool force3d)
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
    const SMDS_MeshNode* n12 = GetMediumNode(n1,n2,force3d);
    const SMDS_MeshNode* n23 = GetMediumNode(n2,n3,force3d);
    const SMDS_MeshNode* n34 = GetMediumNode(n3,n4,force3d);
    const SMDS_MeshNode* n41 = GetMediumNode(n4,n1,force3d);

    if(id)
      elem = meshDS->AddFaceWithID(n1, n2, n3, n4, n12, n23, n34, n41, id);
    else
      elem = meshDS->AddFace(n1, n2, n3, n4, n12, n23, n34, n41);
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
/*!
 * Creates quadratic or linear volume
 */
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
    const SMDS_MeshNode* n12 = GetMediumNode(n1,n2,force3d);
    const SMDS_MeshNode* n23 = GetMediumNode(n2,n3,force3d);
    const SMDS_MeshNode* n31 = GetMediumNode(n3,n1,force3d);

    const SMDS_MeshNode* n45 = GetMediumNode(n4,n5,force3d);
    const SMDS_MeshNode* n56 = GetMediumNode(n5,n6,force3d);
    const SMDS_MeshNode* n64 = GetMediumNode(n6,n4,force3d);

    const SMDS_MeshNode* n14 = GetMediumNode(n1,n4,force3d);
    const SMDS_MeshNode* n25 = GetMediumNode(n2,n5,force3d);
    const SMDS_MeshNode* n36 = GetMediumNode(n3,n6,force3d);

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
/*!
 * Creates quadratic or linear volume
 */
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
    const SMDS_MeshNode* n12 = GetMediumNode(n1,n2,force3d);
    const SMDS_MeshNode* n23 = GetMediumNode(n2,n3,force3d);
    const SMDS_MeshNode* n31 = GetMediumNode(n3,n1,force3d);

    const SMDS_MeshNode* n14 = GetMediumNode(n1,n4,force3d);
    const SMDS_MeshNode* n24 = GetMediumNode(n2,n4,force3d);
    const SMDS_MeshNode* n34 = GetMediumNode(n3,n4,force3d);

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
/*!
 * Creates quadratic or linear pyramid
 */
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
    const SMDS_MeshNode* n12 = GetMediumNode(n1,n2,force3d);
    const SMDS_MeshNode* n23 = GetMediumNode(n2,n3,force3d);
    const SMDS_MeshNode* n34 = GetMediumNode(n3,n4,force3d);
    const SMDS_MeshNode* n41 = GetMediumNode(n4,n1,force3d);

    const SMDS_MeshNode* n15 = GetMediumNode(n1,n5,force3d);
    const SMDS_MeshNode* n25 = GetMediumNode(n2,n5,force3d);
    const SMDS_MeshNode* n35 = GetMediumNode(n3,n5,force3d);
    const SMDS_MeshNode* n45 = GetMediumNode(n4,n5,force3d);

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
/*!
 * Creates quadratic or linear hexahedron
 */
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
    const SMDS_MeshNode* n12 = GetMediumNode(n1,n2,force3d);
    const SMDS_MeshNode* n23 = GetMediumNode(n2,n3,force3d);
    const SMDS_MeshNode* n34 = GetMediumNode(n3,n4,force3d);
    const SMDS_MeshNode* n41 = GetMediumNode(n4,n1,force3d);

    const SMDS_MeshNode* n56 = GetMediumNode(n5,n6,force3d);
    const SMDS_MeshNode* n67 = GetMediumNode(n6,n7,force3d);
    const SMDS_MeshNode* n78 = GetMediumNode(n7,n8,force3d);
    const SMDS_MeshNode* n85 = GetMediumNode(n8,n5,force3d);

    const SMDS_MeshNode* n15 = GetMediumNode(n1,n5,force3d);
    const SMDS_MeshNode* n26 = GetMediumNode(n2,n6,force3d);
    const SMDS_MeshNode* n37 = GetMediumNode(n3,n7,force3d);
    const SMDS_MeshNode* n48 = GetMediumNode(n4,n8,force3d);

    if(id)
      elem = meshDS->AddVolumeWithID(n1, n2, n3, n4, n5, n6, n7, n8,
                                     n12, n23, n34, n41, n56, n67,
                                     n78, n85, n15, n26, n37, n48, id);
    else
      elem = meshDS->AddVolume(n1, n2, n3, n4, n5, n6, n7, n8,
                               n12, n23, n34, n41, n56, n67,
                               n78, n85, n15, n26, n37, n48);
  }
  if ( mySetElemOnShape && myShapeID > 0 )
    meshDS->SetMeshElementOnShape( elem, myShapeID );

  return elem;
}

//=======================================================================
/*!
 * \brief Load nodes bound to face into a map of node columns
 * \param theParam2ColumnMap - map of node columns to fill
 * \param theFace - the face on which nodes are searched for
 * \param theBaseEdge - the edge nodes of which are columns' bases
 * \param theMesh - the mesh containing nodes
 * \retval bool - false if something is wrong
 * 
 * The key of the map is a normalized parameter of each
 * base node on theBaseEdge.
 * This method works in supposition that nodes on the face
 * forms a rectangular grid and elements can be quardrangles or triangles
 */
//=======================================================================

bool SMESH_MesherHelper::LoadNodeColumns(TParam2ColumnMap & theParam2ColumnMap,
                                         const TopoDS_Face& theFace,
                                         const TopoDS_Edge& theBaseEdge,
                                         SMESHDS_Mesh*      theMesh)
{
  // get vertices of theBaseEdge
  TopoDS_Vertex vfb, vlb, vft; // first and last, bottom and top vertices
  TopoDS_Edge eFrw = TopoDS::Edge( theBaseEdge.Oriented( TopAbs_FORWARD ));
  TopExp::Vertices( eFrw, vfb, vlb );

  // find the other edges of theFace and orientation of e1
  TopoDS_Edge e1, e2, eTop;
  bool rev1, CumOri = false;
  TopExp_Explorer exp( theFace, TopAbs_EDGE );
  int nbEdges = 0;
  for ( ; exp.More(); exp.Next() ) {
    if ( ++nbEdges > 4 ) {
      return false; // more than 4 edges in theFace
    }
    TopoDS_Edge e = TopoDS::Edge( exp.Current() );
    if ( theBaseEdge.IsSame( e ))
      continue;
    TopoDS_Vertex vCommon;
    if ( !TopExp::CommonVertex( theBaseEdge, e, vCommon ))
      eTop = e;
    else if ( vCommon.IsSame( vfb )) {
      e1 = e;
      vft = TopExp::LastVertex( e1, CumOri );
      rev1 = vfb.IsSame( vft );
      if ( rev1 )
        vft = TopExp::FirstVertex( e1, CumOri );
    }
    else
      e2 = e;
  }
  if ( nbEdges < 4 ) {
    return false; // less than 4 edges in theFace
  }
  if ( e2.IsNull() && vfb.IsSame( vlb ))
    e2 = e1;

  // submeshes corresponding to shapes
  SMESHDS_SubMesh* smFace = theMesh->MeshElements( theFace );
  SMESHDS_SubMesh* smb = theMesh->MeshElements( theBaseEdge );
  SMESHDS_SubMesh* smt = theMesh->MeshElements( eTop );
  SMESHDS_SubMesh* sm1 = theMesh->MeshElements( e1 );
  SMESHDS_SubMesh* sm2 = theMesh->MeshElements( e2 );
  SMESHDS_SubMesh* smVfb = theMesh->MeshElements( vfb );
  SMESHDS_SubMesh* smVlb = theMesh->MeshElements( vlb );
  SMESHDS_SubMesh* smVft = theMesh->MeshElements( vft );
  if (!smFace || !smb || !smt || !sm1 || !sm2 || !smVfb || !smVlb || !smVft ) {
    RETURN_BAD_RESULT( "NULL submesh " <<smFace<<" "<<smb<<" "<<smt<<" "<<
                       sm1<<" "<<sm2<<" "<<smVfb<<" "<<smVlb<<" "<<smVft);
  }
  if ( smb->NbNodes() != smt->NbNodes() || sm1->NbNodes() != sm2->NbNodes() ) {
    RETURN_BAD_RESULT(" Diff nb of nodes on opposite edges" );
  }
  if (smVfb->NbNodes() != 1 || smVlb->NbNodes() != 1 || smVft->NbNodes() != 1) {
    RETURN_BAD_RESULT("Empty submesh of vertex");
  }
  // define whether mesh is quadratic
  bool isQuadraticMesh = false;
  SMDS_ElemIteratorPtr eIt = smFace->GetElements();
  if ( !eIt->more() ) {
    RETURN_BAD_RESULT("No elements on the face");
  }
  const SMDS_MeshElement* e = eIt->next();
  isQuadraticMesh = e->IsQuadratic();
  
  if ( sm1->NbNodes() * smb->NbNodes() != smFace->NbNodes() ) {
    // check quadratic case
    if ( isQuadraticMesh ) {
      // what if there are quadrangles and triangles mixed?
//       int n1 = sm1->NbNodes()/2;
//       int n2 = smb->NbNodes()/2;
//       int n3 = sm1->NbNodes() - n1;
//       int n4 = smb->NbNodes() - n2;
//       int nf = sm1->NbNodes()*smb->NbNodes() - n3*n4;
//       if( nf != smFace->NbNodes() ) {
//         MESSAGE( "Wrong nb face nodes: " <<
//                 sm1->NbNodes()<<" "<<smb->NbNodes()<<" "<<smFace->NbNodes());
//         return false;
//       }
    }
    else {
      RETURN_BAD_RESULT( "Wrong nb face nodes: " <<
                         sm1->NbNodes()<<" "<<smb->NbNodes()<<" "<<smFace->NbNodes());
    }
  }
  // IJ size
  int vsize = sm1->NbNodes() + 2;
  int hsize = smb->NbNodes() + 2;
  if(isQuadraticMesh) {
    vsize = vsize - sm1->NbNodes()/2 -1;
    hsize = hsize - smb->NbNodes()/2 -1;
  }

  // load nodes from theBaseEdge

  std::set<const SMDS_MeshNode*> loadedNodes;
  const SMDS_MeshNode* nullNode = 0;

  std::vector<const SMDS_MeshNode*> & nVecf = theParam2ColumnMap[ 0.];
  nVecf.resize( vsize, nullNode );
  loadedNodes.insert( nVecf[ 0 ] = smVfb->GetNodes()->next() );

  std::vector<const SMDS_MeshNode*> & nVecl = theParam2ColumnMap[ 1.];
  nVecl.resize( vsize, nullNode );
  loadedNodes.insert( nVecl[ 0 ] = smVlb->GetNodes()->next() );

  double f, l;
  BRep_Tool::Range( eFrw, f, l );
  double range = l - f;
  SMDS_NodeIteratorPtr nIt = smb->GetNodes();
  const SMDS_MeshNode* node;
  while ( nIt->more() ) {
    node = nIt->next();
    if(IsMedium(node, SMDSAbs_Edge))
      continue;
    const SMDS_EdgePosition* pos =
      dynamic_cast<const SMDS_EdgePosition*>( node->GetPosition().get() );
    if ( !pos ) {
      return false;
    }
    double u = ( pos->GetUParameter() - f ) / range;
    std::vector<const SMDS_MeshNode*> & nVec = theParam2ColumnMap[ u ];
    nVec.resize( vsize, nullNode );
    loadedNodes.insert( nVec[ 0 ] = node );
  }
  if ( theParam2ColumnMap.size() != hsize ) {
    RETURN_BAD_RESULT( "Wrong node positions on theBaseEdge" );
  }

  // load nodes from e1

  std::map< double, const SMDS_MeshNode*> sortedNodes; // sort by param on edge
  nIt = sm1->GetNodes();
  while ( nIt->more() ) {
    node = nIt->next();
    if(IsMedium(node))
      continue;
    const SMDS_EdgePosition* pos =
      dynamic_cast<const SMDS_EdgePosition*>( node->GetPosition().get() );
    if ( !pos ) {
      return false;
    }
    sortedNodes.insert( std::make_pair( pos->GetUParameter(), node ));
  }
  loadedNodes.insert( nVecf[ vsize - 1 ] = smVft->GetNodes()->next() );
  std::map< double, const SMDS_MeshNode*>::iterator u_n = sortedNodes.begin();
  int row  = rev1 ? vsize - 1 : 0;
  int dRow = rev1 ? -1 : +1;
  for ( ; u_n != sortedNodes.end(); u_n++ ) {
    row += dRow;
    loadedNodes.insert( nVecf[ row ] = u_n->second );
  }

  // try to load the rest nodes

  // get all faces from theFace
  TIDSortedElemSet allFaces, foundFaces;
  eIt = smFace->GetElements();
  while ( eIt->more() ) {
    const SMDS_MeshElement* e = eIt->next();
    if ( e->GetType() == SMDSAbs_Face )
      allFaces.insert( e );
  }
  // Starting from 2 neighbour nodes on theBaseEdge, look for a face
  // the nodes belong to, and between the nodes of the found face,
  // look for a not loaded node considering this node to be the next
  // in a column of the starting second node. Repeat, starting
  // from nodes next to the previous starting nodes in their columns,
  // and so on while a face can be found. Then go the the next pair
  // of nodes on theBaseEdge.
  TParam2ColumnMap::iterator par_nVec_1 = theParam2ColumnMap.begin();
  TParam2ColumnMap::iterator par_nVec_2 = par_nVec_1;
  // loop on columns
  int col = 0;
  for ( par_nVec_2++; par_nVec_2 != theParam2ColumnMap.end(); par_nVec_1++, par_nVec_2++ ) {
    col++;
    row = 0;
    const SMDS_MeshNode* n1 = par_nVec_1->second[ row ];
    const SMDS_MeshNode* n2 = par_nVec_2->second[ row ];
    const SMDS_MeshElement* face = 0;
    bool lastColOnClosedFace = ( nVecf[ row ] == n2 );
    do {
      // look for a face by 2 nodes
      face = SMESH_MeshEditor::FindFaceInSet( n1, n2, allFaces, foundFaces );
      if ( face ) {
        int nbFaceNodes = face->NbNodes();
        if ( face->IsQuadratic() )
          nbFaceNodes /= 2;
        if ( nbFaceNodes>4 ) {
          RETURN_BAD_RESULT(" Too many nodes in a face: " << nbFaceNodes );
        }
        // look for a not loaded node of the <face>
        bool found = false;
        const SMDS_MeshNode* n3 = 0; // a node defferent from n1 and n2
        for ( int i = 0; i < nbFaceNodes && !found; ++i ) {
          node = face->GetNode( i );
          found = loadedNodes.insert( node ).second;
          if ( !found && node != n1 && node != n2 )
            n3 = node;
        }
        if ( lastColOnClosedFace && row + 1 < vsize ) {
          node = nVecf[ row + 1 ];
          found = ( face->GetNodeIndex( node ) >= 0 );
        }
        if ( found ) {
          if ( ++row > vsize - 1 ) {
            RETURN_BAD_RESULT( "Too many nodes in column "<< col <<": "<< row+1);
          }
          par_nVec_2->second[ row ] = node;
          foundFaces.insert( face );
          n2 = node;
          if ( nbFaceNodes==4 ) {
            n1 = par_nVec_1->second[ row ];
          }
        }
        else if ( nbFaceNodes==3 && n3 == par_nVec_1->second[ row + 1 ] ) {
          n1 = n3;
        }
        else  {
          RETURN_BAD_RESULT( "Not quad mesh, column "<< col );
        }
      }
    }
    while ( face && n1 && n2 );

    if ( row < vsize - 1 ) {
      MESSAGE( "Too few nodes in column "<< col <<": "<< row+1);
      MESSAGE( "Base node 1: "<< par_nVec_1->second[0]);
      MESSAGE( "Base node 2: "<< par_nVec_2->second[0]);
      if ( n1 ) { MESSAGE( "Current node 1: "<< n1); }
      else      { MESSAGE( "Current node 1: NULL");  }
      if ( n2 ) { MESSAGE( "Current node 2: "<< n2); }
      else      { MESSAGE( "Current node 2: NULL");  }
      MESSAGE( "first base node: "<< theParam2ColumnMap.begin()->second[0]);
      MESSAGE( "last base node: "<< theParam2ColumnMap.rbegin()->second[0]);
      return false;
    }
  } // loop on columns

  return true;
}

//=======================================================================
/*!
 * \brief Return number of unique ancestors of the shape
 */
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
/**
 * Check mesh without geometry for: if all elements on this shape are quadratic,
 * quadratic elements will be created.
 * Used then generated 3D mesh without geometry.
 */
//=======================================================================

SMESH_MesherHelper:: MType SMESH_MesherHelper::IsQuadraticMesh()
{
  int NbAllEdgsAndFaces=0;
  int NbQuadFacesAndEdgs=0;
  int NbFacesAndEdges=0;
  //All faces and edges
  NbAllEdgsAndFaces = myMesh->NbEdges() + myMesh->NbFaces();
  
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
/*!
 * \brief Return an alternative parameter for a node on seam
 */
//=======================================================================

double SMESH_MesherHelper::GetOtherParam(const double param) const
{
  return fabs(param-myPar1) < fabs(param-myPar2) ? myPar2 : myPar1;
}

//=======================================================================
namespace { // Structures used by FixQuadraticElements()
//=======================================================================

#define __DMP__(txt) \
//cout << txt
#define MSG(txt) __DMP__(txt<<endl)
#define MSGBEG(txt) __DMP__(txt)

  const double straightTol2 = 1e-33; // to detect straing links

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

    QLink(const SMDS_MeshNode* n1, const SMDS_MeshNode* n2, const SMDS_MeshNode* nm):
      SMESH_TLink( n1,n2 ), _mediumNode(nm), _nodeMove(0,0,0), _nbMoves(0) {
      _faces.reserve(4);
      //if ( MediumPos() != SMDS_TOP_3DSPACE )
        _nodeMove = MediumPnt() - MiddlePnt();
    }
    void SetContinuesFaces() const;
    const QFace* GetContinuesFace( const QFace* face ) const;
    bool OnBoundary() const;
    gp_XYZ MiddlePnt() const { return ( XYZ( node1() ) + XYZ( node2() )) / 2.; }
    gp_XYZ MediumPnt() const { return XYZ( _mediumNode ); }

    SMDS_TypeOfPosition MediumPos() const
    { return _mediumNode->GetPosition()->GetTypeOfPosition(); }
    SMDS_TypeOfPosition EndPos(bool isSecond) const
    { return (isSecond ? node2() : node1())->GetPosition()->GetTypeOfPosition(); }
    const SMDS_MeshNode* EndPosNode(SMDS_TypeOfPosition pos) const
    { return EndPos(0) == pos ? node1() : EndPos(1) == pos ? node2() : 0; }

    void Move(const gp_Vec& move, bool sum=false) const
    { _nodeMove += move; _nbMoves += sum ? (_nbMoves==0) : 1; }
    gp_XYZ Move() const { return _nodeMove.XYZ() / _nbMoves; }
    bool IsMoved() const { return (_nbMoves > 0 && !IsStraight()); }
    bool IsStraight() const { return _nodeMove.SquareMagnitude() <= straightTol2; }

    bool operator<(const QLink& other) const {
      return (node1()->GetID() == other.node1()->GetID() ?
              node2()->GetID() < other.node2()->GetID() :
              node1()->GetID() < other.node1()->GetID());
    }
    struct PtrComparator {
      bool operator() (const QLink* l1, const QLink* l2 ) const { return *l1 < *l2; }
    };
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
    void SetFace(const QFace* face) { int iF = _qfaces[0] ? 1 : 0; _qfaces[iF]=face; }

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
  };
  // --------------------------------------------------------------------
  typedef list< TChainLink > TChain;
  typedef set < TChainLink > TLinkSet;
  typedef TLinkSet::const_iterator TLinkInSet;

  const int theFirstStep = 5;

  enum { ERR_OK, ERR_TRI, ERR_PRISM, ERR_UNKNOWN }; // errors of QFace::GetLinkChain()
  // --------------------------------------------------------------------
  /*!
   * \brief Face shared by two volumes and bound by QLinks
   */
  struct QFace: public TIDSortedElemSet
  {
    mutable const SMDS_MeshElement* _volumes[2];
    mutable vector< const QLink* >  _sides;
    mutable bool                    _sideIsAdded[4]; // added in chain of links
    gp_Vec                          _normal;

    QFace( const vector< const QLink*>& links );

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
    bool GetLinkChain( int iSide, TChain& chain, SMDS_TypeOfPosition pos, int& error) const;

    bool GetLinkChain( TChainLink& link, TChain& chain, SMDS_TypeOfPosition pos, int& error) const
    {
      int i = LinkIndex( link._qlink );
      if ( i < 0 ) return true;
      _sideIsAdded[i] = true;
      link.SetFace( this );
      // continue from opposite link
      return GetLinkChain( (i+2)%_sides.size(), chain, pos, error );
    }
    bool IsBoundary() const { return !_volumes[1]; }

    bool Contains( const SMDS_MeshNode* node ) const { return count(node); }

    TLinkInSet GetBoundaryLink( const TLinkSet&      links,
                                const TChainLink&    avoidLink,
                                TLinkInSet *         notBoundaryLink = 0,
                                const SMDS_MeshNode* nodeToContain = 0,
                                bool *               isAdjacentUsed = 0) const;

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
  ostream& operator << (ostream& out, const QLink& l)
  {
    out <<"QLink nodes: "
        << l.node1()->GetID() << " - "
        << l._mediumNode->GetID() << " - "
        << l.node2()->GetID() << endl;
    return out;
  }
  ostream& operator << (ostream& out, const QFace& f)
  {
    out <<"QFace nodes: "/*<< &f << "  "*/;
    for ( TIDSortedElemSet::const_iterator n = f.begin(); n != f.end(); ++n )
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

  QFace::QFace( const vector< const QLink*>& links )
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
  }
  //================================================================================
  /*!
   * \brief Make up chain of links
   *  \param iSide - link to add first
   *  \param chain - chain to fill in
   *  \param pos   - postion of medium nodes the links should have
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
      for ( int i = 0; i < _sides.size(); ++i ) {
        if ( !_sideIsAdded[i] && _sides[i] ) {
          _sideIsAdded[i]=true;
          TChain::iterator chLink = chain.insert( chain.begin(), TChainLink(_sides[i]));
          chLink->SetFace( this );
          if ( _sides[i]->MediumPos() >= pos )
            if ( const QFace* f = _sides[i]->GetContinuesFace( this ))
              f->GetLinkChain( *chLink, chain, pos, error );
        }
      }
      if ( error < ERR_TRI )
        error = ERR_TRI;
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

    // propagate from rectangle to neighbour faces
    if ( link->MediumPos() >= pos ) {
      int nbLinkFaces = link->_faces.size();
      if ( nbLinkFaces == 4 || nbLinkFaces < 4 && link->OnBoundary()) {
        // hexahedral mesh or boundary quadrangles - goto a continous face
        if ( const QFace* f = link->GetContinuesFace( this ))
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
   */
  //================================================================================

  TLinkInSet QFace::GetBoundaryLink( const TLinkSet&      links,
                                     const TChainLink&    avoidLink,
                                     TLinkInSet *         notBoundaryLink,
                                     const SMDS_MeshNode* nodeToContain,
                                     bool *               isAdjacentUsed) const
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

      if ( boundaryLink == linksEnd && nodeToContain ) // cellect adjacent faces
        if ( const QFace* adj = link->NextFace( this ))
          if ( adj->Contains( nodeToContain ))
            adjacentFaces.push_back( make_pair( adj, link ));
    }

    if ( isAdjacentUsed ) *isAdjacentUsed = false;
    if ( boundaryLink == linksEnd && nodeToContain ) // check adjacent faces
    {
      TFaceLinkList::iterator adj = adjacentFaces.begin();
      for ( ; boundaryLink == linksEnd && adj != adjacentFaces.end(); ++adj )
        boundaryLink = adj->first->GetBoundaryLink( links, *(adj->second),
                                                    0, nodeToContain, isAdjacentUsed);
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
    gp_Vec norm, vecOut;
//     if ( uvHelper ) {
//       TopoDS_Face face = TopoDS::Face( uvHelper->GetSubShape());
//       const SMDS_MeshNode* inFaceNode = uvHelper->GetNodeUVneedInFaceNode() ? GetNodeInFace() : 0;
//       gp_XY uv1 = uvHelper->GetNodeUV( face, _sides[i]->node1(), inFaceNode );
//       gp_XY uv2 = uvHelper->GetNodeUV( face, _sides[i]->node2(), inFaceNode );
//       norm.SetCoord( uv1.Y() - uv2.Y(), uv2.X() - uv1.X(), 0 );

//       const QLink* otherLink = _sides[(i + 1) % _sides.size()];
//       const SMDS_MeshNode* otherNode =
//         otherLink->node1() == _sides[i]->node1() ? otherLink->node2() : otherLink->node1();
//       gp_XY pIn = uvHelper->GetNodeUV( face, otherNode, inFaceNode );
//       vecOut.SetCoord( uv1.X() - pIn.X(), uv1.Y() - pIn.Y(), 0 );
//     }
//     else {
      norm = _normal ^ gp_Vec( XYZ(_sides[i]->node1()), XYZ(_sides[i]->node2()));
      gp_XYZ pIn = ( XYZ( _sides[0]->node1() ) +
                     XYZ( _sides[0]->node2() ) +
                     XYZ( _sides[1]->node1() )) / 3.;
      vecOut.SetXYZ( _sides[i]->MiddlePnt() - pIn );
      //}
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
    const QFace* f1 = link1->NextFace( this ); // adjacent faces
    const QFace* f2 = link2->NextFace( this );

    // propagate to adjacent faces till limit step or boundary
    double len1 = thePrevLen + (theLink->MiddlePnt() - _sides[iL1]->MiddlePnt()).Modulus();
    double len2 = thePrevLen + (theLink->MiddlePnt() - _sides[iL2]->MiddlePnt()).Modulus();
    gp_Vec linkDir1, linkDir2;
    try {
      OCC_CATCH_SIGNALS;
      if ( f1 )
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
      if ( f2 )
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
      theLink->Move( move, true );

      MSG(string(theStep,'.')<<" Move "<< theLink->_mediumNode->GetID()<<
          " by " << refProj * ( 1 - r ) << " following " <<
          (choose1 ? *link1->_qlink : *link2->_qlink));

      if ( theLinkNorm ) *theLinkNorm = linkNorm;
    }
    return fullLen;
  }

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
    int iFaceCont = -1;
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
    }
    if ( iFaceCont > 0 ) // continues faces found, set one by the other
    {
      if ( iFaceCont != 1 )
        std::swap( _faces[1], _faces[iFaceCont] );
    }
    else if ( _faces.size() > 1 ) // not found, set NULL by the first face
    {
      _faces.insert( ++_faces.begin(), 0 );
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
   * \brief Move medium nodes of vertical links of pentahedrons adjacent by side faces
   */
  //================================================================================

  void fixPrism( TChain& allLinks )
  {
    // separate boundary links from internal ones
    typedef set<const QLink*/*, QLink::PtrComparator*/> QLinkSet;
    QLinkSet interLinks, bndLinks1, bndLink2;

    bool isCurved = false;
    for ( TChain::iterator lnk = allLinks.begin(); lnk != allLinks.end(); ++lnk ) {
      if ( (*lnk)->OnBoundary() )
        bndLinks1.insert( lnk->_qlink );
      else
        interLinks.insert( lnk->_qlink );
      isCurved = isCurved || !(*lnk)->IsStraight();
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
          interLinks. erase( pInterLink );
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

    // move in 2d if we are on geom face
//     TopoDS_Face face;
//     TopLoc_Location loc;
//     SMESH_MesherHelper faceHelper( *helper.GetMesh());
//     while ( linkIt->IsBoundary()) ++linkIt;
//     if ( linkIt == linksEnd ) return;
//     if ( (*linkIt)->MediumPos() == SMDS_TOP_FACE ) {
//       bool checkPos = true;
//       TopoDS_Shape f = helper.GetSubShapeByNode( (*linkIt)->_mediumNode, helper.GetMeshDS() );
//       if ( !f.IsNull() && f.ShapeType() == TopAbs_FACE ) {
//         face = TopoDS::Face( f );
//         helper.GetNodeUV( face, (*linkIt)->_mediumNode, 0, &checkPos);
//         if (checkPos)
//           face.Nullify();
//         else
//           faceHelper.SetSubShape( face );
//       }
//     }
    for ( linkIt = linkSet.begin(); linkIt != linksEnd; ++linkIt)
    {
      if ( linkIt->IsBoundary() && !(*linkIt)->IsStraight() && linkIt->_qfaces[0])
      {
//         if ( !face.IsNull() ) {
//           const SMDS_MeshNode* inFaceNode =
//             faceHelper.GetNodeUVneedInFaceNode() ? linkIt->_qfaces[0]->GetNodeInFace() : 0;
//           gp_XY uvm = helper.GetNodeUV( face, (*linkIt)->_mediumNode, inFaceNode );
//           gp_XY uv1 = helper.GetNodeUV( face, (*linkIt)->node1(), inFaceNode);
//           gp_XY uv2 = helper.GetNodeUV( face, (*linkIt)->node2(), inFaceNode);
//           gp_XY uvMove = uvm - helper.GetMiddleUV( BRep_Tool::Surface(face,loc), uv1, uv2);
//           gp_Vec move( uvMove.X(), uvMove.Y(), 0 );
//           linkIt->_qfaces[0]->MoveByBoundary( *linkIt, move, linkSet, &faceHelper );
//         }
//         else {
          linkIt->_qfaces[0]->MoveByBoundary( *linkIt, (*linkIt)->_nodeMove, linkSet );
          //}
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
    _NO_MIDQUAD, _NO_UPTRIA, _BAD_SET_SIZE, _BAD_CORNER, _BAD_START, _NO_BOTLINK };

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

          #ifdef __BORLANDC__
          linkSet.erase( *botLink );
          #else
          linkSet.erase( botLink );
          #endif

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

        #ifdef __BORLANDC__
        linkSet.erase( *botLink );
        #else
        linkSet.erase( botLink );
        #endif

        botLink = upTria->GetLinkByNode( linkSet, (isCase2 ? *sideLink : *midQuadLink), corner );
        if ( botLink == linksEnd || botLink == (isCase2 ? midQuadLink : sideLink))
          return _NO_BOTLINK;

        #ifdef __BORLANDC__
        linkSet.erase( *midQuadLink );
        linkSet.erase( *sideLink );
        #else
        linkSet.erase( midQuadLink );
        linkSet.erase( sideLink );
        #endif

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

      #ifdef __BORLANDC__
      linkSet.erase( *startLink );
      #else
      linkSet.erase( startLink );
      #endif

      startCorner = corner;
    }

    return _OK;
  }
}

//=======================================================================
/*!
 * \brief Move medium nodes of faces and volumes to fix distorted elements
 * \param volumeOnly - to fix nodes on faces or not, if the shape is solid
 * 
 * Issue 0020307: EDF 992 SMESH : Linea/Quadratic with Medium Node on Geometry
 */
//=======================================================================

void SMESH_MesherHelper::FixQuadraticElements(bool volumeOnly)
{
  // apply algorithm to solids or geom faces
  // ----------------------------------------------
  if ( myShape.IsNull() ) {
    if ( !myMesh->HasShapeToMesh() ) return;
    SetSubShape( myMesh->GetShapeToMesh() );

    TopTools_MapOfShape faces; // faces not in solid or in not meshed solid
    for ( TopExp_Explorer f(myShape,TopAbs_FACE,TopAbs_SOLID); f.More(); f.Next() ) {
      faces.Add( f.Current() );
    }
    for ( TopExp_Explorer v(myShape,TopAbs_SOLID); v.More(); v.Next() ) {
      if ( myMesh->GetSubMesh( v.Current() )->IsEmpty() ) { // get faces of solid
        for ( TopExp_Explorer f( v.Current(), TopAbs_FACE); f.More(); f.Next() )
          faces.Add( f.Current() );
      }
      else { // fix nodes in the solid and its faces
        SMESH_MesherHelper h(*myMesh);
        h.SetSubShape( v.Current() );
        h.FixQuadraticElements(false);
      }
    }
    // fix nodes on geom faces
    for ( TopTools_MapIteratorOfMapOfShape fIt( faces ); fIt.More(); fIt.Next() ) {
      SMESH_MesherHelper h(*myMesh);
      h.SetSubShape( fIt.Key() );
      h.FixQuadraticElements();
    }
    return;
  }

  // Find out type of elements and get iterator on them
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

  // Fill in auxiliary data structures
  // ----------------------------------

  set< QLink > links;
  set< QFace > faces;
  set< QLink >::iterator pLink;
  set< QFace >::iterator pFace;

  bool isCurved = false;
  bool hasRectFaces = false;
  set<int> nbElemNodeSet;

  if ( elemType == SMDSAbs_Volume )
  {
    SMDS_VolumeTool volTool;
    while ( elemIt->more() ) // loop on volumes
    {
      const SMDS_MeshElement* vol = elemIt->next();
      if ( !vol->IsQuadratic() || !volTool.Set( vol ))
        return; //continue;
      for ( int iF = 0; iF < volTool.NbFaces(); ++iF ) // loop on faces of volume
      {
        int nbN = volTool.NbFaceNodes( iF );
        nbElemNodeSet.insert( nbN );
        const SMDS_MeshNode** faceNodes = volTool.GetFaceNodes( iF );
        vector< const QLink* > faceLinks( nbN/2 );
        for ( int iN = 0; iN < nbN; iN += 2 ) // loop on links of a face
        {
          // store QLink
          QLink link( faceNodes[iN], faceNodes[iN+2], faceNodes[iN+1] );
          pLink = links.insert( link ).first;
          faceLinks[ iN/2 ] = & *pLink;
          if ( !isCurved )
            isCurved = !link.IsStraight();
          if ( link.MediumPos() == SMDS_TOP_3DSPACE && !link.IsStraight() )
            return; // already fixed
        }
        // store QFace
        pFace = faces.insert( QFace( faceLinks )).first;
        if ( pFace->NbVolumes() == 0 )
          pFace->AddSelfToLinks();
        pFace->SetVolume( vol );
        hasRectFaces = hasRectFaces ||
          ( volTool.GetVolumeType() == SMDS_VolumeTool::QUAD_HEXA ||
            volTool.GetVolumeType() == SMDS_VolumeTool::QUAD_PENTA );
      }
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
      nbElemNodeSet.insert( face->NbNodes() );
      int nbN = face->NbNodes()/2;
      vector< const QLink* > faceLinks( nbN );
      for ( int iN = 0; iN < nbN; ++iN ) // loop on links of a face
      {
        // store QLink
        QLink link( face->GetNode(iN), face->GetNode((iN+1)%nbN), face->GetNode(iN+nbN) );
        pLink = links.insert( link ).first;
        faceLinks[ iN ] = & *pLink;
        if ( !isCurved )
          isCurved = !link.IsStraight();
      }
      // store QFace
      pFace = faces.insert( QFace( faceLinks )).first;
      pFace->AddSelfToLinks();
      hasRectFaces = ( hasRectFaces || nbN == 4 );
    }
  }
  if ( !isCurved )
    return; // no curved edges of faces

  // Compute displacement of medium nodes
  // -------------------------------------

  // two loops on faces: the first is to treat boundary links, the second is for internal ones
  TopLoc_Location loc;
  // not treat boundary of volumic submesh
  int isInside = ( elemType == SMDSAbs_Volume && volumeOnly ) ? 1 : 0;
  for ( ; isInside < 2; ++isInside ) {
    MSG( "--------------- LOOP " << isInside << " ------------------");
    SMDS_TypeOfPosition pos = isInside ? SMDS_TOP_3DSPACE : SMDS_TOP_FACE;

    for ( pFace = faces.begin(); pFace != faces.end(); ++pFace ) {
      if ( bool(isInside) == pFace->IsBoundary() )
        continue;
      for ( int dir = 0; dir < 2; ++dir ) // 2 directions of propagation from quadrangle
      {
        MSG( "CHAIN");
        // make chain of links connected via continues faces
        int error = ERR_OK;
        TChain rawChain;
        if ( !pFace->GetLinkChain( dir, rawChain, pos, error) && error ==ERR_UNKNOWN ) continue;
        rawChain.reverse();
        if ( !pFace->GetLinkChain( dir+2, rawChain, pos, error ) && error ==ERR_UNKNOWN ) continue;

        vector< TChain > chains;
        if ( error == ERR_OK ) { // chains contains continues rectangles
          chains.resize(1);
          chains[0].splice( chains[0].begin(), rawChain );
        }
        else if ( error == ERR_TRI ) {  // chains contains continues triangles
          TSplitTriaResult res = splitTrianglesIntoChains( rawChain, chains, pos );
          if ( res != _OK ) { // not rectangles split into triangles
            fixTriaNearBoundary( rawChain, *this );
            break;
          }
        }
        else if ( error == ERR_PRISM ) { // side faces of prisms
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
          if ( chain.front()->IsStraight() && chain.back()->IsStraight() ) {
            MSG("3D straight");
            continue;
          }
          // mesure chain length and compute link position along the chain
          double chainLen = 0;
          vector< double > linkPos;
          MSGBEG( "Link medium nodes: ");
          TChain::iterator link0 = chain.begin(), link1 = chain.begin(), link2;
          for ( ++link1; link1 != chain.end(); ++link1, ++link0 ) {
            MSGBEG( (*link0)->_mediumNode->GetID() << "-" <<(*link1)->_mediumNode->GetID()<<" ");
            double len = ((*link0)->MiddlePnt() - (*link1)->MiddlePnt()).Modulus();
            while ( len < numeric_limits<double>::min() ) { // remove degenerated link
              link1 = chain.erase( link1 );
              if ( link1 == chain.end() )
                break;
              len = ((*link0)->MiddlePnt() - (*link1)->MiddlePnt()).Modulus();
            }
            chainLen += len;
            linkPos.push_back( chainLen );
          }
          MSG("");
          if ( linkPos.size() < 2 )
            continue;

          gp_Vec move0 = chain.front()->_nodeMove;
          gp_Vec move1 = chain.back ()->_nodeMove;

          TopoDS_Face face;
          bool checkUV = true;
          if ( !isInside ) {
            // compute node displacement of end links in parametric space of face
            const SMDS_MeshNode* nodeOnFace = (*(++chain.begin()))->_mediumNode;
            TopoDS_Shape f = GetSubShapeByNode( nodeOnFace, GetMeshDS() );
            if ( !f.IsNull() && f.ShapeType() == TopAbs_FACE ) {
              face = TopoDS::Face( f );
              for ( int is1 = 0; is1 < 2; ++is1 ) { // move0 or move1
                TChainLink& link = is1 ? chain.back() : chain.front();
                gp_XY uv1 = GetNodeUV( face, link->node1(), nodeOnFace, &checkUV);
                gp_XY uv2 = GetNodeUV( face, link->node2(), nodeOnFace, &checkUV);
                gp_XY uvm = GetNodeUV( face, link->_mediumNode, nodeOnFace, &checkUV);
                gp_XY uvMove = uvm - GetMiddleUV( BRep_Tool::Surface(face,loc), uv1, uv2);
                if ( is1 ) move1.SetCoord( uvMove.X(), uvMove.Y(), 0 );
                else       move0.SetCoord( uvMove.X(), uvMove.Y(), 0 );
              }
              if ( move0.SquareMagnitude() < straightTol2 &&
                   move1.SquareMagnitude() < straightTol2 ) {
                MSG("2D straight");
                continue; // straight - no need to move nodes of internal links
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
              gp_Vec x = x01.Normalized() + x12.Normalized();
              trsf.SetTransformation( gp_Ax3( gp::Origin(), link1->Normal(), x), gp_Ax3() );
              move.Transform(trsf);
            }
            else {
              // compute 3D displacement by 2D one
              gp_XY oldUV   = GetNodeUV( face, (*link1)->_mediumNode, 0, &checkUV);
              gp_XY newUV   = oldUV + gp_XY( move.X(), move.Y() );
              gp_Pnt newPnt = BRep_Tool::Surface(face,loc)->Value( newUV.X(), newUV.Y());
              move = gp_Vec( XYZ((*link1)->_mediumNode), newPnt.Transformed(loc) );
#ifdef _DEBUG_
              if ( (XYZ((*link1)->node1()) - XYZ((*link1)->node2())).SquareModulus() <
                   move.SquareMagnitude())
              {
                gp_XY uv0 = GetNodeUV( face, (*link0)->_mediumNode, 0, &checkUV);
                gp_XY uv2 = GetNodeUV( face, (*link2)->_mediumNode, 0, &checkUV);
                MSG( "uv0: "<<uv0.X()<<", "<<uv0.Y()<<" \t" <<
                     "uv2: "<<uv2.X()<<", "<<uv2.Y()<<" \t" <<
                     "uvOld: "<<oldUV.X()<<", "<<oldUV.Y()<<" \t" <<
                     "newUV: "<<newUV.X()<<", "<<newUV.Y()<<" \t");
              }
#endif
            }
            (*link1)->Move( move );
            MSG( "Move " << (*link1)->_mediumNode->GetID() << " following "
                 << chain.front()->_mediumNode->GetID() <<"-"
                 << chain.back ()->_mediumNode->GetID() <<
                 " by " << move.Magnitude());
          }
        } // loop on chains of links
      } // loop on 2 directions of propagation from quadrangle
    } // loop on faces
  }

  // Move nodes
  // -----------

  for ( pLink = links.begin(); pLink != links.end(); ++pLink ) {
    if ( pLink->IsMoved() ) {
      //gp_Pnt p = pLink->MediumPnt() + pLink->Move();
      gp_Pnt p = pLink->MiddlePnt() + pLink->Move();
      GetMeshDS()->MoveNode( pLink->_mediumNode, p.X(), p.Y(), p.Z());
    }
  }
}
