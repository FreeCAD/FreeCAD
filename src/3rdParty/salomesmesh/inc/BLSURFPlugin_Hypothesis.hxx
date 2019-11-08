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
// File    : BLSURFPlugin_Hypothesis.hxx
// Authors : Francis KLOSS (OCC) & Patrick LAUG (INRIA) & Lioka RAZAFINDRAZAKA (CEA)
//           & Aurelien ALLEAUME (DISTENE)
//           Size maps developement: Nicolas GEIMER (OCC) & Gilles DAVID (EURIWARE)
// ---
//
#ifndef _BLSURFPlugin_Hypothesis_HXX_
#define _BLSURFPlugin_Hypothesis_HXX_

#include <BLSURFPlugin_Defs.hxx>

#include "SMESH_Hypothesis.hxx"
#include <vector>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <cstring>
#include <sstream>
#include <utilities.h>
#include "BLSURFPlugin_Attractor.hxx"

//  Parameters for work of MG-CADSurf

class BLSURFPLUGIN_EXPORT BLSURFPlugin_Hypothesis: public SMESH_Hypothesis
{
public:
  BLSURFPlugin_Hypothesis(int hypId, int studyId, SMESH_Gen * gen, bool hasgeom);

  enum Topology {
    FromCAD,
    Process,
    Process2,
    PreCAD
  };

  enum PhysicalMesh {
    DefaultSize,
    PhysicalGlobalSize,
    PhysicalLocalSize
  };

  enum GeometricMesh {
    DefaultGeom,
    GeometricalGlobalSize,
    GeometricalLocalSize
  };

  static const char* GetHypType(bool hasgeom)
  { return hasgeom ? "MG-CADSurf Parameters" : "MG-CADSurf Parameters_NOGEOM"; }

  TopoDS_Shape entryToShape(std::string entry);

  void SetPhysicalMesh(PhysicalMesh thePhysicalMesh);
  PhysicalMesh GetPhysicalMesh() const { return _physicalMesh; }

  void SetGeometricMesh(GeometricMesh theGeometricMesh);
  GeometricMesh GetGeometricMesh() const { return _geometricMesh; }

  void SetPhySize(double thePhySize, bool isRelative = false);
  double GetPhySize() const { return _phySize; }
  bool IsPhySizeRel() const { return _phySizeRel; }

  void SetMinSize(double theMinSize, bool isRelative = false);
  double GetMinSize() const { return _minSize; }
  bool IsMinSizeRel() const { return _minSizeRel; }

  void SetMaxSize(double theMaxSize, bool isRelative = false);
  double GetMaxSize() const { return _maxSize; }
  bool IsMaxSizeRel() const { return _maxSizeRel; }

  void SetUseGradation(bool toUse);
  bool GetUseGradation() const { return _useGradation; }
  void SetGradation(double theGradation);
  double GetGradation() const { return _gradation; }

  void SetUseVolumeGradation(bool toUse);
  bool GetUseVolumeGradation() const { return _useVolumeGradation; }
  void SetVolumeGradation(double theGradation);
  double GetVolumeGradation() const { return _volumeGradation; }

  void SetQuadAllowed(bool theVal);
  bool GetQuadAllowed() const { return _quadAllowed; }

  void SetAngleMesh(double theAngle);
  double GetAngleMesh() const { return _angleMesh; }

  void SetChordalError(double theDistance);
  double GetChordalError() const { return _chordalError; }

  void SetAnisotropic(bool theVal);
  bool GetAnisotropic() const { return _anisotropic; }

  void SetAnisotropicRatio(double theVal);
  double GetAnisotropicRatio() const { return _anisotropicRatio; }

  void SetRemoveTinyEdges(bool theVal);
  bool GetRemoveTinyEdges() const { return _removeTinyEdges; }

  void SetTinyEdgeLength(double theVal);
  double GetTinyEdgeLength() const { return _tinyEdgeLength; }

  void SetOptimiseTinyEdges(bool theVal);
  bool GetOptimiseTinyEdges() const { return _optimiseTinyEdges; }

  void SetTinyEdgeOptimisationLength(double theVal);
  double GetTinyEdgeOptimisationLength() const { return _tinyEdgeOptimisationLength; }

  void SetCorrectSurfaceIntersection(bool theVal);
  bool GetCorrectSurfaceIntersection() const { return _correctSurfaceIntersec; }

  void SetCorrectSurfaceIntersectionMaxCost(double theVal);
  double GetCorrectSurfaceIntersectionMaxCost() const { return _corrSurfaceIntersCost; }

  void SetBadElementRemoval(bool theVal);
  bool GetBadElementRemoval() const { return _badElementRemoval; }

  void SetBadElementAspectRatio(double theVal);
  double GetBadElementAspectRatio() const { return _badElementAspectRatio; }

  void SetOptimizeMesh(bool theVal);
  bool GetOptimizeMesh() const { return _optimizeMesh; }

  void SetQuadraticMesh(bool theVal);
  bool GetQuadraticMesh() const { return _quadraticMesh; }

  void SetTopology(Topology theTopology);
  Topology GetTopology() const { return _topology; }

  void SetVerbosity(int theVal);
  int GetVerbosity() const { return _verb; }
  
  void ClearEntry(const std::string& entry, const char * attEntry = 0);
  void ClearSizeMaps();

  void SetEnforceCadEdgesSize( bool toEnforce );
  bool GetEnforceCadEdgesSize();

  void SetJacobianRectificationRespectGeometry( bool allowRectification );
  bool GetJacobianRectificationRespectGeometry();
    
  void SetUseDeprecatedPatchMesher( bool useDeprecatedPatchMesher );
  bool GetUseDeprecatedPatchMesher();

  void SetJacobianRectification( bool allowRectification );
  bool GetJacobianRectification();

  void SetMaxNumberOfPointsPerPatch( int nb ) throw (std::invalid_argument);
  int  GetMaxNumberOfPointsPerPatch();

  void SetMaxNumberOfThreads( int nb ) throw (std::invalid_argument);
  int  GetMaxNumberOfThreads();

  void SetRespectGeometry( bool toRespect );
  bool GetRespectGeometry();

  void SetTinyEdgesAvoidSurfaceIntersections( bool toAvoidIntersection );
  bool GetTinyEdgesAvoidSurfaceIntersections();

  void SetClosedGeometry( bool isClosed );
  bool GetClosedGeometry();

  void SetDebug( bool isDebug );
  bool GetDebug();

  void SetPeriodicTolerance( double tol ) throw (std::invalid_argument);
  double GetPeriodicTolerance();

  void SetRequiredEntities( const std::string& howToTreat ) throw (std::invalid_argument);
  std::string GetRequiredEntities();

  void SetSewingTolerance( double tol ) throw (std::invalid_argument);
  double GetSewingTolerance();

  void SetTags( const std::string& howToTreat ) throw (std::invalid_argument);
  std::string GetTags();

  // Hyper-patches
  typedef std::set< int > THyperPatchTags;
  typedef std::vector< THyperPatchTags > THyperPatchList;

  void SetHyperPatches(const THyperPatchList& hpl);
  const THyperPatchList& GetHyperPatches() const { return _hyperPatchList; }
  static int GetHyperPatchTag( int faceTag, const BLSURFPlugin_Hypothesis* hyp, int* iPatch=0 );

  void SetPreCADMergeEdges(bool theVal);
  bool GetPreCADMergeEdges() const { return _preCADMergeEdges; }

  void SetPreCADRemoveDuplicateCADFaces(bool theVal);
  bool GetPreCADRemoveDuplicateCADFaces() const { return _preCADRemoveDuplicateCADFaces; }

  void SetPreCADProcess3DTopology(bool theVal);
  bool GetPreCADProcess3DTopology() const { return _preCADProcess3DTopology; }

  void SetPreCADDiscardInput(bool theVal);
  bool GetPreCADDiscardInput() const { return _preCADDiscardInput; }

  static bool HasPreCADOptions(const BLSURFPlugin_Hypothesis* hyp);
    
  typedef std::map<std::string,std::string> TSizeMap;

  void SetSizeMapEntry(const std::string& entry,const std::string& sizeMap );
  std::string  GetSizeMapEntry(const std::string& entry);
  const TSizeMap& _GetSizeMapEntries() const { return _sizeMap; }
  /*!
   * \brief Return the size maps
   */
  static TSizeMap GetSizeMapEntries(const BLSURFPlugin_Hypothesis* hyp);


  void SetAttractorEntry(const std::string& entry,const std::string& attractor );
  std::string GetAttractorEntry(const std::string& entry);
  const TSizeMap& _GetAttractorEntries() const { return _attractors; };
  /*!
   * \brief Return the attractors
   */
  static TSizeMap GetAttractorEntries(const BLSURFPlugin_Hypothesis* hyp);


/*
  void SetCustomSizeMapEntry(const std::string& entry,const std::string& sizeMap );
  std::string  GetCustomSizeMapEntry(const std::string& entry);
  void UnsetCustomSizeMap(const std::string& entry);
  const TSizeMap& GetCustomSizeMapEntries() const { return _customSizeMap; }
 */
  
  typedef std::multimap< std::string, BLSURFPlugin_Attractor* > TAttractorMap;
  typedef std::map< std::string, std::vector<double> > TParamsMap; //TODO à finir 
  
  void SetClassAttractorEntry(const std::string& entry, const std::string& att_entry, double StartSize, double EndSize, double ActionRadius, double ConstantRadius);
  std::string  GetClassAttractorEntry(const std::string& entry);
  const TAttractorMap& _GetClassAttractorEntries() const { return _classAttractors; }
  /*!
   * \brief Return the attractors entries
   */
  static TAttractorMap GetClassAttractorEntries(const BLSURFPlugin_Hypothesis* hyp);

  /*!
   * To set/get/unset an enforced vertex
   */
  // Name
  typedef std::string TEnfName;
  // Entry
  typedef std::string TEntry;
  // List of entries
  typedef std::set<TEntry> TEntryList;
  // Group name
  typedef std::string TEnfGroupName;
  // Coordinates
  typedef std::vector<double> TEnfVertexCoords;
  typedef std::set< TEnfVertexCoords > TEnfVertexCoordsList;

  // Enforced vertex
  struct TEnfVertex {
    TEnfName name;
    TEntry geomEntry;
    TEnfVertexCoords coords;
    TEnfGroupName grpName;
    TEntryList faceEntries;
    TopoDS_Vertex vertex;
  };
    
  struct CompareEnfVertices
  {
    bool operator () (const TEnfVertex* e1, const TEnfVertex* e2) const {
      if (e1 && e2) {
        if (e1->coords.size() && e2->coords.size())
          return (e1->coords < e2->coords);
        else
          return (e1->geomEntry < e2->geomEntry);
      }
      return false;
    }
  };

  // PreCad Face and Edge periodicity
  struct TPreCadPeriodicity {
    TEntry shape1Entry;
    TEntry shape2Entry;
    std::vector<std::string> theSourceVerticesEntries;
    std::vector<std::string> theTargetVerticesEntries;
  };

  // Edge periodicity
  struct TEdgePeriodicity {
    TEntry theFace1Entry;
    TEntry theEdge1Entry;
    TEntry theFace2Entry;
    TEntry theEdge2Entry;
    int edge_orientation;
  };

  // Vertex periodicity
  struct TVertexPeriodicity {
    TEntry theEdge1Entry;
    TEntry theVertex1Entry;
    TEntry theEdge2Entry;
    TEntry theVertex2Entry;
  };

  typedef std::pair< TEntry, TEntry > TFacesPeriodicity;

  // List of enforced vertices
  typedef std::set< TEnfVertex*, CompareEnfVertices > TEnfVertexList;

  // Map Face Entry / List of enforced vertices
  typedef std::map< TEntry, TEnfVertexList > TFaceEntryEnfVertexListMap;

  // List of Face Entry with internal enforced vertices activated
  typedef std::set< TEntry > TFaceEntryInternalVerticesList;

  // Map Face Entry / List of coords
  typedef std::map< TEntry, TEnfVertexCoordsList > TFaceEntryCoordsListMap;

  // Map Face Entry / List of Vertex entry
  typedef std::map< TEntry, TEntryList > TFaceEntryEnfVertexEntryListMap;
  
  // Map Coords / Enforced vertex
  typedef std::map< TEnfVertexCoords, TEnfVertex* > TCoordsEnfVertexMap;

  // Map Vertex entry / Enforced vertex
  typedef std::map< TEntry, TEnfVertex* > TEnfVertexEntryEnfVertexMap;

  typedef std::map< TEnfGroupName, std::set<int> > TGroupNameNodeIDMap;
  /* TODO GROUPS
  // Map Group Name / List of enforced vertices
  typedef std::map< TEnfGroupName , TEnfVertexList > TGroupNameEnfVertexListMap;
  */

  // Vector of pairs of entries
  typedef std::vector< TPreCadPeriodicity > TPreCadPeriodicityVector;
  typedef std::vector< TFacesPeriodicity > TFacesPeriodicityVector;
  typedef std::vector< TEdgePeriodicity > TEdgesPeriodicityVector;
  typedef std::vector< TVertexPeriodicity > TVerticesPeriodicityVector;
  

  bool                  SetEnforcedVertex(TEntry theFaceEntry, TEnfName theVertexName, TEntry theVertexEntry, TEnfGroupName theGroupName,
                                          double x = 0.0, double y = 0.0, double z = 0.0);
  TEnfVertexList        GetEnfVertexList(const TEntry& theFaceEntry) throw (std::invalid_argument);
  TEnfVertexCoordsList  GetEnfVertexCoordsList(const TEntry& theFaceEntry) throw (std::invalid_argument);
  TEntryList            GetEnfVertexEntryList (const TEntry& theFaceEntry) throw (std::invalid_argument);
  TEnfVertex*           GetEnfVertex(TEnfVertexCoords coords) throw (std::invalid_argument);
  TEnfVertex*           GetEnfVertex(const TEntry& theEnfVertexEntry) throw (std::invalid_argument);
  void                  AddEnfVertexNodeID(TEnfGroupName theGroupName,int theNodeID);
  std::set<int>         GetEnfVertexNodeIDs(TEnfGroupName theGroupName) throw (std::invalid_argument);
  void                  RemoveEnfVertexNodeID(TEnfGroupName theGroupName,int theNodeID) throw (std::invalid_argument);
  
  bool ClearEnforcedVertex(const TEntry& theFaceEntry, double x = 0.0, double y = 0.0, double z = 0.0, const TEntry& theVertexEntry="") throw (std::invalid_argument);
  bool ClearEnforcedVertices(const TEntry& theFaceEntry) throw (std::invalid_argument);

  void ClearAllEnforcedVertices();

  const TFaceEntryEnfVertexListMap  _GetAllEnforcedVerticesByFace() const { return _faceEntryEnfVertexListMap; }
  const TEnfVertexList              _GetAllEnforcedVertices() const { return _enfVertexList; }

  const TFaceEntryCoordsListMap     _GetAllCoordsByFace() const { return _faceEntryCoordsListMap; }
  const TCoordsEnfVertexMap         _GetAllEnforcedVerticesByCoords() const { return _coordsEnfVertexMap; }

  const TFaceEntryEnfVertexEntryListMap _GetAllEnfVertexEntriesByFace() const { return _faceEntryEnfVertexEntryListMap; }
  const TEnfVertexEntryEnfVertexMap     _GetAllEnforcedVerticesByEnfVertexEntry() const { return _enfVertexEntryEnfVertexMap; }

//   TODO GROUPS
//   const TEnfVertexGroupNameMap _GetEnforcedVertexGroupNameMap() const { return _enfVertexGroupNameMap; }
  

  /*!
   * \brief Return the enforced vertices
   */
  static TFaceEntryEnfVertexListMap       GetAllEnforcedVerticesByFace(const BLSURFPlugin_Hypothesis* hyp);
  static TEnfVertexList                   GetAllEnforcedVertices(const BLSURFPlugin_Hypothesis* hyp);

  static TFaceEntryCoordsListMap          GetAllCoordsByFace(const BLSURFPlugin_Hypothesis* hyp);
  static TCoordsEnfVertexMap              GetAllEnforcedVerticesByCoords(const BLSURFPlugin_Hypothesis* hyp);

  static TFaceEntryEnfVertexEntryListMap  GetAllEnfVertexEntriesByFace(const BLSURFPlugin_Hypothesis* hyp);
  static TEnfVertexEntryEnfVertexMap      GetAllEnforcedVerticesByEnfVertexEntry(const BLSURFPlugin_Hypothesis* hyp);

  /*!
   * \brief Internal enforced vertices
   */
  void SetInternalEnforcedVertexAllFaces(bool toEnforceInternalVertices);
  const bool _GetInternalEnforcedVertexAllFaces() const { return _enforcedInternalVerticesAllFaces; }
  static bool GetInternalEnforcedVertexAllFaces( const BLSURFPlugin_Hypothesis* hyp );
  void SetInternalEnforcedVertexAllFacesGroup(TEnfGroupName theGroupName);
  const TEnfGroupName _GetInternalEnforcedVertexAllFacesGroup() const { return _enforcedInternalVerticesAllFacesGroup; }
  static TEnfGroupName GetInternalEnforcedVertexAllFacesGroup( const BLSURFPlugin_Hypothesis* hyp );

//  Enable internal enforced vertices on specific face if requested by user
//  static TFaceEntryInternalVerticesList GetDefaultFaceEntryInternalVerticesMap() { return TFaceEntryInternalVerticesList(); }
//  const TFaceEntryInternalVerticesList  _GetAllInternalEnforcedVerticesByFace() const { return _faceEntryInternalVerticesList; }
//  static TFaceEntryInternalVerticesList GetAllInternalEnforcedVerticesByFace(const BLSURFPlugin_Hypothesis* hyp);
//  void SetInternalEnforcedVertex(TEntry theFaceEntry, bool toEnforceInternalVertices, TEnfGroupName theGroupName);
//  bool GetInternalEnforcedVertex(const TEntry& theFaceEntry);

  static PhysicalMesh    GetDefaultPhysicalMesh() { return PhysicalGlobalSize; }
  static GeometricMesh   GetDefaultGeometricMesh() { return GeometricalGlobalSize; }
  static double          GetDefaultPhySize(double diagonal, double bbSegmentation);
  static double          GetDefaultPhySize() { return undefinedDouble(); }
  static bool            GetDefaultPhySizeRel() { return false; }
  static double          GetDefaultMinSize(double diagonal);
  static double          GetDefaultMinSize() { return undefinedDouble(); }
  static bool            GetDefaultMinSizeRel() { return false; }
  static double          GetDefaultMaxSize(double diagonal);
  static double          GetDefaultMaxSize() { return undefinedDouble(); }
  static bool            GetDefaultMaxSizeRel() { return false; }
  static bool            GetDefaultUseGradation() { return true; }
  static double          GetDefaultGradation() { return 1.3; }
  static bool            GetDefaultUseVolumeGradation() { return false; }
  static double          GetDefaultVolumeGradation() { return 2; }
  static bool            GetDefaultQuadAllowed() { return false; }
  static double          GetDefaultAngleMesh() { return 8.0; }
  
  static double          GetDefaultChordalError(double diagonal);
  static double          GetDefaultChordalError() { return undefinedDouble(); }
  static bool            GetDefaultAnisotropic() { return false; }
  static double          GetDefaultAnisotropicRatio() { return 0.0; }
  static bool            GetDefaultRemoveTinyEdges() { return false; }
  static double          GetDefaultTinyEdgeLength(double diagonal);
  static double          GetDefaultTinyEdgeLength() { return undefinedDouble(); }
  static bool            GetDefaultOptimiseTinyEdges() { return false; }
  static double          GetDefaultTinyEdgeOptimisationLength(double diagonal);
  static double          GetDefaultTinyEdgeOptimisationLength() { return undefinedDouble(); }
  static bool            GetDefaultCorrectSurfaceIntersection() { return true; }
  static double          GetDefaultCorrectSurfaceIntersectionMaxCost() { return 15.; }
  static bool            GetDefaultBadElementRemoval() { return false; }
  static double          GetDefaultBadElementAspectRatio() {return 1000.0; } 
  static bool            GetDefaultOptimizeMesh() { return true; }
  static bool            GetDefaultQuadraticMesh() { return false; }
  
  static int             GetDefaultVerbosity() { return 3; }
  static Topology        GetDefaultTopology() { return FromCAD; }
  // PreCAD
  static bool            GetDefaultPreCADMergeEdges() { return false; }
  static bool            GetDefaultPreCADRemoveDuplicateCADFaces() { return false; }
  static bool            GetDefaultPreCADProcess3DTopology() { return false; }
  static bool            GetDefaultPreCADDiscardInput() { return false; }
  
  static TSizeMap        GetDefaultSizeMap() { return TSizeMap();}
  static TAttractorMap   GetDefaultAttractorMap() { return TAttractorMap(); }

  static TFaceEntryEnfVertexListMap       GetDefaultFaceEntryEnfVertexListMap() { return TFaceEntryEnfVertexListMap(); }
  static TEnfVertexList                   GetDefaultEnfVertexList() { return TEnfVertexList(); }
  static TFaceEntryCoordsListMap          GetDefaultFaceEntryCoordsListMap() { return TFaceEntryCoordsListMap(); }
  static TCoordsEnfVertexMap              GetDefaultCoordsEnfVertexMap() { return TCoordsEnfVertexMap(); }
  static TFaceEntryEnfVertexEntryListMap  GetDefaultFaceEntryEnfVertexEntryListMap() { return TFaceEntryEnfVertexEntryListMap(); }
  static TEnfVertexEntryEnfVertexMap      GetDefaultEnfVertexEntryEnfVertexMap() { return TEnfVertexEntryEnfVertexMap(); }
  static TGroupNameNodeIDMap              GetDefaultGroupNameNodeIDMap() { return TGroupNameNodeIDMap(); }

  static bool            GetDefaultInternalEnforcedVertex() { return false; }

  /* TODO GROUPS
  static TGroupNameEnfVertexListMap GetDefaultGroupNameEnfVertexListMap() { return TGroupNameEnfVertexListMap(); }
  static TEnfVertexGroupNameMap     GetDefaultEnfVertexGroupNameMap() { return TEnfVertexGroupNameMap(); }
  */

//  const TPreCadPeriodicityEntriesVector _GetPreCadFacesPeriodicityEntries() const { return _preCadFacesPeriodicityEntriesVector; }

  static TPreCadPeriodicityVector GetDefaultPreCadFacesPeriodicityVector() { return TPreCadPeriodicityVector(); }
  const TPreCadPeriodicityVector  _GetPreCadFacesPeriodicityVector() const { return _preCadFacesPeriodicityVector; }
  static TPreCadPeriodicityVector GetPreCadFacesPeriodicityVector(const BLSURFPlugin_Hypothesis* hyp);

  static TPreCadPeriodicityVector GetDefaultPreCadEdgesPeriodicityVector() { return TPreCadPeriodicityVector(); }
  const TPreCadPeriodicityVector  _GetPreCadEdgesPeriodicityVector() const { return _preCadEdgesPeriodicityVector; }
  static TPreCadPeriodicityVector GetPreCadEdgesPeriodicityVector(const BLSURFPlugin_Hypothesis* hyp);

  static TFacesPeriodicityVector GetDefaultFacesPeriodicityVector() { return TFacesPeriodicityVector(); }
  const TFacesPeriodicityVector  _GetFacesPeriodicityVector() const { return _facesPeriodicityVector; }
  static TFacesPeriodicityVector GetFacesPeriodicityVector(const BLSURFPlugin_Hypothesis* hyp);

  static TEdgesPeriodicityVector GetDefaultEdgesPeriodicityVector() { return TEdgesPeriodicityVector(); }
  const TEdgesPeriodicityVector  _GetEdgesPeriodicityVector() const { return _edgesPeriodicityVector; }
  static TEdgesPeriodicityVector GetEdgesPeriodicityVector(const BLSURFPlugin_Hypothesis* hyp);

  static TVerticesPeriodicityVector GetDefaultVerticesPeriodicityVector() { return TVerticesPeriodicityVector(); }
  const TVerticesPeriodicityVector  _GetVerticesPeriodicityVector() const { return _verticesPeriodicityVector; }
  static TVerticesPeriodicityVector GetVerticesPeriodicityVector(const BLSURFPlugin_Hypothesis* hyp);

  void ClearPreCadPeriodicityVectors();

  void AddPreCadFacesPeriodicity(TEntry theFace1Entry, TEntry theFace2Entry,
      std::vector<std::string> &theSourceVerticesEntries, std::vector<std::string> &theTargetVerticesEntries);
  void AddPreCadEdgesPeriodicity(TEntry theEdge1Entry, TEntry theEdge2Entry,
      std::vector<std::string> &theSourceVerticesEntries, std::vector<std::string> &theTargetVerticesEntries);

  static double undefinedDouble() { return -1.0; }

  typedef std::map< std::string, std::string > TOptionValues;
  typedef std::set< std::string >              TOptionNames;

  void SetOptionValue(const std::string& optionName,
                      const std::string& optionValue) throw (std::invalid_argument);
  void SetPreCADOptionValue(const std::string& optionName,
                            const std::string& optionValue) throw (std::invalid_argument);
  std::string GetOptionValue(const std::string& optionName, bool* isDefault=0) const throw (std::invalid_argument);
  std::string GetPreCADOptionValue(const std::string& optionName, bool* isDefault=0) const throw (std::invalid_argument);
  void ClearOption(const std::string& optionName);
  void ClearPreCADOption(const std::string& optionName);
  TOptionValues        GetOptionValues()       const;
  TOptionValues        GetPreCADOptionValues() const;
  const TOptionValues& GetCustomOptionValues() const { return _customOption2value; }

  void AddOption(const std::string& optionName, const std::string& optionValue);
  void AddPreCADOption(const std::string& optionName, const std::string& optionValue);
  std::string GetOption(const std::string& optionName) const;
  std::string GetPreCADOption(const std::string& optionName) const;

  static bool  ToBool(const std::string& str, bool* isOk=0) throw (std::invalid_argument);
  static double ToDbl(const std::string& str, bool* isOk=0) throw (std::invalid_argument);
  static int    ToInt(const std::string& str, bool* isOk=0) throw (std::invalid_argument);

  /*!
    * Sets the file for export resulting mesh in GMF format
    */
//   void SetGMFFile(const std::string& theFileName, bool isBinary);
  void SetGMFFile(const std::string& theFileName);
  std::string GetGMFFile() const { return _GMFFileName; }
  static std::string GetDefaultGMFFile() { return "";}
//   bool GetGMFFileMode() const { return _GMFFileMode; }
  
  // Persistence
  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  friend std::ostream & operator <<(std::ostream & save, BLSURFPlugin_Hypothesis & hyp);
  friend std::istream & operator >>(std::istream & load, BLSURFPlugin_Hypothesis & hyp);

  /*!
   * \brief Does nothing
   * \param theMesh - the built mesh
   * \param theShape - the geometry of interest
   * \retval bool - always false
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

  /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);


private:
  PhysicalMesh    _physicalMesh;
  GeometricMesh   _geometricMesh;
  double          _phySize;
  bool            _phySizeRel;
  double          _minSize, _maxSize;
  bool            _minSizeRel, _maxSizeRel;
  bool            _useGradation;
  double          _gradation;
  bool            _useVolumeGradation;
  double          _volumeGradation;
  bool            _quadAllowed;
  double          _angleMesh;
  double          _chordalError;
  bool            _anisotropic;
  double          _anisotropicRatio;
  bool            _removeTinyEdges;
  double          _tinyEdgeLength;
  bool            _optimiseTinyEdges;
  double          _tinyEdgeOptimisationLength;
  bool            _correctSurfaceIntersec;
  double          _corrSurfaceIntersCost;
  bool            _badElementRemoval;
  double          _badElementAspectRatio;
  bool            _optimizeMesh;
  bool            _quadraticMesh;
  int             _verb;
  Topology        _topology;
  
  bool            _preCADMergeEdges;
  bool            _preCADRemoveDuplicateCADFaces;
  bool            _preCADProcess3DTopology;
  bool            _preCADDiscardInput;
  double          _preCADEpsNano;
  
  TOptionValues   _option2value, _preCADoption2value, _customOption2value; // user defined values
  TOptionValues   _defaultOptionValues;               // default values
  TOptionNames    _doubleOptions, _charOptions, _boolOptions; // to find a type of option
  TOptionNames    _preCADdoubleOptions, _preCADcharOptions;

  TSizeMap        _sizeMap;
  TSizeMap        _attractors;
  TAttractorMap   _classAttractors;
  TSizeMap        _attEntry;
  TParamsMap      _attParams;

  TFaceEntryEnfVertexListMap      _faceEntryEnfVertexListMap;
  TEnfVertexList                  _enfVertexList;
  // maps to get "manual" enf vertex (through their coordinates)
  TFaceEntryCoordsListMap         _faceEntryCoordsListMap;
  TCoordsEnfVertexMap             _coordsEnfVertexMap;
  // maps to get "geom" enf vertex (through their geom entries)
  TFaceEntryEnfVertexEntryListMap _faceEntryEnfVertexEntryListMap;
  TEnfVertexEntryEnfVertexMap     _enfVertexEntryEnfVertexMap;
  TGroupNameNodeIDMap             _groupNameNodeIDMap;
  
//  Enable internal enforced vertices on specific face if requested by user
//  TFaceEntryInternalVerticesList  _faceEntryInternalVerticesList;
  bool            _enforcedInternalVerticesAllFaces;
  TEnfGroupName   _enforcedInternalVerticesAllFacesGroup;
  
  TPreCadPeriodicityVector _preCadFacesPeriodicityVector;
  TPreCadPeriodicityVector _preCadEdgesPeriodicityVector;

  TFacesPeriodicityVector _facesPeriodicityVector;
  TEdgesPeriodicityVector _edgesPeriodicityVector;
  TVerticesPeriodicityVector _verticesPeriodicityVector;

  THyperPatchList _hyperPatchList;

  // Called by SaveTo to store content of _preCadFacesPeriodicityVector and _preCadEdgesPeriodicityVector
  void SavePreCADPeriodicity(std::ostream & save, const char* shapeType);

  // Called by LoadFrom to fill _preCadFacesPeriodicityVector and _preCadEdgesPeriodicityVector
  void LoadPreCADPeriodicity(std::istream & load, const char* shapeType);

  // Called by LoadFrom to fill _facesPeriodicityVector
  void LoadFacesPeriodicity(std::istream & load);

  // Called by LoadFrom to fill _edgesPeriodicityVector
  void LoadEdgesPeriodicity(std::istream & load);

  // Called by LoadFrom to fill _verticesPeriodicityVector
  void LoadVerticesPeriodicity(std::istream & load);

  // Called by SaveTo to store content of _facesPeriodicityVector
  void SaveFacesPeriodicity(std::ostream & save);

  // Called by SaveTo to store content of _edgesPeriodicityVector
  void SaveEdgesPeriodicity(std::ostream & save);

  // Called by SaveTo to store content of _verticesPeriodicityVector
  void SaveVerticesPeriodicity(std::ostream & save);

  std::string     _GMFFileName;
//   bool            _GMFFileMode;

//   TSizeMap      _customSizeMap;
};

#endif
