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

//  File   : SMESH_Mesh.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMDS_MeshVolume.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMESHDS_Document.hxx"
#include "SMESHDS_Group.hxx"
#include "SMESHDS_GroupOnGeom.hxx"
#include "SMESHDS_Script.hxx"
#include "SMESHDS_TSubMeshHolder.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Group.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Hypothesis.hxx"
#include "SMESH_subMesh.hxx"

#include "utilities.h"
#include "SMESH_Exception.hxx"
#include "DriverDAT_W_SMDS_Mesh.h"
#include "DriverDAT_R_SMDS_Mesh.h"
#include "DriverGMF_Read.hxx"
#include "DriverGMF_Write.hxx"
#include "DriverMED_R_SMESHDS_Mesh.h"
#include "DriverMED_W_SMESHDS_Mesh.h"
#include "DriverSTL_R_SMDS_Mesh.h"
#include "DriverSTL_W_SMDS_Mesh.h"
#include "DriverUNV_R_SMDS_Mesh.h"
#include "DriverUNV_W_SMDS_Mesh.h"
#ifdef WITH_CGNS
#include "DriverCGNS_Read.hxx"
#include "DriverCGNS_Write.hxx"
#endif

#include <GEOMUtils.hxx>

//#undef _Precision_HeaderFile
#include <BRepBndLib.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Bnd_Box.hxx>
#include <TColStd_MapOfInteger.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS_Iterator.hxx>

#include "SMESH_TryCatch.hxx" // include after OCCT headers!

#include "Utils_ExceptHandlers.hxx"

#ifndef WIN32
#include <boost/thread/thread.hpp>
#include <boost/bind/bind.hpp>
#else 
#include <boost/thread/thread.hpp>
#include <boost/bind/bind.hpp>
//#include <pthread.h>
#endif

using namespace std;

// maximum stored group name length in MED file
#define MAX_MED_GROUP_NAME_LENGTH 80

#ifdef _DEBUG_
static int MYDEBUG = 0;
#else
static int MYDEBUG = 0;
#endif

#define cSMESH_Hyp(h) static_cast<const SMESH_Hypothesis*>(h)

typedef SMESH_HypoFilter THypType;

class SMESH_Mesh::SubMeshHolder : public SMESHDS_TSubMeshHolder< SMESH_subMesh >
{
};

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESH_Mesh::SMESH_Mesh(int               theLocalId, 
                       int               theStudyId, 
                       SMESH_Gen*        theGen,
                       bool              theIsEmbeddedMode,
                       SMESHDS_Document* theDocument):
  _groupId( 0 ), _nbSubShapes( 0 )
{
  MESSAGE("SMESH_Mesh::SMESH_Mesh(int localId)");
  _id            = theLocalId;
  _studyId       = theStudyId;
  _gen           = theGen;
  _myDocument    = theDocument;
  _myMeshDS      = theDocument->NewMesh(theIsEmbeddedMode,theLocalId);
  _isShapeToMesh = false;
  _isAutoColor   = false;
  _isModified    = false;
  _shapeDiagonal = 0.0;
  _callUp        = NULL;
  _myMeshDS->ShapeToMesh( PseudoShape() );
  _subMeshHolder = new SubMeshHolder;
}

//================================================================================
/*!
 * \brief Constructor of SMESH_Mesh being a base of some descendant class
 */
//================================================================================

SMESH_Mesh::SMESH_Mesh():
  _id(-1),
  _studyId(-1),
  _groupId( 0 ),
  _nbSubShapes( 0 ),
  _isShapeToMesh( false ),
  _myDocument( 0 ),
  _myMeshDS( 0 ),
  _gen( 0 ),
  _isAutoColor( false ),
  _isModified( false ),
  _shapeDiagonal( 0.0 ),
  _callUp( 0 )
{
  _subMeshHolder = new SubMeshHolder;
}

namespace
{
#ifndef WIN32
  void deleteMeshDS(SMESHDS_Mesh* meshDS)
  {
    //cout << "deleteMeshDS( " << meshDS << endl;
    delete meshDS;
  }
#else
  static void* deleteMeshDS(void* meshDS)
  {
    //cout << "deleteMeshDS( " << meshDS << endl;
    SMESHDS_Mesh* m = (SMESHDS_Mesh*)meshDS;
    if(m) {
      delete m;
    }
    return 0;
  }
#endif
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESH_Mesh::~SMESH_Mesh()
{
  MESSAGE("SMESH_Mesh::~SMESH_Mesh");

  // avoid usual removal of elements while processing RemoveHypothesis( algo ) event
  SMESHDS_SubMeshIteratorPtr smIt = _myMeshDS->SubMeshes();
  while ( smIt->more() )
    const_cast<SMESHDS_SubMesh*>( smIt->next() )->Clear();

  // issue 0020340: EDF 1022 SMESH : Crash with FindNodeClosestTo in a second new study
  //   Notify event listeners at least that something happens
  if ( SMESH_subMesh * sm = GetSubMeshContaining(1))
    sm->ComputeStateEngine( SMESH_subMesh::MESH_ENTITY_REMOVED );

  // delete groups
  map < int, SMESH_Group * >::iterator itg;
  for (itg = _mapGroup.begin(); itg != _mapGroup.end(); itg++) {
    SMESH_Group *aGroup = (*itg).second;
    delete aGroup;
  }
  _mapGroup.clear();

  // delete sub-meshes
  delete _subMeshHolder;

  if ( _callUp) delete _callUp;
  _callUp = 0;

  // remove self from studyContext
  if ( _gen )
  {
    StudyContextStruct * studyContext = _gen->GetStudyContext( _studyId );
    studyContext->mapMesh.erase( _id );
  }
  if ( _myDocument )
    _myDocument->RemoveMesh( _id );
  _myDocument = 0;

  if ( _myMeshDS ) {
    // delete _myMeshDS, in a thread in order not to block closing a study with large meshes
#ifndef WIN32
    boost::thread aThread(boost::bind( & deleteMeshDS, _myMeshDS ));
#else
    boost::thread aThread(boost::bind( & deleteMeshDS, _myMeshDS ));
//    pthread_t thread;
//    int result=pthread_create(&thread, NULL, deleteMeshDS, (void*)_myMeshDS);
#endif
  }
}

//================================================================================
/*!
 * \brief Return true if a mesh with given id exists
 */
//================================================================================

bool SMESH_Mesh::MeshExists( int meshId ) const
{
  return _myDocument ? bool( _myDocument->GetMesh( meshId )) : false;
}

//================================================================================
/*!
 * \brief Return a mesh by id
 */
//================================================================================

SMESH_Mesh* SMESH_Mesh::FindMesh( int meshId ) const
{
  if ( _id == meshId )
    return (SMESH_Mesh*) this;

  if ( StudyContextStruct *aStudyContext = _gen->GetStudyContext( _studyId ))
  {
    std::map < int, SMESH_Mesh * >::iterator i_m = aStudyContext->mapMesh.find( meshId );
    if ( i_m != aStudyContext->mapMesh.end() )
      return i_m->second;
  }
  return NULL;
}

//=============================================================================
/*!
 * \brief Set geometry to be meshed
 */
//=============================================================================

void SMESH_Mesh::ShapeToMesh(const TopoDS_Shape & aShape)
{
  if(MYDEBUG) MESSAGE("SMESH_Mesh::ShapeToMesh");

  if ( !aShape.IsNull() && _isShapeToMesh ) {
    if ( aShape.ShapeType() != TopAbs_COMPOUND && // group contents is allowed to change
         _myMeshDS->ShapeToMesh().ShapeType() != TopAbs_COMPOUND )
      throw SALOME_Exception(LOCALIZED ("a shape to mesh has already been defined"));
  }
  // clear current data
  if ( !_myMeshDS->ShapeToMesh().IsNull() )
  {
    // removal of a shape to mesh, delete objects referring to sub-shapes:
    // - sub-meshes
    _subMeshHolder->DeleteAll();
    //  - groups on geometry
    map <int, SMESH_Group *>::iterator i_gr = _mapGroup.begin();
    while ( i_gr != _mapGroup.end() ) {
      if ( dynamic_cast<SMESHDS_GroupOnGeom*>( i_gr->second->GetGroupDS() )) {
        _myMeshDS->RemoveGroup( i_gr->second->GetGroupDS() );
        delete i_gr->second;
        _mapGroup.erase( i_gr++ );
      }
      else
        i_gr++;
    }
    _mapAncestors.Clear();

    // clear SMESHDS
    TopoDS_Shape aNullShape;
    _myMeshDS->ShapeToMesh( aNullShape );

    _shapeDiagonal = 0.0;
  }

  // set a new geometry
  if ( !aShape.IsNull() )
  {
    _myMeshDS->ShapeToMesh(aShape);
    _isShapeToMesh = true;
    _nbSubShapes = _myMeshDS->MaxShapeIndex();

    // fill map of ancestors
    fillAncestorsMap(aShape);
  }
  else
  {
    _isShapeToMesh = false;
    _shapeDiagonal = 0.0;
    _myMeshDS->ShapeToMesh( PseudoShape() );
  }
  _isModified = false;
}

//=======================================================================
/*!
 * \brief Return geometry to be meshed. (It may be a PseudoShape()!)
 */
//=======================================================================

TopoDS_Shape SMESH_Mesh::GetShapeToMesh() const
{
  return _myMeshDS->ShapeToMesh();
}

//=======================================================================
/*!
 * \brief Return a solid which is returned by GetShapeToMesh() if
 *        a real geometry to be meshed was not set
 */
//=======================================================================

const TopoDS_Solid& SMESH_Mesh::PseudoShape()
{
  static TopoDS_Solid aSolid;
  if ( aSolid.IsNull() )
  {
    aSolid = BRepPrimAPI_MakeBox(1,1,1);
  }
  return aSolid;
}

//=======================================================================
/*!
 * \brief Return diagonal size of bounding box of a shape
 */
//=======================================================================

double SMESH_Mesh::GetShapeDiagonalSize(const TopoDS_Shape & aShape)
{
  if ( !aShape.IsNull() ) {
    Bnd_Box Box;
    // avoid too long waiting on large shapes. PreciseBoundingBox() was added
    // to assure same result which else depends on presence of triangulation (IPAL52557).
    const int maxNbFaces = 4000;
    int nbFaces = 0;
    for ( TopExp_Explorer f( aShape, TopAbs_FACE ); f.More() && nbFaces < maxNbFaces; f.Next() )
      ++nbFaces;
    if ( nbFaces < maxNbFaces )
      GEOMUtils::PreciseBoundingBox(aShape, Box);
    else
      BRepBndLib::Add( aShape, Box);
    if ( !Box.IsVoid() )
      return sqrt( Box.SquareExtent() );
  }
  return 0;
}

//=======================================================================
/*!
 * \brief Return diagonal size of bounding box of shape to mesh
 */
//=======================================================================

double SMESH_Mesh::GetShapeDiagonalSize() const
{
  if ( _shapeDiagonal == 0. && _isShapeToMesh )
    const_cast<SMESH_Mesh*>(this)->_shapeDiagonal = GetShapeDiagonalSize( GetShapeToMesh() );

  return _shapeDiagonal;
}

//================================================================================
/*!
 * \brief Load mesh from study file
 */
//================================================================================

void SMESH_Mesh::Load()
{
  if (_callUp)
    _callUp->Load();
}

//=======================================================================
/*!
 * \brief Remove all nodes and elements
 */
//=======================================================================

void SMESH_Mesh::Clear()
{
  if ( HasShapeToMesh() ) // remove all nodes and elements
  {
    // clear mesh data
    _myMeshDS->ClearMesh();

    // update compute state of submeshes
    if ( SMESH_subMesh *sm = GetSubMeshContaining( GetShapeToMesh() ) )
    {
      sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
      sm->ComputeSubMeshStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
      sm->ComputeStateEngine( SMESH_subMesh::CLEAN ); // for event listeners (issue 0020918)
      sm->ComputeSubMeshStateEngine( SMESH_subMesh::CLEAN );
    }
  }
  else // remove only nodes/elements computed by algorithms
  {
    if ( SMESH_subMesh *sm = GetSubMeshContaining( GetShapeToMesh() ) )
    {
      sm->ComputeStateEngine( SMESH_subMesh::CLEAN );
      sm->ComputeSubMeshStateEngine( SMESH_subMesh::CLEAN );
      sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
      sm->ComputeSubMeshStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
    }
  }
  _isModified = false;
}

//=======================================================================
/*!
 * \brief Remove all nodes and elements of indicated shape
 */
//=======================================================================

void SMESH_Mesh::ClearSubMesh(const int theShapeId)
{
  // clear sub-meshes; get ready to re-compute as a side-effect 
  if ( SMESH_subMesh *sm = GetSubMeshContaining( theShapeId ) )
  {
    SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(/*includeSelf=*/true,
                                                             /*complexShapeFirst=*/false);
    while ( smIt->more() )
    {
      sm = smIt->next();
      TopAbs_ShapeEnum shapeType = sm->GetSubShape().ShapeType();      
      if ( shapeType == TopAbs_VERTEX || shapeType < TopAbs_SOLID )
        // all other shapes depends on vertices so they are already cleaned
        sm->ComputeStateEngine( SMESH_subMesh::CLEAN );
      // to recompute even if failed
      sm->ComputeStateEngine( SMESH_subMesh::CHECK_COMPUTE_STATE );
    }
  }
}

//=======================================================================
//function : UNVToMesh
//purpose  : 
//=======================================================================

int SMESH_Mesh::UNVToMesh(const char* theFileName)
{
  if(MYDEBUG) MESSAGE("UNVToMesh - theFileName = "<<theFileName);
  if(_isShapeToMesh)
    throw SALOME_Exception(LOCALIZED("a shape to mesh has already been defined"));
  _isShapeToMesh = false;
  DriverUNV_R_SMDS_Mesh myReader;
  myReader.SetMesh(_myMeshDS);
  myReader.SetFile(theFileName);
  myReader.SetMeshId(-1);
  myReader.Perform();
  if(MYDEBUG){
    MESSAGE("UNVToMesh - _myMeshDS->NbNodes() = "<<_myMeshDS->NbNodes());
    MESSAGE("UNVToMesh - _myMeshDS->NbEdges() = "<<_myMeshDS->NbEdges());
    MESSAGE("UNVToMesh - _myMeshDS->NbFaces() = "<<_myMeshDS->NbFaces());
    MESSAGE("UNVToMesh - _myMeshDS->NbVolumes() = "<<_myMeshDS->NbVolumes());
  }
  SMDS_MeshGroup* aGroup = (SMDS_MeshGroup*) myReader.GetGroup();
  if (aGroup != 0) {
    TGroupNamesMap aGroupNames = myReader.GetGroupNamesMap();
    //const TGroupIdMap& aGroupId = myReader.GetGroupIdMap();
    aGroup->InitSubGroupsIterator();
    while (aGroup->MoreSubGroups()) {
      SMDS_MeshGroup* aSubGroup = (SMDS_MeshGroup*) aGroup->NextSubGroup();
      string aName = aGroupNames[aSubGroup];
      int aId;

      SMESH_Group* aSMESHGroup = AddGroup( aSubGroup->GetType(), aName.c_str(), aId );
      if ( aSMESHGroup ) {
        if(MYDEBUG) MESSAGE("UNVToMesh - group added: "<<aName);      
        SMESHDS_Group* aGroupDS = dynamic_cast<SMESHDS_Group*>( aSMESHGroup->GetGroupDS() );
        if ( aGroupDS ) {
          aGroupDS->SetStoreName(aName.c_str());
          aSubGroup->InitIterator();
          const SMDS_MeshElement* aElement = 0;
          while (aSubGroup->More()) {
            aElement = aSubGroup->Next();
            if (aElement) {
              aGroupDS->SMDSGroup().Add(aElement);
            }
          }
          if (aElement)
            aGroupDS->SetType(aElement->GetType());
        }
      }
    }
  }
  return 1;
}

//=======================================================================
//function : MEDToMesh
//purpose  : 
//=======================================================================

int SMESH_Mesh::MEDToMesh(const char* theFileName, const char* theMeshName)
{
  if(MYDEBUG) MESSAGE("MEDToMesh - theFileName = "<<theFileName<<", mesh name = "<<theMeshName);
  if(_isShapeToMesh)
    throw SALOME_Exception(LOCALIZED("a shape to mesh has already been defined"));
  _isShapeToMesh = false;
  DriverMED_R_SMESHDS_Mesh myReader;
  myReader.SetMesh(_myMeshDS);
  myReader.SetMeshId(-1);
  myReader.SetFile(theFileName);
  myReader.SetMeshName(theMeshName);
  Driver_Mesh::Status status = myReader.Perform();
  if(MYDEBUG){
    MESSAGE("MEDToMesh - _myMeshDS->NbNodes() = "<<_myMeshDS->NbNodes());
    MESSAGE("MEDToMesh - _myMeshDS->NbEdges() = "<<_myMeshDS->NbEdges());
    MESSAGE("MEDToMesh - _myMeshDS->NbFaces() = "<<_myMeshDS->NbFaces());
    MESSAGE("MEDToMesh - _myMeshDS->NbVolumes() = "<<_myMeshDS->NbVolumes());
  }

  // Reading groups (sub-meshes are out of scope of MED import functionality)
  list<TNameAndType> aGroupNames = myReader.GetGroupNamesAndTypes();
  if(MYDEBUG) MESSAGE("MEDToMesh - Nb groups = "<<aGroupNames.size()); 
  int anId;
  list<TNameAndType>::iterator name_type = aGroupNames.begin();
  for ( ; name_type != aGroupNames.end(); name_type++ ) {
    SMESH_Group* aGroup = AddGroup( name_type->second, name_type->first.c_str(), anId );
    if ( aGroup ) {
      if(MYDEBUG) MESSAGE("MEDToMesh - group added: "<<name_type->first.c_str());      
      SMESHDS_Group* aGroupDS = dynamic_cast<SMESHDS_Group*>( aGroup->GetGroupDS() );
      if ( aGroupDS ) {
        aGroupDS->SetStoreName( name_type->first.c_str() );
        myReader.GetGroup( aGroupDS );
      }
    }
  }
  return (int) status;
}

//=======================================================================
//function : STLToMesh
//purpose  : 
//=======================================================================

int SMESH_Mesh::STLToMesh(const char* theFileName)
{
  if(MYDEBUG) MESSAGE("STLToMesh - theFileName = "<<theFileName);
  if(_isShapeToMesh)
    throw SALOME_Exception(LOCALIZED("a shape to mesh has already been defined"));
  _isShapeToMesh = false;
  DriverSTL_R_SMDS_Mesh myReader;
  myReader.SetMesh(_myMeshDS);
  myReader.SetFile(theFileName);
  myReader.SetMeshId(-1);
  myReader.Perform();
  if(MYDEBUG){
    MESSAGE("STLToMesh - _myMeshDS->NbNodes() = "<<_myMeshDS->NbNodes());
    MESSAGE("STLToMesh - _myMeshDS->NbEdges() = "<<_myMeshDS->NbEdges());
    MESSAGE("STLToMesh - _myMeshDS->NbFaces() = "<<_myMeshDS->NbFaces());
    MESSAGE("STLToMesh - _myMeshDS->NbVolumes() = "<<_myMeshDS->NbVolumes());
  }
  return 1;
}

//=======================================================================
//function : DATToMesh
//purpose  :
//=======================================================================

int SMESH_Mesh::DATToMesh(const char* theFileName)
{
  if(MYDEBUG) MESSAGE("DATToMesh - theFileName = "<<theFileName);
  if(_isShapeToMesh)
	throw SMESH_Exception(LOCALIZED("a shape to mesh has already been defined"));
  _isShapeToMesh = true;
  DriverDAT_R_SMDS_Mesh myReader;
  myReader.SetMesh(_myMeshDS);
  myReader.SetFile(theFileName);
  myReader.SetMeshId(-1);
  myReader.Perform();
  if(MYDEBUG){
	MESSAGE("DATToMesh - _myMeshDS->NbNodes() = "<<_myMeshDS->NbNodes());
	MESSAGE("DATToMesh - _myMeshDS->NbEdges() = "<<_myMeshDS->NbEdges());
	MESSAGE("DATToMesh - _myMeshDS->NbFaces() = "<<_myMeshDS->NbFaces());
	MESSAGE("DATToMesh - _myMeshDS->NbVolumes() = "<<_myMeshDS->NbVolumes());
  }
  return 1;
}

//================================================================================
/*!
 * \brief Reads the given mesh from the CGNS file
 *  \param theFileName - name of the file
 *  \retval int - Driver_Mesh::Status
 */
//================================================================================

int SMESH_Mesh::CGNSToMesh(const char*  theFileName,
                           const int    theMeshIndex,
                           std::string& theMeshName)
{
  int res = Driver_Mesh::DRS_FAIL;
#ifdef WITH_CGNS

  DriverCGNS_Read myReader;
  myReader.SetMesh(_myMeshDS);
  myReader.SetFile(theFileName);
  myReader.SetMeshId(theMeshIndex);
  res = myReader.Perform();
  theMeshName = myReader.GetMeshName();

  // create groups
  SynchronizeGroups();

#endif
  return res;
}

//================================================================================
/*!
 * \brief Fill its data by reading a GMF file
 */
//================================================================================

SMESH_ComputeErrorPtr SMESH_Mesh::GMFToMesh(const char* theFileName,
                                            bool        theMakeRequiredGroups)
{
  DriverGMF_Read myReader;
  myReader.SetMesh(_myMeshDS);
  myReader.SetFile(theFileName);
  myReader.SetMakeRequiredGroups( theMakeRequiredGroups );
  myReader.Perform();
  //theMeshName = myReader.GetMeshName();

  // create groups
  SynchronizeGroups();

  return myReader.GetError();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESH_Hypothesis::Hypothesis_Status
SMESH_Mesh::AddHypothesis(const TopoDS_Shape & aSubShape,
                          int                  anHypId,
                          std::string*         anError  )
{
  Unexpect aCatch(SalomeException);
  if(MYDEBUG) MESSAGE("SMESH_Mesh::AddHypothesis");

  if ( anError )
    anError->clear();

  SMESH_subMesh *subMesh = GetSubMesh(aSubShape);
  if ( !subMesh || !subMesh->GetId())
    return SMESH_Hypothesis::HYP_BAD_SUBSHAPE;

  SMESH_Hypothesis *anHyp = GetHypothesis( anHypId );
  if ( !anHyp )
    throw SALOME_Exception(LOCALIZED("hypothesis does not exist"));

  bool isGlobalHyp = IsMainShape( aSubShape );

  // NotConformAllowed can be only global
  if ( !isGlobalHyp )
  {
    // NOTE: this is not a correct way to check a name of hypothesis,
    // there should be an attribute of hypothesis saying that it can/can't
    // be global/local
    string hypName = anHyp->GetName();
    if ( hypName == "NotConformAllowed" )
    {
      if(MYDEBUG) MESSAGE( "Hypotesis <NotConformAllowed> can be only global" );
      return SMESH_Hypothesis::HYP_INCOMPATIBLE;
    }
  }

  // shape

  bool isAlgo = ( anHyp->GetType() != SMESHDS_Hypothesis::PARAM_ALGO );
  int   event = isAlgo ? SMESH_subMesh::ADD_ALGO : SMESH_subMesh::ADD_HYP;

  SMESH_Hypothesis::Hypothesis_Status ret = subMesh->AlgoStateEngine(event, anHyp);

  if ( anError && SMESH_Hypothesis::IsStatusFatal(ret) && subMesh->GetComputeError() )
    *anError = subMesh->GetComputeError()->myComment;

  // sub-shapes
  if ( !SMESH_Hypothesis::IsStatusFatal(ret) &&
       anHyp->GetDim() <= SMESH_Gen::GetShapeDim(aSubShape)) // is added on father
  {
    event = isAlgo ? SMESH_subMesh::ADD_FATHER_ALGO : SMESH_subMesh::ADD_FATHER_HYP;

    SMESH_Hypothesis::Hypothesis_Status ret2 =
      subMesh->SubMeshesAlgoStateEngine(event, anHyp, /*exitOnFatal=*/true);
    if (ret2 > ret)
    {
      ret = ret2;
      if ( SMESH_Hypothesis::IsStatusFatal( ret ))
      {
        if ( anError && subMesh->GetComputeError() )
          *anError = subMesh->GetComputeError()->myComment;
        // remove anHyp
        event = isAlgo ? SMESH_subMesh::REMOVE_ALGO : SMESH_subMesh::REMOVE_HYP;
        subMesh->AlgoStateEngine(event, anHyp);
      }
    }

    // check concurent hypotheses on ancestors
    if (ret < SMESH_Hypothesis::HYP_CONCURENT && !isGlobalHyp )
    {
      SMESH_subMeshIteratorPtr smIt = subMesh->getDependsOnIterator(false,false);
      while ( smIt->more() ) {
        SMESH_subMesh* sm = smIt->next();
        if ( sm->IsApplicableHypotesis( anHyp )) {
          ret2 = sm->CheckConcurentHypothesis( anHyp->GetType() );
          if (ret2 > ret) {
            ret = ret2;
            break;
          }
        }
      }
    }
  }
  HasModificationsToDiscard(); // to reset _isModified flag if a mesh becomes empty

  if(MYDEBUG) subMesh->DumpAlgoState(true);
  if(MYDEBUG) SCRUTE(ret);
  return ret;
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESH_Hypothesis::Hypothesis_Status
  SMESH_Mesh::RemoveHypothesis(const TopoDS_Shape & aSubShape,
                               int anHypId)
{
  Unexpect aCatch(SalomeException);
  if(MYDEBUG) MESSAGE("SMESH_Mesh::RemoveHypothesis");
  
  StudyContextStruct *sc = _gen->GetStudyContext(_studyId);
  if (sc->mapHypothesis.find(anHypId) == sc->mapHypothesis.end())
    throw SALOME_Exception(LOCALIZED("hypothesis does not exist"));
  
  SMESH_Hypothesis *anHyp = sc->mapHypothesis[anHypId];
  if(MYDEBUG) {
    SCRUTE(anHyp->GetType());
  }
  
  // shape 
  
  bool isAlgo =  !( anHyp->GetType() == SMESHDS_Hypothesis::PARAM_ALGO );
  int   event = isAlgo ? SMESH_subMesh::REMOVE_ALGO : SMESH_subMesh::REMOVE_HYP;

  SMESH_subMesh *subMesh = GetSubMesh(aSubShape);

  SMESH_Hypothesis::Hypothesis_Status ret = subMesh->AlgoStateEngine(event, anHyp);

  // there may appear concurrent hyps that were covered by the removed hyp
  if (ret < SMESH_Hypothesis::HYP_CONCURENT &&
      subMesh->IsApplicableHypotesis( anHyp ) &&
      subMesh->CheckConcurentHypothesis( anHyp->GetType() ) != SMESH_Hypothesis::HYP_OK)
    ret = SMESH_Hypothesis::HYP_CONCURENT;

  // sub-shapes
  if (!SMESH_Hypothesis::IsStatusFatal(ret) &&
      anHyp->GetDim() <= SMESH_Gen::GetShapeDim(aSubShape)) // is removed from father
  {
    event = isAlgo ? SMESH_subMesh::REMOVE_FATHER_ALGO : SMESH_subMesh::REMOVE_FATHER_HYP;

    SMESH_Hypothesis::Hypothesis_Status ret2 =
      subMesh->SubMeshesAlgoStateEngine(event, anHyp);
    if (ret2 > ret) // more severe
      ret = ret2;

    // check concurent hypotheses on ancestors
    if (ret < SMESH_Hypothesis::HYP_CONCURENT && !IsMainShape( aSubShape ) )
    {
      SMESH_subMeshIteratorPtr smIt = subMesh->getDependsOnIterator(false,false);
      while ( smIt->more() ) {
        SMESH_subMesh* sm = smIt->next();
        if ( sm->IsApplicableHypotesis( anHyp )) {
          ret2 = sm->CheckConcurentHypothesis( anHyp->GetType() );
          if (ret2 > ret) {
            ret = ret2;
            break;
          }
        }
      }
    }
  }

  HasModificationsToDiscard(); // to reset _isModified flag if mesh become empty

  if(MYDEBUG) subMesh->DumpAlgoState(true);
  if(MYDEBUG) SCRUTE(ret);
  return ret;
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

const list<const SMESHDS_Hypothesis*>&
SMESH_Mesh::GetHypothesisList(const TopoDS_Shape & aSubShape) const
{
  return _myMeshDS->GetHypothesis(aSubShape);
}

//=======================================================================
/*!
 * \brief Return the hypothesis assigned to the shape
 *  \param aSubShape    - the shape to check
 *  \param aFilter      - the hypothesis filter
 *  \param andAncestors - flag to check hypos assigned to ancestors of the shape
 *  \param assignedTo   - to return the shape the found hypo is assigned to
 *  \retval SMESH_Hypothesis* - the first hypo passed through aFilter
 */
//=======================================================================

const SMESH_Hypothesis * SMESH_Mesh::GetHypothesis(const TopoDS_Shape &    aSubShape,
                                                   const SMESH_HypoFilter& aFilter,
                                                   const bool              andAncestors,
                                                   TopoDS_Shape*           assignedTo) const
{
  return GetHypothesis( const_cast< SMESH_Mesh* >(this)->GetSubMesh( aSubShape ),
                        aFilter, andAncestors, assignedTo );
}

//=======================================================================
/*!
 * \brief Return the hypothesis assigned to the shape of a sub-mesh
 *  \param aSubMesh     - the sub-mesh to check
 *  \param aFilter      - the hypothesis filter
 *  \param andAncestors - flag to check hypos assigned to ancestors of the shape
 *  \param assignedTo   - to return the shape the found hypo is assigned to
 *  \retval SMESH_Hypothesis* - the first hypo passed through aFilter
 */
//=======================================================================

const SMESH_Hypothesis * SMESH_Mesh::GetHypothesis(const SMESH_subMesh *   aSubMesh,
                                                   const SMESH_HypoFilter& aFilter,
                                                   const bool              andAncestors,
                                                   TopoDS_Shape*           assignedTo) const
{
  if ( !aSubMesh ) return 0;

  {
    const TopoDS_Shape & aSubShape = aSubMesh->GetSubShape();
    const list<const SMESHDS_Hypothesis*>& hypList = _myMeshDS->GetHypothesis(aSubShape);
    list<const SMESHDS_Hypothesis*>::const_iterator hyp = hypList.begin();
    for ( ; hyp != hypList.end(); hyp++ ) {
      const SMESH_Hypothesis * h = cSMESH_Hyp( *hyp );
      if ( aFilter.IsOk( h, aSubShape)) {
        if ( assignedTo ) *assignedTo = aSubShape;
        return h;
      }
    }
  }
  if ( andAncestors )
  {
    // user sorted submeshes of ancestors, according to stored submesh priority
    std::vector< SMESH_subMesh * > & ancestors =
      const_cast< std::vector< SMESH_subMesh * > & > ( aSubMesh->GetAncestors() );
    SortByMeshOrder( ancestors );

    vector<SMESH_subMesh*>::const_iterator smIt = ancestors.begin(); 
    for ( ; smIt != ancestors.end(); smIt++ )
    {
      const TopoDS_Shape& curSh = (*smIt)->GetSubShape();
      const list<const SMESHDS_Hypothesis*>& hypList = _myMeshDS->GetHypothesis(curSh);
      list<const SMESHDS_Hypothesis*>::const_iterator hyp = hypList.begin();
      for ( ; hyp != hypList.end(); hyp++ ) {
        const SMESH_Hypothesis * h = cSMESH_Hyp( *hyp );
        if (aFilter.IsOk( h, curSh )) {
          if ( assignedTo ) *assignedTo = curSh;
          return h;
        }
      }
    }
  }
  return 0;
}

//================================================================================
/*!
 * \brief Return hypotheses assigned to the shape
  * \param aSubShape - the shape to check
  * \param aFilter - the hypothesis filter
  * \param aHypList - the list of the found hypotheses
  * \param andAncestors - flag to check hypos assigned to ancestors of the shape
  * \retval int - number of unique hypos in aHypList
 */
//================================================================================

int SMESH_Mesh::GetHypotheses(const TopoDS_Shape &                aSubShape,
                              const SMESH_HypoFilter&             aFilter,
                              list <const SMESHDS_Hypothesis * >& aHypList,
                              const bool                          andAncestors,
                              list< TopoDS_Shape > *              assignedTo/*=0*/) const
{
  return GetHypotheses( const_cast< SMESH_Mesh* >(this)->GetSubMesh( aSubShape ),
                        aFilter, aHypList, andAncestors, assignedTo );
}

//================================================================================
/*!
 * \brief Return hypotheses assigned to the shape of a sub-mesh
  * \param aSubShape - the sub-mesh to check
  * \param aFilter - the hypothesis filter
  * \param aHypList - the list of the found hypotheses
  * \param andAncestors - flag to check hypos assigned to ancestors of the shape
  * \retval int - number of unique hypos in aHypList
 */
//================================================================================

int SMESH_Mesh::GetHypotheses(const SMESH_subMesh *               aSubMesh,
                              const SMESH_HypoFilter&             aFilter,
                              list <const SMESHDS_Hypothesis * >& aHypList,
                              const bool                          andAncestors,
                              list< TopoDS_Shape > *              assignedTo/*=0*/) const
{
  if ( !aSubMesh ) return 0;

  set<string> hypTypes; // to exclude same type hypos from the result list
  int nbHyps = 0;

  // only one main hypothesis is allowed
  bool mainHypFound = false;

  // fill in hypTypes
  list<const SMESHDS_Hypothesis*>::const_iterator hyp;
  for ( hyp = aHypList.begin(); hyp != aHypList.end(); hyp++ ) {
    if ( hypTypes.insert( (*hyp)->GetName() ).second )
      nbHyps++;
    if ( !cSMESH_Hyp(*hyp)->IsAuxiliary() )
      mainHypFound = true;
  }

  // get hypos from aSubShape
  {
    const TopoDS_Shape & aSubShape = aSubMesh->GetSubShape();
    const list<const SMESHDS_Hypothesis*>& hypList = _myMeshDS->GetHypothesis(aSubShape);
    for ( hyp = hypList.begin(); hyp != hypList.end(); hyp++ )
    {
      const SMESH_Hypothesis* h = cSMESH_Hyp( *hyp );
      if (( aFilter.IsOk( h, aSubShape )) &&
          ( h->IsAuxiliary() || !mainHypFound ) &&
          ( h->IsAuxiliary() || hypTypes.insert( h->GetName() ).second ))
      {
        aHypList.push_back( *hyp );
        nbHyps++;
        if ( !h->IsAuxiliary() )
          mainHypFound = true;
        if ( assignedTo ) assignedTo->push_back( aSubShape );
      }
    }
  }

  // get hypos from ancestors of aSubShape
  if ( andAncestors )
  {
    // user sorted submeshes of ancestors, according to stored submesh priority
    std::vector< SMESH_subMesh * > & ancestors =
      const_cast< std::vector< SMESH_subMesh * > & > ( aSubMesh->GetAncestors() );
    SortByMeshOrder( ancestors );

    vector<SMESH_subMesh*>::const_iterator smIt = ancestors.begin();
    for ( ; smIt != ancestors.end(); smIt++ )
    {
      const TopoDS_Shape& curSh = (*smIt)->GetSubShape();
      const list<const SMESHDS_Hypothesis*>& hypList = _myMeshDS->GetHypothesis(curSh);
      for ( hyp = hypList.begin(); hyp != hypList.end(); hyp++ )
      {
        const SMESH_Hypothesis* h = cSMESH_Hyp( *hyp );
        if (( aFilter.IsOk( h, curSh )) &&
            ( h->IsAuxiliary() || !mainHypFound ) &&
            ( h->IsAuxiliary() || hypTypes.insert( h->GetName() ).second ))
        {
          aHypList.push_back( *hyp );
          nbHyps++;
          if ( !h->IsAuxiliary() )
            mainHypFound = true;
          if ( assignedTo ) assignedTo->push_back( curSh );
        }
      }
    }
  }
  return nbHyps;
}

//================================================================================
/*!
 * \brief Return a hypothesis by its ID
 */
//================================================================================

SMESH_Hypothesis * SMESH_Mesh::GetHypothesis(const int anHypId) const
{
  StudyContextStruct *sc = _gen->GetStudyContext(_studyId);
  if (sc->mapHypothesis.find(anHypId) == sc->mapHypothesis.end())
    return NULL;

  SMESH_Hypothesis *anHyp = sc->mapHypothesis[anHypId];
  return anHyp;
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

const list<SMESHDS_Command*> & SMESH_Mesh::GetLog()
{
  Unexpect aCatch(SalomeException);
  if(MYDEBUG) MESSAGE("SMESH_Mesh::GetLog");
  return _myMeshDS->GetScript()->GetCommands();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================
void SMESH_Mesh::ClearLog()
{
  Unexpect aCatch(SalomeException);
  if(MYDEBUG) MESSAGE("SMESH_Mesh::ClearLog");
  _myMeshDS->GetScript()->Clear();
}

//=============================================================================
/*!
 * Get or Create the SMESH_subMesh object implementation
 */
//=============================================================================

SMESH_subMesh *SMESH_Mesh::GetSubMesh(const TopoDS_Shape & aSubShape)
{
  int index = _myMeshDS->ShapeToIndex(aSubShape);
  if ( !index && aSubShape.IsNull() )
    return 0;

  // for submeshes on GEOM Group
  if (( !index || index > _nbSubShapes ) && aSubShape.ShapeType() == TopAbs_COMPOUND ) {
    TopoDS_Iterator it( aSubShape );
    if ( it.More() )
    {
      index = _myMeshDS->AddCompoundSubmesh( aSubShape, it.Value().ShapeType() );
      // fill map of Ancestors
      while ( _nbSubShapes < index )
        fillAncestorsMap( _myMeshDS->IndexToShape( ++_nbSubShapes ));
    }
  }
  // if ( !index )
  //   return NULL; // neither sub-shape nor a group

  SMESH_subMesh* aSubMesh = _subMeshHolder->Get( index );
  if ( !aSubMesh )
  {
    aSubMesh = new SMESH_subMesh(index, this, _myMeshDS, aSubShape);
    _subMeshHolder->Add( index, aSubMesh );

    // include non-computable sub-meshes in SMESH_subMesh::_ancestors of sub-submeshes
    switch ( aSubShape.ShapeType() ) {
    case TopAbs_COMPOUND:
    case TopAbs_WIRE:
    case TopAbs_SHELL:
      for ( TopoDS_Iterator subIt( aSubShape ); subIt.More(); subIt.Next() )
      {
        SMESH_subMesh* sm = GetSubMesh( subIt.Value() );
        SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(/*inclideSelf=*/true);
        while ( smIt->more() )
          smIt->next()->ClearAncestors();
      }
    default:;
    }
  }
  return aSubMesh;
}

//=============================================================================
/*!
 * Get the SMESH_subMesh object implementation. Dont create it, return null
 * if it does not exist.
 */
//=============================================================================

SMESH_subMesh *SMESH_Mesh::GetSubMeshContaining(const TopoDS_Shape & aSubShape) const
{
  int index = _myMeshDS->ShapeToIndex(aSubShape);
  return GetSubMeshContaining( index );
}

//=============================================================================
/*!
 * Get the SMESH_subMesh object implementation. Dont create it, return null
 * if it does not exist.
 */
//=============================================================================

SMESH_subMesh *SMESH_Mesh::GetSubMeshContaining(const int aShapeID) const
{
  SMESH_subMesh *aSubMesh = _subMeshHolder->Get( aShapeID );

  return aSubMesh;
}

//================================================================================
/*!
 * \brief Return sub-meshes of groups containing the given sub-shape
 */
//================================================================================

list<SMESH_subMesh*>
SMESH_Mesh::GetGroupSubMeshesContaining(const TopoDS_Shape & aSubShape) const
{
  list<SMESH_subMesh*> found;

  SMESH_subMesh * subMesh = GetSubMeshContaining(aSubShape);
  if ( !subMesh )
    return found;

  // sub-meshes of groups have max IDs, so search from the map end
  SMESH_subMeshIteratorPtr smIt( _subMeshHolder->GetIterator( /*reverse=*/true ) );
  while ( smIt->more() ) {
    SMESH_subMesh*    sm = smIt->next();
    SMESHDS_SubMesh * ds = sm->GetSubMeshDS();
    if ( ds && ds->IsComplexSubmesh() ) {
      if ( SMESH_MesherHelper::IsSubShape( aSubShape, sm->GetSubShape() ))
      {
        found.push_back( sm );
        //break;
      }
    } else {
      break; // the rest sub-meshes are not those of groups
    }
  }

  if ( found.empty() ) // maybe the main shape is a COMPOUND (issue 0021530)
  {
    if ( SMESH_subMesh * mainSM = GetSubMeshContaining(1) )
      if ( mainSM->GetSubShape().ShapeType() == TopAbs_COMPOUND )
      {
        TopoDS_Iterator it( mainSM->GetSubShape() );
        if ( it.Value().ShapeType() == aSubShape.ShapeType() &&
             SMESH_MesherHelper::IsSubShape( aSubShape, mainSM->GetSubShape() ))
          found.push_back( mainSM );
      }
  }
  else // issue 0023068
  {
    if ( SMESH_subMesh * mainSM = GetSubMeshContaining(1) )
      if ( mainSM->GetSubShape().ShapeType() == TopAbs_COMPOUND )
        found.push_back( mainSM );
  }
  return found;
}
//=======================================================================
//function : IsUsedHypothesis
//purpose  : Return True if anHyp is used to mesh aSubShape
//=======================================================================

bool SMESH_Mesh::IsUsedHypothesis(SMESHDS_Hypothesis * anHyp,
                                  const SMESH_subMesh* aSubMesh)
{
  SMESH_Hypothesis* hyp = static_cast<SMESH_Hypothesis*>(anHyp);

  // check if anHyp can be used to mesh aSubMesh
  if ( !aSubMesh || !aSubMesh->IsApplicableHypotesis( hyp ))
    return false;

  SMESH_Algo *algo = aSubMesh->GetAlgo();

  // algorithm
  if (anHyp->GetType() > SMESHDS_Hypothesis::PARAM_ALGO)
    return ( anHyp == algo );

  // algorithm parameter
  if (algo)
  {
    // look trough hypotheses used by algo
    const SMESH_HypoFilter* hypoKind;
    if (( hypoKind = algo->GetCompatibleHypoFilter( !hyp->IsAuxiliary() ))) {
      list <const SMESHDS_Hypothesis * > usedHyps;
      if ( GetHypotheses( aSubMesh, *hypoKind, usedHyps, true ))
        return ( find( usedHyps.begin(), usedHyps.end(), anHyp ) != usedHyps.end() );
    }
  }

  return false;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

// const list < SMESH_subMesh * >&
// SMESH_Mesh::GetSubMeshUsingHypothesis(SMESHDS_Hypothesis * anHyp)
//   throw(SALOME_Exception)
// {
//   _subMeshesUsingHypothesisList.clear();
//   SMESH_subMeshIteratorPtr smIt( _subMeshHolder->GetIterator() );
//   while ( smIt->more() )
//   {
//     SMESH_subMesh* aSubMesh = smIt->next();
//     if ( IsUsedHypothesis ( anHyp, aSubMesh ))
//       _subMeshesUsingHypothesisList.push_back( aSubMesh );
//   }
//   return _subMeshesUsingHypothesisList;
// }

//=======================================================================
//function : NotifySubMeshesHypothesisModification
//purpose  : Say all submeshes using theChangedHyp that it has been modified
//=======================================================================

void SMESH_Mesh::NotifySubMeshesHypothesisModification(const SMESH_Hypothesis* hyp)
{
  Unexpect aCatch(SalomeException);

  if ( !GetMeshDS()->IsUsedHypothesis( hyp ))
    return;

  if (_callUp)
    _callUp->HypothesisModified();

  SMESH_Algo *algo;
  const SMESH_HypoFilter* compatibleHypoKind;
  list <const SMESHDS_Hypothesis * > usedHyps;

  // keep sub-meshes not to miss ones whose state can change due to notifying others
  vector< SMESH_subMesh* > smToNotify;

  SMESH_subMeshIteratorPtr smIt( _subMeshHolder->GetIterator() );
  while ( smIt->more() )
  {
    SMESH_subMesh* aSubMesh = smIt->next();

    // if aSubMesh meshing depends on hyp,
    // we call aSubMesh->AlgoStateEngine( MODIF_HYP, hyp ) that causes either
    // 1) clearing of already computed aSubMesh or
    // 2) changing algo_state from MISSING_HYP to HYP_OK when parameters of hyp becomes valid,
    // other possible changes are not interesting. (IPAL0052457 - assigning hyp performance pb)
    if ( aSubMesh->GetComputeState() != SMESH_subMesh::COMPUTE_OK &&
         aSubMesh->GetComputeState() != SMESH_subMesh::FAILED_TO_COMPUTE &&
         aSubMesh->GetAlgoState()    != SMESH_subMesh::MISSING_HYP &&
         !hyp->DataDependOnParams() )
      continue;

    const TopoDS_Shape & aSubShape = aSubMesh->GetSubShape();

    if (( aSubMesh->IsApplicableHypotesis( hyp )) &&
        ( algo = aSubMesh->GetAlgo() )            &&
        ( compatibleHypoKind = algo->GetCompatibleHypoFilter( !hyp->IsAuxiliary() )) &&
        ( compatibleHypoKind->IsOk( hyp, aSubShape )))
    {
      // check if hyp is used by algo
      usedHyps.clear();
      if ( GetHypotheses( aSubMesh, *compatibleHypoKind, usedHyps, true ) &&
           find( usedHyps.begin(), usedHyps.end(), hyp ) != usedHyps.end() )
      {
        smToNotify.push_back( aSubMesh );
      }
    }
  }

  for ( size_t i = 0; i < smToNotify.size(); ++i )
  {
    smToNotify[i]->AlgoStateEngine(SMESH_subMesh::MODIF_HYP,
                                   const_cast< SMESH_Hypothesis*>( hyp ));
  }

  HasModificationsToDiscard(); // to reset _isModified flag if mesh becomes empty
  GetMeshDS()->Modified();
}

//=============================================================================
/*!
 *  Auto color functionality
 */
//=============================================================================
void SMESH_Mesh::SetAutoColor(bool theAutoColor)
{
  Unexpect aCatch(SalomeException);
  _isAutoColor = theAutoColor;
}

bool SMESH_Mesh::GetAutoColor()
{
  Unexpect aCatch(SalomeException);
  return _isAutoColor;
}

//=======================================================================
//function : SetIsModified
//purpose  : Set the flag meaning that the mesh has been edited "manually"
//=======================================================================

void SMESH_Mesh::SetIsModified(bool isModified)
{
  _isModified = isModified;

  if ( _isModified )
    // check if mesh becomes empty as result of modification
    HasModificationsToDiscard();
}

//=======================================================================
//function : HasModificationsToDiscard
//purpose  : Return true if the mesh has been edited since a total re-compute
//           and those modifications may prevent successful partial re-compute.
//           As a side effect reset _isModified flag if mesh is empty
//issue    : 0020693
//=======================================================================

bool SMESH_Mesh::HasModificationsToDiscard() const
{
  if ( ! _isModified )
    return false;

  // return true if the next Compute() will be partial and
  // existing but changed elements may prevent successful re-compute
  bool hasComputed = false, hasNotComputed = false;
  SMESH_subMeshIteratorPtr smIt( _subMeshHolder->GetIterator() );
  while ( smIt->more() )
  {
    const SMESH_subMesh* aSubMesh = smIt->next();
    switch ( aSubMesh->GetSubShape().ShapeType() )
    {
    case TopAbs_EDGE:
    case TopAbs_FACE:
    case TopAbs_SOLID:
      if ( aSubMesh->IsMeshComputed() )
        hasComputed = true;
      else
        hasNotComputed = true;
      if ( hasComputed && hasNotComputed)
        return true;
    }
  }
  if ( NbNodes() < 1 )
    const_cast<SMESH_Mesh*>(this)->_isModified = false;

  return false;
}

//================================================================================
/*!
 * \brief Check if any groups of the same type have equal names
 */
//================================================================================

bool SMESH_Mesh::HasDuplicatedGroupNamesMED()
{
  //set<string> aGroupNames; // Corrected for Mantis issue 0020028
  map< SMDSAbs_ElementType, set<string> > aGroupNames;
  for ( map<int, SMESH_Group*>::iterator it = _mapGroup.begin(); it != _mapGroup.end(); it++ )
  {
    SMESH_Group* aGroup = it->second;
    SMDSAbs_ElementType aType = aGroup->GetGroupDS()->GetType();
    string aGroupName = aGroup->GetName();
    aGroupName.resize(MAX_MED_GROUP_NAME_LENGTH);
    if (!aGroupNames[aType].insert(aGroupName).second)
      return true;
  }

  return false;
}

//================================================================================
/*!
 * \brief Export the mesh to a med file
 *  \param [in] file - name of the MED file
 *  \param [in] theMeshName - name of this mesh
 *  \param [in] theAutoGroups - boolean parameter for creating/not creating
 *              the groups Group_On_All_Nodes, Group_On_All_Faces, ... ;
 *              the typical use is auto_groups=false.
 *  \param [in] theVersion - defines the version of format of MED file, that will be created
 *  \param [in] meshPart - mesh data to export
 *  \param [in] theAutoDimension - if \c true, a space dimension of a MED mesh can be either
     *         - 1D if all mesh nodes lie on OX coordinate axis, or
     *         - 2D if all mesh nodes lie on XOY coordinate plane, or
     *         - 3D in the rest cases.
     *         If \a theAutoDimension is \c false, the space dimension is always 3.
 *  \return int - mesh index in the file
 */
//================================================================================

void SMESH_Mesh::ExportMED(const char *        file, 
                           const char*         theMeshName, 
                           bool                theAutoGroups,
                           int                 theVersion,
                           const SMESHDS_Mesh* meshPart,
                           bool                theAutoDimension,
                           bool                theAddODOnVertices)
{
  SMESH_TRY;

  DriverMED_W_SMESHDS_Mesh myWriter;
  myWriter.SetFile         ( file, MED::EVersion(theVersion) );
  myWriter.SetMesh         ( meshPart ? (SMESHDS_Mesh*) meshPart : _myMeshDS   );
  myWriter.SetAutoDimension( theAutoDimension );
  myWriter.AddODOnVertices ( theAddODOnVertices );
  if ( !theMeshName ) 
    myWriter.SetMeshId     ( _id         );
  else {
    myWriter.SetMeshId     ( -1          );
    myWriter.SetMeshName   ( theMeshName );
  }

  if ( theAutoGroups ) {
    myWriter.AddGroupOfNodes();
    myWriter.AddGroupOfEdges();
    myWriter.AddGroupOfFaces();
    myWriter.AddGroupOfVolumes();
  }

  // Pass groups to writer. Provide unique group names.
  //set<string> aGroupNames; // Corrected for Mantis issue 0020028
  if ( !meshPart )
  {
    map< SMDSAbs_ElementType, set<string> > aGroupNames;
    char aString [256];
    int maxNbIter = 10000; // to guarantee cycle finish
    for ( map<int, SMESH_Group*>::iterator it = _mapGroup.begin(); it != _mapGroup.end(); it++ ) {
      SMESH_Group*       aGroup   = it->second;
      SMESHDS_GroupBase* aGroupDS = aGroup->GetGroupDS();
      if ( aGroupDS ) {
        SMDSAbs_ElementType aType = aGroupDS->GetType();
        string aGroupName0 = aGroup->GetName();
        aGroupName0.resize(MAX_MED_GROUP_NAME_LENGTH);
        string aGroupName = aGroupName0;
        for (int i = 1; !aGroupNames[aType].insert(aGroupName).second && i < maxNbIter; i++) {
          sprintf(&aString[0], "GR_%d_%s", i, aGroupName0.c_str());
          aGroupName = aString;
          aGroupName.resize(MAX_MED_GROUP_NAME_LENGTH);
        }
        aGroupDS->SetStoreName( aGroupName.c_str() );
        myWriter.AddGroup( aGroupDS );
      }
    }
  }
  // Perform export
  myWriter.Perform();

  SMESH_CATCH( SMESH::throwSalomeEx );
}

//================================================================================
/*!
 * \brief Export the mesh to a SAUV file
 */
//================================================================================

void SMESH_Mesh::ExportSAUV(const char *file, 
                            const char* theMeshName, 
                            bool theAutoGroups)
{
  std::string medfilename(file);
  medfilename += ".med";
  std::string cmd;
#ifdef WIN32
  cmd = "%PYTHONBIN% ";
#else
  cmd = "python ";
#endif
  cmd += "-c \"";
  cmd += "from medutilities import my_remove ; my_remove(r'" + medfilename + "')";
  cmd += "\"";
  system(cmd.c_str());
  ExportMED(medfilename.c_str(), theMeshName, theAutoGroups, 1);
#ifdef WIN32
  cmd = "%PYTHONBIN% ";
#else
  cmd = "python ";
#endif
  cmd += "-c \"";
  cmd += "from medutilities import convert ; convert(r'" + medfilename + "', 'MED', 'GIBI', 1, r'" + file + "')";
  cmd += "\"";
  system(cmd.c_str());
#ifdef WIN32
  cmd = "%PYTHONBIN% ";
#else
  cmd = "python ";
#endif
  cmd += "-c \"";
  cmd += "from medutilities import my_remove ; my_remove(r'" + medfilename + "')";
  cmd += "\"";
  system(cmd.c_str());
}

//================================================================================
/*!
 * \brief Export the mesh to a DAT file
 */
//================================================================================

void SMESH_Mesh::ExportDAT(const char *        file,
                           const SMESHDS_Mesh* meshPart)
{
  Unexpect aCatch(SalomeException);
  DriverDAT_W_SMDS_Mesh myWriter;
  myWriter.SetFile( file );
  myWriter.SetMesh( meshPart ? (SMESHDS_Mesh*) meshPart : _myMeshDS );
  myWriter.SetMeshId(_id);
  myWriter.Perform();
}

//================================================================================
/*!
 * \brief Export the mesh to an UNV file
 */
//================================================================================

void SMESH_Mesh::ExportUNV(const char *        file,
                           const SMESHDS_Mesh* meshPart)
{
  Unexpect aCatch(SalomeException);
  DriverUNV_W_SMDS_Mesh myWriter;
  myWriter.SetFile( file );
  myWriter.SetMesh( meshPart ? (SMESHDS_Mesh*) meshPart : _myMeshDS );
  myWriter.SetMeshId(_id);
  //  myWriter.SetGroups(_mapGroup);

  if ( !meshPart )
  {
    for ( map<int, SMESH_Group*>::iterator it = _mapGroup.begin(); it != _mapGroup.end(); it++ ) {
      SMESH_Group*       aGroup   = it->second;
      SMESHDS_GroupBase* aGroupDS = aGroup->GetGroupDS();
      if ( aGroupDS ) {
        string aGroupName = aGroup->GetName();
        aGroupDS->SetStoreName( aGroupName.c_str() );
        myWriter.AddGroup( aGroupDS );
      }
    }
  }
  myWriter.Perform();
}

//================================================================================
/*!
 * \brief Export the mesh to an STL file
 */
//================================================================================

void SMESH_Mesh::ExportSTL(const char *        file,
                           const bool          isascii,
                           const SMESHDS_Mesh* meshPart)
{
  Unexpect aCatch(SalomeException);
  DriverSTL_W_SMDS_Mesh myWriter;
  myWriter.SetFile( file );
  myWriter.SetIsAscii( isascii );
  myWriter.SetMesh( meshPart ? (SMESHDS_Mesh*) meshPart : _myMeshDS);
  myWriter.SetMeshId(_id);
  myWriter.Perform();
}

//================================================================================
/*!
 * \brief Export the mesh to the CGNS file
 */
//================================================================================

void SMESH_Mesh::ExportCGNS(const char *        file,
                            const SMESHDS_Mesh* meshDS,
                            const char *        meshName)
{
  int res = Driver_Mesh::DRS_FAIL;
#ifdef WITH_CGNS
  DriverCGNS_Write myWriter;
  myWriter.SetFile( file );
  myWriter.SetMesh( const_cast<SMESHDS_Mesh*>( meshDS ));
  myWriter.SetMeshName( SMESH_Comment("Mesh_") << meshDS->GetPersistentId());
  if ( meshName && meshName[0] )
    myWriter.SetMeshName( meshName );
  res = myWriter.Perform();
#endif
  if ( res != Driver_Mesh::DRS_OK )
    throw SALOME_Exception("Export failed");
}

//================================================================================
/*!
 * \brief Export the mesh to a GMF file
 */
//================================================================================

void SMESH_Mesh::ExportGMF(const char *        file,
                           const SMESHDS_Mesh* meshDS,
                           bool                withRequiredGroups)
{
  DriverGMF_Write myWriter;
  myWriter.SetFile( file );
  myWriter.SetMesh( const_cast<SMESHDS_Mesh*>( meshDS ));
  myWriter.SetExportRequiredGroups( withRequiredGroups );

  myWriter.Perform();
}

//================================================================================
/*!
 * \brief Return a ratio of "compute cost" of computed sub-meshes to the whole
 *        "compute cost".
 */
//================================================================================

double SMESH_Mesh::GetComputeProgress() const
{
  double totalCost = 1e-100, computedCost = 0;
  const SMESH_subMesh* curSM = _gen->GetCurrentSubMesh();

  // get progress of a current algo
  TColStd_MapOfInteger currentSubIds; 
  if ( curSM )
    if ( SMESH_Algo* algo = curSM->GetAlgo() )
    {
      int algoNotDoneCost = 0, algoDoneCost = 0;
      const std::vector<SMESH_subMesh*>& smToCompute = algo->SubMeshesToCompute();
      for ( size_t i = 0; i < smToCompute.size(); ++i )
      {
        if ( smToCompute[i]->IsEmpty() )
          algoNotDoneCost += smToCompute[i]->GetComputeCost();
        else
          algoDoneCost += smToCompute[i]->GetComputeCost();
        currentSubIds.Add( smToCompute[i]->GetId() );
      }
      double rate = 0;
      try
      {
        OCC_CATCH_SIGNALS;
        rate = algo->GetProgress();
      }
      catch (...) {
#ifdef _DEBUG_
        cerr << "Exception in " << algo->GetName() << "::GetProgress()" << endl;
#endif
      }
      if ( 0. < rate && rate < 1.001 )
      {
        computedCost += rate * ( algoDoneCost + algoNotDoneCost );
      }
      else
      {
        rate = algo->GetProgressByTic();
        computedCost += algoDoneCost + rate * algoNotDoneCost;
      }
      // cout << "rate: "<<rate << " algoNotDoneCost: " << algoNotDoneCost << endl;
    }

  // get cost of already treated sub-meshes
  if ( SMESH_subMesh* mainSM = GetSubMeshContaining( 1 ))
  {
    SMESH_subMeshIteratorPtr smIt = mainSM->getDependsOnIterator(/*includeSelf=*/true);
    while ( smIt->more() )
    {
      const SMESH_subMesh* sm = smIt->next();
      const int smCost = sm->GetComputeCost();
      totalCost += smCost;
      if ( !currentSubIds.Contains( sm->GetId() ) )
      {
        if (( !sm->IsEmpty() ) ||
            ( sm->GetComputeState() == SMESH_subMesh::FAILED_TO_COMPUTE &&
              !sm->DependsOn( curSM ) ))
          computedCost += smCost;
      }
    }
  }
  // cout << "Total: " << totalCost
  //      << " computed: " << computedCost << " progress: " << computedCost / totalCost
  //      << " nbElems: " << GetMeshDS()->GetMeshInfo().NbElements() << endl;
  return computedCost / totalCost;
}

//================================================================================
/*!
 * \brief Return number of nodes in the mesh
 */
//================================================================================

int SMESH_Mesh::NbNodes() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->NbNodes();
}

//================================================================================
/*!
 * \brief  Return number of edges of given order in the mesh
 */
//================================================================================

int SMESH_Mesh::Nb0DElements() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().Nb0DElements();
}

//================================================================================
/*!
 * \brief  Return number of edges of given order in the mesh
 */
//================================================================================

int SMESH_Mesh::NbEdges(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbEdges(order);
}

//================================================================================
/*!
 * \brief Return number of faces of given order in the mesh
 */
//================================================================================

int SMESH_Mesh::NbFaces(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbFaces(order);
}

//================================================================================
/*!
 * \brief Return the number of faces in the mesh
 */
//================================================================================

int SMESH_Mesh::NbTriangles(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbTriangles(order);
}

//================================================================================
/*!
 * \brief Return number of biquadratic triangles in the mesh
 */
//================================================================================

int SMESH_Mesh::NbBiQuadTriangles() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbBiQuadTriangles();
}

//================================================================================
/*!
 * \brief Return the number nodes faces in the mesh
 */
//================================================================================

int SMESH_Mesh::NbQuadrangles(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbQuadrangles(order);
}

//================================================================================
/*!
 * \brief Return number of biquadratic quadrangles in the mesh
 */
//================================================================================

int SMESH_Mesh::NbBiQuadQuadrangles() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbBiQuadQuadrangles();
}

//================================================================================
/*!
 * \brief Return the number of polygonal faces in the mesh
 */
//================================================================================

int SMESH_Mesh::NbPolygons(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbPolygons(order);
}

//================================================================================
/*!
 * \brief Return number of volumes of given order in the mesh
 */
//================================================================================

int SMESH_Mesh::NbVolumes(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbVolumes(order);
}

//================================================================================
/*!
 * \brief  Return number of tetrahedrons of given order in the mesh
 */
//================================================================================

int SMESH_Mesh::NbTetras(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbTetras(order);
}

//================================================================================
/*!
 * \brief  Return number of hexahedrons of given order in the mesh
 */
//================================================================================

int SMESH_Mesh::NbHexas(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbHexas(order);
}

//================================================================================
/*!
 * \brief  Return number of triquadratic hexahedrons in the mesh
 */
//================================================================================

int SMESH_Mesh::NbTriQuadraticHexas() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbTriQuadHexas();
}

//================================================================================
/*!
 * \brief  Return number of pyramids of given order in the mesh
 */
//================================================================================

int SMESH_Mesh::NbPyramids(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbPyramids(order);
}

//================================================================================
/*!
 * \brief  Return number of prisms (penthahedrons) of given order in the mesh
 */
//================================================================================

int SMESH_Mesh::NbPrisms(SMDSAbs_ElementOrder order) const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbPrisms(order);
}

//================================================================================
/*!
 * \brief  Return number of hexagonal prisms in the mesh
 */
//================================================================================

int SMESH_Mesh::NbHexagonalPrisms() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbHexPrisms();
}

//================================================================================
/*!
 * \brief  Return number of polyhedrons in the mesh
 */
//================================================================================

int SMESH_Mesh::NbPolyhedrons() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbPolyhedrons();
}

//================================================================================
/*!
 * \brief  Return number of ball elements in the mesh
 */
//================================================================================

int SMESH_Mesh::NbBalls() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->GetMeshInfo().NbBalls();
}

//================================================================================
/*!
 * \brief  Return number of submeshes in the mesh
 */
//================================================================================

int SMESH_Mesh::NbSubMesh() const
{
  Unexpect aCatch(SalomeException);
  return _myMeshDS->NbSubMesh();
}

//================================================================================
/*!
 * \brief Returns number of meshes in the Study, that is supposed to be
 *        equal to SMESHDS_Document::NbMeshes()
 */
//================================================================================

int SMESH_Mesh::NbMeshes() const // nb meshes in the Study
{
  return _myDocument->NbMeshes();
}

//=======================================================================
//function : IsNotConformAllowed
//purpose  : check if a hypothesis allowing notconform mesh is present
//=======================================================================

bool SMESH_Mesh::IsNotConformAllowed() const
{
  if(MYDEBUG) MESSAGE("SMESH_Mesh::IsNotConformAllowed");

  static SMESH_HypoFilter filter( SMESH_HypoFilter::HasName( "NotConformAllowed" ));
  return GetHypothesis( _myMeshDS->ShapeToMesh(), filter, false );
}

//=======================================================================
//function : IsMainShape
//purpose  : 
//=======================================================================

bool SMESH_Mesh::IsMainShape(const TopoDS_Shape& theShape) const
{
  return theShape.IsSame(_myMeshDS->ShapeToMesh() );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

SMESH_Group* SMESH_Mesh::AddGroup (const SMDSAbs_ElementType theType,
                                   const char*               theName,
                                   int&                      theId,
                                   const TopoDS_Shape&       theShape,
                                   const SMESH_PredicatePtr& thePredicate)
{
  if (_mapGroup.count(_groupId))
    return NULL;
  theId = _groupId;
  SMESH_Group* aGroup = new SMESH_Group (theId, this, theType, theName, theShape, thePredicate);
  GetMeshDS()->AddGroup( aGroup->GetGroupDS() );
  _mapGroup[_groupId++] = aGroup;
  return aGroup;
}

//================================================================================
/*!
 * \brief Creates a group based on an existing SMESHDS group. Group ID should be unique
 */
//================================================================================

SMESH_Group* SMESH_Mesh::AddGroup (SMESHDS_GroupBase* groupDS)
{
  if ( !groupDS ) 
    throw SALOME_Exception(LOCALIZED ("SMESH_Mesh::AddGroup(): NULL SMESHDS_GroupBase"));

  map <int, SMESH_Group*>::iterator i_g = _mapGroup.find( groupDS->GetID() );
  if ( i_g != _mapGroup.end() && i_g->second )
  {
    if ( i_g->second->GetGroupDS() == groupDS )
      return i_g->second;
    else
      throw SALOME_Exception(LOCALIZED ("SMESH_Mesh::AddGroup() wrong ID of SMESHDS_GroupBase"));
  }
  SMESH_Group* aGroup = new SMESH_Group (groupDS);
  _mapGroup[ groupDS->GetID() ] = aGroup;
  GetMeshDS()->AddGroup( aGroup->GetGroupDS() );

  _groupId = 1 + _mapGroup.rbegin()->first;

  return aGroup;
}


//================================================================================
/*!
 * \brief Creates SMESH_Groups for not wrapped SMESHDS_Groups
 *  \retval bool - true if new SMESH_Groups have been created
 * 
 */
//================================================================================

bool SMESH_Mesh::SynchronizeGroups()
{
  int nbGroups = _mapGroup.size();
  const set<SMESHDS_GroupBase*>& groups = _myMeshDS->GetGroups();
  set<SMESHDS_GroupBase*>::const_iterator gIt = groups.begin();
  for ( ; gIt != groups.end(); ++gIt )
  {
    SMESHDS_GroupBase* groupDS = (SMESHDS_GroupBase*) *gIt;
    _groupId = groupDS->GetID();
    if ( !_mapGroup.count( _groupId ))
      _mapGroup[_groupId] = new SMESH_Group( groupDS );
  }
  if ( !_mapGroup.empty() )
    _groupId = _mapGroup.rbegin()->first + 1;

  return nbGroups < _mapGroup.size();
}

//================================================================================
/*!
 * \brief Return iterator on all existing groups
 */
//================================================================================

SMESH_Mesh::GroupIteratorPtr SMESH_Mesh::GetGroups() const
{
  typedef map <int, SMESH_Group *> TMap;
  return GroupIteratorPtr( new SMDS_mapIterator<TMap>( _mapGroup ));
}

//=============================================================================
/*!
 * \brief Return a group by ID
 */
//=============================================================================

SMESH_Group* SMESH_Mesh::GetGroup (const int theGroupID)
{
  if (_mapGroup.find(theGroupID) == _mapGroup.end())
    return NULL;
  return _mapGroup[theGroupID];
}


//=============================================================================
/*!
 * \brief Return IDs of all groups
 */
//=============================================================================

list<int> SMESH_Mesh::GetGroupIds() const
{
  list<int> anIds;
  for ( map<int, SMESH_Group*>::const_iterator it = _mapGroup.begin(); it != _mapGroup.end(); it++ )
    anIds.push_back( it->first );
  
  return anIds;
}

//================================================================================
/*!
 * \brief Set a caller of methods at level of CORBA API implementation.
 * The set upCaller will be deleted by SMESH_Mesh
 */
//================================================================================

void SMESH_Mesh::SetCallUp( TCallUp* upCaller )
{
  if ( _callUp ) delete _callUp;
  _callUp = upCaller;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool SMESH_Mesh::RemoveGroup (const int theGroupID)
{
  if (_mapGroup.find(theGroupID) == _mapGroup.end())
    return false;
  GetMeshDS()->RemoveGroup( _mapGroup[theGroupID]->GetGroupDS() );
  delete _mapGroup[theGroupID];
  _mapGroup.erase (theGroupID);
  if (_callUp)
    _callUp->RemoveGroup( theGroupID );
  return true;
}

//=======================================================================
//function : GetAncestors
//purpose  : return list of ancestors of theSubShape in the order
//           that lower dimension shapes come first.
//=======================================================================

const TopTools_ListOfShape& SMESH_Mesh::GetAncestors(const TopoDS_Shape& theS) const
{
  if ( _mapAncestors.Contains( theS ) )
    return _mapAncestors.FindFromKey( theS );

  static TopTools_ListOfShape emptyList;
  return emptyList;
}

//=======================================================================
//function : Dump
//purpose  : dumps contents of mesh to stream [ debug purposes ]
//=======================================================================

ostream& SMESH_Mesh::Dump(ostream& save)
{
  int clause = 0;
  save << "========================== Dump contents of mesh ==========================" << endl << endl;
  save << ++clause << ") Total number of nodes:      \t" << NbNodes() << endl;
  save << ++clause << ") Total number of edges:      \t" << NbEdges() << endl;
  save << ++clause << ") Total number of faces:      \t" << NbFaces() << endl;
  save << ++clause << ") Total number of polygons:   \t" << NbPolygons() << endl;
  save << ++clause << ") Total number of volumes:    \t" << NbVolumes() << endl;
  save << ++clause << ") Total number of polyhedrons:\t" << NbPolyhedrons() << endl << endl;
  for ( int isQuadratic = 0; isQuadratic < 2; ++isQuadratic )
  {
    string orderStr = isQuadratic ? "quadratic" : "linear";
    SMDSAbs_ElementOrder order  = isQuadratic ? ORDER_QUADRATIC : ORDER_LINEAR;

    save << ++clause << ") Total number of " << orderStr << " edges:\t" << NbEdges(order) << endl;
    save << ++clause << ") Total number of " << orderStr << " faces:\t" << NbFaces(order) << endl;
    if ( NbFaces(order) > 0 ) {
      int nb3 = NbTriangles(order);
      int nb4 = NbQuadrangles(order);
      save << clause << ".1) Number of " << orderStr << " triangles:  \t" << nb3 << endl;
      save << clause << ".2) Number of " << orderStr << " quadrangles:\t" << nb4 << endl;
      if ( nb3 + nb4 !=  NbFaces(order) ) {
        map<int,int> myFaceMap;
        SMDS_FaceIteratorPtr itFaces=_myMeshDS->facesIterator();
        while( itFaces->more( ) ) {
          int nbNodes = itFaces->next()->NbNodes();
          if ( myFaceMap.find( nbNodes ) == myFaceMap.end() )
            myFaceMap[ nbNodes ] = 0;
          myFaceMap[ nbNodes ] = myFaceMap[ nbNodes ] + 1;
        }
        save << clause << ".3) Faces in detail: " << endl;
        map <int,int>::iterator itF;
        for (itF = myFaceMap.begin(); itF != myFaceMap.end(); itF++)
          save << "--> nb nodes: " << itF->first << " - nb elemens:\t" << itF->second << endl;
      }
    }
    save << ++clause << ") Total number of " << orderStr << " volumes:\t" << NbVolumes(order) << endl;
    if ( NbVolumes(order) > 0 ) {
      int nb8 = NbHexas(order);
      int nb4 = NbTetras(order);
      int nb5 = NbPyramids(order);
      int nb6 = NbPrisms(order);
      save << clause << ".1) Number of " << orderStr << " hexahedrons: \t" << nb8 << endl;
      save << clause << ".2) Number of " << orderStr << " tetrahedrons:\t" << nb4 << endl;
      save << clause << ".3) Number of " << orderStr << " prisms:      \t" << nb6 << endl;
      save << clause << ".4) Number of " << orderStr << " pyramids:    \t" << nb5 << endl;
      if ( nb8 + nb4 + nb5 + nb6 != NbVolumes(order) ) {
        map<int,int> myVolumesMap;
        SMDS_VolumeIteratorPtr itVolumes=_myMeshDS->volumesIterator();
        while( itVolumes->more( ) ) {
          int nbNodes = itVolumes->next()->NbNodes();
          if ( myVolumesMap.find( nbNodes ) == myVolumesMap.end() )
            myVolumesMap[ nbNodes ] = 0;
          myVolumesMap[ nbNodes ] = myVolumesMap[ nbNodes ] + 1;
        }
        save << clause << ".5) Volumes in detail: " << endl;
        map <int,int>::iterator itV;
        for (itV = myVolumesMap.begin(); itV != myVolumesMap.end(); itV++)
          save << "--> nb nodes: " << itV->first << " - nb elemens:\t" << itV->second << endl;
      }
    }
    save << endl;
  }
  save << "===========================================================================" << endl;
  return save;
}

//=======================================================================
//function : GetElementType
//purpose  : Returns type of mesh element with certain id
//=======================================================================

SMDSAbs_ElementType SMESH_Mesh::GetElementType( const int id, const bool iselem )
{
  return _myMeshDS->GetElementType( id, iselem );
}

//=============================================================================
/*!
 *  \brief Convert group on geometry into standalone group
 */
//=============================================================================

SMESH_Group* SMESH_Mesh::ConvertToStandalone ( int theGroupID )
{
  SMESH_Group* aGroup = 0;
  map < int, SMESH_Group * >::iterator itg = _mapGroup.find( theGroupID );
  if ( itg == _mapGroup.end() )
    return aGroup;

  SMESH_Group* anOldGrp = (*itg).second;
  if ( !anOldGrp || !anOldGrp->GetGroupDS() )
    return aGroup;
  SMESHDS_GroupBase* anOldGrpDS = anOldGrp->GetGroupDS();

  // create new standalone group
  aGroup = new SMESH_Group (theGroupID, this, anOldGrpDS->GetType(), anOldGrp->GetName() );
  _mapGroup[theGroupID] = aGroup;

  SMESHDS_Group* aNewGrpDS = dynamic_cast<SMESHDS_Group*>( aGroup->GetGroupDS() );
  GetMeshDS()->RemoveGroup( anOldGrpDS );
  GetMeshDS()->AddGroup( aNewGrpDS );

  // add elements (or nodes) into new created group
  SMDS_ElemIteratorPtr anItr = anOldGrpDS->GetElements();
  while ( anItr->more() )
    aNewGrpDS->Add( (anItr->next())->GetID() );

  // set color
  aNewGrpDS->SetColor( anOldGrpDS->GetColor() );

  // remove old group
  delete anOldGrp;

  return aGroup;
}

//=============================================================================
/*!
 *  \brief remove submesh order  from Mesh
 */
//=============================================================================

void SMESH_Mesh::ClearMeshOrder()
{
  _mySubMeshOrder.clear();
}

//=============================================================================
/*!
 *  \brief remove submesh order  from Mesh
 */
//=============================================================================

void SMESH_Mesh::SetMeshOrder(const TListOfListOfInt& theOrder )
{
  _mySubMeshOrder = theOrder;
}

//=============================================================================
/*!
 *  \brief return submesh order if any
 */
//=============================================================================

const TListOfListOfInt& SMESH_Mesh::GetMeshOrder() const
{
  return _mySubMeshOrder;
}

//=============================================================================
/*!
 *  \brief fill _mapAncestors
 */
//=============================================================================

void SMESH_Mesh::fillAncestorsMap(const TopoDS_Shape& theShape)
{
  int desType, ancType;
  if ( !theShape.IsSame( GetShapeToMesh()) && theShape.ShapeType() == TopAbs_COMPOUND )
  {
    // a geom group is added. Insert it into lists of ancestors before
    // the first ancestor more complex than group members
    TopoDS_Iterator subIt( theShape );
    if ( !subIt.More() ) return;
    int memberType = subIt.Value().ShapeType();
    for ( desType = TopAbs_VERTEX; desType >= memberType; desType-- )
      for (TopExp_Explorer des( theShape, TopAbs_ShapeEnum( desType )); des.More(); des.Next())
      {
        if ( !_mapAncestors.Contains( des.Current() )) continue;// issue 0020982
        TopTools_ListOfShape& ancList = _mapAncestors.ChangeFromKey( des.Current() );
        TopTools_ListIteratorOfListOfShape ancIt (ancList);
        while ( ancIt.More() && ancIt.Value().ShapeType() >= memberType )
          ancIt.Next();
        if ( ancIt.More() ) ancList.InsertBefore( theShape, ancIt );
        else                ancList.Append( theShape );
      }
  }
  else // else added for 52457: Addition of hypotheses is 8 time longer than meshing
  {
    for ( desType = TopAbs_VERTEX; desType > TopAbs_COMPOUND; desType-- )
      for ( ancType = desType - 1; ancType >= TopAbs_COMPOUND; ancType-- )
        TopExp::MapShapesAndAncestors ( theShape,
                                        (TopAbs_ShapeEnum) desType,
                                        (TopAbs_ShapeEnum) ancType,
                                        _mapAncestors );
  }
  // visit COMPOUNDs inside a COMPOUND that are not reachable by TopExp_Explorer
  if ( theShape.ShapeType() == TopAbs_COMPOUND )
  {
    TopoDS_Iterator sIt(theShape);
    if ( sIt.More() && sIt.Value().ShapeType() == TopAbs_COMPOUND )
      for ( ; sIt.More(); sIt.Next() )
        if ( sIt.Value().ShapeType() == TopAbs_COMPOUND )
          fillAncestorsMap( sIt.Value() );
  }
}

//=============================================================================
/*!
 * \brief sort submeshes according to stored mesh order
 * \param theListToSort in out list to be sorted
 * \return FALSE if nothing sorted
 */
//=============================================================================

 bool SMESH_Mesh::SortByMeshOrder(std::vector<SMESH_subMesh*>& theListToSort) const
{
  if ( !_mySubMeshOrder.size() || theListToSort.size() < 2)
    return true;
  
  bool res = false;
  vector<SMESH_subMesh*> onlyOrderedList;
  // collect all ordered submeshes in one list as pointers
  // and get their positions within theListToSort
  typedef vector<SMESH_subMesh*>::iterator TPosInList;
  map< int, TPosInList > sortedPos;
  TPosInList smBeg = theListToSort.begin(), smEnd = theListToSort.end();
  TListOfListOfInt::const_iterator listIdsIt = _mySubMeshOrder.begin();
  for( ; listIdsIt != _mySubMeshOrder.end(); listIdsIt++)
  {
    const TListOfInt& listOfId = *listIdsIt;
    // convert sm ids to sm's
    vector<SMESH_subMesh*> smVec;
    TListOfInt::const_iterator idIt = listOfId.begin();
    for ( ; idIt != listOfId.end(); idIt++ ) {
      if ( SMESH_subMesh * sm = GetSubMeshContaining( *idIt )) {
        if ( sm->GetSubMeshDS() && sm->GetSubMeshDS()->IsComplexSubmesh() )
        {
          SMESHDS_SubMeshIteratorPtr smdsIt = sm->GetSubMeshDS()->GetSubMeshIterator();
          while ( smdsIt->more() )
          {
            const SMESHDS_SubMesh* smDS = smdsIt->next();
            if (( sm = GetSubMeshContaining( smDS->GetID() )))
              smVec.push_back( sm );
          }
        }
        else
        {
          smVec.push_back( sm );
        }
      }
    }
    // find smVec items in theListToSort
    for ( size_t i = 0; i < smVec.size(); ++i )
    {
      TPosInList smPos = find( smBeg, smEnd, smVec[i] );
      if ( smPos != smEnd ) {
        onlyOrderedList.push_back( smVec[i] );
        sortedPos[ distance( smBeg, smPos )] = smPos;
      }
    }
  }
  if (onlyOrderedList.size() < 2)
    return res;
  res = true;

  vector<SMESH_subMesh*>::iterator onlyBIt = onlyOrderedList.begin();
  vector<SMESH_subMesh*>::iterator onlyEIt = onlyOrderedList.end();

  // iterate on ordered submeshes and insert them in detected positions
  map< int, TPosInList >::iterator i_pos = sortedPos.begin();
  for ( ; onlyBIt != onlyEIt; ++onlyBIt, ++i_pos )
    *(i_pos->second) = *onlyBIt;

  return res;
}

//================================================================================
/*!
 * \brief Return true if given order of sub-meshes is OK
 */
//================================================================================

bool SMESH_Mesh::IsOrderOK( const SMESH_subMesh* smBefore,
                            const SMESH_subMesh* smAfter ) const
{
  TListOfListOfInt::const_iterator listIdsIt = _mySubMeshOrder.begin();
  TListOfInt::const_iterator idBef, idAft;
  for( ; listIdsIt != _mySubMeshOrder.end(); listIdsIt++)
  {
    const TListOfInt& listOfId = *listIdsIt;
    idBef = std::find( listOfId.begin(), listOfId.end(), smBefore->GetId() );
    if ( idBef != listOfId.end() )
      idAft = std::find( listOfId.begin(), listOfId.end(), smAfter->GetId() );
    if ( idAft != listOfId.end () )
      return ( std::distance( listOfId.begin(), idBef ) <
               std::distance( listOfId.begin(), idAft )   );
  }
  return true; // no order imposed to given submeshes
} 

//=============================================================================
/*!
 * \brief sort submeshes according to stored mesh order
 * \param theListToSort in out list to be sorted
 * \return FALSE if nothing sorted
 */
//=============================================================================

void SMESH_Mesh::getAncestorsSubMeshes (const TopoDS_Shape&            theSubShape,
                                        std::vector< SMESH_subMesh* >& theSubMeshes) const
{
  theSubMeshes.clear();
  TopTools_ListIteratorOfListOfShape it( GetAncestors( theSubShape ));
  for (; it.More(); it.Next() )
    if ( SMESH_subMesh* sm = GetSubMeshContaining( it.Value() ))
      theSubMeshes.push_back(sm);

  // sort submeshes according to stored mesh order
  SortByMeshOrder( theSubMeshes );
}
