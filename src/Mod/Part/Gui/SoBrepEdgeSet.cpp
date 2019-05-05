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

#include "SoBrepEdgeSet.h"
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCSelectionAction.h>

using namespace PartGui;


SO_NODE_SOURCE(SoBrepEdgeSet);

void SoBrepEdgeSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepEdgeSet, SoIndexedLineSet, "IndexedLineSet");
}

SoBrepEdgeSet::SoBrepEdgeSet()
{
    SO_NODE_CONSTRUCTOR(SoBrepEdgeSet);
    SO_NODE_ADD_FIELD(highlightIndex, (-1));
    SO_NODE_ADD_FIELD(selectionIndex, (-1));
    selectionIndex.setNum(0);
}

void SoBrepEdgeSet::GLRender(SoGLRenderAction *action)
{
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

void SoBrepEdgeSet::renderHighlight(SoGLRenderAction *action)
{
    SoState * state = action->getState();
    state->push();
  //SoLineWidthElement::set(state, this, 4.0f);

    SoLazyElement::setEmissive(state, &this->highlightColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, true);
    SoLazyElement::setDiffuse(state, this,1, &this->highlightColor,&this->colorpacker1);
    SoOverrideElement::setDiffuseColorOverride(state, this, true);
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);

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

    int num = (int)this->hl.size();
    if (num > 0) {
        const int32_t* id = &(this->hl[0]);
        if (!validIndexes(coords, this->hl)) {
            SoDebugError::postWarning("SoBrepEdgeSet::renderHighlight", "highlightIndex out of range");
        }
        else {
            renderShape(static_cast<const SoGLCoordinateElement*>(coords), id, num);
        }
    }
    state->pop();
}

void SoBrepEdgeSet::renderSelection(SoGLRenderAction *action)
{
    int numSelected =  this->selectionIndex.getNum();
    if (numSelected == 0) return;

    SoState * state = action->getState();
    state->push();
  //SoLineWidthElement::set(state, this, 4.0f);

    SoLazyElement::setEmissive(state, &this->selectionColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, true);
    SoLazyElement::setDiffuse(state, this,1, &this->selectionColor,&this->colorpacker2);
    SoOverrideElement::setDiffuseColorOverride(state, this, true);
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);

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

    int num = (int)this->sl.size();
    if (num > 0) {
        if (this->sl[0] < 0) {
            renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
        }
        else {
            cindices = &(this->sl[0]);
            numcindices = (int)this->sl.size();
            if (!validIndexes(coords, this->sl)) {
                SoDebugError::postWarning("SoBrepEdgeSet::renderSelection", "selectionIndex out of range");
            }
            else {
                renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
            }
        }
    }
    state->pop();
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

static void createIndexArray(const int32_t* segm, int numsegm,
                             const int32_t* cindices, int numcindices,
                             std::vector<int32_t>& out)
{
    std::vector<int32_t> v;
    for (int j=0; j<numsegm; j++) {
        int index = segm[j];
        int start=0, num=0;
        int section=0;
        for (int i=0;i<numcindices;i++) {
            if (section < index)
                start++;
            else if (section == index)
                num++;
            else if (section > index)
                break;
            if (cindices[i] < 0)
                section++;
        }

        v.insert(v.end(), cindices+start, cindices+start+num);
    }

    out.swap(v);
}

void SoBrepEdgeSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        if (!hlaction->isHighlighted()) {
            this->highlightIndex = -1;
            this->hl.clear();
            return;
        }
        const SoDetail* detail = hlaction->getElement();
        if (detail) {
            if (!detail->isOfType(SoLineDetail::getClassTypeId())) {
                this->highlightIndex = -1;
                this->hl.clear();
                return;
            }

            this->highlightColor = hlaction->getColor();
            int32_t index = static_cast<const SoLineDetail*>(detail)->getLineIndex();
            const int32_t* cindices = this->coordIndex.getValues(0);
            int numcindices = this->coordIndex.getNum();

            createIndexArray(&index, 1, cindices, numcindices, this->hl);
            this->highlightIndex.setValue(index);
        }
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);

        this->selectionColor = selaction->getColor();
        if (selaction->getType() == Gui::SoSelectionElementAction::All) {
            //const int32_t* cindices = this->coordIndex.getValues(0);
            //int numcindices = this->coordIndex.getNum();
            //unsigned int num = std::count_if(cindices, cindices+numcindices,
            //    std::bind2nd(std::equal_to<int32_t>(), -1));

            //this->sl.clear();
            //this->selectionIndex.setNum(num);
            //int32_t* v = this->selectionIndex.startEditing();
            //for (unsigned int i=0; i<num;i++)
            //    v[i] = i;
            //this->selectionIndex.finishEditing();

            //int numsegm = this->selectionIndex.getNum();
            //if (numsegm > 0) {
            //    const int32_t* selsegm = this->selectionIndex.getValues(0);
            //    const int32_t* cindices = this->coordIndex.getValues(0);
            //    int numcindices = this->coordIndex.getNum();
            //    createIndexArray(selsegm, numsegm, cindices, numcindices, this->sl);
            //}
            this->selectionIndex.setValue(-1); // all
            this->sl.clear();
            this->sl.push_back(-1);
            return;
        }
        else if (selaction->getType() == Gui::SoSelectionElementAction::None) {
            this->selectionIndex.setNum(0);
            this->sl.clear();
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

            int numsegm = this->selectionIndex.getNum();
            if (numsegm > 0) {
                const int32_t* selsegm = this->selectionIndex.getValues(0);
                const int32_t* cindices = this->coordIndex.getValues(0);
                int numcindices = this->coordIndex.getNum();
                createIndexArray(selsegm, numsegm, cindices, numcindices, this->sl);
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
