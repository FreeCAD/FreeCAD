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
//  File   : StdMeshers_NumberOfSegments.cxx
//           Moved here from SMESH_NumberOfSegments.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_NumberOfSegments.hxx"

#include "StdMeshers_Distribution.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_Comment.hxx"

#include <ExprIntrp_GenExp.hxx>
#include <Expr_Array1OfNamedUnknown.hxx>
#include <Expr_NamedUnknown.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TCollection_AsciiString.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#if (OCC_VERSION_MAJOR << 16 | OCC_VERSION_MINOR << 8 | OCC_VERSION_MAINTENANCE) > 0x060100
#define NO_CAS_CATCH
#endif

#include <Standard_Failure.hxx>

#ifdef NO_CAS_CATCH
#include <Standard_ErrorHandler.hxx>
#endif

#include <Basics_Utils.hxx>

using namespace StdMeshers;
using namespace std;

const double PRECISION = 1e-7;

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_NumberOfSegments::StdMeshers_NumberOfSegments(int         hypId,
                                                         int         studyId,
                                                         SMESH_Gen * gen)
  : SMESH_Hypothesis(hypId, studyId, gen),
    _numberOfSegments(15),//issue 19923
    _distrType(DT_Regular),
    _scaleFactor(1.),
    _convMode(1)  //cut negative by default
{
  _name = "NumberOfSegments";
  _param_algo_dim = 1;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

StdMeshers_NumberOfSegments::~StdMeshers_NumberOfSegments()
{
}

//=============================================================================
/*!
 *  
 */
//=============================================================================
const vector<double>&
StdMeshers_NumberOfSegments::BuildDistributionExpr( const char* expr,int nbSeg,int conv )
{
  if( !buildDistribution( TCollection_AsciiString( ( Standard_CString )expr ), conv, 0.0, 1.0, nbSeg, _distr, 1E-4 ) )
    _distr.resize( 0 );
  return _distr;
}

const vector<double>&
StdMeshers_NumberOfSegments::BuildDistributionTab( const vector<double>& tab,
                                                   int nbSeg,
                                                   int conv )
{
  if( !buildDistribution( tab, conv, 0.0, 1.0, nbSeg, _distr, 1E-4 ) )
    _distr.resize( 0 );
  return _distr;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_NumberOfSegments::SetNumberOfSegments(int segmentsNumber)
{
  int oldNumberOfSegments = _numberOfSegments;
  if (segmentsNumber <= 0)
    throw SALOME_Exception(LOCALIZED("number of segments must be positive"));
  _numberOfSegments = segmentsNumber;

  if (oldNumberOfSegments != _numberOfSegments)
    NotifySubMeshesHypothesisModification();
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

int StdMeshers_NumberOfSegments::GetNumberOfSegments() const
{
  return _numberOfSegments;
}

//================================================================================
/*!
 * 
 */
//================================================================================

void StdMeshers_NumberOfSegments::SetDistrType(DistrType typ)
{
  if (typ < DT_Regular || typ > DT_ExprFunc)
    throw SALOME_Exception(LOCALIZED("distribution type is out of range"));

  if (typ != _distrType)
  {
    _distrType = typ;
    NotifySubMeshesHypothesisModification();
  }
}

//================================================================================
/*!
 * 
 */
//================================================================================

StdMeshers_NumberOfSegments::DistrType StdMeshers_NumberOfSegments::GetDistrType() const
{
  return _distrType;
}

//================================================================================
/*!
 * 
 */
//================================================================================

void StdMeshers_NumberOfSegments::SetScaleFactor(double scaleFactor)
{
  if (_distrType != DT_Scale)
    _distrType = DT_Scale;
    //throw SALOME_Exception(LOCALIZED("not a scale distribution"));
  if (scaleFactor < PRECISION)
    throw SALOME_Exception(LOCALIZED("scale factor must be positive"));
  if (fabs(scaleFactor - 1.0) < PRECISION)
    _distrType = DT_Regular;

  if (fabs(_scaleFactor - scaleFactor) > PRECISION)
  {
    _scaleFactor = scaleFactor;
    NotifySubMeshesHypothesisModification();
  }
}

//================================================================================
/*!
 * 
 */
//================================================================================

double StdMeshers_NumberOfSegments::GetScaleFactor() const
{
  if (_distrType != DT_Scale)
    throw SALOME_Exception(LOCALIZED("not a scale distribution"));
  return _scaleFactor;
}

//================================================================================
/*!
 * 
 */
//================================================================================

void StdMeshers_NumberOfSegments::SetTableFunction(const vector<double>& table)
{
  if (_distrType != DT_TabFunc)
    _distrType = DT_TabFunc;
  //throw SALOME_Exception(LOCALIZED("not a table function distribution"));
  if ( (table.size() % 2) != 0 )
    throw SALOME_Exception(LOCALIZED("odd size of vector of table function"));

  int i;
  double prev = -PRECISION;
  bool isSame = table.size() == _table.size();

  bool pos = false;
  for (i=0; i < table.size()/2; i++) {
    double par = table[i*2];
    double val = table[i*2+1];
    if( _convMode==0 )
    {
      try {
#ifdef NO_CAS_CATCH
        OCC_CATCH_SIGNALS;
#endif
        val = pow( 10.0, val );
      } catch(Standard_Failure&) {
        throw SALOME_Exception( LOCALIZED( "invalid value"));
        return;
      }
    }
    else if( _convMode==1 && val<0.0 )
      val = 0.0;

    if ( par<0 || par > 1)
      throw SALOME_Exception(LOCALIZED("parameter of table function is out of range [0,1]"));
    if ( fabs(par-prev)<PRECISION )
      throw SALOME_Exception(LOCALIZED("two parameters are the same"));
    if ( val < 0 )
      throw SALOME_Exception(LOCALIZED("value of table function is not positive"));
    if( val>PRECISION )
      pos = true;
    if (isSame)
    {
      double oldpar = _table[i*2];
      double oldval = _table[i*2+1];
      if (fabs(par - oldpar) > PRECISION || fabs(val - oldval) > PRECISION)
        isSame = false;
    }
    prev = par;
  }

  if( !pos )
    throw SALOME_Exception(LOCALIZED("value of table function is not positive"));

  if( pos && !isSame )
  {
    _table = table;
    NotifySubMeshesHypothesisModification();
  }
}

//================================================================================
/*!
 * 
 */
//================================================================================

const vector<double>& StdMeshers_NumberOfSegments::GetTableFunction() const
{
  if (_distrType != DT_TabFunc)
    throw SALOME_Exception(LOCALIZED("not a table function distribution"));
  return _table;
}

//================================================================================
/*! check if only 't' is unknown variable in expression
 */
//================================================================================
bool isCorrectArg( const Handle( Expr_GeneralExpression )& expr )
{
  Handle( Expr_NamedUnknown ) sub = Handle( Expr_NamedUnknown )::DownCast( expr );
  if( !sub.IsNull() )
    return sub->GetName()=="t";

  bool res = true;
  for( int i=1, n=expr->NbSubExpressions(); i<=n && res; i++ )
  {
    Handle( Expr_GeneralExpression ) sub = expr->SubExpression( i );
    Handle( Expr_NamedUnknown ) name = Handle( Expr_NamedUnknown )::DownCast( sub );
    if( !name.IsNull() )
    {
      if( name->GetName()!="t" )
        res = false;
    }
    else
      res = isCorrectArg( sub );
  }
  return res;
}

//================================================================================
/*! this function parses the expression 'str' in order to check if syntax is correct
 *  ( result in 'syntax' ) and if only 't' is unknown variable in expression ( result in 'args' )
 */
//================================================================================
bool process( const TCollection_AsciiString& str, int convMode,
              bool& syntax, bool& args,
              bool& non_neg, bool& non_zero,
              bool& singulars, double& sing_point )
{
  Kernel_Utils::Localizer loc;

  bool parsed_ok = true;
  Handle( ExprIntrp_GenExp ) myExpr;
  try {
#ifdef NO_CAS_CATCH
    OCC_CATCH_SIGNALS;
#endif
    myExpr = ExprIntrp_GenExp::Create();
    myExpr->Process( str.ToCString() );
  } catch(Standard_Failure&) {
    parsed_ok = false;
  }

  syntax = false;
  args = false;
  if( parsed_ok && myExpr->IsDone() )
  {
    syntax = true;
    args = isCorrectArg( myExpr->Expression() );
  }

  bool res = parsed_ok && syntax && args;
  if( !res )
    myExpr.Nullify();

  non_neg = true;
  singulars = false;
  non_zero = false;

  if( res )
  {
    FunctionExpr f( str.ToCString(), convMode );
    const int max = 500;
    for( int i=0; i<=max; i++ )
    {
      double t = double(i)/double(max), val;
      if( !f.value( t, val ) )
      {
        sing_point = t;
        singulars = true;
        break;
      }
      if( val<0 )
      {
        non_neg = false;
        break;
      }
      if( val>PRECISION )
        non_zero = true;
    }
  }

  return res && non_neg && non_zero && ( !singulars );
}

//================================================================================
/*!
 * 
 */
//================================================================================

void StdMeshers_NumberOfSegments::SetExpressionFunction(const char* expr)
{
  if (_distrType != DT_ExprFunc)
    _distrType = DT_ExprFunc;
    //throw SALOME_Exception(LOCALIZED("not an expression function distribution"));

  string func = CheckExpressionFunction( expr, _convMode );
  if( _func != func )
  {
    _func = func;
    NotifySubMeshesHypothesisModification();
  }
}

//=======================================================================
//function : CheckExpressionFunction
//purpose  : Checks validity of  the expression of the function f(t), e.g. "sin(t)".
//           In case of validity returns a cleaned expression
//=======================================================================

std::string
StdMeshers_NumberOfSegments::CheckExpressionFunction( const std::string& expr,
                                                      const int          convMode)
{
  // remove white spaces
  TCollection_AsciiString str((Standard_CString)expr.c_str());
  str.RemoveAll(' ');
  str.RemoveAll('\t');
  str.RemoveAll('\r');
  str.RemoveAll('\n');

  bool syntax, args, non_neg, singulars, non_zero;
  double sing_point;
  bool res = process( str, convMode, syntax, args, non_neg, non_zero, singulars, sing_point );
  if( !res )
  {
    if( !syntax )
      throw SALOME_Exception(SMESH_Comment("invalid expression syntax: ") << str );
    if( !args )
      throw SALOME_Exception(LOCALIZED("only 't' may be used as function argument"));
    if( !non_neg )
      throw SALOME_Exception(LOCALIZED("only non-negative function can be used"));
    if( singulars )
    {
      char buf[1024];
      sprintf( buf, "Function has singular point in %.3f", sing_point );
      throw SALOME_Exception( buf );
    }
    if( !non_zero )
      throw SALOME_Exception(LOCALIZED("f(t)=0 cannot be used"));
  }
 
  return str.ToCString();
}

//================================================================================
/*!
 * 
 */
//================================================================================

const char* StdMeshers_NumberOfSegments::GetExpressionFunction() const
{
  if (_distrType != DT_ExprFunc)
    throw SALOME_Exception(LOCALIZED("not an expression function distribution"));
  return _func.c_str();
}

//================================================================================
/*!
 * 
 */
//================================================================================

void StdMeshers_NumberOfSegments::SetConversionMode( int conv )
{
//   if (_distrType != DT_TabFunc && _distrType != DT_ExprFunc)
//     throw SALOME_Exception(LOCALIZED("not a functional distribution"));

  if( conv != _convMode )
  {
    _convMode = conv;
    NotifySubMeshesHypothesisModification();
  }
}

//================================================================================
/*!
 * 
 */
//================================================================================

int StdMeshers_NumberOfSegments::ConversionMode() const
{
//   if (_distrType != DT_TabFunc && _distrType != DT_ExprFunc)
//     throw SALOME_Exception(LOCALIZED("not a functional distribution"));
  return _convMode;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & StdMeshers_NumberOfSegments::SaveTo(ostream & save)
{
  int listSize = _edgeIDs.size();
  save << _numberOfSegments << " " << (int)_distrType;
  switch (_distrType)
  {
  case DT_Scale:
    save << " " << _scaleFactor;
    break;
  case DT_TabFunc:
    int i;
    save << " " << _table.size();
    for (i=0; i < _table.size(); i++)
      save << " " << _table[i];
    break;
  case DT_ExprFunc:
    save << " " << _func;
    break;
  case DT_Regular:
  default:
    break;
  }

  if (_distrType == DT_TabFunc || _distrType == DT_ExprFunc)
    save << " " << _convMode;

  if ( _distrType != DT_Regular && listSize > 0 ) {
    save << " " << listSize;
    for ( int i = 0; i < listSize; i++ )
      save << " " << _edgeIDs[i];
    save << " " << _objEntry;
  }
  
  return save;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & StdMeshers_NumberOfSegments::LoadFrom(istream & load)
{
  bool isOK = true;
  int a;

  // read number of segments
  isOK = (bool)(load >> a);
  if (isOK)
    _numberOfSegments = a;
  else
    load.clear(ios::badbit | load.rdstate());

  // read second stored value. It can be two variants here:
  // 1. If the hypothesis is stored in old format (nb.segments and scale factor),
  //    we wait here the scale factor, which is double.
  // 2. If the hypothesis is stored in new format
  //    (nb.segments, distr.type, some other params.),
  //    we wait here the ditribution type, which is integer
  double scale_factor;
  isOK = (bool)(load >> scale_factor);
  a = (int)scale_factor;

  // try to interprete ditribution type,
  // supposing that this hypothesis was written in the new format
  if (isOK)
  {
    if (a < DT_Regular || a > DT_ExprFunc)
      _distrType = DT_Regular;
    else
      _distrType = (DistrType) a;
  }
  else
    load.clear(ios::badbit | load.rdstate());

  // parameters of distribution
  double b;
  switch (_distrType)
  {
  case DT_Scale:
    {
      isOK = (bool)(load >> b);
      if (isOK)
        _scaleFactor = b;
      else
      {
        load.clear(ios::badbit | load.rdstate());
        // this can mean, that the hypothesis is stored in old format
        _distrType = DT_Regular;
        _scaleFactor = scale_factor;
      }
    }
    break;
  case DT_TabFunc:
    {
      isOK = (bool)(load >> a);
      if (isOK)
      {
        _table.resize(a, 0.);
        int i;
        for (i=0; i < _table.size(); i++)
        {
          isOK = (bool)(load >> b);
          if (isOK)
            _table[i] = b;
          else
            load.clear(ios::badbit | load.rdstate());
        }
      }
      else
      {
        load.clear(ios::badbit | load.rdstate());
        // this can mean, that the hypothesis is stored in old format
        _distrType = DT_Regular;
        _scaleFactor = scale_factor;
      }
    }
    break;
  case DT_ExprFunc:
    {
      string str;
      isOK = (bool)(load >> str);
      if (isOK)
        _func = str;
      else
      {
        load.clear(ios::badbit | load.rdstate());
        // this can mean, that the hypothesis is stored in old format
        _distrType = DT_Regular;
        _scaleFactor = scale_factor;
      }
    }
    break;
  case DT_Regular:
  default:
    break;
  }

  if (_distrType == DT_TabFunc || _distrType == DT_ExprFunc)
  {
    isOK = (bool)(load >> a);
    if (isOK)
      _convMode = a;
    else
      load.clear(ios::badbit | load.rdstate());
  }

  // load reversed edges IDs
  int intVal;
  isOK = (bool)(load >> intVal);
  if ( isOK && _distrType != DT_Regular && intVal > 0 ) {
    _edgeIDs.reserve( intVal );
    for (int i = 0; i < _edgeIDs.capacity() && isOK; i++) {
      isOK = (bool)(load >> intVal);
      if ( isOK ) _edgeIDs.push_back( intVal );
    }
    isOK = (bool)(load >> _objEntry);
  }

  return load;
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

ostream & operator <<(ostream & save, StdMeshers_NumberOfSegments & hyp)
{
  return hyp.SaveTo( save );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

istream & operator >>(istream & load, StdMeshers_NumberOfSegments & hyp)
{
  return hyp.LoadFrom( load );
}

//================================================================================
/*!
 * \brief Initialize number of segments by the mesh built on the geometry
 * \param theMesh - the built mesh
 * \param theShape - the geometry of interest
 * \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_NumberOfSegments::SetParametersByMesh(const SMESH_Mesh*   theMesh,
                                                      const TopoDS_Shape& theShape)
{
  if ( !theMesh || theShape.IsNull() )
    return false;

  _numberOfSegments = 0;
  _distrType = DT_Regular;

  int nbEdges = 0;
  TopTools_IndexedMapOfShape edgeMap;
  TopExp::MapShapes( theShape, TopAbs_EDGE, edgeMap );
  SMESHDS_Mesh* aMeshDS = const_cast< SMESH_Mesh* >( theMesh )->GetMeshDS();
  for ( int i = 1; i <= edgeMap.Extent(); ++i )
  {
    // get current segment length
    SMESHDS_SubMesh * eSubMesh = aMeshDS->MeshElements( edgeMap( i ));
    if ( eSubMesh && eSubMesh->NbElements())
      _numberOfSegments += eSubMesh->NbElements();

    ++nbEdges;
  }
  if ( nbEdges )
    _numberOfSegments /= nbEdges;

  if (_numberOfSegments == 0) _numberOfSegments = 1;

  return nbEdges;
}
//================================================================================
/*!
 * \brief Initialize my parameter values by default parameters.
 *  \retval bool - true if parameter values have been successfully defined
 */
//================================================================================

bool StdMeshers_NumberOfSegments::SetParametersByDefaults(const TDefaults&  dflts,
                                                          const SMESH_Mesh* /*theMesh*/)
{
  return (_numberOfSegments = dflts._nbSegments );
}

//=============================================================================
/*!
 *  
 */
//=============================================================================

void StdMeshers_NumberOfSegments::SetReversedEdges( std::vector<int>& ids )
{
  if ( ids != _edgeIDs ) {
    _edgeIDs = ids;

    NotifySubMeshesHypothesisModification();
  }
}

