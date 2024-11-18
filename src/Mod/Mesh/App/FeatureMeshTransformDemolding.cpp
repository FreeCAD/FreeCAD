/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"

#include "FeatureMeshTransformDemolding.h"


using namespace Mesh;
using namespace MeshCore;

PROPERTY_SOURCE(Mesh::TransformDemolding, Mesh::Transform)


TransformDemolding::TransformDemolding()
{
    ADD_PROPERTY(Source, (nullptr));
    ADD_PROPERTY(Rotation, (0.0));
    ADD_PROPERTY(Axis, (0.0, 0.0, 1.0));
}

App::DocumentObjectExecReturn* TransformDemolding::execute()
{ /*
  Feature *pcFirst  = dynamic_cast<Feature*>(Source.getValue());
  if (!pcFirst || pcFirst->isError())
      return new App::DocumentObjectExecReturn("Unknown Error");

  MeshCore::MeshKernel* pcKernel = new MeshCore::MeshKernel(pcFirst->Mesh.getValue()); // Result
  Meshkernel Base::Matrix4D trans(Base::Vector3f(0, 0, 0), Axis.getValue(), Rotation.getValue());
  //Matrix4D trans;
  //trans.rotLine(Axis.getValue(), Rotation.getValue());
  pcKernel->Transform(trans);
  Mesh.setValue(pcKernel);
 */
    return App::DocumentObject::StdReturn;
}
