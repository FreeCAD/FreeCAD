/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Bnd_Box.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <Poly_Triangulation.hxx>
# include <Standard_Version.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMultipleCopy.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoTransparencyType.h>
# include <QAction>
# include <QMenu>
#endif

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>

#include "ViewProviderTransformed.h"
#include "TaskTransformedParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderTransformed,PartDesignGui::ViewProvider)

const std::string & ViewProviderTransformed::featureName() const
{
    static const std::string name = "undefined";
    return name;
}

std::string ViewProviderTransformed::featureIcon() const
{
    return std::string("PartDesign_") + featureName();
}

void ViewProviderTransformed::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QString text = QString::fromStdString(getObject()->Label.getStrValue());
    addDefaultAction(menu, QObject::tr("Edit %1").arg(text));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

Gui::ViewProvider *ViewProviderTransformed::startEditing(int ModNum) {
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    if(!pcTransformed->Originals.getSize()) {
        for(auto obj : pcTransformed->getInList()) {
            if(obj->isDerivedFrom(PartDesign::MultiTransform::getClassTypeId())) {
                auto vp = Gui::Application::Instance->getViewProvider(obj);
                if(vp)
                    return vp->startEditing(ModNum);
                return nullptr;
            }
        }
    }
    return ViewProvider::startEditing(ModNum);
}

bool ViewProviderTransformed::setEdit(int ModNum)
{
    pcRejectedRoot = new SoSeparator();
    pcRejectedRoot->ref();

    SoPickStyle* rejectedPickStyle = new SoPickStyle();
    rejectedPickStyle->style = SoPickStyle::UNPICKABLE;

    SoShapeHints* rejectedHints = new SoShapeHints();
    rejectedHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
    rejectedHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    SoMaterialBinding* rejectedBind = new SoMaterialBinding();

    SoTransparencyType* rejectedTransparencyType = new SoTransparencyType();
    rejectedTransparencyType->value.setValue(SoGLRenderAction::BLEND);

    SoMaterial* rejectedMaterial = new SoMaterial();
    rejectedMaterial->diffuseColor.set1Value(0,SbColor(1.f,0.f,0.f));
    rejectedMaterial->transparency.setValue(0.6f);

    SoDrawStyle* rejectedFaceStyle = new SoDrawStyle();
    rejectedFaceStyle->style = SoDrawStyle::FILLED;

    SoNormalBinding* rejectedNormb = new SoNormalBinding();
    rejectedNormb->value = SoNormalBinding::PER_VERTEX_INDEXED;

    // just faces with no edges or points
    pcRejectedRoot->addChild(rejectedPickStyle);
    pcRejectedRoot->addChild(rejectedTransparencyType);
    pcRejectedRoot->addChild(rejectedBind);
    pcRejectedRoot->addChild(rejectedMaterial);
    pcRejectedRoot->addChild(rejectedHints);
    pcRejectedRoot->addChild(rejectedFaceStyle);
    pcRejectedRoot->addChild(rejectedNormb); // NOTE: The code relies on the last child added here being index 6
    pcRoot->addChild(pcRejectedRoot);

    recomputeFeature(false);

    return ViewProvider::setEdit(ModNum);
}

void ViewProviderTransformed::unsetEdit(int ModNum)
{
    ViewProvider::unsetEdit(ModNum);

    while (pcRejectedRoot->getNumChildren() > 7) {
        SoSeparator* sep = static_cast<SoSeparator*>(pcRejectedRoot->getChild(7));
        SoMultipleCopy* rejectedTrfms = static_cast<SoMultipleCopy*>(sep->getChild(2));
        Gui::coinRemoveAllChildren(rejectedTrfms);
        sep->removeChild(1);
        sep->removeChild(0);
        pcRejectedRoot  ->removeChild(7);
    }
    Gui::coinRemoveAllChildren(pcRejectedRoot);

    pcRoot->removeChild(pcRejectedRoot);

    pcRejectedRoot->unref();
}

bool ViewProviderTransformed::onDelete(const std::vector<std::string> &s)
{
    return ViewProvider::onDelete(s);
}

void ViewProviderTransformed::recomputeFeature(bool recompute)
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    if(recompute || (pcTransformed->isError() || pcTransformed->mustExecute()))
        pcTransformed->recomputeFeature(true);

    unsigned rejected = 0;
    TopoDS_Shape cShape = pcTransformed->rejected;
    TopExp_Explorer xp;
    xp.Init(cShape, TopAbs_SOLID);
    for (; xp.More(); xp.Next()) {
        rejected++;
    }

    QString msg = QString::fromLatin1("%1");
    if (rejected > 0) {
        msg = QString::fromLatin1("<font color='orange'>%1<br/></font>\r\n%2");
        if (rejected == 1)
            msg = msg.arg(QObject::tr("One transformed shape does not intersect support"));
        else {
            msg = msg.arg(QObject::tr("%1 transformed shapes do not intersect support"));
            msg = msg.arg(rejected);
        }
    }
    auto error = pcTransformed->getDocument()->getErrorDescription(pcTransformed);
    if (error) {
        msg = msg.arg(QString::fromLatin1("<font color='red'>%1<br/></font>"));
        msg = msg.arg(QString::fromUtf8(error));
    } else {
        msg = msg.arg(QString::fromLatin1("<font color='green'>%1<br/></font>"));
        msg = msg.arg(QObject::tr("Transformation succeeded"));
    }
    diagMessage = msg;
    signalDiagnosis(msg);

    // Clear all the rejected stuff
    while (pcRejectedRoot->getNumChildren() > 7) {
        SoSeparator* sep = static_cast<SoSeparator*>(pcRejectedRoot->getChild(7));
        SoMultipleCopy* rejectedTrfms = static_cast<SoMultipleCopy*>(sep->getChild(2));
        Gui::coinRemoveAllChildren(rejectedTrfms);
        sep->removeChild(1);
        sep->removeChild(0);
        pcRejectedRoot  ->removeChild(7);
    }

    // Display the rejected transformations in red
    if (rejected > 0) {
        showRejectedShape(cShape);
    }
}

void ViewProviderTransformed::showRejectedShape(TopoDS_Shape shape)
{
    try {
        // calculating the deflection value
        Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
        {
            Bnd_Box bounds;
            BRepBndLib::Add(shape, bounds);
            bounds.SetGap(0.0);
            bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
        }
        Standard_Real deflection = ((xMax-xMin)+(yMax-yMin)+(zMax-zMin))/300.0 * Deviation.getValue();

        // create or use the mesh on the data structure
        // Note: This DOES have an effect on shape
        Standard_Real AngDeflectionRads = AngularDeflection.getValue() / 180.0 * M_PI;
        BRepMesh_IncrementalMesh(shape, deflection, Standard_False, AngDeflectionRads, Standard_True);

        // We must reset the location here because the transformation data
        // are set in the placement property
        TopLoc_Location aLoc;
        shape.Location(aLoc);

        // count triangles and nodes in the mesh
        int nbrTriangles=0, nbrNodes=0;
        TopExp_Explorer Ex;
        for (Ex.Init(shape, TopAbs_FACE); Ex.More(); Ex.Next()) {
            Handle (Poly_Triangulation) mesh = BRep_Tool::Triangulation(TopoDS::Face(Ex.Current()), aLoc);
            // Note: we must also count empty faces
            if (!mesh.IsNull()) {
                nbrTriangles += mesh->NbTriangles();
                nbrNodes     += mesh->NbNodes();
            }
        }

        // create memory for the nodes and indexes
        SoCoordinate3* rejectedCoords = new SoCoordinate3();
        rejectedCoords  ->point      .setNum(nbrNodes);
        SoNormal* rejectedNorms = new SoNormal();
        rejectedNorms   ->vector     .setNum(nbrNodes);
        SoIndexedFaceSet* rejectedFaceSet = new SoIndexedFaceSet();
        rejectedFaceSet ->coordIndex .setNum(nbrTriangles*4);

        // get the raw memory for fast fill up
        SbVec3f* verts = rejectedCoords  ->point      .startEditing();
        SbVec3f* norms = rejectedNorms   ->vector     .startEditing();
        int32_t* index = rejectedFaceSet ->coordIndex .startEditing();

        // preset the normal vector with null vector
        for (int i=0; i < nbrNodes; i++)
            norms[i]= SbVec3f(0.0,0.0,0.0);

        int FaceNodeOffset=0,FaceTriaOffset=0;
        for (Ex.Init(shape, TopAbs_FACE); Ex.More(); Ex.Next()) {
            const TopoDS_Face &actFace = TopoDS::Face(Ex.Current());

            // get triangulation
            std::vector<gp_Pnt> points;
            std::vector<Poly_Triangle> facets;
            if (!Part::Tools::getTriangulation(actFace, points, facets))
                continue;

            // get normal per vertex
            std::vector<gp_Vec> vertexnormals;
            Part::Tools::getPointNormals(points, facets, vertexnormals);

            // getting size of node and triangle array of this face
            std::size_t nbNodesInFace = points.size();
            std::size_t nbTriInFace   = facets.size();

            for (std::size_t i = 0; i < points.size(); i++) {
                verts[FaceNodeOffset+i] = SbVec3f(points[i].X(), points[i].Y(), points[i].Z());
            }

            for (std::size_t i = 0; i < vertexnormals.size(); i++) {
                norms[FaceNodeOffset+i] = SbVec3f(vertexnormals[i].X(), vertexnormals[i].Y(), vertexnormals[i].Z());
            }

            // cycling through the poly mesh
            for (std::size_t g=0; g < nbTriInFace; g++) {
                // Get the triangle
                Standard_Integer N1,N2,N3;
                facets[g].Get(N1,N2,N3);

                // set the index vector with the 3 point indexes and the end delimiter
                index[FaceTriaOffset*4+4*g]   = FaceNodeOffset+N1;
                index[FaceTriaOffset*4+4*g+1] = FaceNodeOffset+N2;
                index[FaceTriaOffset*4+4*g+2] = FaceNodeOffset+N3;
                index[FaceTriaOffset*4+4*g+3] = SO_END_FACE_INDEX;
            }

            // counting up the per Face offsets
            FaceNodeOffset += nbNodesInFace;
            FaceTriaOffset += nbTriInFace;

            // normalize all normals
            for (int i=0; i < nbrNodes; i++)
                norms[i].normalize();

            // end the editing of the nodes
            rejectedCoords  ->point      .finishEditing();
            rejectedNorms   ->vector     .finishEditing();
            rejectedFaceSet ->coordIndex .finishEditing();

            // fill in the transformation matrices
            SoMultipleCopy* rejectedTrfms = new SoMultipleCopy();
            rejectedTrfms->matrix.finishEditing();
            rejectedTrfms->addChild(rejectedFaceSet);
            SoSeparator* sep = new SoSeparator();
            sep->addChild(rejectedCoords);
            sep->addChild(rejectedNorms);
            sep->addChild(rejectedTrfms);
            pcRejectedRoot->addChild(sep);
        }
    }
    catch (...) {
        Base::Console().Error("Cannot compute Inventor representation for the rejected transformations of shape of %s.\n",
                              getObject()->getNameInDocument());
    }
}
