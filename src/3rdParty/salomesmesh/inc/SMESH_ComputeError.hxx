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
//  File   : SMESH_Hypothesis.hxx
//  Author : Edward AGAPOV (eap)
//  Module : SMESH
//  $Header: 
//
#ifndef SMESH_ComputeError_HeaderFile
#define SMESH_ComputeError_HeaderFile

#include <string>
#include <list>
#include <boost/shared_ptr.hpp>

class SMESH_Algo;
class SMDS_MeshElement;
struct SMESH_ComputeError;

typedef boost::shared_ptr<SMESH_ComputeError> SMESH_ComputeErrorPtr;

// =============================================================

enum SMESH_ComputeErrorName
{
  // If you modify it, pls update SMESH_ComputeError::CommonName() below.
  // Positive values are for algo specific errors
  COMPERR_OK             = -1,
  COMPERR_BAD_INPUT_MESH = -2,  //!< wrong mesh on lower submesh
  COMPERR_STD_EXCEPTION  = -3,  //!< some std exception raised
  COMPERR_OCC_EXCEPTION  = -4,  //!< OCC exception raised
  COMPERR_SLM_EXCEPTION  = -5,  //!< SALOME exception raised
  COMPERR_EXCEPTION      = -6,  //!< other exception raised
  COMPERR_MEMORY_PB      = -7,  //!< std::bad_alloc exception
  COMPERR_ALGO_FAILED    = -8,  //!< algo failed for some reason
  COMPERR_BAD_SHAPE      = -9   //!< bad geometry
};

// =============================================================
/*!
 * \brief Contains algorithm and description of occured error
 */
// =============================================================

struct SMESH_ComputeError
{
  int               myName; //!< SMESH_ComputeErrorName or anything algo specific
  std::string       myComment;
  const SMESH_Algo* myAlgo;

  std::list<const SMDS_MeshElement*> myBadElements; //!< to explain COMPERR_BAD_INPUT_MESH

  static SMESH_ComputeErrorPtr New( int               error   = COMPERR_OK,
                                    std::string       comment = "",
                                    const SMESH_Algo* algo    = 0)
  { return SMESH_ComputeErrorPtr( new SMESH_ComputeError( error, comment, algo )); }

  SMESH_ComputeError(int               error   = COMPERR_OK,
                     std::string       comment = "",
                     const SMESH_Algo* algo    = 0)
    :myName(error),myComment(comment),myAlgo(algo) {}

  bool IsOK()     { return myName == COMPERR_OK; }
  bool IsCommon() { return myName < 0; }
  inline std::string CommonName() const;

};

#define _case2char(err) case err: return #err;

std::string SMESH_ComputeError::CommonName() const
{
  switch( myName ) {
  _case2char(COMPERR_OK            );
  _case2char(COMPERR_BAD_INPUT_MESH);
  _case2char(COMPERR_STD_EXCEPTION );
  _case2char(COMPERR_OCC_EXCEPTION );
  _case2char(COMPERR_SLM_EXCEPTION );
  _case2char(COMPERR_EXCEPTION     );
  _case2char(COMPERR_MEMORY_PB     );
  _case2char(COMPERR_ALGO_FAILED   );
  default:;
  }
  return "";
}

#endif
