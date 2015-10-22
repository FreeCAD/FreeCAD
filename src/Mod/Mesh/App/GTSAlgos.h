/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef _GTSAlgos_h_
#define _GTSAlgos_h_

#include <gts.h>
#include <vector>

#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include "Mesh.h"


namespace Mesh
{

/** The mesh algorithems container class
 */
class MeshExport GTSAlgos
{
public:
  //GTSAlgos(MeshCore::MeshKernel* Mesh);
  GTSAlgos(Mesh::MeshObject& Mesh):_Mesh(Mesh){};

  /** Coarsen the mesh
  */
  void coarsen(float f);

  /** makes a boolean add
   * The int Type stears the boolean oberation: 0=add;1=intersection;2=diff
  */
  void boolean(const Mesh::MeshObject& ToolMesh, int Type=0);

  /** Creates a GTS Surface from a MeshKernel
  */
  static GtsSurface* createGTSSurface(const Mesh::MeshObject& Mesh);

  /** Creates a GTS Surface from a MeshKernel
  */
  static void fillMeshFromGTSSurface(Mesh::MeshObject& Mesh, GtsSurface* pSurface);

private:

  Mesh::MeshObject& _Mesh;
};



} // namespace Mesh
#endif 
