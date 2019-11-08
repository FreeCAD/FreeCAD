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

// ---
// File    : BLSURFPlugin_Hypothesis.cxx
// Authors : Francis KLOSS (OCC) & Patrick LAUG (INRIA) & Lioka RAZAFINDRAZAKA (CEA)
//           & Aurelien ALLEAUME (DISTENE)
//           Size maps development: Nicolas GEIMER (OCC) & Gilles DAVID (EURIWARE)
// ---
//
#include "BLSURFPlugin_Hypothesis.hxx"
#include "BLSURFPlugin_Attractor.hxx"
// #include "SMESH_Gen_i.hxx"
#include "SMESH_Gen.hxx"
#include <utilities.h>
#include <cstring>
#include <iostream>
#include <sstream>

// cascade include
#include "ShapeAnalysis.hxx"

// CORBA includes
// #include CORBA_CLIENT_HEADER(SALOMEDS)
// #include CORBA_CLIENT_HEADER(GEOM_Gen)

namespace
{
  struct GET_DEFAULT // struct used to get default value from GetOptionValue()
  {
    bool isDefault;
    operator bool* () { return &isDefault; }
  };
}

//=============================================================================
BLSURFPlugin_Hypothesis::BLSURFPlugin_Hypothesis(int hypId, int studyId, SMESH_Gen * gen, bool hasgeom) :
  SMESH_Hypothesis(hypId, studyId, gen), 
  _physicalMesh(GetDefaultPhysicalMesh()),
  _geometricMesh(GetDefaultGeometricMesh()),
  _phySize(GetDefaultPhySize()),
  _phySizeRel(GetDefaultPhySizeRel()),
  _minSize(GetDefaultMinSize()),
  _maxSize(GetDefaultMaxSize()),
  _minSizeRel(GetDefaultMinSizeRel()),
  _maxSizeRel(GetDefaultMaxSizeRel()),
  _useGradation(GetDefaultUseGradation()),
  _gradation(GetDefaultGradation()),
  _useVolumeGradation(GetDefaultUseVolumeGradation()),
  _volumeGradation(GetDefaultVolumeGradation()),
  _quadAllowed(GetDefaultQuadAllowed()),
  _angleMesh(GetDefaultAngleMesh()),
  _chordalError(GetDefaultChordalError()), 
  _anisotropic(GetDefaultAnisotropic()),
  _anisotropicRatio(GetDefaultAnisotropicRatio()),
  _removeTinyEdges(GetDefaultRemoveTinyEdges()),
  _tinyEdgeLength(GetDefaultTinyEdgeLength()),
  _optimiseTinyEdges(GetDefaultOptimiseTinyEdges()),
  _tinyEdgeOptimisationLength(GetDefaultTinyEdgeOptimisationLength()),
  _correctSurfaceIntersec(GetDefaultCorrectSurfaceIntersection()),
  _corrSurfaceIntersCost(GetDefaultCorrectSurfaceIntersectionMaxCost()),
  _badElementRemoval(GetDefaultBadElementRemoval()),
  _badElementAspectRatio(GetDefaultBadElementAspectRatio()),
  _optimizeMesh(GetDefaultOptimizeMesh()),
  _quadraticMesh(GetDefaultQuadraticMesh()),
  _verb(GetDefaultVerbosity()),
  _topology(GetDefaultTopology()),
  _preCADMergeEdges(GetDefaultPreCADMergeEdges()),
  _preCADRemoveDuplicateCADFaces(GetDefaultPreCADRemoveDuplicateCADFaces()),
  _preCADProcess3DTopology(GetDefaultPreCADProcess3DTopology()),
  _preCADDiscardInput(GetDefaultPreCADDiscardInput()),
  _sizeMap(GetDefaultSizeMap()),
  _attractors(GetDefaultSizeMap()),
  _classAttractors(GetDefaultAttractorMap()),
  _faceEntryEnfVertexListMap(GetDefaultFaceEntryEnfVertexListMap()),
  _enfVertexList(GetDefaultEnfVertexList()),
  _faceEntryCoordsListMap(GetDefaultFaceEntryCoordsListMap()),
  _coordsEnfVertexMap(GetDefaultCoordsEnfVertexMap()),
  _faceEntryEnfVertexEntryListMap(GetDefaultFaceEntryEnfVertexEntryListMap()),
  _enfVertexEntryEnfVertexMap(GetDefaultEnfVertexEntryEnfVertexMap()),
  _groupNameNodeIDMap(GetDefaultGroupNameNodeIDMap()),
  _enforcedInternalVerticesAllFaces(GetDefaultInternalEnforcedVertex()),
  _preCadFacesPeriodicityVector(GetDefaultPreCadFacesPeriodicityVector()),
  _preCadEdgesPeriodicityVector(GetDefaultPreCadEdgesPeriodicityVector()),
  _GMFFileName(GetDefaultGMFFile())
{
  _name = GetHypType(hasgeom);
  _param_algo_dim = 2;
  
//   _GMFFileMode = false; // GMF ascii mode

  // Advanced options with their defaults according to MG User Manual

  const char* boolOptionNames[] = {         "enforce_cad_edge_sizes",                   // default = 0
                                            // "correct_surface_intersections",            // default = 1
                                            // "create_tag_on_collision",                  // default = 1
                                            "jacobian_rectification_respect_geometry",  // default = 1
                                            "rectify_jacobian",                         // default = 1
                                            "respect_geometry",                         // default = 1
                                            // "optimise_tiny_edges",                      // default = 0
                                            // "remove_duplicate_cad_faces",               // default = 1
                                            "tiny_edge_avoid_surface_intersections",    // default = 1
                                            "debug",                                    // default = 0 
                                            "use_deprecated_patch_mesher",              // default 0
                                            // "tiny_edge_respect_geometry",               // default = 0
                                            "" // mark of end
      };

  const char* intOptionNames[] = {          "max_number_of_points_per_patch",           // default = 100000
                                            "max_number_of_threads",                    // default = 4
                                            "" // mark of end
      };
  const char* doubleOptionNames[] = {       // "surface_intersections_processing_max_cost",// default = 15
                                            // "periodic_tolerance",                       // default = diag/100
                                            // "volume_gradation",
                                            // "tiny_edge_optimisation_length",            // default = diag * 1e-6
                                            "" // mark of end
      };
  const char* charOptionNames[] = {         // "required_entities",                        // default = "respect"
                                            // "tags",                                     // default = "respect"
                                            "" // mark of end
      };

  // PreCAD advanced options
  const char* preCADboolOptionNames[] = {   "closed_geometry",                          // default = 0
                                            "discard_input_topology",                   // default = 0
                                            "merge_edges",                              // default =  = 1
                                            "remove_duplicate_cad_faces",               // default = 1
                                            // "create_tag_on_collision",                  // default = 1
                                            "process_3d_topology",                      // default = 1
                                            // "remove_tiny_edges",                        // default = 0
                                            // remove_tiny_uv_edges option is not documented
                                            // but it is useful that the user can change it to disable all preprocessing options
                                            "remove_tiny_uv_edges",                        // default = 1
                                            "" // mark of end
      };
  const char* preCADintOptionNames[] = {    // "manifold_geometry",                        // default = 0
                                            "" // mark of end
      };
  const char* preCADdoubleOptionNames[] = { "periodic_tolerance",                       // default = diag * 1e-5
                                            "sewing_tolerance",                         // default = diag * 5e-4
                                            // "tiny_edge_length",                         // default = diag * 1e-5
                                            "" // mark of end
      };
  const char* preCADcharOptionNames[] = {   "required_entities",                        // default = "respect"
                                            "tags",                                     // default = "respect"
                                            "" // mark of end
      };
  
  int i = 0;
  while (boolOptionNames[i][0])
  {
    _boolOptions.insert( boolOptionNames[i] );
    _option2value[boolOptionNames[i++]].clear();
  }
  i = 0;
  while (preCADboolOptionNames[i][0] && hasgeom)
  {
    _boolOptions.insert( preCADboolOptionNames[i] );
    _preCADoption2value[preCADboolOptionNames[i++]].clear();
  }
  i = 0;
  while (intOptionNames[i][0])
    _option2value[intOptionNames[i++]].clear();
  
  i = 0;
  while (preCADintOptionNames[i][0] && hasgeom)
    _preCADoption2value[preCADintOptionNames[i++]].clear();

  i = 0;
  while (doubleOptionNames[i][0]) {
    _doubleOptions.insert(doubleOptionNames[i]);
    _option2value[doubleOptionNames[i++]].clear();
  }
  i = 0;
  while (preCADdoubleOptionNames[i][0] && hasgeom) {
    _preCADdoubleOptions.insert(preCADdoubleOptionNames[i]);
    _preCADoption2value[preCADdoubleOptionNames[i++]].clear();
  }
  i = 0;
  while (charOptionNames[i][0]) {
    _charOptions.insert(charOptionNames[i]);
    _option2value[charOptionNames[i++]].clear();
  }
  i = 0;
  while (preCADcharOptionNames[i][0] && hasgeom) {
    _preCADcharOptions.insert(preCADcharOptionNames[i]);
    _preCADoption2value[preCADcharOptionNames[i++]].clear();
  }

  // default values to be used while MG meshing

  _defaultOptionValues["enforce_cad_edge_sizes"                 ] = "no";
  _defaultOptionValues["jacobian_rectification_respect_geometry"] = "yes";
  _defaultOptionValues["max_number_of_points_per_patch"         ] = "0";
  _defaultOptionValues["max_number_of_threads"                  ] = "4";
  _defaultOptionValues["rectify_jacobian"                       ] = "yes";
  _defaultOptionValues["use_deprecated_patch_mesher"            ] = "yes";
  _defaultOptionValues["respect_geometry"                       ] = "yes";
  _defaultOptionValues["tiny_edge_avoid_surface_intersections"  ] = "yes";
  _defaultOptionValues["use_deprecated_patch_mesher"            ] = "no";
  _defaultOptionValues["debug"                                  ] = "no";
  if ( hasgeom )
  {
    _defaultOptionValues["closed_geometry"                        ] = "no";
    _defaultOptionValues["discard_input_topology"                 ] = "no";
    _defaultOptionValues["merge_edges"                            ] = "no";
    _defaultOptionValues["periodic_tolerance"                     ] = "1e-5*D";
    _defaultOptionValues["process_3d_topology"                    ] = "no";
    _defaultOptionValues["remove_duplicate_cad_faces"             ] = "no";
    _defaultOptionValues["remove_tiny_uv_edges"                   ] = "no";
    _defaultOptionValues["required_entities"                      ] = "respect";
    _defaultOptionValues["sewing_tolerance"                       ] = "5e-4*D";
    _defaultOptionValues["tags"                                   ] = "respect";
  }

#ifdef _DEBUG_
  // check validity of option names of _defaultOptionValues
  TOptionValues::iterator n2v = _defaultOptionValues.begin();
  for ( ; n2v != _defaultOptionValues.end(); ++n2v )
    ASSERT( _option2value.count( n2v->first ) || _preCADoption2value.count( n2v->first ));
  ASSERT( _option2value.size() + _preCADoption2value.size() == _defaultOptionValues.size() );
#endif

  _sizeMap.clear();
  _attractors.clear();
  _faceEntryEnfVertexListMap.clear();
  _enfVertexList.clear();
  _faceEntryCoordsListMap.clear();
  _coordsEnfVertexMap.clear();
  _faceEntryEnfVertexEntryListMap.clear();
  _enfVertexEntryEnfVertexMap.clear();
  _groupNameNodeIDMap.clear();

  /* TODO GROUPS
   _groupNameEnfVertexListMap.clear();
   _enfVertexGroupNameMap.clear();
   */
}

TopoDS_Shape BLSURFPlugin_Hypothesis::entryToShape(std::string entry)
{
  // TODO entryToShape
  TopoDS_Shape S = TopoDS_Shape();
  /*
  GEOM::GEOM_Object_var aGeomObj;
  SMESH_Gen_i* smeshGen_i = SMESH_Gen_i::GetSMESHGen();
  SALOMEDS::Study_ptr myStudy = smeshGen_i->GetCurrentStudy();
  
  TopoDS_Shape S = TopoDS_Shape();
  SALOMEDS::SObject_var aSObj = myStudy->FindObjectID( entry.c_str() );
  if (!aSObj->_is_nil() ) {
    CORBA::Object_var obj = aSObj->GetObject();
    aGeomObj = GEOM::GEOM_Object::_narrow(obj);
    aSObj->UnRegister();
  }
  if ( !aGeomObj->_is_nil() )
    S = smeshGen_i->GeomObjectToShape( aGeomObj.in() );
  */
  return S;
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetPhysicalMesh(PhysicalMesh thePhysicalMesh) {
  if (thePhysicalMesh != _physicalMesh) {
    _physicalMesh = thePhysicalMesh;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetGeometricMesh(GeometricMesh theGeometricMesh) {
  if (theGeometricMesh != _geometricMesh) {
    _geometricMesh = theGeometricMesh;
//     switch (_geometricMesh) {
//       case DefaultGeom:
//       default:
//         _angleMesh = GetDefaultAngleMesh();
//         _gradation = GetDefaultGradation();
//         break;
//     }
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetPhySize(double theVal, bool isRelative) {
  if ((theVal != _phySize) || (isRelative != _phySizeRel)) {
    _phySizeRel = isRelative;
    if (theVal == 0) {
      _phySize = GetMaxSize();
    }
    else
      _phySize = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetMinSize(double theMinSize, bool isRelative) {
  if ((theMinSize != _minSize) || (isRelative != _minSizeRel)) {
    _minSizeRel = isRelative;
    _minSize = theMinSize;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetMaxSize(double theMaxSize, bool isRelative) {
  if ((theMaxSize != _maxSize) || (isRelative != _maxSizeRel)) {
    _maxSizeRel = isRelative;
    _maxSize = theMaxSize;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetUseGradation(bool theVal) {
  if (theVal != _useGradation) {
    _useGradation = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetGradation(double theVal) {
  _useGradation = ( theVal > 0 );
  if (theVal != _gradation) {
    _gradation = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetUseVolumeGradation(bool theVal) {
  if (theVal != _useVolumeGradation) {
    _useVolumeGradation = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetVolumeGradation(double theVal) {
  _useVolumeGradation = ( theVal > 0 );
  if (theVal != _volumeGradation) {
    _volumeGradation = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetQuadAllowed(bool theVal) {
  if (theVal != _quadAllowed) {
    _quadAllowed = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetAngleMesh(double theVal) {
  if (theVal != _angleMesh) {
    _angleMesh = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetChordalError(double theDistance) {
  if (theDistance != _chordalError) {
    _chordalError = theDistance;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetAnisotropic(bool theVal) {
  if (theVal != _anisotropic) {
    _anisotropic = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetAnisotropicRatio(double theVal) {
  if (theVal != _anisotropicRatio) {
    _anisotropicRatio = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetRemoveTinyEdges(bool theVal) {
  if (theVal != _removeTinyEdges) {
    _removeTinyEdges = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetTinyEdgeLength(double theVal) {
  if (theVal != _tinyEdgeLength) {
    _tinyEdgeLength = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetOptimiseTinyEdges(bool theVal) {
  if (theVal != _optimiseTinyEdges) {
    _optimiseTinyEdges = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetTinyEdgeOptimisationLength(double theVal) {
  if (theVal != _tinyEdgeOptimisationLength) {
    _tinyEdgeOptimisationLength = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetCorrectSurfaceIntersection(bool theVal) {
  if (theVal != _correctSurfaceIntersec) {
    _correctSurfaceIntersec = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetCorrectSurfaceIntersectionMaxCost(double theVal) {
  if (theVal != _corrSurfaceIntersCost) {
    _corrSurfaceIntersCost = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetBadElementRemoval(bool theVal) {
  if (theVal != _badElementRemoval) {
    _badElementRemoval = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetBadElementAspectRatio(double theVal) {
  if (theVal != _badElementAspectRatio) {
    _badElementAspectRatio = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetOptimizeMesh(bool theVal) {
  if (theVal != _optimizeMesh) {
    _optimizeMesh = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetQuadraticMesh(bool theVal) {
  if (theVal != _quadraticMesh) {
    _quadraticMesh = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetTopology(Topology theTopology) {
  if (theTopology != _topology) {
    _topology = theTopology;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetVerbosity(int theVal) {
  if (theVal != _verb) {
    _verb = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetEnforceCadEdgesSize( bool toEnforce )
{
  if ( GetEnforceCadEdgesSize() != toEnforce )
  {
    SetOptionValue( "enforce_cad_edge_sizes", toEnforce ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
bool BLSURFPlugin_Hypothesis::GetEnforceCadEdgesSize()
{
  return ToBool( GetOptionValue( "enforce_cad_edge_sizes" ), GET_DEFAULT() );
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetJacobianRectificationRespectGeometry( bool allowRectification )
{
  if ( GetJacobianRectificationRespectGeometry() != allowRectification )
  {
    SetOptionValue("jacobian_rectification_respect_geometry", allowRectification ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
bool BLSURFPlugin_Hypothesis::GetJacobianRectificationRespectGeometry()
{
  return ToBool( GetOptionValue("jacobian_rectification_respect_geometry", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetJacobianRectification( bool allowRectification )
{
  if ( GetJacobianRectification() != allowRectification )
  {
    SetOptionValue( "rectify_jacobian", allowRectification ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
bool BLSURFPlugin_Hypothesis::GetJacobianRectification()
{
  return ToBool( GetOptionValue("rectify_jacobian", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetUseDeprecatedPatchMesher( bool useDeprecatedPatchMesher )
{
  if ( GetUseDeprecatedPatchMesher() != useDeprecatedPatchMesher )
  {
    SetOptionValue( "use_deprecated_patch_mesher", useDeprecatedPatchMesher ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
bool BLSURFPlugin_Hypothesis::GetUseDeprecatedPatchMesher()
{
  return ToBool( GetOptionValue("use_deprecated_patch_mesher", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetMaxNumberOfPointsPerPatch( int nb )
  throw (std::invalid_argument)
{
  if ( nb < 0 )
    throw std::invalid_argument( SMESH_Comment("Invalid number of points: ") << nb );

  if ( GetMaxNumberOfPointsPerPatch() != nb )
  {
    SetOptionValue("max_number_of_points_per_patch", SMESH_Comment( nb ));
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
int BLSURFPlugin_Hypothesis::GetMaxNumberOfPointsPerPatch()
{
  return ToInt( GetOptionValue("max_number_of_points_per_patch", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetMaxNumberOfThreads( int nb )
  throw (std::invalid_argument)
{
  if ( nb < 0 )
    throw std::invalid_argument( SMESH_Comment("Invalid number of threads: ") << nb );

  if ( GetMaxNumberOfThreads() != nb )
  {
    SetOptionValue("max_number_of_threads", SMESH_Comment( nb ));
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
int BLSURFPlugin_Hypothesis::GetMaxNumberOfThreads()
{
  return ToInt( GetOptionValue("max_number_of_threads", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetRespectGeometry( bool toRespect )
{
  if ( GetRespectGeometry() != toRespect )
  {
    SetOptionValue("respect_geometry", toRespect ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
bool BLSURFPlugin_Hypothesis::GetRespectGeometry()
{
  return ToBool( GetOptionValue( "respect_geometry", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetTinyEdgesAvoidSurfaceIntersections( bool toAvoidIntersection )
{
  if ( GetTinyEdgesAvoidSurfaceIntersections() != toAvoidIntersection )
  {
    SetOptionValue("tiny_edge_avoid_surface_intersections", toAvoidIntersection ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
bool BLSURFPlugin_Hypothesis::GetTinyEdgesAvoidSurfaceIntersections()
{
  return ToBool( GetOptionValue("tiny_edge_avoid_surface_intersections", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetClosedGeometry( bool isClosed )
{
  if ( GetClosedGeometry() != isClosed )
  {
    SetPreCADOptionValue("closed_geometry", isClosed ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
bool BLSURFPlugin_Hypothesis::GetClosedGeometry()
{
  return ToBool( GetPreCADOptionValue( "closed_geometry", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetDebug( bool isDebug )
{
  if ( GetDebug() != isDebug )
  {
    SetPreCADOptionValue("debug", isDebug ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
bool BLSURFPlugin_Hypothesis::GetDebug()
{
  return ToBool( GetPreCADOptionValue("debug", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetPeriodicTolerance( double tol )
  throw (std::invalid_argument)
{
  if ( tol <= 0 )
    throw std::invalid_argument( SMESH_Comment("Invalid tolerance: ") << tol );
  if ( GetPeriodicTolerance() != tol )
  {
    SetPreCADOptionValue("periodic_tolerance", SMESH_Comment( tol ) );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
double BLSURFPlugin_Hypothesis::GetPeriodicTolerance()
{
  return ToDbl( GetPreCADOptionValue( "periodic_tolerance", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetRequiredEntities( const std::string& howToTreat )
  throw (std::invalid_argument)
{
  if ( howToTreat != "respect" && howToTreat != "ignore" && howToTreat != "clear"  )
    throw std::invalid_argument
      ( SMESH_Comment("required_entities must be in ['respect','ignore','clear'] "));

  if ( GetRequiredEntities() != howToTreat )
  {
    SetPreCADOptionValue("required_entities", howToTreat );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
std::string BLSURFPlugin_Hypothesis::GetRequiredEntities()
{
  return GetPreCADOptionValue("required_entities", GET_DEFAULT());
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetSewingTolerance( double tol )
  throw (std::invalid_argument)
{
  if ( tol <= 0 )
    throw std::invalid_argument( SMESH_Comment("Invalid tolerance: ") << tol );
  if ( GetSewingTolerance() != tol )
  {
    SetPreCADOptionValue("sewing_tolerance", SMESH_Comment( tol ) );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
double BLSURFPlugin_Hypothesis::GetSewingTolerance()
{
  return ToDbl( GetPreCADOptionValue("sewing_tolerance", GET_DEFAULT()));
}
//=============================================================================

void BLSURFPlugin_Hypothesis::SetTags( const std::string& howToTreat )
  throw (std::invalid_argument)
{
  if ( howToTreat != "respect" && howToTreat != "ignore" && howToTreat != "clear"  )
    throw std::invalid_argument
      ( SMESH_Comment("'tags' must be in ['respect','ignore','clear'] "));

  if ( GetTags() != howToTreat )
  {
    SetPreCADOptionValue("tags", howToTreat );
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
std::string BLSURFPlugin_Hypothesis::GetTags()
{
  return GetPreCADOptionValue("tags", GET_DEFAULT());
}
//=============================================================================
void BLSURFPlugin_Hypothesis::SetHyperPatches(const THyperPatchList& hpl)
{
  if ( hpl != _hyperPatchList )
  {
    // join patches sharing tags
    _hyperPatchList.clear();
    for ( size_t i = 0; i < hpl.size(); ++i )
    {
      const THyperPatchTags& tags = hpl[i];
      if ( tags.size() < 2 ) continue;

      std::set<int> iPatches;
      if ( !_hyperPatchList.empty() )
      {
        THyperPatchTags::iterator t = tags.begin();
        for ( ; t != tags.end(); ++t )
        {
          int iPatch = -1;
          GetHyperPatchTag( *t, this, &iPatch );
          if ( iPatch >= 0 )
            iPatches.insert( iPatch );
        }
      }

      if ( iPatches.empty() )
      {
        _hyperPatchList.push_back( tags );
      }
      else
      {
        std::set<int>::iterator iPatch = iPatches.begin();
        THyperPatchTags&     mainPatch = _hyperPatchList[ *iPatch ];
        mainPatch.insert( tags.begin(), tags.end() );

        for ( ++iPatch; iPatch != iPatches.end(); ++iPatch )
        {
          mainPatch.insert( _hyperPatchList[ *iPatch ].begin(), _hyperPatchList[ *iPatch ].end() );
          _hyperPatchList[ *iPatch ].clear();
        }
        if ( iPatches.size() > 1 )
          for ( int j = _hyperPatchList.size()-1; j > 0; --j )
            if ( _hyperPatchList[j].empty() )
              _hyperPatchList.erase( _hyperPatchList.begin() + j );
      }
    }
    NotifySubMeshesHypothesisModification();
  }
}
//=============================================================================
/*!
 * \brief Return a tag of a face taking into account the hyper-patches. Optionally
 *        return an index of a patch including the face
 */
//================================================================================

int BLSURFPlugin_Hypothesis::GetHyperPatchTag( const int                      faceTag,
                                               const BLSURFPlugin_Hypothesis* hyp,
                                               int*                           iPatch)
{
  if ( hyp )
  {
    const THyperPatchList& hpl = hyp->_hyperPatchList;
    for ( size_t i = 0; i < hpl.size(); ++i )
      if ( hpl[i].count( faceTag ))
      {
        if ( iPatch ) *iPatch = i;
        return *( hpl[i].begin() );
      }
  }
  return faceTag;
}
//=============================================================================
void BLSURFPlugin_Hypothesis::SetPreCADMergeEdges(bool theVal)
{
  if (theVal != ToBool( GetPreCADOptionValue("merge_edges", GET_DEFAULT()))) {
    _preCADMergeEdges = theVal;
    SetPreCADOptionValue("merge_edges", theVal ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetPreCADRemoveDuplicateCADFaces(bool theVal)
{
  if (theVal != ToBool( GetPreCADOptionValue("remove_duplicate_cad_faces", GET_DEFAULT()))) {
    _preCADRemoveDuplicateCADFaces = theVal;
    SetPreCADOptionValue("remove_duplicate_cad_faces", theVal ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetPreCADProcess3DTopology(bool theVal)
{
  if (theVal != ToBool( GetPreCADOptionValue("process_3d_topology", GET_DEFAULT()))) {
    _preCADProcess3DTopology = theVal;
    SetPreCADOptionValue("process_3d_topology", theVal ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetPreCADDiscardInput(bool theVal)
{
  if (theVal != ToBool( GetPreCADOptionValue("discard_input_topology", GET_DEFAULT()))) {
    _preCADDiscardInput = theVal;
    SetPreCADOptionValue("discard_input_topology", theVal ? "yes" : "no" );
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
// Return true if any PreCAD option is activated
bool BLSURFPlugin_Hypothesis::HasPreCADOptions(const BLSURFPlugin_Hypothesis* hyp)
{
  if ( !hyp || hyp->_name == GetHypType(/*hasgeom=*/false))
  {
    return false;
  }
  bool orDefault, isOk;
  return ( ToBool( hyp->GetPreCADOptionValue("closed_geometry"           , &orDefault )) ||
           ToBool( hyp->GetPreCADOptionValue("discard_input_topology"    , &orDefault )) ||
           ToBool( hyp->GetPreCADOptionValue("merge_edges"               , &orDefault )) ||
           ToBool( hyp->GetPreCADOptionValue("remove_duplicate_cad_faces", &orDefault )) ||
           ToBool( hyp->GetPreCADOptionValue("process_3d_topology"       , &orDefault )) ||
           ToBool( hyp->GetPreCADOption     ("manifold_geometry")        , &isOk       ) ||
           hyp->GetPreCADOptionValue("sewing_tolerance", &orDefault ) != "5e-4*D"        ||
           !hyp->_preCadFacesPeriodicityVector.empty()                                   ||
           !hyp->_preCadEdgesPeriodicityVector.empty()                                   ||
           !hyp->_facesPeriodicityVector.empty()                                         ||
           !hyp->_edgesPeriodicityVector.empty()                                         ||
           !hyp->_verticesPeriodicityVector.empty()                                      ||
           !hyp->GetHyperPatches().empty()                                               ||
           hyp->GetTopology() != FromCAD );
}

//=============================================================================
// void BLSURFPlugin_Hypothesis::SetGMFFile(const std::string& theFileName, bool isBinary)
void BLSURFPlugin_Hypothesis::SetGMFFile(const std::string& theFileName)
{
  _GMFFileName = theFileName;
  //   _GMFFileMode = isBinary;
  NotifySubMeshesHypothesisModification();
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetOptionValue(const std::string& optionName, const std::string& optionValue)
  throw (std::invalid_argument) {

  TOptionValues::iterator op_val = _option2value.find(optionName);
  if (op_val == _option2value.end())
  {
    op_val = _preCADoption2value.find(optionName);
    if (op_val == _preCADoption2value.end())
    {
      std::string msg = "Unknown MG-CADSurf option: '" + optionName + "'. Try SetAdvancedOption()";
      throw std::invalid_argument(msg);
    }
  }
  if (op_val->second != optionValue)
  {
    const char* ptr = optionValue.c_str();
    // strip white spaces
    while (ptr[0] == ' ')
      ptr++;
    int i = strlen(ptr);
    while (i != 0 && ptr[i - 1] == ' ')
      i--;
    // check value type
    bool typeOk = true;
    std::string typeName;
    if (i == 0) {
      // empty string
    } else if (_charOptions.count(optionName)) {
      // do not check strings
    } else if (_doubleOptions.count(optionName)) {
      // check if value is double
      ToDbl(ptr, &typeOk);
      typeName = "real";
    } else if (_boolOptions.count(optionName)) {
      // check if value is bool
      ToBool(ptr, &typeOk);
      typeName = "bool";
    } else {
      // check if value is int
      ToInt(ptr, &typeOk);
      typeName = "integer";
    }
    if (!typeOk) {
      std::string msg = "Advanced option '" + optionName + "' = '" + optionValue + "' but must be " + typeName;
      throw std::invalid_argument(msg);
    }
    std::string value( ptr, i );
    if ( _defaultOptionValues[ optionName ] == value )
      value.clear();

    op_val->second = value;

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::SetPreCADOptionValue(const std::string& optionName, const std::string& optionValue)
  throw (std::invalid_argument) {

  TOptionValues::iterator op_val = _preCADoption2value.find(optionName);
  if (op_val == _preCADoption2value.end()) {
    op_val = _option2value.find(optionName);
    if (op_val == _option2value.end()) {
      std::string msg = "Unknown MG-PreCAD option: '" + optionName + "'. Try SetAdvancedOption()";
      throw std::invalid_argument(msg);
    }
  }
  if (op_val->second != optionValue)
  {
    const char* ptr = optionValue.c_str();
    // strip white spaces
    while (ptr[0] == ' ')
      ptr++;
    int i = strlen(ptr);
    while (i != 0 && ptr[i - 1] == ' ')
      i--;
    // check value type
    bool typeOk = true;
    std::string typeName;
    if (i == 0) {
      // empty string
    } else if (_preCADcharOptions.find(optionName) != _preCADcharOptions.end()) {
      // do not check strings
    } else if (_preCADdoubleOptions.find(optionName) != _preCADdoubleOptions.end()) {
      // check if value is double
      char * endPtr;
      strtod(ptr, &endPtr);
      typeOk = (ptr != endPtr);
      typeName = "real";
    } else if (_boolOptions.count(optionName)) {
      // check if value is bool
      ToBool(ptr, &typeOk);
      typeName = "bool";
    } else {
      // check if value is int
      char * endPtr;
      strtol(ptr, &endPtr, 10);
      typeOk = (ptr != endPtr);
      typeName = "integer";
    }
    if (!typeOk) {
      std::string msg = "PreCAD advanced option '" + optionName + "' = '" + optionValue + "' but must be " + typeName;
      throw std::invalid_argument(msg);
    }
    std::string value( ptr, i );
    if ( _defaultOptionValues[ optionName ] == value )
      value.clear();

    op_val->second = value;

    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
std::string BLSURFPlugin_Hypothesis::GetOptionValue(const std::string& optionName,
                                                    bool*              isDefault) const
  throw (std::invalid_argument)
{
  TOptionValues::const_iterator op_val = _option2value.find(optionName);
  if (op_val == _option2value.end())
  {
    op_val = _preCADoption2value.find(optionName);
    if (op_val == _preCADoption2value.end())
    {
      op_val = _customOption2value.find(optionName);
      if (op_val == _customOption2value.end())
      {
        std::string msg = "Unknown MG-CADSurf option: <" + optionName + ">";
        throw std::invalid_argument(msg);
      }
    }
  }
  std::string val = op_val->second;
  if ( isDefault ) *isDefault = ( val.empty() );

  if ( val.empty() && isDefault )
  {
    op_val = _defaultOptionValues.find( optionName );
    if (op_val != _defaultOptionValues.end())
      val = op_val->second;
  }
  return val;
}

//=============================================================================
std::string BLSURFPlugin_Hypothesis::GetPreCADOptionValue(const std::string& optionName,
                                                          bool*              isDefault) const
  throw (std::invalid_argument)
{
  TOptionValues::const_iterator op_val = _preCADoption2value.find(optionName);
  if (op_val == _preCADoption2value.end())
  {
    op_val = _option2value.find(optionName);
    if (op_val == _option2value.end())
    {
      op_val = _customOption2value.find(optionName);
      if (op_val == _customOption2value.end())
      {
        std::string msg = "Unknown MG-CADSurf option: <" + optionName + ">";
        throw std::invalid_argument(msg);
      }
    }
  }
  std::string val = op_val->second;
  if ( isDefault ) *isDefault = ( val.empty() );

  if ( val.empty() && isDefault )
  {
    op_val = _defaultOptionValues.find( optionName );
    if (op_val != _option2value.end())
      val = op_val->second;
  }
  return val;
}

//=============================================================================
void BLSURFPlugin_Hypothesis::ClearOption(const std::string& optionName)
{
  TOptionValues::iterator op_val = _customOption2value.find(optionName);
  if (op_val != _customOption2value.end())
   _customOption2value.erase(op_val);
  else {
    op_val = _option2value.find(optionName);
    if (op_val != _option2value.end())
      op_val->second.clear();
    else {
      op_val = _preCADoption2value.find(optionName);
      if (op_val != _preCADoption2value.end())
        op_val->second.clear();
    }
  }
}

//=============================================================================
void BLSURFPlugin_Hypothesis::ClearPreCADOption(const std::string& optionName)
{
  TOptionValues::iterator op_val = _preCADoption2value.find(optionName);
  if (op_val != _preCADoption2value.end())
    op_val->second.clear();
}

//=============================================================================
void BLSURFPlugin_Hypothesis::AddOption(const std::string& optionName, const std::string& optionValue)
{
  bool modif = true;
  TOptionValues::iterator op_val = _option2value.find(optionName);
  if (op_val != _option2value.end())
  {
    if (op_val->second != optionValue)
      op_val->second = optionValue;
    else
      modif = false;
  }
  else
  {
    op_val = _preCADoption2value.find(optionName);
    if (op_val != _preCADoption2value.end())
    {
      if (op_val->second != optionValue)
        op_val->second = optionValue;
      else
        modif = false;
    }
    else if ( optionValue.empty() )
    {
      _customOption2value.erase( optionName );
    }
    else
    {
      op_val = _customOption2value.find(optionName);
      if (op_val == _customOption2value.end())
        _customOption2value[optionName] = optionValue;
      else if (op_val->second != optionValue)
        op_val->second = optionValue;
      else
        modif = false;
    }
  }
  if ( modif )
    NotifySubMeshesHypothesisModification();
}

//=============================================================================
void BLSURFPlugin_Hypothesis::AddPreCADOption(const std::string& optionName, const std::string& optionValue)
{
  AddOption( optionName, optionValue );
}

//=============================================================================
std::string BLSURFPlugin_Hypothesis::GetOption(const std::string& optionName) const
{
  TOptionValues::const_iterator op_val = _customOption2value.find(optionName);
  if (op_val != _customOption2value.end())
    return op_val->second;
  else
    return "";
}

//=============================================================================
std::string BLSURFPlugin_Hypothesis::GetPreCADOption(const std::string& optionName) const
{
  TOptionValues::const_iterator op_val = _customOption2value.find(optionName);
  if (op_val != _customOption2value.end())
    return op_val->second;
  else
    return "";
}

//=============================================================================
BLSURFPlugin_Hypothesis::TOptionValues BLSURFPlugin_Hypothesis::GetOptionValues() const
{
  TOptionValues vals;
  TOptionValues::const_iterator op_val = _option2value.begin();
  for ( ; op_val != _option2value.end(); ++op_val )
    vals.insert( make_pair( op_val->first, GetOptionValue( op_val->first, GET_DEFAULT() )));

  return vals;
}

//=============================================================================
BLSURFPlugin_Hypothesis::TOptionValues BLSURFPlugin_Hypothesis::GetPreCADOptionValues() const
{
  TOptionValues vals;
  TOptionValues::const_iterator op_val = _preCADoption2value.begin();
  for ( ; op_val != _preCADoption2value.end(); ++op_val )
    vals.insert( make_pair( op_val->first, GetPreCADOptionValue( op_val->first, GET_DEFAULT() )));

  return vals;
}

//=======================================================================
//function : SetSizeMapEntry
//=======================================================================
void BLSURFPlugin_Hypothesis::SetSizeMapEntry(const std::string& entry, const std::string& sizeMap) {
  if (_sizeMap[entry].compare(sizeMap) != 0) {
    SetPhysicalMesh(PhysicalLocalSize);
    _sizeMap[entry] = sizeMap;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetSizeMapEntry
//=======================================================================
std::string BLSURFPlugin_Hypothesis::GetSizeMapEntry(const std::string& entry) {
  TSizeMap::iterator it = _sizeMap.find(entry);
  if (it != _sizeMap.end())
    return it->second;
  else
    return "No_Such_Entry";
}

/*!
 * \brief Return the size maps
 */
BLSURFPlugin_Hypothesis::TSizeMap BLSURFPlugin_Hypothesis::GetSizeMapEntries(const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetSizeMapEntries() : GetDefaultSizeMap();
}

//=======================================================================
//function : SetAttractorEntry
//=======================================================================
void BLSURFPlugin_Hypothesis::SetAttractorEntry(const std::string& entry, const std::string& attractor) {
  if (_attractors[entry].compare(attractor) != 0) {
    SetPhysicalMesh(PhysicalLocalSize);
    _attractors[entry] = attractor;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : GetAttractorEntry
//=======================================================================
std::string BLSURFPlugin_Hypothesis::GetAttractorEntry(const std::string& entry) {
  TSizeMap::iterator it = _attractors.find(entry);
  if (it != _attractors.end())
    return it->second;
  else
    return "No_Such_Entry";
}

/*!
 * \brief Return the attractors
 */
BLSURFPlugin_Hypothesis::TSizeMap BLSURFPlugin_Hypothesis::GetAttractorEntries(const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetAttractorEntries() : GetDefaultSizeMap();
}

//=======================================================================
//function : SetClassAttractorEntry
//=======================================================================
void BLSURFPlugin_Hypothesis::SetClassAttractorEntry(const std::string& entry, const std::string& attEntry, double StartSize, double EndSize, double ActionRadius, double ConstantRadius)
{
  SetPhysicalMesh(PhysicalLocalSize);

  // The new attractor can't be defined on the same face as another sizemap
  TSizeMap::iterator it  = _sizeMap.find( entry );
  if ( it != _sizeMap.end() ) {
    _sizeMap.erase(it);
    NotifySubMeshesHypothesisModification();
  }
  else {
    TSizeMap::iterator itAt  = _attractors.find( entry );
    if ( itAt != _attractors.end() ) {
      _attractors.erase(itAt);
      NotifySubMeshesHypothesisModification();
    }
  }
  
  const TopoDS_Shape AttractorShape = BLSURFPlugin_Hypothesis::entryToShape(attEntry);
  const TopoDS_Face FaceShape = TopoDS::Face(BLSURFPlugin_Hypothesis::entryToShape(entry));
  TAttractorMap::iterator attIt = _classAttractors.find(entry);
  for ( ; attIt != _classAttractors.end(); ++attIt )
    if ( attIt->first == entry && 
         attIt->second->GetAttractorEntry() == attEntry )
      break;
  bool attExists = (attIt != _classAttractors.end());

  BLSURFPlugin_Attractor* myAttractor;
  if ( !attExists ) {
    myAttractor = new BLSURFPlugin_Attractor(FaceShape, AttractorShape, attEntry);//, 0.1 );
    _classAttractors.insert( make_pair( entry, myAttractor ));
  }
  else {
    myAttractor = attIt->second;
  }
  // if (!myAttractor->IsMapBuilt())
  //   myAttractor->BuildMap();
  myAttractor->SetParameters(StartSize, EndSize, ActionRadius, ConstantRadius);

  NotifySubMeshesHypothesisModification();
}

//=======================================================================
//function : SetConstantSizeOnAdjacentFaces
//=======================================================================
// TODO uncomment and test (include the needed .hxx)
// SetConstantSizeOnAdjacentFaces(myShape, att_entry, startSize, endSize = user_size, const_dist  ) {
//   TopTools_IndexedMapOfShapListOdShape anEdge2FaceMap;
//   TopExp::MapShapesAnAncestors(myShape,TopAbs_EDGE, TopAbs_FACE, anEdge2FaceMap);
//   TopTools_IndexedMapOfShapListOdShape::iterator it;
//   for (it = anEdge2FaceMap.begin();it != anEdge2FaceMap.end();it++){
//       SetClassAttractorEntry((*it).first, att_entry, startSize, endSize, 0, const_dist)
//   }






//=======================================================================
//function : GetClassAttractorEntry
//=======================================================================
// BLSURFPlugin_Attractor&  BLSURFPlugin_Hypothesis::GetClassAttractorEntry(const std::string& entry)
// {
//  TAttractorMap::iterator it  = _classAttractors.find( entry );
//  if ( it != _classAttractors.end() )
//    return it->second;
//  else
//    return "No_Such_Entry";
// }
// 
  /*!
   * \brief Return the map of attractor instances
   */
BLSURFPlugin_Hypothesis::TAttractorMap BLSURFPlugin_Hypothesis::GetClassAttractorEntries(const BLSURFPlugin_Hypothesis* hyp)
{
    return hyp ? hyp->_GetClassAttractorEntries():GetDefaultAttractorMap();
}

//=======================================================================
//function : ClearEntry
//=======================================================================
void BLSURFPlugin_Hypothesis::ClearEntry(const std::string& entry,
                                         const char * attEntry/*=0*/)
{
 TSizeMap::iterator it  = _sizeMap.find( entry );
 
 if ( it != _sizeMap.end() ) {
   _sizeMap.erase(it);
   NotifySubMeshesHypothesisModification();
 }
 else {
   TSizeMap::iterator itAt  = _attractors.find( entry );
   if ( itAt != _attractors.end() ) {
     _attractors.erase(itAt);
     NotifySubMeshesHypothesisModification();
   }
   else {
     TAttractorMap::iterator it_clAt = _classAttractors.find( entry );
     if ( it_clAt != _classAttractors.end() ) {
       do {
         if ( !attEntry || it_clAt->second->GetAttractorEntry() == attEntry )
           _classAttractors.erase( it_clAt++ );
         else
           ++it_clAt;
       }
       while ( it_clAt != _classAttractors.end() );
       NotifySubMeshesHypothesisModification();
     }
     else
       std::cout<<"No_Such_Entry"<<std::endl;
   }
 }
}

//=======================================================================
//function : ClearSizeMaps
//=======================================================================
void BLSURFPlugin_Hypothesis::ClearSizeMaps() {
  _sizeMap.clear();
  _attractors.clear();
  _classAttractors.clear();
}

// Enable internal enforced vertices on specific face if requested by user

////=======================================================================
////function : SetInternalEnforcedVertex
////=======================================================================
//void BLSURFPlugin_Hypothesis::SetInternalEnforcedVertex(TEntry theFaceEntry,
//                                                        bool toEnforceInternalVertices,
//                                                        TEnfGroupName theGroupName) {

//      << toEnforceInternalVertices << ", " << theGroupName << ")");
  
//  TFaceEntryInternalVerticesList::iterator it = _faceEntryInternalVerticesList.find(theFaceEntry);
//  if (it != _faceEntryInternalVerticesList.end()) {
//    if (!toEnforceInternalVertices) {
//      _faceEntryInternalVerticesList.erase(it);
//    }
//  }
//  else {
//    if (toEnforceInternalVertices) {
//      _faceEntryInternalVerticesList.insert(theFaceEntry);
//    }
//  }
  
//  // TODO
//  // Take care of groups
//}


//=======================================================================
//function : SetEnforcedVertex
//=======================================================================
bool BLSURFPlugin_Hypothesis::SetEnforcedVertex(TEntry        theFaceEntry,
                                                TEnfName      theVertexName,
                                                TEntry        theVertexEntry,
                                                TEnfGroupName theGroupName,
                                                double x, double y, double z)
{
  SetPhysicalMesh(PhysicalLocalSize);

  bool toNotify = false;
  bool toCreate = true;

  TEnfVertex *oldEnVertex;
  TEnfVertex *newEnfVertex = new TEnfVertex();
  newEnfVertex->name = theVertexName;
  newEnfVertex->geomEntry = theVertexEntry;
  newEnfVertex->coords.clear();
  if (theVertexEntry == "") {
    newEnfVertex->coords.push_back(x);
    newEnfVertex->coords.push_back(y);
    newEnfVertex->coords.push_back(z);
  }
  newEnfVertex->grpName = theGroupName;
  newEnfVertex->faceEntries.clear();
  newEnfVertex->faceEntries.insert(theFaceEntry);


  // update _enfVertexList
  TEnfVertexList::iterator it = _enfVertexList.find(newEnfVertex);
  if (it != _enfVertexList.end()) {
    toCreate = false;
    oldEnVertex = (*it);
    if (oldEnVertex->name != theVertexName) {
      oldEnVertex->name = theVertexName;
      toNotify = true;
    }
    if (oldEnVertex->grpName != theGroupName) {
      oldEnVertex->grpName = theGroupName;
      toNotify = true;
    }
    TEntryList::iterator it_faceEntries = oldEnVertex->faceEntries.find(theFaceEntry);
    if (it_faceEntries == oldEnVertex->faceEntries.end()) {
      oldEnVertex->faceEntries.insert(theFaceEntry);
      _faceEntryEnfVertexListMap[theFaceEntry].insert(oldEnVertex);
      toNotify = true;
    }
    if (toNotify) {
      // update map coords / enf vertex if needed
      if (oldEnVertex->coords.size()) {
        _coordsEnfVertexMap[oldEnVertex->coords] = oldEnVertex;
        _faceEntryCoordsListMap[theFaceEntry].insert(oldEnVertex->coords);
      }

      // update map geom entry / enf vertex if needed
      if (oldEnVertex->geomEntry != "") {
        _enfVertexEntryEnfVertexMap[oldEnVertex->geomEntry] = oldEnVertex;
        _faceEntryEnfVertexEntryListMap[theFaceEntry].insert(oldEnVertex->geomEntry);
      }
    }
  }

//   //////// CREATE ////////////
  if (toCreate) {
    toNotify = true;
    _faceEntryEnfVertexListMap[theFaceEntry].insert(newEnfVertex);
    _enfVertexList.insert(newEnfVertex);
    if (theVertexEntry == "") {
      _faceEntryCoordsListMap[theFaceEntry].insert(newEnfVertex->coords);
      _coordsEnfVertexMap[newEnfVertex->coords] = newEnfVertex;
    }
    else {
      _faceEntryEnfVertexEntryListMap[theFaceEntry].insert(newEnfVertex->geomEntry);
      _enfVertexEntryEnfVertexMap[newEnfVertex->geomEntry] = newEnfVertex;
    }
  }

  if (toNotify)
    NotifySubMeshesHypothesisModification();

  return toNotify;
}


//=======================================================================
//function : GetEnforcedVertices
//=======================================================================

BLSURFPlugin_Hypothesis::TEnfVertexList BLSURFPlugin_Hypothesis::GetEnfVertexList(const TEntry& theFaceEntry)
    throw (std::invalid_argument) {

  if (_faceEntryEnfVertexListMap.count(theFaceEntry) > 0)
    return _faceEntryEnfVertexListMap[theFaceEntry];
  else
    return GetDefaultEnfVertexList();

  std::ostringstream msg;
  msg << "No enforced vertex for face entry " << theFaceEntry;
  throw std::invalid_argument(msg.str());
}

//=======================================================================
//function : GetEnfVertexCoordsList
//=======================================================================

BLSURFPlugin_Hypothesis::TEnfVertexCoordsList BLSURFPlugin_Hypothesis::GetEnfVertexCoordsList(
    const TEntry& theFaceEntry) throw (std::invalid_argument) {

  if (_faceEntryCoordsListMap.count(theFaceEntry) > 0)
    return _faceEntryCoordsListMap[theFaceEntry];

  std::ostringstream msg;
  msg << "No enforced vertex coords for face entry " << theFaceEntry;
  throw std::invalid_argument(msg.str());
}

//=======================================================================
//function : GetEnfVertexEntryList
//=======================================================================

BLSURFPlugin_Hypothesis::TEntryList BLSURFPlugin_Hypothesis::GetEnfVertexEntryList(const TEntry& theFaceEntry)
    throw (std::invalid_argument) {

  if (_faceEntryEnfVertexEntryListMap.count(theFaceEntry) > 0)
    return _faceEntryEnfVertexEntryListMap[theFaceEntry];

  std::ostringstream msg;
  msg << "No enforced vertex entry for face entry " << theFaceEntry;
  throw std::invalid_argument(msg.str());
}

//=======================================================================
//function : GetEnfVertex(TEnfVertexCoords coords)
//=======================================================================

BLSURFPlugin_Hypothesis::TEnfVertex* BLSURFPlugin_Hypothesis::GetEnfVertex(TEnfVertexCoords coords)
    throw (std::invalid_argument) {

  if (_coordsEnfVertexMap.count(coords) > 0)
    return _coordsEnfVertexMap[coords];

  std::ostringstream msg;
  msg << "No enforced vertex with coords (" << coords[0] << ", " << coords[1] << ", " << coords[2] << ")";
  throw std::invalid_argument(msg.str());
}

//=======================================================================
//function : GetEnfVertex(const TEntry& theEnfVertexEntry)
//=======================================================================

BLSURFPlugin_Hypothesis::TEnfVertex* BLSURFPlugin_Hypothesis::GetEnfVertex(const TEntry& theEnfVertexEntry)
    throw (std::invalid_argument) {

  if (_enfVertexEntryEnfVertexMap.count(theEnfVertexEntry) > 0)
    return _enfVertexEntryEnfVertexMap[theEnfVertexEntry];

  std::ostringstream msg;
  msg << "No enforced vertex with entry " << theEnfVertexEntry;
  throw std::invalid_argument(msg.str());
}

//Enable internal enforced vertices on specific face if requested by user
////=======================================================================
////function : GetInternalEnforcedVertex
////=======================================================================

//bool BLSURFPlugin_Hypothesis::GetInternalEnforcedVertex(const TEntry& theFaceEntry)
//{
//  if (_faceEntryInternalVerticesList.count(theFaceEntry) > 0)
//    return true;
//  return false;
//}

//=======================================================================
//function : ClearEnforcedVertex
//=======================================================================

bool BLSURFPlugin_Hypothesis::ClearEnforcedVertex(const TEntry& theFaceEntry, double x, double y, double z,
    const TEntry& theVertexEntry) throw (std::invalid_argument) {

  bool toNotify = false;
  std::ostringstream msg;
  TEnfVertex *oldEnfVertex;
  TEnfVertexCoords coords;
  coords.clear();
  coords.push_back(x);
  coords.push_back(y);
  coords.push_back(z);

  // check that enf vertex with given enf vertex entry exists
  TEnfVertexEntryEnfVertexMap::iterator it_enfVertexEntry = _enfVertexEntryEnfVertexMap.find(theVertexEntry);
  if (it_enfVertexEntry != _enfVertexEntryEnfVertexMap.end()) {
    // Success
    oldEnfVertex = it_enfVertexEntry->second;

    _enfVertexEntryEnfVertexMap.erase(it_enfVertexEntry);

    TEntryList& enfVertexEntryList = _faceEntryEnfVertexEntryListMap[theFaceEntry];
    enfVertexEntryList.erase(theVertexEntry);
    if (enfVertexEntryList.size() == 0)
      _faceEntryEnfVertexEntryListMap.erase(theFaceEntry);
    //    TFaceEntryEnfVertexEntryListMap::iterator it_entry_entry = _faceEntryEnfVertexEntryListMap.find(theFaceEntry);
    //    TEntryList::iterator it_entryList = it_entry_entry->second.find(theVertexEntry);
    //    it_entry_entry->second.erase(it_entryList);
    //    if (it_entry_entry->second.size() == 0)
    //      _faceEntryEnfVertexEntryListMap.erase(it_entry_entry);
  } else {
    // Fail
    MESSAGE("Enforced vertex with geom entry " << theVertexEntry << " not found");
    msg << "No enforced vertex with geom entry " << theVertexEntry;
    // check that enf vertex with given coords exists
    TCoordsEnfVertexMap::iterator it_coords_enf = _coordsEnfVertexMap.find(coords);
    if (it_coords_enf != _coordsEnfVertexMap.end()) {
      // Success
      oldEnfVertex = it_coords_enf->second;

      _coordsEnfVertexMap.erase(it_coords_enf);

      TEnfVertexCoordsList& enfVertexCoordsList = _faceEntryCoordsListMap[theFaceEntry];
      enfVertexCoordsList.erase(coords);
      if (enfVertexCoordsList.size() == 0)
        _faceEntryCoordsListMap.erase(theFaceEntry);
      //      TFaceEntryCoordsListMap::iterator it_entry_coords = _faceEntryCoordsListMap.find(theFaceEntry);
      //      TEnfVertexCoordsList::iterator it_coordsList = it_entry_coords->second.find(coords);
      //      it_entry_coords->second.erase(it_coordsList);
      //      if (it_entry_coords->second.size() == 0)
      //        _faceEntryCoordsListMap.erase(it_entry_coords);
    } else {
      // Fail
      MESSAGE("Enforced vertex with coords " << x << ", " << y << ", " << z << " not found");
      msg << std::endl;
      msg << "No enforced vertex at " << x << ", " << y << ", " << z;
      throw std::invalid_argument(msg.str());
    }
  }

  // update _enfVertexList
  TEnfVertexList::iterator it = _enfVertexList.find(oldEnfVertex);
  if (it != _enfVertexList.end()) {
    (*it)->faceEntries.erase(theFaceEntry);
    if ((*it)->faceEntries.size() == 0){
      _enfVertexList.erase(it);
      toNotify = true;
    }
  }

  // update _faceEntryEnfVertexListMap
  TEnfVertexList& currentEnfVertexList = _faceEntryEnfVertexListMap[theFaceEntry];
  currentEnfVertexList.erase(oldEnfVertex);

  if (currentEnfVertexList.size() == 0) {
    _faceEntryEnfVertexListMap.erase(theFaceEntry);
  }

  if (toNotify)
    NotifySubMeshesHypothesisModification();

  return toNotify;
}

//=======================================================================
//function : ClearEnforcedVertices
//=======================================================================

bool BLSURFPlugin_Hypothesis::ClearEnforcedVertices(const TEntry& theFaceEntry) throw (std::invalid_argument) {

  bool toNotify = false;
  TEnfVertex *oldEnfVertex;

  TFaceEntryCoordsListMap::iterator it_entry_coords = _faceEntryCoordsListMap.find(theFaceEntry);
  if (it_entry_coords != _faceEntryCoordsListMap.end()) {
    toNotify = true;
    TEnfVertexCoordsList coordsList = it_entry_coords->second;
    TEnfVertexCoordsList::iterator it_coordsList = coordsList.begin();
    for (; it_coordsList != coordsList.end(); ++it_coordsList) {
      TEnfVertexCoords coords = (*it_coordsList);
      oldEnfVertex = _coordsEnfVertexMap[coords];
      _coordsEnfVertexMap.erase(coords);
      // update _enfVertexList
      TEnfVertexList::iterator it = _enfVertexList.find(oldEnfVertex);
      if (it != _enfVertexList.end()) {
        (*it)->faceEntries.erase(theFaceEntry);
        if ((*it)->faceEntries.size() == 0){
          _enfVertexList.erase(it);
          toNotify = true;
        }
      }
    }
    _faceEntryCoordsListMap.erase(it_entry_coords);
    _faceEntryEnfVertexListMap.erase(theFaceEntry);
  }

  TFaceEntryEnfVertexEntryListMap::iterator it_entry_entry = _faceEntryEnfVertexEntryListMap.find(theFaceEntry);
  if (it_entry_entry != _faceEntryEnfVertexEntryListMap.end()) {
    toNotify = true;
    TEntryList enfVertexEntryList = it_entry_entry->second;
    TEntryList::iterator it_enfVertexEntryList = enfVertexEntryList.begin();
    for (; it_enfVertexEntryList != enfVertexEntryList.end(); ++it_enfVertexEntryList) {
      TEntry enfVertexEntry = (*it_enfVertexEntryList);
      oldEnfVertex = _enfVertexEntryEnfVertexMap[enfVertexEntry];
      _enfVertexEntryEnfVertexMap.erase(enfVertexEntry);
      // update _enfVertexList
      TEnfVertexList::iterator it = _enfVertexList.find(oldEnfVertex);
      if (it != _enfVertexList.end()) {
        (*it)->faceEntries.erase(theFaceEntry);
        if ((*it)->faceEntries.size() == 0){
          _enfVertexList.erase(it);
          toNotify = true;
        }
      }
    }
    _faceEntryEnfVertexEntryListMap.erase(it_entry_entry);
    _faceEntryEnfVertexListMap.erase(theFaceEntry);
  }

  if (toNotify)
    NotifySubMeshesHypothesisModification();

  return toNotify;
  //  std::ostringstream msg;
  //  msg << "No enforced vertex for " << theFaceEntry;
  //  throw std::invalid_argument(msg.str());
}

//=======================================================================
//function : ClearAllEnforcedVertices
//=======================================================================
void BLSURFPlugin_Hypothesis::ClearAllEnforcedVertices() {
  _faceEntryEnfVertexListMap.clear();
  _enfVertexList.clear();
  _faceEntryCoordsListMap.clear();
  _coordsEnfVertexMap.clear();
  _faceEntryEnfVertexEntryListMap.clear();
  _enfVertexEntryEnfVertexMap.clear();
//  Enable internal enforced vertices on specific face if requested by user
//  _faceEntryInternalVerticesList.clear();
  NotifySubMeshesHypothesisModification();
}

//================================================================================
/*!
 * \brief Return the enforced vertices
 */
//================================================================================


BLSURFPlugin_Hypothesis::TFaceEntryEnfVertexListMap BLSURFPlugin_Hypothesis::GetAllEnforcedVerticesByFace(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetAllEnforcedVerticesByFace() : GetDefaultFaceEntryEnfVertexListMap();
}

//Enable internal enforced vertices on specific face if requested by user
//BLSURFPlugin_Hypothesis::TFaceEntryInternalVerticesList BLSURFPlugin_Hypothesis::GetAllInternalEnforcedVerticesByFace(
//    const BLSURFPlugin_Hypothesis* hyp) {
//  return hyp ? hyp->_GetAllInternalEnforcedVerticesByFace() : GetDefaultFaceEntryInternalVerticesMap();
//}

bool BLSURFPlugin_Hypothesis::GetInternalEnforcedVertexAllFaces(const BLSURFPlugin_Hypothesis* hyp)
{
  return hyp ? hyp->_GetInternalEnforcedVertexAllFaces() : GetDefaultInternalEnforcedVertex();
}

BLSURFPlugin_Hypothesis::TEnfGroupName BLSURFPlugin_Hypothesis::GetInternalEnforcedVertexAllFacesGroup(const BLSURFPlugin_Hypothesis* hyp)
{
  return hyp ? hyp->_GetInternalEnforcedVertexAllFacesGroup() : BLSURFPlugin_Hypothesis::TEnfGroupName();
}

BLSURFPlugin_Hypothesis::TEnfVertexList BLSURFPlugin_Hypothesis::GetAllEnforcedVertices(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetAllEnforcedVertices() : GetDefaultEnfVertexList();
}

BLSURFPlugin_Hypothesis::TFaceEntryCoordsListMap BLSURFPlugin_Hypothesis::GetAllCoordsByFace(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetAllCoordsByFace() : GetDefaultFaceEntryCoordsListMap();
}

BLSURFPlugin_Hypothesis::TCoordsEnfVertexMap BLSURFPlugin_Hypothesis::GetAllEnforcedVerticesByCoords(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetAllEnforcedVerticesByCoords() : GetDefaultCoordsEnfVertexMap();
}

BLSURFPlugin_Hypothesis::TFaceEntryEnfVertexEntryListMap BLSURFPlugin_Hypothesis::GetAllEnfVertexEntriesByFace(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetAllEnfVertexEntriesByFace() : GetDefaultFaceEntryEnfVertexEntryListMap();
}

BLSURFPlugin_Hypothesis::TEnfVertexEntryEnfVertexMap BLSURFPlugin_Hypothesis::GetAllEnforcedVerticesByEnfVertexEntry(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetAllEnforcedVerticesByEnfVertexEntry() : GetDefaultEnfVertexEntryEnfVertexMap();
}

std::set<int> BLSURFPlugin_Hypothesis::GetEnfVertexNodeIDs(TEnfGroupName theGroupName) throw (std::invalid_argument)
{
  TGroupNameNodeIDMap::const_iterator it = _groupNameNodeIDMap.find(theGroupName);
  if (it != _groupNameNodeIDMap.end()) {
    return it->second;
  }
  std::ostringstream msg;
  msg << "No group " << theGroupName;
  throw std::invalid_argument(msg.str());
}

void BLSURFPlugin_Hypothesis::AddEnfVertexNodeID(TEnfGroupName theGroupName,int theNodeID)
{
  _groupNameNodeIDMap[theGroupName].insert(theNodeID);
}

void BLSURFPlugin_Hypothesis::RemoveEnfVertexNodeID(TEnfGroupName theGroupName,int theNodeID) throw (std::invalid_argument)
{
  TGroupNameNodeIDMap::iterator it = _groupNameNodeIDMap.find(theGroupName);
  if (it != _groupNameNodeIDMap.end()) {
    std::set<int>::iterator IDit = it->second.find(theNodeID);
    if (IDit != it->second.end())
      it->second.erase(IDit);
    std::ostringstream msg;
    msg << "No node IDs " << theNodeID << " for group " << theGroupName;
    throw std::invalid_argument(msg.str());
  }
  std::ostringstream msg;
  msg << "No group " << theGroupName;
  throw std::invalid_argument(msg.str());
}


//=============================================================================
void BLSURFPlugin_Hypothesis::SetInternalEnforcedVertexAllFaces(bool toEnforceInternalVertices) {
  if (toEnforceInternalVertices != _enforcedInternalVerticesAllFaces) {
    _enforcedInternalVerticesAllFaces = toEnforceInternalVertices;
    if (toEnforceInternalVertices)
      SetPhysicalMesh(PhysicalLocalSize);
    NotifySubMeshesHypothesisModification();
  }
}


//=============================================================================
void BLSURFPlugin_Hypothesis::SetInternalEnforcedVertexAllFacesGroup(BLSURFPlugin_Hypothesis::TEnfGroupName theGroupName) {
  if (std::string(theGroupName) != std::string(_enforcedInternalVerticesAllFacesGroup)) {
    _enforcedInternalVerticesAllFacesGroup = theGroupName;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
BLSURFPlugin_Hypothesis::TPreCadPeriodicityVector BLSURFPlugin_Hypothesis::GetPreCadFacesPeriodicityVector(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetPreCadFacesPeriodicityVector() : GetDefaultPreCadFacesPeriodicityVector();
}

//=============================================================================
BLSURFPlugin_Hypothesis::TPreCadPeriodicityVector BLSURFPlugin_Hypothesis::GetPreCadEdgesPeriodicityVector(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetPreCadEdgesPeriodicityVector() : GetDefaultPreCadEdgesPeriodicityVector();
}

//=============================================================================
BLSURFPlugin_Hypothesis::TFacesPeriodicityVector BLSURFPlugin_Hypothesis::GetFacesPeriodicityVector(
    const BLSURFPlugin_Hypothesis* hyp) {
  return hyp ? hyp->_GetFacesPeriodicityVector() : GetDefaultFacesPeriodicityVector();
}

//=============================================================================
BLSURFPlugin_Hypothesis::TEdgesPeriodicityVector BLSURFPlugin_Hypothesis::GetEdgesPeriodicityVector(
    const BLSURFPlugin_Hypothesis* hyp){
  return hyp ? hyp->_GetEdgesPeriodicityVector() : GetDefaultEdgesPeriodicityVector();
}

//=============================================================================
BLSURFPlugin_Hypothesis::TVerticesPeriodicityVector BLSURFPlugin_Hypothesis::GetVerticesPeriodicityVector(
    const BLSURFPlugin_Hypothesis* hyp){
  return hyp ? hyp->_GetVerticesPeriodicityVector() : GetDefaultVerticesPeriodicityVector();
}

//=======================================================================
//function : ClearAllEnforcedVertices
//=======================================================================
void BLSURFPlugin_Hypothesis::ClearPreCadPeriodicityVectors() {
  _preCadFacesPeriodicityVector.clear();
  _preCadEdgesPeriodicityVector.clear();
  NotifySubMeshesHypothesisModification();
}

//=======================================================================
//function : AddPreCadFacesPeriodicity
//=======================================================================
void BLSURFPlugin_Hypothesis::AddPreCadFacesPeriodicity(TEntry theFace1Entry, TEntry theFace2Entry,
                                                        std::vector<std::string> &theSourceVerticesEntries, std::vector<std::string> &theTargetVerticesEntries) {

  TPreCadPeriodicity preCadFacesPeriodicity;
  preCadFacesPeriodicity.shape1Entry = theFace1Entry;
  preCadFacesPeriodicity.shape2Entry = theFace2Entry;
  preCadFacesPeriodicity.theSourceVerticesEntries = theSourceVerticesEntries;
  preCadFacesPeriodicity.theTargetVerticesEntries = theTargetVerticesEntries;

  _preCadFacesPeriodicityVector.push_back(preCadFacesPeriodicity);

  NotifySubMeshesHypothesisModification();
}

//=======================================================================
//function : AddPreCadEdgesPeriodicity
//=======================================================================
void BLSURFPlugin_Hypothesis::AddPreCadEdgesPeriodicity(TEntry theEdge1Entry, TEntry theEdge2Entry,
    std::vector<std::string> &theSourceVerticesEntries, std::vector<std::string> &theTargetVerticesEntries) {

  TPreCadPeriodicity preCadEdgesPeriodicity;
  preCadEdgesPeriodicity.shape1Entry = theEdge1Entry;
  preCadEdgesPeriodicity.shape2Entry = theEdge2Entry;
  preCadEdgesPeriodicity.theSourceVerticesEntries = theSourceVerticesEntries;
  preCadEdgesPeriodicity.theTargetVerticesEntries = theTargetVerticesEntries;

  _preCadEdgesPeriodicityVector.push_back(preCadEdgesPeriodicity);

  NotifySubMeshesHypothesisModification();
}

//=============================================================================
std::ostream & BLSURFPlugin_Hypothesis::SaveTo(std::ostream & save)
{
  // We must keep at least the same number of arguments when increasing the SALOME version
  // When MG-CADSurf becomes CADMESH, some parameters were fused into a single one. Thus the same
  // parameter can be written several times to keep the old global number of parameters.

  // Treat old options which are now in the advanced options
  TOptionValues::iterator op_val;
  int _decimesh = -1;
  int _preCADRemoveNanoEdges = -1;
  double _preCADEpsNano = -1.0;
  op_val = _option2value.find("respect_geometry");
  if (op_val != _option2value.end()) {
    std::string value = op_val->second;
    if (!value.empty())
      _decimesh = value.compare("1") == 0 ? 1 : 0;
  }
  op_val = _preCADoption2value.find("remove_tiny_edges");
  if (op_val != _preCADoption2value.end()) {
    std::string value = op_val->second;
    if (!value.empty())
      _preCADRemoveNanoEdges = value.compare("1") == 0 ? 1 : 0;
  }
  op_val = _preCADoption2value.find("tiny_edge_length");
  if (op_val != _preCADoption2value.end()) {
    std::string value = op_val->second;
    if (!value.empty())
      _preCADEpsNano = strtod(value.c_str(), NULL);
  }

  save << " " << (int) _topology << " " << (int) _physicalMesh << " " << (int) _geometricMesh << " " << _phySize << " "
       << _angleMesh << " " << _gradation << " " << (int) _quadAllowed << " " << _decimesh;
  save << " " << _minSize << " " << _maxSize << " " << _angleMesh << " " << _minSize << " " << _maxSize << " " << _verb;
  save << " " << (int) _preCADMergeEdges << " " << _preCADRemoveNanoEdges << " " << (int) _preCADDiscardInput << " " << _preCADEpsNano ;
  save << " " << (int) _enforcedInternalVerticesAllFaces;
  save << " " << (int) _phySizeRel << " " << (int) _minSizeRel << " " << (int) _maxSizeRel << " " << _chordalError ;
  save << " " << (int) _anisotropic << " " << _anisotropicRatio << " " << (int) _removeTinyEdges << " " << _tinyEdgeLength ;
  save << " " << (int) _badElementRemoval << " " << _badElementAspectRatio << " " << (int) _optimizeMesh << " " << (int) _quadraticMesh ;
  save << " " << (int) _preCADProcess3DTopology << " " << (int) _preCADRemoveDuplicateCADFaces;
  save << " " << (int)_optimiseTinyEdges << " " << _tinyEdgeOptimisationLength;
  save << " " << (int)_correctSurfaceIntersec << " " << _corrSurfaceIntersCost;
  save << " " << (int)_useGradation << " " << (int)_useVolumeGradation << " " << _volumeGradation;

  op_val = _option2value.begin();
  if (op_val != _option2value.end()) {
    save << " " << "__OPTIONS_BEGIN__";
    for (; op_val != _option2value.end(); ++op_val) {
      if (!op_val->second.empty())
        save << " " << op_val->first << " " << op_val->second << "%#"; // "%#" is a mark of value end
    }
    save << " " << "__OPTIONS_END__";
  }
  
  op_val = _customOption2value.begin();
  if (op_val != _customOption2value.end()) {
    save << " " << "__CUSTOM_OPTIONS_BEGIN__";
    for (; op_val != _customOption2value.end(); ++op_val) {
      if (!op_val->second.empty())
        save << " " << op_val->first << " " << op_val->second << "%#"; // "%#" is a mark of value end
    }
    save << " " << "__CUSTOM_OPTIONS_END__";
  }

  op_val = _preCADoption2value.begin();
  if (op_val != _preCADoption2value.end()) {
    save << " " << "__PRECAD_OPTIONS_BEGIN__";
    for (; op_val != _preCADoption2value.end(); ++op_val) {
      if (!op_val->second.empty())
        save << " " << op_val->first << " " << op_val->second << "%#"; // "%#" is a mark of value end
    }
    save << " " << "__PRECAD_OPTIONS_END__";
  }

  TSizeMap::iterator it_sm = _sizeMap.begin();
  if (it_sm != _sizeMap.end()) {
    save << " " << "__SIZEMAP_BEGIN__";
    for (; it_sm != _sizeMap.end(); ++it_sm) {
      save << " " << it_sm->first << " " << it_sm->second << "%#"; // "%#" is a mark of value end
    }
    save << " " << "__SIZEMAP_END__";
  }

  TSizeMap::iterator it_at = _attractors.begin();
  if (it_at != _attractors.end()) {
    save << " " << "__ATTRACTORS_BEGIN__";
    for (; it_at != _attractors.end(); ++it_at) {
      save << " " << it_at->first << " " << it_at->second << "%#"; // "%#" is a mark of value end
    }
    save << " " << "__ATTRACTORS_END__";
  }
  
  TAttractorMap::iterator it_At = _classAttractors.begin();
  if (it_At != _classAttractors.end()) {
    std::ostringstream test;
    save << " " << "__NEW_ATTRACTORS_BEGIN__";
    test << " " << "__NEW_ATTRACTORS_BEGIN__";
    for (; it_At != _classAttractors.end(); ++it_At) {
      std::vector<double> attParams;
      attParams   = it_At->second->GetParameters();
//       double step = it_At->second->GetStep();
      save << " " << it_At->first;
      save << " " << it_At->second->GetAttractorEntry();
      save << " " << attParams[0]  << " " <<  attParams[1] << " " <<  attParams[2] << " " <<  attParams[3];
//       save << " " << step;
      test << " " << it_At->first;
      test << " " << it_At->second->GetAttractorEntry();
      test << " " << attParams[0]  << " " <<  attParams[1] << " " <<  attParams[2] << " " <<  attParams[3];
//       test << " " << step;
    }
    save << " " << "__NEW_ATTRACTORS_END__";
    test << " " << "__NEW_ATTRACTORS_END__";
  }

  TEnfVertexList::const_iterator it_enf = _enfVertexList.begin();
  if (it_enf != _enfVertexList.end()) {
    save << " " << "__ENFORCED_VERTICES_BEGIN__";
    for (; it_enf != _enfVertexList.end(); ++it_enf) {
      TEnfVertex *enfVertex = (*it_enf);
      save << " " << "__BEGIN_VERTEX__";
      if (!enfVertex->name.empty()) {
        save << " " << "__BEGIN_NAME__";
        save << " " << enfVertex->name;
        save << " " << "__END_NAME__";
      }
      if (!enfVertex->geomEntry.empty()) {
        save << " " << "__BEGIN_ENTRY__";
        save << " " << enfVertex->geomEntry;
        save << " " << "__END_ENTRY__";
      }
      if (!enfVertex->grpName.empty()) {
        save << " " << "__BEGIN_GROUP__";
        save << " " << enfVertex->grpName;
        save << " " << "__END_GROUP__";
      }
      if (enfVertex->coords.size()) {
        save << " " << "__BEGIN_COORDS__";
        for ( size_t i = 0; i < enfVertex->coords.size(); i++ )
          save << " " << enfVertex->coords[i];
        save << " " << "__END_COORDS__";
      }
      TEntryList::const_iterator faceEntriesIt = enfVertex->faceEntries.begin();
      bool hasFaces = false;
      if (faceEntriesIt != enfVertex->faceEntries.end()) {
        hasFaces = true;
        save << " " << "__BEGIN_FACELIST__";
      }
      for (; faceEntriesIt != enfVertex->faceEntries.end(); ++faceEntriesIt)
        if ( faceEntriesIt->empty() )
          save << " _no_face_";
        else
          save << " " << (*faceEntriesIt);
      if (hasFaces)
        save << " " << "__END_FACELIST__";
      save << " " << "__END_VERTEX__";
    }
    save << " " << "__ENFORCED_VERTICES_END__";
  }

  //PERIODICITY

  SavePreCADPeriodicity(save, "FACES");
  SavePreCADPeriodicity(save, "EDGES");

  SaveFacesPeriodicity(save);
  SaveEdgesPeriodicity(save);
  SaveVerticesPeriodicity(save);

  // HYPER-PATCHES
  save << " " << _hyperPatchList.size() << " ";
  for ( size_t i = 0; i < _hyperPatchList.size(); ++i )
  {
    THyperPatchTags& patch = _hyperPatchList[i];
    save << patch.size() << " ";
    THyperPatchTags::iterator tag = patch.begin();
    for ( ; tag != patch.end(); ++tag )
      save << *tag << " ";
  }

  return save;
}

void BLSURFPlugin_Hypothesis::SaveFacesPeriodicity(std::ostream & save){

  TFacesPeriodicityVector::const_iterator it_faces_periodicity = _facesPeriodicityVector.begin();
  if (it_faces_periodicity != _facesPeriodicityVector.end()) {
    save << " " << "__FACES_PERIODICITY_BEGIN__";
    for (; it_faces_periodicity != _facesPeriodicityVector.end(); ++it_faces_periodicity) {
      TFacesPeriodicity periodicity_i = (*it_faces_periodicity);
      save << " " << "__BEGIN_PERIODICITY_DESCRIPTION__";
      save << " " << "__BEGIN_ENTRY1__";
      save << " " << periodicity_i.first;
      save << " " << "__END_ENTRY1__";
      save << " " << "__BEGIN_ENTRY2__";
      save << " " << periodicity_i.second;
      save << " " << "__END_ENTRY2__";
      save << " " << "__END_PERIODICITY_DESCRIPTION__";
    }
    save << " " << "__FACES_PERIODICITY_END__";
  }
}

void BLSURFPlugin_Hypothesis::SaveEdgesPeriodicity(std::ostream & save){

  TEdgesPeriodicityVector::const_iterator it_edges_periodicity = _edgesPeriodicityVector.begin();
  if (it_edges_periodicity != _edgesPeriodicityVector.end()) {
    save << " " << "__EDGES_PERIODICITY_BEGIN__";
    for (; it_edges_periodicity != _edgesPeriodicityVector.end(); ++it_edges_periodicity) {
      TEdgePeriodicity periodicity_i = (*it_edges_periodicity);
      save << " " << "__BEGIN_PERIODICITY_DESCRIPTION__";
      if (! periodicity_i.theFace1Entry.empty()){
        save << " " << "__BEGIN_FACE1__";
        save << " " << periodicity_i.theFace1Entry;
        save << " " << "__END_FACE1__";
      }
      save << " " << "__BEGIN_EDGE1__";
      save << " " << periodicity_i.theEdge1Entry;
      save << " " << "__END_EDGE1__";
      if (! periodicity_i.theFace2Entry.empty()){
        save << " " << "__BEGIN_FACE2__";
        save << " " << periodicity_i.theFace2Entry;
        save << " " << "__END_FACE2__";
      }
      save << " " << "__BEGIN_EDGE2__";
      save << " " << periodicity_i.theEdge2Entry;
      save << " " << "__END_EDGE2__";
      save << " " << "__BEGIN_EDGE_ORIENTATION__";
      save << " " << periodicity_i.edge_orientation;
      save << " " << "__END_EDGE_ORIENTATION__";
      save << " " << "__END_PERIODICITY_DESCRIPTION__";
    }
    save << " " << "__EDGES_PERIODICITY_END__";
  }
}

void BLSURFPlugin_Hypothesis::SaveVerticesPeriodicity(std::ostream & save){

  TVerticesPeriodicityVector::const_iterator it_vertices_periodicity = _verticesPeriodicityVector.begin();
  if (it_vertices_periodicity != _verticesPeriodicityVector.end()) {
    save << " " << "__VERTICES_PERIODICITY_BEGIN__";
    for (; it_vertices_periodicity != _verticesPeriodicityVector.end(); ++it_vertices_periodicity) {
      TVertexPeriodicity periodicity_i = (*it_vertices_periodicity);
      save << " " << "__BEGIN_PERIODICITY_DESCRIPTION__";
      save << " " << "__BEGIN_EDGE1__";
      save << " " << periodicity_i.theEdge1Entry;
      save << " " << "__END_EDGE1__";
      save << " " << "__BEGIN_VERTEX1__";
      save << " " << periodicity_i.theVertex1Entry;
      save << " " << "__END_VERTEX1__";
      save << " " << "__BEGIN_EDGE2__";
      save << " " << periodicity_i.theEdge2Entry;
      save << " " << "__END_EDGE2__";
      save << " " << "__BEGIN_VERTEX2__";
      save << " " << periodicity_i.theVertex2Entry;
      save << " " << "__END_VERTEX2__";
      save << " " << "__END_PERIODICITY_DESCRIPTION__";
    }
    save << " " << "__VERTICES_PERIODICITY_END__";
  }
}

void BLSURFPlugin_Hypothesis::SavePreCADPeriodicity(std::ostream & save, const char* shapeType) {
  TPreCadPeriodicityVector precad_periodicity;
  if ( shapeType  &&  strcmp( shapeType, "FACES" ) == 0 )
    precad_periodicity = _preCadFacesPeriodicityVector;
  else
    precad_periodicity = _preCadEdgesPeriodicityVector;
  TPreCadPeriodicityVector::const_iterator it_precad_periodicity = precad_periodicity.begin();
  if (it_precad_periodicity != precad_periodicity.end()) {
    save << " " << "__PRECAD_" << shapeType << "_PERIODICITY_BEGIN__";
    for (; it_precad_periodicity != precad_periodicity.end(); ++it_precad_periodicity) {
      TPreCadPeriodicity periodicity_i = (*it_precad_periodicity);
      save << " " << "__BEGIN_PERIODICITY_DESCRIPTION__";
      if (!periodicity_i.shape1Entry.empty()) {
        save << " " << "__BEGIN_ENTRY1__";
        save << " " << periodicity_i.shape1Entry;
        save << " " << "__END_ENTRY1__";
      }
      if (!periodicity_i.shape2Entry.empty()) {
        save << " " << "__BEGIN_ENTRY2__";
        save << " " << periodicity_i.shape2Entry;
        save << " " << "__END_ENTRY2__";
      }

      std::vector<std::string>::const_iterator sourceVerticesEntriesIt = periodicity_i.theSourceVerticesEntries.begin();
      bool hasSourceVertices = false;
      if (sourceVerticesEntriesIt != periodicity_i.theSourceVerticesEntries.end()) {
        hasSourceVertices = true;
        save << " " << "__BEGIN_SOURCE_VERTICES_LIST__";
      }
      for (; sourceVerticesEntriesIt != periodicity_i.theSourceVerticesEntries.end(); ++sourceVerticesEntriesIt)
        save << " " << (*sourceVerticesEntriesIt);
      if (hasSourceVertices)
        save << " " << "__END_SOURCE_VERTICES_LIST__";

      std::vector<std::string>::const_iterator targetVerticesEntriesIt = periodicity_i.theTargetVerticesEntries.begin();
      bool hasTargetVertices = false;
      if (targetVerticesEntriesIt != periodicity_i.theTargetVerticesEntries.end()) {
        hasTargetVertices = true;
        save << " " << "__BEGIN_TARGET_VERTICES_LIST__";
      }
      for (; targetVerticesEntriesIt != periodicity_i.theTargetVerticesEntries.end(); ++targetVerticesEntriesIt)
        save << " " << (*targetVerticesEntriesIt);
      if (hasTargetVertices)
        save << " " << "__END_TARGET_VERTICES_LIST__";

      save << " " << "__END_PERIODICITY_DESCRIPTION__";
    }
    save << " " << "__PRECAD_" << shapeType << "_PERIODICITY_END__";
  }

}

//=============================================================================
std::istream & BLSURFPlugin_Hypothesis::LoadFrom(std::istream & load)
{
  bool isOK = true;
  int i;
  double val;
  std::string option_or_sm;

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    _topology = (Topology) i;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    _physicalMesh = (PhysicalMesh) i;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    _geometricMesh = (GeometricMesh) i;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK)
    _phySize = val;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK)
    _angleMesh = val;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK)
    _gradation = val;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    _quadAllowed = (bool) i;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK) {
    if ( i != -1) { // if value is -1, then this is no longer a standard option
      std::string & value = _option2value["respect_geometry"];
      bool _decimesh = (bool) i;
      value = _decimesh ? "1" : "0";
    }
  }
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK)
    _minSize = val;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK)
    _maxSize = val;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK)
    // former parameter: get min value
    _angleMesh = std::min(val,_angleMesh);
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK)
    // former parameter: get min value
    _minSize = std::min(val,_minSize);
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK)
    // former parameter: get max value
    _maxSize = std::max(val,_maxSize);
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    _verb = i;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    _preCADMergeEdges = (bool) i;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK) {
    if ( i != -1) { // if value is -1, then this is no longer a standard option
      std::string & value = _preCADoption2value["remove_tiny_edges"];
      bool _preCADRemoveNanoEdges = (bool) i;
      value = _preCADRemoveNanoEdges ? "1" : "0";
    }
  }
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    _preCADDiscardInput = (bool) i;
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> val);
  if (isOK) { // _preCADEpsNano
    if ( (i + 1.0) < 1e-6 ) { // if value is -1, then this is no longer a standard option: get optional value "tiny_edge_length" instead
      std::string & value = _preCADoption2value["tiny_edge_length"];
      std::ostringstream oss;
      oss << i;
      value = oss.str();
    }
  }
  else
    load.clear(std::ios::badbit | load.rdstate());

  isOK = static_cast<bool>(load >> i);
  if (isOK)
    _enforcedInternalVerticesAllFaces = (bool) i;
  else
    load.clear(std::ios::badbit | load.rdstate());

  // New options with MeshGems-CADSurf

  bool hasCADSurfOptions = false;
  bool hasOptions = false;
  bool hasCustomOptions = false;
  bool hasPreCADOptions = false;
  bool hasSizeMap = false;
  bool hasAttractor = false;
  bool hasNewAttractor = false;
  bool hasEnforcedVertex = false;
  bool hasPreCADFacesPeriodicity = false;
  bool hasPreCADEdgesPeriodicity = false;
  bool hasFacesPeriodicity = false;
  bool hasEdgesPeriodicity = false;
  bool hasVerticesPeriodicity = false;

  isOK = static_cast<bool>(load >> option_or_sm);
  if (isOK)
    if ( (option_or_sm == "1")||(option_or_sm == "0") ) {
      i = atoi(option_or_sm.c_str());
      hasCADSurfOptions = true;
      _phySizeRel = (bool) i;
    }
    if (option_or_sm == "__OPTIONS_BEGIN__")
      hasOptions = true;
    else if (option_or_sm == "__CUSTOM_OPTIONS_BEGIN__")
      hasCustomOptions = true;
    else if (option_or_sm == "__PRECAD_OPTIONS_BEGIN__")
      hasPreCADOptions = true;
    else if (option_or_sm == "__SIZEMAP_BEGIN__")
      hasSizeMap = true;
    else if (option_or_sm == "__ATTRACTORS_BEGIN__")
      hasAttractor = true;
    else if (option_or_sm == "__NEW_ATTRACTORS_BEGIN__")
      hasNewAttractor = true;
    else if (option_or_sm == "__ENFORCED_VERTICES_BEGIN__")
      hasEnforcedVertex = true;
    else if (option_or_sm == "__PRECAD_FACES_PERIODICITY_BEGIN__")
      hasPreCADFacesPeriodicity = true;
    else if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
      hasPreCADEdgesPeriodicity = true;
    else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
      hasFacesPeriodicity = true;
    else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
      hasEdgesPeriodicity = true;
    else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
      hasVerticesPeriodicity = true;

  if (isOK && hasCADSurfOptions) {
    isOK = static_cast<bool>(load >> i);
    if (isOK)
      _minSizeRel = (bool) i;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> i);
    if (isOK)
      _maxSizeRel = (bool) i;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> val);
    if (isOK)
      _chordalError = val;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> i);
    if (isOK)
      _anisotropic = (bool) i;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> val);
    if (isOK)
      _anisotropicRatio = val;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> i);
    if (isOK)
      _removeTinyEdges = (bool) i;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> val);
    if (isOK)
      _tinyEdgeLength = val;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> i);
    if (isOK)
      _badElementRemoval = (bool) i;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> val);
    if (isOK)
      _badElementAspectRatio = val;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> i);
    if (isOK)
      _optimizeMesh = (bool) i;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> i);
    if (isOK)
      _quadraticMesh = (bool) i;
    else
      load.clear(std::ios::badbit | load.rdstate());

    isOK = static_cast<bool>(load >> i);
    if (isOK)
      _preCADProcess3DTopology = (bool) i;
    else
      load.clear(std::ios::badbit | load.rdstate());

    if (( load >> std::ws).peek() != '_' )
    {
      isOK = static_cast<bool>(load >> i);
      if (isOK)
        _preCADRemoveDuplicateCADFaces = (bool) i;
      else
        load.clear(std::ios::badbit | load.rdstate());

      isOK = static_cast<bool>(load >> i);
      if (isOK)
        _optimiseTinyEdges = (bool) i;
      else
        load.clear(std::ios::badbit | load.rdstate());

      isOK = static_cast<bool>(load >> val);
      if (isOK)
        _tinyEdgeOptimisationLength = val;
      else
        load.clear(std::ios::badbit | load.rdstate());

      isOK = static_cast<bool>(load >> i);
      if (isOK)
        _correctSurfaceIntersec = (bool) i;
      else
        load.clear(std::ios::badbit | load.rdstate());

      isOK = static_cast<bool>(load >> val);
      if (isOK)
        _corrSurfaceIntersCost = val;
      else
        load.clear(std::ios::badbit | load.rdstate());

      isOK = static_cast<bool>(load >> i);
      if (isOK)
        _useGradation = (bool) i;
      else
        load.clear(std::ios::badbit | load.rdstate());

      isOK = static_cast<bool>(load >> i);
      if (isOK)
        _useVolumeGradation = (bool) i;
      else
        load.clear(std::ios::badbit | load.rdstate());

      isOK = static_cast<bool>(load >> val);
      if (isOK)
        _volumeGradation = val;
      else
        load.clear(std::ios::badbit | load.rdstate());
    }
  }


  if (hasCADSurfOptions) {
    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__OPTIONS_BEGIN__")
        hasOptions = true;
      else if (option_or_sm == "__CUSTOM_OPTIONS_BEGIN__")
        hasCustomOptions = true;
      else if (option_or_sm == "__PRECAD_OPTIONS_BEGIN__")
        hasPreCADOptions = true;
      else if (option_or_sm == "__SIZEMAP_BEGIN__")
        hasSizeMap = true;
      else if (option_or_sm == "__ATTRACTORS_BEGIN__")
        hasAttractor = true;
      else if (option_or_sm == "__NEW_ATTRACTORS_BEGIN__")
        hasNewAttractor = true;
      else if (option_or_sm == "__ENFORCED_VERTICES_BEGIN__")
        hasEnforcedVertex = true;
      else if (option_or_sm == "__PRECAD_FACES_PERIODICITY_BEGIN__")
        hasPreCADFacesPeriodicity = true;
      else if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
        hasPreCADEdgesPeriodicity = true;
      else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }
  
  std::string optName, optValue;
  while (isOK && hasOptions) {
    isOK = static_cast<bool>(load >> optName);
    if (isOK) {
      if (optName == "__OPTIONS_END__")
        break;
      isOK = static_cast<bool>(load >> optValue);
    }
    if (isOK) {
      std::string & value = _option2value[optName];
      value = optValue;
      int len = value.size();
      // continue reading until "%#" encountered
      while (value[len - 1] != '#' || value[len - 2] != '%') {
        isOK = static_cast<bool>(load >> optValue);
        if (isOK) {
          value += " ";
          value += optValue;
          len = value.size();
        } else {
          break;
        }
      }
      if ( value[ len - 1] == '#' )
        value.resize(len - 2); //cut off "%#"
    }
  }

  if (hasOptions) {
    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__CUSTOM_OPTIONS_BEGIN__")
        hasCustomOptions = true;
      else if (option_or_sm == "__PRECAD_OPTIONS_BEGIN__")
        hasPreCADOptions = true;
      else if (option_or_sm == "__SIZEMAP_BEGIN__")
        hasSizeMap = true;
      else if (option_or_sm == "__ATTRACTORS_BEGIN__")
        hasAttractor = true;
      else if (option_or_sm == "__NEW_ATTRACTORS_BEGIN__")
        hasNewAttractor = true;
      else if (option_or_sm == "__ENFORCED_VERTICES_BEGIN__")
        hasEnforcedVertex = true;
      else if (option_or_sm == "__PRECAD_FACES_PERIODICITY_BEGIN__")
        hasPreCADFacesPeriodicity = true;
      else if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
        hasPreCADEdgesPeriodicity = true;
      else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }

  while (isOK && hasCustomOptions) {
    isOK = static_cast<bool>(load >> optName);
    if (isOK) {
      if (optName == "__CUSTOM_OPTIONS_END__")
        break;
      isOK = static_cast<bool>(load >> optValue);
    }
    if (isOK) {
      std::string& value = optValue;
      int len = value.size();
      // continue reading until "%#" encountered
      while (value[len - 1] != '#' || value[len - 2] != '%') {
        isOK = static_cast<bool>(load >> optValue);
        if (isOK) {
          value += " ";
          value += optValue;
          len = value.size();
        } else {
          break;
        }
      }
      if ( value[ len - 1] == '#' )
        value.resize(len - 2); //cut off "%#"
      _customOption2value[optName] = value;
    }
  }

  if (hasCustomOptions) {
    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__PRECAD_OPTIONS_BEGIN__")
        hasPreCADOptions = true;
      else if (option_or_sm == "__SIZEMAP_BEGIN__")
        hasSizeMap = true;
      else if (option_or_sm == "__ATTRACTORS_BEGIN__")
        hasAttractor = true;
      else if (option_or_sm == "__NEW_ATTRACTORS_BEGIN__")
        hasNewAttractor = true;
      else if (option_or_sm == "__ENFORCED_VERTICES_BEGIN__")
        hasEnforcedVertex = true;
      else if (option_or_sm == "__PRECAD_FACES_PERIODICITY_BEGIN__")
        hasPreCADFacesPeriodicity = true;
      else if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
        hasPreCADEdgesPeriodicity = true;
      else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }

  while (isOK && hasPreCADOptions) {
    isOK = static_cast<bool>(load >> optName);
    if (isOK) {
      if (optName == "__PRECAD_OPTIONS_END__")
        break;
      isOK = static_cast<bool>(load >> optValue);
    }
    if (isOK) {
      std::string & value = _preCADoption2value[optName];
      value = optValue;
      int len = value.size();
      // continue reading until "%#" encountered
      while (value[len - 1] != '#' || value[len - 2] != '%') {
        isOK = static_cast<bool>(load >> optValue);
        if (isOK) {
          value += " ";
          value += optValue;
          len = value.size();
        } else {
          break;
        }
      }
      if ( value[ len - 1] == '#' )
        value.resize(len - 2); //cut off "%#"
    }
  }

  if (hasPreCADOptions) {
    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__SIZEMAP_BEGIN__")
        hasSizeMap = true;
      else if (option_or_sm == "__ATTRACTORS_BEGIN__")
        hasAttractor = true;
      else if (option_or_sm == "__NEW_ATTRACTORS_BEGIN__")
        hasNewAttractor = true;
      else if (option_or_sm == "__ENFORCED_VERTICES_BEGIN__")
        hasEnforcedVertex = true;
      else if (option_or_sm == "__PRECAD_FACES_PERIODICITY_BEGIN__")
        hasPreCADFacesPeriodicity = true;
      else if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
        hasPreCADEdgesPeriodicity = true;
      else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }
 
  std::string smEntry, smValue;
  while (isOK && hasSizeMap) {
    isOK = static_cast<bool>(load >> smEntry);
    if (isOK) {
      if (smEntry == "__SIZEMAP_END__")
        break;
      isOK = static_cast<bool>(load >> smValue);
    }
    if (isOK) {
      std::string & value2 = _sizeMap[smEntry];
      value2 = smValue;
      int len2 = value2.size();
      // continue reading until "%#" encountered
      while (value2[len2 - 1] != '#' || value2[len2 - 2] != '%') {
        isOK = static_cast<bool>(load >> smValue);
        if (isOK) {
          value2 += " ";
          value2 += smValue;
          len2 = value2.size();
        } else {
          break;
        }
      }
      value2.resize(len2 - 2); //cut off "%#"
    }
  }

  if (hasSizeMap) {
    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK)
      if (option_or_sm == "__ATTRACTORS_BEGIN__")
        hasAttractor = true;
      if (option_or_sm == "__NEW_ATTRACTORS_BEGIN__")
        hasNewAttractor = true;
      else if (option_or_sm == "__ENFORCED_VERTICES_BEGIN__")
        hasEnforcedVertex = true;
      else if (option_or_sm == "__PRECAD_FACES_PERIODICITY_BEGIN__")
        hasPreCADFacesPeriodicity = true;
      else if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
        hasPreCADEdgesPeriodicity = true;
      else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
  }

  std::string atEntry, atValue;
  while (isOK && hasAttractor) {
    isOK = static_cast<bool>(load >> atEntry);
    if (isOK) {
      if (atEntry == "__ATTRACTORS_END__")
        break;
      isOK = static_cast<bool>(load >> atValue);
    }
    if (isOK) {
      std::string & value3 = _attractors[atEntry];
      value3 = atValue;
      int len3 = value3.size();
      // continue reading until "%#" encountered
      while (value3[len3 - 1] != '#' || value3[len3 - 2] != '%') {
        isOK = static_cast<bool>(load >> atValue);
        if (isOK) {
          value3 += " ";
          value3 += atValue;
          len3 = value3.size();
        } else {
          break;
        }
      }
      value3.resize(len3 - 2); //cut off "%#"
    }
  }

  if (hasAttractor) {
    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__NEW_ATTRACTORS_BEGIN__")
        hasNewAttractor = true;
      else if (option_or_sm == "__ENFORCED_VERTICES_BEGIN__")
        hasEnforcedVertex = true;
      else if (option_or_sm == "__PRECAD_FACES_PERIODICITY_BEGIN__")
        hasPreCADFacesPeriodicity = true;
      else if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
        hasPreCADEdgesPeriodicity = true;
      else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }

  std::string newAtFaceEntry, atTestString;
  std::string newAtShapeEntry;
  double attParams[4];
  //double step;
  while (isOK && hasNewAttractor) {
    //std::cout<<"Load new attractor"<<std::endl;
    isOK = static_cast<bool>(load >> newAtFaceEntry);
    if (isOK) {
      if (newAtFaceEntry == "__NEW_ATTRACTORS_END__")
        break;
      isOK = static_cast<bool>(load >> newAtShapeEntry);
      if (!isOK)
    break;
      isOK = static_cast<bool>(load >> attParams[0]>>attParams[1]>>attParams[2]>>attParams[3]); //>>step);
    }
    if (isOK) {
      const TopoDS_Shape attractorShape = BLSURFPlugin_Hypothesis::entryToShape(newAtShapeEntry);
      const TopoDS_Face faceShape = TopoDS::Face(BLSURFPlugin_Hypothesis::entryToShape(newAtFaceEntry));
      BLSURFPlugin_Attractor* attractor = new BLSURFPlugin_Attractor(faceShape, attractorShape, newAtShapeEntry);//, step);
      attractor->SetParameters(attParams[0], attParams[1], attParams[2], attParams[3]);
      //attractor->BuildMap();                     
      _classAttractors.insert( make_pair( newAtFaceEntry, attractor ));
    }
  }
  
  
  if (hasNewAttractor) {
    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__ENFORCED_VERTICES_BEGIN__")
        hasEnforcedVertex = true;
      else if (option_or_sm == "__PRECAD_FACES_PERIODICITY_BEGIN__")
        hasPreCADFacesPeriodicity = true;
      else if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
        hasPreCADEdgesPeriodicity = true;
      else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }


// 
// Here is a example of the saved stream:
// __ENFORCED_VERTICES_BEGIN__ 
// __BEGIN_VERTEX__  => no name, no entry
// __BEGIN_GROUP__ mon groupe __END_GROUP__
// __BEGIN_COORDS__ 10 10 10 __END_COORDS__
// __BEGIN_FACELIST__ 0:1:1:1:1 __END_FACELIST__
// __END_VERTEX__
// __BEGIN_VERTEX__ => no coords
// __BEGIN_NAME__ mes points __END_NAME__
// __BEGIN_ENTRY__ 0:1:1:4 __END_ENTRY__
// __BEGIN_GROUP__ mon groupe __END_GROUP__
// __BEGIN_FACELIST__ 0:1:1:1:3 __END_FACELIST__
// __END_VERTEX__
// __ENFORCED_VERTICES_END__
//

  std::string enfSeparator;
  std::string enfName;
  std::string enfGeomEntry;
  std::string enfGroup;
  TEntryList enfFaceEntryList;
  double enfCoords[3];
  bool hasCoords = false;

  _faceEntryEnfVertexListMap.clear();
  _enfVertexList.clear();
  _faceEntryCoordsListMap.clear();
  _coordsEnfVertexMap.clear();
  _faceEntryEnfVertexEntryListMap.clear();
  _enfVertexEntryEnfVertexMap.clear();


  while (isOK && hasEnforcedVertex)
  {
    isOK = static_cast<bool>(load >> enfSeparator); // __BEGIN_VERTEX__
    TEnfVertex *enfVertex = new TEnfVertex();
    if (enfSeparator == "__ENFORCED_VERTICES_END__")
      break; // __ENFORCED_VERTICES_END__
    if (enfSeparator != "__BEGIN_VERTEX__")
      throw std::exception();

    while (isOK) {
      isOK = static_cast<bool>(load >> enfSeparator);
      if (enfSeparator == "__END_VERTEX__") {

        enfVertex->name = enfName;
        enfVertex->geomEntry = enfGeomEntry;
        enfVertex->grpName = enfGroup;
        enfVertex->coords.clear();
        if (hasCoords)
          enfVertex->coords.assign(enfCoords,enfCoords+3);
        enfVertex->faceEntries = enfFaceEntryList;

        _enfVertexList.insert(enfVertex);

        if (enfVertex->coords.size()) {
          _coordsEnfVertexMap[enfVertex->coords] = enfVertex;
          for (TEntryList::const_iterator it = enfVertex->faceEntries.begin() ; it != enfVertex->faceEntries.end(); ++it) {
            _faceEntryCoordsListMap[(*it)].insert(enfVertex->coords);
            _faceEntryEnfVertexListMap[(*it)].insert(enfVertex);
          }
        }
        if (!enfVertex->geomEntry.empty()) {
          _enfVertexEntryEnfVertexMap[enfVertex->geomEntry] = enfVertex;
          for (TEntryList::const_iterator it = enfVertex->faceEntries.begin() ; it != enfVertex->faceEntries.end(); ++it) {
            _faceEntryEnfVertexEntryListMap[(*it)].insert(enfVertex->geomEntry);
            _faceEntryEnfVertexListMap[(*it)].insert(enfVertex);
          }
        }

        enfName.clear();
        enfGeomEntry.clear();
        enfGroup.clear();
        enfFaceEntryList.clear();
        hasCoords = false;
        break; // __END_VERTEX__
      }

      if (enfSeparator == "__BEGIN_NAME__") {  // __BEGIN_NAME__
        while (isOK && (enfSeparator != "__END_NAME__")) {
          isOK = static_cast<bool>(load >> enfSeparator);
          if (enfSeparator != "__END_NAME__") {
            if (!enfName.empty())
              enfName += " ";
            enfName += enfSeparator;
          }
        }
      }

      if (enfSeparator == "__BEGIN_ENTRY__") {  // __BEGIN_ENTRY__
        isOK = static_cast<bool>(load >> enfGeomEntry);
        isOK = static_cast<bool>(load >> enfSeparator); // __END_ENTRY__
        if (enfSeparator != "__END_ENTRY__")
          throw std::exception();
      }

      if (enfSeparator == "__BEGIN_GROUP__") {  // __BEGIN_GROUP__
        while (isOK && (enfSeparator != "__END_GROUP__")) {
          isOK = static_cast<bool>(load >> enfSeparator);
          if (enfSeparator != "__END_GROUP__") {
            if (!enfGroup.empty())
              enfGroup += " ";
            enfGroup += enfSeparator;
          }
        }
      }

      if (enfSeparator == "__BEGIN_COORDS__") {  // __BEGIN_COORDS__
        hasCoords = true;
        isOK = static_cast<bool>(load >> enfCoords[0] >> enfCoords[1] >> enfCoords[2]);
        isOK = static_cast<bool>(load >> enfSeparator); // __END_COORDS__
        if (enfSeparator != "__END_COORDS__")
          throw std::exception();
      }

      if (enfSeparator == "__BEGIN_FACELIST__") {  // __BEGIN_FACELIST__
        while (isOK && (enfSeparator != "__END_FACELIST__")) {
          isOK = static_cast<bool>(load >> enfSeparator);
          if (enfSeparator != "__END_FACELIST__") {
            enfFaceEntryList.insert(enfSeparator);
          }
        }
      }
    }
  }

  // PERIODICITY

  if (hasPreCADFacesPeriodicity)
  {
    LoadPreCADPeriodicity(load, "FACES");

    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__PRECAD_EDGES_PERIODICITY_BEGIN__")
        hasPreCADEdgesPeriodicity = true;
      else if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }

  if (hasPreCADEdgesPeriodicity)
  {
    LoadPreCADPeriodicity(load, "EDGES");

    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__FACES_PERIODICITY_BEGIN__")
        hasFacesPeriodicity = true;
      else if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }

  if (hasFacesPeriodicity)
  {
    LoadFacesPeriodicity(load);

    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK) {
      if (option_or_sm == "__EDGES_PERIODICITY_BEGIN__")
        hasEdgesPeriodicity = true;
      else if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
    }
  }

  if (hasEdgesPeriodicity)
  {
    LoadEdgesPeriodicity(load);

    isOK = static_cast<bool>(load >> option_or_sm);
    if (isOK)
      if (option_or_sm == "__VERTICES_PERIODICITY_BEGIN__")
        hasVerticesPeriodicity = true;
  }

  if (hasVerticesPeriodicity)
    LoadVerticesPeriodicity(load);

  // HYPER-PATCHES
  if ( !option_or_sm.empty() && option_or_sm[0] == '_' )
    isOK = static_cast<bool>(load >> option_or_sm);
  if ( isOK && !option_or_sm.empty() )
  {
    int nbPatches = atoi( option_or_sm.c_str() );
    if ( nbPatches >= 0 )
    {
      _hyperPatchList.resize( nbPatches );
      for ( int iP = 0; iP < nbPatches && isOK; ++iP )
      {
        isOK = static_cast<bool>(load >> i) && i >= 2;
        if ( !isOK ) break;
        int nbTags = i;
        for ( int iT = 0; iT < nbTags; ++iT )
        {
          if (( isOK = static_cast<bool>(load >> i)))
            _hyperPatchList[ iP ].insert( i );
          else
            break;
        }
      }
      if ( !isOK ) // remove invalid patches
      {
        for ( i = nbPatches - 1; i >= 0; i-- )
          if ( _hyperPatchList[i].size() < 2 )
            _hyperPatchList.resize( i );
      }
    }
  }

  return load;
}

void BLSURFPlugin_Hypothesis::LoadFacesPeriodicity(std::istream & load){

  bool isOK = true;

  std::string periodicitySeparator;
  TEntry shape1Entry;
  TEntry shape2Entry;

  _facesPeriodicityVector.clear();

  while (isOK) {
    isOK = static_cast<bool>(load >> periodicitySeparator); // __BEGIN_PERIODICITY_DESCRIPTION__
    TFacesPeriodicity *periodicity_i = new TFacesPeriodicity();
    if (periodicitySeparator == "__FACES_PERIODICITY_END__")
      break; // __FACES_PERIODICITY_END__
    if (periodicitySeparator != "__BEGIN_PERIODICITY_DESCRIPTION__"){
      throw std::exception();
    }

    while (isOK) {
      isOK = static_cast<bool>(load >> periodicitySeparator);
      if (periodicitySeparator == "__END_PERIODICITY_DESCRIPTION__") {

        periodicity_i->first = shape1Entry;
        periodicity_i->second = shape2Entry;

        _facesPeriodicityVector.push_back(*periodicity_i);

        break; // __END_PERIODICITY_DESCRIPTION__
      }

      if (periodicitySeparator == "__BEGIN_ENTRY1__") {  // __BEGIN_ENTRY1__
        isOK = static_cast<bool>(load >> shape1Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_ENTRY1__
        if (periodicitySeparator != "__END_ENTRY1__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_ENTRY2__") {  // __BEGIN_ENTRY2__
        isOK = static_cast<bool>(load >> shape2Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_ENTRY2__
        if (periodicitySeparator != "__END_ENTRY2__")
          throw std::exception();
      }
    }
  }
}


void BLSURFPlugin_Hypothesis::LoadEdgesPeriodicity(std::istream & load){

  bool isOK = true;

  std::string periodicitySeparator;
  TEntry theFace1Entry;
  TEntry theEdge1Entry;
  TEntry theFace2Entry;
  TEntry theEdge2Entry;
  int edge_orientation = 0;

  _edgesPeriodicityVector.clear();

  while (isOK) {
    isOK = static_cast<bool>(load >> periodicitySeparator); // __BEGIN_PERIODICITY_DESCRIPTION__
    TEdgePeriodicity *periodicity_i = new TEdgePeriodicity();
    if (periodicitySeparator == "__EDGES_PERIODICITY_END__")
      break; // __EDGES_PERIODICITY_END__
    if (periodicitySeparator != "__BEGIN_PERIODICITY_DESCRIPTION__"){
      throw std::exception();
    }

    while (isOK) {
      isOK = static_cast<bool>(load >> periodicitySeparator);
      if (periodicitySeparator == "__END_PERIODICITY_DESCRIPTION__") {

        periodicity_i->theFace1Entry = theFace1Entry;
        periodicity_i->theEdge1Entry = theEdge1Entry;
        periodicity_i->theFace2Entry = theFace2Entry;
        periodicity_i->theEdge2Entry = theEdge2Entry;
        periodicity_i->edge_orientation = edge_orientation;

        _edgesPeriodicityVector.push_back(*periodicity_i);

        break; // __END_PERIODICITY_DESCRIPTION__
      }

      if (periodicitySeparator == "__BEGIN_FACE1__") {  // __BEGIN_FACE1__
        isOK = static_cast<bool>(load >> theFace1Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_FACE1__
        if (periodicitySeparator != "__END_FACE1__"){
          throw std::exception();
        }
      }

      if (periodicitySeparator == "__BEGIN_EDGE1__") {  // __BEGIN_EDGE1__
        isOK = static_cast<bool>(load >> theEdge1Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_EDGE1__
        if (periodicitySeparator != "__END_EDGE1__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_FACE2__") {  // __BEGIN_FACE2__
        isOK = static_cast<bool>(load >> theFace2Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_FACE2__
        if (periodicitySeparator != "__END_FACE2__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_EDGE2__") {  // __BEGIN_EDGE2__
        isOK = static_cast<bool>(load >> theEdge2Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_EDGE2__
        if (periodicitySeparator != "__END_EDGE2__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_EDGE_ORIENTATION__") {  // __BEGIN_EDGE_ORIENTATION__
        isOK = static_cast<bool>(load >> edge_orientation);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_EDGE_ORIENTATION__
        if (periodicitySeparator != "__END_EDGE_ORIENTATION__")
          throw std::exception();
      }
    }
  }
}

void BLSURFPlugin_Hypothesis::LoadVerticesPeriodicity(std::istream & load)
{
  bool isOK = true;

  std::string periodicitySeparator;
  TEntry theEdge1Entry;
  TEntry theVertex1Entry;
  TEntry theEdge2Entry;
  TEntry theVertex2Entry;

  _verticesPeriodicityVector.clear();

  while (isOK) {
    isOK = static_cast<bool>(load >> periodicitySeparator); // __BEGIN_PERIODICITY_DESCRIPTION__
    TVertexPeriodicity *periodicity_i = new TVertexPeriodicity();
    if (periodicitySeparator == "__VERTICES_PERIODICITY_END__")
      break; // __VERTICES_PERIODICITY_END__
    if (periodicitySeparator != "__BEGIN_PERIODICITY_DESCRIPTION__"){
      throw std::exception();
    }

    while (isOK) {
      isOK = static_cast<bool>(load >> periodicitySeparator);
      if (periodicitySeparator == "__END_PERIODICITY_DESCRIPTION__") {

        periodicity_i->theEdge1Entry = theEdge1Entry;
        periodicity_i->theVertex1Entry = theVertex1Entry;
        periodicity_i->theEdge2Entry = theEdge2Entry;
        periodicity_i->theVertex2Entry = theVertex2Entry;

        _verticesPeriodicityVector.push_back(*periodicity_i);

        break; // __END_PERIODICITY_DESCRIPTION__
      }

      if (periodicitySeparator == "__BEGIN_EDGE1__") {  // __BEGIN_EDGE1__
        isOK = static_cast<bool>(load >> theEdge1Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_EDGE1__
        if (periodicitySeparator != "__END_EDGE1__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_VERTEX1__") {  // __BEGIN_VERTEX1__
        isOK = static_cast<bool>(load >> theVertex1Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_VERTEX1__
        if (periodicitySeparator != "__END_VERTEX1__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_EDGE2__") {  // __BEGIN_EDGE2__
        isOK = static_cast<bool>(load >> theEdge2Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_EDGE2__
        if (periodicitySeparator != "__END_EDGE2__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_VERTEX2__") {  // __BEGIN_VERTEX2__
        isOK = static_cast<bool>(load >> theVertex2Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_VERTEX2__
        if (periodicitySeparator != "__END_VERTEX2__")
          throw std::exception();
      }
    }
  }
}

void BLSURFPlugin_Hypothesis::LoadPreCADPeriodicity(std::istream & load, const char* shapeType) {

  bool isOK = true;

  std::string periodicitySeparator;
  TEntry shape1Entry;
  TEntry shape2Entry;
  std::vector<std::string> theSourceVerticesEntries;
  std::vector<std::string> theTargetVerticesEntries;

  bool hasSourceVertices = false;
  bool hasTargetVertices = false;

  if ( shapeType  &&  strcmp( shapeType, "FACES") == 0 )
    _preCadFacesPeriodicityVector.clear();
  else
    _preCadEdgesPeriodicityVector.clear();


  while (isOK) {
    isOK = static_cast<bool>(load >> periodicitySeparator); // __BEGIN_PERIODICITY_DESCRIPTION__
    TPreCadPeriodicity *periodicity_i = new TPreCadPeriodicity();
    std::string endSeparator = "__PRECAD_" + std::string(shapeType) + "_PERIODICITY_END__";
    if (periodicitySeparator == endSeparator)
      break; // __PRECAD_FACES_PERIODICITY_END__
    if (periodicitySeparator != "__BEGIN_PERIODICITY_DESCRIPTION__"){
      throw std::exception();
    }

    while (isOK) {
      isOK = static_cast<bool>(load >> periodicitySeparator);
      if (periodicitySeparator == "__END_PERIODICITY_DESCRIPTION__") {

        periodicity_i->shape1Entry = shape1Entry;
        periodicity_i->shape2Entry = shape2Entry;

        if (hasSourceVertices)
          periodicity_i->theSourceVerticesEntries = theSourceVerticesEntries;
        if (hasTargetVertices)
          periodicity_i->theTargetVerticesEntries = theTargetVerticesEntries;

        if ( shapeType  &&  strcmp( shapeType, "FACES" ) == 0 )
          _preCadFacesPeriodicityVector.push_back(*periodicity_i);
        else
          _preCadEdgesPeriodicityVector.push_back(*periodicity_i);

        theSourceVerticesEntries.clear();
        theTargetVerticesEntries.clear();
        hasSourceVertices = false;
        hasTargetVertices = false;
        break; // __END_PERIODICITY_DESCRIPTION__
      }

      if (periodicitySeparator == "__BEGIN_ENTRY1__") {  // __BEGIN_ENTRY1__
        isOK = static_cast<bool>(load >> shape1Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_ENTRY1__
        if (periodicitySeparator != "__END_ENTRY1__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_ENTRY2__") {  // __BEGIN_ENTRY2__
        isOK = static_cast<bool>(load >> shape2Entry);
        isOK = static_cast<bool>(load >> periodicitySeparator); // __END_ENTRY2__
        if (periodicitySeparator != "__END_ENTRY2__")
          throw std::exception();
      }

      if (periodicitySeparator == "__BEGIN_SOURCE_VERTICES_LIST__") {  // __BEGIN_SOURCE_VERTICES_LIST__
        hasSourceVertices = true;
        while (isOK && (periodicitySeparator != "__END_SOURCE_VERTICES_LIST__")) {
          isOK = static_cast<bool>(load >> periodicitySeparator);
          if (periodicitySeparator != "__END_SOURCE_VERTICES_LIST__") {
            theSourceVerticesEntries.push_back(periodicitySeparator);
          }
        }
      }

      if (periodicitySeparator == "__BEGIN_TARGET_VERTICES_LIST__") {  // __BEGIN_TARGET_VERTICES_LIST__
        hasTargetVertices = true;
        while (isOK && (periodicitySeparator != "__END_TARGET_VERTICES_LIST__")) {
          isOK = static_cast<bool>(load >> periodicitySeparator);
          if (periodicitySeparator != "__END_TARGET_VERTICES_LIST__") {
            theTargetVerticesEntries.push_back(periodicitySeparator);
          }
        }
      }
    }
  }
}

//=============================================================================
std::ostream & operator <<(std::ostream & save, BLSURFPlugin_Hypothesis & hyp) {
  return hyp.SaveTo(save);
}

//=============================================================================
std::istream & operator >>(std::istream & load, BLSURFPlugin_Hypothesis & hyp) {
  return hyp.LoadFrom(load);
}

//================================================================================
/*!
 * \brief Does nothing
 */
//================================================================================

bool BLSURFPlugin_Hypothesis::SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape) {
  return false;
}

//================================================================================
/*!
 * \brief Returns default global constant physical size given a default value of element length ratio
 */
//================================================================================

double BLSURFPlugin_Hypothesis::GetDefaultPhySize(double diagonal, double bbSegmentation) {
  if (bbSegmentation != 0 && diagonal != 0)
    return diagonal / bbSegmentation ;
  return 10;
}

//================================================================================
/*!
 * \brief Returns default min size given a default value of element length ratio
 */
//================================================================================

double BLSURFPlugin_Hypothesis::GetDefaultMinSize(double diagonal) {
  if (diagonal != 0)
    return diagonal / 1000.0 ;
  return undefinedDouble();
}

//================================================================================
/*!
 * \brief Returns default max size given a default value of element length ratio
 */
//================================================================================

double BLSURFPlugin_Hypothesis::GetDefaultMaxSize(double diagonal) {
  if (diagonal != 0)
    return diagonal / 5.0 ;
  return undefinedDouble();
}

//================================================================================
/*!
 * \brief Returns default chordal error given a default value of element length ratio
 */
//================================================================================

double BLSURFPlugin_Hypothesis::GetDefaultChordalError(double diagonal) {
  if (diagonal != 0)
    return diagonal;
  return undefinedDouble();
}

//================================================================================
/*!
 * \brief Returns default tiny edge length given a default value of element length ratio
 */
//================================================================================

double BLSURFPlugin_Hypothesis::GetDefaultTinyEdgeLength(double diagonal) {
  if (diagonal != 0)
    return diagonal * 1e-6 ;
  return undefinedDouble();
}

//================================================================================
/*!
 * \brief Returns default tiny edge optimisation length given a default value of element length ratio
 */
//================================================================================

double BLSURFPlugin_Hypothesis::GetDefaultTinyEdgeOptimisationLength(double diagonal) {
  if (diagonal != 0)
    return diagonal * 1e-6 ;
  return undefinedDouble();
}

//=============================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//=============================================================================

bool BLSURFPlugin_Hypothesis::SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh) {
  double diagonal = dflts._elemLength*_gen->GetBoundaryBoxSegmentation();
  _phySize = GetDefaultPhySize(diagonal, _gen->GetBoundaryBoxSegmentation());
  _minSize = GetDefaultMinSize(diagonal);
  _maxSize = GetDefaultMaxSize(diagonal);
  _chordalError = 0.5 * _phySize; //GetDefaultChordalError(diagonal); IMP 0023307
  _tinyEdgeLength = GetDefaultTinyEdgeLength(diagonal);
  _tinyEdgeOptimisationLength = GetDefaultTinyEdgeOptimisationLength(diagonal);

  return true;
}

//================================================================================
/*!
 * \brief Converts a string to a bool
 */
//================================================================================

bool BLSURFPlugin_Hypothesis::ToBool(const std::string& str, bool* isOk )
  throw (std::invalid_argument)
{
  std::string s = str;
  if ( isOk ) *isOk = true;

  for ( size_t i = 0; i <= s.size(); ++i )
    s[i] = tolower( s[i] );

  if ( s == "1" || s == "true" || s == "active" || s == "yes" )
    return true;

  if ( s == "0" || s == "false" || s == "inactive" || s == "no" )
    return false;

  if ( isOk )
    *isOk = false;
  else {
    std::string msg = "Not a Boolean value:'" + str + "'";
    throw std::invalid_argument(msg);
  }
  return false;
}

//================================================================================
/*!
 * \brief Converts a string to a real value
 */
//================================================================================

double BLSURFPlugin_Hypothesis::ToDbl(const std::string& str, bool* isOk )
  throw (std::invalid_argument)
{
  if ( str.empty() ) throw std::invalid_argument("Empty value provided");

  char * endPtr;
  double val = strtod(&str[0], &endPtr);
  bool ok = (&str[0] != endPtr);

  if ( isOk ) *isOk = ok;

  if ( !ok )
  {
    std::string msg = "Not a real value:'" + str + "'";
    throw std::invalid_argument(msg);
  }
  return val;
}

//================================================================================
/*!
 * \brief Converts a string to a integer value
 */
//================================================================================

int BLSURFPlugin_Hypothesis::ToInt(const std::string& str, bool* isOk )
  throw (std::invalid_argument)
{
  if ( str.empty() ) throw std::invalid_argument("Empty value provided");

  char * endPtr;
  int val = (int)strtol( &str[0], &endPtr, 10);
  bool ok = (&str[0] != endPtr);

  if ( isOk ) *isOk = ok;

  if ( !ok )
  {
    std::string msg = "Not an integer value:'" + str + "'";
    throw std::invalid_argument(msg);
  }
  return val;
}

