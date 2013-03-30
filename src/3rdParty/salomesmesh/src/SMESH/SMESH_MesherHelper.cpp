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

#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom_Curve.hxx>
#include <Geom_Surface.hxx>
#include <ShapeAnalysis.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt2d.hxx>

#include <Standard_Failure.hxx>
#include <Standard_ErrorHandler.hxx>

#include <utilities.h>

#define RETURN_BAD_RESULT(msg) { MESSAGE(msg); return false; }

//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

SMESH_MesherHelper::SMESH_MesherHelper(SMESH_Mesh& theMesh)
  : myMesh(&theMesh), myShapeID(-1), myCreateQuadratic(false)
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
  // also we have to fill myNLinkNodeMap
  myCreateQuadratic = true;
  mySeamShapeIds.clear();
  myDegenShapeIds.clear();
  TopAbs_ShapeEnum subType( aSh.ShapeType()==TopAbs_FACE ? TopAbs_EDGE : TopAbs_FACE );
  SMDSAbs_ElementType elemType( subType==TopAbs_FACE ? SMDSAbs_Face : SMDSAbs_Edge );

  int nbOldLinks = myNLinkNodeMap.size();

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
            // fill NLinkNodeMap
            switch ( e->NbNodes() ) {
            case 3:
              AddNLinkNode(e->GetNode(0),e->GetNode(1),e->GetNode(2)); break;
            case 6:
              AddNLinkNode(e->GetNode(0),e->GetNode(1),e->GetNode(3));
              AddNLinkNode(e->GetNode(1),e->GetNode(2),e->GetNode(4));
              AddNLinkNode(e->GetNode(2),e->GetNode(0),e->GetNode(5)); break;
            case 8:
              AddNLinkNode(e->GetNode(0),e->GetNode(1),e->GetNode(4));
              AddNLinkNode(e->GetNode(1),e->GetNode(2),e->GetNode(5));
              AddNLinkNode(e->GetNode(2),e->GetNode(3),e->GetNode(6));
              AddNLinkNode(e->GetNode(3),e->GetNode(0),e->GetNode(7));
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

  if ( nbOldLinks == myNLinkNodeMap.size() )
    myCreateQuadratic = false;

  if(!myCreateQuadratic) {
    myNLinkNodeMap.clear();
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
    myShapeID  = -1;
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

  Handle(Geom_Surface) aSurface = BRep_Tool::Surface( F );
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
//function : AddNLinkNode
//purpose  : 
//=======================================================================
/*!
 * Auxilary function for filling myNLinkNodeMap
 */
void SMESH_MesherHelper::AddNLinkNode(const SMDS_MeshNode* n1,
                                     const SMDS_MeshNode* n2,
                                     const SMDS_MeshNode* n12)
{
  NLink link( n1, n2 );
  if ( n1 > n2 ) link = NLink( n2, n1 );
  // add new record to map
  myNLinkNodeMap.insert( make_pair(link,n12));
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
 * \retval gp_XY - resulting UV
 * 
 * Auxilary function called form GetMediumNode()
 */
//=======================================================================

gp_XY SMESH_MesherHelper::GetNodeUV(const TopoDS_Face&   F,
                                    const SMDS_MeshNode* n,
                                    const SMDS_MeshNode* n2) const
{
  gp_Pnt2d uv( 1e100, 1e100 );
  const SMDS_PositionPtr Pos = n->GetPosition();
  if(Pos->GetTypeOfPosition()==SMDS_TOP_FACE)
  {
    // node has position on face
    const SMDS_FacePosition* fpos =
      static_cast<const SMDS_FacePosition*>(n->GetPosition().get());
    uv = gp_Pnt2d(fpos->GetUParameter(),fpos->GetVParameter());
  }
  else if(Pos->GetTypeOfPosition()==SMDS_TOP_EDGE)
  {
    // node has position on edge => it is needed to find
    // corresponding edge from face, get pcurve for this
    // edge and recieve value from this pcurve
    const SMDS_EdgePosition* epos =
      static_cast<const SMDS_EdgePosition*>(n->GetPosition().get());
    SMESHDS_Mesh* meshDS = GetMeshDS();
    int edgeID = Pos->GetShapeId();
    TopoDS_Edge E = TopoDS::Edge(meshDS->IndexToShape(edgeID));
    double f, l;
    Handle(Geom2d_Curve) C2d = BRep_Tool::CurveOnSurface(E, F, f, l);
    uv = C2d->Value( epos->GetUParameter() );
    // for a node on a seam edge select one of UVs on 2 pcurves
    if ( n2 && IsSeamShape( edgeID ) )
      uv = GetUVOnSeam( uv, GetNodeUV( F, n2, 0 ));
  }
  else if(Pos->GetTypeOfPosition()==SMDS_TOP_VERTEX)
  {
    if ( int vertexID = n->GetPosition()->GetShapeId() ) {
      bool ok = true;
      const TopoDS_Vertex& V = TopoDS::Vertex(GetMeshDS()->IndexToShape(vertexID));
      try {
        uv = BRep_Tool::Parameters( V, F );
      }
      catch (Standard_Failure& exc) {
        ok = false;
      }
      if ( !ok ) {
        for ( TopExp_Explorer vert(F,TopAbs_VERTEX); !ok && vert.More(); vert.Next() )
          ok = ( V == vert.Current() );
        if ( !ok ) {
#ifdef _DEBUG_
          MESSAGE ( "SMESH_MesherHelper::GetNodeUV(); Vertex " << vertexID
               << " not in face " << GetMeshDS()->ShapeToIndex( F ) );
#endif
          // get UV of a vertex closest to the node
          double dist = 1e100;
          gp_Pnt pn ( n->X(),n->Y(),n->Z() );
          for ( TopExp_Explorer vert(F,TopAbs_VERTEX); !ok && vert.More(); vert.Next() ) {
            TopoDS_Vertex curV = TopoDS::Vertex( vert.Current() );
            gp_Pnt p = BRep_Tool::Pnt( curV );
            double curDist = p.SquareDistance( pn );
            if ( curDist < dist ) {
              dist = curDist;
              uv = BRep_Tool::Parameters( curV, F );
              if ( dist < DBL_MIN ) break;
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
  return uv.XY();
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
                                    const SMDS_MeshNode* n)
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

//=======================================================================
//function : GetMediumNode
//purpose  : 
//=======================================================================
/*!
 * Special function for search or creation medium node
 */
const SMDS_MeshNode* SMESH_MesherHelper::GetMediumNode(const SMDS_MeshNode* n1,
                                                       const SMDS_MeshNode* n2,
                                                       bool force3d)
{
  TopAbs_ShapeEnum shapeType = myShape.IsNull() ? TopAbs_SHAPE : myShape.ShapeType();

  NLink link(( n1 < n2 ? n1 : n2 ), ( n1 < n2 ? n2 : n1 ));
  ItNLinkNode itLN = myNLinkNodeMap.find( link );
  if ( itLN != myNLinkNodeMap.end() ) {
    return (*itLN).second;
  }
  else {
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

    if(!force3d) {
      // we try to create medium node using UV parameters of
      // nodes, else - medium between corresponding 3d points
      if(faceID>-1 || shapeType == TopAbs_FACE) {
	// obtaining a face and 2d points for nodes
	TopoDS_Face F;
	if( myShape.IsNull() )
          F = TopoDS::Face(meshDS->IndexToShape(faceID));
	else {
          F = TopoDS::Face(myShape);
          faceID = myShapeID;
        }

	gp_XY p1 = GetNodeUV(F,n1,n2);
        gp_XY p2 = GetNodeUV(F,n2,n1);

	if ( IsDegenShape( Pos1->GetShapeId() ))
	  p1.SetCoord( myParIndex, p2.Coord( myParIndex ));
	else if ( IsDegenShape( Pos2->GetShapeId() ))
	  p2.SetCoord( myParIndex, p1.Coord( myParIndex ));

	//checking if surface is periodic
	Handle(Geom_Surface) S = BRep_Tool::Surface(F);
	Standard_Real UF,UL,VF,VL;
	S->Bounds(UF,UL,VF,VL);

	Standard_Real u,v;
	Standard_Boolean isUPeriodic = S->IsUPeriodic();
	if(isUPeriodic) {
	  Standard_Real UPeriod = S->UPeriod();
	  Standard_Real p2x = p2.X()+ShapeAnalysis::AdjustByPeriod(p2.X(),p1.X(),UPeriod);
	  Standard_Real pmid = (p1.X()+p2x)/2.;
	  u = pmid+ShapeAnalysis::AdjustToPeriod(pmid,UF,UL);
	}
	else 
	  u= (p1.X()+p2.X())/2.;

	Standard_Boolean isVPeriodic = S->IsVPeriodic();
	if(isVPeriodic) {
	  Standard_Real VPeriod = S->VPeriod();
	  Standard_Real p2y = p2.Y()+ShapeAnalysis::AdjustByPeriod(p2.Y(),p1.Y(),VPeriod);
	  Standard_Real pmid = (p1.Y()+p2y)/2.;
	  v = pmid+ShapeAnalysis::AdjustToPeriod(pmid,VF,VL);
	}
	else
	  v = (p1.Y()+p2.Y())/2.;

        gp_Pnt P = S->Value(u, v);
        n12 = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnFace(n12, faceID, u, v);
        myNLinkNodeMap.insert(NLinkNodeMap::value_type(link,n12));
        return n12;
      }
      if (edgeID>-1 || shapeType == TopAbs_EDGE) {

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
          myNLinkNodeMap.insert(NLinkNodeMap::value_type(link,n12));
          return n12;
	}
      }
    }
    // 3d variant
    double x = ( n1->X() + n2->X() )/2.;
    double y = ( n1->Y() + n2->Y() )/2.;
    double z = ( n1->Z() + n2->Z() )/2.;
    n12 = meshDS->AddNode(x,y,z);
    if(edgeID>-1)
        meshDS->SetNodeOnEdge(n12, edgeID);
    else if(faceID>-1)
        meshDS->SetNodeOnFace(n12, faceID);
    else
      meshDS->SetNodeInVolume(n12, myShapeID);
    myNLinkNodeMap.insert(NLinkNodeMap::value_type(link,n12));
    return n12;
  }
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
