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

#include "Core/Curvature.h"

#include "FeatureMeshCurvature.h"
#include "MeshFeature.h"


using namespace Mesh;

PROPERTY_SOURCE(Mesh::Curvature, App::DocumentObject)


Curvature::Curvature()
{
    ADD_PROPERTY(Source, (nullptr));
    ADD_PROPERTY(CurvInfo, (CurvatureInfo()));
}

short Curvature::mustExecute() const
{
    if (Source.isTouched()) {
        return 1;
    }
    if (Source.getValue() && Source.getValue()->isTouched()) {
        return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn* Curvature::execute()
{
    Mesh::Feature* pcFeat = dynamic_cast<Mesh::Feature*>(Source.getValue());
    if (!pcFeat || pcFeat->isError()) {
        return new App::DocumentObjectExecReturn("No mesh object attached.");
    }

    // get all points
    const MeshCore::MeshKernel& rMesh = pcFeat->Mesh.getValue().getKernel();
    MeshCore::MeshCurvature meshCurv(rMesh);
    meshCurv.ComputePerVertex();
    const std::vector<MeshCore::CurvatureInfo>& curv = meshCurv.GetCurvature();

    std::vector<CurvatureInfo> values;
    values.reserve(curv.size());
    for (const auto& it : curv) {
        CurvatureInfo ci;
        ci.cMaxCurvDir = it.cMaxCurvDir;
        ci.cMinCurvDir = it.cMinCurvDir;
        ci.fMaxCurvature = it.fMaxCurvature;
        ci.fMinCurvature = it.fMinCurvature;
        values.push_back(ci);
    }

    CurvInfo.setValues(values);

    return App::DocumentObject::StdReturn;
}
