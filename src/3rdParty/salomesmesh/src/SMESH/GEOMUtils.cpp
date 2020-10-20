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

// Suppress warning due to use of #import an macOS inside Aspect_RenderingContext.hxx
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wimport-preprocessor-directive-pedantic"
#endif

#include <Standard_Stream.hxx>

#include <GEOMUtils.hxx>

#include <Basics_OCCTVersion.hxx>

#include <OpUtil.hxx>
#include <Utils_ExceptHandlers.hxx>

// OCCT Includes
#include <BRepMesh_IncrementalMesh.hxx>

#include <BRepExtrema_DistShapeShape.hxx>

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>

#include <BRepClass3d_SolidClassifier.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Sewing.hxx>

#include <BRepCheck_Analyzer.hxx>

#include <Bnd_Box.hxx>

#include <BOPTools_AlgoTools.hxx>

#include <TopAbs.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_Array1OfShape.hxx>

#include <Geom_Circle.hxx>
#include <Geom_Surface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_SphericalSurface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>

#include <GeomLProp_CLProps.hxx>
#include <GeomLProp_SLProps.hxx>

#include <GProp_GProps.hxx>
#include <GProp_PrincipalProps.hxx>

#include <TColStd_Array1OfReal.hxx>

#include <gp_Pln.hxx>
#include <gp_Lin.hxx>

#include <ShapeAnalysis.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>

#include <ProjLib.hxx>
#include <ElSLib.hxx>

#include <vector>
#include <sstream>
#include <algorithm>

#include <Standard_Failure.hxx>
#include <Standard_NullObject.hxx>
#include <Standard_ErrorHandler.hxx> // CAREFUL ! position of this file is critic : see Lucien PIGNOLONI / OCC

#define MAX2(X, Y)    (Abs(X) > Abs(Y) ? Abs(X) : Abs(Y))
#define MAX3(X, Y, Z) (MAX2(MAX2(X,Y), Z))

#define STD_SORT_ALGO 1

// When the following macro is defined, ShapeFix_ShapeTolerance function is used to set max tolerance of curve
// in GEOMUtils::FixShapeCurves function; otherwise less restrictive BRep_Builder::UpdateEdge/UpdateVertex
// approach is used
// VSR (29/12/2014): macro disabled
//#define USE_LIMIT_TOLERANCE

namespace
{
  /**
   * This function constructs and returns modified shape from the original one
   * for singular cases. It is used for the method GetMinDistanceSingular.
   *
   * \param theShape the original shape
   * \param theModifiedShape output parameter. The modified shape.
   * \param theAddDist output parameter. The added distance for modified shape.
   * \retval true if the shape is modified; false otherwise.
   *
   * \internal
   */
  Standard_Boolean ModifyShape(const TopoDS_Shape  &theShape,
                               TopoDS_Shape  &theModifiedShape,
                               Standard_Real &theAddDist)
  {
    TopExp_Explorer anExp;
    int nbf = 0;

    theAddDist = 0.;
    theModifiedShape.Nullify();

    for ( anExp.Init( theShape, TopAbs_FACE ); anExp.More(); anExp.Next() ) {
      nbf++;
      theModifiedShape = anExp.Current();
    }
    if(nbf==1) {
      TopoDS_Shape sh = theShape;
      while(sh.ShapeType()==TopAbs_COMPOUND) {
        TopoDS_Iterator it(sh);
        sh = it.Value();
      }
      Handle(Geom_Surface) S = BRep_Tool::Surface(TopoDS::Face(theModifiedShape));
      if( S->IsKind(STANDARD_TYPE(Geom_SphericalSurface)) ||
          S->IsKind(STANDARD_TYPE(Geom_ToroidalSurface)) ||
          S->IsUPeriodic()) {
        const Standard_Boolean isShell =
          (sh.ShapeType()==TopAbs_SHELL || sh.ShapeType()==TopAbs_FACE);

        if ( !isShell && S->IsKind(STANDARD_TYPE(Geom_SphericalSurface)) ) {
          Handle(Geom_SphericalSurface) SS = Handle(Geom_SphericalSurface)::DownCast(S);
          gp_Pnt PC = SS->Location();
          BRep_Builder B;
          TopoDS_Vertex V;
          B.MakeVertex(V,PC,1.e-7);
          theModifiedShape = V;
          theAddDist = SS->Radius();
          return Standard_True;
        }
        if ( !isShell && S->IsKind(STANDARD_TYPE(Geom_ToroidalSurface)) ) {
          Handle(Geom_ToroidalSurface) TS = Handle(Geom_ToroidalSurface)::DownCast(S);
          gp_Ax3 ax3 = TS->Position();
          Handle(Geom_Circle) C = new Geom_Circle(ax3.Ax2(),TS->MajorRadius());
          BRep_Builder B;
          TopoDS_Edge E;
          B.MakeEdge(E,C,1.e-7);
          theModifiedShape = E;
          theAddDist = TS->MinorRadius();
          return Standard_True;
        }

        // non solid case or any periodic surface (Mantis 22454).
        double U1,U2,V1,V2;
        // changes for 0020677: EDF 1219 GEOM: MinDistance gives 0 instead of 20.88
        //S->Bounds(U1,U2,V1,V2); changed by
        ShapeAnalysis::GetFaceUVBounds(TopoDS::Face(theModifiedShape),U1,U2,V1,V2);
        // end of changes for 020677 (dmv)
        Handle(Geom_RectangularTrimmedSurface) TrS1 =
          new Geom_RectangularTrimmedSurface(S,U1,(U1+U2)/2.,V1,V2);
        Handle(Geom_RectangularTrimmedSurface) TrS2 =
          new Geom_RectangularTrimmedSurface(S,(U1+U2)/2.,U2,V1,V2);
        TopoDS_Shape aMShape;
        
        TopoDS_Face F1 = BRepBuilderAPI_MakeFace(TrS1, Precision::Confusion());
        TopoDS_Face F2 = BRepBuilderAPI_MakeFace(TrS2, Precision::Confusion());
        
        if (isShell) {
          BRep_Builder B;
          B.MakeCompound(TopoDS::Compound(aMShape));
          B.Add(aMShape, F1);
          B.Add(aMShape, F2);
        } else {
          // The original shape is a solid.
          BRepBuilderAPI_Sewing aSewing (Precision::Confusion()*10.0);
          aSewing.Add(F1);
          aSewing.Add(F2);
          aSewing.Perform();
          aMShape = aSewing.SewedShape();
          BRep_Builder B;
          TopoDS_Solid aSolid;
          B.MakeSolid(aSolid);
          B.Add(aSolid, aMShape);
          aMShape = aSolid;
        }
        
        Handle(ShapeFix_Shape) sfs = new ShapeFix_Shape;
        sfs->Init(aMShape);
        sfs->SetPrecision(1.e-6);
        sfs->SetMaxTolerance(1.0);
        sfs->Perform();
        theModifiedShape = sfs->Shape();
        return Standard_True;
      }
    }
    
    theModifiedShape = theShape;
    return Standard_False;
  }

  void parseWard( const GEOMUtils::LevelsList &theLevelList, std::string &treeStr )
  {
    treeStr.append( "{" );
    for( GEOMUtils::LevelsList::const_iterator j = theLevelList.begin(); 
         j != theLevelList.end(); ++j ) {
      if ( j != theLevelList.begin() ) {
        treeStr.append( ";" );
      }
      GEOMUtils::LevelInfo level = (*j);
      GEOMUtils::LevelInfo::iterator upIter;
      for ( upIter = level.begin(); upIter != level.end(); ++upIter ) {
        if ( upIter != level.begin() ) {
          treeStr.append( "," );
        }
        treeStr.append( upIter->first );
        for ( std::vector<std::string>::iterator k = upIter->second.begin(); k != upIter->second.end(); ++k ) {
          treeStr.append( "_" );
          treeStr.append( *k );
        }
      }
    }
    treeStr.append( "}" );
  }

  GEOMUtils::LevelsList parseWard( const std::string& theData, std::size_t& theCursor )
  {
    std::size_t indexStart = theData.find( "{", theCursor ) + 1;
    std::size_t indexEnd = theData.find( "}", indexStart );

    std::string ward = theData.substr( indexStart, indexEnd - indexStart );
    std::stringstream ss(ward);
    std::string substr;
    std::vector<std::string> levelsListStr;
    while ( std::getline( ss, substr, ';' ) ) {
      if ( !substr.empty() )
        levelsListStr.push_back( substr );
    }
    GEOMUtils::LevelsList levelsListData;
    for( int level = 0; level < levelsListStr.size(); level++ ) {
      std::vector<std::string> namesListStr;
      std::stringstream ss1( levelsListStr[level] );
      while ( std::getline( ss1, substr, ',' ) ) {
        if ( !substr.empty() )
          namesListStr.push_back( substr );
      }
      GEOMUtils::LevelInfo levelInfoData;
      for( int node = 0; node < namesListStr.size(); node++ ) {
        std::vector<std::string> linksListStr;
        std::stringstream ss2( namesListStr[node] );
        while ( std::getline( ss2, substr, '_' ) ) {
          if ( !substr.empty() )
            linksListStr.push_back( substr );
        }
        std::string nodeItem = linksListStr[0];
        if( !nodeItem.empty() ) {
          GEOMUtils::NodeLinks linksListData;
          for( int link = 1; link < linksListStr.size(); link++ ) {
            std::string linkItem = linksListStr[link];
            linksListData.push_back( linkItem );
          }// Links
          levelInfoData[nodeItem] = linksListData;
        }
      }// Level's objects
      levelsListData.push_back(levelInfoData);
    }// Levels

    theCursor = indexEnd + 1;
    return levelsListData;
  }

}

//=======================================================================
//function : ShapeToDouble
//purpose  : used by CompareShapes::operator()
//=======================================================================
std::pair<double, double> GEOMUtils::ShapeToDouble (const TopoDS_Shape& S, bool isOldSorting)
{
  // Computing of CentreOfMass
  gp_Pnt GPoint;
  double Len;

  if (S.ShapeType() == TopAbs_VERTEX) {
    GPoint = BRep_Tool::Pnt(TopoDS::Vertex(S));
    Len = (double)S.Orientation();
  }
  else {
    GProp_GProps GPr;
    // BEGIN: fix for Mantis issue 0020842
    if (isOldSorting) {
      BRepGProp::LinearProperties(S, GPr);
    }
    else {
      if (S.ShapeType() == TopAbs_EDGE || S.ShapeType() == TopAbs_WIRE) {
        BRepGProp::LinearProperties(S, GPr);
      }
      else if (S.ShapeType() == TopAbs_FACE || S.ShapeType() == TopAbs_SHELL) {
        BRepGProp::SurfaceProperties(S, GPr);
      }
      else {
        BRepGProp::VolumeProperties(S, GPr);
      }
    }
    // END: fix for Mantis issue 0020842
    GPoint = GPr.CentreOfMass();
    Len = GPr.Mass();
  }

  double dMidXYZ = GPoint.X() * 999.0 + GPoint.Y() * 99.0 + GPoint.Z() * 0.9;
  return std::make_pair(dMidXYZ, Len);
}

//=======================================================================
//function : GetPosition
//purpose  :
//=======================================================================
gp_Ax3 GEOMUtils::GetPosition (const TopoDS_Shape& theShape)
{
  gp_Ax3 aResult;

  if (theShape.IsNull())
    return aResult;

  // Axes
  aResult.Transform(theShape.Location().Transformation());
  if (theShape.ShapeType() == TopAbs_FACE) {
    Handle(Geom_Surface) aGS = BRep_Tool::Surface(TopoDS::Face(theShape));
    if (!aGS.IsNull() && aGS->IsKind(STANDARD_TYPE(Geom_Plane))) {
      Handle(Geom_Plane) aGPlane = Handle(Geom_Plane)::DownCast(aGS);
      gp_Pln aPln = aGPlane->Pln();
      aResult = aPln.Position();
      // In case of reverse orinetation of the face invert the plane normal
      // (the face's normal does not mathc the plane's normal in this case)
      if(theShape.Orientation() == TopAbs_REVERSED)
      {
        gp_Dir Vx =  aResult.XDirection();
        gp_Dir N  =  aResult.Direction().Mirrored(Vx);
        gp_Pnt P  =  aResult.Location();
        aResult = gp_Ax3(P, N, Vx);
      }
    }
  }

  // Origin
  gp_Pnt aPnt;

  TopAbs_ShapeEnum aShType = theShape.ShapeType();

  if (aShType == TopAbs_VERTEX) {
    aPnt = BRep_Tool::Pnt(TopoDS::Vertex(theShape));
  }
  else {
    if (aShType == TopAbs_COMPOUND) {
      aShType = GetTypeOfSimplePart(theShape);
    }

    GProp_GProps aSystem;
    if (aShType == TopAbs_EDGE || aShType == TopAbs_WIRE)
      BRepGProp::LinearProperties(theShape, aSystem);
    else if (aShType == TopAbs_FACE || aShType == TopAbs_SHELL)
      BRepGProp::SurfaceProperties(theShape, aSystem);
    else
      BRepGProp::VolumeProperties(theShape, aSystem);

    aPnt = aSystem.CentreOfMass();
  }

  aResult.SetLocation(aPnt);

  return aResult;
}

//=======================================================================
//function : GetVector
//purpose  :
//=======================================================================
gp_Vec GEOMUtils::GetVector (const TopoDS_Shape& theShape,
                             Standard_Boolean doConsiderOrientation)
{
  if (theShape.IsNull())
    Standard_NullObject::Raise("Null shape is given for a vector");

  if (theShape.ShapeType() != TopAbs_EDGE)
    Standard_TypeMismatch::Raise("Invalid shape is given, must be a vector or an edge");

  TopoDS_Edge anE = TopoDS::Edge(theShape);

  TopoDS_Vertex V1, V2;
  TopExp::Vertices(anE, V1, V2, doConsiderOrientation);

  if (V1.IsNull() || V2.IsNull())
    Standard_NullObject::Raise("Invalid edge is given, it must have two points");

  gp_Vec aV (BRep_Tool::Pnt(V1), BRep_Tool::Pnt(V2));
  if (aV.Magnitude() < gp::Resolution()) {
    Standard_ConstructionError::Raise("Vector of zero length is given");
  }

  return aV;
}

//=======================================================================
//function : CompareShapes::operator()
//purpose  : used by std::sort(), called from SortShapes()
//=======================================================================
bool GEOMUtils::CompareShapes::operator() (const TopoDS_Shape& theShape1,
                                           const TopoDS_Shape& theShape2)
{
  if (!myMap.IsBound(theShape1)) {
    myMap.Bind(theShape1, ShapeToDouble(theShape1, myIsOldSorting));
  }

  if (!myMap.IsBound(theShape2)) {
    myMap.Bind(theShape2, ShapeToDouble(theShape2, myIsOldSorting));
  }

  std::pair<double, double> val1 = myMap.Find(theShape1);
  std::pair<double, double> val2 = myMap.Find(theShape2);

  double tol = Precision::Confusion();
  bool exchange = Standard_False;

  double dMidXYZ = val1.first - val2.first;
  if (dMidXYZ >= tol) {
    exchange = Standard_True;
  }
  else if (Abs(dMidXYZ) < tol) {
    double dLength = val1.second - val2.second;
    if (dLength >= tol) {
      exchange = Standard_True;
    }
    else if (Abs(dLength) < tol && theShape1.ShapeType() <= TopAbs_FACE) {
      // PAL17233
      // equal values possible on shapes such as two halves of a sphere and
      // a membrane inside the sphere
      Bnd_Box box1,box2;
      BRepBndLib::Add(theShape1, box1);
      if (!box1.IsVoid()) {
        BRepBndLib::Add(theShape2, box2);
        Standard_Real dSquareExtent = box1.SquareExtent() - box2.SquareExtent();
        if (dSquareExtent >= tol) {
          exchange = Standard_True;
        }
        else if (Abs(dSquareExtent) < tol) {
          Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax, val1, val2;
          box1.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
          val1 = (aXmin+aXmax)*999.0 + (aYmin+aYmax)*99.0 + (aZmin+aZmax)*0.9;
          box2.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
          val2 = (aXmin+aXmax)*999.0 + (aYmin+aYmax)*99.0 + (aZmin+aZmax)*0.9;
          if ((val1 - val2) >= tol) {
            exchange = Standard_True;
          }
        }
      }
    }
  }

  //return val1 < val2;
  return !exchange;
}

//=======================================================================
//function : SortShapes
//purpose  :
//=======================================================================
void GEOMUtils::SortShapes (TopTools_ListOfShape& SL,
                            const Standard_Boolean isOldSorting)
{
#ifdef STD_SORT_ALGO
  std::vector<TopoDS_Shape> aShapesVec;
  aShapesVec.reserve(SL.Extent());

  TopTools_ListIteratorOfListOfShape it (SL);
  for (; it.More(); it.Next()) {
    aShapesVec.push_back(it.Value());
  }
  SL.Clear();

  CompareShapes shComp (isOldSorting);
  std::stable_sort(aShapesVec.begin(), aShapesVec.end(), shComp);
  //std::sort(aShapesVec.begin(), aShapesVec.end(), shComp);

  std::vector<TopoDS_Shape>::const_iterator anIter = aShapesVec.begin();
  for (; anIter != aShapesVec.end(); ++anIter) {
    SL.Append(*anIter);
  }
#else
  // old implementation
  Standard_Integer MaxShapes = SL.Extent();
  TopTools_Array1OfShape  aShapes (1,MaxShapes);
  TColStd_Array1OfInteger OrderInd(1,MaxShapes);
  TColStd_Array1OfReal    MidXYZ  (1,MaxShapes); //X,Y,Z;
  TColStd_Array1OfReal    Length  (1,MaxShapes); //X,Y,Z;

  // Computing of CentreOfMass
  Standard_Integer Index;
  GProp_GProps GPr;
  gp_Pnt GPoint;
  TopTools_ListIteratorOfListOfShape it(SL);
  for (Index=1;  it.More();  Index++)
  {
    TopoDS_Shape S = it.Value();
    SL.Remove( it ); // == it.Next()
    aShapes(Index) = S;
    OrderInd.SetValue (Index, Index);
    if (S.ShapeType() == TopAbs_VERTEX) {
      GPoint = BRep_Tool::Pnt( TopoDS::Vertex( S ));
      Length.SetValue( Index, (Standard_Real) S.Orientation());
    }
    else {
      // BEGIN: fix for Mantis issue 0020842
      if (isOldSorting) {
        BRepGProp::LinearProperties (S, GPr);
      }
      else {
        if (S.ShapeType() == TopAbs_EDGE || S.ShapeType() == TopAbs_WIRE) {
          BRepGProp::LinearProperties (S, GPr);
        }
        else if (S.ShapeType() == TopAbs_FACE || S.ShapeType() == TopAbs_SHELL) {
          BRepGProp::SurfaceProperties(S, GPr);
        }
        else {
          BRepGProp::VolumeProperties(S, GPr);
        }
      }
      // END: fix for Mantis issue 0020842
      GPoint = GPr.CentreOfMass();
      Length.SetValue(Index, GPr.Mass());
    }
    MidXYZ.SetValue(Index, GPoint.X()*999.0 + GPoint.Y()*99.0 + GPoint.Z()*0.9);
    //cout << Index << " L: " << Length(Index) << "CG: " << MidXYZ(Index) << endl;
  }

  // Sorting
  Standard_Integer aTemp;
  Standard_Boolean exchange, Sort = Standard_True;
  Standard_Real    tol = Precision::Confusion();
  while (Sort)
  {
    Sort = Standard_False;
    for (Index=1; Index < MaxShapes; Index++)
    {
      exchange = Standard_False;
      Standard_Real dMidXYZ = MidXYZ(OrderInd(Index)) - MidXYZ(OrderInd(Index+1));
      Standard_Real dLength = Length(OrderInd(Index)) - Length(OrderInd(Index+1));
      if ( dMidXYZ >= tol ) {
//         cout << "MidXYZ: " << MidXYZ(OrderInd(Index))<< " > " <<MidXYZ(OrderInd(Index+1))
//              << " d: " << dMidXYZ << endl;
        exchange = Standard_True;
      }
      else if ( Abs(dMidXYZ) < tol && dLength >= tol ) {
//         cout << "Length: " << Length(OrderInd(Index))<< " > " <<Length(OrderInd(Index+1))
//              << " d: " << dLength << endl;
        exchange = Standard_True;
      }
      else if ( Abs(dMidXYZ) < tol && Abs(dLength) < tol &&
                aShapes(OrderInd(Index)).ShapeType() <= TopAbs_FACE) {
        // PAL17233
        // equal values possible on shapes such as two halves of a sphere and
        // a membrane inside the sphere
        Bnd_Box box1,box2;
        BRepBndLib::Add( aShapes( OrderInd(Index) ), box1 );
        if ( box1.IsVoid() ) continue;
        BRepBndLib::Add( aShapes( OrderInd(Index+1) ), box2 );
        Standard_Real dSquareExtent = box1.SquareExtent() - box2.SquareExtent();
        if ( dSquareExtent >= tol ) {
//           cout << "SquareExtent: " << box1.SquareExtent()<<" > "<<box2.SquareExtent() << endl;
          exchange = Standard_True;
        }
        else if ( Abs(dSquareExtent) < tol ) {
          Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax, val1, val2;
          box1.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
          val1 = (aXmin+aXmax)*999 + (aYmin+aYmax)*99 + (aZmin+aZmax)*0.9;
          box2.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
          val2 = (aXmin+aXmax)*999 + (aYmin+aYmax)*99 + (aZmin+aZmax)*0.9;
          //exchange = val1 > val2;
          if ((val1 - val2) >= tol) {
            exchange = Standard_True;
          }
          //cout << "box: " << val1<<" > "<<val2 << endl;
        }
      }

      if (exchange)
      {
//         cout << "exchange " << Index << " & " << Index+1 << endl;
        aTemp = OrderInd(Index);
        OrderInd(Index) = OrderInd(Index+1);
        OrderInd(Index+1) = aTemp;
        Sort = Standard_True;
      }
    }
  }

  for (Index=1; Index <= MaxShapes; Index++)
    SL.Append( aShapes( OrderInd(Index) ));
#endif
}

//=======================================================================
//function : CompsolidToCompound
//purpose  :
//=======================================================================
TopoDS_Shape GEOMUtils::CompsolidToCompound (const TopoDS_Shape& theCompsolid)
{
  if (theCompsolid.ShapeType() != TopAbs_COMPSOLID) {
    return theCompsolid;
  }

  TopoDS_Compound aCompound;
  BRep_Builder B;
  B.MakeCompound(aCompound);

  TopTools_MapOfShape mapShape;
  TopoDS_Iterator It (theCompsolid, Standard_True, Standard_True);

  for (; It.More(); It.Next()) {
    TopoDS_Shape aShape_i = It.Value();
    if (mapShape.Add(aShape_i)) {
      B.Add(aCompound, aShape_i);
    }
  }

  return TopoDS_Shape(std::move(aCompound));
}

//=======================================================================
//function : AddSimpleShapes
//purpose  :
//=======================================================================
void GEOMUtils::AddSimpleShapes (const TopoDS_Shape& theShape, TopTools_ListOfShape& theList)
{
  if (theShape.ShapeType() != TopAbs_COMPOUND &&
      theShape.ShapeType() != TopAbs_COMPSOLID) {
    theList.Append(theShape);
    return;
  }

  TopTools_MapOfShape mapShape;
  TopoDS_Iterator It (theShape, Standard_True, Standard_True);

  for (; It.More(); It.Next()) {
    TopoDS_Shape aShape_i = It.Value();
    if (mapShape.Add(aShape_i)) {
      if (aShape_i.ShapeType() == TopAbs_COMPOUND ||
          aShape_i.ShapeType() == TopAbs_COMPSOLID) {
        AddSimpleShapes(aShape_i, theList);
      } else {
        theList.Append(aShape_i);
      }
    }
  }
}

//=======================================================================
//function : CheckTriangulation
//purpose  :
//=======================================================================
bool GEOMUtils::CheckTriangulation (const TopoDS_Shape& aShape)
{
  bool isTriangulation = true;

  TopExp_Explorer exp (aShape, TopAbs_FACE);
  if (exp.More())
  {
    TopLoc_Location aTopLoc;
    Handle(Poly_Triangulation) aTRF;
    aTRF = BRep_Tool::Triangulation(TopoDS::Face(exp.Current()), aTopLoc);
    if (aTRF.IsNull()) {
      isTriangulation = false;
    }
  }
  else // no faces, try edges
  {
    TopExp_Explorer expe (aShape, TopAbs_EDGE);
    if (!expe.More()) {
      return false;
    }
    TopLoc_Location aLoc;
    Handle(Poly_Polygon3D) aPE = BRep_Tool::Polygon3D(TopoDS::Edge(expe.Current()), aLoc);
    if (aPE.IsNull()) {
      isTriangulation = false;
    }
  }

  if (!isTriangulation) {
    // calculate deflection
    Standard_Real aDeviationCoefficient = 0.001;

    Bnd_Box B;
    BRepBndLib::Add(aShape, B);
    Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
    B.Get(aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);

    Standard_Real dx = aXmax - aXmin, dy = aYmax - aYmin, dz = aZmax - aZmin;
    Standard_Real aDeflection = Max(Max(dx, dy), dz) * aDeviationCoefficient * 4;
    Standard_Real aHLRAngle = 0.349066;

    BRepMesh_IncrementalMesh Inc (aShape, aDeflection, Standard_False, aHLRAngle);
  }

  return true;
}

//=======================================================================
//function : GetTypeOfSimplePart
//purpose  :
//=======================================================================
TopAbs_ShapeEnum GEOMUtils::GetTypeOfSimplePart (const TopoDS_Shape& theShape)
{
  TopAbs_ShapeEnum aType = theShape.ShapeType();
  if      (aType == TopAbs_VERTEX)                             return TopAbs_VERTEX;
  else if (aType == TopAbs_EDGE  || aType == TopAbs_WIRE)      return TopAbs_EDGE;
  else if (aType == TopAbs_FACE  || aType == TopAbs_SHELL)     return TopAbs_FACE;
  else if (aType == TopAbs_SOLID || aType == TopAbs_COMPSOLID) return TopAbs_SOLID;
  else if (aType == TopAbs_COMPOUND) {
    // Only the iType of the first shape in the compound is taken into account
    TopoDS_Iterator It (theShape, Standard_False, Standard_False);
    if (It.More()) {
      return GetTypeOfSimplePart(It.Value());
    }
  }
  return TopAbs_SHAPE;
}

//=======================================================================
//function : GetEdgeNearPoint
//purpose  :
//=======================================================================
TopoDS_Shape GEOMUtils::GetEdgeNearPoint (const TopoDS_Shape& theShape,
                                          const TopoDS_Vertex& thePoint)
{
  TopoDS_Shape aResult;

  // 1. Explode the shape on edges
  TopTools_MapOfShape mapShape;
  Standard_Integer nbEdges = 0;
  TopExp_Explorer exp (theShape, TopAbs_EDGE);
  for (; exp.More(); exp.Next()) {
    if (mapShape.Add(exp.Current())) {
      nbEdges++;
    }
  }

  if (nbEdges == 0)
    Standard_NullObject::Raise("Given shape contains no edges");

  mapShape.Clear();
  Standard_Integer ind = 1;
  TopTools_Array1OfShape anEdges (1, nbEdges);
  TColStd_Array1OfReal aDistances (1, nbEdges);
  for (exp.Init(theShape, TopAbs_EDGE); exp.More(); exp.Next()) {
    if (mapShape.Add(exp.Current())) {
      TopoDS_Shape anEdge = exp.Current();
      anEdges(ind) = anEdge;

      // 2. Classify the point relatively each edge
      BRepExtrema_DistShapeShape aDistTool (thePoint, anEdges(ind));
      if (!aDistTool.IsDone())
        Standard_ConstructionError::Raise("Cannot find a distance from the given point to one of edges");

      aDistances(ind) = aDistTool.Value();
      ind++;
    }
  }

  // 3. Define edge, having minimum distance to the point
  Standard_Real nearest = RealLast(), nbFound = 0;
  Standard_Real prec = Precision::Confusion();
  for (ind = 1; ind <= nbEdges; ind++) {
    if (Abs(aDistances(ind) - nearest) < prec) {
      nbFound++;
    }
    else if (aDistances(ind) < nearest) {
      nearest = aDistances(ind);
      aResult = anEdges(ind);
      nbFound = 1;
    }
    else {
    }
  }
  if (nbFound > 1) {
    Standard_ConstructionError::Raise("Multiple edges near the given point are found");
  }
  else if (nbFound == 0) {
    Standard_ConstructionError::Raise("There are no edges near the given point");
  }
  else {
  }

  return aResult;
}

//=======================================================================
//function : PreciseBoundingBox
//purpose  : 
//=======================================================================
Standard_Boolean GEOMUtils::PreciseBoundingBox
                          (const TopoDS_Shape &theShape, Bnd_Box &theBox)
{
  if ( theBox.IsVoid() ) BRepBndLib::Add( theShape, theBox );
  if ( theBox.IsVoid() ) return Standard_False;

  Standard_Real aBound[6];
  theBox.Get(aBound[0], aBound[2], aBound[4], aBound[1], aBound[3], aBound[5]);

  Standard_Integer i;
  const gp_Pnt aMid(0.5*(aBound[1] + aBound[0]),  // XMid
                    0.5*(aBound[3] + aBound[2]),  // YMid
                    0.5*(aBound[5] + aBound[4])); // ZMid
  const gp_XYZ aSize(aBound[1] - aBound[0],       // DX
                     aBound[3] - aBound[2],       // DY
                     aBound[5] - aBound[4]);      // DZ
  const gp_Pnt aPnt[6] =
    {
      gp_Pnt(aBound[0] - (aBound[1] - aBound[0]), aMid.Y(), aMid.Z()), // XMin
      gp_Pnt(aBound[1] + (aBound[1] - aBound[0]), aMid.Y(), aMid.Z()), // XMax
      gp_Pnt(aMid.X(), aBound[2] - (aBound[3] - aBound[2]), aMid.Z()), // YMin
      gp_Pnt(aMid.X(), aBound[3] + (aBound[3] - aBound[2]), aMid.Z()), // YMax
      gp_Pnt(aMid.X(), aMid.Y(), aBound[4] - (aBound[5] - aBound[4])), // ZMin
      gp_Pnt(aMid.X(), aMid.Y(), aBound[5] + (aBound[5] - aBound[4]))  // ZMax
    };
  const gp_Dir aDir[3] = { gp::DX(), gp::DY(), gp::DZ() };
  const Standard_Real aPlnSize[3] =
    {
      0.5*Max(aSize.Y(), aSize.Z()), // XMin, XMax planes
      0.5*Max(aSize.X(), aSize.Z()), // YMin, YMax planes
      0.5*Max(aSize.X(), aSize.Y())  // ZMin, ZMax planes
    };
  gp_Pnt aPMin[2];

  for (i = 0; i < 6; i++) {
    const Standard_Integer iHalf = i/2;
    const gp_Pln aPln(aPnt[i], aDir[iHalf]);
    BRepBuilderAPI_MakeFace aMkFace(aPln, -aPlnSize[iHalf], aPlnSize[iHalf],
                                    -aPlnSize[iHalf], aPlnSize[iHalf]);

    if (!aMkFace.IsDone()) {
      return Standard_False;
    }

    TopoDS_Shape aFace = aMkFace.Shape();

    // Get minimal distance between planar face and shape.
    Standard_Real aMinDist =
      GEOMUtils::GetMinDistance(aFace, theShape, aPMin[0], aPMin[1]);

    if (aMinDist < 0.) {
      return Standard_False;
    }

    aBound[i] = aPMin[1].Coord(iHalf + 1);
  }

  // Update Bounding box with the new values.
  theBox.SetVoid();
  theBox.Update(aBound[0], aBound[2], aBound[4], aBound[1], aBound[3], aBound[5]);

  return Standard_True;
}

//=======================================================================
//function : GetMinDistanceSingular
//purpose  : 
//=======================================================================
double GEOMUtils::GetMinDistanceSingular(const TopoDS_Shape& aSh1,
                                         const TopoDS_Shape& aSh2,
                                         gp_Pnt& Ptmp1, gp_Pnt& Ptmp2)
{
  TopoDS_Shape     tmpSh1;
  TopoDS_Shape     tmpSh2;
  Standard_Real    AddDist1 = 0.;
  Standard_Real    AddDist2 = 0.;
  Standard_Boolean IsChange1 = ModifyShape(aSh1, tmpSh1, AddDist1);
  Standard_Boolean IsChange2 = ModifyShape(aSh2, tmpSh2, AddDist2);

  if( !IsChange1 && !IsChange2 )
    return -2.0;

  BRepExtrema_DistShapeShape dst(tmpSh1,tmpSh2);
  if (dst.IsDone()) {
    double MinDist = 1.e9;
    gp_Pnt PMin1, PMin2, P1, P2;
    for (int i = 1; i <= dst.NbSolution(); i++) {
      P1 = dst.PointOnShape1(i);
      P2 = dst.PointOnShape2(i);
      Standard_Real Dist = P1.Distance(P2);
      if (MinDist > Dist) {
        MinDist = Dist;
        PMin1 = P1;
        PMin2 = P2;
      }
    }
    if(MinDist<1.e-7) {
      Ptmp1 = PMin1;
      Ptmp2 = PMin2;
    }
    else {
      gp_Dir aDir(gp_Vec(PMin1,PMin2));
      if( MinDist > (AddDist1+AddDist2) ) {
        Ptmp1 = gp_Pnt( PMin1.X() + aDir.X()*AddDist1,
                        PMin1.Y() + aDir.Y()*AddDist1,
                        PMin1.Z() + aDir.Z()*AddDist1 );
        Ptmp2 = gp_Pnt( PMin2.X() - aDir.X()*AddDist2,
                        PMin2.Y() - aDir.Y()*AddDist2,
                        PMin2.Z() - aDir.Z()*AddDist2 );
        return (MinDist - AddDist1 - AddDist2);
      }
      else {
        if( AddDist1 > 0 ) {
          Ptmp1 = gp_Pnt( PMin1.X() + aDir.X()*AddDist1,
                          PMin1.Y() + aDir.Y()*AddDist1,
                          PMin1.Z() + aDir.Z()*AddDist1 );
          Ptmp2 = Ptmp1;
        }
        else {
          Ptmp2 = gp_Pnt( PMin2.X() - aDir.X()*AddDist2,
                          PMin2.Y() - aDir.Y()*AddDist2,
                          PMin2.Z() - aDir.Z()*AddDist2 );
          Ptmp1 = Ptmp2;
        }
      }
    }
    double res = MinDist - AddDist1 - AddDist2;
    if(res<0.) res = 0.0;
    return res;
  }
  return -2.0;
}

//=======================================================================
//function : GetMinDistance
//purpose  : 
//=======================================================================
Standard_Real GEOMUtils::GetMinDistance
                               (const TopoDS_Shape& theShape1,
                                const TopoDS_Shape& theShape2,
                                gp_Pnt& thePnt1, gp_Pnt& thePnt2)
{
  Standard_Real aResult = 1.e9;

  // Issue 0020231: A min distance bug with torus and vertex.
  // Make GetMinDistance() return zero if a sole VERTEX is inside any of SOLIDs

  // which of shapes consists of only one vertex?
  TopExp_Explorer exp1(theShape1,TopAbs_VERTEX), exp2(theShape2,TopAbs_VERTEX);
  TopoDS_Shape V1 = exp1.More() ? exp1.Current() : TopoDS_Shape();
  TopoDS_Shape V2 = exp2.More() ? exp2.Current() : TopoDS_Shape();
  exp1.Next(); exp2.Next();
  if ( exp1.More() ) V1.Nullify();
  if ( exp2.More() ) V2.Nullify();
  // vertex and container of solids
  TopoDS_Shape V = V1.IsNull() ? V2 : V1;
  TopoDS_Shape S = V1.IsNull() ? theShape1 : theShape2;
  if ( !V.IsNull() ) {
    // classify vertex against solids
    gp_Pnt p = BRep_Tool::Pnt( TopoDS::Vertex( V ) );
    for ( exp1.Init( S, TopAbs_SOLID ); exp1.More(); exp1.Next() ) {
      BRepClass3d_SolidClassifier classifier( exp1.Current(), p, 1e-6);
      if ( classifier.State() == TopAbs_IN ) {
        thePnt1 = p;
        thePnt2 = p;
        return 0.0;
      }
    }
  }
  // End Issue 0020231

  // skl 30.06.2008
  // additional workaround for bugs 19899, 19908 and 19910 from Mantis
  double dist = GEOMUtils::GetMinDistanceSingular
      (theShape1, theShape2, thePnt1, thePnt2);

  if (dist > -1.0) {
    return dist;
  }

  BRepExtrema_DistShapeShape dst (theShape1, theShape2);
  if (dst.IsDone()) {
    gp_Pnt P1, P2;

    for (int i = 1; i <= dst.NbSolution(); i++) {
      P1 = dst.PointOnShape1(i);
      P2 = dst.PointOnShape2(i);

      Standard_Real Dist = P1.Distance(P2);
      if (aResult > Dist) {
        aResult = Dist;
        thePnt1 = P1;
        thePnt2 = P2;
      }
    }
  }

  return aResult;
}

//=======================================================================
// function : ConvertClickToPoint()
// purpose  : Returns the point clicked in 3D view
//=======================================================================
// vejmarie ISSUE
gp_Pnt GEOMUtils::ConvertClickToPoint( int x, int y, Handle(V3d_View) aView )
{
/*
  V3d_Coordinate XEye, YEye, ZEye, XAt, YAt, ZAt;
  aView->Eye( XEye, YEye, ZEye );

  aView->At( XAt, YAt, ZAt );
  gp_Pnt EyePoint( XEye, YEye, ZEye );
  gp_Pnt AtPoint( XAt, YAt, ZAt );

  gp_Vec EyeVector( EyePoint, AtPoint );
  gp_Dir EyeDir( EyeVector );

  gp_Pln PlaneOfTheView = gp_Pln( AtPoint, EyeDir );
  Standard_Real X, Y, Z;
  //aView->Convert( x, y, X, Y, Z );
  gp_Pnt ConvertedPoint( X, Y, Z );

  gp_Pnt2d ConvertedPointOnPlane = ProjLib::Project( PlaneOfTheView, ConvertedPoint );
  gp_Pnt ResultPoint = ElSLib::Value( ConvertedPointOnPlane.X(), ConvertedPointOnPlane.Y(), PlaneOfTheView );
  return ResultPoint;
*/
  return gp_Pnt(0,0,0);
}

//=======================================================================
// function : ConvertTreeToString()
// purpose  : Returns the string representation of dependency tree
//=======================================================================
void GEOMUtils::ConvertTreeToString( const TreeModel& tree,
                                     std::string& dependencyStr )
{
  TreeModel::const_iterator i;
  for ( i = tree.begin(); i != tree.end(); ++i ) {
    dependencyStr.append( i->first );
    dependencyStr.append( "-" );
    std::vector<LevelInfo> upLevelList = i->second.first;
    dependencyStr.append( "upward" );
    parseWard( upLevelList, dependencyStr );
    std::vector<LevelInfo> downLevelList = i->second.second;
    dependencyStr.append( "downward" );
    parseWard( downLevelList, dependencyStr );
  }
}

//=======================================================================
// function : ConvertStringToTree()
// purpose  : Returns the dependency tree
//=======================================================================
void GEOMUtils::ConvertStringToTree( const std::string& dependencyStr,
                                     TreeModel& tree )
{
  std::size_t cursor = 0;

  while( dependencyStr.find('-',cursor) != std::string::npos ) //find next selected object
  {
    std::size_t objectIndex = dependencyStr.find( '-', cursor );
    std::string objectEntry = dependencyStr.substr( cursor, objectIndex - cursor );
    cursor = objectIndex;

    std::size_t upwardIndexBegin = dependencyStr.find("{",cursor) + 1;
    std::size_t upwardIndexFinish = dependencyStr.find("}",upwardIndexBegin);
    LevelsList upwardList = parseWard( dependencyStr, cursor );

    LevelsList downwardList = parseWard( dependencyStr, cursor );

    tree[objectEntry] = std::pair<LevelsList,LevelsList>( upwardList, downwardList );
  }
}

bool GEOMUtils::CheckShape( TopoDS_Shape& shape,
                            bool checkGeometry )
{
  BRepCheck_Analyzer analyzer( shape, checkGeometry );
  return analyzer.IsValid();
}

bool GEOMUtils::FixShapeTolerance( TopoDS_Shape& shape,
                                   TopAbs_ShapeEnum type,
                                   Standard_Real tolerance,
                                   bool checkGeometry )
{
  ShapeFix_ShapeTolerance aSft;
  aSft.LimitTolerance( shape, tolerance, tolerance, type );
  Handle(ShapeFix_Shape) aSfs = new ShapeFix_Shape( shape );
  aSfs->Perform();
  shape = aSfs->Shape();
  return CheckShape( shape, checkGeometry );
}

bool GEOMUtils::FixShapeTolerance( TopoDS_Shape& shape,
                                   Standard_Real tolerance,
                                   bool checkGeometry )
{
  return FixShapeTolerance( shape, TopAbs_SHAPE, tolerance, checkGeometry );
}

bool GEOMUtils::FixShapeTolerance( TopoDS_Shape& shape,
                                   bool checkGeometry )
{
  return FixShapeTolerance( shape, Precision::Confusion(), checkGeometry );
}

bool GEOMUtils::FixShapeCurves( TopoDS_Shape& shape )
{
  Standard_Real aT, aTolE, aD, aDMax = 0.0;
  TopExp_Explorer aExpF, aExpE;
  NCollection_DataMap<TopoDS_Edge, Standard_Real, TopTools_ShapeMapHasher> aDMETol;
  aExpF.Init(shape, TopAbs_FACE);
  for (; aExpF.More(); aExpF.Next()) {
    const TopoDS_Face& aF = *(TopoDS_Face*)&aExpF.Current();
    aExpE.Init(aF, TopAbs_EDGE);
    for (; aExpE.More(); aExpE.Next()) {
      const TopoDS_Edge& aE = *(TopoDS_Edge*)&aExpE.Current();
      try {
#if OCC_VERSION_HEX >= 0x060800
        if (!BOPTools_AlgoTools::ComputeTolerance(aF, aE, aDMax, aT)) {
          continue;
        }
#endif
      }
      catch(...) {
        continue;
      }
      aTolE = BRep_Tool::Tolerance(aE);
      if (aDMax < aTolE) continue;
      if (aDMETol.IsBound(aE)) {
        aD = aDMETol.Find(aE);
        if (aDMax > aD) {
          aDMETol.UnBind(aE);
          aDMETol.Bind(aE, aDMax);
        }
      }
      else {
        aDMETol.Bind(aE, aDMax);
      }
    }
  }
  NCollection_DataMap<TopoDS_Edge, Standard_Real, TopTools_ShapeMapHasher>::Iterator aDMETolIt(aDMETol);
#ifdef USE_LIMIT_TOLERANCE
  ShapeFix_ShapeTolerance sat;
#else
  BRep_Builder b;
#endif
  for (; aDMETolIt.More(); aDMETolIt.Next()) {
#ifdef USE_LIMIT_TOLERANCE
    sat.LimitTolerance(aDMETolIt.Key(), aDMETolIt.Value()*1.001);
#else
    TopoDS_Iterator itv(aDMETolIt.Key());
    for (; itv.More(); itv.Next())
      b.UpdateVertex(TopoDS::Vertex(itv.Value()), aDMETolIt.Value()*1.001);
    b.UpdateEdge(aDMETolIt.Key(), aDMETolIt.Value()*1.001);
#endif
  }
  return CheckShape( shape );
}

bool GEOMUtils::Write( const TopoDS_Shape& shape, const char* fileName )
{
  return BRepTools::Write( shape, fileName );
}

TopoDS_Shape GEOMUtils::ReduceCompound( const TopoDS_Shape& shape )
{
  TopoDS_Shape result = shape;

  if ( shape.ShapeType() == TopAbs_COMPOUND ||
       shape.ShapeType() == TopAbs_COMPSOLID ) {

    TopTools_ListOfShape l;
    
    TopoDS_Iterator it ( shape );
    for ( ; it.More(); it.Next() )
      l.Append( it.Value() );
    if ( l.Extent() == 1 && l.First() != shape )
      result = ReduceCompound( l.First() );
  }
  
  return result;
}

void GEOMUtils::MeshShape( const TopoDS_Shape shape,
                           double deflection, bool theForced )
{
  Standard_Real aDeflection = ( deflection <= 0 ) ? DefaultDeflection() : deflection;
  
  // Is shape triangulated?
  Standard_Boolean alreadyMeshed = true;
  TopExp_Explorer ex;
  TopLoc_Location aLoc;
  for ( ex.Init( shape, TopAbs_FACE ); ex.More() && alreadyMeshed; ex.Next() ) {
    const TopoDS_Face& aFace = TopoDS::Face( ex.Current() );
    Handle(Poly_Triangulation) aPoly = BRep_Tool::Triangulation( aFace, aLoc );
    alreadyMeshed = !aPoly.IsNull(); 
  }
  
  if ( !alreadyMeshed || theForced ) {
    // Compute bounding box
    Bnd_Box B;
    BRepBndLib::Add( shape, B );
    if ( B.IsVoid() )
      return; // NPAL15983 (Bug when displaying empty groups) 
    Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
    B.Get( aXmin, aYmin, aZmin, aXmax, aYmax, aZmax );
    
    // This magic line comes from Prs3d_ShadedShape.gxx in OCCT
    aDeflection = MAX3(aXmax-aXmin, aYmax-aYmin, aZmax-aZmin) * aDeflection * 4;
    
    // Clean triangulation before compute incremental mesh
    BRepTools::Clean( shape );
    
    // Compute triangulation
    BRepMesh_IncrementalMesh mesh( shape, aDeflection ); 
  }
}

double GEOMUtils::DefaultDeflection()
{
  return 0.001;
}

//=======================================================================
//function : IsOpenPath
//purpose  : 
//=======================================================================
bool GEOMUtils::IsOpenPath(const TopoDS_Shape &theShape)
{
  bool isOpen = true;

  if (theShape.IsNull() == Standard_False) {
    if (theShape.Closed()) {
      // The shape is closed
      isOpen = false;
    } else {
      const TopAbs_ShapeEnum aType = theShape.ShapeType();

      if (aType == TopAbs_EDGE || aType == TopAbs_WIRE) {
        // Check if path ends are coinsident.
        TopoDS_Vertex aV[2];

        if (aType == TopAbs_EDGE) {
          // Edge
          TopExp::Vertices(TopoDS::Edge(theShape), aV[0], aV[1]);
        } else {
          // Wire
          TopExp::Vertices(TopoDS::Wire(theShape), aV[0], aV[1]);
        }

        if (aV[0].IsNull() == Standard_False &&
            aV[1].IsNull() == Standard_False) {
          if (aV[0].IsSame(aV[1])) {
            // The shape is closed
            isOpen = false;
          } else {
            const Standard_Real aTol1 = BRep_Tool::Tolerance(aV[0]);
            const Standard_Real aTol2 = BRep_Tool::Tolerance(aV[1]);
            const gp_Pnt        aPnt1 = BRep_Tool::Pnt(aV[0]);
            const gp_Pnt        aPnt2 = BRep_Tool::Pnt(aV[1]);

            if (aPnt1.Distance(aPnt2) <= aTol1 + aTol2) {
              // The shape is closed
              isOpen = false;
            }
          }
        }
      }
    }
  }

  return isOpen;
}
