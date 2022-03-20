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
//  File   : StdMeshers_Distribution.hxx
//  Author : Alexandre SOLOVYOV
//  Module : SMESH
//  $Header$
//
#ifndef _STD_MESHERS_DISTRIBUTION_HXX_
#define _STD_MESHERS_DISTRIBUTION_HXX_

#include "SMESH_StdMeshers.hxx"

#include <vector>
#include <math_Function.hxx>
#include <ExprIntrp_GenExp.hxx>
#include <Expr_Array1OfNamedUnknown.hxx>
#include <TColStd_Array1OfReal.hxx>

namespace StdMeshers
{
class STDMESHERS_EXPORT Function 
{
public:
  Function( const int );
  virtual ~Function();
  virtual bool   value( const double, double& ) const;
  virtual double integral( const double, const double ) const = 0;

private:
  int myConv;
};

class STDMESHERS_EXPORT FunctionIntegral : public Function
{
public:
  FunctionIntegral( const Function*, const double );
  virtual ~FunctionIntegral();
  virtual bool   value( const double, double& ) const;
  virtual double integral( const double, const double ) const;

private:
  Function* myFunc;
  double    myStart;
};

class STDMESHERS_EXPORT FunctionTable : public Function
{
public:
  FunctionTable( const std::vector<double>&, const int );
  virtual ~FunctionTable();
  virtual bool   value( const double, double& ) const;
  virtual double integral( const double, const double ) const;

private:
  bool    findBounds( const double, int&, int& ) const;

  //integral from x[i] to x[i+1]
  double  integral( const int i ) const;

  //integral from x[i] to x[i]+d
  //warning: function is presented as linear on interaval from x[i] to x[i]+d,
  //         for correct result d must be >=0 and <=x[i+1]-x[i]
  double  integral( const int i, const double d ) const;

private:
  std::vector<double>  myData;
};

class STDMESHERS_EXPORT FunctionExpr : public Function, public math_Function
{
public:
  FunctionExpr( const char*, const int );
  virtual ~FunctionExpr();
  virtual Standard_Boolean Value( const Standard_Real, Standard_Real& );
  virtual bool   value( const double, double& ) const;
  virtual double integral( const double, const double ) const;

private:
  Handle(ExprIntrp_GenExp)    myExpr;
  Expr_Array1OfNamedUnknown   myVars;
  TColStd_Array1OfReal        myValues;
};

STDMESHERS_EXPORT
bool buildDistribution( const Function& f,
                        const double start, const double end,
                        const int nbSeg,
                        std::vector<double>& data,
                        const double eps );

STDMESHERS_EXPORT
bool buildDistribution( const TCollection_AsciiString& f, const int conv, const double start, const double end,
                        const int nbSeg, std::vector<double>& data, const double eps );
STDMESHERS_EXPORT
bool buildDistribution( const std::vector<double>& f, const int conv, const double start, const double end,
                        const int nbSeg, std::vector<double>& data, const double eps );
}
#endif
