// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  File   : SMESH_ComputeError.hxx
//  Author : Edward AGAPOV (eap)
//  Module : SMESH
//
#ifndef SMESH_ComputeError_HeaderFile
#define SMESH_ComputeError_HeaderFile

#include "SMESH_Utils.hxx"

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
  COMPERR_OK               = -1,
  COMPERR_BAD_INPUT_MESH   = -2,  //!< wrong mesh on lower submesh
  COMPERR_STD_EXCEPTION    = -3,  //!< some std exception raised
  COMPERR_OCC_EXCEPTION    = -4,  //!< OCC exception raised
  COMPERR_SLM_EXCEPTION    = -5,  //!< SALOME exception raised
  COMPERR_EXCEPTION        = -6,  //!< other exception raised
  COMPERR_MEMORY_PB        = -7,  //!< std::bad_alloc exception
  COMPERR_ALGO_FAILED      = -8,  //!< algo failed for some reason
  COMPERR_BAD_SHAPE        = -9,  //!< bad geometry
  COMPERR_WARNING          = -10, //!< algo reports error but sub-mesh is computed anyway
  COMPERR_CANCELED         = -11, //!< compute canceled
  COMPERR_NO_MESH_ON_SHAPE = -12, //!< no mesh elements assigned to sub-shape
  COMPERR_BAD_PARMETERS    = -13, //!< incorrect hypotheses parameters
  COMPERR_LAST_ALGO_ERROR  = -100,//!< terminator of mesh computation errors
  // Errors of SMESH_MeshEditor follow
  EDITERR_NO_MEDIUM_ON_GEOM= -101 /* during conversion to quadratic,
                                     some medium nodes not placed on geometry
                                     to avoid distorting elements, which are
                                     stored in SMESH_ComputeError::myBadElements */
};

// =============================================================
/*!
 * \brief Contains an algorithm and description of an occurred error
 */
// =============================================================

struct SMESHUtils_EXPORT SMESH_ComputeError
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

  bool IsOK()        const { return myName == COMPERR_OK; }
  bool IsKO()        const { return myName != COMPERR_OK && myName != COMPERR_WARNING; }
  bool IsCommon()    const { return myName < 0 && myName > COMPERR_LAST_ALGO_ERROR; }
  bool HasBadElems() const { return !myBadElements.empty(); }

  // not inline methods are implemented in   src/SMESHUtils/SMESH_TryCatch.cxx

  // Return myName as text, to be used to dump errors in terminal
  std::string CommonName() const;

  // Return the most severe error
  static SMESH_ComputeErrorPtr Worst( SMESH_ComputeErrorPtr er1,
                                      SMESH_ComputeErrorPtr er2 );
};

#endif
