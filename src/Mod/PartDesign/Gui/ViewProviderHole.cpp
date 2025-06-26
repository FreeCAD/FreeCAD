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
# include <QMenu>
# include <QMessageBox>
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
    : textureExtension(std::make_unique<Gui::ViewProviderTextureExtension>())
{
    sPixmap = "PartDesign_Hole.svg";
}

ViewProviderHole::~ViewProviderHole() {
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
    addDefaultAction(menu, QObject::tr("Edit Hole"));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member); // clazy:exclude=skipped-base-method
}

bool ViewProviderHole::onDelete(const std::vector<std::string>& s)
{
    // get the Sketch
    PartDesign::Hole* pcHole = getObject<PartDesign::Hole>();
    Sketcher::SketchObject* pcSketch = nullptr;
    if (pcHole->Profile.getValue()) {
        pcSketch = static_cast<Sketcher::SketchObject*>(pcHole->Profile.getValue());
    }

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch)) {
        Gui::Application::Instance->getViewProvider(pcSketch)->show();
    }

    return ViewProvider::onDelete(s);
}

TaskDlgFeatureParameters* ViewProviderHole::getEditDialog()
{
    return new TaskDlgHoleParameters(this);
}

void ViewProviderHole::attach(App::DocumentObject* obj)
{
    PartDesignGui::ViewProvider::attach(obj);
}


void ViewProviderHole::updateData(const App::Property* prop)
{
    PartDesignGui::ViewProvider::updateData(prop);
}

SoSeparator* ViewProviderHole::createThreadTextureSeparator(const TopoDS_Shape& bodyShape) const
{
    auto* pcHole = getObject<PartDesign::Hole>();
    if (!pcHole
        || !pcHole->Threaded.getValue()
        || pcHole->ModelThread.getValue()
        || !pcHole->CosmeticThread.getValue()
    ) {
        return nullptr;
    }
    const TopoDS_Shape& holeShape = pcHole->Shape.getValue();
    if (bodyShape.IsNull() || holeShape.IsNull()) {
        return nullptr;
    }

    gp_Dir holeFeatureAxis;
    gp_Pnt axisLocationPnt;
    // needs to be a copy because the triangulation can change the data
    TopoDS_Shape cpBodyShape = bodyShape;
    auto boreFaces = collectBoreFaces(pcHole, cpBodyShape, holeFeatureAxis, axisLocationPnt);
    if (boreFaces.empty()) {
        return nullptr;
    }

    double minProj = std::numeric_limits<double>::max();
    double maxProj = std::numeric_limits<double>::lowest();

    std::vector<SbVec3f> vertices;
    std::vector<SbVec3f> normals;
    std::vector<int> indices;
    std::vector<SbVec2f> uvs;

    // Use the boreFaces from the final body to generate the mesh
    if (!generateBoreMeshData(pcHole, boreFaces, holeFeatureAxis, axisLocationPnt, minProj, maxProj, vertices, normals, indices, uvs)) {
        return nullptr;
    }

    if (indices.empty()) {
        return nullptr;
    }

    auto* threadSeparator = new SoSeparator();
    threadSeparator->ref();

    // Create and add the dynamic clipping plane based on the ThreadDepth.
    auto* threadDepthClipper = new SoClipPlane();
    double originalHoleMinProj = std::numeric_limits<double>::max();
    for (TopExp_Explorer expl(holeShape, TopAbs_FACE); expl.More(); expl.Next()) {
        const auto& face = TopoDS::Face(expl.Current());
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            for (const auto& p : meshPoints) {
                double projection = gp_Vec(axisLocationPnt, p).Dot(holeFeatureAxis);
                originalHoleMinProj = std::min(originalHoleMinProj, projection);
            }
        }
    }
    gp_Pnt planeOriginPnt = axisLocationPnt.Translated(
        gp_Vec(holeFeatureAxis) * (originalHoleMinProj - pcHole->ThreadDepth.getValue()));
    SbVec3f sbPlaneOrigin(static_cast<float>(planeOriginPnt.X()), static_cast<float>(planeOriginPnt.Y()), static_cast<float>(planeOriginPnt.Z()));
    SbVec3f endPlaneNormal(static_cast<float>(-holeFeatureAxis.X()), static_cast<float>(-holeFeatureAxis.Y()), static_cast<float>(-holeFeatureAxis.Z()));
    float endPlane_D = -(endPlaneNormal.dot(sbPlaneOrigin));
    threadDepthClipper->plane.setValue(SbPlane(endPlaneNormal, endPlane_D));
    threadDepthClipper->on = TRUE;
    threadSeparator->addChild(threadDepthClipper);

    auto* newMaterialNode = new SoMaterial();
    textureExtension->setCoinAppearance(newMaterialNode, getGlobalMaterial(pcHole));
    threadSeparator->addChild(newMaterialNode);

    auto* newTextureNode = new SoTexture2();
    if (pcHole->ThreadDirection.getValue() != 0) {
        newTextureNode->filename.setValue(":/images/ThreadOverlayR.png");
    } else {
        newTextureNode->filename.setValue(":/images/ThreadOverlayL.png");
    }
    newTextureNode->wrapS = SoTexture2::REPEAT;
    newTextureNode->wrapT = SoTexture2::REPEAT;
    threadSeparator->addChild(newTextureNode);

    auto* newTextureCoords = new SoTextureCoordinate2();
    newTextureCoords->point.setValues(0, static_cast<int>(uvs.size()), uvs.data());
    threadSeparator->addChild(newTextureCoords);

    auto* newNormalBinding = new SoNormalBinding();
    newNormalBinding->value = SoNormalBinding::PER_VERTEX_INDEXED;
    threadSeparator->addChild(newNormalBinding);

    auto* newNormals = new SoNormal();
    newNormals->vector.setValues(0, static_cast<int>(normals.size()), normals.data());
    threadSeparator->addChild(newNormals);

    auto* newFaceCoords = new SoCoordinate3();
    newFaceCoords->point.setValues(0, static_cast<int>(vertices.size()), vertices.data());
    threadSeparator->addChild(newFaceCoords);

    auto* newFaceSet = new SoIndexedFaceSet();
    newFaceSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
    threadSeparator->addChild(newFaceSet);

    return threadSeparator;
}

std::vector<TopoDS_Face> ViewProviderHole::collectBoreFaces(const PartDesign::Hole* pcHole, const TopoDS_Shape& holeShape, gp_Dir& holeFeatureAxis, gp_Pnt& axisLocationPnt) const
{
    std::vector<TopoDS_Face> boreFaces;
    bool axisDetermined = false;
    const double holeRadius = pcHole->Diameter.getValue() / 2.0;
    const bool isTapered = pcHole->Tapered.getValue();
    const double coneSemiAngleRad = isTapered ? Base::toRadians(pcHole->TaperedAngle.getValue() * 0.5) : 0.0;
    for (TopExp_Explorer expl(holeShape, TopAbs_FACE); expl.More(); expl.Next()) {
        const auto& face = TopoDS::Face(expl.Current());
        auto surface = BRep_Tool::Surface(face);
        if (surface.IsNull()) {continue;}

        gp_Dir currentFaceAxis;
        gp_Pnt currentFaceLocation;
        bool isMainBoreCandidate = false;

        if (!isTapered) {
            auto cyl = Handle(Geom_CylindricalSurface)::DownCast(surface);
            if (!cyl.IsNull()) {
                currentFaceAxis = cyl->Axis().Direction();
                currentFaceLocation = cyl->Axis().Location();
                if (std::abs(cyl->Radius() - holeRadius) < Precision::Confusion()) {
                    isMainBoreCandidate = true;
                }
            }
        } else if (isTapered) {
            auto con = Handle(Geom_ConicalSurface)::DownCast(surface);
            if (!con.IsNull()) {
                currentFaceAxis = con->Axis().Direction();
                currentFaceLocation = con->Axis().Location();
                if (std::abs(con->SemiAngle()) - coneSemiAngleRad < Precision::Confusion()) {
                    isMainBoreCandidate = true;
                }
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
                                            double& outMinProj, double& outMaxProj,
                                            std::vector<SbVec3f>& vertices, std::vector<SbVec3f>& normals,
                                            std::vector<int>& indices, std::vector<SbVec2f>& uvs) const
{
    const double threadPitch = pcHole->getThreadPitch();
    if (threadPitch == 0.0) {
        return false;
    }
    vertices.clear();
    normals.clear();
    indices.clear();
    uvs.clear();

    outMinProj = std::numeric_limits<double>::max();
    outMaxProj = std::numeric_limits<double>::lowest();

    // Calculate UVs
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
    gp_Vec refVecInPlane = (std::abs(holeFeatureAxis.Dot(gp_Dir(1,0,0))) < (1 - Precision::Confusion()))
                           ? holeFeatureAxis.Crossed(gp_Dir(1,0,0))
                           : holeFeatureAxis.Crossed(gp_Dir(0,1,0));
    refVecInPlane.Normalize();
    const gp_Vec crossRefVec = holeFeatureAxis.Crossed(refVecInPlane);

    for (const auto& face : boreFaces) {
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (!Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            continue;
        }
        std::vector<int> localToGlobalIndexMap(meshPoints.size());

        const double holeRadius = pcHole->Diameter.getValue() / 2.0;
        const double coneSemiAngleRad = pcHole->Tapered.getValue() ? Base::toRadians(pcHole->TaperedAngle.getValue() * 0.5) : 0.0;
        const double initialRadius = (outMinProj * std::tan(coneSemiAngleRad)) + holeRadius;

        for (size_t i = 0; i < meshPoints.size(); ++i) {
            const auto& vertexPoint = meshPoints[i];
            const gp_Vec toPoint(axisLocationPnt, vertexPoint);
            const gp_Vec radialComponent = toPoint - (toPoint.Dot(holeFeatureAxis) * holeFeatureAxis);
            const double axialDistance = toPoint.Dot(holeFeatureAxis) - outMinProj;

            const double currentRadius = radialComponent.Magnitude();
            const double lengthAlongTaper = std::sqrt((axialDistance * axialDistance) + (currentRadius - initialRadius) * (currentRadius - initialRadius));

            const float vCoord = static_cast<float>(lengthAlongTaper / threadPitch);

            const double angleRad = std::atan2(radialComponent.Dot(crossRefVec), radialComponent.Dot(refVecInPlane));
            float uCoord = static_cast<float>(angleRad / (2.0 * M_PI));
            uCoord -= std::floor(uCoord);

            vertices.emplace_back(vertexPoint.X(), vertexPoint.Y(), vertexPoint.Z());

            const gp_Dir normalDir = (radialComponent.SquareMagnitude() > std::pow(Precision::Confusion(), 2))
                                     ? gp_Dir(radialComponent)
                                     : holeFeatureAxis;
            normals.emplace_back(normalDir.X(), normalDir.Y(), normalDir.Z());

            uvs.emplace_back(uCoord, vCoord);
            localToGlobalIndexMap[i] = static_cast<int>(vertices.size()) - 1;
        }

        for (const auto& facet : meshFacets) {
            Standard_Integer n1 = 1;
            Standard_Integer n2 = 1;
            Standard_Integer n3 = 1;
            facet.Get(n1, n2, n3);

            std::array<int, 3> triangleIndices = {
                localToGlobalIndexMap[static_cast<size_t>(n1 - 1)],
                localToGlobalIndexMap[static_cast<size_t>(n2 - 1)],
                localToGlobalIndexMap[static_cast<size_t>(n3 - 1)]
            };

            const float u0 = uvs[triangleIndices[0]][0];
            const float u1 = uvs[triangleIndices[1]][0];
            const float u2 = uvs[triangleIndices[2]][0];

            constexpr float uvSeamThreshold = 0.5F;
            const bool crossesSeam = std::abs(u0 - u1) > uvSeamThreshold ||
                                     std::abs(u1 - u2) > uvSeamThreshold ||
                                     std::abs(u2 - u0) > uvSeamThreshold;

            if (crossesSeam) {
                for (int j = 0; j < 3; ++j) {
                    if (uvs[triangleIndices.at(j)][0] < uvSeamThreshold) {
                        const int oldIndex = triangleIndices.at(j);

                        SbVec2f newUV = uvs[oldIndex];
                        newUV[0] += 1.0F;

                        const int newIndex = static_cast<int>(vertices.size());
                        vertices.push_back(vertices[oldIndex]);
                        normals.push_back(normals[oldIndex]);
                        uvs.push_back(newUV);

                        triangleIndices.at(j) = newIndex;
                    }
                }
            }
            indices.push_back(triangleIndices[0]);
            indices.push_back(triangleIndices[1]);
            indices.push_back(triangleIndices[2]);
            indices.push_back(-1);
        }
    }
    return true;
}
