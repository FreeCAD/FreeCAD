/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
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
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Poly_Triangulation.hxx>
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
#endif

#include "ViewProviderTransformed.h"
#include "TaskTransformedParameters.h"
#include <Base/Console.h>
#include <Gui/Control.h>
#include <Gui/Application.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/App/FeatureAdditive.h>
#include <Mod/PartDesign/App/FeatureSubtractive.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderTransformed,PartDesignGui::ViewProvider)

void ViewProviderTransformed::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr((std::string("Edit ") + featureName + " feature").c_str()), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderTransformed::setEdit(int ModNum)
{
    pcRejectedRoot = new SoSeparator();
    pcRejectedRoot->ref();

    rejectedTrfms = new SoMultipleCopy();
    rejectedTrfms->ref();

    rejectedCoords = new SoCoordinate3();
    rejectedCoords->ref();

    rejectedNorms = new SoNormal();
    rejectedNorms->ref();

    rejectedFaceSet = new SoIndexedFaceSet();
    rejectedFaceSet->ref();

    SoPickStyle* rejectedPickStyle = new SoPickStyle();
    rejectedPickStyle->style = SoPickStyle::UNPICKABLE;

    SoShapeHints* rejectedHints = new SoShapeHints();
    rejectedHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
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
    pcRejectedRoot->addChild(rejectedCoords);
    pcRejectedRoot->addChild(rejectedNorms);
    pcRejectedRoot->addChild(rejectedNormb);
    pcRejectedRoot->addChild(rejectedTrfms);
    rejectedTrfms->addChild(rejectedFaceSet);

    pcRoot->addChild(pcRejectedRoot);

    recomputeFeature();
    return true;
}

void ViewProviderTransformed::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
    }

    rejectedTrfms->removeAllChildren();
    pcRejectedRoot->removeAllChildren();

    pcRoot->removeChild(pcRejectedRoot);

    pcRejectedRoot->unref();
    rejectedTrfms->unref();
    rejectedCoords->unref();
    rejectedNorms->unref();
    rejectedFaceSet->unref();
}

bool ViewProviderTransformed::onDelete(const std::vector<std::string> &)
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    std::vector<App::DocumentObject*> originals = pcTransformed->Originals.getValues();

    // if abort command deleted the object the originals are visible again
    for (std::vector<App::DocumentObject*>::const_iterator it = originals.begin(); it != originals.end(); ++it)
    {
        if (((*it) != NULL) && Gui::Application::Instance->getViewProvider(*it))
            Gui::Application::Instance->getViewProvider(*it)->show();
    }

    return true;
}

const bool ViewProviderTransformed::checkDlgOpen(TaskDlgTransformedParameters* transformedDlg) {
    // When double-clicking on the item for this feature the
    // object unsets and sets its edit mode without closing
    // the task panel
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    transformedDlg = qobject_cast<TaskDlgTransformedParameters *>(dlg);

    if ((transformedDlg != NULL) && (transformedDlg->getTransformedView() != this))
        transformedDlg = NULL; // another transformed feature left open its task panel

    if ((dlg != NULL) && (transformedDlg == NULL)) {
        QMessageBox msgBox;
        msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
            Gui::Control().closeDialog();
        else
            return false;
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // Continue (usually in virtual method setEdit())
    return true;
}

void ViewProviderTransformed::recomputeFeature(void)
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    pcTransformed->getDocument()->recomputeFeature(pcTransformed);
    const std::vector<App::DocumentObjectExecReturn*> log = pcTransformed->getDocument()->getRecomputeLog();
    unsigned rejected = pcTransformed->getRejectedTransformations().size();
    QString msg = QString::fromAscii("%1");
    if (rejected > 0) {
        msg = QString::fromLatin1("<font color='orange'>%1<br/></font>\r\n%2");
        if (rejected == 1)
            msg = msg.arg(QObject::tr("One transformed shape does not intersect support"));
        else {
            msg = msg.arg(QObject::tr("%1 transformed shapes do not intersect support"));
            msg = msg.arg(rejected);
        }
    }
    if (log.size() > 0) {
        msg = msg.arg(QString::fromLatin1("<font color='red'>%1<br/></font>"));
        msg = msg.arg(QString::fromStdString(log.back()->Why));
    } else {
        msg = msg.arg(QString::fromLatin1("<font color='green'>%1<br/></font>"));
        msg = msg.arg(QObject::tr("Transformation succeeded"));
    }
    signalDiagnosis(msg);

    TopoDS_Shape shape;
    if (rejected != 0) {
        // FIXME: create a compound if there are more than one originals
        App::DocumentObject* original = pcTransformed->Originals.getValues().front();
        if (original->getTypeId().isDerivedFrom(PartDesign::Additive::getClassTypeId())) {
            PartDesign::Additive* addFeature = static_cast<PartDesign::Additive*>(original);
            shape = addFeature->AddShape.getShape()._Shape;
        } else if (original->getTypeId().isDerivedFrom(PartDesign::Subtractive::getClassTypeId())) {
            PartDesign::Subtractive* subFeature = static_cast<PartDesign::Subtractive*>(original);
            shape = subFeature->SubShape.getShape()._Shape;
        }
    }

    if (rejected == 0 || shape.IsNull()) {
        rejectedCoords  ->point      .setNum(0);
        rejectedNorms   ->vector     .setNum(0);
        rejectedFaceSet ->coordIndex .setNum(0);
        rejectedTrfms   ->matrix     .setNum(0);
    } else {
        // Display the rejected transformations in red
        TopoDS_Shape cShape(shape);

        try {
            // calculating the deflection value
            Standard_Real xMin, yMin, zMin, xMax, yMax, zMax;
            {
                Bnd_Box bounds;
                BRepBndLib::Add(cShape, bounds);
                bounds.SetGap(0.0);
                bounds.Get(xMin, yMin, zMin, xMax, yMax, zMax);
            }
            Standard_Real deflection = ((xMax-xMin)+(yMax-yMin)+(zMax-zMin))/300.0 * Deviation.getValue();

            // create or use the mesh on the data structure
            BRepMesh_IncrementalMesh myMesh(cShape,deflection);
            // We must reset the location here because the transformation data
            // are set in the placement property
            TopLoc_Location aLoc;
            cShape.Location(aLoc);

            // count triangles and nodes in the mesh
            int nbrTriangles=0, nbrNodes=0;
            TopExp_Explorer Ex;
            for (Ex.Init(cShape,TopAbs_FACE);Ex.More();Ex.Next()) {
                Handle (Poly_Triangulation) mesh = BRep_Tool::Triangulation(TopoDS::Face(Ex.Current()), aLoc);
                // Note: we must also count empty faces
                if (!mesh.IsNull()) {
                    nbrTriangles += mesh->NbTriangles();
                    nbrNodes     += mesh->NbNodes();
                }
            }

            // create memory for the nodes and indexes
            rejectedCoords  ->point      .setNum(nbrNodes);
            rejectedNorms   ->vector     .setNum(nbrNodes);
            rejectedFaceSet ->coordIndex .setNum(nbrTriangles*4);

            // get the raw memory for fast fill up
            SbVec3f* verts = rejectedCoords  ->point      .startEditing();
            SbVec3f* norms = rejectedNorms   ->vector     .startEditing();
            int32_t* index = rejectedFaceSet ->coordIndex .startEditing();

            // preset the normal vector with null vector
            for (int i=0; i < nbrNodes; i++)
                norms[i]= SbVec3f(0.0,0.0,0.0);

            int ii = 0,FaceNodeOffset=0,FaceTriaOffset=0;
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
                for (int g=1; g <= nbTriInFace; g++) {
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

                    // transform the vertices to the place of the face
                    if (!identity) {
                        V1.Transform(myTransf);
                        V2.Transform(myTransf);
                        V3.Transform(myTransf);
                    }

                    // calculating per vertex normals
                    // Calculate triangle normal
                    gp_Vec v1(V1.X(),V1.Y(),V1.Z()),v2(V2.X(),V2.Y(),V2.Z()),v3(V3.X(),V3.Y(),V3.Z());
                    gp_Vec Normal = (v2-v1)^(v3-v1);

                    // add the triangle normal to the vertex normal for all points of this triangle
                    norms[FaceNodeOffset+N1-1] += SbVec3f(Normal.X(),Normal.Y(),Normal.Z());
                    norms[FaceNodeOffset+N2-1] += SbVec3f(Normal.X(),Normal.Y(),Normal.Z());
                    norms[FaceNodeOffset+N3-1] += SbVec3f(Normal.X(),Normal.Y(),Normal.Z());

                    // set the vertices
                    verts[FaceNodeOffset+N1-1].setValue((float)(V1.X()),(float)(V1.Y()),(float)(V1.Z()));
                    verts[FaceNodeOffset+N2-1].setValue((float)(V2.X()),(float)(V2.Y()),(float)(V2.Z()));
                    verts[FaceNodeOffset+N3-1].setValue((float)(V3.X()),(float)(V3.Y()),(float)(V3.Z()));

                    // set the index vector with the 3 point indexes and the end delimiter
                    index[FaceTriaOffset*4+4*(g-1)]   = FaceNodeOffset+N1-1;
                    index[FaceTriaOffset*4+4*(g-1)+1] = FaceNodeOffset+N2-1;
                    index[FaceTriaOffset*4+4*(g-1)+2] = FaceNodeOffset+N3-1;
                    index[FaceTriaOffset*4+4*(g-1)+3] = SO_END_FACE_INDEX;
                }

                // counting up the per Face offsets
                FaceNodeOffset += nbNodesInFace;
                FaceTriaOffset += nbTriInFace;
            }

            // normalize all normals
            for (int i=0; i < nbrNodes; i++)
                norms[i].normalize();

            // end the editing of the nodes
            rejectedCoords  ->point      .finishEditing();
            rejectedNorms   ->vector     .finishEditing();
            rejectedFaceSet ->coordIndex .finishEditing();

            // fill in the transformation matrices
            rejectedTrfms->matrix.setNum(rejected);
            SbMatrix* mats = rejectedTrfms->matrix.startEditing();

            std::list<gp_Trsf> rejected_trsf = pcTransformed->getRejectedTransformations();
            std::list<gp_Trsf>::const_iterator trsf = rejected_trsf.begin();
            for (unsigned int i=0; i < rejected; i++,trsf++) {
                Base::Matrix4D mat;
                Part::TopoShape::convertToMatrix(*trsf,mat);
                mats[i] = convert(mat);
            }
            rejectedTrfms->matrix.finishEditing();
        }
        catch (...) {
            Base::Console().Error("Cannot compute Inventor representation for the rejected transformations of shape of %s.\n",
                                  pcTransformed->getNameInDocument());
        }
    }

}

