// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Caio Venâncio <caio.venancio784@gmail.com>          *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include <QMenu>
#include <QMessageBox>

#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Poly_Triangle.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
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

#include <Mod/PartDesign/App/FeatureThread.h>
#include <App/PropertyStandard.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Material.h>
#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Base/Placement.h>
#include <Base/Tools.h>
#include <App/Property.h>
#include <Utilities.h>
#include <Gui/Inventor/SoToggleSwitch.h>

#include "TaskThreadParameters.h"
#include "ViewProviderThread.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderThread, PartDesignGui::ViewProviderDressUp)

ViewProviderThread::ViewProviderThread()
    : textureExtension(std::make_unique<Gui::ViewProviderTextureExtension>())
    , pcReducedBasePreview(new PartGui::SoPreviewShape)
    , pcReducedBaseToggle(new SoToggleSwitch)
{
    sPixmap = "PartDesign_Thread.svg";
    menuName = tr("Thread Parameters");
}


ViewProviderThread::~ViewProviderThread()
{
    clearThreadTextures();
}

bool ViewProviderThread::onDelete(const std::vector<std::string>& arg)
{
    restoreBaseVisibility();
    clearThreadTextures();
    return PartDesignGui::ViewProviderDressUp::onDelete(arg);
}

void ViewProviderThread::clearThreadTextures()
{
    SoGroup* root = overlayRoot.get();

    for (auto const& [thread, threadSwitch] : m_threadOverlays) {
        if (root && root->findChild(threadSwitch) >= 0) {
            root->removeChild(threadSwitch);
        }
        threadSwitch->unref();
    }
    m_threadOverlays.clear();
    overlayRoot.reset();
    m_startThreadClipper = nullptr;
    m_endThreadClipper = nullptr;
    m_textureTransform = nullptr;
}

const std::string& ViewProviderThread::featureName() const
{
    static const std::string name = "Thread";
    return name;
}

void ViewProviderThread::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Thread"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderThread::getEditDialog()
{
    return new TaskDlgThreadParameters(this);
}

void ViewProviderThread::updateData(const App::Property* prop)
{
    PartDesignGui::ViewProviderDressUp::updateData(prop);

    auto* thread = getObject<PartDesign::Thread>();
    if (!thread || !prop) {
        return;
    }

    if (prop == &thread->CosmeticThread || prop == &thread->ModelThread || prop == &thread->Tapered
        || prop == &thread->TaperedAngle || prop == &thread->LateralFace
        || prop == &thread->StartPlane || prop == &thread->UpToGeometry
        || prop == &thread->DepthType || prop == &thread->Depth || prop == &thread->Diameter
        || prop == &thread->ThreadSize || prop == &thread->ThreadSizePitch
        || prop == &thread->ThreadType || prop == &thread->Shape) {
        if (thread->getParents().empty()) {
            return;
        }
        updateOverlay();
        return;
    }
    if (prop == &thread->ThreadDirection) {
        updateThreadDirection(thread);
        return;
    }
    if (prop == &thread->Visibility) {
        updateOverlay();
        return;
    }
}

void ViewProviderThread::onChanged(const App::Property* prop)
{
    ViewProviderDressUp::onChanged(prop);

    if (prop == &Visibility) {
        syncExternalPreviewVisibility();
    }
}

void ViewProviderThread::attachPreview()
{
    ViewProviderDressUp::attachPreview();

    pcReducedBasePreview->transparency = 0.0F;
    pcReducedBasePreview->lineWidth.connectFrom(&pcPreviewShape->lineWidth);
    pcReducedBaseToggle->addChild(pcReducedBasePreview);

    pcPreviewRoot->insertChild(pcReducedBaseToggle, 0);
}

void ViewProviderThread::updatePreview()
{
    PartGui::ViewProviderPartExt::setupCoinGeometry(
        TopoDS_Shape(),
        pcPreviewShape,
        Deviation.getValue(),
        AngularDeflection.getValue()
    );

    PartGui::ViewProviderPreviewExtension::updatePreview();

    auto* thread = getObject<PartDesign::Thread>();
    Part::TopoShape reducedBase = thread ? thread->getReducedBasePreviewShape() : Part::TopoShape();
    PartGui::ViewProviderPartExt::setupCoinGeometry(
        reducedBase.getShape(),
        pcReducedBasePreview,
        Deviation.getValue(),
        AngularDeflection.getValue()
    );

    if (auto* baseViewProvider = getBaseFeatureViewProvider()) {
        pcReducedBasePreview->color.setValue(
            baseViewProvider->ShapeAppearance.getDiffuseColor().asValue<SbColor>()
        );
    }

    syncExternalPreviewVisibility();
}

void ViewProviderThread::showPreview(bool enable)
{
    ViewProviderDressUp::showPreview(enable);
    syncExternalPreviewVisibility();
}

PartDesignGui::ViewProvider* ViewProviderThread::getBaseFeatureViewProvider() const
{
    auto* thread = getObject<PartDesign::Thread>();
    auto* baseFeature = thread ? freecad_cast<PartDesign::Feature*>(thread->BaseFeature.getValue())
                               : nullptr;

    return baseFeature ? freecad_cast<PartDesignGui::ViewProvider*>(
                             Gui::Application::Instance->getViewProvider(baseFeature)
                         )
                       : nullptr;
}

void ViewProviderThread::restoreBaseVisibility()
{
    if (!baseHiddenForPreview) {
        return;
    }

    if (auto* baseViewProvider = getBaseFeatureViewProvider()) {
        if (baseViewProvider->Visibility.getValue()) {
            baseViewProvider->Gui::ViewProvider::show();
        }
    }
    baseHiddenForPreview = false;
}

void ViewProviderThread::syncExternalPreviewVisibility()
{
    auto* thread = getObject<PartDesign::Thread>();
    const bool replaceBase = thread && isPreviewEnabled() && !Visibility.getValue()
        && thread->ModelThread.getValue() && !thread->IsInternal.getValue()
        && !thread->getReducedBasePreviewShape().isNull();

    pcReducedBaseToggle->on = replaceBase;

    if (!replaceBase) {
        restoreBaseVisibility();
        return;
    }

    if (auto* baseViewProvider = getBaseFeatureViewProvider()) {
        baseViewProvider->Gui::ViewProvider::hide();
        baseHiddenForPreview = true;
    }
}

SoSeparator* ViewProviderThread::createThreadTextureSeparator()
{

    auto* thread = getObject<PartDesign::Thread>();
    if (!thread) {
        return nullptr;
    }

    if (thread->isRecomputing() || thread->isError()) {
        return nullptr;
    }

    std::optional<PartDesign::Thread::ThreadExtent> extent;
    try {
        extent = thread->resolveThreadExtent();
    }
    catch (const Standard_Failure&) {
        return nullptr;
    }
    catch (const Base::Exception&) {
        return nullptr;
    }
    catch (const std::exception&) {
        return nullptr;
    }
    const gp_Pnt threadOriginPnt = extent->origin;

    std::vector<SbVec3f> vertices;
    std::vector<SbVec3f> normals;
    std::vector<int> indices;
    std::vector<SbVec2f> uvs;

    bool meshGenerated = false;
    try {
        meshGenerated = generateBoreMeshData(thread, threadOriginPnt, vertices, normals, indices, uvs);
    }
    catch (const Standard_Failure&) {
        return nullptr;
    }
    catch (const Base::Exception&) {
        return nullptr;
    }
    if (!meshGenerated || vertices.empty() || normals.empty() || indices.empty() || uvs.empty()) {
        return nullptr;
    }

    auto* threadSep = new SoSeparator();
    threadSep->ref();

    auto* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    threadSep->addChild(pickStyle);

    auto* tt = new SoTransparencyType();
    tt->value = SoTransparencyType::DELAYED_BLEND;
    threadSep->addChild(tt);

    m_startThreadClipper = new SoClipPlane();
    threadSep->addChild(m_startThreadClipper);

    m_endThreadClipper = new SoClipPlane();
    threadSep->addChild(m_endThreadClipper);

    auto* mat = new SoMaterial();
    if (textureExtension) {
        App::Material globalMat = getGlobalMaterial();
        textureExtension->setCoinAppearance(mat, globalMat);
    }
    threadSep->addChild(mat);

    auto* threadTexture = new SoTexture2();
    threadTexture->filename.setValue(":/images/ThreadOverlay.png");
    threadTexture->wrapS = SoTexture2::REPEAT;
    threadTexture->wrapT = SoTexture2::REPEAT;
    threadSep->addChild(threadTexture);

    m_textureTransform = new SoTexture2Transform();
    updateThreadDirection(thread);
    threadSep->addChild(m_textureTransform);

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

    updateThreadClipper(extent->origin, extent->direction, extent->length);
    applyThreadPhaseOffset(thread);

    return threadSep;
}

void ViewProviderThread::updateThreadDirection(const PartDesign::Thread* thread)
{
    if (!thread || !m_textureTransform) {
        return;
    }
    if (thread->ThreadDirection.getValue() == 0) {
        m_textureTransform->scaleFactor.setValue(SbVec2f(-1.0F, 1.0F));
    }
    else {
        m_textureTransform->scaleFactor.setValue(SbVec2f(1.0F, 1.0F));
    }
}

void ViewProviderThread::applyThreadPhaseOffset(const PartDesign::Thread* thread)
{
    if (!thread || !m_textureTransform) {
        return;
    }
    const std::string key = thread->getNameInDocument();
    const unsigned hash = std::hash<std::string> {}(key);
    constexpr float invMax = 1.0F / static_cast<float>(std::numeric_limits<unsigned>::max());
    const float phase = static_cast<float>(hash) * invMax;
    m_textureTransform->translation.setValue(SbVec2f(phase, 0.0F));
}

void ViewProviderThread::updateThreadClipper(const gp_Pnt& origin, const gp_Dir& direction, double length)
{
    if (!m_startThreadClipper || !m_endThreadClipper) {
        return;
    }

    const gp_Pnt end = origin.Translated(gp_Vec(direction) * length);
    m_startThreadClipper->plane.setValue(
        SbPlane(Base::convertTo<SbVec3f>(direction), Base::convertTo<SbVec3f>(origin))
    );
    m_endThreadClipper->plane.setValue(
        SbPlane(Base::convertTo<SbVec3f>(direction.Reversed()), Base::convertTo<SbVec3f>(end))
    );
    m_startThreadClipper->on = TRUE;
    m_endThreadClipper->on = TRUE;
}

std::optional<gp_Dir> ViewProviderThread::getThreadNormal(const PartDesign::Thread* thread) const
{
    return thread ? thread->getThreadNormal() : std::nullopt;
}

std::optional<gp_Pnt> ViewProviderThread::getThreadOrigin(const PartDesign::Thread* thread) const
{
    return thread ? thread->getThreadOrigin() : std::nullopt;
}

std::vector<gp_Pnt> ViewProviderThread::getThreadLocations(const PartDesign::Thread* thread) const
{
    if (!thread) {
        return {};
    }
    return thread->getThreadLocations();
}

namespace
{
struct ThreadSurfaceSection
{
    gp_Ax1 axis;
    double radius;
    double slope;
};

std::optional<ThreadSurfaceSection> describeThreadSurface(
    const TopoDS_Face& face,
    const gp_Pnt& origin,
    const gp_Dir& threadAxis
)
{
    BRepAdaptor_Surface surface(face);
    if (surface.GetType() == GeomAbs_Cylinder) {
        const gp_Cylinder cylinder = surface.Cylinder();
        return ThreadSurfaceSection {cylinder.Axis(), cylinder.Radius(), 0.0};
    }
    if (surface.GetType() == GeomAbs_Cone) {
        const gp_Cone cone = surface.Cone();
        const gp_Ax1 axis = cone.Axis();
        const double slope = std::tan(cone.SemiAngle());
        const double distance = gp_Vec(axis.Location(), origin).Dot(gp_Vec(axis.Direction()));
        return ThreadSurfaceSection {
            axis,
            cone.RefRadius() + distance * slope,
            slope * axis.Direction().Dot(threadAxis)
        };
    }
    return std::nullopt;
}
}  // namespace

std::vector<TopoDS_Face> ViewProviderThread::collectBoreFaces(
    const PartDesign::Thread* thread,
    const gp_Pnt& threadOrigin
) const
{
    std::vector<TopoDS_Face> boreFaces;
    if (!thread) {
        return boreFaces;
    }
    const TopoDS_Shape bodyShape = getCurrentlyVisibleShape(thread);
    const auto threadAxis = getThreadNormal(thread);
    auto* support = dynamic_cast<Part::Feature*>(thread->LateralFace.getValue());
    const auto& subs = thread->LateralFace.getSubValues();
    if (bodyShape.IsNull() || !threadAxis || !support || subs.size() != 1) {
        return boreFaces;
    }

    std::optional<ThreadSurfaceSection> selected;
    try {
        const TopoDS_Shape shape = support->Shape.getShape().getSubShape(subs.front().c_str());
        if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE) {
            return boreFaces;
        }
        selected = describeThreadSurface(TopoDS::Face(shape), threadOrigin, *threadAxis);
    }
    catch (const Standard_Failure&) {
        return boreFaces;
    }
    catch (const Base::Exception&) {
        return boreFaces;
    }
    if (!selected || !std::isfinite(selected->radius) || selected->radius <= Precision::Confusion()) {
        return boreFaces;
    }

    for (TopExp_Explorer expl(bodyShape, TopAbs_FACE); expl.More(); expl.Next()) {
        const TopoDS_Face face = TopoDS::Face(expl.Current());
        const auto section = describeThreadSurface(face, threadOrigin, *threadAxis);
        if (!section || !section->axis.Direction().IsParallel(*threadAxis, Precision::Angular())) {
            continue;
        }
        if (gp_Lin(section->axis).Distance(threadOrigin) > 2.0 * Precision::Confusion()
            || std::abs(section->radius - selected->radius) > Precision::Confusion()
            || std::abs(std::atan(section->slope) - std::atan(selected->slope))
                > Precision::Angular()) {
            continue;
        }
        boreFaces.push_back(face);
    }
    return boreFaces;
}

App::Material ViewProviderThread::getGlobalMaterial()
{

    if (auto* materialProp = dynamic_cast<App::PropertyMaterial*>(getPropertyByName("Material"))) {
        return materialProp->getValue();
    }

    if (auto* bodyVp = getBodyViewProvider()) {
        if (auto* materialProp
            = freecad_cast<App::PropertyMaterial*>(bodyVp->getPropertyByName("Material"))) {
            return materialProp->getValue();
        }
    }

    return App::Material::getDefaultAppearance();
}

TopoDS_Shape ViewProviderThread::getCurrentlyVisibleShape(const PartDesign::Thread* thread) const
{
    auto* body = PartDesign::Body::findBodyOf(thread);
    if (!body) {
        return {};
    }
    const auto& features = body->Group.getValues();
    const auto threadIt = std::ranges::find(features, thread);
    if (threadIt == features.end()) {
        return {};
    }
    for (auto it = threadIt; it != features.end(); ++it) {
        auto* posteriorFeature = dynamic_cast<PartDesign::Feature*>(*it);
        if (posteriorFeature && posteriorFeature->Visibility.getValue()) {
            return posteriorFeature->Shape.getValue();
        }
    }
    return body->Shape.getValue();
}

std::pair<gp_Dir, gp_Dir> ViewProviderThread::buildOrthonormalFrame(const gp_Dir& axis) const
{
    gp_Dir ref(0, 0, 1);
    if (axis.IsParallel(ref, Precision::Angular())) {
        ref = gp_Dir(0, 1, 0);
    }
    gp_Vec xVector = axis.Crossed(ref);
    if (xVector.SquareMagnitude() < Precision::Confusion()) {
        ref = gp_Dir(1, 0, 0);
        xVector = axis.Crossed(ref);
    }
    const gp_Dir xDir(xVector);
    const gp_Dir yDir(axis.Crossed(xDir));
    return {xDir, yDir};
}

SbVec2f ViewProviderThread::addVertex(
    std::vector<SbVec3f>& vertices,
    std::vector<SbVec3f>& normals,
    const gp_Pnt& pt,
    const gp_Pnt& origin,
    const gp_Dir& axis,
    const gp_Dir& xDir,
    const gp_Dir& yDir,
    double surfaceSlope,
    double threadPitch,
    bool reverseNormal
) const
{
    const gp_Vec toPoint(origin, pt);
    const gp_Vec radialComp = toPoint - (toPoint.Dot(axis) * axis);
    const double axialDist = toPoint.Dot(axis);
    const float vCoord = static_cast<float>(axialDist / threadPitch);
    const double angleRad = std::atan2(radialComp.Dot(yDir), radialComp.Dot(xDir));
    float uCoord = static_cast<float>(angleRad / (2 * M_PI));
    uCoord -= std::floor(uCoord);

    vertices.emplace_back(pt.X(), pt.Y(), pt.Z());
    gp_Dir normalDir = (radialComp.SquareMagnitude() > std::pow(Precision::Confusion(), 2))
        ? gp_Dir(radialComp.Normalized() - surfaceSlope * gp_Vec(axis))
        : axis;
    if (reverseNormal) {
        normalDir.Reverse();
    }
    normals.emplace_back(normalDir.X(), normalDir.Y(), normalDir.Z());

    return SbVec2f(uCoord, vCoord);
}

void ViewProviderThread::handleSeamTriangle(
    std::vector<SbVec3f>& vertices,
    std::vector<SbVec3f>& normals,
    std::vector<SbVec2f>& uvs,
    std::array<int, 3>& triIndices
) const
{
    constexpr float seamThreshold = 0.5F;

    const bool crossesSeam = std::abs(uvs[triIndices[0]][0] - uvs[triIndices[1]][0]) > seamThreshold
        || std::abs(uvs[triIndices[1]][0] - uvs[triIndices[2]][0]) > seamThreshold
        || std::abs(uvs[triIndices[2]][0] - uvs[triIndices[0]][0]) > seamThreshold;

    if (!crossesSeam) {
        return;
    }

    const auto originalIndices = triIndices;
    for (size_t index = 0; index < originalIndices.size(); ++index) {
        const int vertexIndex = originalIndices[index];
        if (uvs[vertexIndex][0] < seamThreshold) {
            SbVec2f uv = uvs[vertexIndex];
            uv[0] += 1.0F;
            const int newIdx = static_cast<int>(vertices.size());
            vertices.push_back(vertices[vertexIndex]);
            normals.push_back(normals[vertexIndex]);
            uvs.push_back(uv);
            triIndices[index] = newIdx;
        }
    }
}

bool ViewProviderThread::generateBoreMeshData(
    const PartDesign::Thread* thread,
    const gp_Pnt& threadOriginPnt,
    std::vector<SbVec3f>& vertices,
    std::vector<SbVec3f>& normals,
    std::vector<int>& indices,
    std::vector<SbVec2f>& uvs
) const
{

    const double threadPitch = thread->getThreadPitch();
    if (!std::isfinite(threadPitch) || threadPitch <= Precision::Confusion()) {
        return false;
    }

    vertices.clear();
    normals.clear();
    indices.clear();
    uvs.clear();

    const auto& boreFaces = collectBoreFaces(thread, threadOriginPnt);
    if (boreFaces.empty()) {
        return false;
    }

    const auto threadNormalOpt = getThreadNormal(thread);
    if (!threadNormalOpt.has_value()) {
        return false;
    }
    const gp_Dir threadNormalAxis = *threadNormalOpt;

    const auto [xDir, yDir] = buildOrthonormalFrame(threadNormalAxis);

    bool success = false;

    for (size_t i = 0; i < boreFaces.size(); ++i) {
        const auto& face = boreFaces[i];
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (!Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            continue;
        }

        const auto section = describeThreadSurface(face, threadOriginPnt, threadNormalAxis);
        if (!section) {
            continue;
        }

        std::vector<int> localToGlobalIndex(meshPoints.size());
        for (size_t pIdx = 0; pIdx < meshPoints.size(); ++pIdx) {
            localToGlobalIndex[pIdx] = static_cast<int>(vertices.size());
            uvs.push_back(addVertex(
                vertices,
                normals,
                meshPoints[pIdx],
                threadOriginPnt,
                threadNormalAxis,
                xDir,
                yDir,
                section->slope,
                threadPitch,
                thread->IsInternal.getValue()
            ));
        }

        for (const auto& facet : meshFacets) {
            std::array<int, 3> n = {1, 1, 1};
            facet.Get(n[0], n[1], n[2]);
            std::array<int, 3> triIndices
                = {localToGlobalIndex[n[0]], localToGlobalIndex[n[1]], localToGlobalIndex[n[2]]};
            handleSeamTriangle(vertices, normals, uvs, triIndices);

            indices.insert(indices.end(), {triIndices[0], triIndices[1], triIndices[2], -1});
        }

        success = true;
    }

    return success;
}

bool ViewProviderThread::isHoleThreadVisible() const
{
    return isThreadVisible();
}

bool ViewProviderThread::isThreadVisible() const
{

    auto* thread = getObject<PartDesign::Thread>();
    if (!thread) {
        return false;
    }

    auto* body = PartDesign::Body::findBodyOf(thread);
    if (!body) {
        return false;
    }

    const bool isBodyVisible = body->Visibility.getValue();
    const bool isSuppressed = thread->Suppressed.getValue();
    const bool isCosmetic = thread->CosmeticThread.getValue();
    const bool isModel = thread->ModelThread.getValue();

    if (!isBodyVisible || isSuppressed || !isCosmetic || isModel) {
        return false;
    }

    const auto& features = body->Group.getValues();
    const auto threadIt = std::ranges::find(features, thread);
    if (threadIt == features.end()) {
        return false;
    }

    for (auto it = threadIt; it != features.end(); ++it) {
        auto* posteriorFeature = dynamic_cast<PartDesign::Feature*>(*it);
        if (posteriorFeature) {
            const bool featureVis = posteriorFeature->Visibility.getValue();

            if (featureVis) {
                return true;
            }
        }
    }

    return false;
}

void ViewProviderThread::updateOverlay()
{

    auto* thread = getObject<PartDesign::Thread>();
    if (!thread) {
        return;
    }

    const bool threadVisible = isThreadVisible();

    clearThreadTextures();

    auto* bodyVp = getBodyViewProvider();
    if (!bodyVp) {
        return;
    }

    if (threadVisible) {
        if (SoSeparator* newSep = createThreadTextureSeparator()) {

            auto* threadSwitch = new SoSwitch();
            threadSwitch->ref();
            threadSwitch->addChild(newSep);
            newSep->unref();

            if (bodyVp->getRoot()) {
                overlayRoot = bodyVp->getRoot();
                overlayRoot->addChild(threadSwitch);
                threadSwitch->whichChild = SO_SWITCH_ALL;
                m_threadOverlays[thread] = threadSwitch;
            }
            else {
                Base::Console().error(
                    "[updateOverlay]: Failed to attach SoSwitch — scene graph root is null\n"
                );
                threadSwitch->unref();
            }
        }
    }
}
