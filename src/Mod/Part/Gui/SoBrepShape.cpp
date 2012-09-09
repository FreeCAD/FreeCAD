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
# include <Inventor/errors/SoReadError.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/misc/SoState.h>
#endif

#include "SoBrepShape.h"
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCSelectionAction.h>

using namespace PartGui;


SO_NODE_SOURCE(SoBrepFaceSet);

void SoBrepFaceSet::initClass()
{
    SO_NODE_INIT_CLASS(SoBrepFaceSet, SoIndexedFaceSet, "IndexedFaceSet");
}

SoBrepFaceSet::SoBrepFaceSet()
{
    SO_NODE_CONSTRUCTOR(SoBrepFaceSet);
    SO_NODE_ADD_FIELD(partIndex, (-1));
    SO_NODE_ADD_FIELD(highlightIndex, (-1));
    SO_NODE_ADD_FIELD(selectionIndex, (-1));
    selectionIndex.setNum(0);
}

void SoBrepFaceSet::doAction(SoAction* action)
{
    if (action->getTypeId() == Gui::SoHighlightElementAction::getClassTypeId()) {
        Gui::SoHighlightElementAction* hlaction = static_cast<Gui::SoHighlightElementAction*>(action);
        if (!hlaction->isHighlighted()) {
            this->highlightIndex = -1;
            return;
        }

        const SoDetail* detail = hlaction->getElement();
        if (detail) {
            if (detail->isOfType(SoFaceDetail::getClassTypeId())) {
                int index = static_cast<const SoFaceDetail*>(detail)->getPartIndex();
                this->highlightIndex.setValue(index);
                this->highlightColor = hlaction->getColor();
            }
            else {
                this->highlightIndex = -1;
                return;
            }
        }
    }
    else if (action->getTypeId() == Gui::SoSelectionElementAction::getClassTypeId()) {
        Gui::SoSelectionElementAction* selaction = static_cast<Gui::SoSelectionElementAction*>(action);
        this->selectionColor = selaction->getColor();
        if (selaction->getType() == Gui::SoSelectionElementAction::All) {
            int num = this->partIndex.getNum();
            this->selectionIndex.setNum(num);
            int32_t* v = this->selectionIndex.startEditing();
            for (int i=0; i<num;i++)
                v[i] = i;
            this->selectionIndex.finishEditing();
            return;
        }
        else if (selaction->getType() == Gui::SoSelectionElementAction::None) {
            this->selectionIndex.setNum(0);
            return;
        }

        const SoDetail* detail = selaction->getElement();
        if (detail) {
            if (!detail->isOfType(SoFaceDetail::getClassTypeId())) {
                return;
            }

            int index = static_cast<const SoFaceDetail*>(detail)->getPartIndex();
            switch (selaction->getType()) {
            case Gui::SoSelectionElementAction::Append:
                {
                    int start = this->selectionIndex.getNum();
                    this->selectionIndex.set1Value(start, index);
                }
                break;
            case Gui::SoSelectionElementAction::Remove:
                {
                    int start = this->selectionIndex.find(index);
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

void SoBrepFaceSet::GLRender(SoGLRenderAction *action)
{
    if (this->coordIndex.getNum() < 3)
        return;
    if (this->selectionIndex.getNum() > 0)
        renderSelection(action);
    if (this->highlightIndex.getValue() >= 0)
        renderHighlight(action);
    if (!this->shouldGLRender(action))
        return;

    SoState * state = action->getState();

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numindices;
    const int32_t * nindices;
    const int32_t * tindices;
    const int32_t * mindices;
    const int32_t * pindices;
    int numparts;
    SbBool doTextures;
    SbBool normalCacheUsed;

    SoMaterialBundle mb(action);

    SoTextureCoordinateBundle tb(action, TRUE, FALSE);
    doTextures = tb.needCoordinates();
    SbBool sendNormals = !mb.isColorOnly() || tb.isFunction();

    this->getVertexData(state, coords, normals, cindices,
                        nindices, tindices, mindices, numindices,
                        sendNormals, normalCacheUsed);

    mb.sendFirst(); // make sure we have the correct material

    // just in case someone forgot
    if (!mindices) mindices = cindices;
    if (!nindices) nindices = cindices;
    pindices = this->partIndex.getValues(0);
    numparts = this->partIndex.getNum();
    renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numindices,
        pindices, numparts, normals, nindices, &mb, mindices, &tb, tindices, nbind, mbind, doTextures?1:0);
    // Disable caching for this node
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);

    // Workaround for #0000433
//#if !defined(FC_OS_WIN32)
    if (this->highlightIndex.getValue() >= 0)
        renderHighlight(action);
    if (this->selectionIndex.getNum() > 0)
        renderSelection(action);
//#endif
}

void SoBrepFaceSet::GLRenderBelowPath(SoGLRenderAction * action)
{
    inherited::GLRenderBelowPath(action);
}

void SoBrepFaceSet::renderHighlight(SoGLRenderAction *action)
{
    SoState * state = action->getState();
    state->push();

    SoLazyElement::setEmissive(state, &this->highlightColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, TRUE);
#if 0 // disables shading effect
    // sendNormals will be FALSE
    SoLazyElement::setDiffuse(state, this,1, &this->highlightColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
#endif

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numindices;
    const int32_t * nindices;
    const int32_t * tindices;
    const int32_t * mindices;
    const int32_t * pindices;
    SbBool doTextures;
    SbBool normalCacheUsed;

    SoMaterialBundle mb(action);
    SoTextureCoordinateBundle tb(action, TRUE, FALSE);
    doTextures = tb.needCoordinates();
    SbBool sendNormals = !mb.isColorOnly() || tb.isFunction();

    this->getVertexData(state, coords, normals, cindices,
                        nindices, tindices, mindices, numindices,
                        sendNormals, normalCacheUsed);

    mb.sendFirst(); // make sure we have the correct material

    int32_t id = this->highlightIndex.getValue();

    // just in case someone forgot
    if (!mindices) mindices = cindices;
    if (!nindices) nindices = cindices;
    pindices = this->partIndex.getValues(0);

    // coords
    int length = (int)pindices[id]*4;
    int start=0;
    for (int i=0;i<id;i++)
        start+=(int)pindices[i];
    start *= 4;

    // normals
    if (nbind == PER_VERTEX_INDEXED)
        nindices = &(nindices[start]);
    else if (nbind == PER_VERTEX)
        normals = &(normals[start]);
    else 
        nbind = OVERALL;

    // materials
    mbind = OVERALL;
    doTextures = FALSE;

    renderShape(static_cast<const SoGLCoordinateElement*>(coords), &(cindices[start]), length,
        &(pindices[id]), 1, normals, nindices, &mb, mindices, &tb, tindices, nbind, mbind, doTextures?1:0);
    state->pop();
}

void SoBrepFaceSet::renderSelection(SoGLRenderAction *action)
{
    int numSelected =  this->selectionIndex.getNum();
    const int32_t* selected = this->selectionIndex.getValues(0);
    if (numSelected == 0) return;

    SoState * state = action->getState();
    state->push();

    SoLazyElement::setEmissive(state, &this->selectionColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, TRUE);
#if 0 // disables shading effect
    SoLazyElement::setDiffuse(state, this,1, &this->selectionColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
    SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
#endif

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numindices;
    const int32_t * nindices;
    const int32_t * tindices;
    const int32_t * mindices;
    const int32_t * pindices;
    SbBool doTextures;
    SbBool normalCacheUsed;

    SoMaterialBundle mb(action);
    SoTextureCoordinateBundle tb(action, TRUE, FALSE);
    doTextures = tb.needCoordinates();
    SbBool sendNormals = !mb.isColorOnly() || tb.isFunction();

    this->getVertexData(state, coords, normals, cindices,
                        nindices, tindices, mindices, numindices,
                        sendNormals, normalCacheUsed);

    mb.sendFirst(); // make sure we have the correct material

    // just in case someone forgot
    if (!mindices) mindices = cindices;
    if (!nindices) nindices = cindices;
    pindices = this->partIndex.getValues(0);

    // materials
    mbind = OVERALL;
    doTextures = FALSE;

    for (int i=0; i<numSelected; i++) {
        int id = selected[i];

        // coords
        int length = (int)pindices[id]*4;
        int start=0;
        for (int j=0;j<id;j++)
            start+=(int)pindices[j];
        start *= 4;

        // normals
        const SbVec3f * normals_s = normals;
        const int32_t * nindices_s = nindices;
        if (nbind == PER_VERTEX_INDEXED)
            nindices_s = &(nindices[start]);
        else if (nbind == PER_VERTEX)
            normals_s = &(normals[start]);
        else 
            nbind = OVERALL;

        renderShape(static_cast<const SoGLCoordinateElement*>(coords), &(cindices[start]), length,
            &(pindices[id]), 1, normals_s, nindices_s, &mb, mindices, &tb, tindices, nbind, mbind, doTextures?1:0);
    }
    state->pop();
}

void SoBrepFaceSet::renderShape(const SoGLCoordinateElement * const vertexlist,
                                const int32_t *vertexindices,
                                int numindices,
                                const int32_t *partindices,
                                int num_partindices,
                                const SbVec3f *normals,
                                const int32_t *normalindices,
                                SoMaterialBundle *const materials,
                                const int32_t *matindices,
                                SoTextureCoordinateBundle * const texcoords,
                                const int32_t *texindices,
                                const int nbind,
                                const int mbind,
                                const int texture)
{
    int texidx = 0;

    const SbVec3f * coords3d = NULL;
    coords3d = vertexlist->getArrayPtr3();

    const int32_t *viptr = vertexindices;
    const int32_t *viendptr = viptr + numindices;
    const int32_t *piptr = partindices;
    const int32_t *piendptr = piptr + num_partindices;
    int32_t v1, v2, v3, v4, pi;
    SbVec3f dummynormal(0,0,1);
    int numverts = vertexlist->getNum();

    const SbVec3f *currnormal = &dummynormal;
    if (normals) currnormal = normals;

    int matnr = 0;
    int trinr = 0;
    pi = piptr < piendptr ? *piptr++ : -1;
    while (pi == 0) {
        // It may happen that a part has no triangles
        pi = piptr < piendptr ? *piptr++ : -1;
        if (mbind == PER_PART)
            matnr++;
        else if (mbind == PER_PART_INDEXED)
            matindices++;
    }

    glBegin(GL_TRIANGLES);
    while (viptr + 2 < viendptr) {
        v1 = *viptr++;
        v2 = *viptr++;
        v3 = *viptr++;

        // This test is for robustness upon buggy data sets
        if (v1 < 0 || v2 < 0 || v3 < 0 ||
            v1 >= numverts || v2 >= numverts || v3 >= numverts) {
            break;
        }
        v4 = viptr < viendptr ? *viptr++ : -1;

        /* vertex 1 *********************************************************/
        if (mbind == PER_PART) {
            if (trinr == 0)
                materials->send(matnr++, TRUE);
        }
        else if (mbind == PER_PART_INDEXED) {
            if (trinr == 0)
                materials->send(*matindices++, TRUE);
        }
        else if (mbind == PER_VERTEX || mbind == PER_FACE) {
            materials->send(matnr++, TRUE);
        }
        else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
            materials->send(*matindices++, TRUE);
        }

        if (normals) {
            if (nbind == PER_VERTEX || nbind == PER_FACE) {
                currnormal = normals++;
                glNormal3fv((const GLfloat*)currnormal);
            }
            else if (nbind == PER_VERTEX_INDEXED || nbind == PER_FACE_INDEXED) {
                currnormal = &normals[*normalindices++];
                glNormal3fv((const GLfloat*)currnormal);
            }
        }

        if (texture) {
            texcoords->send(texindices ? *texindices++ : texidx++,
                        vertexlist->get3(v1),
                        *currnormal);
        }

        glVertex3fv((const GLfloat*) (coords3d + v1));

        /* vertex 2 *********************************************************/
        if (mbind == PER_VERTEX)
            materials->send(matnr++, TRUE);
        else if (mbind == PER_VERTEX_INDEXED)
            materials->send(*matindices++, TRUE);

        if (normals) {
            if (nbind == PER_VERTEX) {
                currnormal = normals++;
                glNormal3fv((const GLfloat*)currnormal);
            }
            else if (nbind == PER_VERTEX_INDEXED) {
                currnormal = &normals[*normalindices++];
                glNormal3fv((const GLfloat*)currnormal);
            }
        }

        if (texture) {
            texcoords->send(texindices ? *texindices++ : texidx++,
                            vertexlist->get3(v2),
                            *currnormal);
        }

        glVertex3fv((const GLfloat*) (coords3d + v2));

        /* vertex 3 *********************************************************/
        if (mbind == PER_VERTEX)
            materials->send(matnr++, TRUE);
        else if (mbind == PER_VERTEX_INDEXED)
            materials->send(*matindices++, TRUE);

        if (normals) {
            if (nbind == PER_VERTEX) {
                currnormal = normals++;
                glNormal3fv((const GLfloat*)currnormal);
            }
            else if (nbind == PER_VERTEX_INDEXED) {
                currnormal = &normals[*normalindices++];
                glNormal3fv((const GLfloat*)currnormal);
            }
        }

        if (texture) {
            texcoords->send(texindices ? *texindices++ : texidx++,
                            vertexlist->get3(v3),
                            *currnormal);
        }

        glVertex3fv((const GLfloat*) (coords3d + v3));

        if (mbind == PER_VERTEX_INDEXED)
            matindices++;

        if (nbind == PER_VERTEX_INDEXED)
            normalindices++;

        if (texture && texindices) {
            texindices++;
        }

        trinr++;
        if (pi == trinr) {
            pi = piptr < piendptr ? *piptr++ : -1;
            while (pi == 0) {
                // It may happen that a part has no triangles
                pi = piptr < piendptr ? *piptr++ : -1;
                if (mbind == PER_PART)
                    matnr++;
                else if (mbind == PER_PART_INDEXED)
                    matindices++;
            }
            trinr = 0;
        }
    }
    glEnd();
}

SoDetail * SoBrepFaceSet::createTriangleDetail(SoRayPickAction * action,
                                               const SoPrimitiveVertex * v1,
                                               const SoPrimitiveVertex * v2,
                                               const SoPrimitiveVertex * v3,
                                               SoPickedPoint * pp)
{
    SoDetail* detail = inherited::createTriangleDetail(action, v1, v2, v3, pp);
    const int32_t * indices = this->partIndex.getValues(0);
    int num = this->partIndex.getNum();
    if (indices) {
        SoFaceDetail* face_detail = static_cast<SoFaceDetail*>(detail);
        int index = face_detail->getFaceIndex();
        int count = 0;
        for (int i=0; i<num; i++) {
            count += indices[i];
            if (index < count) {
                face_detail->setPartIndex(i);
                break;
            }
        }
    }
    return detail;
}

SoBrepFaceSet::Binding
SoBrepFaceSet::findMaterialBinding(SoState * const state) const
{
    Binding binding = OVERALL;
    SoMaterialBindingElement::Binding matbind =
        SoMaterialBindingElement::get(state);

    switch (matbind) {
    case SoMaterialBindingElement::OVERALL:
        binding = OVERALL;
        break;
    case SoMaterialBindingElement::PER_VERTEX:
        binding = PER_VERTEX;
        break;
    case SoMaterialBindingElement::PER_VERTEX_INDEXED:
        binding = PER_VERTEX_INDEXED;
        break;
    case SoMaterialBindingElement::PER_PART:
        binding = PER_PART;
        break;
    case SoMaterialBindingElement::PER_FACE:
        binding = PER_FACE;
        break;
    case SoMaterialBindingElement::PER_PART_INDEXED:
        binding = PER_PART_INDEXED;
        break;
    case SoMaterialBindingElement::PER_FACE_INDEXED:
        binding = PER_FACE_INDEXED;
        break;
    default:
        break;
    }
    return binding;
}

SoBrepFaceSet::Binding
SoBrepFaceSet::findNormalBinding(SoState * const state) const
{
    Binding binding = PER_VERTEX_INDEXED;
    SoNormalBindingElement::Binding normbind =
        (SoNormalBindingElement::Binding) SoNormalBindingElement::get(state);

    switch (normbind) {
    case SoNormalBindingElement::OVERALL:
        binding = OVERALL;
        break;
    case SoNormalBindingElement::PER_VERTEX:
        binding = PER_VERTEX;
        break;
    case SoNormalBindingElement::PER_VERTEX_INDEXED:
        binding = PER_VERTEX_INDEXED;
        break;
    case SoNormalBindingElement::PER_PART:
        binding = PER_PART;
        break;
    case SoNormalBindingElement::PER_FACE:
        binding = PER_FACE;
        break;
    case SoNormalBindingElement::PER_PART_INDEXED:
        binding = PER_PART_INDEXED;
        break;
    case SoNormalBindingElement::PER_FACE_INDEXED:
        binding = PER_FACE_INDEXED;
        break;
    default:
        break;
    }
    return binding;
}

// ---------------------------------------------------------------------

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
    SoOverrideElement::setEmissiveColorOverride(state, this, TRUE);
    SoLazyElement::setDiffuse(state, this,1, &this->highlightColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
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
        tindices, mindices, numcindices, FALSE, normalCacheUsed);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    const int32_t* id = &(this->hl[0]);
    int num = (int)this->hl.size();

    renderShape(static_cast<const SoGLCoordinateElement*>(coords), id, num);
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
    SoOverrideElement::setEmissiveColorOverride(state, this, TRUE);
    SoLazyElement::setDiffuse(state, this,1, &this->selectionColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
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
        tindices, mindices, numcindices, FALSE, normalCacheUsed);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    cindices = &(this->sl[0]);
    numcindices = (int)this->sl.size();

    renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
    state->pop();
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
            const int32_t* cindices = this->coordIndex.getValues(0);
            int numcindices = this->coordIndex.getNum();
            unsigned int num = std::count_if(cindices, cindices+numcindices, 
                std::bind2nd(std::equal_to<int32_t>(), -1));

            this->sl.clear();
            this->selectionIndex.setNum(num);
            int32_t* v = this->selectionIndex.startEditing();
            for (unsigned int i=0; i<num;i++)
                v[i] = i;
            this->selectionIndex.finishEditing();

            int numsegm = this->selectionIndex.getNum();
            if (numsegm > 0) {
                const int32_t* selsegm = this->selectionIndex.getValues(0);
                const int32_t* cindices = this->coordIndex.getValues(0);
                int numcindices = this->coordIndex.getNum();
                createIndexArray(selsegm, numsegm, cindices, numcindices, this->sl);
            }
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
                    int start = this->selectionIndex.getNum();
                    this->selectionIndex.set1Value(start, index);
                }
                break;
            case Gui::SoSelectionElementAction::Remove:
                {
                    int start = this->selectionIndex.find(index);
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

// ---------------------------------------------------------------------

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
    SoOverrideElement::setEmissiveColorOverride(state, this, TRUE);
    SoLazyElement::setDiffuse(state, this,1, &this->highlightColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;

    this->getVertexData(state, coords, normals, FALSE);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    int32_t id = this->highlightIndex.getValue();

    renderShape(static_cast<const SoGLCoordinateElement*>(coords), &id, 1);
    state->pop();
}

void SoBrepPointSet::renderSelection(SoGLRenderAction *action)
{
    SoState * state = action->getState();
    state->push();
    float ps = SoPointSizeElement::get(state);
    if (ps < 4.0f) SoPointSizeElement::set(state, this, 4.0f);

    SoLazyElement::setEmissive(state, &this->selectionColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, TRUE);
    SoLazyElement::setDiffuse(state, this,1, &this->selectionColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numcindices;

    this->getVertexData(state, coords, normals, FALSE);

    SoMaterialBundle mb(action);
    mb.sendFirst(); // make sure we have the correct material

    cindices = this->selectionIndex.getValues(0);
    numcindices = this->selectionIndex.getNum();

    renderShape(static_cast<const SoGLCoordinateElement*>(coords), cindices, numcindices);
    state->pop();
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
                    int start = this->selectionIndex.getNum();
                    this->selectionIndex.set1Value(start, index);
                }
                break;
            case Gui::SoSelectionElementAction::Remove:
                {
                    int start = this->selectionIndex.find(index);
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
