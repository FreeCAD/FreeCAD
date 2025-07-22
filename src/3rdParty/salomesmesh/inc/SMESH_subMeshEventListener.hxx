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

// File    : SMESH_subMeshEventListener.hxx
// Created : Mon Nov 13 10:45:49 2006
// Author  : Edward AGAPOV (eap)
//
#ifndef SMESH_subMeshEventListener_HeaderFile
#define SMESH_subMeshEventListener_HeaderFile

#include "SMESH_SMESH.hxx"

#include <list>
#include <set>

class  SMESH_subMesh;
class  SMESH_Hypothesis;
struct SMESH_subMeshEventListenerData;

// ------------------------------------------------------------------
/*!
 * \brief A base for objects reacting on submesh events
 */
// ------------------------------------------------------------------

class SMESH_EXPORT SMESH_subMeshEventListener
{
  bool myIsDeletable; //!< if true, it will be deleted by SMESH_subMesh
  mutable std::set<SMESH_subMesh*> myBusySM; //!< to avoid infinite recursion via events
  const char*                      myName;   //!< identifier
  friend class SMESH_subMesh;

 public:
  SMESH_subMeshEventListener(bool isDeletable, const char* name)
    :myIsDeletable(isDeletable), myName(name) {}
  virtual      ~SMESH_subMeshEventListener() {}
  bool         IsDeletable() const { return myIsDeletable; }
  const char*  GetName()     const { return myName; }
  virtual void BeforeDelete(SMESH_subMesh*                  subMesh,
                            SMESH_subMeshEventListenerData* data)
  {}
  /*!
   * \brief Do something on a certain event
   * \param event - algo_event or compute_event itself (of SMESH_subMesh)
   * \param eventType - ALGO_EVENT or COMPUTE_EVENT (of SMESH_subMesh)
   * \param subMesh - the submesh where the event occurs
   * \param data - listener data stored in the subMesh
   * \param hyp - hypothesis, if eventType is algo_event
   * 
   * The base implementation (see SMESH_subMesh.cxx) translates CLEAN event
   * to the subMeshes stored in the listener data. Also it sends SUBMESH_COMPUTED
   * event in case of successful COMPUTE event.
   */
  virtual void ProcessEvent(const int          event,
                            const int          eventType,
                            SMESH_subMesh*     subMesh,
                            SMESH_subMeshEventListenerData* data,
                            const SMESH_Hypothesis*         hyp = 0);
};

// ------------------------------------------------------------------
/*!
 * \brief Data specific for EventListener and to be stored in a submesh
 */
// ------------------------------------------------------------------

struct SMESH_subMeshEventListenerData
{
  bool myIsDeletable; //!< if true, it will be deleted by SMESH_subMesh
  int myType;         //!< to recognize data type
  std::list<SMESH_subMesh*> mySubMeshes; /* generally: submeshes depending
                                            on the one storing this data;
                                            !! they are used to track intermesh
                                            dependencies at mesh loading as well !! */
public:
  SMESH_subMeshEventListenerData(bool isDeletable):myIsDeletable(isDeletable) {}
  virtual ~SMESH_subMeshEventListenerData() {}
  bool IsDeletable() const { return myIsDeletable; }

  /*!
   * \brief Create a default listener data.
   * \param dependentSM - subMesh to store
   * \param type - data type
   * \retval SMESH_subMeshEventListenerData* - a new listener data
   *
   * See SMESH_subMeshEventListener::ProcessEvent() to know how the default
   * listener uses it (implementation is in SMESH_subMesh.cxx)
   */
  static SMESH_subMeshEventListenerData* MakeData(SMESH_subMesh* dependentSM,
                                                  const int      type = 0)
  {
    SMESH_subMeshEventListenerData* data = new SMESH_subMeshEventListenerData(true);
    data->mySubMeshes.push_back( dependentSM );
    data->myType = type;
    return data;
  }
};


#endif
