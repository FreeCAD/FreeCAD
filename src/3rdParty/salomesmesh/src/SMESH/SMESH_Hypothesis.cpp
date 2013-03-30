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
//  SMESH SMESH : implementaion of SMESH idl descriptions
//  File   : SMESH_Hypothesis.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/SMESH/SMESH_Hypothesis.cxx,v 1.10 2009/02/17 05:27:39 vsr Exp $
//
#include "SMESH_Hypothesis.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_subMesh.hxx"
#include "utilities.h"

using namespace std;

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESH_Hypothesis::SMESH_Hypothesis(int hypId,
				   int studyId,
				   SMESH_Gen* gen) : SMESHDS_Hypothesis(hypId)
{
  //MESSAGE("SMESH_Hypothesis::SMESH_Hypothesis");
  _gen = gen;
  _studyId = studyId;
  StudyContextStruct* myStudyContext = _gen->GetStudyContext(_studyId);
  myStudyContext->mapHypothesis[_hypId] = this;
  _type = PARAM_ALGO;
  _shapeType = 0; // to be set by algo with TopAbs_Enum
  _param_algo_dim = -1; // to be set by algo parameter
  _parameters = string();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

SMESH_Hypothesis::~SMESH_Hypothesis()
{
  MESSAGE("SMESH_Hypothesis::~SMESH_Hypothesis");
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

int SMESH_Hypothesis::GetDim() const
{
  int dim = 0;
  switch (_type)
    {
    case ALGO_1D: dim = 1; break;
    case ALGO_2D: dim = 2; break;
    case ALGO_3D: dim = 3; break;
    case PARAM_ALGO:
      dim = ( _param_algo_dim < 0 ) ? -_param_algo_dim : _param_algo_dim; break;
    }
  return dim;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

int SMESH_Hypothesis::GetShapeType() const
{
  return _shapeType;
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

int SMESH_Hypothesis::GetStudyId() const
{
  return _studyId;
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

void SMESH_Hypothesis::NotifySubMeshesHypothesisModification()
{
  MESSAGE("SMESH_Hypothesis::NotifySubMeshesHypothesisModification");

  // for all meshes in study

  StudyContextStruct* myStudyContext = _gen->GetStudyContext(_studyId);
  map<int, SMESH_Mesh*>::iterator itm;
  for (itm = myStudyContext->mapMesh.begin();
       itm != myStudyContext->mapMesh.end();
       itm++)
    {
      SMESH_Mesh* mesh = (*itm).second;
      mesh->NotifySubMeshesHypothesisModification( this );
    }
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

const char* SMESH_Hypothesis::GetLibName() const
{
  return _libName.c_str();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================

void SMESH_Hypothesis::SetLibName(const char* theLibName)
{
  _libName = string(theLibName);
}

//=============================================================================
/*!
 * 
 */
//=============================================================================
void SMESH_Hypothesis::SetParameters(const char *theParameters)
{
  string aNewParameters(theParameters);
  if(aNewParameters.size()==0 && _parameters.size()==0)
    aNewParameters = " ";
  if(_parameters.size()>0)
    _parameters +="|";
  _parameters +=aNewParameters;
  SetLastParameters(theParameters);
}

//=============================================================================
/*!
 * 
 */
//=============================================================================
void SMESH_Hypothesis::ClearParameters()
{
  _parameters = string();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================
char* SMESH_Hypothesis::GetParameters() const
{
  return (char*)_parameters.c_str();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================
char* SMESH_Hypothesis::GetLastParameters() const
{
  return (char*)_lastParameters.c_str();
}

//=============================================================================
/*!
 * 
 */
//=============================================================================
void SMESH_Hypothesis::SetLastParameters(const char* theParameters)
{
  _lastParameters = string(theParameters);
}
