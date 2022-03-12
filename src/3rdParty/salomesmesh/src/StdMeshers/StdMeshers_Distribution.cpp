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

//  SMESH StdMeshers : implementation of point distribution algorithm
//  File   : StdMeshers_Distribution.cxx
//  Author : Alexandre SOLOVYOV
//  Module : SMESH
//  $Header$
//
#include "StdMeshers_Distribution.hxx"

#include <math_GaussSingleIntegration.hxx>
#include <utilities.h>

#if (OCC_VERSION_MAJOR << 16 | OCC_VERSION_MINOR << 8 | OCC_VERSION_MAINTENANCE) > 0x060100
#define NO_CAS_CATCH
#endif

#include <Standard_Failure.hxx>
#include <Expr_NamedUnknown.hxx>

#ifdef NO_CAS_CATCH
#include <Standard_ErrorHandler.hxx>
#endif
#include <Expr_NamedUnknown.hxx>
using namespace std;

namespace StdMeshers {

Function::Function( const int conv )
: myConv( conv )
{
}

Function::~Function()
{
}

bool Function::value( const double, double& f ) const
{
  bool ok = true;
  if (myConv == 0) {
    try {
#ifdef NO_CAS_CATCH
      OCC_CATCH_SIGNALS;
#endif
      f = pow( 10., f );
    } catch(Standard_Failure&) {
      f = 0.0;
      ok = false;
    }
  }
  else if( myConv==1 && f<0.0 )
    f = 0.0;

  return ok;
}

FunctionIntegral::FunctionIntegral( const Function* f, const double st )
: Function( -1 ),
  myFunc( const_cast<Function*>( f ) ),
  myStart( st )
{
}

FunctionIntegral::~FunctionIntegral()
{
}

bool FunctionIntegral::value( const double t, double& f ) const
{
  f = myFunc ? myFunc->integral( myStart, t ) : 0;
  return myFunc!=0 && Function::value( t, f );
}

double FunctionIntegral::integral( const double, const double ) const
{
  return 0;
}

FunctionTable::FunctionTable( const std::vector<double>& data, const int conv )
: Function( conv )
{
  myData = data;
}

FunctionTable::~FunctionTable()
{
}

bool FunctionTable::value( const double t, double& f ) const
{
  int i1, i2;
  if( !findBounds( t, i1, i2 ) )
    return false;

  if( i1==i2 ) {
    f = myData[ 2*i1+1 ];
    Function::value( t, f );
    return true;
  }
      
  double
    x1 = myData[2*i1], y1 = myData[2*i1+1],
    x2 = myData[2*i2], y2 = myData[2*i2+1];

  Function::value( x1, y1 );
  Function::value( x2, y2 );
  
  f = y1 + ( y2-y1 ) * ( t-x1 ) / ( x2-x1 );
  return true;
}

double FunctionTable::integral( const int i ) const
{
  if( i>=0 && i<myData.size()-1 )
    return integral( i, myData[2*(i+1)]-myData[2*i] );
  else
    return 0;
}

double FunctionTable::integral( const int i, const double d ) const
{
  double f1,f2, res = 0.0;
  if( value( myData[2*i]+d, f1 ) )
    if(!value(myData[2*i], f2)) {
      f2 = myData[2*i+1];
      Function::value( 1, f2 );
    }
  res = (f2+f1) * d / 2.0;
  return res;
}

double FunctionTable::integral( const double a, const double b ) const
{
  int x1s, x1f, x2s, x2f;
  findBounds( a, x1s, x1f );
  findBounds( b, x2s, x2f );
  double J = 0;
  for( int i=x1s; i<x2s; i++ )
    J+=integral( i );
  J-=integral( x1s, a-myData[2*x1s] );
  J+=integral( x2s, b-myData[2*x2s] );
  return J;
}

bool FunctionTable::findBounds( const double x, int& x_ind_1, int& x_ind_2 ) const
{
  int n = myData.size() / 2;
  if( n==0 || x<myData[0] )
  {
    x_ind_1 = x_ind_2 = 0;
    return false;
  }

  for( int i=0; i<n-1; i++ )
    if( myData[2*i]<=x && x<myData[2*(i+1)] )
    {
      x_ind_1 = i;
      x_ind_2 = i+1;
      return true;
    }
  x_ind_1 = n-1;
  x_ind_2 = n-1;
  return ( fabs( x - myData[2*x_ind_2] ) < 1.e-10 );
}

FunctionExpr::FunctionExpr( const char* str, const int conv )
: Function( conv ),
  myVars( 1, 1 ),
  myValues( 1, 1 )
{
  bool ok = true;
  try {
#ifdef NO_CAS_CATCH
    OCC_CATCH_SIGNALS;
#endif
    myExpr = ExprIntrp_GenExp::Create();
    myExpr->Process( ( Standard_CString )str );
  } catch(Standard_Failure&) {
    ok = false;
  }

  if( !ok || !myExpr->IsDone() )
    myExpr.Nullify();

  myVars.ChangeValue( 1 ) = new Expr_NamedUnknown( "t" );
}

FunctionExpr::~FunctionExpr()
{
}

Standard_Boolean FunctionExpr::Value( const Standard_Real T, Standard_Real& F )
{
  double f;
  Standard_Boolean res = value( T, f );
  F = f;
  return res;
}

bool FunctionExpr::value( const double t, double& f ) const
{
  if( myExpr.IsNull() )
    return false;

  ( ( TColStd_Array1OfReal& )myValues ).ChangeValue( 1 ) = t;
  bool ok = true;
  try {
#ifdef NO_CAS_CATCH
    OCC_CATCH_SIGNALS;
#endif
    f = myExpr->Expression()->Evaluate( myVars, myValues );
  } catch(Standard_Failure&) {
    f = 0.0;
    ok = false;
  }

  ok = Function::value( t, f ) && ok;
  return ok;
}

double FunctionExpr::integral( const double a, const double b ) const
{
  double res = 0.0;
  try {
#ifdef NO_CAS_CATCH
    OCC_CATCH_SIGNALS;
#endif
    math_GaussSingleIntegration _int
      ( *static_cast<math_Function*>( const_cast<FunctionExpr*> (this) ), a, b, 20 );
    if( _int.IsDone() )
      res = _int.Value();
  } catch(Standard_Failure&) {
    res = 0.0;
    MESSAGE( "Exception in integral calculating" );
  }
  return res;
}

double dihotomySolve( Function& f, const double val, const double _start, const double _fin, const double eps, bool& ok )
{
  double start = _start, fin = _fin, start_val, fin_val; bool ok1, ok2;
  ok1 = f.value( start, start_val );
  ok2 = f.value( fin, fin_val );

  if( !ok1 || !ok2 )
  {
    ok = false;
    return 0.0;
  }

  bool start_pos = start_val>=val, fin_pos = fin_val>=val;
  ok = true;
  
  while( fin-start>eps )
  {
    double mid = ( start+fin )/2.0, mid_val;
    ok = f.value( mid, mid_val );
    if( !ok )
      return 0.0;

    //char buf[1024];
    //sprintf( buf, "start=%f\nfin=%f\nmid_val=%f\n", float( start ), float( fin ), float( mid_val ) );
    //MESSAGE( buf );

    bool mid_pos = mid_val>=val;
    if( start_pos!=mid_pos )
    {
      fin_pos = mid_pos;
      fin = mid;
    }
    else if( fin_pos!=mid_pos )
    {
      start_pos = mid_pos;
      start = mid;
    }
    else
    {
      ok = false;
      break;
    }
  }
  return (start+fin)/2.0;
}

bool buildDistribution( const TCollection_AsciiString& f, const int conv, const double start, const double end,
                        const int nbSeg, vector<double>& data, const double eps )
{
  FunctionExpr F( f.ToCString(), conv );
  return buildDistribution( F, start, end, nbSeg, data, eps );
}

bool buildDistribution( const std::vector<double>& f, const int conv, const double start, const double end,
                        const int nbSeg, vector<double>& data, const double eps )
{
  FunctionTable F( f, conv );
  return buildDistribution( F, start, end, nbSeg, data, eps );
}

bool buildDistribution( const Function& func, const double start, const double end, const int nbSeg,
                        vector<double>& data, const double eps )
{
  if( nbSeg<=0 )
    return false;

  data.resize( nbSeg+1 );
  data[0] = start;
  double J = func.integral( start, end ) / nbSeg;
  if( J<1E-10 )
    return false;

  bool ok;
  //MESSAGE( "distribution:" );
  //char buf[1024];
  for( int i=1; i<nbSeg; i++ )
  {
    FunctionIntegral f_int( &func, data[i-1] );
    data[i] = dihotomySolve( f_int, J, data[i-1], end, eps, ok );
    //sprintf( buf, "%f\n", float( data[i] ) );
    //MESSAGE( buf );
    if( !ok )
      return false;
  }

  data[nbSeg] = end;
  return true;
}
}
