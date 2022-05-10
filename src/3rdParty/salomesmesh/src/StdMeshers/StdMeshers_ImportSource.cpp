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

//  SMESH StdMeshers_ImportSource1D : implementation of SMESH idl descriptions
//  File   : StdMeshers_ImportSource1D.cxx
//  Module : SMESH
//
#include "StdMeshers_ImportSource.hxx"

#include "SMESHDS_GroupOnGeom.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Group.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_subMeshEventListener.hxx"

#include "utilities.h"

#include <Standard_ErrorHandler.hxx>

#include <boost/shared_ptr.hpp>

using namespace std;

//=============================================================================
/*!
 * Creates StdMeshers_ImportSource1D
 */
//=============================================================================

StdMeshers_ImportSource1D::StdMeshers_ImportSource1D(int         hypId,
                                                     int         studyId,
                                                     SMESH_Gen * gen)
  :SMESH_Hypothesis(hypId, studyId, gen),
   _toCopyMesh(false),
   _toCopyGroups(false)
{
  _name = "ImportSource1D";
  _param_algo_dim = 1; // is used by StdMeshers_Import_1D;
}

//=============================================================================
/*!
 * Creates StdMeshers_ImportSource2D
 */
//=============================================================================

StdMeshers_ImportSource2D::StdMeshers_ImportSource2D(int         hypId,
                                                     int         studyId,
                                                     SMESH_Gen * gen)
  :StdMeshers_ImportSource1D(hypId, studyId, gen)
{
  _name = "ImportSource2D";
  _param_algo_dim = 2; // is used by StdMeshers_Import_2D;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

StdMeshers_ImportSource1D::~StdMeshers_ImportSource1D()
{
}
//=============================================================================
/*!
 *  Sets groups to import elements from
 */
//=============================================================================

void StdMeshers_ImportSource1D::SetGroups(const std::vector<SMESH_Group*>& groups)
{
  if (_groups != groups)
  {
    _groups = groups;
    NotifySubMeshesHypothesisModification();
  }
}

void StdMeshers_ImportSource1D::SetCopySourceMesh(bool toCopyMesh, bool toCopyGroups)
{
  if ( !toCopyMesh ) toCopyGroups = false;
  if ( _toCopyMesh != toCopyMesh || _toCopyGroups != toCopyGroups )
  {
    _toCopyMesh = toCopyMesh; _toCopyGroups = toCopyGroups;
    NotifySubMeshesHypothesisModification();
  }
}
void StdMeshers_ImportSource1D::GetCopySourceMesh(bool& toCopyMesh, bool& toCopyGroups) const
{
  toCopyMesh = _toCopyMesh; toCopyGroups = _toCopyGroups;
}
  
namespace
{
  //================================================================================
  /*!
   * \brief Return only alive groups
   */
  //================================================================================

  vector<SMESH_Group*> getValidGroups(const vector<SMESH_Group*>& groups,
                                      StudyContextStruct*         studyContext,
                                      bool                        loaded=false)
  {
    vector<SMESH_Group*> okGroups;
    for ( int i = 0; i < groups.size(); ++i )
    {
      try
      {
        // we expect SIGSEGV on a dead group
        OCC_CATCH_SIGNALS;
        SMESH_Group* okGroup = 0;
        map<int, SMESH_Mesh*>::iterator itm = studyContext->mapMesh.begin();
        for ( ; !okGroup && itm != studyContext->mapMesh.end(); itm++)
        {
          SMESH_Mesh::GroupIteratorPtr gIt = itm->second->GetGroups();
          while ( gIt->more() && !okGroup )
            if ( gIt->next() == groups[i] )
            {
              okGroup = groups[i];
              if ( loaded )
                itm->second->Load();
            }
        }
        if ( okGroup )
          okGroups.push_back( okGroup );
      }
      catch(...)
      {
      }
    }
    return okGroups;
  }
  //================================================================================
  /*!
   * \brief Pack meshes into a pair of ints
   */
  //================================================================================

  pair<int, int> getResMapKey(const SMESHDS_Mesh& srcMesh, const SMESHDS_Mesh& tgtMesh)
  {
    return make_pair( srcMesh.GetPersistentId() , tgtMesh.GetPersistentId() );
  }
  //================================================================================
  /*!
   * \brief Return a target mesh by a pair of ints
   */
  //================================================================================

  SMESH_Mesh* getTgtMeshByKey( const pair<int, int> & resMapKey,
                               StudyContextStruct*    studyContext)
  {
    int tgtID = resMapKey.second;
    SMESH_Mesh* tgtMesh = 0;
    map<int, SMESH_Mesh*>::iterator itm = studyContext->mapMesh.begin();
    for ( ; !tgtMesh && itm != studyContext->mapMesh.end(); itm++)
    {
      tgtMesh = (*itm).second;
      if ( tgtMesh->GetMeshDS()->GetPersistentId() != tgtID )
        tgtMesh = 0;
    }
    return tgtMesh;
  }
  //================================================================================
  /*!
   * \brief Return a target mesh by a pair of ints
   */
  //================================================================================

  int getSrcMeshID( const pair<int, int> & resMapKey )
  {
    return resMapKey.first;
  }
}

//=============================================================================
/*!
 *  Returns groups to import elements from
 *  \param [in] loaded - if \c true, meshes holding the groups are loaded
 */
//=============================================================================

const std::vector<SMESH_Group*>&  StdMeshers_ImportSource1D::GetGroups(bool loaded) const
{
  // filter off deleted groups
  vector<SMESH_Group*> okGroups = getValidGroups( _groups,
                                                  _gen->GetStudyContext(_studyId),
                                                  loaded);
  if ( okGroups.size() != _groups.size() )
    ((StdMeshers_ImportSource1D*)this)->_groups = okGroups;

  return _groups;
}

//================================================================================
/*!
 * \brief Return source meshes
 */
//================================================================================

std::vector<SMESH_Mesh*> StdMeshers_ImportSource1D::GetSourceMeshes() const
{
  // GetPersistentId()'s of meshes
  set<int> meshIDs;
  const vector<SMESH_Group*>& groups = GetGroups();
  if ( !groups.empty() )
  {
    for ( unsigned i = 0; i < groups.size(); ++i )
    {
      const SMESHDS_GroupBase* gDS = groups[i]->GetGroupDS();
      int id = gDS->GetMesh()->GetPersistentId();
      meshIDs.insert( id );
    }
  }
  else
  {
    if ( _resultGroups.empty() )
      ((StdMeshers_ImportSource1D*)this)->RestoreGroups(_groups);
    TResGroupMap::const_iterator key_groups = _resultGroups.begin();
    for ( ; key_groups != _resultGroups.end(); ++key_groups )
      meshIDs.insert( getSrcMeshID( key_groups->first ));
  }

  // Find corresponding meshes
  vector<SMESH_Mesh*> meshes;
  if ( !meshIDs.empty() )
  {
    StudyContextStruct* studyContext = _gen->GetStudyContext(_studyId);
    for ( set<int>::iterator id = meshIDs.begin(); id != meshIDs.end(); ++id )
    {
      map<int, SMESH_Mesh*>::iterator itm = studyContext->mapMesh.begin();
      for ( ; itm != studyContext->mapMesh.end(); itm++)
      {
        SMESH_Mesh* mesh = (*itm).second;
        if ( mesh->GetMeshDS()->GetPersistentId() == *id )
        {
          meshes.push_back( mesh );
          break;
        }
      }
    }
  }
  return meshes;
}

//================================================================================
/*!
 * \brief Return submeshes whose events affect the target mesh
 */
//================================================================================

std::vector<SMESH_subMesh*>
StdMeshers_ImportSource1D::GetSourceSubMeshes(const SMESH_Mesh* srcMesh) const
{
  if ( !srcMesh->HasShapeToMesh() )
  {
    SMESH_Mesh* srcM = const_cast< SMESH_Mesh* >( srcMesh );
    return vector<SMESH_subMesh*>(1, srcM->GetSubMesh( srcM->GetShapeToMesh()));
  }
  set<int> shapeIDs;
  const vector<SMESH_Group*>& groups = GetGroups();
  const SMESHDS_Mesh * srcMeshDS = srcMesh->GetMeshDS();
  for ( size_t i = 0; i < groups.size(); ++i )
  {
    SMESHDS_GroupBase * grDS = groups[i]->GetGroupDS();
    if ( grDS->GetMesh() != srcMeshDS )
      continue;
    if ( SMESHDS_GroupOnGeom* gog = dynamic_cast<SMESHDS_GroupOnGeom*>( grDS ))
    {
      shapeIDs.insert( srcMeshDS->ShapeToIndex( gog->GetShape() ));
    }
    else
    {
      SMDS_ElemIteratorPtr elIt = grDS->GetElements();
      while ( elIt->more() )
        shapeIDs.insert( elIt->next()->getshapeId() );
    }
  }
  if ( !shapeIDs.empty() && *shapeIDs.begin() < 1 )
  {
    shapeIDs.erase( shapeIDs.begin() );
    shapeIDs.insert( 1 );
  }

  vector<SMESH_subMesh*> smVec( shapeIDs.size());
  set<int>::iterator sID = shapeIDs.begin();
  for ( int i = 0; sID != shapeIDs.end(); ++sID, ++i )
    smVec[i] = srcMesh->GetSubMeshContaining( *sID );

  return smVec;
}

//=============================================================================
/*!
 * Save _toCopyMesh and _toCopyGroups to a stream
 */
//=============================================================================

ostream & StdMeshers_ImportSource1D::SaveTo(ostream & save)
{
  resultGroupsToIntVec();

  save << " " << _toCopyMesh << " " << _toCopyGroups;
  save << " " << _resultGroupsStorage.size();
  for ( unsigned i = 0; i < _resultGroupsStorage.size(); ++i )
    save << " " << _resultGroupsStorage[i];

  return save;
}

//=============================================================================
/*!
 * Load _toCopyMesh and _toCopyGroups from a stream
 */
//=============================================================================

istream & StdMeshers_ImportSource1D::LoadFrom(istream & load)
{
  load >> _toCopyMesh >> _toCopyGroups;

  _resultGroupsStorage.clear();
  int val;
  if ( load >> val )
  {
    _resultGroupsStorage.reserve(val);
    while ( _resultGroupsStorage.size() < _resultGroupsStorage.capacity() && load >> val )
      _resultGroupsStorage.push_back( val );
  }
  return load;
}

//================================================================================
/*!
 * \brief Convert result groups into _resultGroupsStorage
 */
//================================================================================

void StdMeshers_ImportSource1D::resultGroupsToIntVec()
{
  _resultGroupsStorage.clear();
  
  // store result groups
  TResGroupMap::iterator key2groups = _resultGroups.begin();
  for ( ; key2groups != _resultGroups.end(); ++key2groups )
  {
    const pair<int, int>&          key = key2groups->first;
    const vector<SMESH_Group*>& groups = key2groups->second;
    // mesh ids, nb groups
    _resultGroupsStorage.push_back( key.first );
    _resultGroupsStorage.push_back( key.second );
    _resultGroupsStorage.push_back( groups.size() );
    for ( unsigned i = 0; i < groups.size(); ++i )
    {
      // store group names as sequence of ints each standing for a char
      // of a name; that is to avoid pb with names containing white spaces
      string name = groups[i]->GetGroupDS()->GetStoreName();
      _resultGroupsStorage.push_back( name.size() );
      for ( unsigned j = 0; j < name.size(); ++j )
        _resultGroupsStorage.push_back( name[j] );
    }
  }
}

//================================================================================
/*!
 * \brief Restore source groups and result groups by _resultGroupsStorage
 */
//================================================================================

void StdMeshers_ImportSource1D::RestoreGroups(const std::vector<SMESH_Group*>& groups)
{
  _groups = groups;

  _resultGroups.clear();
  int i = 0;
  while ( i < _resultGroupsStorage.size() )
  {
    int key1 = _resultGroupsStorage[i++];
    int key2 = _resultGroupsStorage[i++];
    pair<int, int> resMapKey( key1, key2 );
    SMESH_Mesh* mesh = getTgtMeshByKey( resMapKey, _gen->GetStudyContext(_studyId));
    // restore mesh ids at least
    _resultGroups.insert( make_pair (resMapKey,vector<SMESH_Group*>() )); 

    int nbGroups = _resultGroupsStorage[i++];
    for ( int j = 0; j < nbGroups; ++j )
    {
      string::size_type nameSize = _resultGroupsStorage[i++];
      string groupName(nameSize, '\0');
      for ( unsigned k = 0; k < nameSize; ++k )
        groupName[k] = (char) _resultGroupsStorage[i++];

      // find a group by name
      if ( mesh )
      {
        SMESH_Group* group = 0;
        SMESH_Mesh::GroupIteratorPtr gIt = mesh->GetGroups();
        while ( !group && gIt->more() )
        {
          group = gIt->next();
          if ( !group->GetGroupDS() || groupName != group->GetGroupDS()->GetStoreName() )
            group = 0;
        }
        if ( group )
          _resultGroups[ resMapKey ].push_back( group );
      }
    }
  }
}

//================================================================================
/*!
 * \brief Remember groups imported from other mesh
 *  \param groups - result groups
 *  \param srcMesh - source mesh
 *  \param tgtMesh - destination mesh
 */
//================================================================================

void StdMeshers_ImportSource1D::StoreResultGroups(const std::vector<SMESH_Group*>& groups,
                                                  const SMESHDS_Mesh&              srcMesh,
                                                  const SMESHDS_Mesh&              tgtMesh)
{
  _resultGroups[ getResMapKey(srcMesh,tgtMesh) ] = groups;
}

//================================================================================
/*!
 * \brief Return groups imported from other mesh
 *  \param srcMesh - source mesh
 *  \param tgtMesh - destination mesh
 *  \retval const std::vector<SMESH_Group*>& - groups
 */
//================================================================================

std::vector<SMESH_Group*>*
StdMeshers_ImportSource1D::GetResultGroups(const SMESHDS_Mesh& srcMesh,
                                           const SMESHDS_Mesh& tgtMesh) 
{
  TResGroupMap::iterator key2groups = _resultGroups.find( getResMapKey(srcMesh,tgtMesh ));
  if ( key2groups == _resultGroups.end() )
    return 0;
  vector<SMESH_Group*> vec = getValidGroups((*key2groups).second,
                                            _gen->GetStudyContext(_studyId) );
  if ( vec.size() != key2groups->second.size())
    key2groups->second = vec;

  return & key2groups->second;
}

//================================================================================
/*!
 * \brief Initialize ImportSource value by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_ImportSource1D::SetParametersByMesh(const SMESH_Mesh*, const TopoDS_Shape&)
{
  return false;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_ImportSource1D::SetParametersByDefaults(const TDefaults&, const SMESH_Mesh* )
{
  return false;
}
