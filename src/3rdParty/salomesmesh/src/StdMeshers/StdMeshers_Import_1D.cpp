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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : StdMeshers_Import_1D.cxx
//  Module : SMESH
//
#include "StdMeshers_Import_1D.hxx"
#include "StdMeshers_ImportSource.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESHDS_Group.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_Group.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"

#include "Utils_SALOME_Exception.hxx"
#include "utilities.h"

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

using namespace std;

//=============================================================================
/*!
 * Creates StdMeshers_Import_1D
 */
//=============================================================================

StdMeshers_Import_1D::StdMeshers_Import_1D(int hypId, int studyId, SMESH_Gen * gen)
  :SMESH_1D_Algo(hypId, studyId, gen), _sourceHyp(0)
{
  MESSAGE("StdMeshers_Import_1D::StdMeshers_Import_1D");
  _name = "Import_1D";
  _shapeType = (1 << TopAbs_EDGE);

  _compatibleHypothesis.push_back("ImportSource1D");
}

//================================================================================
namespace // INTERNAL STUFF
//================================================================================
{
  int getSubmeshIDForCopiedMesh(const SMESHDS_Mesh* srcMeshDS, SMESH_Mesh* tgtMesh);

  enum _ListenerDataType
    {
      WAIT_HYP_MODIF=1, // data indicating awaiting for valid parameters of src hyp
      LISTEN_SRC_MESH, // data storing submesh depending on source mesh state
      SRC_HYP // data storing ImportSource hyp
    };
  //================================================================================
  /*!
   * \brief _ListenerData holding ImportSource hyp holding in its turn
   *  imported groups
   */
  struct _ListenerData : public SMESH_subMeshEventListenerData
  {
    const StdMeshers_ImportSource1D* _srcHyp;
    _ListenerData(const StdMeshers_ImportSource1D* h, _ListenerDataType type=SRC_HYP):
      SMESH_subMeshEventListenerData(/*isDeletable=*/true), _srcHyp(h)
    {
      myType = type;
    }
  };
  //================================================================================
  /*!
   * \brief Comparator of sub-meshes
   */
  struct _SubLess
  {
    bool operator()(const SMESH_subMesh* sm1, const SMESH_subMesh* sm2 ) const
    {
      if ( sm1 == sm2 ) return false;
      if ( !sm1 || !sm2 ) return sm1 < sm2;
      const TopoDS_Shape& s1 = sm1->GetSubShape();
      const TopoDS_Shape& s2 = sm2->GetSubShape();
      TopAbs_ShapeEnum t1 = s1.IsNull() ? TopAbs_SHAPE : s1.ShapeType();
      TopAbs_ShapeEnum t2 = s2.IsNull() ? TopAbs_SHAPE : s2.ShapeType();
      if ( t1 == t2)
        return (sm1 < sm2);
      return t1 < t2; // to have: face < edge
    }
  };
  //================================================================================
  /*!
   * \brief Container of data dedicated to one source mesh
   */
  struct _ImportData
  {
    const SMESH_Mesh* _srcMesh;
    StdMeshers_Import_1D::TNodeNodeMap _n2n;
    StdMeshers_Import_1D::TElemElemMap _e2e;

    set< SMESH_subMesh*, _SubLess > _subM; // submeshes relating to this srcMesh
    set< SMESH_subMesh*, _SubLess > _copyMeshSubM; // submeshes requesting mesh copying
    set< SMESH_subMesh*, _SubLess > _copyGroupSubM; // submeshes requesting group copying
    set< SMESH_subMesh*, _SubLess > _computedSubM;

    SMESHDS_SubMesh*     _importMeshSubDS; // submesh storing a copy of _srcMesh
    int                  _importMeshSubID; // id of _importMeshSubDS

    _ImportData(const SMESH_Mesh* srcMesh=0):
      _srcMesh(srcMesh), _importMeshSubDS(0),_importMeshSubID(-1) {}

    void removeImportedMesh( SMESHDS_Mesh* meshDS )
    {
      if ( !_importMeshSubDS ) return;
      SMDS_ElemIteratorPtr eIt = _importMeshSubDS->GetElements();
      while ( eIt->more() )
        meshDS->RemoveFreeElement( eIt->next(), 0, /*fromGroups=*/false );
      SMDS_NodeIteratorPtr nIt = _importMeshSubDS->GetNodes();
      while ( nIt->more() )
        meshDS->RemoveFreeNode( nIt->next(), 0, /*fromGroups=*/false );
      _importMeshSubDS->Clear();
      _n2n.clear();
      _e2e.clear();
    }
    void removeGroups( SMESH_subMesh* subM, const StdMeshers_ImportSource1D* srcHyp )
    {
      if ( !srcHyp ) return;
      SMESH_Mesh*           tgtMesh = subM->GetFather();
      const SMESHDS_Mesh* tgtMeshDS = tgtMesh->GetMeshDS();
      const SMESHDS_Mesh* srcMeshDS = _srcMesh->GetMeshDS();
      vector<SMESH_Group*>*  groups =
        const_cast<StdMeshers_ImportSource1D*>(srcHyp)->GetResultGroups(*srcMeshDS,*tgtMeshDS);
      if ( groups )
      {
        for ( unsigned i = 0; i < groups->size(); ++i )
          tgtMesh->RemoveGroup( groups->at(i)->GetGroupDS()->GetID() );
        groups->clear();
      }
    }
    void trackHypParams( SMESH_subMesh* sm, const StdMeshers_ImportSource1D* srcHyp )
    {
      if ( !srcHyp ) return;
      bool toCopyMesh, toCopyGroups;
      srcHyp->GetCopySourceMesh(toCopyMesh, toCopyGroups);

      if ( toCopyMesh )_copyMeshSubM.insert( sm );
      else             _copyMeshSubM.erase( sm );

      if ( toCopyGroups ) _copyGroupSubM.insert( sm );
      else                _copyGroupSubM.erase( sm );
    }
    void addComputed( SMESH_subMesh* sm )
    {
      SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(/*includeSelf=*/true,
                                                               /*complexShapeFirst=*/true);
      while ( smIt->more() )
      {
        sm = smIt->next();
        switch ( sm->GetSubShape().ShapeType() )
        {
        case TopAbs_EDGE:
          if ( SMESH_Algo::isDegenerated( TopoDS::Edge( sm->GetSubShape() )))
            continue;
          /* FALLTHRU */
        case TopAbs_FACE:
          _subM.insert( sm );
          if ( !sm->IsEmpty() )
            _computedSubM.insert( sm );
        case TopAbs_VERTEX:
          break;
        default:;
        }
      }
    }
  };
  //================================================================================
  /*!
   * Listener notified on events relating to imported submesh
   */
  class _Listener : public SMESH_subMeshEventListener
  {
    typedef map< SMESH_Mesh*, list< _ImportData > > TMesh2ImpData;
    TMesh2ImpData _tgtMesh2ImportData;

    _Listener():SMESH_subMeshEventListener(/*isDeletable=*/false,
                                           "StdMeshers_Import_1D::_Listener") {}

  public:
    // return poiter to a static listener
    static _Listener* get() { static _Listener theListener; return &theListener; }

    static _ImportData* getImportData(const SMESH_Mesh* srcMesh, SMESH_Mesh* tgtMesh);

    static void storeImportSubmesh(SMESH_subMesh*                   importSub,
                                   const SMESH_Mesh*                srcMesh,
                                   const StdMeshers_ImportSource1D* srcHyp);

    virtual void ProcessEvent(const int                       event,
                              const int                       eventType,
                              SMESH_subMesh*                  subMesh,
                              SMESH_subMeshEventListenerData* data,
                              const SMESH_Hypothesis*         hyp);
    void removeSubmesh( SMESH_subMesh* sm, _ListenerData* data );
    void clearSubmesh ( SMESH_subMesh* sm, _ListenerData* data, bool clearAllSub );
    void clearN2N     ( SMESH_Mesh* tgtMesh );

    // mark sm as missing src hyp with valid groups
    static void waitHypModification(SMESH_subMesh* sm)
    {
      sm->SetEventListener
        (get(), SMESH_subMeshEventListenerData::MakeData( sm, WAIT_HYP_MODIF ), sm);
    }
  };
  //--------------------------------------------------------------------------------
  /*!
   * \brief Find or create ImportData for given meshes
   */
  _ImportData* _Listener::getImportData(const SMESH_Mesh* srcMesh,
                                        SMESH_Mesh*       tgtMesh)
  {
    list< _ImportData >& dList = get()->_tgtMesh2ImportData[tgtMesh];
    list< _ImportData >::iterator d = dList.begin();
    for ( ; d != dList.end(); ++d )
      if ( d->_srcMesh == srcMesh )
        return &*d;
    dList.push_back(_ImportData(srcMesh));
    return &dList.back();
  }

  //--------------------------------------------------------------------------------
  /*!
   * \brief Remember an imported sub-mesh and set needed even listeners
   *  \param importSub - submesh computed by Import algo
   *  \param srcMesh - source mesh
   *  \param srcHyp - ImportSource hypothesis
   */
  void _Listener::storeImportSubmesh(SMESH_subMesh*                   importSub,
                                     const SMESH_Mesh*                srcMesh,
                                     const StdMeshers_ImportSource1D* srcHyp)
  {
    // set listener to hear events of the submesh computed by "Import" algo
    importSub->SetEventListener( get(), new _ListenerData(srcHyp), importSub );

    // set listeners to hear events of the source mesh
    SMESH_subMesh* smToNotify = importSub;
    vector<SMESH_subMesh*> smToListen = srcHyp->GetSourceSubMeshes( srcMesh );
    for ( size_t i = 0; i < smToListen.size(); ++i )
    {
      SMESH_subMeshEventListenerData* data = new _ListenerData(srcHyp, LISTEN_SRC_MESH);
      data->mySubMeshes.push_back( smToNotify );
      importSub->SetEventListener( get(), data, smToListen[i] );
    }
    // remember the submesh importSub and its sub-submeshes
    _ImportData* iData = _Listener::getImportData( srcMesh, importSub->GetFather());
    iData->trackHypParams( importSub, srcHyp );
    iData->addComputed( importSub );
    if ( !iData->_copyMeshSubM.empty() && iData->_importMeshSubID < 1 )
    {
      SMESH_Mesh* tgtMesh = importSub->GetFather();
      iData->_importMeshSubID = getSubmeshIDForCopiedMesh( srcMesh->GetMeshDS(),tgtMesh);
      iData->_importMeshSubDS = tgtMesh->GetMeshDS()->NewSubMesh( iData->_importMeshSubID );
    }
  }
  //--------------------------------------------------------------------------------
  /*!
   * \brief Remove imported mesh and/or groups if needed
   *  \param sm - submesh loosing Import algo
   *  \param data - data holding imported groups
   */
  void _Listener::removeSubmesh( SMESH_subMesh* sm, _ListenerData* data )
  {
    list< _ImportData > &  dList = _tgtMesh2ImportData[ sm->GetFather() ];
    list< _ImportData >::iterator d = dList.begin();
    for ( ; d != dList.end(); ++d )
      if ( (*d)._subM.erase( sm ))
      {
        d->_computedSubM.erase( sm );
        bool rmMesh   = d->_copyMeshSubM.erase( sm ) && d->_copyMeshSubM.empty();
        bool rmGroups = (d->_copyGroupSubM.erase( sm ) && d->_copyGroupSubM.empty()) || rmMesh;
        if ( rmMesh )
          d->removeImportedMesh( sm->GetFather()->GetMeshDS() );
        if ( rmGroups && data && data->myType == SRC_HYP )
          d->removeGroups( sm, data->_srcHyp );
      }
  }
  //--------------------------------------------------------------------------------
  /*!
   * \brief Clear _ImportData::_n2n.
   *        _n2n is usefull within one mesh.Compute() only
   */
  void _Listener::clearN2N( SMESH_Mesh* tgtMesh )
  {
    list< _ImportData >& dList = get()->_tgtMesh2ImportData[tgtMesh];
    list< _ImportData >::iterator d = dList.begin();
    for ( ; d != dList.end(); ++d )
      d->_n2n.clear();
  }
  //--------------------------------------------------------------------------------
  /*!
   * \brief Clear submeshes and remove imported mesh and/or groups if necessary
   *  \param sm - cleared submesh
   *  \param data - data holding imported groups
   */
  void _Listener::clearSubmesh(SMESH_subMesh* sm, _ListenerData* data, bool clearAllSub)
  {
    list< _ImportData > &  dList = _tgtMesh2ImportData[ sm->GetFather() ];
    list< _ImportData >::iterator d = dList.begin();
    for ( ; d != dList.end(); ++d )
    {
      if ( !d->_subM.count( sm )) continue;
      if ( (*d)._computedSubM.erase( sm ) )
      {
        bool copyMesh = !d->_copyMeshSubM.empty();
        if ( copyMesh || clearAllSub )
        {
          // remove imported mesh and groups
          d->removeImportedMesh( sm->GetFather()->GetMeshDS() );

          if ( data && data->myType == SRC_HYP )
            d->removeGroups( sm, data->_srcHyp );

          // clear the rest submeshes
          if ( !d->_computedSubM.empty() )
          {
            d->_computedSubM.clear();
            set< SMESH_subMesh*, _SubLess>::iterator sub = d->_subM.begin();
            for ( ; sub != d->_subM.end(); ++sub )
            {
              SMESH_subMesh* subM = *sub;
              _ListenerData* hypData = (_ListenerData*) subM->GetEventListenerData( get() );
              if ( hypData && hypData->myType == SRC_HYP )
                d->removeGroups( sm, hypData->_srcHyp );

              subM->ComputeStateEngine( SMESH_subMesh::CLEAN );
              if ( subM->GetSubShape().ShapeType() == TopAbs_FACE )
                subM->ComputeSubMeshStateEngine( SMESH_subMesh::CLEAN );
            }
          }
        }
        sm->ComputeStateEngine( SMESH_subMesh::CLEAN );
        if ( sm->GetSubShape().ShapeType() == TopAbs_FACE )
          sm->ComputeSubMeshStateEngine( SMESH_subMesh::CLEAN );
      }
      if ( data && data->myType == SRC_HYP )
        d->trackHypParams( sm, data->_srcHyp );
      d->_n2n.clear();
      d->_e2e.clear();
    }
  }
  //--------------------------------------------------------------------------------
  /*!
   * \brief Remove imported mesh and/or groups
   */
  void _Listener::ProcessEvent(const int                       event,
                               const int                       eventType,
                               SMESH_subMesh*                  subMesh,
                               SMESH_subMeshEventListenerData* data,
                               const SMESH_Hypothesis*         /*hyp*/)
  {
    if ( data && data->myType == WAIT_HYP_MODIF )
    {
      // event of Import submesh
      if ( SMESH_subMesh::MODIF_HYP  == event &&
           SMESH_subMesh::ALGO_EVENT == eventType )
      {
        // re-call SetEventListener() to take into account valid parameters
        // of ImportSource hypothesis
        if ( SMESH_Algo* algo = subMesh->GetAlgo() )
          algo->SetEventListener( subMesh );
      }
    }
    else if ( data && data->myType == LISTEN_SRC_MESH )
    {
      // event of source mesh
      if ( SMESH_subMesh::COMPUTE_EVENT == eventType )
      {
        switch ( event ) {
        case SMESH_subMesh::CLEAN:
          // source mesh cleaned -> clean target mesh
          clearSubmesh( data->mySubMeshes.front(), (_ListenerData*) data, /*all=*/true );
          break;
        case SMESH_subMesh::SUBMESH_COMPUTED: {
          // source mesh computed -> reset FAILED state of Import submeshes to
          // READY_TO_COMPUTE
          SMESH_Mesh* srcMesh = subMesh->GetFather();
          if ( srcMesh->NbEdges() > 0 || srcMesh->NbFaces() > 0 )
          {
            SMESH_Mesh* m = data->mySubMeshes.front()->GetFather();
            if ( SMESH_subMesh* sm1 = m->GetSubMeshContaining(1))
            {
              sm1->ComputeStateEngine(SMESH_subMesh::SUBMESH_COMPUTED );
              sm1->ComputeSubMeshStateEngine( SMESH_subMesh::SUBMESH_COMPUTED );
            }
          }
          break;
        }
        default:;
        }
      }
      if ( !data->mySubMeshes.empty() )
        clearN2N( data->mySubMeshes.front()->GetFather() );
    }
    else // event of Import submesh
    {
      // find out what happens: import hyp modified or removed
      bool removeImport = false, modifHyp = false;
      if ( SMESH_subMesh::ALGO_EVENT == eventType )
        modifHyp = true;
      if ( subMesh->GetAlgoState() != SMESH_subMesh::HYP_OK )
      {
        removeImport = true;
      }
      else if (( SMESH_subMesh::REMOVE_ALGO == event ||
                 SMESH_subMesh::REMOVE_FATHER_ALGO == event ) &&
               SMESH_subMesh::ALGO_EVENT == eventType )
      {
        SMESH_Algo* algo = subMesh->GetAlgo();
        removeImport = ( strncmp( "Import", algo->GetName(), 6 ) != 0 );
      }

      if ( removeImport )
      {
        // treate removal of Import algo from subMesh
        removeSubmesh( subMesh, (_ListenerData*) data );
      }
      else if ( modifHyp ||
                ( SMESH_subMesh::CLEAN         == event &&
                  SMESH_subMesh::COMPUTE_EVENT == eventType))
      {
        // treate modification of ImportSource hypothesis
        clearSubmesh( subMesh, (_ListenerData*) data, /*all=*/false );
      }
      else if ( SMESH_subMesh::CHECK_COMPUTE_STATE == event &&
                SMESH_subMesh::COMPUTE_EVENT       == eventType )
      {
        // check compute state of all submeshes impoting from same src mesh;
        // this is to take into account 1D computed submeshes hidden by 2D import algo;
        // else source mesh is not copied as _subM.size != _computedSubM.size()
        list< _ImportData > &  dList = _tgtMesh2ImportData[ subMesh->GetFather() ];
        list< _ImportData >::iterator d = dList.begin();
        for ( ; d != dList.end(); ++d )
          if ( d->_subM.count( subMesh ))
          {
            set<SMESH_subMesh*,_SubLess>::iterator smIt = d->_subM.begin();
            for( ; smIt != d->_subM.end(); ++smIt )
              if ( (*smIt)->IsMeshComputed() )
                d->_computedSubM.insert( *smIt);
          }
      }
      // Clear _ImportData::_n2n if it's no more useful, i.e. when
      // the event is not within mesh.Compute()
      if ( SMESH_subMesh::ALGO_EVENT == eventType )
        clearN2N( subMesh->GetFather() );
    }
  }

  //================================================================================
  /*!
   * \brief Return an ID of submesh to store nodes and elements of a copied mesh
   */
  //================================================================================

  int getSubmeshIDForCopiedMesh(const SMESHDS_Mesh* srcMeshDS,
                                SMESH_Mesh*         tgtMesh)
  {
    // To get SMESH_subMesh corresponding to srcMeshDS we need to have a shape
    // for which SMESHDS_Mesh::IsGroupOfSubShapes() returns true.
    // And this shape must be different from sub-shapes of the main shape.
    // So we create a compound containing
    // 1) some sub-shapes of SMESH_Mesh::PseudoShape() corresponding to
    //    srcMeshDS->GetPersistentId()
    // 2) the 1-st vertex of the main shape to assure
    //    SMESHDS_Mesh::IsGroupOfSubShapes(shape)==true
    TopoDS_Shape shapeForSrcMesh;
    TopTools_IndexedMapOfShape pseudoSubShapes;
    TopExp::MapShapes( SMESH_Mesh::PseudoShape(), pseudoSubShapes );

    // index of pseudoSubShapes corresponding to srcMeshDS
    int    subIndex = 1 + srcMeshDS->GetPersistentId() % pseudoSubShapes.Extent();
    int nbSubShapes = 1 + srcMeshDS->GetPersistentId() / pseudoSubShapes.Extent();

    // try to find already present shapeForSrcMesh
    SMESHDS_Mesh* tgtMeshDS = tgtMesh->GetMeshDS();
    for ( int i = tgtMeshDS->MaxShapeIndex(); i > 0 && shapeForSrcMesh.IsNull(); --i )
    {
      const TopoDS_Shape& s = tgtMeshDS->IndexToShape(i);
      if ( s.ShapeType() != TopAbs_COMPOUND ) break;
      TopoDS_Iterator sSubIt( s );
      for ( int iSub = 0; iSub < nbSubShapes && sSubIt.More(); ++iSub, sSubIt.Next() )
        if ( pseudoSubShapes( subIndex+iSub ).IsSame( sSubIt.Value()))
          if ( iSub+1 == nbSubShapes )
          {
            shapeForSrcMesh = s;
            break;
          }
    }
    if ( shapeForSrcMesh.IsNull() )
    {
      // make a new shapeForSrcMesh
      BRep_Builder aBuilder;
      TopoDS_Compound comp;
      aBuilder.MakeCompound( comp );
      shapeForSrcMesh = comp;
      for ( int iSub = 0; iSub < nbSubShapes; ++iSub )
        if ( subIndex+iSub <= pseudoSubShapes.Extent() )
          aBuilder.Add( comp, pseudoSubShapes( subIndex+iSub ));
      TopExp_Explorer vExp( tgtMeshDS->ShapeToMesh(), TopAbs_VERTEX );
      aBuilder.Add( comp, vExp.Current() );
    }
    SMESH_subMesh* sm = tgtMesh->GetSubMesh( shapeForSrcMesh );
    SMESHDS_SubMesh* smDS = sm->GetSubMeshDS();
    if ( !smDS )
      smDS = tgtMeshDS->NewSubMesh( sm->GetId() );

    // make ordinary submesh from a complex one
    if ( smDS->IsComplexSubmesh() )
    {
      list< const SMESHDS_SubMesh* > subSM;
      SMESHDS_SubMeshIteratorPtr smIt = smDS->GetSubMeshIterator();
      while ( smIt->more() ) subSM.push_back( smIt->next() );
      list< const SMESHDS_SubMesh* >::iterator sub = subSM.begin();
      for ( ; sub != subSM.end(); ++sub)
        smDS->RemoveSubMesh( *sub );
    }
    return sm->GetId();
  }

  //================================================================================
  /*!
   * \brief Return a submesh to store nodes and elements of a copied mesh
   * and set event listeners in order to clear
   * imported mesh and groups as soon as submesh state requires it
   */
  //================================================================================

  SMESHDS_SubMesh* getSubmeshForCopiedMesh(const SMESH_Mesh*                    srcMesh,
                                           SMESH_Mesh*                          tgtMesh,
                                           const TopoDS_Shape&                  tgtShape,
                                           StdMeshers_Import_1D::TNodeNodeMap*& n2n,
                                           StdMeshers_Import_1D::TElemElemMap*& e2e,
                                           bool &                               toCopyGroups)
  {
    StdMeshers_Import_1D::getMaps( srcMesh, tgtMesh, n2n,e2e );

    _ImportData* iData = _Listener::getImportData(srcMesh,tgtMesh);

    SMESH_subMesh* importedSM = tgtMesh->GetSubMesh( tgtShape );
    iData->addComputed( importedSM );
    if ( iData->_computedSubM.size() != iData->_subM.size() )
      return 0; // not all submeshes computed yet

    toCopyGroups = !iData->_copyGroupSubM.empty();

    if ( !iData->_copyMeshSubM.empty())
    {
      // make submesh to store a copied mesh
      int smID = getSubmeshIDForCopiedMesh( srcMesh->GetMeshDS(), tgtMesh );
      SMESHDS_SubMesh* subDS = tgtMesh->GetMeshDS()->NewSubMesh( smID );

      iData->_importMeshSubID = smID;
      iData->_importMeshSubDS = subDS;
      return subDS;
    }
    return 0;
  }

} // namespace

//=============================================================================
/*!
 * Check presence of a hypothesis
 */
//=============================================================================

bool StdMeshers_Import_1D::CheckHypothesis
                         (SMESH_Mesh&                          aMesh,
                          const TopoDS_Shape&                  aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  _sourceHyp = 0;

  const list <const SMESHDS_Hypothesis * >&hyps = GetUsedHypothesis(aMesh, aShape);
  if ( hyps.size() == 0 )
  {
    aStatus = SMESH_Hypothesis::HYP_MISSING;
    return false;  // can't work with no hypothesis
  }

  if ( hyps.size() > 1 )
  {
    aStatus = SMESH_Hypothesis::HYP_ALREADY_EXIST;
    return false;
  }

  const SMESHDS_Hypothesis *theHyp = hyps.front();

  string hypName = theHyp->GetName();

  if (hypName == _compatibleHypothesis.front())
  {
    _sourceHyp = (StdMeshers_ImportSource1D *)theHyp;
    aStatus = _sourceHyp->GetGroups().empty() ? HYP_BAD_PARAMETER : HYP_OK;
    if ( aStatus == HYP_BAD_PARAMETER )
      _Listener::waitHypModification( aMesh.GetSubMesh( aShape ));
    return aStatus == HYP_OK;
  }

  aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;
  return false;
}

//=============================================================================
/*!
 * Import elements from the other mesh
 */
//=============================================================================

bool StdMeshers_Import_1D::Compute(SMESH_Mesh & theMesh, const TopoDS_Shape & theShape)
{
  if ( !_sourceHyp ) return false;

  //MESSAGE("---------> StdMeshers_Import_1D::Compute");
  const vector<SMESH_Group*>& srcGroups = _sourceHyp->GetGroups(/*loaded=*/true);
  if ( srcGroups.empty() )
    return error("Invalid source groups");

  SMESH_MesherHelper helper(theMesh);
  helper.SetSubShape(theShape);
  SMESHDS_Mesh* tgtMesh = theMesh.GetMeshDS();

  const TopoDS_Edge& geomEdge = TopoDS::Edge( theShape );
  const double edgeTol = BRep_Tool::Tolerance( geomEdge );
  const int shapeID = tgtMesh->ShapeToIndex( geomEdge );

  set<int> subShapeIDs;
  subShapeIDs.insert( shapeID );

  // get nodes on vertices
  list < SMESH_TNodeXYZ > vertexNodes;
  list < SMESH_TNodeXYZ >::iterator vNIt;
  TopExp_Explorer vExp( theShape, TopAbs_VERTEX );
  for ( ; vExp.More(); vExp.Next() )
  {
    const TopoDS_Vertex& v = TopoDS::Vertex( vExp.Current() );
    if ( !subShapeIDs.insert( tgtMesh->ShapeToIndex( v )).second )
      continue; // closed edge
    const SMDS_MeshNode* n = SMESH_Algo::VertexNode( v, tgtMesh );
    if ( !n )
    {
      _gen->Compute(theMesh,v,/*anUpward=*/true);
      n = SMESH_Algo::VertexNode( v, tgtMesh );
      //MESSAGE("_gen->Compute " << n);
      if ( !n ) return false; // very strange
    }
    vertexNodes.push_back( SMESH_TNodeXYZ( n ));
    //MESSAGE("SMESH_Algo::VertexNode " << n->GetID() << " " << n->X() << " " << n->Y() << " " << n->Z() );
  }

  // import edges from groups
  TNodeNodeMap* n2n;
  TElemElemMap* e2e;
  for ( int iG = 0; iG < srcGroups.size(); ++iG )
  {
    const SMESHDS_GroupBase* srcGroup = srcGroups[iG]->GetGroupDS();

    const int meshID = srcGroup->GetMesh()->GetPersistentId();
    const SMESH_Mesh* srcMesh = GetMeshByPersistentID( meshID );
    if ( !srcMesh ) continue;
    getMaps( srcMesh, &theMesh, n2n, e2e );

    SMDS_ElemIteratorPtr srcElems = srcGroup->GetElements();
    vector<const SMDS_MeshNode*> newNodes;
    SMDS_MeshNode *tmpNode = helper.AddNode(0,0,0);
    double u = 0.314159; // "random" value between 0 and 1, avoid 0 and 1, false detection possible on edge restrictions
    while ( srcElems->more() ) // loop on group contents
    {
      const SMDS_MeshElement* edge = srcElems->next();
      // find or create nodes of a new edge
      newNodes.resize( edge->NbNodes() );
      //MESSAGE("edge->NbNodes " << edge->NbNodes());
      newNodes.back() = 0;
      SMDS_MeshElement::iterator node = edge->begin_nodes();
      SMESH_TNodeXYZ a(edge->GetNode(0));
      // --- define a tolerance relative to the length of an edge
      double mytol = a.Distance(edge->GetNode(edge->NbNodes()-1))/25;
      //mytol = max(1.E-5, 10*edgeTol); // too strict and not necessary
      //MESSAGE("mytol = " << mytol);
      for ( unsigned i = 0; i < newNodes.size(); ++i, ++node )
      {
        TNodeNodeMap::iterator n2nIt = n2n->insert( make_pair( *node, (SMDS_MeshNode*)0 )).first;
        if ( n2nIt->second )
        {
          if ( !subShapeIDs.count( n2nIt->second->getshapeId() ))
            break;
        }
        else
        {
          // find an existing vertex node
          double checktol = max(1.E-10, 10*edgeTol*edgeTol);
          for ( vNIt = vertexNodes.begin(); vNIt != vertexNodes.end(); ++vNIt)
            if ( vNIt->SquareDistance( *node ) < checktol)
            {
              //MESSAGE("SquareDistance " << vNIt->SquareDistance( *node ) << " checktol " << checktol <<" "<<vNIt->X()<<" "<<vNIt->Y()<<" "<<vNIt->Z());
              (*n2nIt).second = vNIt->_node;
              vertexNodes.erase( vNIt );
              break;
            }
            else if ( vNIt->SquareDistance( *node ) < 10*checktol)
              MESSAGE("SquareDistance missed" << vNIt->SquareDistance( *node ) << " checktol " << checktol <<" "<<vNIt->X()<<" "<<vNIt->Y()<<" "<<vNIt->Z());
        }
        if ( !n2nIt->second )
        {
          // find out if node lies on theShape
          //double dxyz[4];
          tmpNode->setXYZ( (*node)->X(), (*node)->Y(), (*node)->Z());
          if ( helper.CheckNodeU( geomEdge, tmpNode, u, mytol, /*force=*/true)) // , dxyz )) // dxyz used for debug purposes
          {
            SMDS_MeshNode* newNode = tgtMesh->AddNode( (*node)->X(), (*node)->Y(), (*node)->Z());
            n2nIt->second = newNode;
            tgtMesh->SetNodeOnEdge( newNode, shapeID, u );
            //MESSAGE("u=" << u << " " << newNode->X()<< " " << newNode->Y()<< " " << newNode->Z());
            //MESSAGE("d=" << dxyz[0] << " " << dxyz[1] << " " << dxyz[2] << " " << dxyz[3]);
          }
        }
        if ( !(newNodes[i] = n2nIt->second ))
          break;
      }
      if ( !newNodes.back() )
      {
        //MESSAGE("not all nodes of edge lie on theShape");
        continue; // not all nodes of edge lie on theShape
      }

      // make a new edge
      SMDS_MeshElement * newEdge;
      if ( newNodes.size() == 3 )
        newEdge = tgtMesh->AddEdge( newNodes[0], newNodes[1], newNodes[2] );
      else
        newEdge = tgtMesh->AddEdge( newNodes[0], newNodes[1]);
      //MESSAGE("add Edge " << newNodes[0]->GetID() << " " << newNodes[1]->GetID());
      tgtMesh->SetMeshElementOnShape( newEdge, shapeID );
      e2e->insert( make_pair( edge, newEdge ));
    }
    helper.GetMeshDS()->RemoveNode(tmpNode);
  }
  if ( n2n->empty())
    return error("Empty source groups");

  // check if the whole geom edge is covered by imported segments;
  // the check consist in passing by segments from one vetrex node to another
  bool isEdgeMeshed = false;
  if ( SMESHDS_SubMesh* tgtSM = tgtMesh->MeshElements( theShape ))
  {
    const TopoDS_Vertex& v = ( vExp.ReInit(), TopoDS::Vertex( vExp.Current() ));
    const SMDS_MeshNode* n = SMESH_Algo::VertexNode( v, tgtMesh );
    const SMDS_MeshElement* seg = 0;
    SMDS_ElemIteratorPtr segIt = n->GetInverseElementIterator(SMDSAbs_Edge);
    while ( segIt->more() && !seg )
      if ( !tgtSM->Contains( seg = segIt->next()))
        seg = 0;
    int nbPassedSegs = 0;
    while ( seg )
    {
      ++nbPassedSegs;
      const SMDS_MeshNode* n2 = seg->GetNode(0);
      n = ( n2 == n ? seg->GetNode(1) : n2 );
      if ( n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX )
        break;
      const SMDS_MeshElement* seg2 = 0;
      segIt = n->GetInverseElementIterator(SMDSAbs_Edge);
      while ( segIt->more() && !seg2 )
        if ( seg == ( seg2 = segIt->next()))
          seg2 = 0;
      seg = seg2;
    }
    if (nbPassedSegs > 0 && tgtSM->NbElements() > nbPassedSegs )
      return error( "Source elements overlap one another");

    isEdgeMeshed = ( tgtSM->NbElements() == nbPassedSegs &&
                     n->GetPosition()->GetTypeOfPosition() == SMDS_TOP_VERTEX );
  }
  if ( !isEdgeMeshed )
    return error( "Source elements don't cover totally the geometrical edge" );

  // copy meshes
  vector<SMESH_Mesh*> srcMeshes = _sourceHyp->GetSourceMeshes();
  for ( unsigned i = 0; i < srcMeshes.size(); ++i )
    importMesh( srcMeshes[i], theMesh, _sourceHyp, theShape );

  return true;
}

//================================================================================
/*!
 * \brief Copy mesh and groups
 */
//================================================================================

void StdMeshers_Import_1D::importMesh(const SMESH_Mesh*          srcMesh,
                                      SMESH_Mesh &               tgtMesh,
                                      StdMeshers_ImportSource1D* srcHyp,
                                      const TopoDS_Shape&        tgtShape)
{
  // get submesh to store the imported mesh
  TNodeNodeMap* n2n;
  TElemElemMap* e2e;
  bool toCopyGroups;
  SMESHDS_SubMesh* tgtSubMesh =
    getSubmeshForCopiedMesh( srcMesh, &tgtMesh, tgtShape, n2n, e2e, toCopyGroups );
  if ( !tgtSubMesh || tgtSubMesh->NbNodes() + tgtSubMesh->NbElements() > 0 )
    return; // not to copy srcMeshDS twice

  SMESHDS_Mesh* tgtMeshDS = tgtMesh.GetMeshDS();
  SMESH_MeshEditor additor( &tgtMesh );

  // 1. Copy mesh

  SMESH_MeshEditor::ElemFeatures elemType;
  vector<const SMDS_MeshNode*> newNodes;
  const SMESHDS_Mesh* srcMeshDS = srcMesh->GetMeshDS();
  SMDS_ElemIteratorPtr eIt = srcMeshDS->elementsIterator();
  while ( eIt->more() )
  {
    const SMDS_MeshElement* elem = eIt->next();
    TElemElemMap::iterator e2eIt = e2e->insert( make_pair( elem, (SMDS_MeshElement*)0 )).first;
    if ( e2eIt->second ) continue; // already copied by Compute()
    newNodes.resize( elem->NbNodes() );
    SMDS_MeshElement::iterator node = elem->begin_nodes();
    for ( unsigned i = 0; i < newNodes.size(); ++i, ++node )
    {
      TNodeNodeMap::iterator n2nIt = n2n->insert( make_pair( *node, (SMDS_MeshNode*)0 )).first;
      if ( !n2nIt->second )
      {
        (*n2nIt).second = tgtMeshDS->AddNode( (*node)->X(), (*node)->Y(), (*node)->Z());
        tgtSubMesh->AddNode( n2nIt->second );
      }
      newNodes[i] = n2nIt->second;
    }
    const SMDS_MeshElement* newElem =
      tgtMeshDS->FindElement( newNodes, elem->GetType(), /*noMedium=*/false );
    if ( !newElem )
    {
      newElem = additor.AddElement( newNodes, elemType.Init( elem, /*basicOnly=*/false ));
      tgtSubMesh->AddElement( newElem );
    }
    if ( toCopyGroups )
      (*e2eIt).second = newElem;
  }
  // copy free nodes
  if ( srcMeshDS->NbNodes() > n2n->size() )
  {
    SMDS_NodeIteratorPtr nIt = srcMeshDS->nodesIterator();
    while( nIt->more() )
    {
      const SMDS_MeshNode* node = nIt->next();
      if ( node->NbInverseElements() == 0 )
      {
        const SMDS_MeshNode* newNode = tgtMeshDS->AddNode( node->X(), node->Y(), node->Z());
        n2n->insert( make_pair( node, newNode ));
        tgtSubMesh->AddNode( newNode );
      }
    }
  }

  // 2. Copy groups

  vector<SMESH_Group*> resultGroups;
  if ( toCopyGroups )
  {
    // collect names of existing groups to assure uniqueness of group names within a type
    map< SMDSAbs_ElementType, set<string> > namesByType;
    SMESH_Mesh::GroupIteratorPtr groupIt = tgtMesh.GetGroups();
    while ( groupIt->more() )
    {
      SMESH_Group* tgtGroup = groupIt->next();
      namesByType[ tgtGroup->GetGroupDS()->GetType() ].insert( tgtGroup->GetName() );
    }
    if (srcMesh)
    {
      SMESH_Mesh::GroupIteratorPtr groupIt = srcMesh->GetGroups();
      while ( groupIt->more() )
      {
        SMESH_Group* srcGroup = groupIt->next();
        SMESHDS_GroupBase* srcGroupDS = srcGroup->GetGroupDS();
        string name = srcGroup->GetName();
        int nb = 1;
        while ( !namesByType[ srcGroupDS->GetType() ].insert( name ).second )
          name = SMESH_Comment(srcGroup->GetName()) << "_imported_" << nb++;
        SMESH_Group* newGroup = tgtMesh.AddGroup( srcGroupDS->GetType(), name.c_str(), nb );
        SMESHDS_Group* newGroupDS = (SMESHDS_Group*)newGroup->GetGroupDS();
        resultGroups.push_back( newGroup );

        eIt = srcGroupDS->GetElements();
        if ( srcGroupDS->GetType() == SMDSAbs_Node )
          while (eIt->more())
          {
            TNodeNodeMap::iterator n2nIt = n2n->find((const SMDS_MeshNode*) eIt->next() );
            if ( n2nIt != n2n->end() && n2nIt->second )
              newGroupDS->SMDSGroup().Add((*n2nIt).second );
          }
        else
          while (eIt->more())
          {
            TElemElemMap::iterator e2eIt = e2e->find( eIt->next() );
            if ( e2eIt != e2e->end() && e2eIt->second )
              newGroupDS->SMDSGroup().Add((*e2eIt).second );
          }
      }
    }
  }
  n2n->clear();
  e2e->clear();

  // Remember created groups in order to remove them as soon as the srcHyp is
  // modified or something other similar happens. This imformation must be persistent,
  // for that store them in a hypothesis as it stores its values in the file anyway
  srcHyp->StoreResultGroups( resultGroups, *srcMeshDS, *tgtMeshDS );
}

//=============================================================================
/*!
 * \brief Set needed event listeners and create a submesh for a copied mesh
 *
 * This method is called only if a submesh has HYP_OK algo_state.
 */
//=============================================================================

void StdMeshers_Import_1D::setEventListener(SMESH_subMesh*             subMesh,
                                            StdMeshers_ImportSource1D* sourceHyp)
{
  if ( sourceHyp )
  {
    vector<SMESH_Mesh*> srcMeshes = sourceHyp->GetSourceMeshes();
    if ( srcMeshes.empty() )
      _Listener::waitHypModification( subMesh );
    for ( unsigned i = 0; i < srcMeshes.size(); ++i )
      // set a listener to remove the imported mesh and groups
      _Listener::storeImportSubmesh( subMesh, srcMeshes[i], sourceHyp );
  }
}
void StdMeshers_Import_1D::SetEventListener(SMESH_subMesh* subMesh)
{
  if ( !_sourceHyp )
  {
    const TopoDS_Shape& tgtShape = subMesh->GetSubShape();
    SMESH_Mesh*         tgtMesh  = subMesh->GetFather();
    Hypothesis_Status aStatus;
    CheckHypothesis( *tgtMesh, tgtShape, aStatus );
  }
  setEventListener( subMesh, _sourceHyp );
}

void StdMeshers_Import_1D::SubmeshRestored(SMESH_subMesh* subMesh)
{
  SetEventListener(subMesh);
}

//=============================================================================
/*!
 * Predict nb of mesh entities created by Compute()
 */
//=============================================================================

bool StdMeshers_Import_1D::Evaluate(SMESH_Mesh &         theMesh,
                                    const TopoDS_Shape & theShape,
                                    MapShapeNbElems&     aResMap)
{
  if ( !_sourceHyp ) return false;

  const vector<SMESH_Group*>& srcGroups = _sourceHyp->GetGroups();
  if ( srcGroups.empty() )
    return error("Invalid source groups");

  vector<int> aVec(SMDSEntity_Last,0);

  bool toCopyMesh, toCopyGroups;
  _sourceHyp->GetCopySourceMesh(toCopyMesh, toCopyGroups);
  if ( toCopyMesh ) // the whole mesh is copied
  {
    vector<SMESH_Mesh*> srcMeshes = _sourceHyp->GetSourceMeshes();
    for ( unsigned i = 0; i < srcMeshes.size(); ++i )
    {
      SMESH_subMesh* sm = getSubMeshOfCopiedMesh( theMesh, *srcMeshes[i]);
      if ( !sm || aResMap.count( sm )) continue; // already counted
      aVec.assign( SMDSEntity_Last, 0);
      const SMDS_MeshInfo& aMeshInfo = srcMeshes[i]->GetMeshDS()->GetMeshInfo();
      for (int i = 0; i < SMDSEntity_Last; i++)
        aVec[i] = aMeshInfo.NbEntities((SMDSAbs_EntityType)i);
    }
  }
  else
  {
    SMESH_MesherHelper helper(theMesh);

    const TopoDS_Edge& geomEdge = TopoDS::Edge( theShape );
    const double edgeTol = helper.MaxTolerance( geomEdge );

    // take into account nodes on vertices
    TopExp_Explorer vExp( theShape, TopAbs_VERTEX );
    for ( ; vExp.More(); vExp.Next() )
      theMesh.GetSubMesh( vExp.Current())->Evaluate( aResMap );

    // count edges imported from groups
    int nbEdges = 0, nbQuadEdges = 0;
    for ( int iG = 0; iG < srcGroups.size(); ++iG )
    {
      const SMESHDS_GroupBase* srcGroup = srcGroups[iG]->GetGroupDS();
      SMDS_ElemIteratorPtr srcElems = srcGroup->GetElements();
      SMDS_MeshNode *tmpNode = helper.AddNode(0,0,0);
      while ( srcElems->more() ) // loop on group contents
      {
        const SMDS_MeshElement* edge = srcElems->next();
        // find out if edge is located on geomEdge by projecting
        // a middle of edge to geomEdge
        SMESH_TNodeXYZ p1( edge->GetNode(0));
        SMESH_TNodeXYZ p2( edge->GetNode(1));
        gp_XYZ middle = ( p1 + p2 ) / 2.;
        tmpNode->setXYZ( middle.X(), middle.Y(), middle.Z());
        double u = 0;
        if ( helper.CheckNodeU( geomEdge, tmpNode, u, 10 * edgeTol, /*force=*/true ))
          ++( edge->IsQuadratic() ? nbQuadEdges : nbEdges);
      }
      helper.GetMeshDS()->RemoveNode(tmpNode);
    }

    int nbNodes = nbEdges + 2 * nbQuadEdges - 1;

    aVec[SMDSEntity_Node     ] = nbNodes;
    aVec[SMDSEntity_Edge     ] = nbEdges;
    aVec[SMDSEntity_Quad_Edge] = nbQuadEdges;
  }

  SMESH_subMesh * sm = theMesh.GetSubMesh(theShape);
  aResMap.insert(make_pair(sm,aVec));

  return true;
}

//================================================================================
/*!
 * \brief Return node-node and element-element maps for import of geiven source mesh
 */
//================================================================================

void StdMeshers_Import_1D::getMaps(const SMESH_Mesh* srcMesh,
                                   SMESH_Mesh*       tgtMesh,
                                   TNodeNodeMap*&    n2n,
                                   TElemElemMap*&    e2e)
{
  _ImportData* iData = _Listener::getImportData(srcMesh,tgtMesh);
  n2n = &iData->_n2n;
  e2e = &iData->_e2e;
  if ( iData->_copyMeshSubM.empty() )
  {
    // n2n->clear(); -- for sharing nodes on EDGEs
    e2e->clear();
  }
}

//================================================================================
/*!
 * \brief Return submesh corresponding to the copied mesh
 */
//================================================================================

SMESH_subMesh* StdMeshers_Import_1D::getSubMeshOfCopiedMesh( SMESH_Mesh& tgtMesh,
                                                             SMESH_Mesh& srcMesh )
{
  _ImportData* iData = _Listener::getImportData(&srcMesh,&tgtMesh);
  if ( iData->_copyMeshSubM.empty() ) return 0;
  SMESH_subMesh* sm = tgtMesh.GetSubMeshContaining( iData->_importMeshSubID );
  return sm;
}

