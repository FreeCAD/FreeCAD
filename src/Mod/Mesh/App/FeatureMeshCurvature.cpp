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
#ifndef _PreComp_
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Sequencer.h>
#include <Mod/Mesh/App/WildMagic4/Wm4Vector3.h>
#include <Mod/Mesh/App/WildMagic4/Wm4MeshCurvature.h>
#include "FeatureMeshCurvature.h"
#include "MeshFeature.h"

#include "Core/Elements.h"
#include "Core/Iterator.h"



using namespace Mesh;
using namespace MeshCore;

PROPERTY_SOURCE(Mesh::Curvature, App::DocumentObject)


Curvature::Curvature(void)
{
    ADD_PROPERTY(Source,(0));
    ADD_PROPERTY(CurvInfo, (CurvatureInfo()));
}

short Curvature::mustExecute() const
{
    if (Source.isTouched())
        return 1;
    if (Source.getValue() && Source.getValue()->isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Curvature::execute(void)
{
    Mesh::Feature *pcFeat  = dynamic_cast<Mesh::Feature*>(Source.getValue());
    if(!pcFeat || pcFeat->isError()) {
        return new App::DocumentObjectExecReturn("No mesh object attached.");
    }
 
    // get all points
    const MeshKernel& rMesh = pcFeat->Mesh.getValue().getKernel();
    std::vector< Wm4::Vector3<float> > aPnts;
    aPnts.reserve(rMesh.CountPoints());
    MeshPointIterator cPIt( rMesh );
    for (cPIt.Init(); cPIt.More(); cPIt.Next()) {
        Wm4::Vector3<float> cP(cPIt->x, cPIt->y, cPIt->z);
        aPnts.push_back(cP);
    }

    // get all point connections
    std::vector<int> aIdx;
    aIdx.reserve(3*rMesh.CountFacets());
    const std::vector<MeshFacet>& raFts = rMesh.GetFacets();
    for (std::vector<MeshFacet>::const_iterator jt = raFts.begin(); jt != raFts.end(); ++jt) {
        for (int i=0; i<3; i++) {
            aIdx.push_back((int)jt->_aulPoints[i]);
        }
    }

    // compute vertex based curvatures
    Wm4::MeshCurvature<float> meshCurv(rMesh.CountPoints(), &(aPnts[0]), rMesh.CountFacets(), &(aIdx[0]));

    // get curvature information now
    const Wm4::Vector3<float>* aMaxCurvDir = meshCurv.GetMaxDirections();
    const Wm4::Vector3<float>* aMinCurvDir = meshCurv.GetMinDirections();
    const float* aMaxCurv = meshCurv.GetMaxCurvatures();
    const float* aMinCurv = meshCurv.GetMinCurvatures();

    std::vector<CurvatureInfo> values(rMesh.CountPoints());
    for (unsigned long i=0; i<rMesh.CountPoints(); i++) {
        CurvatureInfo ci;
        ci.cMaxCurvDir = Base::Vector3f(aMaxCurvDir[i].X(), aMaxCurvDir[i].Y(), aMaxCurvDir[i].Z());
        ci.cMinCurvDir = Base::Vector3f(aMinCurvDir[i].X(), aMinCurvDir[i].Y(), aMinCurvDir[i].Z());
        ci.fMaxCurvature = aMaxCurv[i];
        ci.fMinCurvature = aMinCurv[i];
        values[i] = ci;
    }

    CurvInfo.setValues(values);

    return App::DocumentObject::StdReturn;
}
