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
// File      : DriverGMF_Write.hxx
// Created   : Mon Sep 17 15:36:47 2012
// Author    : Edward AGAPOV (eap)


#ifndef __DriverGMF_Write_HXX__
#define __DriverGMF_Write_HXX__

#include "SMESH_DriverGMF.hxx"

#include "Driver_SMESHDS_Mesh.h"
#include "SMDSAbs_ElementType.hxx"
#include "SMDS_ElemIterator.hxx"

#include <gp_Pnt.hxx>

/*!
 * \brief Class for storing control points for writing GMF size maps
 */
class MESHDriverGMF_EXPORT Control_Pnt : public gp_Pnt
{
public:
  Control_Pnt();
  Control_Pnt(const gp_Pnt& aPnt, double theSize);
  Control_Pnt(double x, double y, double z);
  Control_Pnt(double x, double y, double z, double size);

  double Size() const { return size; };
  void SetSize( double theSize ) { size = theSize; };
  
private:
  double size;
};

/*!
 * \brief Driver Writing a mesh into a GMF file.
 */
class MESHDriverGMF_EXPORT DriverGMF_Write : public Driver_SMESHDS_Mesh
{
public:

  DriverGMF_Write();
  ~DriverGMF_Write();

  void SetExportRequiredGroups( bool toExport )
  {
    _exportRequiredGroups = toExport;
  }
   
  virtual Status Perform();
  
  // Size Maps
  Status PerformSizeMap( const std::vector<Control_Pnt>& points );
  void SetSizeMapPrefix( std::string prefix )
  {
    myVerticesFile = prefix + ".mesh";
    mySolFile = prefix + ".sol";
  };
  std::vector<std::string> GetSizeMapFiles();

 private:

  SMDS_ElemIteratorPtr elementIterator(SMDSAbs_ElementType  type); 
  SMDS_ElemIteratorPtr elementIterator(SMDSAbs_EntityType   type);  
  SMDS_ElemIteratorPtr elementIterator(SMDSAbs_GeometryType type);

  bool _exportRequiredGroups;
  std::string myVerticesFile;
  std::string mySolFile;
};

#endif
