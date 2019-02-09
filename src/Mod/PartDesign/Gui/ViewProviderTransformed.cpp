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
# include <Standard_Version.hxx>
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
# include <QAction>
# include <QMenu>
# include <QMessageBox>
#endif

#include "ViewProviderTransformed.h"
#include "TaskTransformedParameters.h"
#include <Base/Console.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/PartDesign/App/FeatureTransformed.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderTransformed,PartDesignGui::ViewProvider)

void ViewProviderTransformed::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit %1").arg(QString::fromStdString(featureName)), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
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

    recomputeFeature();

    return ViewProvider::setEdit(ModNum);
}

void ViewProviderTransformed::unsetEdit(int ModNum)
{
    ViewProvider::unsetEdit(ModNum);

    while (pcRejectedRoot->getNumChildren() > 7) {
        SoSeparator* sep = static_cast<SoSeparator*>(pcRejectedRoot->getChild(7));
        SoMultipleCopy* rejectedTrfms = static_cast<SoMultipleCopy*>(sep->getChild(2));
        rejectedTrfms   ->removeAllChildren();
        sep->removeChild(1);
        sep->removeChild(0);
        pcRejectedRoot  ->removeChild(7);
    }
    pcRejectedRoot->removeAllChildren();

    pcRoot->removeChild(pcRejectedRoot);

    pcRejectedRoot->unref();
}

bool ViewProviderTransformed::onDelete(const std::vector<std::string> &s)
{
    return ViewProvider::onDelete(s);
}

void ViewProviderTransformed::recomputeFeature(void)
{
    PartDesign::Transformed* pcTransformed = static_cast<PartDesign::Transformed*>(getObject());
    pcTransformed->recomputeFeature(true);
    const PartDesign::Transformed::rejectedMap &rejected_trsf = pcTransformed->getRejectedTransformations();
    unsigned rejected = rejected_trsf.size();
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
    signalDiagnosis(msg);

    // Clear all the rejected stuff
    while (pcRejectedRoot->getNumChildren() > 7) {
        SoSeparator* sep = static_cast<SoSeparator*>(pcRejectedRoot->getChild(7));
        SoMultipleCopy* rejectedTrfms = static_cast<SoMultipleCopy*>(sep->getChild(2));
        rejectedTrfms   ->removeAllChildren();
        sep->removeChild(1);
        sep->removeChild(0);
        pcRejectedRoot  ->removeChild(7);
    }

    for (PartDesign::Transformed::rejectedMap::const_iterator o = rejected_trsf.begin(); o != rejected_trsf.end(); o++) {
        // Display the rejected transformations in red
        TopoDS_Shape cShape(o->first.getShape());

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
            // Note: This DOES have an effect on cShape
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
            SoMultipleCopy* rejectedTrfms = new SoMultipleCopy();
            rejectedTrfms->matrix.setNum((o->second).size());
            SbMatrix* mats = rejectedTrfms->matrix.startEditing();

            auto trsf = (o->second).begin();
            for (unsigned int i=0; i < (o->second).size(); i++,trsf++) {
                Base::Matrix4D mat;
                Part::TopoShape::convertToMatrix(*trsf,mat);
                mats[i] = convert(mat);
            }
            rejectedTrfms->matrix.finishEditing();
            rejectedTrfms->addChild(rejectedFaceSet);
            SoSeparator* sep = new SoSeparator();
            sep->addChild(rejectedCoords);
            sep->addChild(rejectedNorms);
            sep->addChild(rejectedTrfms);
            pcRejectedRoot->addChild(sep);
        }
        catch (...) {
            Base::Console().Error("Cannot compute Inventor representation for the rejected transformations of shape of %s.\n",
                                  pcTransformed->getNameInDocument());
        }
    }

}

