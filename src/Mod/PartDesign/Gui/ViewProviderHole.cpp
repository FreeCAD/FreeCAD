// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <QMenu>
#include <QMessageBox>


#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Poly_Triangle.hxx>

#include <BRep_Tool.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_Surface.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/FeatureHole.h>
#include <Mod/PartDesign/Gui/ViewProviderHole.h>

#include <Base/Placement.h>
#include <Base/Tools.h>
#include <App/Property.h>

#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTexture2Transform.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
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

ViewProviderHole::~ViewProviderHole() = default;

bool ViewProviderHole::onDelete(const std::vector<std::string>& arg)
{
    clearThreadTextures();
    return PartDesignGui::ViewProvider::onDelete(arg);
}

void ViewProviderHole::clearThreadTextures()
{
    if (m_threadOverlays.empty()) {
        return;
    }

    auto* bodyVp = getBodyViewProvider();
    SoGroup* root = bodyVp ? bodyVp->getRoot() : nullptr;

    for (auto const& [hole, sw] : m_threadOverlays) {
        if (root && root->findChild(sw) >= 0) {
            root->removeChild(sw);
        }
        sw->unref();
    }
    m_threadOverlays.clear();
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
    if (!pcHole || !prop) {
        return;
    }

    if (prop == &pcHole->Threaded || prop == &pcHole->CosmeticThread || prop == &pcHole->ModelThread) {
        if (pcHole->getParents().empty()) {
            return;
        }
        updateOverlay();
        return;
    }
    if (prop == &pcHole->ThreadDepth || prop == &pcHole->ThreadDepthType) {
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
    if (!pcHole) {
        return nullptr;
    }

    gp_Pnt holeOriginPnt;
    auto holeOriginOpt = getHoleOrigin(pcHole);
    if (!holeOriginOpt.has_value()) {
        return nullptr;
    }
    holeOriginPnt = *holeOriginOpt;

    std::vector<SbVec3f> vertices;
    std::vector<SbVec3f> normals;
    std::vector<int> indices;
    std::vector<SbVec2f> uvs;

    if (!generateBoreMeshData(pcHole, holeOriginPnt, vertices, normals, indices, uvs)
        || vertices.empty() || normals.empty() || indices.empty() || uvs.empty()) {
        return nullptr;
    }

    // Create subtree
    auto* threadSep = new SoSeparator();
    threadSep->ref();

    // The face is selectable but not the texture
    auto* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    threadSep->addChild(pickStyle);

    // Avoid flicker on transparent objects
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
    auto* threadTexture = new SoTexture2();
    threadTexture->filename.setValue(":/images/ThreadOverlay.png");
    threadTexture->wrapS = SoTexture2::REPEAT;
    threadTexture->wrapT = SoTexture2::REPEAT;
    threadSep->addChild(threadTexture);

    // --- Texture transform for flipping ---
    m_textureTransform = new SoTexture2Transform();
    updateThreadDirection(pcHole);  // apply initial direction
    threadSep->addChild(m_textureTransform);

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
    applyThreadPhaseOffset(pcHole);

    return threadSep;
}

void ViewProviderHole::updateThreadDirection(const PartDesign::Hole* pcHole)
{
    if (!pcHole || !m_textureTransform) {
        return;
    }
    if (pcHole->ThreadDirection.getValue() == 0) {
        m_textureTransform->scaleFactor.setValue(SbVec2f(-1.0F, 1.0F));
    }
    else {
        m_textureTransform->scaleFactor.setValue(SbVec2f(1.0F, 1.0F));
    }
}

void ViewProviderHole::applyThreadPhaseOffset(const PartDesign::Hole* pcHole)
{
    if (!pcHole || !m_textureTransform) {
        return;
    }
    // Applies a unique offset so overlapping threads can be shown as crossed
    // Uses a stable hash of the hole name so it's deterministic between runs
    const std::string key = pcHole->getNameInDocument();
    unsigned hash = std::hash<std::string> {}(key);
    // Map hash to 0..1 range for UV offset
    constexpr float invMax = 1.0F / static_cast<float>(std::numeric_limits<unsigned>::max());
    const float phase = static_cast<float>(hash) * invMax;
    // Apply only horizontal (U) offset
    m_textureTransform->translation.setValue(SbVec2f(phase, 0.0F));
}

void ViewProviderHole::updateThreadClipper(const PartDesign::Hole* pcHole)
{
    if (!pcHole || pcHole->isRecomputing() || !m_endThreadClipper) {
        return;
    }
    std::string theadDepthType = pcHole->ThreadDepthType.getValueAsString();
    if (theadDepthType == "Hole depth") {
        m_endThreadClipper->on = FALSE;
        Base::Console().message("Thread clip plane disabled\n");
        return;
    }
    m_endThreadClipper->on = TRUE;

    auto holeNormalOpt = getHoleNormal(pcHole);
    if (!holeNormalOpt.has_value()) {
        return;
    }
    gp_Dir holeNormalAxis = *holeNormalOpt;

    auto holeOriginOpt = getHoleOrigin(pcHole);
    if (!holeOriginOpt.has_value()) {
        return;
    }
    gp_Pnt holeOriginPnt = *holeOriginOpt;

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

std::optional<gp_Dir> ViewProviderHole::getHoleNormal(const PartDesign::Hole* pcHole) const
{
    if (!pcHole) {
        return std::nullopt;
    }

    Base::Vector3d normal = pcHole->guessNormalDirection(pcHole->getProfileShape());

    // Reject if direction is mathematically zero (invalid for gp_Dir)
    if (normal.IsNull()) {
        return std::nullopt;
    }

    return Base::convertTo<gp_Dir>(normal);
}

std::optional<gp_Pnt> ViewProviderHole::getHoleOrigin(const PartDesign::Hole* pcHole) const
{
    if (!pcHole) {
        return std::nullopt;
    }

    auto* sketch = freecad_cast<Part::Part2DObject*>(pcHole->Profile.getValue());

    if (!sketch) {
        return std::nullopt;
    }

    const Base::Vector3d& pos = sketch->Placement.getValue().getPosition();
    return Base::convertTo<gp_Pnt>(pos);
}

std::vector<TopoDS_Face> ViewProviderHole::collectBoreFaces(const PartDesign::Hole* pcHole) const
{
    std::vector<TopoDS_Face> boreFaces;
    if (!pcHole) {
        return boreFaces;
    }

    TopoDS_Shape bodyShape = getCurrentlyVisibleShape(pcHole);
    if (bodyShape.IsNull()) {
        return boreFaces;
    }

    auto holeNormalOpt = getHoleNormal(pcHole);
    if (!holeNormalOpt.has_value()) {
        return boreFaces;
    }
    gp_Dir holeAxis = *holeNormalOpt;

    const double holeRadius = pcHole->Diameter.getValue() / 2.0;
    const bool isTapered = pcHole->Tapered.getValue();
    const double taperSemiAngleRad = isTapered
        ? Base::toRadians(90 - pcHole->TaperedAngle.getValue())
        : 0.0;

    for (TopExp_Explorer expl(bodyShape, TopAbs_FACE); expl.More(); expl.Next()) {
        const TopoDS_Face& face = TopoDS::Face(expl.Current());
        Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
        if (surf.IsNull()) {
            continue;
        }

        // Unwrap trimmed surfaces
        if (surf->IsKind(STANDARD_TYPE(Geom_RectangularTrimmedSurface))) {
            surf = Handle(Geom_RectangularTrimmedSurface)::DownCast(surf)->BasisSurface();
        }

        gp_Ax1 axis;
        if (!isTapered) {
            if (!surf->IsKind(STANDARD_TYPE(Geom_CylindricalSurface))) {
                continue;
            }
            auto cyl = Handle(Geom_CylindricalSurface)::DownCast(surf);
            if (std::abs(cyl->Radius() - holeRadius) >= Precision::Confusion()) {
                continue;
            }
            axis = cyl->Axis();
        }
        else {
            if (!surf->IsKind(STANDARD_TYPE(Geom_ConicalSurface))) {
                continue;
            }
            auto con = Handle(Geom_ConicalSurface)::DownCast(surf);
            double angle = std::abs(con->SemiAngle());
            if (std::abs(angle - taperSemiAngleRad) >= Precision::Angular()) {
                continue;
            }
            axis = con->Axis();
        }

        if (!axis.Direction().IsParallel(holeAxis, Precision::Angular())) {
            continue;
        }

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
        if (auto* materialProp
            = dynamic_cast<App::PropertyMaterial*>(bodyVp->getPropertyByName("Material"))) {
            return materialProp->getValue();
        }
    }

    return App::Material::getDefaultAppearance();
}

TopoDS_Shape ViewProviderHole::getCurrentlyVisibleShape(const PartDesign::Hole* pcHole) const
{
    auto* body = PartDesign::Body::findBodyOf(pcHole);
    if (!body) {
        return {};
    }
    const auto& features = body->Group.getValues();
    auto holeIt = std::ranges::find(features, pcHole);
    if (holeIt == features.end()) {
        return {};
    }
    for (auto it = holeIt; it != features.end(); ++it) {
        auto* posteriorFeature = dynamic_cast<PartDesign::Feature*>(*it);
        if (posteriorFeature && posteriorFeature->Visibility.getValue()) {
            return posteriorFeature->Shape.getValue();
        }
    }
    return body->Shape.getValue();
}

std::pair<gp_Dir, gp_Dir> ViewProviderHole::buildOrthonormalFrame(const gp_Dir& axis)
{
    gp_Dir ref(0, 0, 1);
    if (axis.IsParallel(ref, Precision::Angular())) {
        ref = gp_Dir(0, 1, 0);
    }
    gp_Vec x_vec = axis.Crossed(ref);
    if (x_vec.SquareMagnitude() < Precision::Confusion()) {
        ref = gp_Dir(1, 0, 0);
        x_vec = axis.Crossed(ref);
    }
    gp_Dir x_dir(x_vec);
    gp_Dir y_dir(axis.Crossed(x_dir));
    return {x_dir, y_dir};
}

SbVec2f ViewProviderHole::addVertex(
    std::vector<SbVec3f>& vertices,
    std::vector<SbVec3f>& normals,
    const gp_Pnt& pt,
    const gp_Pnt& origin,
    const gp_Dir& axis,
    const gp_Dir& x_dir,
    const gp_Dir& y_dir,
    double minProj,
    double initialRadius,
    double threadPitch
)
{
    gp_Vec toPoint(origin, pt);
    gp_Vec radialComp = toPoint - (toPoint.Dot(axis) * axis);
    double axialDist = toPoint.Dot(axis) - minProj;
    double currentRadius = radialComp.Magnitude();
    double radialOffset = currentRadius - initialRadius;
    double lengthAlongTaper = std::sqrt((axialDist * axialDist) + (radialOffset * radialOffset));

    float vCoord = static_cast<float>(lengthAlongTaper / threadPitch);
    double angleRad = std::atan2(radialComp.Dot(y_dir), radialComp.Dot(x_dir));
    float uCoord = static_cast<float>(angleRad / (2 * M_PI));
    uCoord -= std::floor(uCoord);

    vertices.emplace_back(pt.X(), pt.Y(), pt.Z());
    gp_Dir normalDir = (radialComp.SquareMagnitude() > std::pow(Precision::Confusion(), 2))
        ? gp_Dir(radialComp)
        : axis;
    normals.emplace_back(normalDir.X(), normalDir.Y(), normalDir.Z());

    return SbVec2f(uCoord, vCoord);
}

Handle(Geom_Surface) unwrapSurface(const TopoDS_Face& face)
{
    Handle(Geom_Surface) surf = BRep_Tool::Surface(face);
    if (!surf.IsNull() && surf->IsKind(STANDARD_TYPE(Geom_RectangularTrimmedSurface))) {
        surf = Handle(Geom_RectangularTrimmedSurface)::DownCast(surf)->BasisSurface();
    }
    return surf;
}


void ViewProviderHole::handleSeamTriangle(
    std::vector<SbVec3f>& vertices,
    std::vector<SbVec3f>& normals,
    std::vector<SbVec2f>& uvs,
    std::array<int, 3>& triIndices
)
{
    constexpr float seamThreshold = 0.5F;

    bool crossesSeam = std::abs(uvs[triIndices[0]][0] - uvs[triIndices[1]][0]) > seamThreshold
        || std::abs(uvs[triIndices[1]][0] - uvs[triIndices[2]][0]) > seamThreshold
        || std::abs(uvs[triIndices[2]][0] - uvs[triIndices[0]][0]) > seamThreshold;

    if (!crossesSeam) {
        return;
    }

    int idx0 = triIndices[0];
    int idx1 = triIndices[1];
    int idx2 = triIndices[2];

    if (uvs[idx0][0] < seamThreshold) {
        SbVec2f uv = uvs[idx0];
        uv[0] += 1.0F;
        int newIdx = static_cast<int>(vertices.size());
        vertices.push_back(vertices[idx0]);
        normals.push_back(normals[idx0]);
        uvs.push_back(uv);
        triIndices[0] = newIdx;
    }

    if (uvs[idx1][0] < seamThreshold) {
        SbVec2f uv = uvs[idx1];
        uv[0] += 1.0F;
        int newIdx = static_cast<int>(vertices.size());
        vertices.push_back(vertices[idx1]);
        normals.push_back(normals[idx1]);
        uvs.push_back(uv);
        triIndices[1] = newIdx;
    }

    if (uvs[idx2][0] < seamThreshold) {
        SbVec2f uv = uvs[idx2];
        uv[0] += 1.0F;
        int newIdx = static_cast<int>(vertices.size());
        vertices.push_back(vertices[idx2]);
        normals.push_back(normals[idx2]);
        uvs.push_back(uv);
        triIndices[2] = newIdx;
    }
}

bool ViewProviderHole::generateBoreMeshData(
    const PartDesign::Hole* pcHole,
    const gp_Pnt& holeOriginPnt,
    std::vector<SbVec3f>& vertices,
    std::vector<SbVec3f>& normals,
    std::vector<int>& indices,
    std::vector<SbVec2f>& uvs
)
{
    const double threadPitch = pcHole->getThreadPitch();
    if (threadPitch == 0.0) {
        return false;
    }

    vertices.clear();
    normals.clear();
    indices.clear();
    uvs.clear();

    const auto& boreFaces = collectBoreFaces(pcHole);
    if (boreFaces.empty()) {
        return false;
    }

    auto holeNormalOpt = getHoleNormal(pcHole);
    if (!holeNormalOpt.has_value()) {
        return false;
    }
    gp_Dir holeNormalAxis = *holeNormalOpt;

    double minProj = std::numeric_limits<double>::max();
    double maxProj = std::numeric_limits<double>::lowest();

    // --- Compute projection bounds ---
    for (const auto& face : boreFaces) {
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            for (const auto& p : meshPoints) {
                double proj = gp_Vec(holeOriginPnt, p).Dot(holeNormalAxis);
                minProj = std::min(minProj, proj);
                maxProj = std::max(maxProj, proj);
            }
        }
    }

    const double holeRadius = pcHole->Diameter.getValue() / 2.0;
    const double coneSemiAngleRad = pcHole->Tapered.getValue()
        ? Base::toRadians(pcHole->TaperedAngle.getValue() * 0.5)
        : 0.0;
    const double initialRadius = (minProj * std::tan(coneSemiAngleRad)) + holeRadius;

    bool success = false;

    for (const auto& face : boreFaces) {
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (!Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            continue;
        }

        Handle(Geom_Surface) surf = unwrapSurface(face);
        gp_Ax3 surfPos;
        if (auto cyl = Handle(Geom_CylindricalSurface)::DownCast(surf)) {
            surfPos = cyl->Position();
        }
        else if (auto cone = Handle(Geom_ConicalSurface)::DownCast(surf)) {
            surfPos = cone->Position();
        }
        else {
            continue;
        }

        auto [x_dir, y_dir] = buildOrthonormalFrame(surfPos.Direction());
        gp_Pnt localOrigin = surfPos.Location();

        std::vector<int> localToGlobalIndex(meshPoints.size());
        for (size_t i = 0; i < meshPoints.size(); ++i) {
            localToGlobalIndex[i] = static_cast<int>(vertices.size()),
            uvs.push_back(addVertex(
                vertices,
                normals,
                meshPoints[i],
                localOrigin,
                surfPos.Direction(),
                x_dir,
                y_dir,
                minProj,
                initialRadius,
                threadPitch
            ));
        }
        // --- Build indices ---
        for (const auto& facet : meshFacets) {
            std::array<int, 3> n = {1, 1, 1};
            facet.Get(n[0], n[1], n[2]);
            std::array<int, 3> triIndices = {
                localToGlobalIndex[n[0] - 1],
                localToGlobalIndex[n[1] - 1],
                localToGlobalIndex[n[2] - 1]
            };
            handleSeamTriangle(vertices, normals, uvs, triIndices);

            indices.insert(indices.end(), {triIndices[0], triIndices[1], triIndices[2], -1});
        }
        success = true;
    }

    return success;
}

bool ViewProviderHole::isHoleThreadVisible() const
{
    auto* hole = getObject<PartDesign::Hole>();
    auto* body = PartDesign::Body::findBodyOf(hole);
    if (!body || !body->Visibility.getValue() || hole->Suppressed.getValue()
        || !hole->Threaded.getValue() || !hole->CosmeticThread.getValue()
        || hole->ModelThread.getValue()) {
        return false;
    }
    const auto& features = body->Group.getValues();
    auto holeIt = std::ranges::find(features, hole);
    if (holeIt == features.end()) {
        return false;
    }
    for (auto it = holeIt; it != features.end(); ++it) {
        auto* posteriorFeature = dynamic_cast<PartDesign::Feature*>(*it);
        if (posteriorFeature && posteriorFeature->Visibility.getValue()) {
            return true;
        }
    }
    // We've reached the end and no posterior feature is visible,
    return false;
}

void ViewProviderHole::updateOverlay()
{
    auto* hole = getObject<PartDesign::Hole>();
    bool isThreadVisible = isHoleThreadVisible();
    auto* bodyVp = getBodyViewProvider();
    if (!bodyVp) {
        return;
    }
    // Cleanup
    auto it = m_threadOverlays.find(hole);
    if (it != m_threadOverlays.end()) {
        SoSwitch* existingSwitch = it->second;
        bodyVp->getRoot()->removeChild(existingSwitch);
        existingSwitch->unref();
        m_threadOverlays.erase(it);
    }
    // Add the thread
    if (isThreadVisible) {
        if (SoSeparator* newSep = createThreadTextureSeparator()) {
            auto* threadSwitch = new SoSwitch();
            threadSwitch->ref();
            threadSwitch->addChild(newSep);
            bodyVp->getRoot()->addChild(threadSwitch);
            threadSwitch->whichChild = SO_SWITCH_ALL;
            m_threadOverlays[hole] = threadSwitch;
        }
    }
}
