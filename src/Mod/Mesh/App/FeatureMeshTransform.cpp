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

#include <Base/Matrix.h>

#include "FeatureMeshTransform.h"


using Base::Matrix4D;
using namespace Mesh;
using namespace MeshCore;

PROPERTY_SOURCE(Mesh::Transform, Mesh::Feature)

Transform::Transform()
{
    ADD_PROPERTY(Source, (nullptr));
    ADD_PROPERTY(Position, (Matrix4D()));
}

App::DocumentObjectExecReturn* Transform::execute()
{
    /*
        Feature* pcFirst = dynamic_cast<Feature*>(Source.getValue());
        if (!pcFirst || pcFirst->isError())
            return new App::DocumentObjectExecReturn("Unknown Error");

        Matrix4D Matrix = Position.getValue();


        MeshCore::MeshKernel* pcKernel =
            new MeshCore::MeshKernel(pcFirst->Mesh.getValue());// Result Meshkernel
        pcKernel->Transform(Matrix);
        Mesh.setValue(pcKernel);
    */
    return App::DocumentObject::StdReturn;
}
