// Copyright (C) 2007-2016  CEA/DEN, EDF R&D
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

// Author : Lioka RAZAFINDRAZAKA (CEA)

#include "SMESH_ControlPnt.hxx"

#include <BRepBndLib.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_Curve.hxx>
#include <IntCurvesFace_Intersector.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS_Solid.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <set>

namespace SMESHUtils
{
  // Some functions for surface sampling
  void subdivideTriangle( const gp_Pnt& p1,
                          const gp_Pnt& p2,
                          const gp_Pnt& p3,
                          const double& theSize,
                          std::vector<ControlPnt>& thePoints );

  std::vector<gp_Pnt> computePointsForSplitting( const gp_Pnt& p1,
                                                 const gp_Pnt& p2,
                                                 const gp_Pnt& p3 );
  gp_Pnt tangencyPoint(const gp_Pnt& p1,
                       const gp_Pnt& p2,
                       const gp_Pnt& Center);

}

//================================================================================
/*!
 * \brief Fills a vector of points from which a size map input file can be written
 */
//================================================================================

void SMESHUtils::createControlPoints( const TopoDS_Shape&      theShape,
                                      const double&            theSize,
                                      std::vector<ControlPnt>& thePoints )
{
  if ( theShape.ShapeType() == TopAbs_VERTEX )
  {
    gp_Pnt aPnt = BRep_Tool::Pnt( TopoDS::Vertex(theShape) );
    ControlPnt aControlPnt( aPnt, theSize );
    thePoints.push_back( aControlPnt );
  }
  if ( theShape.ShapeType() == TopAbs_EDGE )
  {
    createPointsSampleFromEdge( TopoDS::Edge( theShape ), theSize, thePoints );
  }
  else if ( theShape.ShapeType() == TopAbs_WIRE )
  {
    TopExp_Explorer Ex;
    for (Ex.Init(theShape,TopAbs_EDGE); Ex.More(); Ex.Next())
    {
      createPointsSampleFromEdge( TopoDS::Edge( Ex.Current() ), theSize, thePoints );
    }
  }
  else if ( theShape.ShapeType() ==  TopAbs_FACE )
  {
    createPointsSampleFromFace( TopoDS::Face( theShape ), theSize, thePoints );
  }
  else if ( theShape.ShapeType() ==  TopAbs_SOLID )
  {
    createPointsSampleFromSolid( TopoDS::Solid( theShape ), theSize, thePoints );
  }
  else if ( theShape.ShapeType() == TopAbs_COMPOUND )
  {
    TopoDS_Iterator it( theShape );
    for(; it.More(); it.Next())
    {
      createControlPoints( it.Value(), theSize, thePoints );
    }
  }
}

//================================================================================
/*!
 * \brief Fills a vector of points with point samples approximately
 * \brief spaced with a given size
 */
//================================================================================

void SMESHUtils::createPointsSampleFromEdge( const TopoDS_Edge&       theEdge,
                                             const double&            theSize,
                                             std::vector<ControlPnt>& thePoints )
{
  double step = theSize;
  double first, last;
  Handle( Geom_Curve ) aCurve = BRep_Tool::Curve( theEdge, first, last );
  GeomAdaptor_Curve C ( aCurve );
  GCPnts_UniformAbscissa DiscretisationAlgo(C, step , first, last, Precision::Confusion());
  int nbPoints = DiscretisationAlgo.NbPoints();

  ControlPnt aPnt;
  aPnt.SetSize(theSize);

  for ( int i = 1; i <= nbPoints; i++ )
  {
    double param = DiscretisationAlgo.Parameter( i );
    aCurve->D0( param, aPnt );
    thePoints.push_back( aPnt );
  }
}

//================================================================================
/*!
 * \brief Fills a vector of points with point samples approximately
 * \brief spaced with a given size
 */
//================================================================================

void SMESHUtils::createPointsSampleFromFace( const TopoDS_Face&       theFace,
                                             const double&            theSize,
                                             std::vector<ControlPnt>& thePoints )
{
  BRepMesh_IncrementalMesh M(theFace, 0.01, Standard_True);
  TopLoc_Location aLocation;

  // Triangulate the face
  Handle(Poly_Triangulation) aTri = BRep_Tool::Triangulation (theFace, aLocation);

  // Get the transformation associated to the face location
  gp_Trsf aTrsf = aLocation.Transformation();

  // Get triangles
  int nbTriangles = aTri->NbTriangles();
  Poly_Array1OfTriangle triangles(1,nbTriangles);
  triangles=aTri->Triangles();

  // GetNodes
  int nbNodes = aTri->NbNodes();
  TColgp_Array1OfPnt nodes(1,nbNodes);
  nodes = aTri->Nodes();

  // Iterate on triangles and subdivide them
  for(int i=1; i<=nbTriangles; i++)
  {
    Poly_Triangle aTriangle = triangles.Value(i);
    gp_Pnt p1 = nodes.Value(aTriangle.Value(1));
    gp_Pnt p2 = nodes.Value(aTriangle.Value(2));
    gp_Pnt p3 = nodes.Value(aTriangle.Value(3));

    p1.Transform(aTrsf);
    p2.Transform(aTrsf);
    p3.Transform(aTrsf);

    subdivideTriangle(p1, p2, p3, theSize, thePoints);
  }
}

//================================================================================
/*!
 * \brief Fills a vector of points with point samples approximately
 * \brief spaced with a given size
 */
//================================================================================

void SMESHUtils::createPointsSampleFromSolid( const TopoDS_Solid&      theSolid,
                                              const double&            theSize,
                                              std::vector<ControlPnt>& thePoints )
{
  // Compute the bounding box
  double Xmin, Ymin, Zmin, Xmax, Ymax, Zmax;
  Bnd_Box B;
  BRepBndLib::Add(theSolid, B);
  B.Get(Xmin, Ymin, Zmin, Xmax, Ymax, Zmax);

  // Create the points
  double step = theSize;

  for ( double x=Xmin; x-Xmax<Precision::Confusion(); x=x+step )
  {
    for ( double y=Ymin; y-Ymax<Precision::Confusion(); y=y+step )
    {
      // Step1 : generate the Zmin -> Zmax line
      gp_Pnt startPnt(x, y, Zmin);
      gp_Pnt endPnt(x, y, Zmax);
      gp_Vec aVec(startPnt, endPnt);
      gp_Lin aLine(startPnt, aVec);
      double endParam = Zmax - Zmin;

      // Step2 : for each face of theSolid:
      std::set<double> intersections;
      std::set<double>::iterator it = intersections.begin();

      TopExp_Explorer Ex;
      for (Ex.Init(theSolid,TopAbs_FACE); Ex.More(); Ex.Next())
      {
        // check if there is an intersection
        IntCurvesFace_Intersector anIntersector(TopoDS::Face(Ex.Current()), Precision::Confusion());
        anIntersector.Perform(aLine, 0, endParam);

        // get the intersection's parameter and store it
        int nbPoints = anIntersector.NbPnt();
        for(int i = 0 ; i < nbPoints ; i++ )
        {
          it = intersections.insert( it, anIntersector.WParameter(i+1) );
        }
      }
      // Step3 : go through the line chunk by chunk
      if ( intersections.begin() != intersections.end() )
      {
        std::set<double>::iterator intersectionsIterator=intersections.begin();
        double first = *intersectionsIterator;
        intersectionsIterator++;
        bool innerPoints = true;
        for ( ; intersectionsIterator!=intersections.end() ; intersectionsIterator++ )
        {
          double second = *intersectionsIterator;
          if ( innerPoints )
          {
            // If the last chunk was outside of the shape or this is the first chunk
            // add the points in the range [first, second] to the points vector
            double localStep = (second -first) / ceil( (second - first) / step );
            for ( double z = Zmin + first; z < Zmin + second; z = z + localStep )
            {
              thePoints.push_back(ControlPnt( x, y, z, theSize ));
            }
            thePoints.push_back(ControlPnt( x, y, Zmin + second, theSize ));
          }
          first = second;
          innerPoints = !innerPoints;
        }
      }
    }
  }
}

//================================================================================
/*!
 * \brief Subdivides a triangle until it reaches a certain size (recursive function)
 */
//================================================================================

void SMESHUtils::subdivideTriangle( const gp_Pnt& p1,
                                    const gp_Pnt& p2,
                                    const gp_Pnt& p3,
                                    const double& theSize,
                                    std::vector<ControlPnt>& thePoints)
{
  // Size threshold to stop subdividing
  // This value ensures that two control points are distant no more than 2*theSize
  // as shown below
  //
  // The greater distance D of the mass center M to each Edge is 1/3 * Median
  // and Median < sqrt(3/4) * a  where a is the greater side (by using Apollonius' thorem).
  // So D < 1/3 * sqrt(3/4) * a and if a < sqrt(3) * S then D < S/2
  // and the distance between two mass centers of two neighbouring triangles
  // sharing an edge is < 2 * 1/2 * S = S
  // If the traingles share a Vertex and no Edge the distance of the mass centers
  // to the Vertices is 2*D < S so the mass centers are distant of less than 2*S 

  double threshold = sqrt( 3. ) * theSize;

  if ( (p1.Distance(p2) > threshold ||
        p2.Distance(p3) > threshold ||
        p3.Distance(p1) > threshold))
  {
    std::vector<gp_Pnt> midPoints = computePointsForSplitting(p1, p2, p3);

    subdivideTriangle( midPoints[0], midPoints[1], midPoints[2], theSize, thePoints );
    subdivideTriangle( midPoints[0], p2, midPoints[1], theSize, thePoints );
    subdivideTriangle( midPoints[2], midPoints[1], p3, theSize, thePoints );
    subdivideTriangle( p1, midPoints[0], midPoints[2], theSize, thePoints );
  }
  else
  {
    double x = (p1.X() + p2.X() + p3.X()) / 3 ;
    double y = (p1.Y() + p2.Y() + p3.Y()) / 3 ;
    double z = (p1.Z() + p2.Z() + p3.Z()) / 3 ;

    ControlPnt massCenter( x ,y ,z, theSize );
    thePoints.push_back( massCenter );
  }
}

//================================================================================
/*!
 * \brief Returns the appropriate points for splitting a triangle
 * \brief the tangency points of the incircle are used in order to have mostly
 * \brief well-shaped sub-triangles
 */
//================================================================================

std::vector<gp_Pnt> SMESHUtils::computePointsForSplitting( const gp_Pnt& p1,
                                                           const gp_Pnt& p2,
                                                           const gp_Pnt& p3 )
{
  std::vector<gp_Pnt> midPoints;
  //Change coordinates
  gp_Trsf Trsf_1;            // Identity transformation
  gp_Ax3 reference_system(gp::Origin(), gp::DZ(), gp::DX());   // OXY

  gp_Vec Vx(p1, p3);
  gp_Vec Vaux(p1, p2);
  gp_Dir Dx(Vx);
  gp_Dir Daux(Vaux);
  gp_Dir Dz = Dx.Crossed(Daux);
  gp_Ax3 current_system(p1, Dz, Dx);

  Trsf_1.SetTransformation( reference_system, current_system );

  gp_Pnt A = p1.Transformed(Trsf_1);
  gp_Pnt B = p2.Transformed(Trsf_1);
  gp_Pnt C = p3.Transformed(Trsf_1);

  double a =  B.Distance(C) ;
  double b =  A.Distance(C) ;
  double c =  B.Distance(A) ;

  // Incenter coordinates
  // see http://mathworld.wolfram.com/Incenter.html
  double Xi = ( b*B.X() + c*C.X() ) / ( a + b + c );
  double Yi = ( b*B.Y() ) / ( a + b + c );
  gp_Pnt Center(Xi, Yi, 0);

  // Calculate the tangency points of the incircle
  gp_Pnt T1 = tangencyPoint( A, B, Center);
  gp_Pnt T2 = tangencyPoint( B, C, Center);
  gp_Pnt T3 = tangencyPoint( C, A, Center);

  gp_Pnt p1_2 = T1.Transformed(Trsf_1.Inverted());
  gp_Pnt p2_3 = T2.Transformed(Trsf_1.Inverted());
  gp_Pnt p3_1 = T3.Transformed(Trsf_1.Inverted());

  midPoints.push_back(p1_2);
  midPoints.push_back(p2_3);
  midPoints.push_back(p3_1);

  return midPoints;
}

//================================================================================
/*!
 * \brief Computes the tangency points of the circle of center Center with
 * \brief the straight line (p1 p2)
 */
//================================================================================

gp_Pnt SMESHUtils::tangencyPoint(const gp_Pnt& p1,
                                 const gp_Pnt& p2,
                                 const gp_Pnt& Center)
{
  double Xt = 0;
  double Yt = 0;

  // The tangency point is the intersection of the straight line (p1 p2)
  // and the straight line (Center T) which is orthogonal to (p1 p2)
  if ( fabs(p1.X() - p2.X()) <= Precision::Confusion() )
  {
    Xt=p1.X();     // T is on (p1 p2)
    Yt=Center.Y(); // (Center T) is orthogonal to (p1 p2)
  }
  else if ( fabs(p1.Y() - p2.Y()) <= Precision::Confusion() )
  {
    Yt=p1.Y();     // T is on (p1 p2)
    Xt=Center.X(); // (Center T) is orthogonal to (p1 p2)
  }
  else
  {
    // First straight line coefficients (equation y=a*x+b)
    double a = (p2.Y() - p1.Y()) / (p2.X() - p1.X())  ;
    double b = p1.Y() - a*p1.X();         // p1 is on this straight line

    // Second straight line coefficients (equation y=c*x+d)
    double c = -1 / a;                    // The 2 lines are orthogonal
    double d = Center.Y() - c*Center.X(); // Center is on this straight line

    Xt = (d - b) / (a - c);
    Yt = a*Xt + b;
  }

  return gp_Pnt( Xt, Yt, 0 );
}
