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

//  File   : StdMeshers_CartesianParameters3D.cxx
//  Author : Edward AGAPOV
//  Module : SMESH
//
#include "StdMeshers_CartesianParameters3D.hxx"

#include "StdMeshers_NumberOfSegments.hxx"
#include "StdMeshers_Distribution.hxx"
#include "SMESH_Gen.hxx"

#include "utilities.h"

#include <map>
#include <limits>

#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomLib_IsPlanarSurface.hxx>
#include <Geom_Surface.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_MapIteratorOfMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Dir.hxx>
#include <gp_Mat.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec.hxx>

using namespace std;

//=======================================================================
//function : StdMeshers_CartesianParameters3D
//purpose  : Constructor
//=======================================================================

StdMeshers_CartesianParameters3D::StdMeshers_CartesianParameters3D(int         hypId,
                                                                   int         studyId,
                                                                   SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen),
    _sizeThreshold( 4.0 ), // default according to the customer specification
    _toAddEdges( false )
{
  _name = "CartesianParameters3D"; // used by "Cartesian_3D"
  _param_algo_dim = 3; // 3D

  _axisDirs[0] = 1.;
  _axisDirs[1] = 0.;
  _axisDirs[2] = 0.;

  _axisDirs[3] = 0.;
  _axisDirs[4] = 1.;
  _axisDirs[5] = 0.;

  _axisDirs[6] = 0.;
  _axisDirs[7] = 0.;
  _axisDirs[8] = 1.;

  _fixedPoint[0] = 0.;
  _fixedPoint[1] = 0.;
  _fixedPoint[2] = 0.;
  SetFixedPoint( _fixedPoint, /*toUnset=*/true );
}


namespace
{
  const char* axisName[3] = { "X", "Y", "Z" };

  typedef std::pair< double, std::pair< double, double > > TCooTriple;

#define gpXYZ( cTriple ) gp_XYZ( (cTriple).first, (cTriple).second.first, (cTriple).second.second )

  //================================================================================
  /*!
   * \brief Compare two normals
   */
  //================================================================================

  bool sameDir( const TCooTriple& n1, const TCooTriple& n2 )
  {
    gp_XYZ xyz1 = gpXYZ( n1 ), xyz2 = gpXYZ( n2 );
    return ( xyz1 - xyz2 ).Modulus() < 0.01;
  }

  //================================================================================
  /*!
   * \brief Checks validity of an axis index, throws in case of invalidity
   */
  //================================================================================

  void checkAxis(const int axis)
  {
    if ( axis < 0 || axis > 2 )
      throw SALOME_Exception(SMESH_Comment("Invalid axis index ") << axis <<
                             ". Valid axis indices are 0, 1 and 2");
  }

  //================================================================================
  /*!
   * \brief Checks validity of spacing data, throws in case of invalidity
   */
  //================================================================================

  void checkGridSpacing(std::vector<std::string>& spaceFunctions,
                        std::vector<double>&      internalPoints,
                        const std::string&        axis)
  {
    if ( spaceFunctions.empty() )
      throw SALOME_Exception(SMESH_Comment("Empty space function for ") << axis );

    for ( size_t i = 1; i < internalPoints.size(); ++i )
      if ( internalPoints[i] - internalPoints[i-1] < 0 )
        throw SALOME_Exception(SMESH_Comment("Wrong order of internal points along ") << axis);
      else if ( internalPoints[i] - internalPoints[i-1] < 1e-3 )
        throw SALOME_Exception(SMESH_Comment("Too close internal points along ") << axis );

    const double tol = Precision::Confusion();
    if ( !internalPoints.empty() &&
         ( internalPoints.front() < -tol || internalPoints.back() > 1 + tol ))
      throw SALOME_Exception(SMESH_Comment("Invalid internal points along ") << axis);

    if ( internalPoints.empty() || internalPoints.front() > tol )
      internalPoints.insert( internalPoints.begin(), 0. );
    if ( internalPoints.size() < 2 || internalPoints.back() < 1 - tol )
      internalPoints.push_back( 1. );

    if ( internalPoints.size() != spaceFunctions.size() + 1 )
      throw SALOME_Exception
        (SMESH_Comment("Numbre of internal points mismatch number of functions for ") << axis);

    for ( size_t i = 0; i < spaceFunctions.size(); ++i )
      spaceFunctions[i] =
        StdMeshers_NumberOfSegments::CheckExpressionFunction( spaceFunctions[i], -1 );
  }
}

//=======================================================================
//function : SetGrid
//purpose  : Sets coordinates of node positions along an axes
//=======================================================================

void StdMeshers_CartesianParameters3D::SetGrid(std::vector<double>& coords, int axis)
{
  checkAxis( axis );

  if ( coords.size() < 2 )
    throw SALOME_Exception(LOCALIZED("Wrong number of grid coordinates"));

  std::sort( coords.begin(), coords.end() );

  bool changed = ( _coords[axis] != coords );
  if ( changed )
  {
    _coords[axis] = coords;
    NotifySubMeshesHypothesisModification();
  }

  _spaceFunctions[axis].clear();
  _internalPoints[axis].clear();
}

//=======================================================================
//function : SetGridSpacing
//purpose  : Set grid spacing along the three axes
//=======================================================================

void StdMeshers_CartesianParameters3D::SetGridSpacing(std::vector<std::string>& xSpaceFuns,
                                                      std::vector<double>& xInternalPoints,
                                                      const int            axis)
{
  checkAxis( axis );

  checkGridSpacing( xSpaceFuns, xInternalPoints, axisName[axis] );

  bool changed = ( xSpaceFuns      != _spaceFunctions[axis] ||
                   xInternalPoints != _internalPoints[axis] );

  _spaceFunctions[axis] = xSpaceFuns;
  _internalPoints[axis] = xInternalPoints;
  _coords[axis].clear();

  if ( changed )
    NotifySubMeshesHypothesisModification();
}

//=======================================================================
//function : SetFixedPoint
//purpose  : * Set/unset a fixed point, at which a node will be created provided that grid
//           * is defined by spacing in all directions
//=======================================================================

void StdMeshers_CartesianParameters3D::SetFixedPoint(const double p[3], bool toUnset)
{
  if ( toUnset != Precision::IsInfinite( _fixedPoint[0] ))
    NotifySubMeshesHypothesisModification();

  if ( toUnset )
    _fixedPoint[0] = Precision::Infinite();
  else
    std::copy( &p[0], &p[0]+3, &_fixedPoint[0] );
}

//=======================================================================
//function : GetFixedPoint
//purpose  : Returns either false or (true + point coordinates)
//=======================================================================

bool StdMeshers_CartesianParameters3D::GetFixedPoint(double p[3]) const
{
  if ( Precision::IsInfinite( _fixedPoint[0] ))
    return false;
  std::copy( &_fixedPoint[0], &_fixedPoint[0]+3, &p[0] );
  return true;
}


//=======================================================================
//function : SetSizeThreshold
//purpose  : Set size threshold
//=======================================================================

void StdMeshers_CartesianParameters3D::SetSizeThreshold(const double threshold)
{
  if ( threshold <= 1.0 )
    throw SALOME_Exception(LOCALIZED("threshold must be > 1.0"));

  bool changed = fabs( _sizeThreshold - threshold ) > 1e-6;
  _sizeThreshold = threshold;

  if ( changed )
    NotifySubMeshesHypothesisModification();
}

//=======================================================================
//function : GetGridSpacing
//purpose  : return spacing
//=======================================================================

void StdMeshers_CartesianParameters3D::GetGridSpacing(std::vector<std::string>& spaceFunctions,
                                                      std::vector<double>&      internalPoints,
                                                      const int                 axis) const
{
  if ( !IsGridBySpacing(axis) )
    throw SALOME_Exception(LOCALIZED("The grid is defined by coordinates and not by spacing"));

  spaceFunctions = _spaceFunctions[axis];
  internalPoints = _internalPoints[axis];
}

//=======================================================================
//function : IsGridBySpacing
//=======================================================================

bool StdMeshers_CartesianParameters3D::IsGridBySpacing(const int axis) const
{
  checkAxis(axis);
  return !_spaceFunctions[axis].empty();
}


//=======================================================================
//function : ComputeCoordinates
//purpose  : Computes node coordinates by spacing functions
//=======================================================================

void StdMeshers_CartesianParameters3D::ComputeCoordinates(const double    x0,
                                                          const double    x1,
                                                          vector<string>& theSpaceFuns,
                                                          vector<double>& thePoints,
                                                          vector<double>& coords,
                                                          const string&   axis,
                                                          const double*   xForced )
{
  checkGridSpacing( theSpaceFuns, thePoints, axis );

  vector<string> spaceFuns = theSpaceFuns;
  vector<double> points    = thePoints;

  bool forced = false;
  if (( forced = ( xForced && ( x0 < *xForced ) && ( *xForced < x1 ))))
  {
    // divide a range at xForced

    // find a range to insert xForced
    double pos = ( *xForced - x0 ) / ( x1 - x0 );
    int iR = 1;
    while ( pos > points[ iR ] ) ++iR;

    // insert xForced
    vector<double>::iterator pntIt = points.begin() + iR;
    points.insert( pntIt, pos );
    vector<string>::iterator funIt = spaceFuns.begin() + iR;
    spaceFuns.insert( funIt, spaceFuns[ iR-1 ]);
  }

  coords.clear();
  for ( size_t i = 0; i < spaceFuns.size(); ++i )
  {
    StdMeshers::FunctionExpr fun( spaceFuns[i].c_str(), /*convMode=*/-1 );

    const double p0 = x0 * ( 1. - points[i])   + x1 * points[i];
    const double p1 = x0 * ( 1. - points[i+1]) + x1 * points[i+1];
    const double length = p1 - p0;

    const size_t nbSections = 1000;
    const double sectionLen = ( p1 - p0 ) / nbSections;
    vector< double > nbSegments( nbSections + 1 );
    nbSegments[ 0 ] = 0.;

    double t, spacing = 0;
    for ( size_t i = 1; i <= nbSections; ++i )
    {
      t = double( i ) / nbSections;
      if ( !fun.value( t, spacing ) || spacing < std::numeric_limits<double>::min() )
        throw SALOME_Exception(LOCALIZED("Invalid spacing function"));
      nbSegments[ i ] = nbSegments[ i-1 ] + std::min( 1., sectionLen / spacing );
    }

    const int nbCells = max (1, int(floor(nbSegments.back()+0.5)));
    const double corr = nbCells / nbSegments.back();

    if ( coords.empty() ) coords.push_back( p0 );

    for ( size_t iCell = 1, i = 1; i <= nbSections; ++i )
    {
      if ( nbSegments[i]*corr >= iCell )
      {
        t = (i - ( nbSegments[i] - iCell/corr )/( nbSegments[i] - nbSegments[i-1] )) / nbSections;
        coords.push_back( p0 + t * length );
        ++iCell;
      }
    }
    const double lastCellLen = coords.back() - coords[ coords.size() - 2 ];
    if ( fabs( coords.back() - p1 ) > 0.5 * lastCellLen )
      coords.push_back ( p1 );
  }

  // correct coords if a forced point is too close to a neighbor node
  if ( forced )
  {
    int iF = 0;
    double minLen = ( x1 - x0 );
    for ( size_t i = 1; i < coords.size(); ++i )
    {
      if ( !iF && Abs( coords[i] - *xForced ) < 1e-20 )
        iF = i++; // xForced found
      else
        minLen = Min( minLen, coords[i] - coords[i-1] );
    }
    const double tol = minLen * 1e-3;
    int iRem = -1;
    if (( iF > 1 ) && ( coords[iF] - coords[iF-1] < tol ))
      iRem = iF-1;
    else if (( iF < coords.size()-2 ) && ( coords[iF+1] - coords[iF] < tol ))
      iRem = iF+1;
    if ( iRem > 0 )
      coords.erase( coords.begin() + iRem );
  }
}

//=======================================================================
//function : GetCoordinates
//purpose  : Return coordinates of node positions along the three axes.
//           If the grid is defined by spacing functions, the coordinates are computed
//=======================================================================

void StdMeshers_CartesianParameters3D::GetCoordinates(std::vector<double>& xNodes,
                                                      std::vector<double>& yNodes,
                                                      std::vector<double>& zNodes,
                                                      const Bnd_Box&       bndBox) const
{
  double x0,y0,z0, x1,y1,z1;
  if ( IsGridBySpacing(0) || IsGridBySpacing(1) || IsGridBySpacing(2))
  {
    if ( bndBox.IsVoid() ||
         bndBox.IsXThin( Precision::Confusion() ) ||
         bndBox.IsYThin( Precision::Confusion() ) ||
         bndBox.IsZThin( Precision::Confusion() ) )
      throw SALOME_Exception(LOCALIZED("Invalid bounding box"));
    bndBox.Get(x0,y0,z0, x1,y1,z1);
  }

  double fp[3], *pfp[3] = { NULL, NULL, NULL };
  if ( GetFixedPoint( fp ))
  {
    // convert fp into a basis defined by _axisDirs
    gp_XYZ axis[3] = { gp_XYZ( _axisDirs[0], _axisDirs[1], _axisDirs[2] ),
                       gp_XYZ( _axisDirs[3], _axisDirs[4], _axisDirs[5] ),
                       gp_XYZ( _axisDirs[6], _axisDirs[7], _axisDirs[8] ) };
    axis[0].Normalize();
    axis[1].Normalize();
    axis[2].Normalize();

    gp_Mat basis( axis[0], axis[1], axis[2] );
    gp_Mat bi = basis.Inverted();

    gp_XYZ p( fp[0], fp[1], fp[2] );
    p *= bi;
    p.Coord( fp[0], fp[1], fp[2] );

    pfp[0] = & fp[0];
    pfp[1] = & fp[1];
    pfp[2] = & fp[2];
  }

  StdMeshers_CartesianParameters3D* me = const_cast<StdMeshers_CartesianParameters3D*>(this);
  if ( IsGridBySpacing(0) )
    ComputeCoordinates
      ( x0, x1, me->_spaceFunctions[0], me->_internalPoints[0], xNodes, "X", pfp[0] );
  else
    xNodes = _coords[0];

  if ( IsGridBySpacing(1) )
    ComputeCoordinates
      ( y0, y1, me->_spaceFunctions[1], me->_internalPoints[1], yNodes, "Y", pfp[1] );
  else
    yNodes = _coords[1];

  if ( IsGridBySpacing(2) )
    ComputeCoordinates
      ( z0, z1, me->_spaceFunctions[2], me->_internalPoints[2], zNodes, "Z", pfp[2] );
  else
    zNodes = _coords[2];
}

//=======================================================================
//function : ComputeOptimalAxesDirs
//purpose  : Returns axes at which number of hexahedra is maximal
//=======================================================================

void StdMeshers_CartesianParameters3D::
ComputeOptimalAxesDirs(const TopoDS_Shape& shape,
                       const bool          isOrthogonal,
                       double              dirCoords[9])
{
  for ( int i = 0; i < 9; ++i ) dirCoords[i] = 0.;
  dirCoords[0] = dirCoords[4] = dirCoords[8] = 1.;

  if ( shape.IsNull() ) return;

  TopLoc_Location loc;
  TopExp_Explorer exp;

  // get external FACEs of the shape
  TopTools_MapOfShape faceMap;
  for ( exp.Init( shape, TopAbs_FACE ); exp.More(); exp.Next() )
    if ( !faceMap.Add( exp.Current() ))
      faceMap.Remove( exp.Current() );

  // sort areas of planar faces by normal direction

  std::multimap< TCooTriple, double > areasByNormal;

  TopTools_MapIteratorOfMapOfShape fIt ( faceMap );
  for ( ; fIt.More(); fIt.Next() )
  {
    const TopoDS_Face&   face = TopoDS::Face( fIt.Key() );
    Handle(Geom_Surface) surf = BRep_Tool::Surface( face, loc );
    if ( surf.IsNull() ) continue;

    GeomLib_IsPlanarSurface check( surf, 1e-5 );
    if ( !check.IsPlanar() ) continue;

    GProp_GProps SProps;
    BRepGProp::SurfaceProperties( face, SProps );
    double area = SProps.Mass();

    gp_Pln pln  = check.Plan();
    gp_Dir norm = pln.Axis().Direction().Transformed( loc );
    if ( norm.X() < -1e-3 ) { // negative X
      norm.Reverse();
    } else if ( norm.X() < 1e-3 ) { // zero X
      if ( norm.Y() < -1e-3 ) { // negative Y
        norm.Reverse();
      } else if ( norm.Y() < 1e-3 ) { // zero X && zero Y
        if ( norm.Y() < -1e-3 ) // negative Z
          norm.Reverse();
      }
    }
    TCooTriple coo3( norm.X(), make_pair( norm.Y(), norm.Z() ));
    areasByNormal.insert( make_pair( coo3, area ));
  }

  // group coplanar normals and sort groups by sum area

  std::multimap< double, vector< const TCooTriple* > > normsByArea;
  std::multimap< TCooTriple, double >::iterator norm2a = areasByNormal.begin();
  const TCooTriple*           norm1 = 0;
  double                      sumArea = 0;
  vector< const TCooTriple* > norms;
  for ( int iF = 1; norm2a != areasByNormal.end(); ++norm2a, ++iF )
  {

    if ( !norm1 || !sameDir( *norm1, norm2a->first ))
    {
      if ( !norms.empty() )
      {
        normsByArea.insert( make_pair( sumArea, norms ));
        norms.clear();
      }
      norm1   = & norm2a->first;
      sumArea = norm2a->second;
      norms.push_back( norm1 );
    }
    else
    {
      sumArea += norm2a->second;
      norms.push_back( & norm2a->first );
    }
    if ( iF == areasByNormal.size() )
      normsByArea.insert( make_pair( sumArea, norms ));
  }

  // try to set dirs by planar faces

  gp_XYZ normDirs[3]; // normals to largest planes

  if ( !normsByArea.empty() )
  {
    norm1 = normsByArea.rbegin()->second[0];
    normDirs[0] = gpXYZ( *norm1 );

    if ( normsByArea.size() == 1 )
    {
      normDirs[1] = normDirs[0];
      if ( Abs( normDirs[0].Y() ) < 1e-100 &&
           Abs( normDirs[0].Z() ) < 1e-100 ) // normDirs[0] || OX
        normDirs[1].SetY( normDirs[0].Y() + 1. );
      else
        normDirs[1].SetX( normDirs[0].X() + 1. );
    }
    else
    {
      // look for 2 other directions
      gp_XYZ testDir = normDirs[0], minDir, maxDir;
      for ( int is2nd = 0; is2nd < 2; ++is2nd )
      {
        double maxMetric = 0, minMetric = 1e100;
        std::multimap< double, vector< const TCooTriple* > >::iterator a2n;
        for ( a2n = normsByArea.begin(); a2n != normsByArea.end(); ++a2n )
        {
          gp_XYZ n = gpXYZ( *( a2n->second[0]) );
          double dot = Abs( n * testDir );
          double metric = ( 1. - dot ) * ( isOrthogonal ? 1 : a2n->first );
          if ( metric > maxMetric )
          {
            maxDir = n;
            maxMetric = metric;
          }
          if ( metric < minMetric )
          {
            minDir = n;
            minMetric = metric;
          }
        }
        if ( is2nd )
        {
          normDirs[2] = minDir;
        }
        else
        {
          normDirs[1] = maxDir;
          normDirs[2] = normDirs[0] ^ normDirs[1];
          if ( isOrthogonal || normsByArea.size() < 3 )
            break;
          testDir = normDirs[2];
        }
      }
    }
    if ( isOrthogonal || normsByArea.size() == 1 )
    {
      normDirs[2] = normDirs[0] ^ normDirs[1];
      normDirs[1] = normDirs[2] ^ normDirs[0];
    }
  }
  else
  {
    return;
  }

  gp_XYZ dirs[3];
  dirs[0] = normDirs[0] ^ normDirs[1];
  dirs[1] = normDirs[1] ^ normDirs[2];
  dirs[2] = normDirs[2] ^ normDirs[0];

  dirs[0].Normalize();
  dirs[1].Normalize();
  dirs[2].Normalize();

  // Select dirs for X, Y and Z axes
  int iX = ( Abs( dirs[0].X() ) > Abs( dirs[1].X() )) ? 0 : 1;
  if ( Abs( dirs[iX].X() ) < Abs( dirs[2].X() ))
    iX = 2;
  int iY = ( iX == 0 ) ? 1 : (( Abs( dirs[0].Y() ) > Abs( dirs[1].Y() )) ? 0 : 1 );
  if ( Abs( dirs[iY].Y() ) < Abs( dirs[2].Y() ) && iX != 2 )
    iY = 2;
  int iZ = 3 - iX - iY;

  if ( dirs[iX].X() < 0 ) dirs[iX].Reverse();
  if ( dirs[iY].Y() < 0 ) dirs[iY].Reverse();
  gp_XYZ zDir = dirs[iX] ^ dirs[iY];
  if ( dirs[iZ] * zDir < 0 )
    dirs[iZ].Reverse();

  dirCoords[0] = dirs[iX].X();
  dirCoords[1] = dirs[iX].Y();
  dirCoords[2] = dirs[iX].Z();
  dirCoords[3] = dirs[iY].X();
  dirCoords[4] = dirs[iY].Y();
  dirCoords[5] = dirs[iY].Z();
  dirCoords[6] = dirs[iZ].X();
  dirCoords[7] = dirs[iZ].Y();
  dirCoords[8] = dirs[iZ].Z();
}

//=======================================================================
//function : SetAxisDirs
//purpose  : Sets custom direction of axes
//=======================================================================

void StdMeshers_CartesianParameters3D::SetAxisDirs(const double* the9DirComps)
{
  gp_Vec x( the9DirComps[0],
            the9DirComps[1],
            the9DirComps[2] );
  gp_Vec y( the9DirComps[3],
            the9DirComps[4],
            the9DirComps[5] );
  gp_Vec z( the9DirComps[6],
            the9DirComps[7],
            the9DirComps[8] );
  if ( x.Magnitude() < RealSmall() ||
       y.Magnitude() < RealSmall() ||
       z.Magnitude() < RealSmall() )
    throw SALOME_Exception("Zero magnitude of axis direction");

  if ( x.IsParallel( y, M_PI / 180. ) ||
       x.IsParallel( z, M_PI / 180. ) ||
       y.IsParallel( z, M_PI / 180. ))
    throw SALOME_Exception("Parallel axis directions");

  gp_Vec normXY = x ^ y, normYZ = y ^ z;
  if ( normXY.IsParallel( normYZ, M_PI / 180. ))
    throw SALOME_Exception("Axes lie in one plane");

  bool isChanged = false;
  for ( int i = 0; i < 9; ++i )
  {
    if ( Abs( _axisDirs[i] - the9DirComps[i] ) > 1e-7 )
      isChanged = true;
    _axisDirs[i] = the9DirComps[i];
  }
  if ( isChanged )
    NotifySubMeshesHypothesisModification();
}

//=======================================================================
//function : GetGrid
//purpose  : Return coordinates of node positions along the three axes
//=======================================================================

void StdMeshers_CartesianParameters3D::GetGrid(std::vector<double>& coords, int axis) const
{
  if ( IsGridBySpacing(axis) )
    throw SALOME_Exception(LOCALIZED("The grid is defined by spacing and not by coordinates"));

  coords = _coords[axis];
}

//=======================================================================
//function : GetSizeThreshold
//purpose  : Return size threshold
//=======================================================================

double StdMeshers_CartesianParameters3D::GetSizeThreshold() const
{
  return _sizeThreshold;
}

//=======================================================================
//function : SetToAddEdges
//purpose  : Enables implementation of geometrical edges into the mesh. If this feature
//           is disabled, sharp edges of the shape are lost ("smoothed") in the mesh if
//           they don't coincide with the grid lines
//=======================================================================

void StdMeshers_CartesianParameters3D::SetToAddEdges(bool toAdd)
{
  if ( _toAddEdges != toAdd )
  {
    _toAddEdges = toAdd;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetToAddEdges
//purpose  : Returns true if implementation of geometrical edges into the
//           mesh is enabled
//=======================================================================

bool StdMeshers_CartesianParameters3D::GetToAddEdges() const
{
  return _toAddEdges;
}

//=======================================================================
//function : IsDefined
//purpose  : Return true if parameters are well defined
//=======================================================================

bool StdMeshers_CartesianParameters3D::IsDefined() const
{
  for ( int i = 0; i < 3; ++i )
    if (_coords[i].empty() && (_spaceFunctions[i].empty() || _internalPoints[i].empty()))
      return false;

  return ( _sizeThreshold > 1.0 );
}

//=======================================================================
//function : SaveTo
//purpose  : store my parameters into a stream
//=======================================================================

std::ostream & StdMeshers_CartesianParameters3D::SaveTo(std::ostream & save)
{
  save << _sizeThreshold << " ";

  for ( int i = 0; i < 3; ++i )
  {
    save << _coords[i].size() << " ";
    for ( size_t j = 0; j < _coords[i].size(); ++j )
      save << _coords[i][j] << " ";

    save << _internalPoints[i].size() << " ";
    for ( size_t j = 0; j < _internalPoints[i].size(); ++j )
      save << _internalPoints[i][j] << " ";

    save << _spaceFunctions[i].size() << " ";
    for ( size_t j = 0; j < _spaceFunctions[i].size(); ++j )
      save << _spaceFunctions[i][j] << " ";
  }
  save << _toAddEdges << " ";

  save.setf( save.scientific );
  save.precision( 12 );
  for ( int i = 0; i < 9; ++i )
    save << _axisDirs[i] << " ";

  for ( int i = 0; i < 3; ++i )
    save << _fixedPoint[i] << " ";

  return save;
}

//=======================================================================
//function : LoadFrom
//purpose  : restore my parameters from a stream
//=======================================================================

std::istream & StdMeshers_CartesianParameters3D::LoadFrom(std::istream & load)
{
  bool ok;

  ok = (bool)( load >> _sizeThreshold );
  for ( int ax = 0; ax < 3; ++ax )
  {
    if (ok)
    {
      size_t i = 0;
      ok = (bool)(load >> i  );
      if ( i > 0 && ok )
      {
        _coords[ax].resize( i );
        for ( i = 0; i < _coords[ax].size() && ok; ++i )
          ok = (bool)(load >> _coords[ax][i]  );
      }
    }
    if (ok)
    {
      size_t i = 0;
      ok = (bool)(load >> i  );
      if ( i > 0 && ok )
      {
        _internalPoints[ax].resize( i );
        for ( i = 0; i < _internalPoints[ax].size() && ok; ++i )
          ok = (bool)(load >> _internalPoints[ax][i]  );
      }
    }
    if (ok)
    {
      size_t i = 0;
      ok = (bool)(load >> i  );
      if ( i > 0 && ok )
      {
        _spaceFunctions[ax].resize( i );
        for ( i = 0; i < _spaceFunctions[ax].size() && ok; ++i )
          ok = (bool)(load >> _spaceFunctions[ax][i]  );
      }
    }
  }

  ok = (bool)( load >> _toAddEdges );

  for ( int i = 0; i < 9 && ok; ++i )
    ok = (bool)( load >> _axisDirs[i]);

  for ( int i = 0; i < 3 && ok ; ++i )
    ok = (bool)( load >> _fixedPoint[i]);

  return load;
}

//=======================================================================
//function : SetParametersByMesh
//=======================================================================

bool StdMeshers_CartesianParameters3D::SetParametersByMesh(const SMESH_Mesh*   ,
                                                           const TopoDS_Shape& )
{
  return false;
}

//=======================================================================
//function : SetParametersByDefaults
//=======================================================================

bool StdMeshers_CartesianParameters3D::SetParametersByDefaults(const TDefaults&  dflts,
                                                               const SMESH_Mesh* /*theMesh*/)
{
  if ( dflts._elemLength > 1e-100 )
  {
    vector<string> spacing( 1, SMESH_Comment(dflts._elemLength));
    vector<double> intPnts;
    SetGridSpacing( spacing, intPnts, 0 );
    SetGridSpacing( spacing, intPnts, 1 );
    SetGridSpacing( spacing, intPnts, 2 );
    return true;
  }
  return false;
}

