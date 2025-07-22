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

//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_Hypothesis.hxx
// Author    : Michael Sazonov (OCN)
// Date      : 27/03/2006
// Project   : SALOME
//
#ifndef _NETGENPlugin_Hypothesis_HXX_
#define _NETGENPlugin_Hypothesis_HXX_

#include "NETGENPlugin_Defs.hxx"

#include "SMESH_Hypothesis.hxx"
#include "Utils_SALOME_Exception.hxx"

#include <map>

//  Parameters for work of NETGEN
//

using namespace std;

class NETGENPLUGIN_EXPORT NETGENPlugin_Hypothesis: public SMESH_Hypothesis
{
public:

  NETGENPlugin_Hypothesis(int hypId, int studyId, SMESH_Gen * gen);

  void SetMaxSize(double theSize);
  double GetMaxSize() const { return _maxSize; }

  void SetMinSize(double theSize);
  double GetMinSize() const { return _minSize; }

  void SetSecondOrder(bool theVal);
  bool GetSecondOrder() const { return _secondOrder; }

  void SetOptimize(bool theVal);
  bool GetOptimize() const { return _optimize; }

  enum Fineness
  {
    VeryCoarse,
    Coarse,
    Moderate,
    Fine,
    VeryFine,
    UserDefined
  };

  void SetFineness(Fineness theFineness);
  Fineness GetFineness() const { return _fineness; }

  // the following parameters are controlled by Fineness

  void SetGrowthRate(double theRate);
  double GetGrowthRate() const { return _growthRate; }

  void SetNbSegPerEdge(double theVal);
  double GetNbSegPerEdge() const { return _nbSegPerEdge; }

  void SetNbSegPerRadius(double theVal);
  double GetNbSegPerRadius() const { return _nbSegPerRadius; }

  typedef std::map<std::string, double> TLocalSize;
  static TLocalSize GetDefaultLocalSize() { return TLocalSize(); }
  void SetLocalSizeOnEntry(const std::string& entry, double localSize);
  double GetLocalSizeOnEntry(const std::string& entry);
  const TLocalSize& GetLocalSizesAndEntries() const { return _localSize; }
  void UnsetLocalSizeOnEntry(const std::string& entry);

  void SetQuadAllowed(bool theVal);
  bool GetQuadAllowed() const { return _quadAllowed; }

  void SetSurfaceCurvature(bool theVal);
  bool GetSurfaceCurvature() const { return _surfaceCurvature; }

  void SetFuseEdges(bool theVal);
  bool GetFuseEdges() const { return _fuseEdges; }

  // the default values (taken from NETGEN 4.5 sources)

  static double GetDefaultMaxSize();
  static Fineness GetDefaultFineness();
  static double GetDefaultGrowthRate();
  static double GetDefaultNbSegPerEdge();
  static double GetDefaultNbSegPerRadius();
  static bool GetDefaultSecondOrder();
  static bool GetDefaultOptimize();
  static bool GetDefaultQuadAllowed();
  static bool GetDefaultSurfaceCurvature();
  static bool GetDefaultFuseEdges();

  // Persistence
  virtual ostream & SaveTo(ostream & save);
  virtual istream & LoadFrom(istream & load);
  friend NETGENPLUGIN_EXPORT ostream & operator <<(ostream & save, NETGENPlugin_Hypothesis & hyp);
  friend NETGENPLUGIN_EXPORT istream & operator >>(istream & load, NETGENPlugin_Hypothesis & hyp);

  /*!
   * \brief Does nothing
   * \param theMesh - the built mesh
   * \param theShape - the geometry of interest
   * \retval bool - always false
   */
  virtual bool SetParametersByMesh(const SMESH_Mesh* theMesh, const TopoDS_Shape& theShape);

  /*!
   * \brief Initialize my parameter values by default parameters.
   *  \retval bool - true if parameter values have been successfully defined
   */
  virtual bool SetParametersByDefaults(const TDefaults& dflts, const SMESH_Mesh* theMesh=0);

private:
  double        _maxSize, _minSize;
  double        _growthRate;
  double        _nbSegPerEdge;
  double        _nbSegPerRadius;
  Fineness      _fineness;
  bool          _secondOrder;
  bool          _optimize;
  TLocalSize    _localSize;
  bool          _quadAllowed;
  bool          _surfaceCurvature;
  bool          _fuseEdges;
};

#endif
