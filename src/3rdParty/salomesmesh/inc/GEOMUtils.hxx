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

#ifndef _GEOMUtils_HXX_
#define _GEOMUtils_HXX_

#include <Standard_Macro.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>

#include <TopTools_ListOfShape.hxx>

#include <TopAbs.hxx>

#include <gp_Ax3.hxx>
#include <gp_Vec.hxx>

#include <V3d_View.hxx>

#include <NCollection_DataMap.hxx>

#include <functional>

#include <map>
#include <vector>
#include <string>
#include <utility>

class Bnd_Box;

inline Standard_Boolean IsEqual (const TopoDS_Shape& S1, const TopoDS_Shape& S2)
{
  return S1.IsSame(S2);
}

namespace GEOMUtils
{

  typedef std::vector<std::string> NodeLinks;
  typedef std::map<std::string, NodeLinks> LevelInfo;
  typedef std::vector<LevelInfo> LevelsList;
  typedef std::map<std::string,std::pair<LevelsList,LevelsList> > TreeModel;

  /*!
   * \brief Compute numerical functor for the shape.
   *
   * Resulting value can be used to sort out shapes according to some parameter.
   * 
   * Returns a pair of two values (dist, functor) where
   * - \a dist is a some value that is computed according to the center of mass of given shape;
   * - \a functor is a numerical functor value
   *
   * The numerical functor is computed according to the shape's topological properties as follows:
   * - orientation for vertices 
   * - length for edges and wires
   * - area for faces and shells
   * - volume for solids, compounds, compsolids
   *
   * If \a isOldSorting parameter is set to \c true, for all cases linear properties of the shape
   * are used (to support backward compatibility in some methods). By default, this parameter is
   * set to \c false.
   */
  Standard_EXPORT std::pair<double, double> ShapeToDouble (const TopoDS_Shape& theShape,
                                                           bool isOldSorting = false);

  /*!
   * \brief Get Local Coordinate System, corresponding to the given shape.
   *
   * Origin of the LCS is situated at the shape's center of mass.
   * Axes of the LCS are obtained from shape's location or,
   * if the shape is a planar face, from position of its plane.
   */
  Standard_EXPORT gp_Ax3 GetPosition (const TopoDS_Shape& theShape);

  /*!
   * \brief Get vector, defined by the given edge.
   * \param theShape The edge.
   * \param doConsiderOrientation If True, take into account the edge orientation.
   * \note It is recommended to use doConsiderOrientation=Standard_False, because
   *       the same edge can have different orientation depending on the way it was
   *       extracted from a shape.
   */
  Standard_EXPORT gp_Vec GetVector (const TopoDS_Shape& theShape,
				    Standard_Boolean doConsiderOrientation);

  /*!
   * \brief Sort shapes in the list by their coordinates.
   * \param SL The list of shapes to sort.
   */
  struct CompareShapes
  {
    typedef TopoDS_Shape first_argument_type;
    typedef TopoDS_Shape second_argument_type;
    typedef bool result_type;
    CompareShapes (bool isOldSorting)
      : myIsOldSorting(isOldSorting) {}

    bool operator() (const TopoDS_Shape& lhs, const TopoDS_Shape& rhs);

    typedef NCollection_DataMap<TopoDS_Shape, std::pair<double, double> > GEOMUtils_DataMapOfShapeDouble;
    GEOMUtils_DataMapOfShapeDouble myMap;
    bool myIsOldSorting;
  };

  /*!
   * \brief Sort shapes by their centers of mass, using formula X*999 + Y*99 + Z*0.9
   */
  Standard_EXPORT void SortShapes (TopTools_ListOfShape& SL,
				   const Standard_Boolean isOldSorting = Standard_True);

  /*!
   * \brief Convert TopoDS_COMPSOLID to TopoDS_COMPOUND.
   *
   * If the argument shape is not of type TopoDS_COMPSOLID, this method returns it as is.
   *
   * \param theCompsolid The compsolid to be converted.
   * \retval TopoDS_Shape Returns the resulting compound.
   */
  Standard_EXPORT TopoDS_Shape CompsolidToCompound (const TopoDS_Shape& theCompsolid);

  /*!
   * \brief Recursively extract all shapes from compounds and compsolids of the given shape into theList.
   *
   * If theShape is not compound or compsolid, theList will contain only theShape itself.
   *
   * \param theShape The shape to be exploded.
   * \param theList Output parameter.
   */
  Standard_EXPORT void AddSimpleShapes (const TopoDS_Shape& theShape,
					TopTools_ListOfShape& theList);

  /*!
   * \brief Build a triangulation on \a theShape if it is absent.
   * \param theShape The shape to check/build triangulation on.
   * \retval bool Returns false if the shape has no faces, i.e. impossible to build triangulation.
   */
  Standard_EXPORT bool CheckTriangulation (const TopoDS_Shape& theShape);
  
  /*!
   * \brief Return type of shape for explode. In case of compound it will be a type of its first sub shape.
   * \param theShape The shape to get type of.
   * \retval TopAbs_ShapeEnum Return type of shape for explode.
   */
  Standard_EXPORT TopAbs_ShapeEnum GetTypeOfSimplePart (const TopoDS_Shape& theShape);

  /*!
   * \brief Find an edge of theShape, closest to thePoint.
   *
   * \param theShape The shape to explore.
   * \param thePoint The point near the required edge.
   * \retval TopoDS_Shape Returns the found edge or an empty shape if multiple edges found.
   */
  Standard_EXPORT TopoDS_Shape GetEdgeNearPoint (const TopoDS_Shape&  theShape,
						 const TopoDS_Vertex& thePoint);

  /*!
   * \brief Compute precise bounding box of the shape based on the rough bounding box.
   *
   * \param theShape the shape.
   * \param theBox rough bounding box on input; precise bounding box on output.
   * \retval Standard_True in case of success; Standard_False otherwise.
   */
  Standard_EXPORT Standard_Boolean PreciseBoundingBox(const TopoDS_Shape &theShape, Bnd_Box &theBox);

  /*!
   * \brief Computes minumal distance between two shapes for singular cases
   *        (workaround for bugs 19899, 19908 and 19910 from Mantis).
   *
   * \param aSh1 the first shape
   * \param aSh2 the second shape
   * \param Ptmp1 the output result point on the first shape
   * \param Ptmp2 the output result point on the second shape
   * \retval negative value if it is not a singular case; actual distance for singular case.
   */
  Standard_EXPORT Standard_Real GetMinDistanceSingular(const TopoDS_Shape& aSh1,
						       const TopoDS_Shape& aSh2,
						       gp_Pnt& Ptmp1, gp_Pnt& Ptmp2);
  
  /*!
   * \brief Computes minumal distance between two shapes.
   *
   * \param theShape1 the first shape
   * \param theShape2 the second shape
   * \param thePnt1 the output result point on the first shape
   * \param thePnt2 the output result point on the second shape
   * \retval negative value in case of failure; otherwise the real distance.
   */
  Standard_EXPORT Standard_Real GetMinDistance(const TopoDS_Shape& theShape1,
					       const TopoDS_Shape& theShape2,
					       gp_Pnt& thePnt1, gp_Pnt& thePnt2);
  
  /*!
   * \brief Returns the point clicked in 3D view.
   *
   * \param x The X coordinate in the view.
   * \param y The Y coordinate in the view.
   * \param theView View where the given point takes place.
   * \retval gp_Pnt Returns the point clicked in 3D view
   */
  Standard_EXPORT gp_Pnt ConvertClickToPoint( int x, int y, Handle(V3d_View) theView );

  /*!
   * \brief Convert dependency tree data to the string representation
   *
   * \param tree dependency tree data
   * \param dependencyStr output string
   */
  Standard_EXPORT void ConvertTreeToString( const TreeModel& tree,
					    std::string& dependencyStr );

  /*!
   * \brief Restore dependency tree data from the string representation
   *
   * \param dependencyStr string representation of tree data
   * \param tree output dependency tree data
   */
  Standard_EXPORT void ConvertStringToTree( const std::string& dependencyStr,
					    TreeModel& tree );

  /*!
   * \brief Check shape
   *
   * \param shape input shape object
   * \param checkGeometry when set to \c true, causes check of underlying geometry
   *        in addition to the topology
   * \return \c true if shape is valid or \c false otherwise
   */
  Standard_EXPORT bool CheckShape( TopoDS_Shape& shape, bool checkGeometry = false );
  
  /*!
   * \brief Limit shape tolerance to the given value
   *
   * \param shape shape being fixed
   * \param type topology type which tolerance is to be limited; TopAbs_SHAPE means
   *             all types of topology
   * \param tolerance expected tolerance value (1e-7 by default)
   * \param checkGeometry check geometry validity of result
   * \return \c true if resulting shape is valid
   *
   * \note Resulting tolerance of the shape is not mandatory equal to requested value
   *       as it might be changed by fixshape operation in order to get valid shape where possible
   * \note By default, result only checked for topology validity; check of geometry can be done by
   *       passing \c true to \a checkGeometry parameter
   */
  Standard_EXPORT bool FixShapeTolerance( TopoDS_Shape& shape,
                                          TopAbs_ShapeEnum type,
                                          Standard_Real tolerance = Precision::Confusion(),
                                          bool checkGeometry = false );

  /*!
   * \brief Limit shape tolerance to the given value
   * This is overloaded function, it behaves exactly as previous one
   */
  Standard_EXPORT bool FixShapeTolerance( TopoDS_Shape& shape,
                                          Standard_Real tolerance = Precision::Confusion(),
                                          bool checkGeometry = false );

  /*!
   * \brief Limit shape tolerance to the given value
   * This is overloaded function, it behaves exactly as previous one
   */
  Standard_EXPORT bool FixShapeTolerance( TopoDS_Shape& shape,
                                          bool checkGeometry );

  /*!
   * \brief Fix curves of the given shape
   * 
   * The function checks each curve of the input shape in the following way:
   * - compute deviation of the curve from the underlying surface in a set of points
   *   computed with the certain discretization step value
   * - find maximum tolerance between computed deviation values
   * - limit tolerance of the curve with the computed maximum value
   * 
   * \param shape shape being fixed
   * \return \c true if resulting shape is valid
   */
  Standard_EXPORT bool FixShapeCurves( TopoDS_Shape& shape );

  /*!
   * \brief Write shape to the BREP file
   *
   * \param source shape
   * \return \c true if file was written or \c false otherwise
   */
  Standard_EXPORT bool Write( const TopoDS_Shape& shape,
                              const char* fileName );
  
  /*!
   * \brief Extract single SOLID from COMPSOLID or COMPOUND.
   *
   * If the argument shape is a COMPOUND or COMPSOLID and there's
   * only single simple-shape type inside, this sub-shape is returned as a result;
   * otherwise, the shape is not changed.
   *
   * \param shape compound or compsolid being processed.
   * \retval TopoDS_Shape resulting shape
   */
  Standard_EXPORT TopoDS_Shape ReduceCompound( const TopoDS_Shape& shape );

  /*!
   * \brief Generate triangulation for the shape.
   *
   * \param shape shape being meshed
   * \param deflection deflection coefficient to be used
   * \param forced if \c true, causes generation of mesh regardless it is already present in the shape
   */
  Standard_EXPORT void MeshShape( const TopoDS_Shape shape,
                                  double deflection, bool forced = true );

  /*!
   * \brief Get default deflection coefficient used for triangulation
   * \return default deflection value
   */
  Standard_EXPORT double DefaultDeflection();

  /**
   * \brief Check if the shape is not a closed wire or edge.
   *
   * This function is used for pipe creation algorithm to test if
   * the pipe path is not closed. It returns false if theShape is a wire or
   * an edge with coincident end vertices. Otherwise it returns true.
   *
   * \param theShape the shape to be tested.
   * \return true if theShape is not a closed wire or edge.
   */
  Standard_EXPORT bool IsOpenPath(const TopoDS_Shape &theShape);

};

#endif
