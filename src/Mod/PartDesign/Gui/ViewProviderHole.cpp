/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
#include <QMenu>
#include <QMessageBox>
#endif

// OpenCascade
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Poly_Triangle.hxx>

#include <BRep_Tool.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Surface.hxx>
#include <gp_Pnt2d.hxx>
#include <Precision.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include "App/Document.h"
#include <App/PropertyStandard.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Base/Console.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/FeatureHole.h>
#include <Mod/PartDesign/Gui/ViewProviderHole.h>
#include <Gui/SoFCDB.h>
#include <Gui/View3DInventor.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include "Base/Tools.h"
#include <App/GeoFeature.h>
#include <App/Property.h>
#include <App/PropertyFile.h>
#include <App/PropertyLinks.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/ViewProviderExt.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLMultiTextureEnabledElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoClipPlane.h>

#include "ViewProviderHole.h"
#include "TaskHoleParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderHole, PartGui::ViewProviderPartExt)

ViewProviderHole::ViewProviderHole()
    : textureCoords(new SoTextureCoordinate2()),
      boreTextureNode(new SoTexture2()),
      holeMaterialNode(new SoMaterial()),
      boreFacesMaterialNode(new SoMaterial()),
      boreFacesTextureCoords(new SoTextureCoordinate2()),
      boreIndexedFaceSet(new SoIndexedFaceSet()),
      boreFaceCoordinates(new SoCoordinate3()),
      boreFacesSeparator(new SoSeparator()),
      faceset(new SoIndexedFaceSet()),
      boreNormals(new SoNormal()),
      boreNormalBinding(new SoNormalBinding()),
      boreEndClipPlane(new SoClipPlane())
{
    sPixmap = "PartDesign_Hole.svg";
    boreFaceCoordinates->ref();
    boreIndexedFaceSet->ref();
    boreFacesMaterialNode->ref();
    boreFacesTextureCoords->ref();
    boreTextureNode->ref();
    holeMaterialNode->ref();
    faceset->ref();
    textureCoords->ref();
    boreFacesSeparator->ref();
    boreNormals->ref();
    boreNormalBinding->ref();
    boreEndClipPlane->ref();
    textureExtension = std::make_unique<Gui::ViewProviderTextureExtension>();

    SoGroup* root = getRoot();
    if (root && root->findChild(holeMaterialNode) == -1) {
        root->addChild(holeMaterialNode);
    }
}

ViewProviderHole::~ViewProviderHole() {
    if (faceset) {faceset->unref();}
    if (textureCoords) {textureCoords->unref();}
    if (boreFaceCoordinates) {boreFaceCoordinates->unref();}
    if (boreIndexedFaceSet) {boreIndexedFaceSet->unref();}
    if (boreFacesMaterialNode) {boreFacesMaterialNode->unref();}
    if (boreFacesTextureCoords) {boreFacesTextureCoords->unref();}
    if (holeMaterialNode) {holeMaterialNode->unref();}
    if (boreFacesSeparator) {boreFacesSeparator->unref();}
    if (boreTextureNode) {boreTextureNode->unref();}
    if (boreNormals) {boreNormals->unref();}
    if (boreNormalBinding) {boreNormalBinding->unref();}
    if (boreEndClipPlane) {boreEndClipPlane->unref();}
}

std::vector<App::DocumentObject*> ViewProviderHole::claimChildren() const
{
    std::vector<App::DocumentObject*> temp;
    if (auto* profile = getObject<PartDesign::Hole>()->Profile.getValue()) {
        temp.push_back(profile);
    }
    return temp;
}
void ViewProviderHole::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit hole"));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member); // clazy:exclude=skipped-base-method
}

bool ViewProviderHole::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // When double-clicking on the item for this hole the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgHoleParameters *holeDlg = qobject_cast<TaskDlgHoleParameters *>(dlg);
        if (holeDlg && holeDlg->getViewObject() != this)
            holeDlg = nullptr; // another hole left open its task panel
        if (dlg && !holeDlg) {
            QMessageBox msgBox(Gui::getMainWindow());
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

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog
        if (holeDlg)
            Gui::Control().showDialog(holeDlg);
        else
            Gui::Control().showDialog(new TaskDlgHoleParameters(this));

        return true;
    }
    else {
        return PartGui::ViewProviderPart::setEdit(ModNum); // clazy:exclude=skipped-base-method
    }
}

bool ViewProviderHole::onDelete(const std::vector<std::string> &s)
{
    // get the Sketch
    PartDesign::Hole* pcHole = getObject<PartDesign::Hole>();
    Sketcher::SketchObject *pcSketch = nullptr;
    if (pcHole->Profile.getValue())
        pcSketch = static_cast<Sketcher::SketchObject*>(pcHole->Profile.getValue());

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();

    return ViewProvider::onDelete(s);
}

void ViewProviderHole::attach(App::DocumentObject* obj)
{
    PartDesignGui::ViewProvider::attach(obj);

    SoGroup* rootNode = getRoot();
    if (!rootNode) {
        Base::Console().error("ViewProviderHole::attach: Root node is NULL.\n");
        return;
    }

    textureExtension->setup(holeMaterialNode);

    int facesetIndex = rootNode->findChild(faceset);
    if (facesetIndex != -1) {
        rootNode->insertChild(textureCoords, facesetIndex);
    } else {
        rootNode->addChild(textureCoords);
    }

    if (rootNode->findChild(boreFacesSeparator) == -1) {
        rootNode->addChild(boreFacesSeparator);
    }

    boreFacesSeparator->removeAllChildren();

    // Order matters for Coin3D:
    boreFacesSeparator->addChild(boreEndClipPlane); // Place the clipper first to affect subsequent bore geometry

    // Material and texture for the bore faces. Put these before geometry.
    boreFacesSeparator->addChild(boreFacesMaterialNode);
    boreFacesSeparator->addChild(boreTextureNode);
    boreFacesSeparator->addChild(boreFacesTextureCoords);

    // Geometry and normals for the bore faces
    boreFacesSeparator->addChild(boreNormalBinding);
    boreFacesSeparator->addChild(boreNormals);
    boreFacesSeparator->addChild(boreFaceCoordinates);
    boreFacesSeparator->addChild(boreIndexedFaceSet);
}

void ViewProviderHole::updateData(const App::Property* prop)
{
    PartDesignGui::ViewProvider::updateData(prop);

    auto* pcHole = getObject<PartDesign::Hole>();
    if (!pcHole) {
        clearBoreGeometry();
        return;
    }

    const bool isThreaded = pcHole->Threaded.getValue();
    const bool modelThread = pcHole->ModelThread.getValue();
    const bool cosmeticThread = pcHole->CosmeticThread.getValue();
    if (!isThreaded || modelThread || !cosmeticThread) {
        clearBoreGeometry();
        return;
    }

    const TopoDS_Shape& holeShape = pcHole->Shape.getValue();
    if (holeShape.IsNull()) {
        clearBoreGeometry();
        return;
    }

    gp_Dir holeFeatureAxis;
    gp_Pnt axisLocationPnt;
    auto boreFaces = collectBoreFaces(pcHole, holeShape, holeFeatureAxis, axisLocationPnt);

    if (boreFaces.empty()) {
        clearBoreGeometry();
        return;
    }

    auto globalMaterial = getGlobalMaterial(pcHole);
    textureExtension->setCoinAppearance(holeMaterialNode, globalMaterial);
    textureExtension->setCoinAppearance(boreFacesMaterialNode, globalMaterial);

    double calculatedMinProj = std::numeric_limits<double>::max();
    double calculatedMaxProj = std::numeric_limits<double>::lowest();

    bool generated = generateBoreMeshData(pcHole, boreFaces, holeFeatureAxis, axisLocationPnt, calculatedMinProj, calculatedMaxProj);

    if (!generated){
        clearBoreGeometry();
        return;
    }

    boreTextureNode->filename.setValue(":/images/ThreadOverlay.png");
    boreTextureNode->wrapS = SoTexture2::REPEAT;
    boreTextureNode->wrapT = SoTexture2::REPEAT;

    gp_Pnt planeOriginPnt = axisLocationPnt.Translated(
        gp_Vec(holeFeatureAxis)
        * (calculatedMinProj - pcHole->ThreadDepth.getValue())
    );

    // Convert gp_Pnt to SbVec3f for the SbPlane constructor
    SbVec3f sbPlaneOrigin(
        static_cast<float>(planeOriginPnt.X()),
        static_cast<float>(planeOriginPnt.Y()),
        static_cast<float>(planeOriginPnt.Z())
    );

    SbVec3f endPlaneNormal(static_cast<float>(-holeFeatureAxis.X()),
                               static_cast<float>(-holeFeatureAxis.Y()),
                               static_cast<float>(-holeFeatureAxis.Z()));

    float endPlane_D = -(endPlaneNormal.dot(sbPlaneOrigin));

    boreEndClipPlane->plane.setValue(SbPlane(endPlaneNormal, endPlane_D));
    boreEndClipPlane->on = TRUE;
}

void ViewProviderHole::clearBoreGeometry()
{
    boreTextureNode->filename.setValue("");
    boreFaceCoordinates->point.setNum(0);
    boreIndexedFaceSet->coordIndex.setNum(0);
    boreFacesTextureCoords->point.setNum(0);
    boreNormals->vector.setNum(0);
    boreEndClipPlane->on = FALSE;
}

std::vector<TopoDS_Face> ViewProviderHole::collectBoreFaces(const PartDesign::Hole* pcHole, const TopoDS_Shape& holeShape, gp_Dir& holeFeatureAxis, gp_Pnt& axisLocationPnt)
{
    std::vector<TopoDS_Face> boreFaces;
    bool axisDetermined = false;
    const double holeRadius = pcHole->Diameter.getValue() / 2.0;
    const bool isTapered = pcHole->Tapered.getValue();
    const double coneSemiAngleRad = isTapered ? Base::toRadians(pcHole->TaperedAngle.getValue()) : 0.0;

    for (TopExp_Explorer expl(holeShape, TopAbs_FACE); expl.More(); expl.Next()) {
        const auto& face = TopoDS::Face(expl.Current());
        auto surface = BRep_Tool::Surface(face);
        if (surface.IsNull()) {continue;}

        gp_Dir currentFaceAxis;
        gp_Pnt currentFaceLocation;
        bool isMainBoreCandidate = false;

        if (auto cyl = Handle(Geom_CylindricalSurface)::DownCast(surface)) {
            currentFaceAxis = cyl->Axis().Direction();
            currentFaceLocation = cyl->Axis().Location();
            if (!isTapered && std::abs(cyl->Radius() - holeRadius) < Precision::Confusion()) {
                isMainBoreCandidate = true;
            }
        } else if (auto con = Handle(Geom_ConicalSurface)::DownCast(surface)) {
            currentFaceAxis = con->Axis().Direction();
            currentFaceLocation = con->Axis().Location();
            if (isTapered && std::abs(con->SemiAngle() - coneSemiAngleRad) < Precision::Confusion()) {
                isMainBoreCandidate = true;
            }
        }

        if (isMainBoreCandidate) {
            if (!axisDetermined) {
                holeFeatureAxis = currentFaceAxis;
                axisLocationPnt = currentFaceLocation;
                axisDetermined = true;
            }
            // Add to boreFaces if its axis is aligned
            // Check for alignment in both directions
            if (std::abs(currentFaceAxis.Dot(holeFeatureAxis) - 1.0) < Precision::Confusion() ||
                std::abs(currentFaceAxis.Dot(holeFeatureAxis) + 1.0) < Precision::Confusion())
            {
                boreFaces.push_back(face);
            }
        }
    }
    if (!axisDetermined) {
        return {};
    }
    return boreFaces;
}

App::Material ViewProviderHole::getGlobalMaterial(const PartDesign::Hole* pcHole) const
{
    if (auto* materialProp = dynamic_cast<App::PropertyMaterial*>(getPropertyByName("Material"))) {
        return materialProp->getValue();
    }

    std::vector<App::DocumentObject*> linkedObjects;
    pcHole->_Body.getLinks(linkedObjects, true, nullptr, false);

    if (!linkedObjects.empty() && linkedObjects[0]) {
        if (auto* doc = Gui::Application::Instance->getDocument(linkedObjects[0]->getDocument())) {
            if (auto* bodyVp = doc->getViewProvider(linkedObjects[0])) {
                if (auto* materialProp = dynamic_cast<App::PropertyMaterial*>(bodyVp->getPropertyByName("Material"))) {
                    return materialProp->getValue();
                }
            }
        }
    }

    return App::Material::getDefaultAppearance();
}

bool ViewProviderHole::generateBoreMeshData(const PartDesign::Hole* pcHole, const std::vector<TopoDS_Face>& boreFaces,
                                            const gp_Dir& holeFeatureAxis, const gp_Pnt& axisLocationPnt,
                                            double& outMinProj, double& outMaxProj)
{
    double threadPitch = pcHole->getThreadPitch();
    if (threadPitch == 0.0) {
        return false;
    }

    std::vector<SbVec3f> vertices;
    std::vector<SbVec3f> normals;
    std::vector<int> indices;
    std::vector<SbVec2f> uvs;

    outMinProj = std::numeric_limits<double>::max();
    outMaxProj = std::numeric_limits<double>::lowest();

    // Find the min/max projections for the bore geometry
    for (TopExp_Explorer expl(pcHole->Shape.getValue(), TopAbs_FACE); expl.More(); expl.Next()) {
        const auto& face = TopoDS::Face(expl.Current());
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            for (const auto& p : meshPoints) {
                double projection = gp_Vec(axisLocationPnt, p).Dot(holeFeatureAxis);
                outMinProj = std::min(outMinProj, projection);
                outMaxProj = std::max(outMaxProj, projection);
            }
        }
    }
    gp_Vec refVecInPlane = (std::abs(holeFeatureAxis.Dot(gp_Dir(1,0,0))) < (1 -Precision::Confusion()))
                           ? holeFeatureAxis.Crossed(gp_Dir(1,0,0))
                           : holeFeatureAxis.Crossed(gp_Dir(0,1,0));
    refVecInPlane.Normalize();
    gp_Vec crossRefVec = holeFeatureAxis.Crossed(refVecInPlane);

    for (const auto& face : boreFaces) {
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (!Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            continue;
        }

        int vertexOffset = static_cast<int>(vertices.size());

        for (const auto& pnt : meshPoints) {
            vertices.emplace_back(pnt.X(), pnt.Y(), pnt.Z());

            gp_Vec toPoint(axisLocationPnt, pnt);
            gp_Vec radial = toPoint - (toPoint.Dot(holeFeatureAxis) * holeFeatureAxis);

            // Calculate V-coordinate (axial direction)
            if (radial.SquareMagnitude() >  std::pow(Precision::Confusion(), 2)) {
                gp_Dir normal(radial);
                normals.emplace_back(normal.X(), normal.Y(), normal.Z());
            } else {
                normals.emplace_back(holeFeatureAxis.X(), holeFeatureAxis.Y(), holeFeatureAxis.Z());
            }

            // V-coordinate (axial direction)
            double axialPos = toPoint.Dot(holeFeatureAxis);
            float v_coord = static_cast<float>((axialPos - outMinProj) / threadPitch);

            // U-coordinate (around the circumference)
            double angleRad = std::atan2(radial.Dot(crossRefVec), radial.Dot(refVecInPlane));
            float u_coord = static_cast<float>(angleRad / (2 * M_PI));
            u_coord -= std::floor(u_coord);

            uvs.emplace_back(u_coord, v_coord);
        }

        for (const auto& facet : meshFacets) {
            Standard_Integer n1 = 1;
            Standard_Integer n2 = 1;
            Standard_Integer n3 = 1;
            facet.Get(n1, n2, n3);
            indices.push_back(n1 - 1 + vertexOffset);
            indices.push_back(n2 - 1 + vertexOffset);
            indices.push_back(n3 - 1 + vertexOffset);
            indices.push_back(-1); // End of face marker for SoIndexedFaceSet
        }
    }

    boreFaceCoordinates->point.setValues(0, static_cast<int>(vertices.size()), vertices.data());
    boreIndexedFaceSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
    boreFacesTextureCoords->point.setValues(0, static_cast<int>(uvs.size()), uvs.data()); // This one is key!
    boreNormals->vector.setValues(0, static_cast<int>(normals.size()), normals.data());
    boreNormalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
    return true;
}