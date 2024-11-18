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

#include "Core/Iterator.h"
#include "Core/SetOperations.h"

#include "FeatureMeshSetOperations.h"


using namespace Mesh;
using namespace std;

PROPERTY_SOURCE(Mesh::SetOperations, Mesh::Feature)


SetOperations::SetOperations()
{
    ADD_PROPERTY(Source1, (nullptr));
    ADD_PROPERTY(Source2, (nullptr));
    ADD_PROPERTY(OperationType, ("union"));
}

short SetOperations::mustExecute() const
{
    if (Source1.getValue() && Source2.getValue()) {
        if (Source1.isTouched()) {
            return 1;
        }
        if (Source2.isTouched()) {
            return 1;
        }
        if (OperationType.isTouched()) {
            return 1;
        }
    }

    return 0;
}

App::DocumentObjectExecReturn* SetOperations::execute()
{
    Mesh::Feature* mesh1 = dynamic_cast<Mesh::Feature*>(Source1.getValue());
    Mesh::Feature* mesh2 = dynamic_cast<Mesh::Feature*>(Source2.getValue());

    if (mesh1 && mesh2) {
        const MeshObject& meshKernel1 = mesh1->Mesh.getValue();
        const MeshObject& meshKernel2 = mesh2->Mesh.getValue();

        std::unique_ptr<MeshObject> pcKernel(new MeshObject());  // Result Meshkernel

        MeshCore::SetOperations::OperationType type {};
        string ot(OperationType.getValue());
        if (ot == "union") {
            type = MeshCore::SetOperations::Union;
        }
        else if (ot == "intersection") {
            type = MeshCore::SetOperations::Intersect;
        }
        else if (ot == "difference") {
            type = MeshCore::SetOperations::Difference;
        }
        else if (ot == "inner") {
            type = MeshCore::SetOperations::Inner;
        }
        else if (ot == "outer") {
            type = MeshCore::SetOperations::Outer;
        }
        else {
            throw Base::ValueError("Operation type must either be 'union' or 'intersection'"
                                   " or 'difference' or 'inner' or 'outer'");
        }

        MeshCore::SetOperations setOp(meshKernel1.getKernel(),
                                      meshKernel2.getKernel(),
                                      pcKernel->getKernel(),
                                      type,
                                      1.0e-5f);
        setOp.Do();
        Mesh.setValuePtr(pcKernel.release());
    }
    else {
        // Error mesh property
        if (!mesh1) {
            throw Base::ValueError("First input mesh not set");
        }
        if (!mesh2) {
            throw Base::ValueError("Second input mesh not set");
        }
    }

    return App::DocumentObject::StdReturn;
}
