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
//  NETGENPlugin : C++ implementation
// File      : NETGENPlugin_Mesher.hxx
// Author    : Michael Sazonov (OCN)
// Date      : 31/03/2006
// Project   : SALOME
// $Header: /home/server/cvs/NETGENPLUGIN/NETGENPLUGIN_SRC/src/NETGENPlugin/NETGENPlugin_Mesher.hxx,v 1.4.2.2 2008/11/27 14:29:44 abd Exp $
//=============================================================================
//
#ifndef _NETGENPlugin_Mesher_HXX_
#define _NETGENPlugin_Mesher_HXX_

#include "NETGENPlugin_Defs.hxx"
#include "StdMeshers_FaceSide.hxx"
#include <map>

class SMESH_Mesh;
class SMESHDS_Mesh;
class TopoDS_Shape;
class NETGENPlugin_Hypothesis;
class NETGENPlugin_SimpleHypothesis_2D;
namespace netgen {
  class OCCGeometry;
  class Mesh;
}

/*!
 * \brief This class calls the NETGEN mesher of OCC geometry
 */

class NETGENPLUGIN_EXPORT NETGENPlugin_Mesher 
{
 public:
  // ---------- PUBLIC METHODS ----------

  NETGENPlugin_Mesher (SMESH_Mesh* mesh, const TopoDS_Shape& aShape,
                       const bool isVolume);

  void SetParameters(const NETGENPlugin_Hypothesis* hyp);
  void SetParameters(const NETGENPlugin_SimpleHypothesis_2D* hyp);

  bool Compute();

  static void PrepareOCCgeometry(netgen::OCCGeometry&          occgeom,
                                 const TopoDS_Shape&           shape,
                                 SMESH_Mesh&                   mesh,
                                 std::list< SMESH_subMesh* > * meshedSM=0);

  static void RemoveTmpFiles();

protected:

  bool fillNgMesh(netgen::OCCGeometry&                occgeom,
                  netgen::Mesh&                       ngMesh,
                  std::vector<SMDS_MeshNode*>&        nodeVec,
                  const std::list< SMESH_subMesh* > & meshedSM);

  void defaultParameters();


 private:
  SMESH_Mesh*          _mesh;
  const TopoDS_Shape&  _shape;
  bool                 _isVolume;
  bool                 _optimize;

  const NETGENPlugin_SimpleHypothesis_2D * _simpleHyp;
  std::map< int, std::pair<int,int> >      _faceDescriptors;
};

#endif
