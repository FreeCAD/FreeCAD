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
#include <Base/Sequencer.h>

#include "Core/Algorithm.h"
#include "Core/Evaluation.h"
#include "Core/Iterator.h"
#include "Core/Visitor.h"

#include "FeatureMeshSegmentByMesh.h"


using namespace Mesh;
using namespace MeshCore;

PROPERTY_SOURCE(Mesh::SegmentByMesh, Mesh::Feature)


SegmentByMesh::SegmentByMesh(void)
{
    ADD_PROPERTY(Source  ,(0));
    ADD_PROPERTY(Tool    ,(0));
    ADD_PROPERTY(Base    ,(0.0,0.0,0.0));
    ADD_PROPERTY(Normal  ,(0.0,0.0,1.0));
}

short SegmentByMesh::mustExecute() const
{
    if (Source.isTouched() || Tool.isTouched())
        return 1;
    if (Source.getValue() && Source.getValue()->isTouched())
        return 1;
    if (Tool.getValue() && Tool.getValue()->isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *SegmentByMesh::execute(void)
{
    Mesh::PropertyMeshKernel *kernel=0;
    App::DocumentObject* mesh = Source.getValue();
    if (mesh) {
        App::Property* prop = mesh->getPropertyByName("Mesh");
        if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId())
            kernel = static_cast<Mesh::PropertyMeshKernel*>(prop);
    }
    if (!kernel)
        return new App::DocumentObjectExecReturn("No mesh specified.\n");
    else if (mesh->isError())
        return new App::DocumentObjectExecReturn("No valid mesh.\n");

    Mesh::PropertyMeshKernel *toolmesh=0;
    App::DocumentObject* tool = Tool.getValue();
    if (tool) {
        App::Property* prop = tool->getPropertyByName("Mesh");
        if (prop && prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
            toolmesh = static_cast<Mesh::PropertyMeshKernel*>(prop);
        }
    }
    if (!toolmesh) {
        return new App::DocumentObjectExecReturn("No toolmesh specified.\n");
    }
    else if (tool->isError()) {
        return new App::DocumentObjectExecReturn("No valid toolmesh.\n");
    }

    // the clipping plane
    Base::Vector3f cBase, cNormal;
    cBase =   Base::convertTo<Base::Vector3f>(Base.getValue());
    cNormal = Base::convertTo<Base::Vector3f>(Normal.getValue());


    const MeshKernel& rMeshKernel = kernel->getValue().getKernel();
    const MeshKernel& rToolMesh   = toolmesh->getValue().getKernel();

    // check if the toolmesh is a solid
    if (!MeshEvalSolid(rToolMesh).Evaluate()) {
        return new App::DocumentObjectExecReturn("Toolmesh is not solid.\n");
    }

    std::vector<unsigned long> faces;
    std::vector<MeshGeomFacet> aFaces;

    MeshAlgorithm cAlg(rMeshKernel);
    if (cNormal.Length() > 0.1f) // not a null vector
        cAlg.GetFacetsFromToolMesh(rToolMesh, cNormal, faces);
    else
        cAlg.GetFacetsFromToolMesh(rToolMesh, Base::Vector3f(0.0, 1.0f, 0.0f), faces);

    // if the clipping plane was set then we want only the visible facets
    if ( cNormal.Length() > 0.1f ) { // not a null vector 
        // now we have too many facets since we have (invisible) facets near to the back clipping plane, 
        // so we need the nearest facet to the front clipping plane
        //
        float fDist = FLOAT_MAX;
        unsigned long uIdx=ULONG_MAX;
        MeshFacetIterator cFIt(rMeshKernel);

        // get the nearest facet to the user (front clipping plane)
        for ( std::vector<unsigned long>::iterator it = faces.begin(); it != faces.end(); ++it ) {
            cFIt.Set(*it);
            float dist = (float)fabs(cFIt->GetGravityPoint().DistanceToPlane( cBase, cNormal ));
            if ( dist < fDist ) {
                fDist = dist;
                uIdx = *it;
            }
        }

        // succeeded
        if ( uIdx != ULONG_MAX ) {
            // set VISIT-Flag to all outer facets
            cAlg.SetFacetFlag( MeshFacet::VISIT );
            cAlg.ResetFacetsFlag(faces, MeshFacet::VISIT);

            faces.clear();
            MeshTopFacetVisitor clVisitor(faces);
            rMeshKernel.VisitNeighbourFacets(clVisitor, uIdx);

            // append also the start facet
            faces.push_back(uIdx);
        }
    }

    for ( std::vector<unsigned long>::iterator it = faces.begin(); it != faces.end(); ++it )
        aFaces.push_back( rMeshKernel.GetFacet(*it) );

    std::auto_ptr<MeshObject> pcKernel(new MeshObject);
    pcKernel->addFacets(aFaces);
    Mesh.setValuePtr(pcKernel.release());

    return App::DocumentObject::StdReturn;
}

