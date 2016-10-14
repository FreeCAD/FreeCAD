// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SMESH DriverMED : driver to write a field to 'med' file
//  Module : SMESH
//
#ifndef _INCLUDE_DriverMED_W_Field
#define _INCLUDE_DriverMED_W_Field

#include "SMESH_DriverMED.hxx"

#include "Driver_SMESHDS_Mesh.h"
#include "SMDSAbs_ElementType.hxx"
#include "SMDS_ElemIterator.hxx"

#include <string>
#include <vector>

class MESHDRIVERMED_EXPORT DriverMED_W_Field: public Driver_SMESHDS_Mesh
{
 public:

  DriverMED_W_Field();

  void AddODOnVertices(bool toAdd) { _addODOnVertices = toAdd; }

  bool Set(SMESHDS_Mesh *      mesh,
           const std::string & fieldName,
           SMDSAbs_ElementType type,
           const int           nbComps,
           const bool          isIntData);

  void SetCompName(const int iComp, const char* name);

  void SetDtIt(const int dt, const int it);

  void AddValue( double val );
  void AddValue( int    val );

  /*
   * Returns elements in the order they are written in MED file. Result can be NULL!
   */
  SMDS_ElemIteratorPtr GetOrderedElems();

  /*
   * Add one field to the file
   */
  virtual Status Perform();

 private:

  std::string                _fieldName;
  SMDSAbs_ElementType        _elemType;
  std::vector< std::string > _compNames;
  std::vector< double >      _dblValues;
  std::vector< int >         _intValues;
  int                        _dt, _it;
  bool                       _addODOnVertices;

  std::vector< const SMDS_MeshElement* >              _elemsByGeom[SMDSEntity_Last];
  std::vector< std::pair< SMDSAbs_EntityType, int > > _nbElemsByGeom;
};

#endif
