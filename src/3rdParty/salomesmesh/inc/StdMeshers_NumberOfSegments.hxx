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
//  File   : StdMeshers_NumberOfSegments.hxx
//           Moved here from SMESH_NumberOfSegments.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/StdMeshers/StdMeshers_NumberOfSegments.hxx,v 1.12.2.1 2008/11/27 13:03:50 abd Exp $
//
#ifndef _SMESH_NUMBEROFSEGMENTS_HXX_
#define _SMESH_NUMBEROFSEGMENTS_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_Hypothesis.hxx"
#include "SMESH_Exception.hxx"
#include <vector>

/*!
 * \brief This class represents hypothesis for 1d algorithm
 * 
 * It provides parameters for subdivision an edge by various
 * distribution types, considering the given number of resulting segments
 */
class STDMESHERS_EXPORT StdMeshers_NumberOfSegments:
  public SMESH_Hypothesis
{
public:
  StdMeshers_NumberOfSegments(int hypId, int studyId, SMESH_Gen* gen);
  virtual ~StdMeshers_NumberOfSegments();

  // Builds point distribution according to passed function
  const std::vector<double>& BuildDistributionExpr( const char*, int, int ) throw ( SMESH_Exception );
  const std::vector<double>& BuildDistributionTab( const std::vector<double>&, int, int ) throw ( SMESH_Exception );

  /*!
   * \brief Set the number of segments
    * \param segmentsNumber - must be greater than zero
   */
  void SetNumberOfSegments(int segmentsNumber)
    throw (SMESH_Exception);

  /*!
   * \brief Get the number of segments
   */
  int GetNumberOfSegments() const;

  /*!
   * \brief This enumeration presents available types of distribution
   */
  enum DistrType
  {
    DT_Regular, //!< equidistant distribution
    DT_Scale,   //!< scale distribution
    DT_TabFunc, //!< distribution with density function presented by table
    DT_ExprFunc //!< distribution with density function presented by expression
  };

  /*!
   * \brief Set distribution type
   */
  void SetDistrType(DistrType typ)
    throw (SMESH_Exception);

  /*!
   * \brief Get distribution type
   */
  DistrType GetDistrType() const;

  /*!
   * \brief Set scale factor for scale distribution
   * \param scaleFactor - positive value different from 1
   * 
   * Throws SMESH_Exception if distribution type is not DT_Scale,
   * or scaleFactor is not a positive value different from 1
   */
  virtual void SetScaleFactor(double scaleFactor)
    throw (SMESH_Exception);

  /*!
   * \brief Get scale factor for scale distribution
   * 
   * Throws SMESH_Exception if distribution type is not DT_Scale
   */
  double GetScaleFactor() const
    throw (SMESH_Exception);

  /*!
   * \brief Set table function for distribution DT_TabFunc
    * \param table - this vector contains the pairs (parameter, value)
   * following each by other, so the number of elements in the vector
   * must be even. The parameters must be in range [0,1] and sorted in
   * increase order. The values of function must be positive.
   * 
   * Throws SMESH_Exception if distribution type is not DT_TabFunc
   */
  void SetTableFunction(const std::vector<double>& table)
    throw (SMESH_Exception);

  /*!
   * \brief Get table function for distribution DT_TabFunc
   * 
   * Throws SMESH_Exception if distribution type is not DT_TabFunc
   */
  const std::vector<double>& GetTableFunction() const
    throw (SMESH_Exception);

  /*!
   * \brief Set expression function for distribution DT_ExprFunc
    * \param expr - string containing the expression of the function
    *               f(t), e.g. "sin(t)"
   * 
   * Throws SMESH_Exception if distribution type is not DT_ExprFunc
   */
  void SetExpressionFunction( const char* expr)
    throw (SMESH_Exception);

  /*!
   * \brief Get expression function for distribution DT_ExprFunc
   * 
   * Throws SMESH_Exception if distribution type is not DT_ExprFunc
   */
  const char* GetExpressionFunction() const
    throw (SMESH_Exception);

  /*!
   * \brief Set conversion mode. When it is 0, it means "exponent mode":
   * the function of distribution of density is used as an exponent of 10, i,e, 10^f(t).
   * When it is 1, it means "cut negative mode". The function of distribution is used as
   * F(t), where F(t0)=f(t0), if f(t0)>=0, otherwise F(t0) = 0.
   * This mode is sensible only when function distribution is used (DT_TabFunc or DT_ExprFunc)
   * 
   * Throws SMESH_Exception if distribution type is not functional
   */
  void SetConversionMode( int conv )
    throw (SMESH_Exception);

  /*!
   * \brief Returns conversion mode
   * 
   * Throws SMESH_Exception if distribution type is not functional
   */
  int ConversionMode() const
    throw (SMESH_Exception);


  /*!
   * \brief Initialize number of segments by the mesh built on the geometry
   * \param theMesh - the built mesh
   * \param theShape - the geometry of interest
   * \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

   /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

  virtual std::ostream & SaveTo(std::ostream & save);
  virtual std::istream & LoadFrom(std::istream & load);
  friend std::ostream& operator << (std::ostream & save, StdMeshers_NumberOfSegments & hyp);
  friend std::istream& operator >> (std::istream & load, StdMeshers_NumberOfSegments & hyp);

protected:
  int                 _numberOfSegments; //!< an edge will be split on to this number of segments
  DistrType           _distrType;        //!< the type of distribution of density function
  double              _scaleFactor;      //!< the scale parameter for DT_Scale
  std::vector<double> _table, _distr;    //!< the table for DT_TabFunc, a sequence of pairs of numbers
  std::string         _func;             //!< the expression of the function for DT_ExprFunc
  int                 _convMode;         //!< flag of conversion mode: 0=exponent, 1=cut negative
};

#endif
