/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Standard_Version.hxx>
#endif

#include "ViewProviderAddSub.h"
#include "Mod/Part/Gui/SoBrepFaceSet.h"
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Base/Console.h>



using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderAddSub,PartDesignGui::ViewProvider)

ViewProviderAddSub::ViewProviderAddSub()
{
    previewShape = new SoSeparator();
    previewShape->ref();
    previewFaceSet = new PartGui::SoBrepFaceSet();
    previewFaceSet->ref();
    previewCoords = new SoCoordinate3();
    previewCoords->ref();
    previewNorm = new SoNormal();
    previewNorm->ref();
    whichChild = -1;
}

ViewProviderAddSub::~ViewProviderAddSub()
{
    previewFaceSet->unref();
    previewCoords->unref();
    previewNorm->unref();
    previewShape->unref();
}

void ViewProviderAddSub::attach(App::DocumentObject* obj) {

    ViewProvider::attach(obj);

    auto* bind = new SoMaterialBinding();
    bind->value = SoMaterialBinding::OVERALL;
    auto* material = new SoMaterial();
    if (static_cast<PartDesign::FeatureAddSub*>(obj)->getAddSubType() == PartDesign::FeatureAddSub::Additive)
        material->diffuseColor = SbColor(1,1,0);
    else
        material->diffuseColor = SbColor(1,0,0);

    material->transparency = 0.7f;
    auto* pick = new SoPickStyle();
    pick->style = SoPickStyle::UNPICKABLE;

    previewShape->addChild(pick);
    previewShape->addChild(bind);
    previewShape->addChild(material);
    previewShape->addChild(previewCoords);
    previewShape->addChild(previewNorm);
    previewShape->addChild(previewFaceSet);

    addDisplayMaskMode(previewShape, "Shape preview");
    updateAddSubShapeIndicator();
}

void ViewProviderAddSub::updateAddSubShapeIndicator() {

    TopoDS_Shape cShape(static_cast<PartDesign::FeatureAddSub*>(getObject())->AddSubShape.getValue());
    if (cShape.IsNull()) {
        previewCoords  ->point      .setNum(0);
        previewNorm    ->vector     .setNum(0);
        previewFaceSet ->coordIndex .setNum(0);
        previewFaceSet ->partIndex  .setNum(0);
        return;
    }

    int numTriangles=0,numNodes=0,numNorms=0,numFaces=0;
    std::set<int> faceEdges;

    try {
        // calculating the deflection value
        Bnd_Box bounds;
        BRepBndLib::Add(cShape, bounds);
        bounds.SetGap(0.0);
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        Standard_Real deflection = ((xMax-xMin)+(yMax-yMin)+(zMax-zMin))/300.0 *
            Deviation.getValue();

        // create or use the mesh on the data structure
#if OCC_VERSION_HEX >= 0x060600
        Standard_Real AngDeflectionRads = AngularDeflection.getValue() / 180.0 * M_PI;
        BRepMesh_IncrementalMesh(cShape,deflection,Standard_False,
                                    AngDeflectionRads,Standard_True);
#else
        BRepMesh_IncrementalMesh(cShape,deflection);
#endif
        // We must reset the location here because the transformation data
        // are set in the placement property
        TopLoc_Location aLoc;
        cShape.Location(aLoc);

        // count triangles and nodes in the mesh
        TopExp_Explorer Ex;
        for (Ex.Init(cShape,TopAbs_FACE);Ex.More();Ex.Next()) {
            Handle (Poly_Triangulation) mesh = BRep_Tool::Triangulation(TopoDS::Face(Ex.Current()), aLoc);
            // Note: we must also count empty faces
            if (!mesh.IsNull()) {
                numTriangles += mesh->NbTriangles();
                numNodes     += mesh->NbNodes();
                numNorms     += mesh->NbNodes();
            }
            numFaces++;
        }

        // create memory for the nodes and indexes
        previewCoords  ->point      .setNum(numNodes);
        previewNorm    ->vector     .setNum(numNorms);
        previewFaceSet ->coordIndex .setNum(numTriangles*4);
        previewFaceSet ->partIndex  .setNum(numFaces);
        // get the raw memory for fast fill up
        SbVec3f* verts = previewCoords  ->point       .startEditing();
        SbVec3f* previewNorms = previewNorm    ->vector      .startEditing();
        int32_t* index = previewFaceSet ->coordIndex  .startEditing();
        int32_t* parts = previewFaceSet ->partIndex   .startEditing();

        // preset the previewNormal vector with null vector
        for (int i=0;i < numNorms;i++)
            previewNorms[i]= SbVec3f(0.0,0.0,0.0);

        int ii = 0,faceNodeOffset=0,faceTriaOffset=0;
        for (Ex.Init(cShape, TopAbs_FACE); Ex.More(); Ex.Next(),ii++) {
            TopLoc_Location aLoc;
            const TopoDS_Face &actFace = TopoDS::Face(Ex.Current());
            // get the mesh of the shape
            Handle (Poly_Triangulation) mesh = BRep_Tool::Triangulation(actFace,aLoc);
            if (mesh.IsNull()) continue;

            // getting the transformation of the shape/face
            gp_Trsf myTransf;
            Standard_Boolean identity = true;
            if (!aLoc.IsIdentity()) {
                identity = false;
                myTransf = aLoc.Transformation();
            }

            // getting size of node and triangle array of this face
            int nbNodesInFace = mesh->NbNodes();
            int nbTriInFace   = mesh->NbTriangles();
            // check orientation
            TopAbs_Orientation orient = actFace.Orientation();


            // cycling through the poly mesh
            const Poly_Array1OfTriangle& Triangles = mesh->Triangles();
            const TColgp_Array1OfPnt& Nodes = mesh->Nodes();
            TColgp_Array1OfDir Normals (Nodes.Lower(), Nodes.Upper());
            getNormals(actFace, mesh, Normals);

            for (int g=1;g<=nbTriInFace;g++) {
                // Get the triangle
                Standard_Integer N1,N2,N3;
                Triangles(g).Get(N1,N2,N3);

                // change orientation of the triangle if the face is reversed
                if ( orient != TopAbs_FORWARD ) {
                    Standard_Integer tmp = N1;
                    N1 = N2;
                    N2 = tmp;
                }

                // get the 3 points of this triangle
                gp_Pnt V1(Nodes(N1)), V2(Nodes(N2)), V3(Nodes(N3));

                // get the 3 previewNormals of this triangle
                gp_Dir NV1(Normals(N1)), NV2(Normals(N2)), NV3(Normals(N3));

                // transform the vertices and previewNormals to the place of the face
                if(!identity) {
                    V1.Transform(myTransf);
                    V2.Transform(myTransf);
                    V3.Transform(myTransf);
                    NV1.Transform(myTransf);
                    NV2.Transform(myTransf);
                    NV3.Transform(myTransf);
                }

                // add the previewNormals for all points of this triangle
                previewNorms[faceNodeOffset+N1-1] += SbVec3f(NV1.X(),NV1.Y(),NV1.Z());
                previewNorms[faceNodeOffset+N2-1] += SbVec3f(NV2.X(),NV2.Y(),NV2.Z());
                previewNorms[faceNodeOffset+N3-1] += SbVec3f(NV3.X(),NV3.Y(),NV3.Z());

                // set the vertices
                verts[faceNodeOffset+N1-1].setValue((float)(V1.X()),(float)(V1.Y()),(float)(V1.Z()));
                verts[faceNodeOffset+N2-1].setValue((float)(V2.X()),(float)(V2.Y()),(float)(V2.Z()));
                verts[faceNodeOffset+N3-1].setValue((float)(V3.X()),(float)(V3.Y()),(float)(V3.Z()));

                // set the index vector with the 3 point indexes and the end delimiter
                index[faceTriaOffset*4+4*(g-1)]   = faceNodeOffset+N1-1;
                index[faceTriaOffset*4+4*(g-1)+1] = faceNodeOffset+N2-1;
                index[faceTriaOffset*4+4*(g-1)+2] = faceNodeOffset+N3-1;
                index[faceTriaOffset*4+4*(g-1)+3] = SO_END_FACE_INDEX;
            }

            parts[ii] = nbTriInFace; // new part

            // counting up the per Face offsets
            faceNodeOffset += nbNodesInFace;
            faceTriaOffset += nbTriInFace;
        }

        // previewNormalize all previewNormals
        for (int i = 0; i< numNorms ;i++)
            previewNorms[i].normalize();

        // end the editing of the nodes
        previewCoords  ->point       .finishEditing();
        previewNorm    ->vector      .finishEditing();
        previewFaceSet ->coordIndex  .finishEditing();
        previewFaceSet ->partIndex   .finishEditing();
    }
    catch (...) {
        Base::Console().Error("Cannot compute Inventor representation for the shape of %s.\n",pcObject->getNameInDocument());
    }
}

void ViewProviderAddSub::updateData(const App::Property* p) {

    if(strcmp(p->getName(), "AddSubShape")==0)
        updateAddSubShapeIndicator();

    PartDesignGui::ViewProvider::updateData(p);
}

void ViewProviderAddSub::setPreviewDisplayMode(bool onoff) {
    // A mask mode is always set, also for hidden objects.
    // Now when changing to another mask mode this automatically
    // displays an object and when restoring the previous state it's
    // not sufficient to only revert the mask mode. Also the child
    // number of the switch node must be reverted.
    if (onoff && displayMode!="Shape preview") {
        displayMode = getActiveDisplayMode();
        whichChild = pcModeSwitch->whichChild.getValue();
        setDisplayMaskMode("Shape preview");
    }

    if (!onoff) {
        setDisplayMaskMode(displayMode.c_str());
        pcModeSwitch->whichChild.setValue(whichChild);
    }

    App::DocumentObject* obj = static_cast<PartDesign::Feature*>(getObject())->BaseFeature.getValue();
    if (obj)
        static_cast<PartDesignGui::ViewProvider*>(Gui::Application::Instance->getViewProvider(obj))->makeTemporaryVisible(onoff);
}
