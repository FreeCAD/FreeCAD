// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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

// File      : DriverMED_W_Field.cxx
// Created   : Thu Feb 27 17:45:00 2014
// Author    : eap

#include "DriverMED_W_Field.h"

#include "DriverMED.hxx"
#include "DriverMED_W_SMESHDS_Mesh.h"
#include "MED_Factory.hxx"
#include "MED_Utilities.hxx"
#include "MED_Wrapper.hxx"
#include "SMDS_IteratorOnIterators.hxx"
#include "SMDS_MeshElement.hxx"
#include "SMDS_PolyhedralVolumeOfNodes.hxx"
#include "SMDS_SetIterator.hxx"
#include "SMESHDS_Mesh.hxx"

//================================================================================
/*!
 * \brief Constructor
 */
//================================================================================

DriverMED_W_Field::DriverMED_W_Field():
  //_medFileID( -1 ),
  _elemType( SMDSAbs_All ),
  _dt( -1 ),
  _it( -1 )
{
}

//================================================================================
/*!
 * \brief Sets basic data
 *  \param [in] mesh - supporting mesh
 *  \param [in] fieldName - name of a field
 *  \param [in] type - type of supporting elements
 *  \param [in] nbComps - number of components
 *  \param [in] isIntData - type of data: double or integer
 */
//================================================================================

bool DriverMED_W_Field::Set(SMESHDS_Mesh *      mesh,
                            const std::string & fieldName,
                            SMDSAbs_ElementType type,
                            const int           nbComps,
                            const bool          isIntData)
{
  _fieldName = fieldName;
  _compNames.resize( nbComps, "" );

  if ( type == SMDSAbs_All )
  {
    if ( mesh->NbVolumes() > 0 )
      type = SMDSAbs_Volume;
    else if ( mesh->NbFaces() > 0 )
      type = SMDSAbs_Face;
    else if ( mesh->NbEdges() > 0 )
      type = SMDSAbs_Edge;
    else
      type = SMDSAbs_Node;
  }
  if ( myMesh != mesh )
  {
    _nbElemsByGeom.clear();
    for ( int iG = 0; iG < SMDSEntity_Last; ++iG )
      _elemsByGeom[iG].clear();
    SetMesh( mesh );
  }

  // find out "MED order" of elements - sort elements by geom type
  int nbElems;
  if ( _nbElemsByGeom.empty() || _elemType != type )
  {
    _elemType = type;
    _nbElemsByGeom.resize( 1, std::make_pair( SMDSEntity_Last, 0 ));

    // count nb of elems of each geometry
    for ( int iG = 0; iG < SMDSEntity_Last; ++iG )
    {
      SMDSAbs_EntityType  geom = (SMDSAbs_EntityType) iG;
      SMDSAbs_ElementType t = SMDS_MeshCell::toSmdsType( geom );
      if ( t != _elemType ) continue;

      nbElems = mesh->GetMeshInfo().NbElements( geom );
      if ( nbElems < 1 ) continue;

      _nbElemsByGeom.push_back( std::make_pair( geom, nbElems + _nbElemsByGeom.back().second ));
    }
    // add nodes of missing 0D elements on VERTEXes
    if ( _addODOnVertices && _elemType == SMDSAbs_0DElement )
    {
      std::vector< const SMDS_MeshElement* >& nodes = _elemsByGeom[SMDSEntity_Node];
      if ( nodes.empty() )
        DriverMED_W_SMESHDS_Mesh::getNodesOfMissing0DOnVert( myMesh, nodes );
      if ( !nodes.empty() )
      {
        if ( _nbElemsByGeom.size() == 1 )
          _nbElemsByGeom.push_back( std::make_pair( SMDSEntity_0D, 0));
        _nbElemsByGeom.push_back( std::make_pair( SMDSEntity_Node,
                                             nodes.size() + _nbElemsByGeom.back().second ));
      }
    }

    // sort elements by their geometry
    int iGeoType, nbGeomTypes = _nbElemsByGeom.size() - 1;
    if ( nbGeomTypes > 1 )
    {
      for ( size_t iG = 1; iG < _nbElemsByGeom.size(); ++iG )
      {
        iGeoType = _nbElemsByGeom[iG].first;
        nbElems  = _nbElemsByGeom[iG].second - _nbElemsByGeom[iG-1].second;
        _elemsByGeom[ iGeoType ].reserve( nbElems );
      }
      iGeoType = _nbElemsByGeom[1].first; // for missing 0D
      if ( _elemsByGeom[ iGeoType ].empty() )
      {
        nbElems = mesh->GetMeshInfo().NbElements( _elemType );
        SMDS_ElemIteratorPtr eIt = mesh->elementsIterator( _elemType );
        for ( int iE = 0; iE < nbElems && eIt->more(); ++iE )
        {
          const SMDS_MeshElement* e = eIt->next();
          _elemsByGeom[ e->GetEntityType() ].push_back( e );
        }
      }
    }
  }
  _intValues.clear();
  _dblValues.clear();

  // allocate memory for values
  nbElems = _nbElemsByGeom.empty() ? 0 : _nbElemsByGeom.back().second;
  if ( isIntData )
    _intValues.reserve( nbElems * nbComps );
  else
    _dblValues.reserve( nbElems * nbComps );

  return nbElems * nbComps > 0;
}

//================================================================================
/*!
 * \brief Set a name of a component countered from zero
 */
//================================================================================

void DriverMED_W_Field::SetCompName(const int iComp, const char* name)
{
  if ( (int)_compNames.size() <= iComp )
    _compNames.resize( iComp + 1 );
  _compNames[ iComp ] = name;
}

//================================================================================
/*!
 * \brief Sets numdt and numit field features. Call this fun before AddValue()!
 */
//================================================================================

void DriverMED_W_Field::SetDtIt(const int dt, const int it)
{
  _dt = dt;
  _it = it;
  _intValues.clear();
  _dblValues.clear();
}

//================================================================================
/*!
 * \brief Adds a float field value 
 */
//================================================================================

void DriverMED_W_Field::AddValue( double val )
{
  _dblValues.push_back( val );
}

//================================================================================
/*!
 * \brief Adds an integer field value 
 */
//================================================================================

void DriverMED_W_Field::AddValue( int    val )
{
  _intValues.push_back( val );
}

//================================================================================
/*!
 * Returns elements in the order they are written in MED file
 */
//================================================================================

SMDS_ElemIteratorPtr DriverMED_W_Field::GetOrderedElems()
{
  if ( _nbElemsByGeom.size() < 2 )
    return SMDS_ElemIteratorPtr();

  if ( _nbElemsByGeom.size() == 2 )
    // sole geom type of elements
    return myMesh->elementsIterator( _elemType );

  std::vector< SMDS_ElemIteratorPtr > iterVec( _nbElemsByGeom.size()-1 );
  for ( size_t iG = 1; iG < _nbElemsByGeom.size(); ++iG )
  {
    int iGeoType = _nbElemsByGeom[ iG ].first;
    iterVec[ iG-1 ] = SMDS_ElemIteratorPtr
      ( new SMDS_ElementVectorIterator( _elemsByGeom[ iGeoType ].begin(),
                                        _elemsByGeom[ iGeoType ].end() ));
  }
  typedef SMDS_IteratorOnIterators
    < const SMDS_MeshElement *, std::vector< SMDS_ElemIteratorPtr > > TItIterator;
  return SMDS_ElemIteratorPtr( new TItIterator( iterVec ));
}

//================================================================================
/*!
 * Writes a field to the file
 */
//================================================================================

Driver_Mesh::Status DriverMED_W_Field::Perform()
{
  if ( myFile.empty() )
    return addMessage("File name not set", /*isFatal=*/true ); // 'fatal' means 'bug'
  if ( myMeshId < 0 && myMeshName.empty() )
    return addMessage("Mesh in file not specified", /*isFatal=*/true );
  if ( _nbElemsByGeom.size() < 2 )
    return addMessage("No values to write", /*isFatal=*/false );
  if ( !myMesh )
    return addMessage("Supporting mesh not set", /*isFatal=*/true );

  MED::PWrapper medFile = MED::CrWrapper( myFile, MED::eV2_2 );
  MED::PMeshInfo meshInfo;
  if ( myMeshId > 0 )
  {
    meshInfo = medFile->GetPMeshInfo( myMeshId );
  }
  else
  {
    // look for a mesh by name
    int aNbMeshes = medFile->GetNbMeshes();
    for ( int iMesh = aNbMeshes; iMesh > 0 && myMeshId < 1; --iMesh )
    {
      meshInfo = medFile->GetPMeshInfo( iMesh );
      if ( !meshInfo || meshInfo->GetName() != myMeshName )
        meshInfo.reset();
      else
        myMeshId = iMesh;
    }
  }
  if (( !meshInfo ) ||
      ( !myMeshName.empty() && meshInfo->GetName() != myMeshName ))
  {
    myMeshId = -1;
    return addMessage("Specified mesh not found in the file", /*isFatal=*/true );
  }

  // create a field
  MED::ETypeChamp  dataType = _dblValues.empty() ? MED::eINT : MED::eFLOAT64;
  MED::PFieldInfo fieldInfo = medFile->CrFieldInfo( meshInfo,
                                                    _compNames.size(),
                                                    dataType );
  fieldInfo->SetName( _fieldName );
  for ( size_t iC = 0; iC < _compNames.size(); ++iC )
  {
    fieldInfo->SetCompName( iC, _compNames[ iC ]);
    fieldInfo->SetUnitName( iC, "");
  }
  if ( _compNames.size() > 1 )
  {
    for ( size_t i = 0; i < fieldInfo->myCompNames.size()-1; ++i )
      if ( !fieldInfo->myCompNames[i] )
        fieldInfo->myCompNames[i] = ' ';
  }
  medFile->SetFieldInfo( fieldInfo );

  // specific treatment of added 0D elements
  if ( _nbElemsByGeom.size()   == 3 &&
       _nbElemsByGeom[1].first == SMDSEntity_0D )
  {
    _nbElemsByGeom[1].second += _nbElemsByGeom[2].second;
    _nbElemsByGeom.resize( 2 );
  }

  // create a time stamp
  MED::TGeom2Size type2nb;
  for ( size_t iG = 1; iG < _nbElemsByGeom.size(); ++iG )
  {
    SMDSAbs_EntityType    smdsType = _nbElemsByGeom[iG].first;
    MED::EGeometrieElement medType = (MED::EGeometrieElement) DriverMED::GetMedGeoType( smdsType );
    int                    nbElems = _nbElemsByGeom[iG].second - _nbElemsByGeom[iG-1].second;
    type2nb.insert( std::make_pair( medType, nbElems ));
  }

  MED::EEntiteMaillage       entity = ( _elemType == SMDSAbs_Node ? MED::eNOEUD : MED::eMAILLE );
  MED::PTimeStampInfo timeStampInfo = medFile->CrTimeStampInfo( fieldInfo, entity, type2nb );
  timeStampInfo->myNumDt  = _dt;
  timeStampInfo->myNumOrd = _it;

  MED::PTimeStampValueBase  timeStampVal = medFile->CrTimeStampValue( timeStampInfo, dataType );
  MED::PFloatTimeStampValue timeStampFltVal = timeStampVal;
  MED::PIntTimeStampValue   timeStampIntVal = timeStampVal;

  // set values
  int iVal = 0;
  MED::TFloat* ptrDbl = 0;
  MED::TInt*   ptrInt = 0;
  for ( size_t iG = 1; iG < _nbElemsByGeom.size(); ++iG )
  {
    SMDSAbs_EntityType    smdsType = _nbElemsByGeom[iG].first;
    MED::EGeometrieElement medType = (MED::EGeometrieElement) DriverMED::GetMedGeoType( smdsType );
    int nbElems = ( _nbElemsByGeom[iG].second - _nbElemsByGeom[iG-1].second ) * _compNames.size();
    if ( dataType == MED::eFLOAT64 )
    {
      ptrDbl = timeStampFltVal->GetMeshValue( medType ).GetPointer();
      for ( int i = 0; i < nbElems; ++i, ++iVal )
        ptrDbl[ i ] = _dblValues[ iVal ];
    }
    else
    {
      ptrInt = timeStampIntVal->GetMeshValue( medType ).GetPointer();
      for ( int i = 0; i < nbElems; ++i, ++iVal )
        ptrInt[ i ] = _intValues[ iVal ];
    }
  }

  // write
  medFile->SetTimeStampValue( timeStampVal );

  _dblValues.clear();
  _intValues.clear();

  return DRS_OK;
}

namespace DriverMED // Implemetation of fuctions declared in DriverMED.hxx
{
  //================================================================================
  /*!
   * Returns a vector containing MED::EGeometrieElement for each SMDSAbs_EntityType
   */
  //================================================================================

  const std::vector< MED::EGeometrieElement >& getMedTypesVec()
  {
    static std::vector< MED::EGeometrieElement > theVec;
    if ( theVec.empty() )
    {
      theVec.resize( SMDSEntity_Last, MED::eAllGeoType );
      theVec[ SMDSEntity_Node               ] = MED::eNONE    ;
      theVec[ SMDSEntity_0D                 ] = MED::ePOINT1  ;
      theVec[ SMDSEntity_Edge               ] = MED::eSEG2    ;
      theVec[ SMDSEntity_Quad_Edge          ] = MED::eSEG3    ;
      theVec[ SMDSEntity_Triangle           ] = MED::eTRIA3   ;
      theVec[ SMDSEntity_Quad_Triangle      ] = MED::eTRIA6   ;
      theVec[ SMDSEntity_BiQuad_Triangle    ] = MED::eTRIA7   ;
      theVec[ SMDSEntity_Quadrangle         ] = MED::eQUAD4   ;
      theVec[ SMDSEntity_Quad_Quadrangle    ] = MED::eQUAD8   ;
      theVec[ SMDSEntity_BiQuad_Quadrangle  ] = MED::eQUAD9   ;
      theVec[ SMDSEntity_Polygon            ] = MED::ePOLYGONE;
      //theVec[ SMDSEntity_Quad_Polygon       ] = MED::ePOLYGONE; // !!
      theVec[ SMDSEntity_Tetra              ] = MED::eTETRA4  ;
      theVec[ SMDSEntity_Quad_Tetra         ] = MED::eTETRA10 ;
      theVec[ SMDSEntity_Pyramid            ] = MED::ePYRA5   ;
      theVec[ SMDSEntity_Quad_Pyramid       ] = MED::ePYRA13  ;
      theVec[ SMDSEntity_Hexa               ] = MED::eHEXA8   ;
      theVec[ SMDSEntity_Quad_Hexa          ] = MED::eHEXA20  ;
      theVec[ SMDSEntity_TriQuad_Hexa       ] = MED::eHEXA27  ;
      theVec[ SMDSEntity_Penta              ] = MED::ePENTA6  ;
      theVec[ SMDSEntity_Quad_Penta         ] = MED::ePENTA15 ;
      theVec[ SMDSEntity_Hexagonal_Prism    ] = MED::eOCTA12  ;
      theVec[ SMDSEntity_Polyhedra          ] = MED::ePOLYEDRE;
      //theVec[ SMDSEntity_Quad_Polyhedra     ] = MED::ePOLYEDRE; // !!
      theVec[ SMDSEntity_Ball               ] = MED::eBALL    ;
    }
    return theVec;
  }

  //================================================================================
  /*!
   * Returns MED element geom type (MED::EGeometrieElement) by SMDS type
   */
  //================================================================================

  int GetMedGeoType( SMDSAbs_EntityType smdsType )
  {
    return getMedTypesVec()[ smdsType ];
  }

  //================================================================================
  /*!
   * Returns SMDS element geom type by MED type (MED::EGeometrieElement)
   */
  //================================================================================

  SMDSAbs_EntityType GetSMDSType( int medType )
  {
    const std::vector< MED::EGeometrieElement >& theVec = getMedTypesVec();

    std::vector< MED::EGeometrieElement >::const_iterator i =
      std::find( theVec.begin(), theVec.end(), medType );

    return SMDSAbs_EntityType( std::distance( theVec.begin(), i ));
  }
}
