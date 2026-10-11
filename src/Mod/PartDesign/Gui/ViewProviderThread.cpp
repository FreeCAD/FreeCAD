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

#include <Mod/PartDesign/App/FeatureThread.h>
#include <App/PropertyStandard.h>

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
#include <Gui/Inventor/SoToggleSwitch.h>

#include "TaskThreadParameters.h"
#include "ViewProviderThread.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderThread, PartDesignGui::ViewProviderDressUp)

bool DEBUG = false;

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

    for (auto const& [hole, sw] : m_threadOverlays) {
        if (root && root->findChild(sw) >= 0) {
            root->removeChild(sw);
        }
        sw->unref();
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
    
    auto* pcThread = getObject<PartDesign::Thread>();
    if(!pcThread || !prop) {
        if (DEBUG) Base::Console().message("exit empty\n");
        return;
    }
    
    if (prop == &pcThread->CosmeticThread || prop == &pcThread->ModelThread
        || prop == &pcThread->Tapered || prop == &pcThread->TaperedAngle
        || prop == &pcThread->LateralFace || prop == &pcThread->StartPlane
        || prop == &pcThread->UpToGeometry || prop == &pcThread->DepthType
        || prop == &pcThread->Depth || prop == &pcThread->Diameter
        || prop == &pcThread->ThreadSize || prop == &pcThread->ThreadSizePitch
        || prop == &pcThread->ThreadType || prop == &pcThread->Shape) {
        if (DEBUG) Base::Console().message("I'm running!!!: %s\n", prop->getName());
        // if (DEBUG) Base::Console().message("O prop sou eu!\n");
        if (pcThread->getParents().empty()) {
            if (DEBUG) Base::Console().message("exit parents empty\n");
            return;
        }
        updateOverlay();
        if (DEBUG) Base::Console().message("exit update overlay\n");
        return;
    }
    if(prop == &pcThread->ThreadDirection) {
        updateThreadDirection(pcThread);
        if (DEBUG) Base::Console().message("exit thread direction\n");
        return;
    }
    if (prop == &pcThread->Visibility) {
        if (DEBUG) Base::Console().message("[updateData]: Visibility changed -> updateOverlay\n");
        updateOverlay();
        return;
    }
    // if (DEBUG) Base::Console().message("I'm ending!!!\n");
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

    // Render the opaque reduced base before the translucent thread tool.
    pcPreviewRoot->insertChild(pcReducedBaseToggle, 0);
}

void ViewProviderThread::updatePreview()
{
    // The shared preview updater ignores empty shapes. Clear this feature's retained geometry
    // first so switching from modelled to cosmetic mode cannot leave a stale thread visible.
    PartGui::ViewProviderPartExt::setupCoinGeometry(
        TopoDS_Shape(),
        pcPreviewShape,
        Deviation.getValue(),
        AngularDeflection.getValue()
    );

    // PreviewShape already contains either the additive thread or the intersected removed volume.
    // Do not invoke ViewProvider::updatePreview(), which would add the full subtractive tool too.
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
    auto* baseFeature = thread
        ? freecad_cast<PartDesign::Feature*>(thread->BaseFeature.getValue())
        : nullptr;

    return baseFeature
        ? freecad_cast<PartDesignGui::ViewProvider*>(
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
    if (DEBUG) Base::Console().message("[createThreadTextureSeparator]: Starting texture separator creation...\n");

    auto* pcThread = getObject<PartDesign::Thread>();
    if (!pcThread) {
        if (DEBUG) Base::Console().warning("[createThreadTextureSeparator]: Thread object is null -> nullptr\n");
        return nullptr;
    }

    // The Body rebuilds overlays after recompute. Never resolve against intermediate geometry.
    if (pcThread->isRecomputing() || pcThread->isError()) {
        return nullptr;
    }

    std::optional<PartDesign::Thread::ThreadExtent> extent;
    try {
        extent = pcThread->resolveThreadExtent();
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

    if (DEBUG) Base::Console().message("[createThreadTextureSeparator]: Generating bore mesh data for '%s'...\n", pcThread->getNameInDocument());

    bool meshGenerated = false;
    try {
        meshGenerated = generateBoreMeshData(pcThread, threadOriginPnt, vertices, normals, indices, uvs);
    }
    catch (const Standard_Failure&) {
        return nullptr;
    }
    catch (const Base::Exception&) {
        return nullptr;
    }
    if (!meshGenerated || vertices.empty() || normals.empty() || indices.empty() || uvs.empty()) {
        if (DEBUG) Base::Console().warning("[createThreadTextureSeparator]: Mesh generation failed or produced empty data for '%s' (Verts: %zu, Norms: %zu, Inds: %zu, UVs: %zu) -> nullptr\n",
                                pcThread->getNameInDocument(),
                                vertices.size(), normals.size(), indices.size(), uvs.size());
        return nullptr;
    }

    if (DEBUG) Base::Console().message("[createThreadTextureSeparator]: Mesh data generated successfully (Verts: %zu, Inds: %zu). Assembling Coin3D nodes...\n",
                            vertices.size(), indices.size());

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

    // Limit the cosmetic overlay to the same nominal interval as the modelled thread.
    m_startThreadClipper = new SoClipPlane();
    threadSep->addChild(m_startThreadClipper);

    m_endThreadClipper = new SoClipPlane();
    threadSep->addChild(m_endThreadClipper);

    // Material
    if (DEBUG) Base::Console().message("[createThreadTextureSeparator]: Setting up material...\n");
    auto* mat = new SoMaterial();
    if (textureExtension) {
        App::Material globalMat = getGlobalMaterial();
        if (DEBUG) Base::Console().message("[createThreadTextureSeparator]: Applying coin appearance from textureExtension...\n");
        textureExtension->setCoinAppearance(mat, globalMat);
    } else {
        if (DEBUG) Base::Console().warning("[createThreadTextureSeparator]: textureExtension pointer is NULL! Skipping appearance setup.\n");
    }
    threadSep->addChild(mat);

    // Texture
    auto* threadTexture = new SoTexture2();
    threadTexture->filename.setValue(":/images/ThreadOverlay.png");
    threadTexture->wrapS = SoTexture2::REPEAT;
    threadTexture->wrapT = SoTexture2::REPEAT;
    threadSep->addChild(threadTexture);

    // --- Texture transform for flipping ---
    m_textureTransform = new SoTexture2Transform();
    updateThreadDirection(pcThread);  // apply initial direction
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

    if (DEBUG) Base::Console().message("[createThreadTextureSeparator]: Updating clipper and phase offset...\n");
    updateThreadClipper(extent->origin, extent->direction, extent->length);
    applyThreadPhaseOffset(pcThread);

    if (DEBUG) Base::Console().message("[createThreadTextureSeparator]: Successfully created thread texture separator for '%s'\n", pcThread->getNameInDocument());

    return threadSep;
}

void ViewProviderThread::updateThreadDirection(const PartDesign::Thread* pcThread)
{
    if (!pcThread || !m_textureTransform) {
        return;
    }
    if (pcThread->ThreadDirection.getValue() == 0) {
        m_textureTransform->scaleFactor.setValue(SbVec2f(-1.0F, 1.0F));
    }
    else {
        m_textureTransform->scaleFactor.setValue(SbVec2f(1.0F, 1.0F));
    }
}

void ViewProviderThread::applyThreadPhaseOffset(const PartDesign::Thread* pcThread)
{
    if (!pcThread || !m_textureTransform) {
        return;
    }
    // Applies a unique offset so overlapping threads can be shown as crossed
    // Uses a stable hash of the hole name so it's deterministic between runs
    const std::string key = pcThread->getNameInDocument();
    unsigned hash = std::hash<std::string> {}(key);
    // Map hash to 0..1 range for UV offset
    constexpr float invMax = 1.0F / static_cast<float>(std::numeric_limits<unsigned>::max());
    const float phase = static_cast<float>(hash) * invMax;
    // Apply only horizontal (U) offset
    m_textureTransform->translation.setValue(SbVec2f(phase, 0.0F));
}

void ViewProviderThread::updateThreadClipper(
    const gp_Pnt& origin,
    const gp_Dir& direction,
    double length
)
{
    if (!m_startThreadClipper || !m_endThreadClipper) {
        return;
    }

    const gp_Pnt end = origin.Translated(gp_Vec(direction) * length);
    m_startThreadClipper->plane.setValue(SbPlane(
        Base::convertTo<SbVec3f>(direction), Base::convertTo<SbVec3f>(origin)
    ));
    m_endThreadClipper->plane.setValue(SbPlane(
        Base::convertTo<SbVec3f>(direction.Reversed()), Base::convertTo<SbVec3f>(end)
    ));
    m_startThreadClipper->on = TRUE;
    m_endThreadClipper->on = TRUE;
}

std::optional<gp_Dir> ViewProviderThread::getThreadNormal(const PartDesign::Thread* pcThread) const
{
    return pcThread ? pcThread->getThreadNormal() : std::nullopt;
}

std::optional<gp_Pnt> ViewProviderThread::getThreadOrigin(const PartDesign::Thread* pcThread) const
{
    return pcThread ? pcThread->getThreadOrigin() : std::nullopt;
}

std::vector<gp_Pnt> ViewProviderThread::getThreadLocations(const PartDesign::Thread* pcThread) const
{
    if (!pcThread) {
        return {};
    }
    return pcThread->getThreadLocations();
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
    const TopoDS_Face& face, const gp_Pnt& origin, const gp_Dir& threadAxis
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
    const PartDesign::Thread* pcThread, const gp_Pnt& threadOrigin
) const
{
    std::vector<TopoDS_Face> boreFaces;
    if (!pcThread) {
        return boreFaces;
    }
    const TopoDS_Shape bodyShape = getCurrentlyVisibleShape(pcThread);
    const auto threadAxis = getThreadNormal(pcThread);
    auto* support = dynamic_cast<Part::Feature*>(pcThread->LateralFace.getValue());
    const auto& subs = pcThread->LateralFace.getSubValues();
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
    if (!selected || !std::isfinite(selected->radius)
        || selected->radius <= Precision::Confusion()) {
        return boreFaces;
    }

    // Match the selected support at the resolved entrance, not at each surface's
    // reference plane. Manual taper changes must not hide the original support.
    for (TopExp_Explorer expl(bodyShape, TopAbs_FACE); expl.More(); expl.Next()) {
        const TopoDS_Face face = TopoDS::Face(expl.Current());
        const auto section = describeThreadSurface(face, threadOrigin, *threadAxis);
        if (!section || !section->axis.Direction().IsParallel(*threadAxis, Precision::Angular())) {
            continue;
        }
        if (gp_Lin(section->axis).Distance(threadOrigin) > 2.0 * Precision::Confusion()
            || std::abs(section->radius - selected->radius) > Precision::Confusion()
            || std::abs(std::atan(section->slope) - std::atan(selected->slope)) > Precision::Angular()) {
            continue;
        }
        boreFaces.push_back(face);
    }
    return boreFaces;
}

App::Material ViewProviderThread::getGlobalMaterial()
{
    if (DEBUG) Base::Console().message("[getGlobalMaterial]: Fetching global material...\n");

    if (auto* materialProp = dynamic_cast<App::PropertyMaterial*>(getPropertyByName("Material"))) {
        if (DEBUG) Base::Console().message("[getGlobalMaterial]: Found material directly on Thread ViewProvider\n");
        return materialProp->getValue();
    }
    else {
        if (DEBUG) Base::Console().message("[getGlobalMaterial]: Material property not found on Thread ViewProvider\n");
    }

    if (auto* bodyVp = getBodyViewProvider()) {
        if (DEBUG) Base::Console().message("[getGlobalMaterial]: Found parent Body ViewProvider, checking material...\n");
        if (auto* materialProp
            = freecad_cast<App::PropertyMaterial*>(bodyVp->getPropertyByName("Material"))) {
            if (DEBUG) Base::Console().message("[getGlobalMaterial]: Found material on parent Body ViewProvider\n");
            return materialProp->getValue();
        }
        else {
            if (DEBUG) Base::Console().message("[getGlobalMaterial]: Material property not found on parent Body ViewProvider\n");
        }
    }
    else {
        if (DEBUG) Base::Console().warning("[getGlobalMaterial]: Body ViewProvider is NULL!\n");
    }

    if (DEBUG) Base::Console().message("[getGlobalMaterial]: Falling back to App::Material::getDefaultAppearance()\n");
    return App::Material::getDefaultAppearance();
}

TopoDS_Shape ViewProviderThread::getCurrentlyVisibleShape(const PartDesign::Thread* pcThread) const
{
    auto* body = PartDesign::Body::findBodyOf(pcThread);
    if (!body) {
        return {};
    }
    const auto& features = body->Group.getValues();
    auto threadIt = std::ranges::find(features, pcThread);
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

std::pair<gp_Dir, gp_Dir> ViewProviderThread::buildOrthonormalFrame(const gp_Dir& axis)
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

SbVec2f ViewProviderThread::addVertex(
    std::vector<SbVec3f>& vertices,
    std::vector<SbVec3f>& normals,
    const gp_Pnt& pt,
    const gp_Pnt& origin,
    const gp_Dir& axis,
    const gp_Dir& x_dir,
    const gp_Dir& y_dir,
    double surfaceSlope,
    double threadPitch,
    bool reverseNormal
)
{
    gp_Vec toPoint(origin, pt);
    gp_Vec radialComp = toPoint - (toPoint.Dot(axis) * axis);
    // Pitch is axial, including on cones. Keep a signed coordinate anchored at
    // the resolved entrance so split faces and offset starts share one phase.
    const double axialDist = toPoint.Dot(axis);
    float vCoord = static_cast<float>(axialDist / threadPitch);
    double angleRad = std::atan2(radialComp.Dot(y_dir), radialComp.Dot(x_dir));
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

bool ViewProviderThread::generateBoreMeshData(
    const PartDesign::Thread* pcThread,
    const gp_Pnt& threadOriginPnt,
    std::vector<SbVec3f>& vertices,
    std::vector<SbVec3f>& normals,
    std::vector<int>& indices,
    std::vector<SbVec2f>& uvs
)
{
    if (DEBUG) Base::Console().message("[generateBoreMeshData]: Starting mesh generation...\n");

    const double threadPitch = pcThread->getThreadPitch();
    if (!std::isfinite(threadPitch) || threadPitch <= Precision::Confusion()) {
        if (DEBUG) Base::Console().warning("[generateBoreMeshData]: Thread pitch is 0.0 -> FALSE\n");
        return false;
    }

    vertices.clear();
    normals.clear();
    indices.clear();
    uvs.clear();

    const auto& boreFaces = collectBoreFaces(pcThread, threadOriginPnt);
    if (boreFaces.empty()) {
        if (DEBUG) Base::Console().warning("[generateBoreMeshData]: No bore faces collected for thread '%s' -> FALSE\n", pcThread->getNameInDocument());
        return false;
    }

    if (DEBUG) Base::Console().message("[generateBoreMeshData]: Collected %zu bore faces\n", boreFaces.size());

    auto threadNormalOpt = getThreadNormal(pcThread);
    if (!threadNormalOpt.has_value()) {
        if (DEBUG) Base::Console().warning("[generateBoreMeshData]: Failed to get thread normal axis -> FALSE\n");
        return false;
    }
    gp_Dir threadNormalAxis = *threadNormalOpt;

    const auto [x_dir, y_dir] = buildOrthonormalFrame(threadNormalAxis);

    bool success = false;
    size_t skippedFacesCount = 0;

    for (size_t i = 0; i < boreFaces.size(); ++i) {
        const auto& face = boreFaces[i];
        std::vector<gp_Pnt> meshPoints;
        std::vector<Poly_Triangle> meshFacets;
        if (!Part::Tools::getTriangulation(face, meshPoints, meshFacets)) {
            if (DEBUG) Base::Console().warning("[generateBoreMeshData]: Face index %zu skipped (triangulation failed)\n", i);
            skippedFacesCount++;
            continue;
        }

        const auto section = describeThreadSurface(face, threadOriginPnt, threadNormalAxis);
        if (!section) {
            skippedFacesCount++;
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
                x_dir,
                y_dir,
                section->slope,
                threadPitch,
                pcThread->IsInternal.getValue()
            ));
        }

        // --- Build indices ---
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

    if (!success) {
        if (DEBUG) Base::Console().warning("[generateBoreMeshData]: Failed to process any bore faces (Skipped %zu/%zu faces) -> FALSE\n", 
                                skippedFacesCount, boreFaces.size());
    } else {
        if (DEBUG) Base::Console().message("[generateBoreMeshData]: Mesh generated successfully (Verts: %zu, Norms: %zu, Inds: %zu, UVs: %zu)\n",
                                vertices.size(), normals.size(), indices.size(), uvs.size());
    }

    return success;
}

bool ViewProviderThread::isHoleThreadVisible() const
{
    if (DEBUG) Base::Console().message("[isHoleThreadVisible]: Checking thread visibility status...\n");

    auto* thread = getObject<PartDesign::Thread>();
    if (!thread) {
        if (DEBUG) Base::Console().warning("[isHoleThreadVisible]: Thread object is null -> FALSE\n");
        return false;
    }

    auto* body = PartDesign::Body::findBodyOf(thread);
    if (!body) {
        if (DEBUG) Base::Console().warning("[isHoleThreadVisible]: Parent Body not found for thread '%s' -> FALSE\n", thread->getNameInDocument());
        return false;
    }

    // --- Checagem de Visibilidade e Flags do Objeto ---
    bool isBodyVisible = body->Visibility.getValue();
    bool isSuppressed = thread->Suppressed.getValue();
    bool isCosmetic = thread->CosmeticThread.getValue();
    bool isModel = thread->ModelThread.getValue();

    if (DEBUG) Base::Console().message("[isHoleThreadVisible]: Conditions for '%s': BodyVis=%s, Suppressed=%s, Cosmetic=%s, Model=%s\n",
                            thread->getNameInDocument(),
                            isBodyVisible ? "TRUE" : "FALSE",
                            isSuppressed ? "TRUE" : "FALSE",
                            isCosmetic ? "TRUE" : "FALSE",
                            isModel ? "TRUE" : "FALSE");

    if (!isBodyVisible || isSuppressed || !isCosmetic || isModel) {
        if (DEBUG) Base::Console().message("[isHoleThreadVisible]: Initial condition check failed -> FALSE\n");
        return false;
    }

    // --- Verificação na árvore de recursos do Body ---
    const auto& features = body->Group.getValues();
    auto threadIt = std::ranges::find(features, thread);
    if (threadIt == features.end()) {
        if (DEBUG) Base::Console().warning("[isHoleThreadVisible]: Thread '%s' not found inside parent Body group -> FALSE\n", thread->getNameInDocument());
        return false;
    }

    if (DEBUG) Base::Console().message("[isHoleThreadVisible]: Checking posterior features visibility starting from thread '%s'...\n", thread->getNameInDocument());

    for (auto it = threadIt; it != features.end(); ++it) {
        auto* posteriorFeature = dynamic_cast<PartDesign::Feature*>(*it);
        if (posteriorFeature) {
            bool featureVis = posteriorFeature->Visibility.getValue();
            if (DEBUG) Base::Console().message("[isHoleThreadVisible]: Feature '%s' visibility = %s\n",
                                    posteriorFeature->getNameInDocument(),
                                    featureVis ? "TRUE" : "FALSE");

            if (featureVis) {
                if (DEBUG) Base::Console().message("[isHoleThreadVisible]: Visible posterior feature found ('%s') -> TRUE\n", posteriorFeature->getNameInDocument());
                return true;
            }
        }
    }

    // Chegou ao fim e nenhuma feature posterior está visível
    if (DEBUG) Base::Console().message("[isHoleThreadVisible]: Reached end of features list with no visible posterior feature -> FALSE\n");
    return false;
}

void ViewProviderThread::updateOverlay()
{
    if (DEBUG) Base::Console().message("[updateOverlay]: Starting overlay update\n");

    auto* thread = getObject<PartDesign::Thread>();
    if (!thread) {
        if (DEBUG) Base::Console().warning("[updateOverlay]: Thread object is null, aborting\n");
        return;
    }

    if (DEBUG) Base::Console().message("[updateOverlay]: Processing thread object '%s'\n", thread->getNameInDocument());

    bool isThreadVisible = isHoleThreadVisible();
    if (DEBUG) Base::Console().message("[updateOverlay]: Thread visibility status = %s\n", isThreadVisible ? "TRUE" : "FALSE");

    // Cleanup must not depend on resolving the Body again: during transaction rollback the
    // Thread can already be detached while its Coin node is still attached to the old root.
    clearThreadTextures();

    auto* bodyVp = getBodyViewProvider();
    if (!bodyVp) {
        if (DEBUG) Base::Console().warning("[updateOverlay]: Body view provider is null, aborting\n");
        return;
    }

    // --- Add the cosmetic thread overlay ---
    if (isThreadVisible) {
        if (DEBUG) Base::Console().message("[updateOverlay]: Generating new cosmetic thread texture separator... HEHE\n");
        if (SoSeparator* newSep = createThreadTextureSeparator()) {
            if (DEBUG) Base::Console().message("[updateOverlay]: Texture separator created successfully (ptr:)\n");
            
            auto* threadSwitch = new SoSwitch();
            threadSwitch->ref();
            threadSwitch->addChild(newSep);
            newSep->unref();

            if (bodyVp->getRoot()) {
                overlayRoot = bodyVp->getRoot();
                overlayRoot->addChild(threadSwitch);
                threadSwitch->whichChild = SO_SWITCH_ALL;
                m_threadOverlays[thread] = threadSwitch;
                if (DEBUG) Base::Console().message("[updateOverlay]: New cosmetic thread successfully attached to scene graph (SoSwitch ptr:)\n");
            } else {
                if (DEBUG) Base::Console().error("[updateOverlay]: Failed to attach SoSwitch — scene graph root is null\n");
                threadSwitch->unref();
            }
        } else {
            if (DEBUG) Base::Console().warning("[updateOverlay]: Failed to create texture separator (createThreadTextureSeparator returned null)\n");
        }
    } else {
        if (DEBUG) Base::Console().message("[updateOverlay]: Overlay skipped because thread is not visible\n");
    }

    if (DEBUG) Base::Console().message("[updateOverlay]: Overlay update finished\n");
}
