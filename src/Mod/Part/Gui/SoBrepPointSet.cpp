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
{
    SO_NODE_CONSTRUCTOR(SoBrepPointSet);
    SO_NODE_ADD_FIELD(highlightIndex, (-1));
    SO_NODE_ADD_FIELD(selectionIndex, (-1));
    selectionIndex.setNum(0);
}

void SoBrepPointSet::GLRender(SoGLRenderAction *action)
{
    const SoCoordinateElement* coords = SoCoordinateElement::getInstance(action->getState());
    int num = coords->getNum() - this->startIndex.getValue();
    if (num < 0) {
        // Fixes: #0000545: Undo revolve causes crash 'illegal storage'
        return;
    }
    if (this->selectionIndex.getNum() > 0)
        renderSelection(action);
    if (this->highlightIndex.getValue() >= 0)
        renderHighlight(action);
    inherited::GLRender(action);

    // Workaround for #0000433
//#if !defined(FC_OS_WIN32)
    if (this->highlightIndex.getValue() >= 0)
        renderHighlight(action);
    if (this->selectionIndex.getNum() > 0)
        renderSelection(action);
//#endif
}

void SoBrepPointSet::GLRenderBelowPath(SoGLRenderAction * action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepPointSet::renderShape(const SoGLCoordinateElement * const coords,
                                 const int32_t *cindices,
                                 int numindices)
{
    const SbVec3f * coords3d = coords->getArrayPtr3();
    if(coords3d == nullptr)
        return;

    int previ;
    const int32_t *end = cindices + numindices;
    glBegin(GL_POINTS);
    while (cindices < end) {
        previ = *cindices++;
        glVertex3fv((const GLfloat*) (coords3d + previ));
    }
    glEnd();
}

void SoBrepPointSet::renderHighlight(SoGLRenderAction *action)
{
    SoState * state = action->getState();
    state->push();
    float ps = SoPointSizeElement::get(state);
    if (ps < 4.0f) SoPointSizeElement::set(state, this, 4.0f);

    SoLazyElement::setEmissive(state, &this->highlightColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, true);
    SoLazyElement::setDiffuse(state, this,1, &this->highlightColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, true);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;

    this->getVertexData(state, coords, normals, false);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    int32_t id = this->highlightIndex.getValue();
    if (id < this->startIndex.getValue() || id >= coords->getNum()) {
        SoDebugError::postWarning("SoBrepPointSet::renderHighlight", "highlightIndex out of range");
    }
    else {
        renderShape(static_cast<const SoGLCoordinateElement*>(coords), &id, 1);
    }
    state->pop();
}

void SoBrepPointSet::renderSelection(SoGLRenderAction *action)
{
    SoState * state = action->getState();
    state->push();
    float ps = SoPointSizeElement::get(state);
    if (ps < 4.0f) SoPointSizeElement::set(state, this, 4.0f);

    SoLazyElement::setEmissive(state, &this->selectionColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, true);
    SoLazyElement::setDiffuse(state, this,1, &this->selectionColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, true);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numcindices;

    this->getVertexData(state, coords, normals, false);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    cindices = this->selectionIndex.getValues(0);
    numcindices = this->selectionIndex.getNum();

    if (!validIndexes(coords, this->startIndex.getValue(), cindices, numcindices)) {
        SoDebugError::postWarning("SoBrepPointSet::renderSelection", "selectionIndex out of range");
    }
    else {
        renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
    }
    state->pop();
}

bool SoBrepPointSet::validIndexes(const SoCoordinateElement* coords, int32_t startIndex, const int32_t * cindices, int numcindices) const
{
    for (int i=0; i<numcindices; i++) {
        int32_t id = cindices[i];
        if (id < startIndex || id >= coords->getNum()) {
            return false;
        }
    }
    return true;
}

void SoBrepPointSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        if (!hlaction->isHighlighted()) {
            this->highlightIndex = -1;
            return;
        }
        const SoDetail* detail = hlaction->getElement();
        if (detail) {
            if (!detail->isOfType(SoPointDetail::getClassTypeId())) {
                this->highlightIndex = -1;
                return;
            }

            int index = static_cast<const SoPointDetail*>(detail)->getCoordinateIndex();
            this->highlightIndex.setValue(index);
            this->highlightColor = hlaction->getColor();
        }
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);
        this->selectionColor = selaction->getColor();
        if (selaction->getType() == Gui::SoSelectionElementAction::All) {
            const SoCoordinateElement* coords = SoCoordinateElement::getInstance(action->getState());
            int num = coords->getNum() - this->startIndex.getValue();
            this->selectionIndex.setNum(num);
            int32_t* v = this->selectionIndex.startEditing();
            int32_t s = this->startIndex.getValue();
            for (int i=0; i<num;i++)
                v[i] = i + s;
            this->selectionIndex.finishEditing();
            return;
        }
        else if (selaction->getType() == Gui::SoSelectionElementAction::None) {
            this->selectionIndex.setNum(0);
            return;
        }

        const SoDetail* detail = selaction->getElement();
        if (detail) {
            if (!detail->isOfType(SoPointDetail::getClassTypeId())) {
                return;
            }

            int index = static_cast<const SoPointDetail*>(detail)->getCoordinateIndex();
            switch (selaction->getType()) {
            case Gui::SoSelectionElementAction::Append:
                {
                    if (this->selectionIndex.find(index) < 0) {
                        int start = this->selectionIndex.getNum();
                        this->selectionIndex.set1Value(start, index);
                    }
                }
                break;
            case Gui::SoSelectionElementAction::Remove:
                {
                    int start = this->selectionIndex.find(index);
                    if (start >= 0)
                        this->selectionIndex.deleteValues(start,1);
                }
                break;
            default:
                break;
            }
        }
    }

    inherited::doAction(action);
}
