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
//  File   : SMESH_subMesh.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/SMESH/SMESH_subMesh.hxx,v 1.12.2.3 2008/11/27 12:25:15 abd Exp $
//
#ifndef _SMESH_SUBMESH_HXX_
#define _SMESH_SUBMESH_HXX_

#include "SMESH_SMESH.hxx"

#include "SMESHDS_Mesh.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Hypothesis.hxx"
#include "SMESH_ComputeError.hxx"

#include "SMESH_Exception.hxx"

#include <TopoDS_Shape.hxx>

#include <list>
#include <map>

class SMESH_Mesh;
class SMESH_Hypothesis;
class SMESH_Algo;
class SMESH_Gen;
class SMESH_subMeshEventListener;
class SMESH_subMeshEventListenerData;
class SMESH_subMesh;

typedef SMESH_subMeshEventListener     EventListener;
typedef SMESH_subMeshEventListenerData EventListenerData;

typedef boost::shared_ptr< SMDS_Iterator<SMESH_subMesh*> > SMESH_subMeshIteratorPtr;


class SMESH_EXPORT SMESH_subMesh
{
 public:
  SMESH_subMesh(int Id, SMESH_Mesh * father, SMESHDS_Mesh * meshDS,
		const TopoDS_Shape & aSubShape);
  virtual ~ SMESH_subMesh();

  int GetId() const;

  SMESH_Mesh* GetFather() { return _father; }
  
  SMESHDS_SubMesh * GetSubMeshDS();

  SMESHDS_SubMesh* CreateSubMeshDS();
  // Explicit SMESHDS_SubMesh creation method, required for persistence mechanism

  SMESH_subMesh *GetFirstToCompute();

  const std::map < int, SMESH_subMesh * >& DependsOn();
  //const map < int, SMESH_subMesh * >&Dependants();
  /*!
   * \brief Return iterator on the submeshes this one depends on
   */
  SMESH_subMeshIteratorPtr getDependsOnIterator(const bool includeSelf,
                                                const bool complexShapeFirst);

  const TopoDS_Shape & GetSubShape() const;

  enum compute_state
  {
    NOT_READY, READY_TO_COMPUTE,
    COMPUTE_OK, FAILED_TO_COMPUTE
    };
  enum algo_state
  {
    NO_ALGO, MISSING_HYP, HYP_OK
    };
  enum algo_event
  {
    ADD_HYP          , ADD_ALGO,
    REMOVE_HYP       , REMOVE_ALGO,
    ADD_FATHER_HYP   , ADD_FATHER_ALGO,
    REMOVE_FATHER_HYP, REMOVE_FATHER_ALGO,
    MODIF_HYP
    };
  enum compute_event
  {
    MODIF_ALGO_STATE, COMPUTE,
    CLEAN, SUBMESH_COMPUTED, SUBMESH_RESTORED,
    MESH_ENTITY_REMOVED, CHECK_COMPUTE_STATE
    };
  enum event_type
  {
    ALGO_EVENT, COMPUTE_EVENT
  };

  // ==================================================================
  // Members to track non hierarchical dependencies between submeshes 
  // ==================================================================

  /*!
   * \brief Sets an event listener and its data to a submesh
    * \param listener - the listener to store
    * \param data - the listener data to store
    * \param where - the submesh to store the listener and it's data
   * 
   * The method remembers the submesh \awhere it puts the listener in order to delete
   * them when HYP_OK algo_state is lost
   * After being set, event listener is notified on each event of \awhere submesh.
   */
  void SetEventListener(EventListener*     listener,
                        EventListenerData* data,
                        SMESH_subMesh*     where);

  /*!
   * \brief Return an event listener data
    * \param listener - the listener whose data is
    * \retval EventListenerData* - found data, maybe NULL
   */
  EventListenerData* GetEventListenerData(EventListener* listener) const;

  /*!
   * \brief Unregister the listener and delete it and it's data
    * \param listener - the event listener to delete
   */
  void DeleteEventListener(EventListener* listener);

protected:

  //!< event listeners to notify
  std::map< EventListener*, EventListenerData* >           myEventListeners;
  //!< event listeners to delete when HYP_OK algo_state is lost
  std::list< std::pair< SMESH_subMesh*, EventListener* > > myOwnListeners;

  /*!
   * \brief Sets an event listener and its data to a submesh
    * \param listener - the listener to store
    * \param data - the listener data to store
   * 
   * After being set, event listener is notified on each event of a submesh.
   */
  void SetEventListener(EventListener* listener, EventListenerData* data);

  /*!
   * \brief Notify stored event listeners on the occured event
   * \param event - algo_event or compute_event itself
   * \param eventType - algo_event or compute_event
   * \param hyp - hypothesis, if eventType is algo_event
   */
  void NotifyListenersOnEvent( const int         event,
                               const event_type  eventType,
                               SMESH_Hypothesis* hyp = 0);

  /*!
   * \brief Delete event listeners depending on algo of this submesh
   */
  void DeleteOwnListeners();

  // ==================================================================

public:

  SMESH_Hypothesis::Hypothesis_Status
    AlgoStateEngine(int event, SMESH_Hypothesis * anHyp);

  SMESH_Hypothesis::Hypothesis_Status
    SubMeshesAlgoStateEngine(int event, SMESH_Hypothesis * anHyp);

  int GetAlgoState() const { return _algoState; }
  int GetComputeState() const { return _computeState; };
  SMESH_ComputeErrorPtr& GetComputeError() { return _computeError; }

  void DumpAlgoState(bool isMain);

  bool ComputeStateEngine(int event);

  bool IsConform(const SMESH_Algo* theAlgo);
  // check if a conform mesh will be produced by the Algo

  bool CanAddHypothesis(const SMESH_Hypothesis* theHypothesis) const;
  // return true if theHypothesis can be attached to me:
  // its dimention is checked

  static bool IsApplicableHypotesis(const SMESH_Hypothesis* theHypothesis,
                                    const TopAbs_ShapeEnum  theShapeType);

  bool IsApplicableHypotesis(const SMESH_Hypothesis* theHypothesis) const
  { return IsApplicableHypotesis( theHypothesis, _subShape.ShapeType() ); }
  // return true if theHypothesis can be used to mesh me:
  // its shape type is checked
  
  SMESH_Hypothesis::Hypothesis_Status CheckConcurentHypothesis (const int theHypType);
  // check if there are several applicable hypothesis on fathers

  /*!
   * \brief Return true if no mesh entities is bound to the submesh
   */
  bool IsEmpty() const;

  bool IsMeshComputed() const;
  // check if _subMeshDS contains mesh elements

  /*!
   * \brief Allow algo->Compute() if a subshape of lower dim is meshed but
   *        none mesh entity is bound to it
   */
  void SetIsAlwaysComputed(bool isAlCo);
  
  bool IsAlwaysComputed() { return _alwaysComputed; }

  
protected:
  // ==================================================================
  void InsertDependence(const TopoDS_Shape aSubShape);

  bool SubMeshesComputed();

  bool SubMeshesReady();

  void RemoveSubMeshElementsAndNodes();
  void UpdateDependantsState(const compute_event theEvent);
  void UpdateSubMeshState(const compute_state theState);
  void ComputeSubMeshStateEngine(int event);
  void CleanDependants();
  void CleanDependsOn();
  void SetAlgoState(int state);

  /*!
   * \brief Return a shape containing all sub-shapes of the MainShape that can be
   * meshed at once along with _subShape
   */
  TopoDS_Shape GetCollection(SMESH_Gen * theGen,
                             SMESH_Algo* theAlgo,
                             bool &      theSubComputed);

  /*!
   * \brief Apply theAlgo to all subshapes in theCollection
   */
  bool ApplyToCollection (SMESH_Algo*         theAlgo,
                          const TopoDS_Shape& theCollection);

  /*!
   * \brief Update compute_state by _computeError
    * \retval bool - false if there are errors
   */
  bool CheckComputeError(SMESH_Algo* theAlgo, const TopoDS_Shape& theShape=TopoDS_Shape());

  /*!
   * \brief Return a hypothesis attached to theShape.
   * 
   * If theHyp is provided, similar but not same hypotheses
   * is returned; else an applicable ones having theHypType
   * is returned
   */
  const SMESH_Hypothesis* GetSimilarAttached(const TopoDS_Shape&      theShape,
                                             const SMESH_Hypothesis * theHyp,
                                             const int                theHypType = 0);
  // 

protected:

  TopoDS_Shape          _subShape;
  SMESHDS_SubMesh *     _subMeshDS;
  SMESH_Mesh *          _father;
  int                   _Id;

  std::map < int, SMESH_subMesh * >_mapDepend;
  bool                  _dependenceAnalysed;

  int                   _algoState;
  int                   _computeState;
  SMESH_ComputeErrorPtr _computeError;

  // allow algo->Compute() if a subshape of lower dim is meshed but
  // none mesh entity is bound to it. Eg StdMeshers_CompositeSegment_1D can
  // mesh several edges as a whole and leave some of them  without mesh entities
  bool                  _alwaysComputed;

};

#endif
