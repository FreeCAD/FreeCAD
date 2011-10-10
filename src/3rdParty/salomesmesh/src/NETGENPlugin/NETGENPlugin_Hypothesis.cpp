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
//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_Hypothesis.cxx
// Author    : Michael Sazonov (OCN)
// Date      : 28/03/2006
// Project   : SALOME
//
#include "NETGENPlugin_Hypothesis.hxx"
#include <utilities.h>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================
NETGENPlugin_Hypothesis::NETGENPlugin_Hypothesis (int hypId, int studyId,
                                                  SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen),
    _maxSize       (GetDefaultMaxSize()),
    _growthRate    (GetDefaultGrowthRate()),
    _nbSegPerEdge  (GetDefaultNbSegPerEdge()),
    _nbSegPerRadius(GetDefaultNbSegPerRadius()),
    _fineness      (GetDefaultFineness()),
    _secondOrder   (GetDefaultSecondOrder()),
    _optimize      (GetDefaultOptimize())
{
  _name = "NETGEN_Parameters";
  _param_algo_dim = 3;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_Hypothesis::SetMaxSize(double theSize)
{
  if (theSize != _maxSize)
  {
    _maxSize = theSize;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_Hypothesis::SetSecondOrder(bool theVal)
{
  if (theVal != _secondOrder)
  {
    _secondOrder = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_Hypothesis::SetOptimize(bool theVal)
{
  if (theVal != _optimize)
  {
    _optimize = theVal;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_Hypothesis::SetFineness(Fineness theFineness)
{
  if (theFineness != _fineness)
  {
    _fineness = theFineness;
    // the predefined values are taken from NETGEN 4.5 sources
    switch (_fineness)
    {
    case VeryCoarse:
      _growthRate = 0.7;
      _nbSegPerEdge = 0.3;
      _nbSegPerRadius = 1;
      break;
    case Coarse:
      _growthRate = 0.5;
      _nbSegPerEdge = 0.5;
      _nbSegPerRadius = 1.5;
      break;
    case Fine:
      _growthRate = 0.2;
      _nbSegPerEdge = 2;
      _nbSegPerRadius = 3;
      break;
    case VeryFine:
      _growthRate = 0.1;
      _nbSegPerEdge = 3;
      _nbSegPerRadius = 5;
      break;
    case UserDefined:
      break;
    case Moderate:
    default:
      _growthRate = 0.3;
      _nbSegPerEdge = 1;
      _nbSegPerRadius = 2;
      break;
    }
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_Hypothesis::SetGrowthRate(double theRate)
{
  if (theRate != _growthRate)
  {
    _growthRate = theRate;
    _fineness = UserDefined;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_Hypothesis::SetNbSegPerEdge(double theVal)
{
  if (theVal != _nbSegPerEdge)
  {
    _nbSegPerEdge = theVal;
    _fineness = UserDefined;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
void NETGENPlugin_Hypothesis::SetNbSegPerRadius(double theVal)
{
  if (theVal != _nbSegPerRadius)
  {
    _nbSegPerRadius = theVal;
    _fineness = UserDefined;
    NotifySubMeshesHypothesisModification();
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
ostream & NETGENPlugin_Hypothesis::SaveTo(ostream & save)
{
  save << _maxSize << " " << _fineness;

  if (_fineness == UserDefined)
    save << " " << _growthRate << " " << _nbSegPerEdge << " " << _nbSegPerRadius;

  save << " " << (int)_secondOrder << " " << (int)_optimize;

  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
istream & NETGENPlugin_Hypothesis::LoadFrom(istream & load)
{
  bool isOK = true;
  int is;
  double val;

  isOK = (load >> val);
  if (isOK)
    _maxSize = val;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> is);
  if (isOK)
    SetFineness((Fineness) is);
  else
    load.clear(ios::badbit | load.rdstate());

  if (_fineness == UserDefined)
  {
    isOK = (load >> val);
    if (isOK)
      _growthRate = val;
    else
      load.clear(ios::badbit | load.rdstate());

    isOK = (load >> val);
    if (isOK)
      _nbSegPerEdge = val;
    else
      load.clear(ios::badbit | load.rdstate());

    isOK = (load >> val);
    if (isOK)
      _nbSegPerRadius = val;
    else
      load.clear(ios::badbit | load.rdstate());
  }

  isOK = (load >> is);
  if (isOK)
    _secondOrder = (bool) is;
  else
    load.clear(ios::badbit | load.rdstate());

  isOK = (load >> is);
  if (isOK)
    _optimize = (bool) is;
  else
    load.clear(ios::badbit | load.rdstate());
  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
ostream & operator <<(ostream & save, NETGENPlugin_Hypothesis & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
istream & operator >>(istream & load, NETGENPlugin_Hypothesis & hyp)
{
  return hyp.LoadFrom( load );
}


//================================================================================
/*!
 * \brief Does nothing
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - always false
 */
//================================================================================
bool NETGENPlugin_Hypothesis::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                  const TopoDS_Shape& theShape)
{
  return false;
}

//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool NETGENPlugin_Hypothesis::SetParametersByDefaults(const TDefaults&  dflts,
                                                      const SMESH_Mesh* /*theMesh*/)
{
  _nbSegPerEdge = dflts._nbSegments;
  _maxSize      = dflts._elemLength;
  return _nbSegPerEdge && _maxSize > 0;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
double NETGENPlugin_Hypothesis::GetDefaultMaxSize()
{
  return 1000;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
NETGENPlugin_Hypothesis::Fineness NETGENPlugin_Hypothesis::GetDefaultFineness()
{
  return Moderate;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
double NETGENPlugin_Hypothesis::GetDefaultGrowthRate()
{
  return 0.3;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
double NETGENPlugin_Hypothesis::GetDefaultNbSegPerEdge()
{
  return 1;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
double NETGENPlugin_Hypothesis::GetDefaultNbSegPerRadius()
{
  return 2;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
bool NETGENPlugin_Hypothesis::GetDefaultSecondOrder()
{
  return false;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
bool NETGENPlugin_Hypothesis::GetDefaultOptimize()
{
  return true;
}
