// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <Inventor/SbVec2f.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/nodes/SoSeparator.h>
#include <FCGlobal.h>

class SoAction;
class SoGLRenderAction;
class SoGetBoundingBoxAction;
class SoRayPickAction;
class SoState;

namespace Gui
{
namespace Inventor
{

/**
 * Retained subtree wrapper for screen-space rendering.
 *
 * FreeCAD has several overlay-like nodes whose children are authored in a
 * screen-aligned coordinate system instead of world space. Historically each
 * node applied its own ad hoc render-state setup inside custom GLRender()
 * implementations. That made the logic hard to share, and it also meant other
 * traversal paths could not reuse the same retained child subtree without more
 * node-specific glue.
 *
 * SoFCScreenSpaceGroup centralizes that common behavior. It traverses a normal
 * retained child subtree, but temporarily replaces the model, view, projection,
 * depth-buffer, texture and light-model state so the children render in a
 * screen-space coordinate system.
 *
 * The node is intentionally a SoSeparator, not a SoShape. The children provide
 * the actual geometry while this wrapper only defines how that geometry should
 * be interpreted during rendering.
 *
 * The wrapper also opts out of world-space bbox contribution and ray picking.
 * Screen-space overlays should not affect viewAll(), scene bounds, or normal
 * 3D picking unless a derived node adds explicit behavior on top.
 */
class GuiExport SoFCScreenSpaceGroup: public SoSeparator
{
    using inherited = SoSeparator;

    SO_NODE_HEADER(SoFCScreenSpaceGroup);

public:
    /**
     * Coordinate system expected from child geometry.
     */
    enum class CoordinateSpace
    {
        /**
         * Child vertices are authored directly in clip-space coordinates.
         *
         * The wrapper keeps an identity projection matrix so geometry reaches
         * the rasterizer unchanged. A matching orthographic SoViewVolume is
         * still installed for Coin subsystems that consult the logical view
         * volume during traversal.
         */
        ClipSpace,

        /**
         * Child vertices are authored in a normalized screen quad covering the
         * range [-1, 1] in X and Y.
         *
         * This is convenient for retained full-screen quads such as background
         * gradients, where the subtree should not need to know the viewport
         * pixel size.
         */
        Normalized,

        /**
         * Child vertices are authored in viewport pixel coordinates.
         *
         * The resulting orthographic projection uses the active viewport size
         * in pixels, with the origin at the lower-left corner to match Coin's
         * usual OpenGL-facing convention.
         */
        ViewportPixels
    };

    static void initClass();
    SoFCScreenSpaceGroup();

    /**
     * Select which coordinate system child geometry uses.
     */
    void setCoordinateSpace(CoordinateSpace coordinateSpace);

    /**
     * Return the coordinate system currently expected from child geometry.
     */
    [[nodiscard]] CoordinateSpace getCoordinateSpace() const;

    /**
     * Force the BASE_COLOR light model while traversing children.
     *
     * Overlay geometry often wants stable authored colors that do not depend on
     * viewer light setup. This flag lets wrappers opt into that behavior
     * without each node reapplying the same SoLazyElement setup.
     */
    void setBaseColorLightModel(bool enable);

    /**
     * Enable or disable texturing for the wrapped child traversal.
     */
    void setTexturesEnabled(bool enable);

    /**
     * Enable or disable multitexturing for the wrapped child traversal.
     */
    void setMultiTexturesEnabled(bool enable);

    /**
     * Configure temporary depth-buffer state for child traversal.
     *
     * This is primarily for overlays that need to choose whether they ignore
     * scene depth entirely, test against it, or write their own depth values.
     * The state is applied only while traversing this node's children.
     */
    void setDepthBuffer(
        bool test,
        bool write,
        SoDepthBufferElement::DepthWriteFunction function = SoDepthBufferElement::ALWAYS,
        const SbVec2f& range = SbVec2f(0.0f, 1.0f)
    );

    void doAction(SoAction* action) override;
    void GLRenderBelowPath(SoGLRenderAction* action) override;
    void GLRenderInPath(SoGLRenderAction* action) override;
    void GLRenderOffPath(SoGLRenderAction* action) override;

    /**
     * Screen-space wrappers do not contribute a world-space bounding box.
     */
    void getBoundingBox(SoGetBoundingBoxAction* action) override;

    /**
     * Screen-space wrappers are not directly pickable by default.
     */
    void rayPick(SoRayPickAction* action) override;

protected:
    ~SoFCScreenSpaceGroup() override;

    /**
     * Apply the configured screen-space traversal state to \a state.
     *
     * Derived classes normally do not need to call this directly because the
     * wrapper already handles the common GLRender entry points. It remains
     * protected so specialized subclasses can build additional rendering
     * behavior on top of the shared state setup.
     */
    void applyScreenSpaceState(SoState* state);

private:
    CoordinateSpace coordinateSpace {CoordinateSpace::Normalized};
    bool useBaseColorLightModel {true};
    bool texturesEnabled {false};
    bool multiTexturesEnabled {false};
    bool depthTestEnabled {false};
    bool depthWriteEnabled {false};
    SoDepthBufferElement::DepthWriteFunction depthFunction {SoDepthBufferElement::ALWAYS};
    SbVec2f depthRange {0.0f, 1.0f};
};

}  // namespace Inventor
}  // namespace Gui
