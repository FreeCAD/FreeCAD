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
//  File   : StdMeshers_Regular_1D.cxx
//           Moved here from SMESH_Regular_1D.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH

#include "StdMeshers_Regular_1D.hxx"
#include "StdMeshers_Distribution.hxx"

#include "StdMeshers_Arithmetic1D.hxx"
#include "StdMeshers_AutomaticLength.hxx"
#include "StdMeshers_Deflection1D.hxx"
#include "StdMeshers_LocalLength.hxx"
#include "StdMeshers_MaxLength.hxx"
#include "StdMeshers_NumberOfSegments.hxx"
#include "StdMeshers_Propagation.hxx"
#include "StdMeshers_SegmentLengthAroundVertex.hxx"
#include "StdMeshers_StartEndLength.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "SMESH_Comment.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"

#include "SMESH_Exception.hxx"
#include "utilities.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

#include <string>

using namespace std;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Regular_1D::StdMeshers_Regular_1D(int hypId, int studyId,
	SMESH_Gen * gen):SMESH_1D_Algo(hypId, studyId, gen)
{
	MESSAGE("StdMeshers_Regular_1D::StdMeshers_Regular_1D");
	_name = "Regular_1D";
	_shapeType = (1 << TopAbs_EDGE);

	_compatibleHypothesis.push_back("LocalLength");
	_compatibleHypothesis.push_back("MaxLength");
	_compatibleHypothesis.push_back("NumberOfSegments");
	_compatibleHypothesis.push_back("StartEndLength");
	_compatibleHypothesis.push_back("Deflection1D");
	_compatibleHypothesis.push_back("Arithmetic1D");
	_compatibleHypothesis.push_back("AutomaticLength");

	_compatibleHypothesis.push_back("QuadraticMesh"); // auxiliary !!!
	_compatibleHypothesis.push_back("Propagation"); // auxiliary !!!
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_Regular_1D::~StdMeshers_Regular_1D()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Regular_1D::CheckHypothesis
                         (SMESH_Mesh&                          aMesh,
                          const TopoDS_Shape&                  aShape,
                          SMESH_Hypothesis::Hypothesis_Status& aStatus)
{
  _hypType = NONE;
  _quadraticMesh = false;

  const bool ignoreAuxiliaryHyps = false;
  const list <const SMESHDS_Hypothesis * > & hyps =
    GetUsedHypothesis(aMesh, aShape, ignoreAuxiliaryHyps);

  // find non-auxiliary hypothesis
  const SMESHDS_Hypothesis *theHyp = 0;
  list <const SMESHDS_Hypothesis * >::const_iterator h = hyps.begin();
  for ( ; h != hyps.end(); ++h ) {
    if ( static_cast<const SMESH_Hypothesis*>(*h)->IsAuxiliary() ) {
      if ( strcmp( "QuadraticMesh", (*h)->GetName() ) == 0 )
        _quadraticMesh = true;
    }
    else {
      if ( !theHyp )
        theHyp = *h; // use only the first non-auxiliary hypothesis
    }
  }

  if ( !theHyp )
  {
    aStatus = SMESH_Hypothesis::HYP_MISSING;
    return false;  // can't work without a hypothesis
  }

  string hypName = theHyp->GetName();

  if (hypName == "LocalLength")
  {
    const StdMeshers_LocalLength * hyp =
      dynamic_cast <const StdMeshers_LocalLength * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetLength();
    _value[ PRECISION_IND ] = hyp->GetPrecision();
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 );
    _hypType = LOCAL_LENGTH;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if (hypName == "MaxLength")
  {
    const StdMeshers_MaxLength * hyp =
      dynamic_cast <const StdMeshers_MaxLength * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetLength();
    if ( hyp->GetUsePreestimatedLength() ) {
      if ( int nbSeg = aMesh.GetGen()->GetBoundaryBoxSegmentation() )
        _value[ BEG_LENGTH_IND ] = aMesh.GetShapeDiagonalSize() / nbSeg;
    }
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 );
    _hypType = MAX_LENGTH;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if (hypName == "NumberOfSegments")
  {
    const StdMeshers_NumberOfSegments * hyp =
      dynamic_cast <const StdMeshers_NumberOfSegments * >(theHyp);
    ASSERT(hyp);
    _ivalue[ NB_SEGMENTS_IND  ] = hyp->GetNumberOfSegments();
    ASSERT( _ivalue[ NB_SEGMENTS_IND ] > 0 );
    _ivalue[ DISTR_TYPE_IND ] = (int) hyp->GetDistrType();
    switch (_ivalue[ DISTR_TYPE_IND ])
    {
    case StdMeshers_NumberOfSegments::DT_Scale:
      _value[ SCALE_FACTOR_IND ] = hyp->GetScaleFactor();
      break;
    case StdMeshers_NumberOfSegments::DT_TabFunc:
      _vvalue[ TAB_FUNC_IND ] = hyp->GetTableFunction();
      break;
    case StdMeshers_NumberOfSegments::DT_ExprFunc:
      _svalue[ EXPR_FUNC_IND ] = hyp->GetExpressionFunction();
      break;
    case StdMeshers_NumberOfSegments::DT_Regular:
      break;
    default:
      ASSERT(0);
      break;
    }
    if (_ivalue[ DISTR_TYPE_IND ] == StdMeshers_NumberOfSegments::DT_TabFunc ||
        _ivalue[ DISTR_TYPE_IND ] == StdMeshers_NumberOfSegments::DT_ExprFunc)
        _ivalue[ CONV_MODE_IND ] = hyp->ConversionMode();
    _hypType = NB_SEGMENTS;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if (hypName == "Arithmetic1D")
  {
    const StdMeshers_Arithmetic1D * hyp =
      dynamic_cast <const StdMeshers_Arithmetic1D * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetLength( true );
    _value[ END_LENGTH_IND ] = hyp->GetLength( false );
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 && _value[ END_LENGTH_IND ] > 0 );
    _hypType = ARITHMETIC_1D;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if (hypName == "StartEndLength")
  {
    const StdMeshers_StartEndLength * hyp =
      dynamic_cast <const StdMeshers_StartEndLength * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetLength( true );
    _value[ END_LENGTH_IND ] = hyp->GetLength( false );
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 && _value[ END_LENGTH_IND ] > 0 );
    _hypType = BEG_END_LENGTH;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if (hypName == "Deflection1D")
  {
    const StdMeshers_Deflection1D * hyp =
      dynamic_cast <const StdMeshers_Deflection1D * >(theHyp);
    ASSERT(hyp);
    _value[ DEFLECTION_IND ] = hyp->GetDeflection();
    ASSERT( _value[ DEFLECTION_IND ] > 0 );
    _hypType = DEFLECTION;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if (hypName == "AutomaticLength")
  {
    StdMeshers_AutomaticLength * hyp = const_cast<StdMeshers_AutomaticLength *>
      (dynamic_cast <const StdMeshers_AutomaticLength * >(theHyp));
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = _value[ END_LENGTH_IND ] = hyp->GetLength( &aMesh, aShape );
//     _value[ BEG_LENGTH_IND ] = hyp->GetLength( &aMesh, aShape );
//     _value[ END_LENGTH_IND ] = Precision::Confusion(); // ?? or set to zero?
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 );
    _hypType = MAX_LENGTH;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }
  else
    aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;

  return ( _hypType != NONE );
}

static bool computeParamByFunc(Adaptor3d_Curve& C3d, double first, double last,
                               double length, bool theReverse,
                               int nbSeg, Function& func,
                               list<double>& theParams)
{
  // never do this way
  //OSD::SetSignal( true );

  if (nbSeg <= 0)
    return false;

  MESSAGE( "computeParamByFunc" );

  int nbPnt = 1 + nbSeg;
  vector<double> x(nbPnt, 0.);

  if (!buildDistribution(func, 0.0, 1.0, nbSeg, x, 1E-4))
     return false;

  MESSAGE( "Points:\n" );
  char buf[1024];
  for ( int i=0; i<=nbSeg; i++ )
  {
    sprintf(  buf, "%f\n", float(x[i] ) );
    MESSAGE( buf );
  }



  // apply parameters in range [0,1] to the space of the curve
  double prevU = first;
  double sign = 1.;
  if (theReverse)
  {
    prevU = last;
    sign = -1.;
  }
  for( int i = 1; i < nbSeg; i++ )
  {
    double curvLength = length * (x[i] - x[i-1]) * sign;
    GCPnts_AbscissaPoint Discret( C3d, curvLength, prevU );
    if ( !Discret.IsDone() )
      return false;
    double U = Discret.Parameter();
    if ( U > first && U < last )
      theParams.push_back( U );
    else
      return false;
    prevU = U;
  }
  return true;
}


//================================================================================
/*!
 * \brief adjust internal node parameters so that the last segment length == an
  * \param a1 - the first segment length
  * \param an - the last segment length
  * \param U1 - the first edge parameter
  * \param Un - the last edge parameter
  * \param length - the edge length
  * \param C3d - the edge curve
  * \param theParams - internal node parameters to adjust
  * \param adjustNeighbors2an - to adjust length of segments next to the last one
  *  and not to remove parameters
 */
//================================================================================

static void compensateError(double a1, double an,
                            double U1, double Un,
                            double            length,
                            Adaptor3d_Curve&  C3d,
                            list<double> &    theParams,
                            bool              adjustNeighbors2an = false)
{
  int i, nPar = theParams.size();
  if ( a1 + an < length && nPar > 1 )
  {
    bool reverse = ( U1 > Un );
    GCPnts_AbscissaPoint Discret(C3d, reverse ? an : -an, Un);
    if ( !Discret.IsDone() )
      return;
    double Utgt = Discret.Parameter(); // target value of the last parameter
    list<double>::reverse_iterator itU = theParams.rbegin();
    double Ul = *itU++; // real value of the last parameter
    double dUn = Utgt - Ul; // parametric error of <an>
    if ( Abs(dUn) <= Precision::Confusion() )
      return;
    double dU = Abs( Ul - *itU ); // parametric length of the last but one segment
    if ( adjustNeighbors2an || Abs(dUn) < 0.5 * dU ) { // last segment is a bit shorter than it should
      // move the last parameter to the edge beginning
    }
    else {  // last segment is much shorter than it should -> remove the last param and
      theParams.pop_back(); nPar--; // move the rest points toward the edge end
      dUn = Utgt - theParams.back();
    }

    double q  = dUn / ( nPar - 1 );
    if ( !adjustNeighbors2an ) {
      for ( itU = theParams.rbegin(), i = 1; i < nPar; itU++, i++ ) {
        (*itU) += dUn;
        dUn -= q;
      }
    }
    else {
      theParams.back() += dUn;
      double sign = reverse ? -1 : 1;
      double prevU = theParams.back();
      itU = theParams.rbegin();
      for ( ++itU, i = 2; i < nPar; ++itU, i++ ) {
        double newU = *itU + dUn;
        if ( newU*sign < prevU*sign ) {
          prevU = *itU = newU;
          dUn -= q;
        }
        else { // set U between prevU and next valid param
          list<double>::reverse_iterator itU2 = itU;
          ++itU2;
          int nb = 2;
          while ( (*itU2)*sign > prevU*sign ) {
            ++itU2; ++nb;
          }
          dU = ( *itU2 - prevU ) / nb;
          while ( itU != itU2 ) {
            *itU += dU; ++itU;
          }
          break;
        }
      }
    }
  }
}

//================================================================================
/*!
 * \brief Class used to clean mesh on edges when 0D hyp modified.
 * Common approach doesn't work when 0D algo is missing because the 0D hyp is
 * considered as not participating in computation whereas it is used by 1D algo.
 */
//================================================================================

// struct VertexEventListener : public SMESH_subMeshEventListener
// {
//   VertexEventListener():SMESH_subMeshEventListener(0) // won't be deleted by submesh
//   {}
//   /*!
//    * \brief Clean mesh on edges
//    * \param event - algo_event or compute_event itself (of SMESH_subMesh)
//    * \param eventType - ALGO_EVENT or COMPUTE_EVENT (of SMESH_subMesh)
//    * \param subMesh - the submesh where the event occures
//    */
//   void ProcessEvent(const int event, const int eventType, SMESH_subMesh* subMesh,
//                     EventListenerData*, const SMESH_Hypothesis*)
//   {
//     if ( eventType == SMESH_subMesh::ALGO_EVENT) // all algo events
//     {
//       subMesh->ComputeStateEngine( SMESH_subMesh::MODIF_ALGO_STATE );
//     }
//   }
// }; // struct VertexEventListener

//=============================================================================
/*!
 * \brief Sets event listener to vertex submeshes
 * \param subMesh - submesh where algo is set
 *
 * This method is called when a submesh gets HYP_OK algo_state.
 * After being set, event listener is notified on each event of a submesh.
 */
//=============================================================================

void StdMeshers_Regular_1D::SetEventListener(SMESH_subMesh* subMesh)
{
  StdMeshers_Propagation::SetPropagationMgr( subMesh );
}

//=============================================================================
/*!
 * \brief Do nothing
 * \param subMesh - restored submesh
 *
 * This method is called only if a submesh has HYP_OK algo_state.
 */
//=============================================================================

void StdMeshers_Regular_1D::SubmeshRestored(SMESH_subMesh* subMesh)
{
}

//=============================================================================
/*!
 * \brief Return StdMeshers_SegmentLengthAroundVertex assigned to vertex
 */
//=============================================================================

const StdMeshers_SegmentLengthAroundVertex*
StdMeshers_Regular_1D::getVertexHyp(SMESH_Mesh &          theMesh,
                                    const TopoDS_Vertex & theV)
{
  static SMESH_HypoFilter filter( SMESH_HypoFilter::HasName("SegmentAroundVertex_0D"));
  if ( const SMESH_Hypothesis * h = theMesh.GetHypothesis( theV, filter, true ))
  {
    SMESH_Algo* algo = const_cast< SMESH_Algo* >( static_cast< const SMESH_Algo* > ( h ));
    const list <const SMESHDS_Hypothesis *> & hypList = algo->GetUsedHypothesis( theMesh, theV, 0 );
    if ( !hypList.empty() && string("SegmentLengthAroundVertex") == hypList.front()->GetName() )
      return static_cast<const StdMeshers_SegmentLengthAroundVertex*>( hypList.front() );
  }
  return 0;
}

//================================================================================
/*!
 * \brief Tune parameters to fit "SegmentLengthAroundVertex" hypothesis
  * \param theC3d - wire curve
  * \param theLength - curve length
  * \param theParameters - internal nodes parameters to modify
  * \param theVf - 1st vertex
  * \param theVl - 2nd vertex
 */
//================================================================================

void StdMeshers_Regular_1D::redistributeNearVertices (SMESH_Mesh &          theMesh,
                                                      Adaptor3d_Curve &     theC3d,
                                                      double                theLength,
                                                      std::list< double > & theParameters,
                                                      const TopoDS_Vertex & theVf,
                                                      const TopoDS_Vertex & theVl)
{
  double f = theC3d.FirstParameter(), l = theC3d.LastParameter();
  int nPar = theParameters.size();
  for ( int isEnd1 = 0; isEnd1 < 2; ++isEnd1 )
  {
    const TopoDS_Vertex & V = isEnd1 ? theVf : theVl;
    const StdMeshers_SegmentLengthAroundVertex* hyp = getVertexHyp (theMesh, V );
    if ( hyp ) {
      double vertexLength = hyp->GetLength();
      if ( vertexLength > theLength / 2.0 )
        continue;
      if ( isEnd1 ) { // to have a segment of interest at end of theParameters
        theParameters.reverse();
        std::swap( f, l );
      }
      if ( _hypType == NB_SEGMENTS )
      {
        compensateError(0, vertexLength, f, l, theLength, theC3d, theParameters, true );
      }
      else if ( nPar <= 3 )
      {
        if ( !isEnd1 )
          vertexLength = -vertexLength;
        GCPnts_AbscissaPoint Discret(theC3d, vertexLength, l);
        if ( Discret.IsDone() ) {
          if ( nPar == 0 )
            theParameters.push_back( Discret.Parameter());
          else {
            double L = GCPnts_AbscissaPoint::Length( theC3d, theParameters.back(), l);
            if ( vertexLength < L / 2.0 )
              theParameters.push_back( Discret.Parameter());
            else
              compensateError(0, vertexLength, f, l, theLength, theC3d, theParameters, true );
          }
        }
      }
      else
      {
        // recompute params between the last segment and a middle one.
        // find size of a middle segment
        int nHalf = ( nPar-1 ) / 2;
        list< double >::reverse_iterator itU = theParameters.rbegin();
        std::advance( itU, nHalf );
        double Um = *itU++;
        double Lm = GCPnts_AbscissaPoint::Length( theC3d, Um, *itU);
        double L = GCPnts_AbscissaPoint::Length( theC3d, *itU, l);
        StdMeshers_Regular_1D algo( *this );
        algo._hypType = BEG_END_LENGTH;
        algo._value[ BEG_LENGTH_IND ] = Lm;
        algo._value[ END_LENGTH_IND ] = vertexLength;
        double from = *itU, to = l;
        if ( isEnd1 ) {
          std::swap( from, to );
          std::swap( algo._value[ BEG_LENGTH_IND ], algo._value[ END_LENGTH_IND ]);
        }
        list<double> params;
        if ( algo.computeInternalParameters( theMesh, theC3d, L, from, to, params, false ))
        {
          if ( isEnd1 ) params.reverse();
          while ( 1 + nHalf-- )
            theParameters.pop_back();
          theParameters.splice( theParameters.end(), params );
        }
        else
        {
          compensateError(0, vertexLength, f, l, theLength, theC3d, theParameters, true );
        }
      }
      if ( isEnd1 )
        theParameters.reverse();
    }
  }
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
bool StdMeshers_Regular_1D::computeInternalParameters(SMESH_Mesh &     theMesh,
                                                      Adaptor3d_Curve& theC3d,
                                                      double           theLength,
                                                      double           theFirstU,
                                                      double           theLastU,
                                                      list<double> &   theParams,
                                                      const bool       theReverse,
                                                      bool             theConsiderPropagation)
{
  theParams.clear();

  double f = theFirstU, l = theLastU;

  switch( _hypType )
  {
  case LOCAL_LENGTH:
  case MAX_LENGTH:
  case NB_SEGMENTS: {

    double eltSize = 1;
    if ( _hypType == MAX_LENGTH )
    {
      double nbseg = ceil(theLength / _value[ BEG_LENGTH_IND ]); // integer sup
      if (nbseg <= 0)
        nbseg = 1;                        // degenerated edge
      eltSize = theLength / nbseg;
    }
    else if ( _hypType == LOCAL_LENGTH )
    {
      // Local Length hypothesis
      double nbseg = ceil(theLength / _value[ BEG_LENGTH_IND ]); // integer sup

      // NPAL17873:
      bool isFound = false;
      if (theConsiderPropagation && !_mainEdge.IsNull()) // propagated from some other edge
      {
        // Advanced processing to assure equal number of segments in case of Propagation
        SMESH_subMesh* sm = theMesh.GetSubMeshContaining(_mainEdge);
        if (sm) {
          bool computed = sm->IsMeshComputed();
          if (!computed) {
            if (sm->GetComputeState() == SMESH_subMesh::READY_TO_COMPUTE) {
              sm->ComputeStateEngine(SMESH_subMesh::COMPUTE);
              computed = sm->IsMeshComputed();
            }
          }
          if (computed) {
            SMESHDS_SubMesh* smds = sm->GetSubMeshDS();
            int nb_segments = smds->NbElements();
            if (nbseg - 1 <= nb_segments && nb_segments <= nbseg + 1) {
              isFound = true;
              nbseg = nb_segments;
            }
          }
        }
      }
      if (!isFound) // not found by meshed edge in the propagation chain, use precision
      {
        double aPrecision = _value[ PRECISION_IND ];
        double nbseg_prec = ceil((theLength / _value[ BEG_LENGTH_IND ]) - aPrecision);
        if (nbseg_prec == (nbseg - 1)) nbseg--;
      }

      if (nbseg <= 0)
        nbseg = 1;                        // degenerated edge
      eltSize = theLength / nbseg;
    }
    else
    {
      // Number Of Segments hypothesis
      int NbSegm = _ivalue[ NB_SEGMENTS_IND ];
      if ( NbSegm < 1 )  return false;
      if ( NbSegm == 1 ) return true;

      switch (_ivalue[ DISTR_TYPE_IND ])
      {
      case StdMeshers_NumberOfSegments::DT_Scale:
        {
          double scale = _value[ SCALE_FACTOR_IND ];

          if (fabs(scale - 1.0) < Precision::Confusion()) {
            // special case to avoid division by zero
            for (int i = 1; i < NbSegm; i++) {
              double param = f + (l - f) * i / NbSegm;
              theParams.push_back( param );
            }
          } else {
            // general case of scale distribution
            if ( theReverse )
              scale = 1.0 / scale;

            double alpha = pow(scale, 1.0 / (NbSegm - 1));
            double factor = (l - f) / (1.0 - pow(alpha, NbSegm));

            for (int i = 1; i < NbSegm; i++) {
              double param = f + factor * (1.0 - pow(alpha, i));
              theParams.push_back( param );
            }
          }
          return true;
        }
        break;
      case StdMeshers_NumberOfSegments::DT_TabFunc:
        {
          FunctionTable func(_vvalue[ TAB_FUNC_IND ], _ivalue[ CONV_MODE_IND ]);
          return computeParamByFunc(theC3d, f, l, theLength, theReverse,
                                    _ivalue[ NB_SEGMENTS_IND ], func,
                                    theParams);
        }
        break;
      case StdMeshers_NumberOfSegments::DT_ExprFunc:
        {
          FunctionExpr func(_svalue[ EXPR_FUNC_IND ].c_str(), _ivalue[ CONV_MODE_IND ]);
          return computeParamByFunc(theC3d, f, l, theLength, theReverse,
                                    _ivalue[ NB_SEGMENTS_IND ], func,
                                    theParams);
        }
        break;
      case StdMeshers_NumberOfSegments::DT_Regular:
        eltSize = theLength / _ivalue[ NB_SEGMENTS_IND ];
        break;
      default:
        return false;
      }
    }
    GCPnts_UniformAbscissa Discret(theC3d, eltSize, f, l);
    if ( !Discret.IsDone() )
      return error( "GCPnts_UniformAbscissa failed");

    int NbPoints = Discret.NbPoints();
    for ( int i = 2; i < NbPoints; i++ )
    {
      double param = Discret.Parameter(i);
      theParams.push_back( param );
    }
    compensateError( eltSize, eltSize, f, l, theLength, theC3d, theParams ); // for PAL9899
    return true;
  }

  case BEG_END_LENGTH: {

    // geometric progression: SUM(n) = ( a1 - an * q ) / ( 1 - q ) = theLength

    double a1 = _value[ BEG_LENGTH_IND ];
    double an = _value[ END_LENGTH_IND ];
    double q  = ( theLength - a1 ) / ( theLength - an );

    double U1 = theReverse ? l : f;
    double Un = theReverse ? f : l;
    double param = U1;
    double eltSize = theReverse ? -a1 : a1;
    while ( 1 ) {
      // computes a point on a curve <theC3d> at the distance <eltSize>
      // from the point of parameter <param>.
      GCPnts_AbscissaPoint Discret( theC3d, eltSize, param );
      if ( !Discret.IsDone() ) break;
      param = Discret.Parameter();
      if ( f < param && param < l )
        theParams.push_back( param );
      else
        break;
      eltSize *= q;
    }
    compensateError( a1, an, U1, Un, theLength, theC3d, theParams );
    if (theReverse) theParams.reverse(); // NPAL18025
    return true;
  }

  case ARITHMETIC_1D: {

    // arithmetic progression: SUM(n) = ( an - a1 + q ) * ( a1 + an ) / ( 2 * q ) = theLength

    double a1 = _value[ BEG_LENGTH_IND ];
    double an = _value[ END_LENGTH_IND ];

    double  q = ( an - a1 ) / ( 2 *theLength/( a1 + an ) - 1 );
    int     n = int( 1 + ( an - a1 ) / q );

    double U1 = theReverse ? l : f;
    double Un = theReverse ? f : l;
    double param = U1;
    double eltSize = a1;
    if ( theReverse ) {
      eltSize = -eltSize;
      q = -q;
    }
    while ( n-- > 0 && eltSize * ( Un - U1 ) > 0 ) {
      // computes a point on a curve <theC3d> at the distance <eltSize>
      // from the point of parameter <param>.
      GCPnts_AbscissaPoint Discret( theC3d, eltSize, param );
      if ( !Discret.IsDone() ) break;
      param = Discret.Parameter();
      if ( param > f && param < l )
        theParams.push_back( param );
      else
        break;
      eltSize += q;
    }
    compensateError( a1, an, U1, Un, theLength, theC3d, theParams );
    if (theReverse) theParams.reverse(); // NPAL18025

    return true;
  }

  case DEFLECTION: {

    GCPnts_UniformDeflection Discret(theC3d, _value[ DEFLECTION_IND ], f, l, true);
    if ( !Discret.IsDone() )
      return false;

    int NbPoints = Discret.NbPoints();
    for ( int i = 2; i < NbPoints; i++ )
    {
      double param = Discret.Parameter(i);
      theParams.push_back( param );
    }
    return true;
  }

  default:;
  }

  return false;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

bool StdMeshers_Regular_1D::Compute(SMESH_Mesh & theMesh, const TopoDS_Shape & theShape)
{
  if ( _hypType == NONE )
    return false;

  SMESHDS_Mesh * meshDS = theMesh.GetMeshDS();

  const TopoDS_Edge & EE = TopoDS::Edge(theShape);
  TopoDS_Edge E = TopoDS::Edge(EE.Oriented(TopAbs_FORWARD));
  int shapeID = meshDS->ShapeToIndex( E );

  double f, l;
  Handle(Geom_Curve) Curve = BRep_Tool::Curve(E, f, l);

  TopoDS_Vertex VFirst, VLast;
  TopExp::Vertices(E, VFirst, VLast);   // Vfirst corresponds to f and Vlast to l

  ASSERT(!VFirst.IsNull());
  ASSERT(!VLast.IsNull());
  const SMDS_MeshNode * idFirst = SMESH_Algo::VertexNode( VFirst, meshDS );
  const SMDS_MeshNode * idLast = SMESH_Algo::VertexNode( VLast, meshDS );
  if (!idFirst || !idLast)
    return error( COMPERR_BAD_INPUT_MESH, "No node on vertex");

  if (!Curve.IsNull())
  {
    list< double > params;
    bool reversed = false;
    if ( !_mainEdge.IsNull() )
      reversed = ( _mainEdge.Orientation() == TopAbs_REVERSED );

    BRepAdaptor_Curve C3d( E );
    double length = EdgeLength( E );
    if ( ! computeInternalParameters( theMesh, C3d, length, f, l, params, reversed, true )) {
      return false;
    }
    redistributeNearVertices( theMesh, C3d, length, params, VFirst, VLast );

    // edge extrema (indexes : 1 & NbPoints) already in SMDS (TopoDS_Vertex)
    // only internal nodes receive an edge position with param on curve

    const SMDS_MeshNode * idPrev = idFirst;
    double parPrev = f;
    double parLast = l;

    /* NPAL18025
    if (reversed) {
      idPrev = idLast;
      idLast = idFirst;
      idFirst = idPrev;
      parPrev = l;
      parLast = f;
    }
    */

    for (list<double>::iterator itU = params.begin(); itU != params.end(); itU++) {
      double param = *itU;
      gp_Pnt P = Curve->Value(param);

      //Add the Node in the DataStructure
      SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
      meshDS->SetNodeOnEdge(node, shapeID, param);

      if(_quadraticMesh) {
        // create medium node
        double prm = ( parPrev + param )/2;
        gp_Pnt PM = Curve->Value(prm);
        SMDS_MeshNode * NM = meshDS->AddNode(PM.X(), PM.Y(), PM.Z());
        meshDS->SetNodeOnEdge(NM, shapeID, prm);
        SMDS_MeshEdge * edge = meshDS->AddEdge(idPrev, node, NM);
        meshDS->SetMeshElementOnShape(edge, shapeID);
      }
      else {
        SMDS_MeshEdge * edge = meshDS->AddEdge(idPrev, node);
        meshDS->SetMeshElementOnShape(edge, shapeID);
      }

      idPrev = node;
      parPrev = param;
    }
    if(_quadraticMesh) {
      double prm = ( parPrev + parLast )/2;
      gp_Pnt PM = Curve->Value(prm);
      SMDS_MeshNode * NM = meshDS->AddNode(PM.X(), PM.Y(), PM.Z());
      meshDS->SetNodeOnEdge(NM, shapeID, prm);
      SMDS_MeshEdge * edge = meshDS->AddEdge(idPrev, idLast, NM);
      meshDS->SetMeshElementOnShape(edge, shapeID);
    }
    else {
      SMDS_MeshEdge* edge = meshDS->AddEdge(idPrev, idLast);
      meshDS->SetMeshElementOnShape(edge, shapeID);
    }
  }
  else
  {
    //MESSAGE("************* Degenerated edge! *****************");

    // Edge is a degenerated Edge : We put n = 5 points on the edge.
    const int NbPoints = 5;
    BRep_Tool::Range( E, f, l ); // PAL15185
    double du = (l - f) / (NbPoints - 1);

    gp_Pnt P = BRep_Tool::Pnt(VFirst);

    const SMDS_MeshNode * idPrev = idFirst;
    for (int i = 2; i < NbPoints; i++) {
      double param = f + (i - 1) * du;
      SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
      if(_quadraticMesh) {
        // create medium node
        double prm = param - du/2.;
        SMDS_MeshNode * NM = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnEdge(NM, shapeID, prm);
        SMDS_MeshEdge * edge = meshDS->AddEdge(idPrev, node, NM);
        meshDS->SetMeshElementOnShape(edge, shapeID);
      }
      else {
        SMDS_MeshEdge * edge = meshDS->AddEdge(idPrev, node);
        meshDS->SetMeshElementOnShape(edge, shapeID);
      }
      meshDS->SetNodeOnEdge(node, shapeID, param);
      idPrev = node;
    }
    if(_quadraticMesh) {
      // create medium node
      double prm = l - du/2.;
      SMDS_MeshNode * NM = meshDS->AddNode(P.X(), P.Y(), P.Z());
      meshDS->SetNodeOnEdge(NM, shapeID, prm);
      SMDS_MeshEdge * edge = meshDS->AddEdge(idPrev, idLast, NM);
      meshDS->SetMeshElementOnShape(edge, shapeID);
    }
    else {
      SMDS_MeshEdge * edge = meshDS->AddEdge(idPrev, idLast);
      meshDS->SetMeshElementOnShape(edge, shapeID);
    }
  }
  return true;
}

//=============================================================================
/*!
 *  See comments in SMESH_Algo.cxx
 */
//=============================================================================

const list <const SMESHDS_Hypothesis *> &
StdMeshers_Regular_1D::GetUsedHypothesis(SMESH_Mesh &         aMesh,
                                         const TopoDS_Shape & aShape,
                                         const bool           ignoreAuxiliary)
{
  _usedHypList.clear();
  _mainEdge.Nullify();

  SMESH_HypoFilter auxiliaryFilter, compatibleFilter;
  auxiliaryFilter.Init( SMESH_HypoFilter::IsAuxiliary() );
  const bool ignoreAux = true;
  InitCompatibleHypoFilter( compatibleFilter, ignoreAux );

  // get non-auxiliary assigned to aShape
  int nbHyp = aMesh.GetHypotheses( aShape, compatibleFilter, _usedHypList, false );

  if (nbHyp == 0 && aShape.ShapeType() == TopAbs_EDGE)
  {
    // Check, if propagated from some other edge
    _mainEdge = StdMeshers_Propagation::GetPropagationSource( aMesh, aShape );
    if ( !_mainEdge.IsNull() )
    {
      // Propagation of 1D hypothesis from <aMainEdge> on this edge;
      // get non-auxiliary assigned to _mainEdge
      nbHyp = aMesh.GetHypotheses( _mainEdge, compatibleFilter, _usedHypList, true );
    }
  }

  if (nbHyp == 0) // nothing propagated nor assigned to aShape
  {
    SMESH_Algo::GetUsedHypothesis( aMesh, aShape, ignoreAuxiliary );
    nbHyp = _usedHypList.size();
  }
  else
  {
    // get auxiliary hyps from aShape
    aMesh.GetHypotheses( aShape, auxiliaryFilter, _usedHypList, true );
  }
  if ( nbHyp > 1 && ignoreAuxiliary )
    _usedHypList.clear(); //only one compatible non-auxiliary hypothesis allowed

  return _usedHypList;
}
