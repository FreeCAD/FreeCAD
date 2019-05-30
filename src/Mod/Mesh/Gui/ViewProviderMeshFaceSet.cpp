/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedLineSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoPolygonOffset.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <QAction>
# include <QMenu>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Sequencer.h>
#include <Base/Tools2D.h>
#include <Base/ViewProj.h>

#include <App/Document.h>
#include <App/PropertyLinks.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/SoFCSelection.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>
#include <Gui/Window.h>
#include <Gui/Flag.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/Evaluation.h>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Triangulation.h>
#include <Mod/Mesh/App/Core/Visitor.h>
#include <Mod/Mesh/App/Mesh.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Mesh/App/MeshProperties.h>

#include "ViewProviderMeshFaceSet.h"
#include "SoFCMeshObject.h"
#include "SoFCIndexedFaceSet.h"


using namespace MeshGui;


PROPERTY_SOURCE(MeshGui::ViewProviderMeshFaceSet, MeshGui::ViewProviderMesh)

ViewProviderMeshFaceSet::ViewProviderMeshFaceSet()
{
    directRendering = false;
    triangleCount = 2500000;

    pcMeshNode = new SoFCMeshObjectNode;
    pcMeshNode->ref();
    pcMeshShape = new SoFCMeshObjectShape;
    pcMeshShape->ref();
    pcMeshCoord = new SoCoordinate3;
    pcMeshCoord->ref();
    pcMeshFaces = new SoFCIndexedFaceSet;
    pcMeshFaces->ref();

    // setup engine to notify 'pcMeshFaces' node about material changes.
    // When the affected nodes are deleted the engine will be deleted, too.
    SoFCMaterialEngine* engine = new SoFCMaterialEngine();
    engine->diffuseColor.connectFrom(&pcShapeMaterial->diffuseColor);
    pcMeshFaces->updateGLArray.connectFrom(&engine->trigger);
}

ViewProviderMeshFaceSet::~ViewProviderMeshFaceSet()
{
    pcMeshNode->unref();
    pcMeshShape->unref();
    pcMeshCoord->unref();
    pcMeshFaces->unref();
}

void ViewProviderMeshFaceSet::attach(App::DocumentObject *pcFeat)
{
    ViewProviderMesh::attach(pcFeat);

    pcShapeGroup->addChild(pcMeshCoord);
    pcShapeGroup->addChild(pcMeshFaces);

    // read the threshold from the preferences
    Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter()->GetGroup("Mod/Mesh");
    int size = hGrp->GetInt("RenderTriangleLimit", -1);
    if (size > 0) {
        pcMeshShape->renderTriangleLimit = (unsigned int)(pow(10.0f,size));
        static_cast<SoFCIndexedFaceSet*>(pcMeshFaces)->renderTriangleLimit = (unsigned int)(pow(10.0f,size));
    }
}

void ViewProviderMeshFaceSet::updateData(const App::Property* prop)
{
    ViewProviderMesh::updateData(prop);
    if (prop->getTypeId() == Mesh::PropertyMeshKernel::getClassTypeId()) {
        const Mesh::MeshObject* mesh = static_cast<const Mesh::PropertyMeshKernel*>(prop)->getValuePtr();

        bool direct = MeshRenderer::shouldRenderDirectly(mesh->countFacets() > this->triangleCount);
        if (direct) {
            this->pcMeshNode->mesh.setValue(mesh);
            // Needs to update internal bounding box caches
            this->pcMeshShape->touch();
            pcMeshCoord->point.setNum(0);
            pcMeshFaces->coordIndex.setNum(0);
        }
        else {
            ViewProviderMeshBuilder builder;
            builder.createMesh(prop, pcMeshCoord, pcMeshFaces);
            pcMeshFaces->invalidate();
        }

        if (direct != directRendering) {
            directRendering = direct;
            Gui::coinRemoveAllChildren(pcShapeGroup);

            if (directRendering) {
                pcShapeGroup->addChild(pcMeshNode);
                pcShapeGroup->addChild(pcMeshShape);
            }
            else {
                pcShapeGroup->addChild(pcMeshCoord);
                pcShapeGroup->addChild(pcMeshFaces);
            }
        }

        showOpenEdges(OpenEdges.getValue());
        std::vector<unsigned long> selection;
        mesh->getFacetsFromSelection(selection);
        if (selection.empty())
            unhighlightSelection();
        else
            highlightSelection();
    }
}

void ViewProviderMeshFaceSet::showOpenEdges(bool show)
{
    if (pcOpenEdge) {
        // remove the node and destroy the data
        pcRoot->removeChild(pcOpenEdge);
        pcOpenEdge = 0;
    }

    if (show) {
        pcOpenEdge = new SoSeparator();
        pcOpenEdge->addChild(pcLineStyle);
        pcOpenEdge->addChild(pOpenColor);

        if (directRendering) {
            pcOpenEdge->addChild(pcMeshNode);
            pcOpenEdge->addChild(new SoFCMeshObjectBoundary);
        }
        else {
            pcOpenEdge->addChild(pcMeshCoord);
            SoIndexedLineSet* lines = new SoIndexedLineSet;
            pcOpenEdge->addChild(lines);

            // Build up the lines with indices to the list of vertices 'pcMeshCoord'
            int index=0;
            const MeshCore::MeshKernel& rMesh = static_cast<Mesh::Feature*>(pcObject)->Mesh.getValue().getKernel();
            const MeshCore::MeshFacetArray& rFaces = rMesh.GetFacets();
            for (MeshCore::MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it) {
                for (int i=0; i<3; i++) {
                    if (it->_aulNeighbours[i] == ULONG_MAX) {
                        lines->coordIndex.set1Value(index++,it->_aulPoints[i]);
                        lines->coordIndex.set1Value(index++,it->_aulPoints[(i+1)%3]);
                        lines->coordIndex.set1Value(index++,SO_END_LINE_INDEX);
                    }
                }
            }
        }

        // add to the highlight node
        pcRoot->addChild(pcOpenEdge);
    }
}

SoShape* ViewProviderMeshFaceSet::getShapeNode() const
{
    if (directRendering)
        return this->pcMeshShape;
    return this->pcMeshFaces;
}

SoNode* ViewProviderMeshFaceSet::getCoordNode() const
{
    if (directRendering)
        return this->pcMeshNode;
    return this->pcMeshCoord;
}
