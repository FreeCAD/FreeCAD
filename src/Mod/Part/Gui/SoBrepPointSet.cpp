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
# include <Inventor/elements/SoLineWidthElement.h>
# include <Inventor/elements/SoPointSizeElement.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/errors/SoReadError.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/misc/SoState.h>
#endif

#include "SoBrepPointSet.h"
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCSelectionAction.h>

using namespace PartGui;

SO_NODE_SOURCE(SoBrepPointSet);

void SoBrepPointSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepPointSet, SoPointSet, "PointSet");
}

SoBrepPointSet::SoBrepPointSet()
    :selContext(std::make_shared<SelContext>())
    ,selContext2(std::make_shared<SelContext>())
{
    SO_NODE_CONSTRUCTOR(SoBrepPointSet);
}

void SoBrepPointSet::GLRender(SoGLRenderAction *action)
{
    auto state = action->getState();
    selCounter.checkRenderCache(state);

    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(state);
    int num = coords->getNum() - this->startIndex.getValue();
    if (num < 0) {
        // Fixes: #0000545: Undo revolve causes crash 'illegal storage'
        return;
    }
    SelContextPtr ctx2;
    SelContextPtr ctx = Gui::SoFCSelectionRoot::getRenderContext<SelContext>(this,selContext,ctx2);
    if(ctx2 && ctx2->selectionIndex.empty())
        return;
    if(selContext2->checkGlobal(ctx))
        ctx = selContext2;

    if(ctx && ctx->highlightIndex==INT_MAX) {
        if(ctx->selectionIndex.empty() || ctx->isSelectAll()) {
            if(ctx2) {
                ctx2->selectionColor = ctx->highlightColor;
                renderSelection(action,ctx2); 
            } else
                renderHighlight(action,ctx);
        }else{
            if(!action->isRenderingDelayedPaths())
                renderSelection(action,ctx); 
            if(ctx2) {
                ctx2->selectionColor = ctx->highlightColor;
                renderSelection(action,ctx2); 
            } else
                renderHighlight(action,ctx);
            if(action->isRenderingDelayedPaths())
                renderSelection(action,ctx); 
        }
        return;
    }

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

void SoBrepPointSet::GLRenderBelowPath(SoGLRenderAction * action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepPointSet::renderHighlight(SoGLRenderAction *action, SelContextPtr ctx)
{
    if(!ctx || ctx->highlightIndex<0)
        return;

    SoState * state = action->getState();
    state->push();
    float ps = SoPointSizeElement::get(state);
    if (ps < 4.0f) SoPointSizeElement::set(state, this, 4.0f);

    SoLazyElement::setEmissive(state, &ctx->highlightColor);
    packedColor = ctx->highlightColor.getPackedValue(0.0);
    SoLazyElement::setPacked(state, this,1, &packedColor,false);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;

    this->getVertexData(state, coords, normals, false);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    int id = ctx->highlightIndex;
    const SbVec3f * coords3d = coords->getArrayPtr3();
    if(coords3d) {
        if(id == INT_MAX) {
            glBegin(GL_POINTS);
            for(int idx=startIndex.getValue();idx<coords->getNum();++idx)
                glVertex3fv((const GLfloat*) (coords3d + idx));
            glEnd();
        }else if (id < this->startIndex.getValue() || id >= coords->getNum()) {
            SoDebugError::postWarning("SoBrepPointSet::renderHighlight", "highlightIndex out of range");
        }
        else {
            glBegin(GL_POINTS);
            glVertex3fv((const GLfloat*) (coords3d + id));
            glEnd();
        }
    }
    state->pop();
}

void SoBrepPointSet::renderSelection(SoGLRenderAction *action, SelContextPtr ctx, bool push)
{
    SoState * state = action->getState();
    if(push) {
        state->push();
        float ps = SoPointSizeElement::get(state);
        if (ps < 4.0f) SoPointSizeElement::set(state, this, 4.0f);

        SoLazyElement::setEmissive(state, &ctx->selectionColor);
        packedColor = ctx->selectionColor.getPackedValue(0.0);
        SoLazyElement::setPacked(state, this,1, &packedColor,false);
    }

    const SoCoordinateElement * coords;
    const SbVec3f * normals;

    this->getVertexData(state, coords, normals, false);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    bool warn = false;
    int startIndex = this->startIndex.getValue();
    const SbVec3f * coords3d = coords->getArrayPtr3();
    if(coords3d) {
        glBegin(GL_POINTS);
        if(ctx->isSelectAll()) {
            for(int idx=startIndex;idx<coords->getNum();++idx)
                glVertex3fv((const GLfloat*) (coords3d + idx));
        }else{
            for(auto idx : ctx->selectionIndex) {
                if(idx >= startIndex && idx < coords->getNum())
                    glVertex3fv((const GLfloat*) (coords3d + idx));
                else
                    warn = true;
            }
        }
        glEnd();
    }
    if(warn)
        SoDebugError::postWarning("SoBrepPointSet::renderSelection", "selectionIndex out of range");
    if(push) state->pop();
}

void SoBrepPointSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        selCounter.checkAction(hlaction);
        if (!hlaction->isHighlighted()) {
            SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
            if(ctx) {
                ctx->highlightIndex = -1;
                touch();
            }
            return;
        }
        SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
        const SoDetail* detail = hlaction->getElement();
        if (!detail) {
            ctx->highlightIndex = INT_MAX;
            ctx->highlightColor = hlaction->getColor();
            touch();
            return;
        }else if (!detail->isOfType(SoPointDetail::getClassTypeId())) {
            ctx->highlightIndex = -1;
            touch();
            return;
        }

        int index = static_cast<const SoPointDetail*>(detail)->getCoordinateIndex();
        if(index!=ctx->highlightIndex) {
            ctx->highlightIndex = index;
            ctx->highlightColor = hlaction->getColor();
            touch();
        }
        return;
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);
        switch(selaction->getType()) {
        case Gui::SoSelectionElementAction::All: {
            SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
            selCounter.checkAction(selaction,ctx);
            ctx->selectionColor = selaction->getColor();
            ctx->selectionIndex.clear();
            ctx->selectionIndex.insert(-1);
            touch();
            return;
        } case Gui::SoSelectionElementAction::None:
            if(selaction->isSecondary()) {
                if(Gui::SoFCSelectionRoot::removeActionContext(action,this))
                    touch();
            } else {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
                if(ctx) {
                    ctx->selectionIndex.clear();
                    touch();
                }
            }
            return;
        case Gui::SoSelectionElementAction::Remove:
        case Gui::SoSelectionElementAction::Append: {
            const SoDetail* detail = selaction->getElement();
            if (!detail || !detail->isOfType(SoPointDetail::getClassTypeId())) {
                if(selaction->isSecondary()) {
                    // For secondary context, a detail of different type means
                    // the user may want to partial render only other type of
                    // geometry. So we call below to obtain a action context.
                    // If no secondary context exist, it will create an empty
                    // one, and an empty secondary context inhibites drawing
                    // here.
                    auto ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
                    selCounter.checkAction(selaction,ctx);
                    touch();
                }
                return;
            }
            int index = static_cast<const SoPointDetail*>(detail)->getCoordinateIndex();
            if(selaction->getType() == Gui::SoSelectionElementAction::Append) {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext);
                selCounter.checkAction(selaction,ctx);
                ctx->selectionColor = selaction->getColor();
                if(ctx->isSelectAll()) 
                    ctx->selectionIndex.clear();
                if(ctx->selectionIndex.insert(index).second)
                    touch();
            } else {
                SelContextPtr ctx = Gui::SoFCSelectionRoot::getActionContext(action,this,selContext,false);
                if(ctx && ctx->removeIndex(index))
                    touch();
            }
            break;
        } default:
            break;
        }
        return;
    }

    inherited::doAction(action);
}

