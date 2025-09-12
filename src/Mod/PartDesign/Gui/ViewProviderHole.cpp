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
#include <gp_Ax1.hxx>
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
#include "Mod/PartDesign/App/Body.h"
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
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoTransparencyType.h>

#include "ViewProviderHole.h"
#include "TaskHoleParameters.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderHole, PartDesignGui::ViewProvider)


ViewProviderHole::ViewProviderHole()
    : textureExtension(std::make_unique<Gui::ViewProviderTextureExtension>())
{
    sPixmap = "PartDesign_Hole.svg";
}

ViewProviderHole::~ViewProviderHole()
{
    if (m_endThreadClipper) {
        m_endThreadClipper->unref();
        m_endThreadClipper = nullptr;
    }
    if (m_threadTexture) {
        m_threadTexture->unref();
        m_threadTexture = nullptr;
    }
}


std::vector<App::DocumentObject*> ViewProviderHole::claimChildren() const
{
    std::vector<App::DocumentObject*> temp;

    if (App::DocumentObject* profile = getObject<PartDesign::Hole>()->Profile.getValue();
        profile && !profile->isDerivedFrom<PartDesign::Feature>()) {
        temp.push_back(profile);
    }

    return temp;
}

void ViewProviderHole::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Hole"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderHole::getEditDialog()
{
    return new TaskDlgHoleParameters(this);
}

void ViewProviderHole::updateData(const App::Property* prop)
{
    PartDesignGui::ViewProvider::updateData(prop);

    auto* pcHole = getObject<PartDesign::Hole>();
    if (!pcHole || !prop) { return; }

    if (prop == &pcHole->Threaded
        || prop == &pcHole->CosmeticThread
        || prop == &pcHole->ModelThread
    ) {
        if (pcHole->getParents().empty()) {return;}
        App::DocumentObject* parentDO = pcHole->getParents()[0].first;
        Gui::ViewProvider* parentVp = Gui::Application::Instance->getViewProvider(parentDO);
        auto* vpBody = dynamic_cast<PartDesignGui::ViewProviderBody*>(parentVp);
        if (vpBody) {
            vpBody->updateThreadTextureForHole(pcHole);
        }
        return;
    }
    if (prop == &pcHole->ThreadDepth) {
        updateThreadClipper(pcHole);
        return;
    }
    if (prop == &pcHole->ThreadDirection) {
        updateThreadDirection(pcHole);
        return;
    }
}

SoSeparator* ViewProviderHole::createThreadTextureSeparator()
{
    auto* pcHole = getObject<PartDesign::Hole>();
    if (!pcHole) {return nullptr;}

    gp_Pnt holeOriginPnt = getHoleOrigin(pcHole);

    auto boreFaces = collectBoreFaces(pcHole);
    if (boreFaces.empty()) {return nullptr;}

    std::vector<SbVec3f> vertices;
    std::vector<SbVec3f> normals;
    std::vector<int> indices;
    std::vector<SbVec2f> uvs;

    if (!generateBoreMeshData(pcHole, boreFaces, holeOriginPnt, vertices, normals, indices, uvs)
        || indices.empty()
    ) {
        return nullptr;
    }

    // Create subtree
    auto* threadSep = new SoSeparator();
    threadSep->ref();

    // The face is selectable but not the texture
    auto* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    threadSep->addChild(pickStyle);

    auto* tt = new SoTransparencyType();
    tt->value = SoTransparencyType::DELAYED_BLEND;
    threadSep->addChild(tt);

    // End Clipping plane
    m_endThreadClipper = new SoClipPlane();
    threadSep->addChild(m_endThreadClipper);

    // Material
    auto* mat = new SoMaterial();
    textureExtension->setCoinAppearance(mat, getGlobalMaterial());
    threadSep->addChild(mat);

    // Texture
    m_threadTexture = new SoTexture2();
    m_threadTexture->filename.setValue(
        pcHole->ThreadDirection.getValue() != 0
        ? ":/images/ThreadOverlayR.png"
        : ":/images/ThreadOverlayL.png");
    m_threadTexture->wrapS = SoTexture2::REPEAT;
    m_threadTexture->wrapT = SoTexture2::REPEAT;
    threadSep->addChild(m_threadTexture);


    // Texcoords / normals / geometry
    auto* tc = new SoTextureCoordinate2();
    tc->point.setValues(0, (int)uvs.size(), uvs.data());
    threadSep->addChild(tc);

    auto* nb = new SoNormalBinding();
    nb->value = SoNormalBinding::PER_VERTEX_INDEXED;
    threadSep->addChild(nb);

    auto* ns = new SoNormal();
    ns->vector.setValues(0, (int)normals.size(), normals.data());
    threadSep->addChild(ns);

    auto* coords = new SoCoordinate3();
    coords->point.setValues(0, (int)vertices.size(), vertices.data());
    threadSep->addChild(coords);

    auto* faces = new SoIndexedFaceSet();
    faces->coordIndex.setValues(0, (int)indices.size(), indices.data());
    threadSep->addChild(faces);

    updateThreadClipper(pcHole);

    return threadSep;
}

void ViewProviderHole::updateThreadDirection(const PartDesign::Hole* pcHole)
{
    if (!pcHole || !m_threadTexture) {
        return;
    }

    m_threadTexture->filename.setValue(
        pcHole->ThreadDirection.getValue() != 0
        ? ":/images/ThreadOverlayR.png"
        : ":/images/ThreadOverlayL.png");
}

void ViewProviderHole::updateThreadClipper(const PartDesign::Hole* pcHole)
{
    if (!pcHole
        || pcHole->isRecomputing()
        || !m_endThreadClipper
    ) {
        return;
    }
    if (static_cast<std::string>(pcHole->ThreadDepthType.getValueAsString()) == "Hole depth") {
        m_endThreadClipper->on = FALSE;
        return;
    }
    m_endThreadClipper->on = TRUE;
    // Collect bore faces and hole axis
    gp_Dir holeNormalAxis = getHoleNormal(pcHole);
    gp_Pnt holeOriginPnt = getHoleOrigin(pcHole);
    auto boreFaces = collectBoreFaces(pcHole);
    if (boreFaces.empty()) {return;}

    // Compute clipping plane origin at the end of the threaded portion
    gp_Pnt endPlanePnt = holeOriginPnt.Translated(
        gp_Vec(holeNormalAxis) * -pcHole->ThreadDepth.getValue()
    );

    SbVec3f endPlanePoint(
        static_cast<float>(endPlanePnt.X()),
        static_cast<float>(endPlanePnt.Y()),
        static_cast<float>(endPlanePnt.Z())
    );

    SbVec3f endPlaneNormal(
        static_cast<float>(holeNormalAxis.X()),
        static_cast<float>(holeNormalAxis.Y()),
        static_cast<float>(holeNormalAxis.Z())
    );

    // Update the end thread clipper plane
    m_endThreadClipper->plane.setValue(SbPlane(endPlaneNormal, endPlanePoint));
}

gp_Dir ViewProviderHole::getHoleNormal(const PartDesign::Hole* pcHole) const
{
    if (!pcHole) {
        return {0, 0, 1};
}
    Base::Vector3d normal = pcHole->guessNormalDirection(pcHole->getProfileShape());
    return {normal.x, normal.y, normal.z};
}

gp_Pnt ViewProviderHole::getHoleOrigin(const PartDesign::Hole* pcHole) const
{
    if (!pcHole) {
        return {0, 0, 0};
    }
    auto* sketch = dynamic_cast<Part::Part2DObject*>(pcHole->Profile.getValue());
    const Base::Vector3d& pos = sketch->Placement.getValue().getPosition();
    return {pos.x, pos.y, pos.z};
}

std::vector<TopoDS_Face> ViewProviderHole::collectBoreFaces(const PartDesign::Hole* pcHole) const
{
    std::vector<TopoDS_Face> boreFaces;
    if (!pcHole) {return boreFaces;}

    const double holeRadius = pcHole->Diameter.getValue() / 2.0;
    const bool isTapered = pcHole->Tapered.getValue();
    const double taperSemiAngleRad = isTapered ?
        Base::toRadians(90 - pcHole->TaperedAngle.getValue()) : 0.0;

    TopoDS_Shape bodyShape = getLastShownShape(pcHole);
    if (bodyShape.IsNull()) {return boreFaces;}

    // Sketch axis direction
    gp_Dir holeAxis = getHoleNormal(pcHole);

    // Find bore faces
    for (TopExp_Explorer expl(bodyShape, TopAbs_FACE); expl.More(); expl.Next()) {
        const TopoDS_Face& face = TopoDS::Face(expl.Current());
        Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
        if (surface.IsNull()) {continue;}

        bool isCandidate = false;
        gp_Ax1 axis;

        if (!isTapered) {
            Handle(Geom_CylindricalSurface) cyl = Handle(Geom_CylindricalSurface)::DownCast(surface);
            if (!cyl.IsNull() &&
                std::abs(cyl->Radius() - holeRadius) < Precision::Confusion())
            {
                isCandidate = true;
                axis = cyl->Axis();
            }
        } else {
            Handle(Geom_ConicalSurface) con = Handle(Geom_ConicalSurface)::DownCast(surface);
            if (!con.IsNull() &&
                std::abs(std::abs(con->SemiAngle()) - taperSemiAngleRad) < Precision::Angular())
            {
                isCandidate = true;
                axis = con->Axis();
            }
        }

        if (!isCandidate) {continue;}

        // Check alignment with sketch axis
        gp_Dir currentAxis = axis.Direction();
        if (std::abs(currentAxis.Dot(holeAxis)) < 1.0 - Precision::Confusion()) {continue;}

        boreFaces.push_back(face);
    }
    return boreFaces;
}

App::Material ViewProviderHole::getGlobalMaterial()
{
    if (auto* materialProp = dynamic_cast<App::PropertyMaterial*>(getPropertyByName("Material"))) {
        return materialProp->getValue();
    }
    if (auto* bodyVp = getBodyViewProvider()) {
        if (auto* materialProp = dynamic_cast<App::PropertyMaterial*>(bodyVp->getPropertyByName("Material"))) {
            return materialProp->getValue();
        }
    }

    return App::Material::getDefaultAppearance();
}

TopoDS_Shape ViewProviderHole::getLastShownShape(const PartDesign::Hole* pcHole) const
{
    auto* body = PartDesign::Body::findBodyOf(pcHole);
    if (!body) {
        return {};
    }
    const auto& features = body->Group.getValues();
    auto holeIt = std::ranges::find(features, pcHole);
    if (holeIt == features.end()) {return {};}
    for (auto it = holeIt; it != features.end(); ++it) {
        auto* posteriorFeature = dynamic_cast<PartDesign::Feature*>(*it);
        if (posteriorFeature && posteriorFeature->Visibility.getValue()) {
            return posteriorFeature->Shape.getValue();
        }
    }
    return body->Shape.getValue();
}

bool ViewProviderHole::generateBoreMeshData(const PartDesign::Hole* pcHole, const std::vector<TopoDS_Face>& boreFaces,
                                            const gp_Pnt& holeOriginPnt,
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
    gp_Dir holeNormalAxis = getHoleNormal(pcHole);

    double minProj = std::numeric_limits<double>::max();
    double maxProj = std::numeric_limits<double>::lowest();
    // Calculate UVs
    for (const auto& face : boreFaces) {
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            for (const auto& p : meshPoints) {
                double projection = gp_Vec(holeOriginPnt, p).Dot(holeNormalAxis);
                minProj = std::min(minProj, projection);
                maxProj = std::max(maxProj, projection);
            }
        }
    }
    bool success = false;
    for (const auto& face : boreFaces) {
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (!Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            continue;
        }

        // --- Get local axis from face geometry ---
        Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
        if (surf.IsNull()) { continue; }
        gp_Ax3 surfPos;
        if (auto cyl = Handle(Geom_CylindricalSurface)::DownCast(surf)) {
            surfPos = cyl->Position();
        }
        else if (auto cone = Handle(Geom_ConicalSurface)::DownCast(surf)) {
            surfPos = cone->Position();
        }
        else {
            continue; // skip unsupported surface
        }

        const gp_Dir localAxis   = surfPos.Direction();
        const gp_Pnt localOrigin = surfPos.Location();

        // Build stable orthonormal frame around local axis
        gp_Dir ref(0, 0, 1);
        if (localAxis.IsParallel(ref, Precision::Angular())) {
            ref = gp_Dir(0, 1, 0);
        }

        gp_Vec x_vec = localAxis.Crossed(ref);
        if (x_vec.SquareMagnitude() < Precision::Confusion()) {
            ref = gp_Dir(1, 0, 0);
            x_vec = localAxis.Crossed(ref);
        }
        const gp_Dir x_dir(x_vec);
        const gp_Dir y_dir(localAxis.Crossed(x_dir));

        std::vector<int> localToGlobalIndexMap(meshPoints.size());

        const double holeRadius = pcHole->Diameter.getValue() / 2.0;
        const double coneSemiAngleRad = pcHole->Tapered.getValue()
            ? Base::toRadians(pcHole->TaperedAngle.getValue() * 0.5)
            : 0.0;
        const double initialRadius = (minProj * std::tan(coneSemiAngleRad)) + holeRadius;

        for (size_t i = 0; i < meshPoints.size(); ++i) {
            const auto& vertexPoint = meshPoints[i];
            gp_Vec toPoint(localOrigin, vertexPoint);
            gp_Vec radialComponent = toPoint - (toPoint.Dot(localAxis) * localAxis);
            const double axialDistance = toPoint.Dot(localAxis) - minProj;

            const double currentRadius = radialComponent.Magnitude();
            const double lengthAlongTaper = std::sqrt((axialDistance * axialDistance) + ((currentRadius - initialRadius) * (currentRadius - initialRadius)));

            // V coordinate: thread pitch repetition
            const float vCoord = static_cast<float>(lengthAlongTaper / threadPitch);

            // U coordinate: angle around bore axis
            const double angleRad = std::atan2(
                radialComponent.Dot(y_dir),
                radialComponent.Dot(x_dir)
            );
            float uCoord = static_cast<float>(angleRad / (2.0 * M_PI));
            uCoord -= std::floor(uCoord);

            vertices.emplace_back(vertexPoint.X(), vertexPoint.Y(), vertexPoint.Z());

            const gp_Dir normalDir = (radialComponent.SquareMagnitude() > std::pow(Precision::Confusion(), 2))
                                     ? gp_Dir(radialComponent)
                                     : localAxis;
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
                // Duplicate vertices with adjusted U to fix wrapping
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
        success = true;
    }
    return success;
}
