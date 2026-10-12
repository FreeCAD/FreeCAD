/******************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association <www.freecad.org>         *
 *                                                                            *
 *   This file is part of FreeCAD.                                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify it       *
 *   under the terms of the GNU Lesser General Public License as              *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU         *
 *   Lesser General Public License for more details.                          *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see                                  *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <FCConfig.h>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <Inventor/SbMatrix.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SoPath.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoClipPlaneElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoIndexedShape.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/system/gl.h>

#include "SoFCSectionCap.h"

using namespace Gui::Inventor;

SO_NODE_SOURCE(SoFCSectionCap)

namespace
{

/// FNV-1a, so default styles are the same on every run.
std::uint64_t digest(const std::string& key)
{
    std::uint64_t h = 14695981039346656037ULL;
    for (unsigned char c : key) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

const std::vector<SbColor>& palette()
{
    static const std::vector<SbColor> colors = {
        {0.90f, 0.37f, 0.26f},
        {0.25f, 0.55f, 0.85f},
        {0.36f, 0.72f, 0.36f},
        {0.93f, 0.69f, 0.13f},
        {0.58f, 0.40f, 0.74f},
        {0.20f, 0.70f, 0.70f},
        {0.85f, 0.45f, 0.65f},
        {0.60f, 0.60f, 0.22f},
        {0.62f, 0.38f, 0.22f},
        {0.47f, 0.57f, 0.95f},
        {0.75f, 0.20f, 0.27f},
        {0.45f, 0.47f, 0.52f},
    };
    return colors;
}

const std::vector<std::string>& hatches()
{
    // ordered so that neighbours differ the most
    static const std::vector<std::string> names = {
        "diag45",
        "horizontal",
        "dots",
        "diag135",
        "vertical",
        "diag45_dots",
        "diagcross",
        "horizontal_dots",
        "cross",
    };
    return names;
}

/// A 32x32 polygon stipple for a hatch name (screen space, 8 px spacing).
bool stipplePattern(const std::string& hatch, GLubyte out[128])
{
    const auto& names = hatches();
    if (std::find(names.begin(), names.end(), hatch) == names.end()) {
        return false;  // "none"
    }
    std::memset(out, 0, 128);
    constexpr int s = 8;
    constexpr int dot = 2;
    auto mod = [](int a, int m) {
        return ((a % m) + m) % m;
    };
    const bool horizontal = hatch == "horizontal" || hatch == "cross" || hatch == "horizontal_dots";
    const bool vertical = hatch == "vertical" || hatch == "cross";
    const bool diag45 = hatch == "diag45" || hatch == "diagcross" || hatch == "diag45_dots";
    const bool diag135 = hatch == "diag135" || hatch == "diagcross";
    const bool dots = hatch == "dots" || hatch == "horizontal_dots";
    const bool diagDots = hatch == "diag45_dots";
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            bool on = (horizontal && mod(y, s) == 0)
                || (vertical && mod(x, s) == 0)
                // stipple rows start at the bottom, so x - y = k rises like '/'
                || (diag45 && mod(x - y, s) == 0) || (diag135 && mod(x + y, s) == 0);
            if (dots) {
                on = on || (mod(x - s / 2, s) < dot && mod(y - s / 2, s) < dot);
            }
            if (diagDots) {  // on x - y = s/2, halfway between the '/' lines
                on = on || (mod(x - 3 * s / 4, s) < dot && mod(y - s / 4, s) < dot);
            }
            if (on) {
                out[y * 4 + x / 8] |= static_cast<GLubyte>(0x80 >> (x % 8));
            }
        }
    }
    return true;
}

SbVec3f boxCorner(const SbBox3f& box, int i)
{
    SbVec3f lo, hi;
    box.getBounds(lo, hi);
    return {(i & 1) ? hi[0] : lo[0], (i & 2) ? hi[1] : lo[1], (i & 4) ? hi[2] : lo[2]};
}

/// Screen rectangle of a world box; the whole viewport if the box reaches behind the camera.
void screenRect(const SbBox3f& box, const SbMatrix& viewProj, const SbViewportRegion& region, int rect[4])
{
    const SbVec2s origin = region.getViewportOriginPixels();
    const SbVec2s size = region.getViewportSizePixels();
    rect[0] = origin[0];
    rect[1] = origin[1];
    rect[2] = origin[0] + size[0];
    rect[3] = origin[1] + size[1];
    float minx = 1e30f, miny = 1e30f, maxx = -1e30f, maxy = -1e30f;
    for (int i = 0; i < 8; ++i) {
        const SbVec3f c = boxCorner(box, i);
        SbVec4f res;
        viewProj.multVecMatrix(SbVec4f(c[0], c[1], c[2], 1.0f), res);
        if (res[3] <= 1e-6f) {
            return;  // no divide: w <= 0 means behind the camera
        }
        const float px = origin[0] + (res[0] / res[3] * 0.5f + 0.5f) * size[0];
        const float py = origin[1] + (res[1] / res[3] * 0.5f + 0.5f) * size[1];
        minx = std::min(minx, px);
        maxx = std::max(maxx, px);
        miny = std::min(miny, py);
        maxy = std::max(maxy, py);
    }
    const int x0 = std::max<int>(origin[0], static_cast<int>(std::floor(minx)) - 2);
    const int y0 = std::max<int>(origin[1], static_cast<int>(std::floor(miny)) - 2);
    const int x1 = std::min<int>(origin[0] + size[0], static_cast<int>(std::ceil(maxx)) + 2);
    const int y1 = std::min<int>(origin[1] + size[1], static_cast<int>(std::ceil(maxy)) + 2);
    if (x1 > x0 && y1 > y0) {
        rect[0] = x0;
        rect[1] = y0;
        rect[2] = x1;
        rect[3] = y1;
    }
}

bool planeCutsBox(const SbPlane& plane, const SbBox3f& box)
{
    bool above = false, below = false;
    for (int i = 0; i < 8; ++i) {
        const float d = plane.getNormal().dot(boxCorner(box, i)) - plane.getDistanceFromOrigin();
        above = above || d > 0.0f;
        below = below || d < 0.0f;
    }
    return above && below;
}

/// The occurrence key of the current path; component is the object owning the geometry.
std::string keyForPath(
    const SoFullPath* path,
    const SoFCSectionCap::Resolver& resolve,
    std::string& component,
    bool& excluded
)
{
    // view providers along the path, with the child indices between them (not of switches)
    std::vector<std::string> pending;
    std::string key;
    excluded = false;
    for (int i = 1; i < path->getLength(); ++i) {
        SoNode* node = path->getNode(i);
        bool skip = false;
        std::string name = resolve ? resolve(node, skip) : std::string();
        if (!name.empty()) {
            // "Doc#Link\tDoc#Target": the component is the link target
            std::string target = name;
            const auto tab = name.find('\t');
            if (tab != std::string::npos) {
                target = name.substr(tab + 1);
                name = name.substr(0, tab);
            }
            if (!key.empty()) {
                for (const std::string& idx : pending) {
                    key += "/" + idx;
                }
                key += "/";
            }
            key += name;
            component = target;
            excluded = excluded || skip;
            pending.clear();
            continue;
        }
        if (!key.empty()) {
            SoNode* parent = path->getNode(i - 1);
            if (!parent->isOfType(SoSwitch::getClassTypeId())) {
                pending.push_back(std::to_string(path->getIndex(i)));
            }
        }
    }
    // trailing indices tell the elements of a link array apart
    for (const std::string& idx : pending) {
        key += "/" + idx;
    }
    if (key.empty()) {
        key = "(unresolved)";
        component = key;
    }
    return key;
}

}  // namespace

void SoFCSectionCap::initClass()
{
    SO_NODE_INIT_CLASS(SoFCSectionCap, SoNode, "Node");
}

SoFCSectionCap::SoFCSectionCap()
{
    SO_NODE_CONSTRUCTOR(SoFCSectionCap);
}

SoFCSectionCap::~SoFCSectionCap()
{
    setScene(nullptr);
}

void SoFCSectionCap::setScene(SoNode* node)
{
    if (node == scene) {
        return;
    }
    if (node) {
        node->ref();
    }
    if (scene) {
        scene->unref();
    }
    scene = node;
    proxyValid = false;
    touch();
}

void SoFCSectionCap::setResolver(Resolver r)
{
    resolve = std::move(r);
    proxyValid = false;
    touch();
}

void SoFCSectionCap::setStyle(bool perPart, bool hatching, const SbColor& color, bool shade)
{
    shaded = shade;
    colored = perPart && !shade;
    hatched = hatching;
    singleColor = color;
    allocated.clear();
    usedCombos.clear();
    touch();
}

SectionCapStyle SoFCSectionCap::styleFor(const Group& group)
{
    auto found = allocated.find(group.key);
    if (found != allocated.end()) {
        return found->second;
    }
    // Spread what varies (colors, else hatches) first, then unused (color, hatch)
    // pairs; both searches start at the key's digest, so the result is the same
    // on every run.
    const int nc = colored ? static_cast<int>(palette().size()) : 1;
    const int nh = hatched ? static_cast<int>(hatches().size()) : 1;
    const std::uint64_t h = digest(group.key);
    const int c0 = static_cast<int>(h % nc);
    const int t0 = static_cast<int>((h >> 32) % nh);
    auto colorUsed = [this](int c) {
        auto it = usedCombos.lower_bound({c, 0});
        return it != usedCombos.end() && it->first == c;
    };
    int chosen = c0 + t0 * nc;
    bool done = false;
    for (int k = 0; k < nc && !done; ++k) {
        const int c = (c0 + k) % nc;
        if (!colorUsed(c)) {
            chosen = c + t0 * nc;
            done = true;
        }
    }
    for (int k = 0; k < nc * nh && !done; ++k) {
        const int idx = (c0 + t0 * nc + k) % (nc * nh);
        if (!usedCombos.count({idx % nc, idx / nc})) {
            chosen = idx;
            done = true;
        }
    }
    usedCombos.insert({chosen % nc, chosen / nc});
    SectionCapStyle style;
    style.color = shaded ? group.material.diffuse : (colored ? palette()[chosen % nc] : singleColor);
    style.hatch = hatched ? hatches()[chosen / nc] : std::string("none");
    allocated[group.key] = style;
    return style;
}

void SoFCSectionCap::GLRender(SoGLRenderAction* action)
{
    SoState* state = action->getState();
    records.clear();

    if (!scene) {
        return;
    }
    const SoClipPlaneElement* elt = SoClipPlaneElement::getInstance(state);
    const int nplanes = elt ? elt->getNum() : 0;
    if (nplanes == 0) {
        status = "no-plane";
        return;
    }
    GLint stencilBits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    if (stencilBits < 1) {
        status = "no-stencil";  // the framebuffer has no stencil buffer: leave the cut open
        return;
    }
    updateProxy();
    if (groups.empty()) {
        status = "no-geometry";
        return;
    }

    const SbMatrix& view = SoViewingMatrixElement::get(state);
    SbMatrix viewProj = view;
    viewProj.multRight(SoProjectionMatrixElement::get(state));
    const SbViewportRegion& region = SoViewportRegionElement::get(state);
    const SbColor hatchColor = colored ? SbColor(0.15f, 0.15f, 0.15f) : singleColor * 0.5f;
    SbVec3f viewDir;
    view.inverse().multDirMatrix(SbVec3f(0, 0, -1), viewDir);

    // GL state is changed and drawn outside the scene graph's caching
    SoCacheElement::invalidate(state);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(view[0]);  // the proxy and the cap polygons are in world space

    // Upload every plane instead of trusting what the traversal left in the GL
    // clip plane units; with the view matrix loaded the world planes match the
    // world-space geometry, as in SoGLClipPlaneElement.
    for (int j = 0; j < nplanes; ++j) {
        const SbPlane plane = elt->get(j, TRUE);
        const SbVec3f normal = plane.getNormal();
        const GLdouble equation[4] = {
            normal[0],
            normal[1],
            normal[2],
            -plane.getDistanceFromOrigin(),
        };
        glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0 + j), equation);
    }

    bool drewCap = false;
    glClearStencil(0);
    glStencilMask(0x1);
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    GLubyte stipple[128];
    for (int p = 0; p < nplanes; ++p) {
        const SbPlane plane = elt->get(p, TRUE);
        const SbVec3f normal = plane.getNormal();
        SbVec3f u = std::fabs(normal[0]) < 0.9f ? SbVec3f(1, 0, 0) : SbVec3f(0, 1, 0);
        u = u - normal * normal.dot(u);
        u.normalize();
        SbVec3f v = normal.cross(u);
        v.normalize();
        // only plane p clips, or every plane but p
        auto clipBy = [&](bool onlyP) {
            for (int j = 0; j < nplanes; ++j) {
                if ((j == p) == onlyP) {
                    glEnable(static_cast<GLenum>(GL_CLIP_PLANE0 + j));
                }
                else {
                    glDisable(static_cast<GLenum>(GL_CLIP_PLANE0 + j));
                }
            }
        };

        for (Group& group : groups) {
            SectionCapRecord& rec = records.emplace_back();
            rec.key = group.key;
            rec.component = group.component;
            rec.plane = p;
            if (group.transparent) {
                rec.status = "unsupported: transparent";
                continue;
            }
            if (!group.closed) {
                rec.status = "unsupported: open mesh";
                continue;
            }
            if (!planeCutsBox(plane, group.box)) {
                rec.status = "skipped: not cut";
                continue;
            }
            rec.style = styleFor(group);

            int rect[4];
            screenRect(group.box, viewProj, region, rect);
            glEnable(GL_SCISSOR_TEST);
            glScissor(rect[0], rect[1], rect[2] - rect[0], rect[3] - rect[1]);

            // 1. mark
            clipBy(true);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glDepthMask(GL_FALSE);
            glDisable(GL_DEPTH_TEST);
            glStencilMask(0x1);
            glStencilFunc(GL_ALWAYS, 0, 0x1);
            glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
            glBegin(GL_TRIANGLES);
            for (std::size_t i = 0; i + 2 < group.tris.size(); i += 3) {
                glVertex3f(group.tris[i], group.tris[i + 1], group.tris[i + 2]);
            }
            glEnd();

            // 2. fill
            clipBy(false);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glStencilFunc(GL_EQUAL, 1, 0x1);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glEnable(GL_POLYGON_OFFSET_FILL);  // behind its outline
            glPolygonOffset(1.0f, 1.0f);

            SbVec3f center = group.box.getCenter();
            center = center - normal * (normal.dot(center) - plane.getDistanceFromOrigin());
            float dx {}, dy {}, dz {};
            group.box.getSize(dx, dy, dz);
            const float half = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz) * 1.05f + 1e-3f;
            auto quad = [&]() {
                glBegin(GL_QUADS);
                for (const SbVec3f& c : {u + v, -u + v, -u - v, u - v}) {
                    const SbVec3f corner = center + c * half;
                    glVertex3f(corner[0], corner[1], corner[2]);
                }
                glEnd();
            };
            if (shaded) {
                // lit like a face of the part lying in the plane, facing the viewer
                const SbVec3f n = normal.dot(viewDir) < 0.0f ? normal : -normal;
                // a step darker than the part's faces, so the cut still reads as one
                auto rgba = [](const SbColor& c) {
                    return std::array<GLfloat, 4> {0.8f * c[0], 0.8f * c[1], 0.8f * c[2], 1.0f};
                };
                glEnable(GL_LIGHTING);
                glDisable(GL_COLOR_MATERIAL);  // else the current color replaces the diffuse
                // n already faces the viewer; two-sided lighting would flip it for a
                // quad that happens to be wound clockwise on screen
                glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
                glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, rgba(group.material.ambient).data());
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, rgba(group.material.diffuse).data());
                glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, rgba(group.material.specular).data());
                glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, rgba(group.material.emission).data());
                glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, group.material.shininess * 128.0f);
                glNormal3f(n[0], n[1], n[2]);
                quad();
                glDisable(GL_LIGHTING);
            }
            else {
                glColor3f(rec.style.color[0], rec.style.color[1], rec.style.color[2]);
                quad();
            }
            if (stipplePattern(rec.style.hatch, stipple)) {
                const SbColor hatch = shaded ? SbColor(group.material.diffuse * 0.4f) : hatchColor;
                glEnable(GL_POLYGON_STIPPLE);
                glPolygonStipple(stipple);
                glColor3f(hatch[0], hatch[1], hatch[2]);
                quad();
                glDisable(GL_POLYGON_STIPPLE);
            }
            glDisable(GL_POLYGON_OFFSET_FILL);

            // outline: where the plane crosses the part's triangles
            glDisable(GL_STENCIL_TEST);
            glLineWidth(2.0f);
            glColor3f(0.1f, 0.1f, 0.1f);
            const std::vector<float>& outline = outlineOf(group, plane);
            glBegin(GL_LINES);
            for (std::size_t i = 0; i + 2 < outline.size(); i += 3) {
                glVertex3f(outline[i], outline[i + 1], outline[i + 2]);
            }
            glEnd();
            glEnable(GL_STENCIL_TEST);

            // 3. clear the bit inside this group's rectangle
            glStencilMask(0x1);
            glClear(GL_STENCIL_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);

            rec.status = "capped";
            drewCap = true;
        }
    }

    glPopMatrix();
    glPopAttrib();
    status = drewCap ? "capped" : "no-cap";
}

std::uint64_t SoFCSectionCap::geometrySignature(SoNode* root)
{
    // Highlighting touches indexed shapes (and so every group above them) on
    // each hover; use leaf node ids and group structure instead.
    std::uint64_t h = 0;
    auto mix = [&h](std::uint64_t v) {
        h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    };
    std::vector<SoNode*> stack {root};
    while (!stack.empty()) {
        SoNode* node = stack.back();
        stack.pop_back();
        mix(reinterpret_cast<std::uintptr_t>(node));
        if (node->isOfType(SoPointSet::getClassTypeId())
            || node->isOfType(SoLineSet::getClassTypeId())
            || node->isOfType(SoMarkerSet::getClassTypeId())) {
            continue;  // no triangles; highlighted vertices / edges are touched on hover
        }
        if (node->isOfType(SoIndexedShape::getClassTypeId())) {
            mix(static_cast<std::uint64_t>(static_cast<SoIndexedShape*>(node)->coordIndex.getNum()));
            continue;
        }
        SoChildList* children = node->getChildren();
        if (!children) {
            mix(node->getNodeId());  // a leaf: coordinates, transform, material, ...
            continue;
        }
        mix(static_cast<std::uint64_t>(children->getLength()));
        if (node->isOfType(SoSwitch::getClassTypeId())) {
            mix(static_cast<std::uint64_t>(static_cast<SoSwitch*>(node)->whichChild.getValue()));
        }
        for (int i = children->getLength() - 1; i >= 0; --i) {
            stack.push_back((*children)[i]);
        }
    }
    return h;
}

void SoFCSectionCap::updateProxy()
{
    // nothing below the scene changed
    const std::uint32_t sceneId = scene->getNodeId();
    if (proxyValid && sceneId == proxySceneId) {
        return;
    }
    proxySceneId = sceneId;
    const std::uint64_t signature = geometrySignature(scene);
    if (proxyValid && signature == proxySignature) {
        return;
    }
    groups.clear();
    groupIndex.clear();
    lastShape = nullptr;
    SoCallbackAction collect;
    collect.addPreCallback(
        SoShape::getClassTypeId(),
        [](void* self, SoCallbackAction* action, const SoNode* node) -> SoCallbackAction::Response {
            auto* cap = static_cast<SoFCSectionCap*>(self);
            std::string component;
            bool excluded = false;
            const std::string key = keyForPath(
                static_cast<const SoFullPath*>(action->getCurPath()),
                cap->resolve,
                component,
                excluded
            );
            if (excluded) {
                cap->lastShape = nullptr;
                return SoCallbackAction::PRUNE;
            }
            auto it = cap->groupIndex.find(key);
            if (it == cap->groupIndex.end()) {
                Group group;
                group.key = key;
                group.component = component;
                cap->groups.push_back(group);
                it = cap->groupIndex.emplace(key, cap->groups.size() - 1).first;
            }
            cap->lastGroup = it->second;
            cap->lastShape = node;
            SbColor ambient, diffuse, specular, emission;
            float shininess = 0.0f, transparency = 0.0f;
            action->getMaterial(ambient, diffuse, specular, emission, shininess, transparency, 0);
            if (transparency > 0.01f) {
                cap->groups[it->second].transparent = true;
            }
            return SoCallbackAction::CONTINUE;
        },
        this
    );
    collect.addTriangleCallback(
        SoShape::getClassTypeId(),
        [](void* self,
           SoCallbackAction* action,
           const SoPrimitiveVertex* v1,
           const SoPrimitiveVertex* v2,
           const SoPrimitiveVertex* v3) {
            auto* cap = static_cast<SoFCSectionCap*>(self);
            if (!cap->lastShape) {
                return;
            }
            Group& group = cap->groups[cap->lastGroup];
            // the material of most triangles (faces, not edges, which use the line color)
            Material m;
            float transparency = 0.0f;
            action->getMaterial(
                m.ambient,
                m.diffuse,
                m.specular,
                m.emission,
                m.shininess,
                transparency,
                v1->getMaterialIndex()
            );
            auto& vote = group.materials[m.diffuse.getPackedValue()];
            if (vote.second++ == 0) {
                vote.first = m;
            }
            const SbMatrix& model = action->getModelMatrix();
            for (const SoPrimitiveVertex* v : {v1, v2, v3}) {
                SbVec3f world;
                model.multVecMatrix(v->getPoint(), world);
                group.tris.push_back(world[0]);
                group.tris.push_back(world[1]);
                group.tris.push_back(world[2]);
                group.box.extendBy(world);
            }
        },
        this
    );
    collect.apply(scene);
    for (Group& group : groups) {
        int most = 0;
        for (const auto& [color, vote] : group.materials) {
            if (vote.second > most) {
                most = vote.second;
                group.material = vote.first;
            }
        }
        group.materials.clear();
    }
    // drop groups with lines and points only
    groups.erase(
        std::remove_if(groups.begin(), groups.end(), [](const Group& g) { return g.tris.empty(); }),
        groups.end()
    );
    groupIndex.clear();
    decltype(closedCache) closed;
    for (Group& group : groups) {
        // the closed check is costly; reuse it while the triangles are unchanged
        std::uint64_t digestTris = 14695981039346656037ULL;
        for (float f : group.tris) {
            std::uint32_t bits = 0;
            std::memcpy(&bits, &f, sizeof(bits));
            digestTris = (digestTris ^ bits) * 1099511628211ULL;
        }
        auto cached = closedCache.find(group.key);
        ClosedInfo info;
        if (cached != closedCache.end() && cached->second.digest == digestTris) {
            info = std::move(cached->second);
        }
        else {
            info.digest = digestTris;
            checkClosed(group, info.patch);
            info.closed = group.closed;
        }
        group.closed = info.closed;
        group.tris.insert(group.tris.end(), info.patch.begin(), info.patch.end());
        closed[group.key] = std::move(info);
    }
    closedCache.swap(closed);
    proxySignature = signature;
    proxyValid = true;
}

const std::vector<float>& SoFCSectionCap::outlineOf(Group& group, const SbPlane& plane)
{
    // cached per plane: it only changes while the plane is moved
    const SbVec3f n = plane.getNormal();
    const float dist = plane.getDistanceFromOrigin();
    for (const auto& [key, segments] : group.outlines) {
        if (key[0] == n[0] && key[1] == n[1] && key[2] == n[2] && key[3] == dist) {
            return segments;
        }
    }
    if (group.outlines.size() > 8) {
        group.outlines.clear();
    }
    auto& [key, segments] = group.outlines.emplace_back();
    key = {n[0], n[1], n[2], dist};
    // where the plane crosses each triangle; a half-open sign test gives zero or two points
    for (std::size_t i = 0; i + 8 < group.tris.size(); i += 9) {
        const float* t = &group.tris[i];
        float d[3];
        for (int k = 0; k < 3; ++k) {
            d[k] = n[0] * t[3 * k] + n[1] * t[3 * k + 1] + n[2] * t[3 * k + 2] - dist;
        }
        for (int k = 0; k < 3; ++k) {
            const int j = (k + 1) % 3;
            if ((d[k] > 0.0f) != (d[j] > 0.0f)) {
                const float f = d[k] / (d[k] - d[j]);
                for (int c = 0; c < 3; ++c) {
                    segments.push_back(t[3 * k + c] + f * (t[3 * j + c] - t[3 * k + c]));
                }
            }
        }
    }
    return segments;
}

void SoFCSectionCap::checkClosed(Group& group, std::vector<float>& patch)
{
    // closed: (nearly) every edge is used an even number of times, vertices welded by position
    std::map<std::tuple<float, float, float>, int> ids;
    std::vector<std::size_t> first;  // per id: offset of the vertex in tris
    auto id = [&](std::size_t i) {
        auto key = std::make_tuple(group.tris[i], group.tris[i + 1], group.tris[i + 2]);
        auto it = ids.find(key);
        if (it == ids.end()) {
            it = ids.emplace(key, static_cast<int>(ids.size())).first;
            first.push_back(i);
        }
        return it->second;
    };
    std::map<std::pair<int, int>, int> edges;
    for (std::size_t t = 0; t + 8 < group.tris.size(); t += 9) {
        const int a = id(t), b = id(t + 3), c = id(t + 6);
        for (auto e : {std::make_pair(a, b), std::make_pair(b, c), std::make_pair(c, a)}) {
            if (e.first == e.second) {
                continue;  // degenerate triangle
            }
            if (e.first > e.second) {
                std::swap(e.first, e.second);
            }
            ++edges[e];
        }
    }
    // tessellation can leave a few cracks where faces meet; an open surface has far more
    std::vector<std::pair<int, int>> odd;
    for (const auto& kv : edges) {
        if (kv.second % 2) {
            odd.push_back(kv.first);
        }
    }
    group.closed = !edges.empty() && odd.size() * 100 <= edges.size();
    if (!group.closed || odd.empty()) {
        return;
    }
    // A ray through a crack misses a crossing and flips the stencil parity, which
    // shows as slivers. Fan every connected set of unpaired edges from one of its
    // vertices: every vertex has an even number of them, so this closes the mesh.
    std::unordered_map<int, int> parent;
    auto find = [&parent](int v) {
        int root = parent.emplace(v, v).first->second;
        while (parent[root] != root) {
            root = parent[root];
        }
        while (v != root) {
            int& up = parent[v];
            v = std::exchange(up, root);
        }
        return root;
    };
    for (const auto& e : odd) {
        parent[find(e.first)] = find(e.second);
    }
    auto push = [&](int v) {
        patch.insert(patch.end(), group.tris.begin() + first[v], group.tris.begin() + first[v] + 3);
    };
    for (const auto& e : odd) {
        const int apex = find(e.first);
        if (apex != e.first && apex != e.second) {
            push(apex);
            push(e.first);
            push(e.second);
        }
    }
}
