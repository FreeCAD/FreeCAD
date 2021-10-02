/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
#  include <OpenGL/gl.h>
# else
#  include <GL/gl.h>
# endif
# include <float.h>
# include <algorithm>
# include <Python.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/SoPrimitiveVertex.h>
# include <Inventor/actions/SoCallbackAction.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGetPrimitiveCountAction.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/actions/SoPickAction.h>
# include <Inventor/actions/SoWriteAction.h>
# include <Inventor/bundles/SoMaterialBundle.h>
# include <Inventor/bundles/SoTextureCoordinateBundle.h>
# include <Inventor/elements/SoOverrideElement.h>
# include <Inventor/elements/SoCoordinateElement.h>
# include <Inventor/elements/SoGLCoordinateElement.h>
# include <Inventor/elements/SoGLCacheContextElement.h>
# include <Inventor/elements/SoPointSizeElement.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/errors/SoReadError.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/misc/SoState.h>
#endif

#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/caches/SoBoundingBoxCache.h>

#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "PartParams.h"
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCSelectionAction.h>
#include <Gui/ViewParams.h>

using namespace Gui;
using namespace PartGui;

////////////////////////////////////////////////////////////////////////////

SO_NODE_SOURCE(SoBrepEdgeSet)

void SoBrepEdgeSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepEdgeSet, SoIndexedLineSet, "IndexedLineSet");
}

SoBrepEdgeSet::SoBrepEdgeSet()
    : selContext(std::make_shared<SelContext>())
    , selContext2(std::make_shared<SelContext>())
{
    SO_NODE_CONSTRUCTOR(SoBrepEdgeSet);
    SO_NODE_ADD_FIELD(highlightIndices, (-1));
    SO_NODE_ADD_FIELD(highlightColor, (0,0,0));
    highlightIndices.setNum(0);
    SO_NODE_ADD_FIELD(seamIndices, (-1));
    seamIndices.setNum(0);
    SO_NODE_ADD_FIELD(elementSelectable, (TRUE));
    SO_NODE_ADD_FIELD(onTopPattern, (TRUE));
}

void SoBrepEdgeSet::notify(SoNotList * list)
{
    SoField *f = list->getLastField();
    if (f == &this->coordIndex) {
        const int32_t* cindices = this->coordIndex.getValues(0);
        int numcindices = this->coordIndex.getNum();
        this->segments.clear();
        for(int i=0;i<numcindices;i++) {
            if(cindices[i] < 0)
                this->segments.push_back(i+1);
        }
        bboxPicker.clear();
        bboxMap.clear();
    }
    SoIndexedLineSet::notify(list);
}

bool SoBrepEdgeSet::isSelected(SelContextPtr ctx) {
    if(ctx) 
        return ctx->isSelected();
    for(auto node : siblings) {
        auto sctx = Gui::SoFCSelectionRoot::getRenderContext<Gui::SoFCSelectionContext>(node);
        if(sctx && sctx->isSelected())
            return true;
    }
    return false;
}

void SoBrepEdgeSet::setSiblings(std::vector<SoNode*> &&s) {
    // No need to ref() here, because we only use the pointer as keys to lookup
    // selection context
    siblings = std::move(s);
}

void SoBrepEdgeSet::GLRender(SoGLRenderAction *action)
{
    glRender(action, false);
}

void SoBrepEdgeSet::GLRenderInPath(SoGLRenderAction *action)
{
    glRender(action, true);
}

void SoBrepEdgeSet::glRender(SoGLRenderAction *action, bool inpath)
{
    auto state = action->getState();

    bool delayrendering = action->isRenderingDelayedPaths();
    if (!inpath && !delayrendering) {
        ///////////////////////////////////////////////////////////////////////////////////////////////
        // Copied from SoShape::shouldGLRender(). Put here for early render skipping
        const SoShapeStyleElement * shapestyle = SoShapeStyleElement::get(state);
        unsigned int shapestyleflags = shapestyle->getFlags();
        if ((shapestyleflags & SoShapeStyleElement::INVISIBLE)
                || (shapestyleflags & SoShapeStyleElement::SHADOWMAP))
            return;
        if (getBoundingBoxCache() && !state->isCacheOpen() && !SoCullElement::completelyInside(state)) {
            if (getBoundingBoxCache()->isValid(state)) {
                if (SoCullElement::cullTest(state, getBoundingBoxCache()->getProjectedBox())) {
                    return;
                }
            }
        }
        //////////////////////////////////////////////////////////////////////////////////////////////

        selCounter.checkCache(state);
    }


    SelContextPtr ctx2;
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext<SelContext>(this,selContext,ctx2);
    if(ctx2 && !ctx2->isSelected())
        return;

    if(selContext2->checkGlobal(ctx)) {
        SoCacheElement::invalidate(state);
        ctx = selContext2;
    }

    Gui::FCDepthFunc depthGuard;
    if(!inpath && !delayrendering) {
        if (ctx && ((!Gui::ViewParams::getShowSelectionOnTop() && ctx->isSelected())
                    || ctx->isHighlighted())) {
            if (ctx->isHighlightAll() || ctx->isSelectAll()) {
                action->addDelayedPath(action->getCurPath()->copy());
                return;
            } else if (!action->isOfType(SoBoxSelectionRenderAction::getClassTypeId()))
                action->addDelayedPath(action->getCurPath()->copy());
        }
        depthGuard.set(GL_LEQUAL);
    } else if (inpath && delayrendering)
        depthGuard.set(GL_LEQUAL);
    else
        inpath = false;

    if(ctx && ctx->isHighlightAll()
           && (!highlightIndices.getNum()
               || (ctx2 && !ctx2->isSelectAll())))
    {
        if(ctx2 && !ctx2->isSelectAll()) {
            ctx2->selectionColor = ctx->highlightColor;
            renderSelection(action,ctx2); 
        } else
            renderHighlight(action,ctx);
        return;
    }

    int pass = 2;

    if((!ctx2 || !ctx2->isSelectAll())
       && Gui::ViewParams::getShowSelectionOnTop()
       && (!ctx || !ctx->isSelectAll()
                || !Gui::ViewParams::highlightIndicesOnFullSelect()
                || highlightIndices.getNum())
       && !Gui::SoFCUnifiedSelection::getShowSelectionBoundingBox()) 
    {
        // If we are rendering on top, we shall perform a two pass rendering.
        // The first pass keep depth test disabled (default in on top
        // rendering), and default transparency override, with an optional
        // selection line pattern (default 0xff00). This pass is for rendering
        // hidden lines.
        //
        // The second pass renables depth test, and set depth function to
        // GL_LEQUAL to render the outline.
        if(delayrendering) {
            if (!inpath)
                pass = 0;
        } else if (isSelected(ctx)) {
            // If we are selected but not rendering inside the group on top.
            // Just skip the rendering
            return;
        }
    }

    SoColorPacker packer;
    float trans = 0.0;
    float width = 0.0;
    float oldWidth = 0.0f;

    for(;pass<=2;++pass) {
        state->push();
        if(pass==0) {
            int pattern = Gui::ViewParams::getSelectionLinePattern();
            if(pattern) {
                SoLinePatternElement::set(state, pattern);
                if (Gui::ViewParams::getSelectionLinePatternScale() > 1)
                    glLineStipple((GLint) (Gui::ViewParams::getSelectionLinePatternScale()),
                                (GLushort) (pattern & 0xffff));
            }
        } else if(pass==1) {
            depthGuard.set(GL_LEQUAL);
            if(!SoFCSwitch::testTraverseState(SoFCSwitch::TraverseInvisible)) {
                // If we are visible, disable transparency to get a solid
                // outline, or else on top rendering will have some default
                // transparency, which will give a fainted appearance that is
                // ideal of drawing hidden line, or indicating we are invisible
                // (but forced to shown by on top rendering)
                SoLazyElement::setTransparency(state,this,1,&trans,&packer);
            }
            width = oldWidth = SoLineWidthElement::get(state);
            if(width < 1.0)
                width = 1.0;
            if (SoFCDisplayModeElement::showHiddenLines(state))
                width = std::max(width, (float)Gui::ViewParams::getSelectionHiddenLineWidth());
            pass = 2;
        }

        if(ctx && ctx->isSelected()) {
            if(!(Gui::ViewParams::highlightIndicesOnFullSelect()
                        && highlightIndices.getNum())
                    && ctx->isSelectAll()
                    && ctx->hasSelectionColor())
            {
                if(ctx2 && !ctx2->isSelectAll()) {
                    ctx2->selectionColor = ctx->selectionColor;
                    renderSelection(action,ctx2); 
                }else if(ctx->isSelectAll())
                    renderSelection(action,ctx); 
                renderHighlight(action,ctx);

                state->pop();
                continue;
            }
        }

        if (width != oldWidth)
            SoLineWidthElement::set(state,width);

        if(!inpath && ctx2 && ctx2->isSelected())
            renderSelection(action,ctx2,false);
        else if (!inpath) {
            uint32_t color;
            SoColorPacker packer;
            float trans = 0.0;

            // SoFCDisplayModeElement::getTransparency() specifies face only
            // transparency. When there is a face only transparency, we'll make
            // edge/point rendering to be opque. Maybe we'll add support for
            // edge/point transparency in SoFCDisplayModeElement later.

            if(SoFCDisplayModeElement::getTransparency(state) == 0.0f) {
                // Work around Coin bug of losing per line/point color when
                // rendering with transparency type SORTED_OBJECT_SORTED_TRIANGLE_BLEND
                SoShapeStyleElement::setTransparencyType(state,SoGLRenderAction::SORTED_OBJECT_BLEND);

                inherited::GLRender(action);
            } else {
                state->push();
                if (pass == 0 && !SoFCDisplayModeElement::showHiddenLines(state)) {
                    trans = SoFCDisplayModeElement::getTransparency(state);
                } else
                    trans = 0.0f;
                SoLazyElement::setTransparency(state,this,1,&trans,&packer);
                SoLightModelElement::set(state,SoLightModelElement::BASE_COLOR);
                auto lineColor = SoFCDisplayModeElement::getLineColor(state);
                if(lineColor) {
                    color = lineColor->getPackedValue(0.0);
                    SoMaterialBindingElement::set(state,SoMaterialBindingElement::OVERALL);
                    SoLazyElement::setPacked(state, this, 1, &color, false);
                }
                inherited::GLRender(action);
                state->pop();
            }
        }

        if (width != oldWidth)
            SoLineWidthElement::set(state,oldWidth);

        if(ctx && ctx->isSelected() && ctx->hasSelectionColor())
            renderSelection(action,ctx);
        renderHighlight(action,ctx);

        state->pop();
    }
}

void SoBrepEdgeSet::GLRenderBelowPath(SoGLRenderAction * action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepEdgeSet::getBoundingBox(SoGetBoundingBoxAction * action) {

    auto state = action->getState();
    selCounter.checkCache(state,true);

    SelContextPtr ctx2 = Gui::SoFCSelectionRoot::getSecondaryActionContext<SelContext>(action,this);
    if(!ctx2 || ctx2->isSelectAll()) {
        inherited::getBoundingBox(action);
        return;
    }

    if(!ctx2->isSelected())
        return;

    auto coords = SoCoordinateElement::getInstance(state);
    const SbVec3f *coords3d = coords->getArrayPtr3();
    int numcoords = coords->getNum();

    const int32_t* indices = this->coordIndex.getValues(0);
    int numindices = this->coordIndex.getNum();

    SbBox3f bbox;

    for(auto &v : ctx2->selectionIndex) {
        int idx = v.first;
        if(idx < 0 || idx >= (int)segments.size())
            break;
        int offset = idx==0 ? 0 : segments[idx-1];
        int num = segments[idx];
        if(num > numindices)
            break;
        int32_t i;
        const int32_t *cindices = indices + offset;
        const int32_t *end = indices + num;
        while (cindices < end) {
            i = *cindices++;
            if (i >= 0 && i < numcoords)
                bbox.extendBy(coords3d[i]);
        }
    }

    if(!bbox.isEmpty())
        action->extendBy(bbox);
}

static inline void renderLines(const SoGLCoordinateElement * const coords,
                               const int32_t *cindices, int numindices)
{

    const SbVec3f * coords3d = coords->getArrayPtr3();
    int num = coords->getNum();

    int32_t i;
    int previ;
    const int32_t *end = cindices + numindices;
    while (cindices < end) {
        previ = *cindices++;
        if(previ < 0 || previ >= num)
            continue;
        i = (cindices < end) ? *cindices++ : -1;
        glBegin(GL_LINE_STRIP);
        while (i >= 0) {
            if(i >= num)
                break;
            glVertex3fv((const GLfloat*) (coords3d + previ));
            glVertex3fv((const GLfloat*) (coords3d + i));
            previ = i;
            i = cindices < end ? *cindices++ : -1;
        }
        glEnd();
    }
}

static FC_COIN_THREAD_LOCAL std::vector<int> RenderIndices;

void SoBrepEdgeSet::renderHighlight(SoGLRenderAction *action, SelContextPtr ctx)
{
    if(!ctx || !ctx->isHighlighted())
        return;

    if (!ctx->isHighlightAll()
            && action->isOfType(SoBoxSelectionRenderAction::getClassTypeId())
            && static_cast<SoBoxSelectionRenderAction*>(
                action)->addLateDelayedPath(action->getCurPath(), true))
        return;

    Gui::FCDepthFunc depthGuard;
    if (action->isRenderingDelayedPaths())
        depthGuard.set(GL_ALWAYS);

    SbColor color = ctx->highlightColor;
    bool checkColor = true;

    RenderIndices.clear();
    if(ctx->isHighlightAll()) {
        if(highlightIndices.getNum()) {
            if(highlightColor.getValue().getPackedValue(1.0f)) {
                color = highlightColor.getValue();
                checkColor = false;
            }
            auto indices = highlightIndices.getValues(0);
            RenderIndices.insert(RenderIndices.end(), indices, indices + highlightIndices.getNum());
        }
    } else
        RenderIndices.insert(RenderIndices.end(), ctx->highlightIndex.begin(), ctx->highlightIndex.end());

    _renderSelection(action, checkColor, color, 0xFFFF, true);
}

void SoBrepEdgeSet::renderSelection(SoGLRenderAction *action, SelContextPtr ctx, bool push)
{
    if(!ctx || !ctx->isSelected())
        return;

    if (!ctx->isSelectAll()
            && action->isOfType(SoBoxSelectionRenderAction::getClassTypeId())
            && static_cast<SoBoxSelectionRenderAction*>(
                action)->addLateDelayedPath(action->getCurPath(), true))
        return;

    Gui::FCDepthFunc depthGuard;
    if (action->isRenderingDelayedPaths())
        depthGuard.set(GL_ALWAYS);

    bool checkColor = true;
    SbColor color = ctx->selectionColor;
    RenderIndices.clear();
    if(!ctx->isSelectAll()) {
        for(auto &v : ctx->selectionIndex)
            RenderIndices.push_back(v.first);
    } else if(Gui::ViewParams::highlightIndicesOnFullSelect()
                && highlightIndices.getNum())
    {
        if(highlightColor.getValue().getPackedValue(1.0f)) {
            checkColor = false;
            color = highlightColor.getValue();
        }
        auto indices = highlightIndices.getValues(0);
        RenderIndices.insert(RenderIndices.end(), indices, indices + highlightIndices.getNum());
    }
    _renderSelection(action, checkColor, color, 0, push);
}

void SoBrepEdgeSet::_renderSelection(SoGLRenderAction *action, 
        bool checkColor, SbColor _color, unsigned pattern, bool push)
{
    SoState * state = action->getState();
    uint32_t color;
    if(push){
        state->push();
        if(checkColor && !RenderIndices.empty()) {
            int idx = -1;
            if(SoMaterialBindingElement::get(state) == SoMaterialBindingElement::OVERALL)
                idx = 0;
            else if (RenderIndices.size() == 1 && RenderIndices[0] < materialIndex.getNum())
                idx = materialIndex[RenderIndices[0]];
            if(idx >= 0 && idx < SoLazyElement::getInstance(state)->getNumDiffuse())
                SoBrepFaceSet::makeDistinctColor(_color, _color, SoLazyElement::getDiffuse(state, idx));
        }
        color = _color.getPackedValue(0.0);
        Gui::SoFCSelectionRoot::setupSelectionLineRendering(
                state,this,&color,!RenderIndices.empty());
        if(pattern) {
            SoLinePatternElement::set(state, this, pattern);
            if (Gui::ViewParams::getSelectionLinePatternScale() > 1)
                glLineStipple((GLint) (Gui::ViewParams::getSelectionLinePatternScale()),
                              (GLushort) (pattern & 0xffff));
        }
    }

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numcindices;
    const int32_t * nindices;
    const int32_t * tindices;
    const int32_t * mindices;
    SbBool normalCacheUsed;

    this->getVertexData(state, coords, normals, cindices, nindices,
        tindices, mindices, numcindices, false, normalCacheUsed);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    if(RenderIndices.empty())
        renderLines(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
    else {
        for(int idx : RenderIndices) {
            if (idx < 0 || idx >= (int)segments.size() || segments[idx] > numcindices)
                break;

            int offset = idx==0 ? 0 : segments[idx-1];
            int num = segments[idx] - offset;
            renderLines(static_cast<const SoGLCoordinateElement*>(coords), cindices + offset, num);
        }
    }
    if (normalCacheUsed)
        this->readUnlockNormalCache();
    if(push) state->pop();
}

void SoBrepEdgeSet::doAction(SoAction* action)
{
    if (Gui::SoFCSelectionRoot::handleSelectionAction(
                action, this, SoFCDetail::Edge, selContext, selCounter))
        return;

    inherited::doAction(action);
}

SoDetail * SoBrepEdgeSet::createLineSegmentDetail(SoRayPickAction * action,
                                                  const SoPrimitiveVertex * v1,
                                                  const SoPrimitiveVertex * v2,
                                                  SoPickedPoint * pp)
{
    SoDetail* detail = inherited::createLineSegmentDetail(action, v1, v2, pp);
    SoLineDetail* line_detail = static_cast<SoLineDetail*>(detail);
    int index = line_detail->getLineIndex();
    line_detail->setPartIndex(index);
    return detail;
}

void SoBrepEdgeSet::initBoundingBoxes(const SbVec3f *coords, int numverts)
{
    bboxMap.clear();
    bboxPicker.clear();

    const int32_t* cindices = this->coordIndex.getValues(0);
    int numcindices = this->coordIndex.getNum();

    int threshold = PartParams::SelectionPickThreshold();
    int threshold2 = PartParams::SelectionPickThreshold2();
    int step = std::max(10, (numcindices / threshold));
    std::vector<SbBox3f> boxes;
    boxes.reserve(step * threshold + 1);
    bboxMap.reserve(step * threshold + 1);
    SbBox3f bbox;
    SegmentInfo *info = nullptr;

    auto pushInfo = [&](bool force) {
        if (!info || bbox.isEmpty())
            return;
        if (!force && info->count < step) {
            if (info->count <= 1 || cindices[info->start + info->count - 2] == -1)
                return;
        }
        boxes.push_back(bbox);
        if (threshold2 >= 0 && info->count >= threshold2) {
            std::vector<SbBox3f> cboxes;
            cboxes.reserve(info->count);
            int prev = cindices[info->start];
            for (int n = info->start+1, end = info->start+info->count; n < end; ++n) {
                int vidx = cindices[n];
                bbox.makeEmpty();
                if (vidx >= 0 && vidx < numverts && prev >= 0 && prev < numverts) {
                    bbox.extendBy(coords[prev]);
                    bbox.extendBy(coords[vidx]);
                }
                // We must push the box even if it is empty, because
                // BoundBoxRayPick returns the result as an index to the bound
                // boxes. And we need to use this index to get back the vertex
                // index.
                cboxes.push_back(bbox);
                prev = vidx;
            }
            info->picker.init(std::move(cboxes));
        }
        bbox.makeEmpty();
        info = nullptr;
    };

    for (int i = 0; i < numcindices; ++i) {
        int vidx = cindices[i];
        if (vidx < 0 || vidx >= numverts)
            continue;
        if (!info) {
            bboxMap.emplace_back();
            info = &bboxMap.back();
            info->start = i;
            info->count = 0;
        }
        bbox.extendBy(coords[vidx]);
        info->count = i - info->start + 1;
        pushInfo(false);
    }
    pushInfo(true);
    if (bboxMap.size() > boxes.size())
        bboxMap.pop_back();
    bboxPicker.init(std::move(boxes));
}

void SoBrepEdgeSet::rayPick(SoRayPickAction *action) {

    SelContextPtr ctx2 = Gui::SoFCSelectionRoot::getSecondaryActionContext<SelContext>(action,this);
    if(ctx2 && !ctx2->isSelected())
        return;

    SoState *state = action->getState();

    int threshold = PartParams::SelectionPickThreshold();
    int threshold2 = PartParams::SelectionPickThreshold2();
    const int32_t *cindices = this->coordIndex.getValues(0);
    int numindices = this->coordIndex.getNum();
    auto coords = SoCoordinateElement::getInstance(state);
    const SbVec3f *coords3d = coords->getArrayPtr3();
    int numverts = coords->getNum();

    if(threshold<=0 || numindices < threshold) {
        inherited::rayPick(action);
        return;
    }

    if (!shouldRayPick(action))
        return;

    computeObjectSpaceRay(action);

    if (getBoundingBoxCache() && getBoundingBoxCache()->isValid(state)) {
        SbBox3f box = getBoundingBoxCache()->getProjectedBox();
        if(box.isEmpty() || !action->intersect(box,TRUE))
            return;
    }

    static thread_local std::vector<int> results;
    results.clear();

    if (bboxPicker.empty())
        initBoundingBoxes(coords3d, numverts);

    auto pickSegment = [&](int idx) {
        if (idx < 1 || idx >= numindices)
            return false;
        int vidx0 = cindices[idx-1];
        if (vidx0 < 0 || vidx0 >= numverts)
            return false;
        int vidx1 = cindices[idx];
        if (vidx1 < 0 || vidx1 >= numverts)
            return false;
        const auto &p0 = coords3d[vidx0];
        const auto &p1 = coords3d[vidx1];
        SbVec3f intersection;
        if (action->intersect(p0, p1, intersection)) {
            if (action->isBetweenPlanes(intersection)) {
                auto it = std::upper_bound(segments.begin(), segments.end(), idx);
                assert(it != segments.end());
                int id = it - segments.begin();
                if(ctx2 && !ctx2->isSelectAll() && !ctx2->selectionIndex.count(id))
                    return false;
                SoPickedPoint * pp = action->addIntersection(intersection);
                if (pp) {
                    auto ld = new SoLineDetail;
                    SoPointDetail pd;
                    pd.setCoordinateIndex(vidx0);
                    ld->setPoint0(&pd);
                    pd.setCoordinateIndex(vidx1);
                    ld->setPoint1(&pd);
                    ld->setLineIndex(id);
                    ld->setPartIndex(id);
                    pp->setDetail(ld, this);
                    return true;
                }
            }
        }
        return false;
    };

    auto pick = [&](int bboxId) {
        auto &info = bboxMap[bboxId];
        if (!PartParams::SelectionPickRTree() || threshold2 < 0 || info.count < threshold2) {
            for (int i = info.start+1, end = info.start + info.count; i < end; ++i)
                pickSegment(i);
        } else {
            std::size_t offset = results.size();
            info.picker.rayPick(action, results);
            for(auto i=offset;i<results.size();++i)
                pickSegment(results[i] + info.start + 1);
            results.resize(offset);
        }
    };

    const auto &boxes = bboxPicker.getBoundBoxes();
    int numparts = (int)bboxMap.size();

    if(!PartParams::SelectionPickRTree() || numparts < threshold) {
        for(int bboxId=0;bboxId<numparts;++bboxId) {
            auto &box = boxes[bboxId];
            if(box.isEmpty() || !action->intersect(box,TRUE))
                continue;
            pick(bboxId);
        }
    } else {
        bboxPicker.rayPick(action, results);
        for(std::size_t i=0;i<results.size();++i)
            pick(results[i]);
    }
}
