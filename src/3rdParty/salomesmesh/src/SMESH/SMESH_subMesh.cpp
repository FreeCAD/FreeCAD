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
//  File   : SMESH_subMesh.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "SMESH_subMesh.hxx"

#include "SMESH_Algo.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Hypothesis.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "SMESH_Comment.hxx"
#include "SMDS_SetIterator.hxx"

#include "utilities.h"
//#include "OpUtil.hxx"

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Pnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Iterator.hxx>

#include <Standard_OutOfMemory.hxx>
#include <Standard_ErrorHandler.hxx>

using namespace std;

//=============================================================================
/*!
 * \brief Allocate some memory at construction and release it at destruction.
 * Is used to be able to continue working after mesh generation breaks due to
 * lack of memory
 */
//=============================================================================

struct MemoryReserve
{
  char* myBuf;
  MemoryReserve(): myBuf( new char[1024*1024*2] ){}
  ~MemoryReserve() { delete [] myBuf; }
};

//=============================================================================
/*!
 *  default constructor:
 */
//=============================================================================

SMESH_subMesh::SMESH_subMesh(int                  Id,
                             SMESH_Mesh *         father,
                             SMESHDS_Mesh *       meshDS,
                             const TopoDS_Shape & aSubShape)
{
	_subShape = aSubShape;
	_subMeshDS = meshDS->MeshElements(_subShape);	// may be null ...
	_father = father;
	_Id = Id;
	_dependenceAnalysed = _alwaysComputed = false;

	if (_subShape.ShapeType() == TopAbs_VERTEX)
	{
		_algoState = HYP_OK;
		_computeState = READY_TO_COMPUTE;
	}
	else
	{
          _algoState = NO_ALGO;
          _computeState = NOT_READY;
	}
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESH_subMesh::~SMESH_subMesh()
{
  //MESSAGE("SMESH_subMesh::~SMESH_subMesh");
  // ****
  DeleteOwnListeners();
}

//=============================================================================
/*!
 *
 */
//=============================================================================

int SMESH_subMesh::GetId() const
{
  //MESSAGE("SMESH_subMesh::GetId");
  return _Id;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESHDS_SubMesh * SMESH_subMesh::GetSubMeshDS()
{
  // submesh appears in DS only when a mesher set nodes and elements on a shape
  return _subMeshDS ? _subMeshDS : _subMeshDS = _father->GetMeshDS()->MeshElements(_subShape); // may be null
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESHDS_SubMesh* SMESH_subMesh::CreateSubMeshDS()
{
  if ( !GetSubMeshDS() ) {
    SMESHDS_Mesh* meshDS = _father->GetMeshDS();
    meshDS->NewSubMesh( meshDS->ShapeToIndex( _subShape ) );
  }
  return GetSubMeshDS();
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESH_subMesh *SMESH_subMesh::GetFirstToCompute()
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(true,false);
  while ( smIt->more() ) {
    SMESH_subMesh *sm = smIt->next();
    if ( sm->GetComputeState() == READY_TO_COMPUTE )
      return sm;
  }
  return 0;                     // nothing to compute
}

//================================================================================
/*!
 * \brief Allow algo->Compute() if a subshape of lower dim is meshed but
 *        none mesh entity is bound to it (PAL13615, 2nd part)
 */
//================================================================================

void SMESH_subMesh::SetIsAlwaysComputed(bool isAlCo)
{
  _alwaysComputed = isAlCo;
  if ( _alwaysComputed )
    _computeState = COMPUTE_OK;
  else
    ComputeStateEngine( CHECK_COMPUTE_STATE );
}

//=======================================================================
/*!
 * \brief Return true if no mesh entities is bound to the submesh
 */
//=======================================================================

bool SMESH_subMesh::IsEmpty() const
{
  if (SMESHDS_SubMesh * subMeshDS = ((SMESH_subMesh*)this)->GetSubMeshDS())
    return (!subMeshDS->NbElements() && !subMeshDS->NbNodes());
  return true;
}

//=======================================================================
//function : IsMeshComputed
//purpose  : check if _subMeshDS contains mesh elements
//=======================================================================

bool SMESH_subMesh::IsMeshComputed() const
{
  if ( _alwaysComputed )
    return true;
  // algo may bind a submesh not to _subShape, eg 3D algo
  // sets nodes on SHELL while _subShape may be SOLID

  SMESHDS_Mesh* meshDS = _father->GetMeshDS();
  int dim = SMESH_Gen::GetShapeDim( _subShape );
  int type = _subShape.ShapeType();
  for ( ; type <= TopAbs_VERTEX; type++) {
    if ( dim == SMESH_Gen::GetShapeDim( (TopAbs_ShapeEnum) type ))
    {
      TopExp_Explorer exp( _subShape, (TopAbs_ShapeEnum) type );
      for ( ; exp.More(); exp.Next() )
      {
        if ( SMESHDS_SubMesh * smDS = meshDS->MeshElements( exp.Current() ))
        {
          bool computed = (dim > 0) ? smDS->NbElements() : smDS->NbNodes();
          if ( computed )
            return true;
        }
      }
    }
    else
      break;
  }

  return false;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool SMESH_subMesh::SubMeshesComputed()
{
  int myDim = SMESH_Gen::GetShapeDim( _subShape );
  int dimToCheck = myDim - 1;
  bool subMeshesComputed = true;
  // check subMeshes with upper dimension => reverse iteration
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,true);
  while ( smIt->more() )
  {
    SMESH_subMesh *sm = smIt->next();
    if ( sm->_alwaysComputed )
      continue;
    const TopoDS_Shape & ss = sm->GetSubShape();
    // MSV 07.04.2006: restrict checking to myDim-1 only. Ex., there is no sense
    // in checking of existence of edges if the algo needs only faces. Moreover,
    // degenerated edges may have no submesh, as after computing NETGEN_2D.
    int dim = SMESH_Gen::GetShapeDim( ss );
    if (dim < dimToCheck)
      break; // the rest subMeshes are all of less dimension
    SMESHDS_SubMesh * ds = sm->GetSubMeshDS();
    bool computeOk = (sm->GetComputeState() == COMPUTE_OK ||
                      (ds && ( ds->NbNodes() || ds->NbElements() )));
    if (!computeOk)
    {
      int type = ss.ShapeType();

      subMeshesComputed = false;

      switch (type)
      {
      case TopAbs_COMPOUND:
        {
          MESSAGE("The not computed sub mesh is a COMPOUND");
          break;
        }
      case TopAbs_COMPSOLID:
        {
          MESSAGE("The not computed sub mesh is a COMPSOLID");
          break;
        }
      case TopAbs_SHELL:
        {
          MESSAGE("The not computed sub mesh is a SHEL");
          break;
        }
      case TopAbs_WIRE:
        {
          MESSAGE("The not computed sub mesh is a WIRE");
          break;
        }
      case TopAbs_SOLID:
        {
          MESSAGE("The not computed sub mesh is a SOLID");
          break;
        }
      case TopAbs_FACE:
        {
          MESSAGE("The not computed sub mesh is a FACE");
          break;
        }
      case TopAbs_EDGE:
        {
          MESSAGE("The not computed sub mesh is a EDGE");
          break;
        }
      default:
        {
          MESSAGE("The not computed sub mesh is of unknown type");
          break;
        }
      }

      break;
    }
  }
  return subMeshesComputed;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool SMESH_subMesh::SubMeshesReady()
{
  bool subMeshesReady = true;
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,true);
  while ( smIt->more() ) {
    SMESH_subMesh *sm = smIt->next();
    bool computeOk = (sm->GetComputeState() == COMPUTE_OK ||
                      sm->GetComputeState() == READY_TO_COMPUTE);
    if (!computeOk)
    {
      subMeshesReady = false;
      SCRUTE(sm->GetId());
      break;
    }
  }
  return subMeshesReady;
}

//=============================================================================
/*!
 * Construct dependence on first level subMeshes. complex shapes (compsolid,
 * shell, wire) are not analysed the same way as simple shapes (solid, face,
 * edge).
 * For collection shapes (compsolid, shell, wire) prepare a list of submeshes
 * with possible multiples occurences. Multiples occurences corresponds to
 * internal frontiers within shapes of the collection and must not be keeped.
 * See FinalizeDependence.
 */
//=============================================================================

const map < int, SMESH_subMesh * >& SMESH_subMesh::DependsOn()
{
  if (_dependenceAnalysed)
    return _mapDepend;

  //MESSAGE("SMESH_subMesh::DependsOn");

  int type = _subShape.ShapeType();
  //SCRUTE(type);
  switch (type)
  {
  case TopAbs_COMPOUND:
    {
      //MESSAGE("compound");
      for (TopExp_Explorer exp(_subShape, TopAbs_SOLID); exp.More();
           exp.Next())
      {
        InsertDependence(exp.Current());
      }
      for (TopExp_Explorer exp(_subShape, TopAbs_SHELL, TopAbs_SOLID); exp.More();
           exp.Next())
      {
          InsertDependence(exp.Current());      //only shell not in solid
      }
      for (TopExp_Explorer exp(_subShape, TopAbs_FACE, TopAbs_SHELL); exp.More();
           exp.Next())
      {
        InsertDependence(exp.Current());
      }
      for (TopExp_Explorer exp(_subShape, TopAbs_EDGE, TopAbs_FACE); exp.More();
           exp.Next())
      {
        InsertDependence(exp.Current());
      }
      break;
    }
  case TopAbs_COMPSOLID:
    {
		//MESSAGE("compsolid");
      for (TopExp_Explorer exp(_subShape, TopAbs_SOLID); exp.More();
           exp.Next())
      {
        InsertDependence(exp.Current());
      }
      break;
    }
  case TopAbs_SHELL:
    {
      //MESSAGE("shell");
      for (TopExp_Explorer exp(_subShape, TopAbs_FACE); exp.More();
           exp.Next())
      {
        InsertDependence(exp.Current());
      }
      break;
    }
  case TopAbs_WIRE:
    {
      //MESSAGE("wire");
      for (TopExp_Explorer exp(_subShape, TopAbs_EDGE); exp.More();
           exp.Next())
      {
        InsertDependence(exp.Current());
      }
      break;
    }
  case TopAbs_SOLID:
    {
      //MESSAGE("solid");
      if(_father->HasShapeToMesh()) {
        for (TopExp_Explorer exp(_subShape, TopAbs_FACE); exp.More();
             exp.Next())
        {
          InsertDependence(exp.Current());
        }
      }
      break;
    }
  case TopAbs_FACE:
    {
      //MESSAGE("face");
      for (TopExp_Explorer exp(_subShape, TopAbs_EDGE); exp.More();
           exp.Next())
      {
        InsertDependence(exp.Current());
      }
      break;
    }
  case TopAbs_EDGE:
    {
      //MESSAGE("edge");
      for (TopExp_Explorer exp(_subShape, TopAbs_VERTEX); exp.More();
           exp.Next())
      {
			InsertDependence(exp.Current());
                      }
      break;
    }
  case TopAbs_VERTEX:
    {
      break;
    }
  default:
    {
      break;
    }
  }
  _dependenceAnalysed = true;
  return _mapDepend;
}

//=============================================================================
/*!
 * For simple Shapes (solid, face, edge): add subMesh into dependence list.
 */
//=============================================================================

void SMESH_subMesh::InsertDependence(const TopoDS_Shape aSubShape)
{
  //MESSAGE("SMESH_subMesh::InsertDependence");
  SMESH_subMesh *aSubMesh = _father->GetSubMesh(aSubShape);
  int type = aSubShape.ShapeType();
  int ordType = 9 - type;               // 2 = Vertex, 8 = CompSolid
  int cle = aSubMesh->GetId();
  cle += 10000000 * ordType;    // sort map by ordType then index
  if ( _mapDepend.find( cle ) == _mapDepend.end())
  {
    _mapDepend[cle] = aSubMesh;
    const map < int, SMESH_subMesh * > & subMap = aSubMesh->DependsOn();
    _mapDepend.insert( subMap.begin(), subMap.end() );
  }
}

//=============================================================================
/*!
 *
 */
//=============================================================================

const TopoDS_Shape & SMESH_subMesh::GetSubShape() const
{
	//MESSAGE("SMESH_subMesh::GetSubShape");
	return _subShape;
}


//=======================================================================
//function : CanAddHypothesis
//purpose  : return true if theHypothesis can be attached to me:
//           its dimention is checked
//=======================================================================

bool SMESH_subMesh::CanAddHypothesis(const SMESH_Hypothesis* theHypothesis) const
{
  int aHypDim   = theHypothesis->GetDim();
  int aShapeDim = SMESH_Gen::GetShapeDim(_subShape);
  if (aHypDim == 3 && aShapeDim == 3) {
    // check case of open shell
    //if (_subShape.ShapeType() == TopAbs_SHELL && !_subShape.Closed())
    if (_subShape.ShapeType() == TopAbs_SHELL && !BRep_Tool::IsClosed(_subShape))
      return false;
  }
  if ( aHypDim <= aShapeDim )
    return true;

  return false;
}

//=======================================================================
//function : IsApplicableHypotesis
//purpose  :
//=======================================================================

bool SMESH_subMesh::IsApplicableHypotesis(const SMESH_Hypothesis* theHypothesis,
                                          const TopAbs_ShapeEnum  theShapeType)
{
  if ( theHypothesis->GetType() > SMESHDS_Hypothesis::PARAM_ALGO)
    // algorithm
    return ( theHypothesis->GetShapeType() & (1<< theShapeType));

  // hypothesis
  switch ( theShapeType ) {
  case TopAbs_VERTEX:
  case TopAbs_EDGE:
  case TopAbs_FACE:
  case TopAbs_SOLID:
    return SMESH_Gen::GetShapeDim( theShapeType ) == theHypothesis->GetDim();

  case TopAbs_SHELL:
    // Special case for algorithms, building 2D mesh on a whole shell.
    // Before this fix there was a problem after restoring from study,
    // because in that case algorithm is assigned before hypothesis
    // (on shell in problem case) and hypothesis is checked on faces
    // (because it is 2D), where we have NO_ALGO state.
    // Now 2D hypothesis is also applicable to shells.
    return (theHypothesis->GetDim() == 2 || theHypothesis->GetDim() == 3);

//   case TopAbs_WIRE:
//   case TopAbs_COMPSOLID:
//   case TopAbs_COMPOUND:
  default:;
  }
  return false;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESH_Hypothesis::Hypothesis_Status
  SMESH_subMesh::AlgoStateEngine(int event, SMESH_Hypothesis * anHyp)
{
  //  MESSAGE("SMESH_subMesh::AlgoStateEngine");
  //SCRUTE(_algoState);
  //SCRUTE(event);

  // **** les retour des evenement shape sont significatifs
  // (add ou remove fait ou non)
  // le retour des evenement father n'indiquent pas que add ou remove fait

  SMESH_Hypothesis::Hypothesis_Status aux_ret, ret = SMESH_Hypothesis::HYP_OK;

  SMESHDS_Mesh* meshDS =_father->GetMeshDS();
  SMESH_Gen*    gen    =_father->GetGen();
  SMESH_Algo*   algo   = 0;

  if (_subShape.ShapeType() == TopAbs_VERTEX )
  {
    if ( anHyp->GetDim() != 0) {
      if (event == ADD_HYP || event == ADD_ALGO)
        return SMESH_Hypothesis::HYP_BAD_DIM;
      else
        return SMESH_Hypothesis::HYP_OK;
    }
    // 0D hypothesis
    else if ( _algoState == HYP_OK ) {
      // update default _algoState
      if ( event != REMOVE_FATHER_ALGO )
      {
        _algoState = NO_ALGO;
        algo = gen->GetAlgo(*_father, _subShape);
        if ( algo ) {
          _algoState = MISSING_HYP;
          if ( event == REMOVE_FATHER_HYP ||
               algo->CheckHypothesis(*_father,_subShape, aux_ret))
            _algoState = HYP_OK;
        }
      }
    }
  }

  int oldAlgoState = _algoState;
  bool modifiedHyp = (event == MODIF_HYP);  // if set to true, force event MODIF_ALGO_STATE
  bool needFullClean = false;

  bool isApplicableHyp = IsApplicableHypotesis( anHyp );

  if (event == ADD_ALGO || event == ADD_FATHER_ALGO)
  {
    // -------------------------------------------
    // check if a shape needed by algo is present
    // -------------------------------------------
    algo = static_cast< SMESH_Algo* >( anHyp );
    if ( !_father->HasShapeToMesh() && algo->NeedShape() )
      return SMESH_Hypothesis::HYP_NEED_SHAPE;
    // ----------------------
    // check mesh conformity
    // ----------------------
    if (isApplicableHyp && !_father->IsNotConformAllowed() && !IsConform( algo ))
      return SMESH_Hypothesis::HYP_NOTCONFORM;

    // check if all-dimensional algo is hidden by other local one
    if ( event == ADD_ALGO ) {
      SMESH_HypoFilter filter( SMESH_HypoFilter::HasType( algo->GetType() ));
      filter.Or( SMESH_HypoFilter::HasType( algo->GetType()+1 ));
      filter.Or( SMESH_HypoFilter::HasType( algo->GetType()+2 ));
      if ( SMESH_Algo * curAlgo = (SMESH_Algo*) _father->GetHypothesis( _subShape, filter, true ))
        needFullClean = ( !curAlgo->NeedDescretBoundary() );
    }
  }

  // ----------------------------------
  // add a hypothesis to DS if possible
  // ----------------------------------
  if (event == ADD_HYP || event == ADD_ALGO)
  {
    if ( ! CanAddHypothesis( anHyp )) // check dimension
      return SMESH_Hypothesis::HYP_BAD_DIM;

    if ( /*!anHyp->IsAuxiliary() &&*/ GetSimilarAttached( _subShape, anHyp ) )
      return SMESH_Hypothesis::HYP_ALREADY_EXIST;

    if ( !meshDS->AddHypothesis(_subShape, anHyp))
      return SMESH_Hypothesis::HYP_ALREADY_EXIST;
  }

  // --------------------------
  // remove a hypothesis from DS
  // --------------------------
  if (event == REMOVE_HYP || event == REMOVE_ALGO)
  {
    if (!meshDS->RemoveHypothesis(_subShape, anHyp))
      return SMESH_Hypothesis::HYP_OK; // nothing changes

    if (event == REMOVE_ALGO)
    {
      algo = dynamic_cast<SMESH_Algo*> (anHyp);
      if (!algo->NeedDescretBoundary())
      {
        // clean all mesh in the tree of the current submesh;
        // we must perform it now because later
        // we will have no information about the type of the removed algo
        needFullClean = true;
      }
    }
  }

  // ------------------
  // analyse algo state
  // ------------------
  if (!isApplicableHyp)
    return ret; // not applicable hypotheses do not change algo state

  switch (_algoState)
  {

    // ----------------------------------------------------------------------

  case NO_ALGO:
    switch (event) {
    case ADD_HYP:
      break;
    case ADD_ALGO: {
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if (algo->CheckHypothesis((*_father),_subShape, aux_ret))
        SetAlgoState(HYP_OK);
      else if ( algo->IsStatusFatal( aux_ret )) {
        meshDS->RemoveHypothesis(_subShape, anHyp);
        ret = aux_ret;
      }
      else
        SetAlgoState(MISSING_HYP);
      break;
    }
    case REMOVE_HYP:
    case REMOVE_ALGO:
    case ADD_FATHER_HYP:
      break;
    case ADD_FATHER_ALGO: {    // Algo just added in father
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if ( algo == anHyp ) {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret))
          SetAlgoState(HYP_OK);
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    }
    case REMOVE_FATHER_HYP:
      break;
    case REMOVE_FATHER_ALGO: {
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo)
      {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
            SetAlgoState(HYP_OK);
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    }
    case MODIF_HYP: break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case MISSING_HYP:
    switch (event)
    {
    case ADD_HYP: {
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, ret ))
        SetAlgoState(HYP_OK);
      if (SMESH_Hypothesis::IsStatusFatal( ret ))
        meshDS->RemoveHypothesis(_subShape, anHyp);
      else if (!_father->IsUsedHypothesis( anHyp, this ))
      {
        meshDS->RemoveHypothesis(_subShape, anHyp);
        ret = SMESH_Hypothesis::HYP_INCOMPATIBLE;
      }
      break;
    }
    case ADD_ALGO: {           //already existing algo : on father ?
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))// ignore hyp status
        SetAlgoState(HYP_OK);
      else if ( algo->IsStatusFatal( aux_ret )) {
        meshDS->RemoveHypothesis(_subShape, anHyp);
        ret = aux_ret;
      }
      else
        SetAlgoState(MISSING_HYP);
      break;
    }
    case REMOVE_HYP:
      break;
    case REMOVE_ALGO: {        // perhaps a father algo applies ?
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo == NULL)  // no more algo applying on subShape...
      {
        SetAlgoState(NO_ALGO);
      }
      else
      {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
          SetAlgoState(HYP_OK);
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    }
    case MODIF_HYP: // assigned hypothesis value may become good
    case ADD_FATHER_HYP: {
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
        SetAlgoState(HYP_OK);
      else
        SetAlgoState(MISSING_HYP);
      break;
    }
    case ADD_FATHER_ALGO: { // new father algo
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT( algo );
      if ( algo == anHyp ) {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
          SetAlgoState(HYP_OK);
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    }
    case REMOVE_FATHER_HYP:    // nothing to do
      break;
    case REMOVE_FATHER_ALGO: {
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo == NULL)  // no more applying algo on father
      {
        SetAlgoState(NO_ALGO);
      }
      else
      {
        if ( algo->CheckHypothesis((*_father),_subShape , aux_ret ))
          SetAlgoState(HYP_OK);
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    }
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case HYP_OK:
    switch (event)
    {
    case ADD_HYP: {
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if (!algo->CheckHypothesis((*_father),_subShape, ret ))
      {
        if ( !SMESH_Hypothesis::IsStatusFatal( ret ))
          // ret should be fatal: anHyp was not added
          ret = SMESH_Hypothesis::HYP_INCOMPATIBLE;
      }
      else if (!_father->IsUsedHypothesis(  anHyp, this ))
        ret = SMESH_Hypothesis::HYP_INCOMPATIBLE;

      if (SMESH_Hypothesis::IsStatusFatal( ret ))
      {
        MESSAGE("do not add extra hypothesis");
        meshDS->RemoveHypothesis(_subShape, anHyp);
      }
      else
      {
        modifiedHyp = true;
      }
      break;
    }
    case ADD_ALGO: {           //already existing algo : on father ?
      algo = gen->GetAlgo((*_father), _subShape);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
        // check if algo changes
        SMESH_HypoFilter f;
        f.Init(   SMESH_HypoFilter::IsAlgo() );
        f.And(    SMESH_HypoFilter::IsApplicableTo( _subShape ));
        f.AndNot( SMESH_HypoFilter::Is( algo ));
        const SMESH_Hypothesis * prevAlgo = _father->GetHypothesis( _subShape, f, true );
        if (prevAlgo &&
            string(algo->GetName()) != string(prevAlgo->GetName()) )
          modifiedHyp = true;
      }
      else
        SetAlgoState(MISSING_HYP);
      break;
    }
    case REMOVE_HYP: {
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
        SetAlgoState(HYP_OK);
      else
        SetAlgoState(MISSING_HYP);
      modifiedHyp = true;
      break;
    }
    case REMOVE_ALGO: {         // perhaps a father algo applies ?
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo == NULL)   // no more algo applying on subShape...
      {
        SetAlgoState(NO_ALGO);
      }
      else
      {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
          // check if algo remains
          if ( anHyp != algo && strcmp( anHyp->GetName(), algo->GetName()) )
            modifiedHyp = true;
        }
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    }
    case MODIF_HYP: // hypothesis value may become bad
    case ADD_FATHER_HYP: {  // new father hypothesis ?
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
      {
        if (_father->IsUsedHypothesis( anHyp, this )) // new Hyp
          modifiedHyp = true;
      }
      else
        SetAlgoState(MISSING_HYP);
      break;
    }
    case ADD_FATHER_ALGO: {
      algo = gen->GetAlgo((*_father), _subShape);
      if ( algo == anHyp ) { // a new algo on father
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
          // check if algo changes
          SMESH_HypoFilter f;
          f.Init(   SMESH_HypoFilter::IsAlgo() );
          f.And(    SMESH_HypoFilter::IsApplicableTo( _subShape ));
          f.AndNot( SMESH_HypoFilter::Is( algo ));
          const SMESH_Hypothesis* prevAlgo = _father->GetHypothesis( _subShape, f, true );
          if (prevAlgo &&
              string(algo->GetName()) != string(prevAlgo->GetName()) )
            modifiedHyp = true;
        }
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    }
    case REMOVE_FATHER_HYP: {
      algo = gen->GetAlgo((*_father), _subShape);
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
        // is there the same local hyp or maybe a new father algo applied?
        if ( !GetSimilarAttached( _subShape, anHyp ) )
          modifiedHyp = true;
      }
      else
        SetAlgoState(MISSING_HYP);
      break;
    }
    case REMOVE_FATHER_ALGO: {
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo == NULL)  // no more applying algo on father
      {
        SetAlgoState(NO_ALGO);
      }
      else
      {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
          // check if algo changes
          if ( string(algo->GetName()) != string( anHyp->GetName()) )
            modifiedHyp = true;
        }
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    }
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  default:
    ASSERT(0);
    break;
  }

  // detect algorithm hiding
  //
  if ( ret == SMESH_Hypothesis::HYP_OK &&
       ( event == ADD_ALGO || event == ADD_FATHER_ALGO ) &&
       algo->GetName() == anHyp->GetName() )
  {
    // is algo hidden?
    SMESH_Gen* gen = _father->GetGen();
    TopTools_ListIteratorOfListOfShape it( _father->GetAncestors( _subShape ));
    for ( ; ( ret == SMESH_Hypothesis::HYP_OK && it.More()); it.Next() ) {
      if ( SMESH_Algo* upperAlgo = gen->GetAlgo( *_father, it.Value() ))
        if ( !upperAlgo->NeedDescretBoundary() && !upperAlgo->SupportSubmeshes())
          ret = SMESH_Hypothesis::HYP_HIDDEN_ALGO;
    }
    // is algo hiding?
    if ( ret == SMESH_Hypothesis::HYP_OK &&
         !algo->NeedDescretBoundary()    &&
         !algo->SupportSubmeshes()) {
      map<int, SMESH_subMesh*>::reverse_iterator i_sm = _mapDepend.rbegin();
      for ( ; ( ret == SMESH_Hypothesis::HYP_OK && i_sm != _mapDepend.rend()) ; ++i_sm )
        if ( gen->GetAlgo( *_father, i_sm->second->_subShape ))
          ret = SMESH_Hypothesis::HYP_HIDING_ALGO;
    }
  }

  bool stateChange = ( _algoState != oldAlgoState );

  if ( stateChange && _algoState == HYP_OK ) // hyp becomes OK
    algo->SetEventListener( this );

  NotifyListenersOnEvent( event, ALGO_EVENT, anHyp );

  if ( stateChange && oldAlgoState == HYP_OK ) { // hyp becomes KO
    DeleteOwnListeners();
    SetIsAlwaysComputed( false );
    if (_subShape.ShapeType() == TopAbs_VERTEX ) {
      // restore default states
      _algoState = HYP_OK;
      _computeState = READY_TO_COMPUTE;
    }
  }

  if ( needFullClean ) {
    // added or removed algo is all-dimensional
    ComputeStateEngine( CLEAN );
    CleanDependsOn();
    ComputeSubMeshStateEngine( CHECK_COMPUTE_STATE );
  }

  if (stateChange || modifiedHyp)
    ComputeStateEngine(MODIF_ALGO_STATE);

  return ret;
}

//=======================================================================
//function : IsConform
//purpose  : check if a conform mesh will be produced by the Algo
//=======================================================================

bool SMESH_subMesh::IsConform(const SMESH_Algo* theAlgo)
{
//  MESSAGE( "SMESH_subMesh::IsConform" );
  if ( !theAlgo ) return false;

  // Suppose that theAlgo is applicable to _subShape, do not check it here
  //if ( !IsApplicableHypotesis( theAlgo )) return false;

  // check only algo that doesn't NeedDescretBoundary(): because mesh made
  // on a sub-shape will be ignored by theAlgo
  if ( theAlgo->NeedDescretBoundary() ||
       !theAlgo->OnlyUnaryInput() ) // all adjacent shapes will be meshed by this algo?
    return true;

  SMESH_Gen* gen =_father->GetGen();

  // only local algo is to be checked
  //if ( gen->IsGlobalHypothesis( theAlgo, *_father ))
  if ( _subShape.ShapeType() == _father->GetMeshDS()->ShapeToMesh().ShapeType() )
    return true;

  // check algo attached to adjacent shapes

  // loop on one level down sub-meshes
  TopoDS_Iterator itsub( _subShape );
  for (; itsub.More(); itsub.Next())
  {
    // loop on adjacent subShapes
    TopTools_ListIteratorOfListOfShape it( _father->GetAncestors( itsub.Value() ));
    for (; it.More(); it.Next())
    {
      const TopoDS_Shape& adjacent = it.Value();
      if ( _subShape.IsSame( adjacent )) continue;
      if ( adjacent.ShapeType() != _subShape.ShapeType())
        break;

      // check algo attached to smAdjacent
      SMESH_Algo * algo = gen->GetAlgo((*_father), adjacent);
      if (algo &&
          !algo->NeedDescretBoundary() &&
          algo->OnlyUnaryInput())
        return false; // NOT CONFORM MESH WILL BE PRODUCED
    }
  }

  return true;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

void SMESH_subMesh::SetAlgoState(int state)
{
  _algoState = state;
}

//=============================================================================
/*!
 *
 */
//=============================================================================
SMESH_Hypothesis::Hypothesis_Status
  SMESH_subMesh::SubMeshesAlgoStateEngine(int event,
                                          SMESH_Hypothesis * anHyp)
{
  SMESH_Hypothesis::Hypothesis_Status ret = SMESH_Hypothesis::HYP_OK;
  //EAP: a wire (dim==1) should notify edges (dim==1)
  //EAP: int dim = SMESH_Gen::GetShapeDim(_subShape);
  //if (_subShape.ShapeType() < TopAbs_EDGE ) // wire,face etc
  {
    SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,false);
    while ( smIt->more() ) {
      SMESH_Hypothesis::Hypothesis_Status ret2 =
        smIt->next()->AlgoStateEngine(event, anHyp);
      if ( ret2 > ret )
        ret = ret2;
    }
  }
  return ret;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

void SMESH_subMesh::CleanDependsOn()
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,false);
  while ( smIt->more() )
    smIt->next()->ComputeStateEngine(CLEAN);
}

//=============================================================================
/*!
 *
 */
//=============================================================================

void SMESH_subMesh::DumpAlgoState(bool isMain)
{
	int dim = SMESH_Gen::GetShapeDim(_subShape);
//   if (dim < 1) return;
	if (isMain)
	{
		const map < int, SMESH_subMesh * >&subMeshes = DependsOn();

		map < int, SMESH_subMesh * >::const_iterator itsub;
		for (itsub = subMeshes.begin(); itsub != subMeshes.end(); itsub++)
		{
			SMESH_subMesh *sm = (*itsub).second;
			sm->DumpAlgoState(false);
		}
	}
	int type = _subShape.ShapeType();
	MESSAGE("dim = " << dim << " type of shape " << type);
	switch (_algoState)
	{
	case NO_ALGO:
		MESSAGE(" AlgoState = NO_ALGO");
		break;
	case MISSING_HYP:
		MESSAGE(" AlgoState = MISSING_HYP");
		break;
	case HYP_OK:
		MESSAGE(" AlgoState = HYP_OK");
		break;
	}
	switch (_computeState)
	{
	case NOT_READY:
		MESSAGE(" ComputeState = NOT_READY");
		break;
	case READY_TO_COMPUTE:
		MESSAGE(" ComputeState = READY_TO_COMPUTE");
		break;
	case COMPUTE_OK:
		MESSAGE(" ComputeState = COMPUTE_OK");
		break;
	case FAILED_TO_COMPUTE:
		MESSAGE(" ComputeState = FAILED_TO_COMPUTE");
		break;
	}
}

//================================================================================
/*!
 * \brief Remove nodes and elements bound to submesh
  * \param subMesh - submesh containing nodes and elements
 */
//================================================================================

static void cleanSubMesh( SMESH_subMesh * subMesh )
{
  if (subMesh) {
    if (SMESHDS_SubMesh * subMeshDS = subMesh->GetSubMeshDS()) {
      SMESHDS_Mesh * meshDS = subMesh->GetFather()->GetMeshDS();
      SMDS_ElemIteratorPtr ite = subMeshDS->GetElements();
      while (ite->more()) {
        const SMDS_MeshElement * elt = ite->next();
        //MESSAGE( " RM elt: "<<elt->GetID()<<" ( "<<elt->NbNodes()<<" )" );
        //meshDS->RemoveElement(elt);
        meshDS->RemoveFreeElement(elt, subMeshDS);
      }

      SMDS_NodeIteratorPtr itn = subMeshDS->GetNodes();
      while (itn->more()) {
        const SMDS_MeshNode * node = itn->next();
        //MESSAGE( " RM node: "<<node->GetID());
        if ( node->NbInverseElements() == 0 )
          meshDS->RemoveFreeNode(node, subMeshDS);
        else // for StdMeshers_CompositeSegment_1D: node in one submesh, edge in another
          meshDS->RemoveNode(node);
      }
    }
  }
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool SMESH_subMesh::ComputeStateEngine(int event)
{
  _computeError.reset();

  //MESSAGE("SMESH_subMesh::ComputeStateEngine");
  //SCRUTE(_computeState);
  //SCRUTE(event);

  if (_subShape.ShapeType() == TopAbs_VERTEX)
  {
    _computeState = READY_TO_COMPUTE;
    SMESHDS_SubMesh* smDS = GetSubMeshDS();
    if ( smDS && smDS->NbNodes() ) {
      if ( event == CLEAN ) {
        CleanDependants();
        cleanSubMesh( this );
      }
      else
        _computeState = COMPUTE_OK;
    }
    else if ( event == COMPUTE && !_alwaysComputed ) {
      const TopoDS_Vertex & V = TopoDS::Vertex( _subShape );
      gp_Pnt P = BRep_Tool::Pnt(V);
      if ( SMDS_MeshNode * n = _father->GetMeshDS()->AddNode(P.X(), P.Y(), P.Z()) ) {
        _father->GetMeshDS()->SetNodeOnVertex(n,_Id);
        _computeState = COMPUTE_OK;
      }
    }
    if ( event == MODIF_ALGO_STATE )
      CleanDependants();
    return true;
  }
  SMESH_Gen *gen = _father->GetGen();
  SMESH_Algo *algo = 0;
  bool ret = true;
  SMESH_Hypothesis::Hypothesis_Status hyp_status;
  //algo_state oldAlgoState = (algo_state) GetAlgoState();

  switch (_computeState)
  {

    // ----------------------------------------------------------------------

  case NOT_READY:
    switch (event)
    {
    case MODIF_ALGO_STATE:
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo && !algo->NeedDescretBoundary())
        CleanDependsOn(); // clean sub-meshes with event CLEAN
      if ( _algoState == HYP_OK )
        _computeState = READY_TO_COMPUTE;
      break;
    case COMPUTE:		// nothing to do
      break;
    case CLEAN:
      CleanDependants();
      RemoveSubMeshElementsAndNodes();
      break;
    case SUBMESH_COMPUTED:	// nothing to do
      break;
    case SUBMESH_RESTORED:
      ComputeSubMeshStateEngine( SUBMESH_RESTORED );
      break;
    case MESH_ENTITY_REMOVED:
      break;
    case CHECK_COMPUTE_STATE:
      if ( IsMeshComputed() )
        _computeState = COMPUTE_OK;
      break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case READY_TO_COMPUTE:
    switch (event)
    {
    case MODIF_ALGO_STATE:
      _computeState = NOT_READY;
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo)
      {
        if (!algo->NeedDescretBoundary())
          CleanDependsOn(); // clean sub-meshes with event CLEAN
        if ( _algoState == HYP_OK )
          _computeState = READY_TO_COMPUTE;
      }
      break;
    case COMPUTE:
      {
        algo = gen->GetAlgo((*_father), _subShape);
        ASSERT(algo);
        ret = algo->CheckHypothesis((*_father), _subShape, hyp_status);
        if (!ret)
        {
          MESSAGE("***** verify compute state *****");
          _computeState = NOT_READY;
          SetAlgoState(MISSING_HYP);
          break;
        }
        TopoDS_Shape shape = _subShape;
        // check submeshes needed
        if (_father->HasShapeToMesh() ) {
          bool subComputed = false;
          if (!algo->OnlyUnaryInput())
            shape = GetCollection( gen, algo, subComputed );
          else
            subComputed = SubMeshesComputed();
          ret = ( algo->NeedDescretBoundary() ? subComputed :
                  algo->SupportSubmeshes() ? true :
                  ( !subComputed || _father->IsNotConformAllowed() ));
          if (!ret) {
            _computeState = FAILED_TO_COMPUTE;
            if ( !algo->NeedDescretBoundary() )
              _computeError =
                SMESH_ComputeError::New(COMPERR_BAD_INPUT_MESH,
                                        "Unexpected computed submesh",algo);
            break;
          }
        }
        // compute
//         CleanDependants(); for "UseExisting_*D" algos
//         RemoveSubMeshElementsAndNodes();
        ret = false;
        _computeState = FAILED_TO_COMPUTE;
        _computeError = SMESH_ComputeError::New(COMPERR_OK,"",algo);
        try {
#if (OCC_VERSION_MAJOR << 16 | OCC_VERSION_MINOR << 8 | OCC_VERSION_MAINTENANCE) > 0x060100
          OCC_CATCH_SIGNALS;
#endif
          algo->InitComputeError();
          MemoryReserve aMemoryReserve;
          SMDS_Mesh::CheckMemory();
          if ( !_father->HasShapeToMesh() ) // no shape
          {
            SMESH_MesherHelper helper( *_father );
            helper.SetSubShape( shape );
            helper.SetElementsOnShape( true );
            ret = algo->Compute(*_father, &helper );
          }
          else
          {
            ret = algo->Compute((*_father), shape);
          }
          if ( !_computeError || ( !ret && _computeError->IsOK() ) ) // algo can set _computeError of submesh
            _computeError = algo->GetComputeError();
        }
        catch ( std::bad_alloc& exc ) {
          MESSAGE("std::bad_alloc thrown inside algo->Compute()");
          if ( _computeError ) {
            _computeError->myName = COMPERR_MEMORY_PB;
            //_computeError->myComment = exc.what();
          }
          cleanSubMesh( this );
          throw exc;
        }
        catch ( Standard_OutOfMemory& exc ) {
          MESSAGE("Standard_OutOfMemory thrown inside algo->Compute()");
          if ( _computeError ) {
            _computeError->myName = COMPERR_MEMORY_PB;
            //_computeError->myComment = exc.what();
          }
          cleanSubMesh( this );
          throw std::bad_alloc();
        }
        catch (Standard_Failure& ex) {
          if ( !_computeError ) _computeError = SMESH_ComputeError::New();
          _computeError->myName    = COMPERR_OCC_EXCEPTION;
          _computeError->myComment += ex.DynamicType()->Name();
          if ( ex.GetMessageString() && strlen( ex.GetMessageString() )) {
            _computeError->myComment += ": ";
            _computeError->myComment += ex.GetMessageString();
          }
        }
        catch ( SMESH_Exception& S_ex ) {
          if ( !_computeError ) _computeError = SMESH_ComputeError::New();
          _computeError->myName    = COMPERR_SLM_EXCEPTION;
          _computeError->myComment = S_ex.what();
        }
        catch ( std::exception& exc ) {
          if ( !_computeError ) _computeError = SMESH_ComputeError::New();
          _computeError->myName    = COMPERR_STD_EXCEPTION;
          _computeError->myComment = exc.what();
        }
        catch ( ... ) {
          if ( _computeError )
            _computeError->myName = COMPERR_EXCEPTION;
          else
            ret = false;
        }
        if (ret && !_alwaysComputed && shape == _subShape) { // check if anything was built
          ret = ( GetSubMeshDS() && ( GetSubMeshDS()->NbElements() || GetSubMeshDS()->NbNodes() ));
        }
        bool isComputeErrorSet = !CheckComputeError( algo, shape );
        if (!ret && !isComputeErrorSet)
        {
          // Set _computeError
          if ( !_computeError )
            _computeError = SMESH_ComputeError::New();
          if ( _computeError->IsOK() )
            _computeError->myName = COMPERR_ALGO_FAILED;
          _computeState = FAILED_TO_COMPUTE;
        }
        if (ret)
        {
          _computeError.reset();
        }
        UpdateDependantsState( SUBMESH_COMPUTED ); // send event SUBMESH_COMPUTED
      }
      break;
    case CLEAN:
      CleanDependants();
      RemoveSubMeshElementsAndNodes();
      _computeState = NOT_READY;
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo)
      {
        ret = algo->CheckHypothesis((*_father), _subShape, hyp_status);
        if (ret)
          _computeState = READY_TO_COMPUTE;
        else
          SetAlgoState(MISSING_HYP);
      }
      break;
    case SUBMESH_COMPUTED:      // nothing to do
      break;
    case SUBMESH_RESTORED:
      // check if a mesh is already computed that may
      // happen after retrieval from a file
      ComputeStateEngine( CHECK_COMPUTE_STATE );
      ComputeSubMeshStateEngine( SUBMESH_RESTORED );
      algo = gen->GetAlgo(*_father, _subShape);
      if (algo) algo->SubmeshRestored( this );
      break;
    case MESH_ENTITY_REMOVED:
      break;
    case CHECK_COMPUTE_STATE:
      if ( IsMeshComputed() )
        _computeState = COMPUTE_OK;
      break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case COMPUTE_OK:
    switch (event)
    {
    case MODIF_ALGO_STATE:
      ComputeStateEngine( CLEAN );
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo && !algo->NeedDescretBoundary())
        CleanDependsOn(); // clean sub-meshes with event CLEAN
      break;
    case COMPUTE:               // nothing to do
      break;
    case CLEAN:
      CleanDependants();  // clean sub-meshes, dependant on this one, with event CLEAN
      RemoveSubMeshElementsAndNodes();
      _computeState = NOT_READY;
      if ( _algoState == HYP_OK )
        _computeState = READY_TO_COMPUTE;
      break;
    case SUBMESH_COMPUTED:      // nothing to do
      break;
    case SUBMESH_RESTORED:
      ComputeStateEngine( CHECK_COMPUTE_STATE );
      ComputeSubMeshStateEngine( SUBMESH_RESTORED );
      algo = gen->GetAlgo(*_father, _subShape);
      if (algo) algo->SubmeshRestored( this );
      break;
    case MESH_ENTITY_REMOVED:
      UpdateDependantsState( CHECK_COMPUTE_STATE );
      ComputeStateEngine( CHECK_COMPUTE_STATE );
      ComputeSubMeshStateEngine( CHECK_COMPUTE_STATE );
      break;
    case CHECK_COMPUTE_STATE:
      if ( !IsMeshComputed() )
        if (_algoState == HYP_OK)
          _computeState = READY_TO_COMPUTE;
        else
          _computeState = NOT_READY;
      break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case FAILED_TO_COMPUTE:
    switch (event)
    {
    case MODIF_ALGO_STATE:
      algo = gen->GetAlgo((*_father), _subShape);
      if (algo && !algo->NeedDescretBoundary())
        CleanDependsOn(); // clean sub-meshes with event CLEAN
      if (_algoState == HYP_OK)
        _computeState = READY_TO_COMPUTE;
      else
        _computeState = NOT_READY;
      break;
    case COMPUTE:      // nothing to do
      break;
    case CLEAN:
      CleanDependants(); // submeshes dependent on me should be cleaned as well
      RemoveSubMeshElementsAndNodes();
      break;
    case SUBMESH_COMPUTED:      // allow retry compute
      if (_algoState == HYP_OK)
        _computeState = READY_TO_COMPUTE;
      else
        _computeState = NOT_READY;
      break;
    case SUBMESH_RESTORED:
      ComputeSubMeshStateEngine( SUBMESH_RESTORED );
      break;
    case MESH_ENTITY_REMOVED:
      break;
    case CHECK_COMPUTE_STATE:
      if ( IsMeshComputed() )
        _computeState = COMPUTE_OK;
      else
        if (_algoState == HYP_OK)
          _computeState = READY_TO_COMPUTE;
        else
          _computeState = NOT_READY;
      break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------
  default:
    ASSERT(0);
    break;
  }

  NotifyListenersOnEvent( event, COMPUTE_EVENT );

  return ret;
}

//=======================================================================
/*!
 * \brief Update compute_state by _computeError and send proper events to
 * dependent submeshes
  * \retval bool - true if _computeError is NOT set
 */
//=======================================================================

bool SMESH_subMesh::CheckComputeError(SMESH_Algo* theAlgo, const TopoDS_Shape& theShape)
{
  bool noErrors = true;

  if ( !theShape.IsNull() )
  {
    // Check state of submeshes
    if ( !theAlgo->NeedDescretBoundary())
    {
      SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,false);
      while ( smIt->more() )
        if ( !smIt->next()->CheckComputeError( theAlgo ))
          noErrors = false;
    }

    // Check state of neighbours
    if ( !theAlgo->OnlyUnaryInput() &&
         theShape.ShapeType() == TopAbs_COMPOUND &&
         !theShape.IsSame( _subShape ))
    {
      for (TopoDS_Iterator subIt( theShape ); subIt.More(); subIt.Next()) {
        SMESH_subMesh* sm = _father->GetSubMesh( subIt.Value() );
        if ( sm != this ) {
          if ( !sm->CheckComputeError( theAlgo, sm->GetSubShape() ))
            noErrors = false;
          UpdateDependantsState( SUBMESH_COMPUTED ); // send event SUBMESH_COMPUTED
        }
      }
    }
  }
  {
    // Check my state
    if ( !_computeError || _computeError->IsOK() )
    {
      _computeState = COMPUTE_OK;
    }
    else
    {
      if ( !_computeError->myAlgo )
        _computeError->myAlgo = theAlgo;

      // Show error
      SMESH_Comment text;
      text << theAlgo->GetName() << " failed on subshape #" << _Id << " with error ";
      if (_computeError->IsCommon() )
        text << _computeError->CommonName();
      else
        text << _computeError->myName;
      if ( _computeError->myComment.size() > 0 )
        text << " \"" << _computeError->myComment << "\"";

#ifdef _DEBUG_
      MESSAGE_BEGIN ( text );
      // Show vertices location of a failed shape
      TopTools_IndexedMapOfShape vMap;
      TopExp::MapShapes( _subShape, TopAbs_VERTEX, vMap );
      MESSAGE_ADD ( "Subshape vertices " << ( vMap.Extent()>10 ? "(first 10):" : ":") );
      for ( int iv = 1; iv <= vMap.Extent() && iv < 11; ++iv ) {
        gp_Pnt P( BRep_Tool::Pnt( TopoDS::Vertex( vMap( iv ) )));
        MESSAGE_ADD ( "#" << _father->GetMeshDS()->ShapeToIndex( vMap( iv )) << " "
                   << P.X() << " " << P.Y() << " " << P.Z() << " " );
      }
#else
      INFOS( text );
#endif
      _computeState = FAILED_TO_COMPUTE;
      noErrors = false;
    }
  }
  return noErrors;
}

//=======================================================================
//function : ApplyToCollection
//purpose  : Apply theAlgo to all subshapes in theCollection
//=======================================================================

bool SMESH_subMesh::ApplyToCollection (SMESH_Algo*         theAlgo,
                                       const TopoDS_Shape& theCollection)
{
  MESSAGE("SMESH_subMesh::ApplyToCollection");
  ASSERT ( !theAlgo->NeedDescretBoundary() );

  if ( _computeError )
    _computeError->myName = COMPERR_OK;

  bool ok = theAlgo->Compute( *_father, theCollection );

  // set _computeState of subshapes
  TopExp_Explorer anExplorer( theCollection, _subShape.ShapeType() );
  for ( ; anExplorer.More(); anExplorer.Next() )
  {
    if ( SMESH_subMesh* subMesh = _father->GetSubMeshContaining( anExplorer.Current() ))
    {
      bool localOK = subMesh->CheckComputeError( theAlgo );
      if ( !ok && localOK && !subMesh->IsMeshComputed() )
      {
        subMesh->_computeError = theAlgo->GetComputeError();
        if ( subMesh->_computeError->IsOK() )
          _computeError = SMESH_ComputeError::New(COMPERR_ALGO_FAILED);
        localOK = CheckComputeError( theAlgo );
      }
      if ( localOK )
        subMesh->UpdateDependantsState( SUBMESH_COMPUTED );
      subMesh->UpdateSubMeshState( localOK ? COMPUTE_OK : FAILED_TO_COMPUTE );
    }
  }

  return true;
}


//=======================================================================
//function : UpdateSubMeshState
//purpose  :
//=======================================================================

void SMESH_subMesh::UpdateSubMeshState(const compute_state theState)
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,false);
  while ( smIt->more() )
    smIt->next()->_computeState = theState;
}

//=======================================================================
//function : ComputeSubMeshStateEngine
//purpose  :
//=======================================================================

void SMESH_subMesh::ComputeSubMeshStateEngine(int event)
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,false);
  while ( smIt->more() )
    smIt->next()->ComputeStateEngine(event);
}

//=======================================================================
//function : UpdateDependantsState
//purpose  :
//=======================================================================

void SMESH_subMesh::UpdateDependantsState(const compute_event theEvent)
{
  //MESSAGE("SMESH_subMesh::UpdateDependantsState");
  TopTools_ListIteratorOfListOfShape it( _father->GetAncestors( _subShape ));
  for (; it.More(); it.Next())
  {
    const TopoDS_Shape& ancestor = it.Value();
    SMESH_subMesh *aSubMesh =
      _father->GetSubMeshContaining(ancestor);
    if (aSubMesh)
      aSubMesh->ComputeStateEngine( theEvent );
  }
}

//=============================================================================
/*!
 *
 */
//=============================================================================

void SMESH_subMesh::CleanDependants()
{
  int dimToClean = SMESH_Gen::GetShapeDim( _subShape ) + 1;

  TopTools_ListIteratorOfListOfShape it( _father->GetAncestors( _subShape ));
  for (; it.More(); it.Next())
  {
    const TopoDS_Shape& ancestor = it.Value();
    if ( SMESH_Gen::GetShapeDim( ancestor ) == dimToClean ) {
      // PAL8021. do not go upper than SOLID, else ComputeStateEngine(CLEAN)
      // will erase mesh on other shapes in a compound
      if ( ancestor.ShapeType() >= TopAbs_SOLID ) {
        SMESH_subMesh *aSubMesh = _father->GetSubMeshContaining(ancestor);
        if (aSubMesh)
          aSubMesh->ComputeStateEngine(CLEAN);
      }
    }
  }
}

//=============================================================================
/*!
 *
 */
//=============================================================================

void SMESH_subMesh::RemoveSubMeshElementsAndNodes()
{
  //SCRUTE(_subShape.ShapeType());

  cleanSubMesh( this );

  // algo may bind a submesh not to _subShape, eg 3D algo
  // sets nodes on SHELL while _subShape may be SOLID

  int dim = SMESH_Gen::GetShapeDim( _subShape );
  int type = _subShape.ShapeType() + 1;
  for ( ; type <= TopAbs_EDGE; type++) {
    if ( dim == SMESH_Gen::GetShapeDim( (TopAbs_ShapeEnum) type ))
    {
      TopExp_Explorer exp( _subShape, (TopAbs_ShapeEnum) type );
      for ( ; exp.More(); exp.Next() )
        cleanSubMesh( _father->GetSubMeshContaining( exp.Current() ));
    }
    else
      break;
  }
}

//=======================================================================
//function : GetCollection
//purpose  : return a shape containing all sub-shapes of the MainShape that can be
//           meshed at once along with _subShape
//=======================================================================

TopoDS_Shape SMESH_subMesh::GetCollection(SMESH_Gen * theGen,
                                          SMESH_Algo* theAlgo,
                                          bool &      theSubComputed)
{
  MESSAGE("SMESH_subMesh::GetCollection");

  theSubComputed = SubMeshesComputed();

  TopoDS_Shape mainShape = _father->GetMeshDS()->ShapeToMesh();

  if ( mainShape.IsSame( _subShape ))
    return _subShape;

  const bool ignoreAuxiliaryHyps = false;
  list<const SMESHDS_Hypothesis*> aUsedHyp =
    theAlgo->GetUsedHypothesis( *_father, _subShape, ignoreAuxiliaryHyps ); // copy

  // put in a compound all shapes with the same hypothesis assigned
  // and a good ComputState

  TopoDS_Compound aCompound;
  BRep_Builder aBuilder;
  aBuilder.MakeCompound( aCompound );

  TopExp_Explorer anExplorer( mainShape, _subShape.ShapeType() );
  for ( ; anExplorer.More(); anExplorer.Next() )
  {
    const TopoDS_Shape& S = anExplorer.Current();
    SMESH_subMesh* subMesh = _father->GetSubMesh( S );
    if ( subMesh == this )
    {
      aBuilder.Add( aCompound, S );
    }
    else if ( subMesh->GetComputeState() == READY_TO_COMPUTE )
    {
      SMESH_Algo* anAlgo = theGen->GetAlgo( *_father, S );
      if (strcmp( anAlgo->GetName(), theAlgo->GetName()) == 0 && // same algo
          anAlgo->GetUsedHypothesis( *_father, S, ignoreAuxiliaryHyps ) == aUsedHyp) // same hyps
        aBuilder.Add( aCompound, S );
      if ( !subMesh->SubMeshesComputed() )
        theSubComputed = false;
    }
  }

  return aCompound;
}

//=======================================================================
//function : GetSimilarAttached
//purpose  : return a hypothesis attached to theShape.
//           If theHyp is provided, similar but not same hypotheses
//           is returned; else only applicable ones having theHypType
//           is returned
//=======================================================================

const SMESH_Hypothesis* SMESH_subMesh::GetSimilarAttached(const TopoDS_Shape&      theShape,
                                                          const SMESH_Hypothesis * theHyp,
                                                          const int                theHypType)
{
  SMESH_HypoFilter hypoKind;
  hypoKind.Init( hypoKind.HasType( theHyp ? theHyp->GetType() : theHypType ));
  if ( theHyp ) {
    hypoKind.And   ( hypoKind.HasDim( theHyp->GetDim() ));
    hypoKind.AndNot( hypoKind.Is( theHyp ));
    if ( theHyp->IsAuxiliary() )
      hypoKind.And( hypoKind.HasName( theHyp->GetName() ));
    else
      hypoKind.AndNot( hypoKind.IsAuxiliary());
  }
  else {
    hypoKind.And( hypoKind.IsApplicableTo( theShape ));
  }

  return _father->GetHypothesis( theShape, hypoKind, false );
}

//=======================================================================
//function : CheckConcurentHypothesis
//purpose  : check if there are several applicable hypothesis attached to
//           ancestors
//=======================================================================

SMESH_Hypothesis::Hypothesis_Status
  SMESH_subMesh::CheckConcurentHypothesis (const int theHypType)
{
  MESSAGE ("SMESH_subMesh::CheckConcurentHypothesis");

  // is there local hypothesis on me?
  if ( GetSimilarAttached( _subShape, 0, theHypType ) )
    return SMESH_Hypothesis::HYP_OK;


  TopoDS_Shape aPrevWithHyp;
  const SMESH_Hypothesis* aPrevHyp = 0;
  TopTools_ListIteratorOfListOfShape it( _father->GetAncestors( _subShape ));
  for (; it.More(); it.Next())
  {
    const TopoDS_Shape& ancestor = it.Value();
    const SMESH_Hypothesis* hyp = GetSimilarAttached( ancestor, 0, theHypType );
    if ( hyp )
    {
      if ( aPrevWithHyp.IsNull() || aPrevWithHyp.IsSame( ancestor ))
      {
        aPrevWithHyp = ancestor;
        aPrevHyp     = hyp;
      }
      else if ( aPrevWithHyp.ShapeType() == ancestor.ShapeType() && aPrevHyp != hyp )
        return SMESH_Hypothesis::HYP_CONCURENT;
      else
        return SMESH_Hypothesis::HYP_OK;
    }
  }
  return SMESH_Hypothesis::HYP_OK;
}

//================================================================================
/*!
 * \brief Sets an event listener and its data to a submesh
 * \param listener - the listener to store
 * \param data - the listener data to store
 * \param where - the submesh to store the listener and it's data
 * \param deleteListener - if true then the listener will be deleted as
 *        it is removed from where submesh
 * 
 * It remembers the submesh where it puts the listener in order to delete
 * them when HYP_OK algo_state is lost
 * After being set, event listener is notified on each event of where submesh.
 */
//================================================================================

void SMESH_subMesh::SetEventListener(EventListener*     listener,
                                     EventListenerData* data,
                                     SMESH_subMesh*     where)
{
  if ( listener && where ) {
    where->SetEventListener( listener, data );
    myOwnListeners.push_back( make_pair( where, listener ));
  }
}

//================================================================================
/*!
 * \brief Sets an event listener and its data to a submesh
 * \param listener - the listener to store
 * \param data - the listener data to store
 * 
 * After being set, event listener is notified on each event of a submesh.
 */
//================================================================================

void SMESH_subMesh::SetEventListener(EventListener* listener, EventListenerData* data)
{
  map< EventListener*, EventListenerData* >::iterator l_d =
    myEventListeners.find( listener );
  if ( l_d != myEventListeners.end() ) {
    EventListenerData* curData = l_d->second;
    if ( curData && curData != data && curData->IsDeletable() )
      delete curData;
    l_d->second = data;
  }
  else 
    myEventListeners.insert( make_pair( listener, data ));
}

//================================================================================
/*!
 * \brief Return an event listener data
 * \param listener - the listener whose data is
 * \retval EventListenerData* - found data, maybe NULL
 */
//================================================================================

EventListenerData* SMESH_subMesh::GetEventListenerData(EventListener* listener) const
{
  map< EventListener*, EventListenerData* >::const_iterator l_d =
    myEventListeners.find( listener );
  if ( l_d != myEventListeners.end() )
    return l_d->second;
  return 0;
}

//================================================================================
/*!
 * \brief Notify stored event listeners on the occured event
 * \param event - algo_event or compute_event itself
 * \param eventType - algo_event or compute_event
 * \param subMesh - the submesh where the event occures
 * \param data - listener data stored in the subMesh
 * \param hyp - hypothesis, if eventType is algo_event
 */
//================================================================================

void SMESH_subMesh::NotifyListenersOnEvent( const int         event,
                                            const event_type  eventType,
                                            SMESH_Hypothesis* hyp)
{
  map< EventListener*, EventListenerData* >::iterator l_d = myEventListeners.begin();
  for ( ; l_d != myEventListeners.end(); ++l_d )
    l_d->first->ProcessEvent( event, eventType, this, l_d->second, hyp );
}

//================================================================================
/*!
 * \brief Unregister the listener and delete listener's data
 * \param listener - the event listener
 */
//================================================================================

void SMESH_subMesh::DeleteEventListener(EventListener* listener)
{
  map< EventListener*, EventListenerData* >::iterator l_d =
    myEventListeners.find( listener );
  if ( l_d != myEventListeners.end() ) {
    if ( l_d->first  && l_d->first->IsDeletable() )  delete l_d->first;
    if ( l_d->second && l_d->second->IsDeletable() ) delete l_d->second;
    myEventListeners.erase( l_d );
  }
}

//================================================================================
/*!
 * \brief Delete event listeners depending on algo of this submesh
 */
//================================================================================

void SMESH_subMesh::DeleteOwnListeners()
{
  list< pair< SMESH_subMesh*, EventListener* > >::iterator sm_l;
  for ( sm_l = myOwnListeners.begin(); sm_l != myOwnListeners.end(); ++sm_l)
    sm_l->first->DeleteEventListener( sm_l->second );
  myOwnListeners.clear();
}

//================================================================================
/*!
 * \brief Do something on a certain event
 * \param event - algo_event or compute_event itself
 * \param eventType - algo_event or compute_event
 * \param subMesh - the submesh where the event occures
 * \param data - listener data stored in the subMesh
 * \param hyp - hypothesis, if eventType is algo_event
 * 
 * The base implementation translates CLEAN event to the subMesh
 * stored in listener data. Also it sends SUBMESH_COMPUTED event in case of
 * successful COMPUTE event.
 */
//================================================================================

void SMESH_subMeshEventListener::ProcessEvent(const int          event,
                                              const int          eventType,
                                              SMESH_subMesh*     subMesh,
                                              EventListenerData* data,
                                              const SMESH_Hypothesis*  /*hyp*/)
{
  if ( data && !data->mySubMeshes.empty() &&
       eventType == SMESH_subMesh::COMPUTE_EVENT)
  {
    ASSERT( data->mySubMeshes.front() != subMesh );
    list<SMESH_subMesh*>::iterator smIt = data->mySubMeshes.begin();
    list<SMESH_subMesh*>::iterator smEnd = data->mySubMeshes.end();
    switch ( event ) {
    case SMESH_subMesh::CLEAN:
      for ( ; smIt != smEnd; ++ smIt)
        (*smIt)->ComputeStateEngine( event );
      break;
    case SMESH_subMesh::COMPUTE:
      if ( subMesh->GetComputeState() == SMESH_subMesh::COMPUTE_OK )
        for ( ; smIt != smEnd; ++ smIt)
          (*smIt)->ComputeStateEngine( SMESH_subMesh::SUBMESH_COMPUTED );
      break;
    default:;
    }
  }
}

namespace {

  //================================================================================
  /*!
   * \brief Iterator over submeshes and optionally prepended or appended one
   */
  //================================================================================

  struct _Iterator : public SMDS_Iterator<SMESH_subMesh*>
  {
    _Iterator(SMDS_Iterator<SMESH_subMesh*>* subIt,
              SMESH_subMesh*                 prepend,
              SMESH_subMesh*                 append): myIt(subIt),myAppend(append)
    {
      myCur = prepend ? prepend : myIt->more() ? myIt->next() : append;
      if ( myCur == append ) append = 0;
    }
    /// Return true if and only if there are other object in this iterator
    virtual bool more()
    {
      return myCur;
    }
    /// Return the current object and step to the next one
    virtual SMESH_subMesh* next()
    {
      SMESH_subMesh* res = myCur;
      if ( myIt->more() ) { myCur = myIt->next(); }
      else                { myCur = myAppend; myAppend = 0; }
      return res;
    }
    /// ~
    ~_Iterator()
    { delete myIt; }
    ///
    SMESH_subMesh                 *myAppend, *myCur;
    SMDS_Iterator<SMESH_subMesh*> *myIt;
  };
}

//================================================================================
/*!
 * \brief  Return iterator on the submeshes this one depends on
  * \param includeSelf - this submesh to be returned also
  * \param reverse - if true, complex shape submeshes go first
 */
//================================================================================

SMESH_subMeshIteratorPtr SMESH_subMesh::getDependsOnIterator(const bool includeSelf,
                                                             const bool reverse)
{
  SMESH_subMesh *prepend=0, *append=0;
  if ( includeSelf ) {
    if ( reverse ) prepend = this;
    else            append = this;
  }
  typedef map < int, SMESH_subMesh * > TMap;
  if ( reverse )
  {
    return SMESH_subMeshIteratorPtr
      ( new _Iterator( new SMDS_mapReverseIterator<TMap>( DependsOn() ), prepend, append ));
  }
  {
    return SMESH_subMeshIteratorPtr
      ( new _Iterator( new SMDS_mapIterator<TMap>( DependsOn() ), prepend, append ));
  }
}
