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
# include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# endif
# include <float.h>
# include <algorithm>
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
# include <Inventor/elements/SoLineWidthElement.h>
# include <Inventor/elements/SoPointSizeElement.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/errors/SoReadError.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/misc/SoState.h>
#endif

#include "SoBrepEdgeSet.h"
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCSelectionAction.h>

using namespace PartGui;

SO_NODE_SOURCE(SoBrepEdgeSet);

class SoBrepEdgeSet::SelContext {
public:
    SelContext():highlightIndex(-1)
    {}
     
    bool removeIndex(int index) {
        auto it = selectionIndex.find(index);
        if(it == selectionIndex.end()) return false;
        selectionIndex.erase(it);
        return true;
    }

    bool isSelectAll() const {
        return selectionIndex.size() && *selectionIndex.begin()<0;
    }

    int highlightIndex;
    std::set<int> selectionIndex;

    std::vector<int32_t> hl, sl;
    SbColor selectionColor;
    SbColor highlightColor;
};

void SoBrepEdgeSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepEdgeSet, SoIndexedLineSet, "IndexedLineSet");
}

SoBrepEdgeSet::SoBrepEdgeSet()
    :selContext(std::make_shared<SelContext>())
{
    SO_NODE_CONSTRUCTOR(SoBrepEdgeSet);
}

void SoBrepEdgeSet::GLRender(SoGLRenderAction *action)
{
    auto state = action->getState();
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);

    SelContextPtr ctx2;
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext<SelContext>(this,selContext,&ctx2);
    if(ctx2 && ctx2->selectionIndex.empty())
        return;

    if(!action->isRenderingDelayedPaths())
        renderHighlight(action,ctx);
    if(ctx && ctx->selectionIndex.size()) {
        if(ctx->isSelectAll()) {
            if(ctx2 && ctx2->selectionIndex.size()) {
                ctx2->selectionColor = ctx->selectionColor;
                renderSelection(action,ctx2); 
            }else
                renderSelection(action,ctx); 
            if(action->isRenderingDelayedPaths())
                renderHighlight(action,ctx);
            return;
        }
        if(!action->isRenderingDelayedPaths())
            renderSelection(action,ctx); 
    }
    if(ctx2 && ctx2->selectionIndex.size())
        renderSelection(action,ctx2,false);
    else
        inherited::GLRender(action);

    // Workaround for #0000433
//#if !defined(FC_OS_WIN32)
    if(!action->isRenderingDelayedPaths())
        renderHighlight(action,ctx);
    if(ctx && ctx->selectionIndex.size())
        renderSelection(action,ctx);
    if(action->isRenderingDelayedPaths())
        renderHighlight(action,ctx);
//#endif
}

void SoBrepEdgeSet::GLRenderBelowPath(SoGLRenderAction * action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepEdgeSet::renderShape(const SoGLCoordinateElement * const coords,
                                const int32_t *cindices, int numindices)
{

    const SbVec3f * coords3d = coords->getArrayPtr3();

    int32_t i;
    int previ;
    const int32_t *end = cindices + numindices;
    while (cindices < end) {
        glBegin(GL_LINE_STRIP);
        previ = *cindices++;
        i = (cindices < end) ? *cindices++ : -1;
        while (i >= 0) {
            glVertex3fv((const GLfloat*) (coords3d + previ));
            glVertex3fv((const GLfloat*) (coords3d + i));
            previ = i;
            i = cindices < end ? *cindices++ : -1;
        }
        glEnd();
    }
}

void SoBrepEdgeSet::renderHighlight(SoGLRenderAction *action, SelContextPtr ctx)
{
    if(!ctx || ctx->highlightIndex<0)
        return;

    SoState * state = action->getState();
    state->push();
  //SoLineWidthElement::set(state, this, 4.0f);

    SoLazyElement::setEmissive(state, &ctx->highlightColor);
    packedColor = ctx->highlightColor.getPackedValue(0.0);
    SoLazyElement::setPacked(state, this,1, &packedColor,false);

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

    int num = (int)ctx->hl.size();
    if (num > 0) {
        const int32_t* id = &(ctx->hl[0]);
        if (!validIndexes(coords, ctx->hl)) {
            SoDebugError::postWarning("SoBrepEdgeSet::renderHighlight", "highlightIndex out of range");
        }
        else {
            renderShape(static_cast<const SoGLCoordinateElement*>(coords), id, num);
        }
    }
    state->pop();
}

void SoBrepEdgeSet::renderSelection(SoGLRenderAction *action, SelContextPtr ctx, bool push)
{
    SoState * state = action->getState();
    if(push){
        state->push();
        //SoLineWidthElement::set(state, this, 4.0f);

        SoLazyElement::setEmissive(state, &ctx->selectionColor);
        packedColor = ctx->selectionColor.getPackedValue(0.0);
        SoLazyElement::setPacked(state, this,1, &packedColor,false);
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

    int num = (int)ctx->sl.size();
    if (num > 0) {
        if (ctx->sl[0] < 0) {
            renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
        }
        else {
            cindices = &(ctx->sl[0]);
            numcindices = (int)ctx->sl.size();
            if (!validIndexes(coords, ctx->sl)) {
                SoDebugError::postWarning("SoBrepEdgeSet::renderSelection", "selectionIndex out of range");
            }
            else {
                renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
            }
        }
    }
    if(push) state->pop();
}

bool SoBrepEdgeSet::validIndexes(const SoCoordinateElement* coords, const std::vector<int32_t>& pts) const
{
    for (std::vector<int32_t>::const_iterator it = pts.begin(); it != pts.end(); ++it) {
        if (*it >= coords->getNum()) {
            return false;
        }
    }
    return true;
}

void SoBrepEdgeSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action,this,selContext);
        if (!hlaction->isHighlighted()) {
            ctx->highlightIndex = -1;
            ctx->hl.clear();
            touch();
            return;
        }
        const SoDetail* detail = hlaction->getElement();
        if (detail) {
            if (!detail->isOfType(SoLineDetail::getClassTypeId())) {
                ctx->highlightIndex = -1;
                ctx->hl.clear();
                touch();
                return;
            }

            ctx->highlightColor = hlaction->getColor();
            int index = static_cast<const SoLineDetail*>(detail)->getLineIndex();
            const int32_t* cindices = this->coordIndex.getValues(0);
            int numcindices = this->coordIndex.getNum();

            ctx->hl.clear();
            for(int section=0,i=0;i<numcindices;i++) {
                if(cindices[i] < 0) {
                    if(++section > index)
                        break;
                }else if(section == index)
                    ctx->hl.push_back(cindices[i]);
            }
            if(ctx->hl.size())
                ctx->highlightIndex = index;
            else
                ctx->highlightIndex = -1;
            touch();
        }
        return;
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);

        if(selaction->isSecondary() && selaction->getType() == Gui::SoSelectionElementAction::None) {
            Gui::SoFCSelectionRoot::removeActionContext(action,this);
            touch();
            return;
        }

        SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext<SelContext>(action,this,selContext);
        ctx->selectionColor = selaction->getColor();
        if (selaction->getType() == Gui::SoSelectionElementAction::All) {
            ctx->selectionIndex.clear();
            ctx->selectionIndex.insert(-1); // all
            ctx->sl.clear();
            ctx->sl.push_back(-1);
            touch();
            return;
        }
        else if (selaction->getType() == Gui::SoSelectionElementAction::None) {
            ctx->selectionIndex.clear();
            ctx->sl.clear();
            touch();
            return;
        }

        const SoDetail* detail = selaction->getElement();
        if (detail) {
            if (!detail->isOfType(SoLineDetail::getClassTypeId())) {
                return;
            }

            int index = static_cast<const SoLineDetail*>(detail)->getLineIndex();
            switch (selaction->getType()) {
            case Gui::SoSelectionElementAction::Append:
                if(ctx->isSelectAll()) 
                    ctx->selectionIndex.clear();
                if(!ctx->selectionIndex.insert(index).second)
                    return;
                touch();
                break;
            case Gui::SoSelectionElementAction::Remove:
                if(!ctx->removeIndex(index))
                    return;
                touch();
                break;
            default:
                return;
            }

            ctx->sl.clear();
            if(ctx->selectionIndex.size()) {
                const int32_t* cindices = this->coordIndex.getValues(0);
                int numcindices = this->coordIndex.getNum();
                auto it = ctx->selectionIndex.begin();
                for(int section=0,i=0;i<numcindices;i++) {
                    if(section == *it)
                        ctx->sl.push_back(cindices[i]);
                    if(cindices[i] < 0) {
                        if(++section > *it) {
                            if(++it == ctx->selectionIndex.end())
                                break;
                        }
                    }
                }
            }
        }
    }

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

