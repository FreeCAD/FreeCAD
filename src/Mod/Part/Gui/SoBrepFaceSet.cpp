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

#ifndef FC_OS_WIN32
#define GL_GLEXT_PROTOTYPES
#endif

#ifndef _PreComp_
#include <float.h>
#include <algorithm>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/bundles/SoTextureCoordinateBundle.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/misc/SoState.h>
#endif

#include "SoBrepFaceSet.h"
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/SoFCSelectionAction.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <stdio.h>
#include <string.h>
#ifdef FC_OS_WIN32
#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#else
#ifdef FC_OS_MACOSX
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#endif

#if QT_VERSION < 0x50000
#include <QGLContext>
#else
#include <QOpenGLContext>
#endif


using namespace PartGui;


#if QT_VERSION < 0x50000
#define _PROC(addr) QString::fromLatin1(addr)
#else
#define _PROC(addr) QByteArray(addr)
#endif

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

// We are creating the VBO
    vbo_available=0;
#ifdef GL_NUM_EXTENSIONS
// We are running OpenGL version higher than 3.0
/*    GLint n,i;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for ( i = 0 ; i < n ; i++ )
    { 
	if ( strstr(glGetStringi(GL_EXTENSIONS, i),"GL_ARB_vertex_buffer_object") != NULL )
		vbo_available=1;
    }
*/
// we are assuming that the VBO extension is available !
   vbo_available=1;

#else
// We are probably running an old OpenGL version
// Must check into the GL_EXTENSIONS string instead
   GL_extension=(char *)glGetString(GL_EXTENSIONS);
   if ( strstr((char *)GL_extension,(char *)"GL_ARB_vertex_buffer_object") != NULL )
	vbo_available=1;
#endif

#ifdef FC_OS_WIN32
    /* Windows OpenGL implementation is very basic (aka 1.4) */
/*    if ( myglGenBuffers == NULL )
    {
	    myglGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffersARB");
	    myglDeleteBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glDeleteBuffersARB");
	    myglBindBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glBindBuffersARB");
	    myglBufferData = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glBufferDataARB");
	    myglMapBufferARB = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glMapBufferARB");
	    myglunMapBufferARB = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glunMapBufferARB");
	    myglEnableClientState = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glEnableClientState");
	    myglDisableClientState = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glDisableClientState");
	    myglVertexPointer = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glVertexPointer");
	    myglNormalPointer = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glNormalPointer");
	    myglColorPointer = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glColorPointer");
	    myglDrawElements = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glDrawElements");
    }
*/
#endif
#ifdef FC_OS_WIN32
#if QT_VERSION < 0x50000
   const QGLContext* gl = QGLContext::currentContext();
#else
   QOpenGLContext* gl = QOpenGLContext::currentContext();
#endif
   PFNGLGENBUFFERSPROC glGenBuffersARB = (PFNGLGENBUFFERSPROC)gl->getProcAddress(_PROC("glGenBuffersARB"));
#endif

    vbo_available=1;
    update_vbo=0;
    if ( vbo_available )
    {
	    glGenBuffersARB(2, &myvbo[0]);
	    vbo_loaded=0;
	    indice_array=0;
    }
    selectionIndex.setNum(0);
}

SoBrepFaceSet::~SoBrepFaceSet()
{
#ifdef FC_OS_WIN32
#if QT_VERSION < 0x50000
   const QGLContext* gl = QGLContext::currentContext();
#else
    QOpenGLContext* gl = QOpenGLContext::currentContext();
#endif
    if (!gl)
        return;
    PFNGLDELETEBUFFERSARBPROC glDeleteBuffersARB = (PFNGLDELETEBUFFERSARBPROC)gl->getProcAddress(_PROC("glDeleteBuffersARB"));
#endif
   glDeleteBuffersARB(2, &myvbo[0]);
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
    else if (action->getTypeId() == Gui::SoVRMLAction::getClassTypeId()) {
        // update the materialIndex field to match with the number of triangles if needed
        SoState * state = action->getState();
        Binding mbind = this->findMaterialBinding(state);
        if (mbind == PER_PART) {
            const SoLazyElement* mat = SoLazyElement::getInstance(state);
            int numColor = 1;
            int numParts = partIndex.getNum();
            if (mat) {
                numColor = mat->getNumDiffuse();
                if (numColor == numParts) {
                    int count = 0;
                    const int32_t * indices = this->partIndex.getValues(0);
                    for (int i=0; i<numParts; i++) {
                        count += indices[i];
                    }
                    this->materialIndex.setNum(count);
                    int32_t * matind = this->materialIndex.startEditing();
                    int32_t k = 0;
                    for (int i=0; i<numParts; i++) {
                        for (int j=0; j<indices[i]; j++) {
                            matind[k++] = i;
                        }
                    }
                    this->materialIndex.finishEditing();
                }
            }
        }
    }

    inherited::doAction(action);
}

#ifdef RENDER_GLARRAYS
void SoBrepFaceSet::GLRender(SoGLRenderAction *action)
{
    SoState * state = action->getState();

    SoMaterialBundle mb(action);
    Binding mbind = this->findMaterialBinding(state);

    SoTextureCoordinateBundle tb(action, true, false);
    SbBool doTextures = tb.needCoordinates();
    int32_t hl_idx = this->highlightIndex.getValue();
    int32_t num_selected = this->selectionIndex.getNum();

    if (this->coordIndex.getNum() < 3)
        return;
    if (num_selected > 0)
        renderSelection(action);
    if (hl_idx >= 0)
        renderHighlight(action);

    // When setting transparency shouldGLRender() handles the rendering and returns false.
    // Therefore generatePrimitives() needs to be re-implemented to handle the materials
    // correctly.
    if (!this->shouldGLRender(action))
        return;

#ifdef RENDER_GLARRAYS
    if (!doTextures && index_array.size() && hl_idx < 0 && num_selected <= 0) {
        if (mbind == 0) {
            mb.sendFirst(); // only one material -> apply it!
            renderSimpleArray();
            return;
        }
        else if (mbind == 1) {
            renderColoredArray(&mb);
            return;
        }
    }
#endif

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
    SbBool normalCacheUsed;

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
    if (hl_idx >= 0)
        renderHighlight(action);
    if (num_selected > 0)
        renderSelection(action);
//#endif
    
    if(normalCacheUsed)
        this->readUnlockNormalCache();
}

//****************************************************************************
// renderSimpleArray: normal and coord from vertex_array;
// no texture, color, highlight or selection but highet possible speed;
// all vertices written in one go!
//
void SoBrepFaceSet::renderSimpleArray()
{
    int cnt = index_array.size();
    if (cnt == 0) return;

    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
#if 0
    glInterleavedArrays(GL_N3F_V3F, 0, vertex_array.data());
    glDrawElements(GL_TRIANGLES, cnt, GL_UNSIGNED_INT, index_array.data());
#else
    glInterleavedArrays(GL_N3F_V3F, 0, &(vertex_array[0]));
    glDrawElements(GL_TRIANGLES, cnt, GL_UNSIGNED_INT, &(index_array[0]));
#endif

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

//****************************************************************************
// renderColoredArray: normal and coord from vertex_array;
// no texture, highlight or selection but color / material array.
// needs to iterate over parts (i.e. geometry faces)
//
void SoBrepFaceSet::renderColoredArray(SoMaterialBundle *const materials)
{
    int num_parts = partIndex.getNum();
    int cnt = index_array.size();
    if (cnt == 0) return;

    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

#if 0
    glInterleavedArrays(GL_N3F_V3F, 0, vertex_array.data());
    const int32_t* ptr = index_array.data();
#else
    glInterleavedArrays(GL_N3F_V3F, 0, &(vertex_array[0]));
    const int32_t* ptr = &(index_array[0]);
#endif

    for (int part_id = 0; part_id < num_parts; part_id++) {
        int tris = partIndex[part_id];

        if (tris > 0) {
            materials->send(part_id, true);
            glDrawElements(GL_TRIANGLES, 3 * tris, GL_UNSIGNED_INT, ptr);
            ptr += 3 * tris;
        }
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}
#else
void SoBrepFaceSet::GLRender(SoGLRenderAction *action)
{
    if (this->coordIndex.getNum() < 3)
        return;
    if (this->selectionIndex.getNum() > 0)
        renderSelection(action);
    if (this->highlightIndex.getValue() >= 0)
        renderHighlight(action);
    // When setting transparency shouldGLRender() handles the rendering and returns false.
    // Therefore generatePrimitives() needs to be re-implemented to handle the materials
    // correctly.
    if (!this->shouldGLRender(action))
        return;

    SoState * state = action->getState();
    current_state=state;


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
    SbColor mycolor1;


    SoMaterialBundle mb(action);

    SoTextureCoordinateBundle tb(action, true, false);
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
//    update_vbo=1;
    // Disable caching for this node
    SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);

    if(normalCacheUsed)
        this->readUnlockNormalCache();
        
    // Workaround for #0000433
//#if !defined(FC_OS_WIN32)
    if (this->highlightIndex.getValue() >= 0)
        renderHighlight(action);
    if (this->selectionIndex.getNum() > 0)
        renderSelection(action);
//#endif
}
#endif

void SoBrepFaceSet::GLRenderBelowPath(SoGLRenderAction * action)
{
    inherited::GLRenderBelowPath(action);
}

  // this macro actually makes the code below more readable  :-)
#define DO_VERTEX(idx) \
  if (mbind == PER_VERTEX) {                  \
    pointDetail.setMaterialIndex(matnr);      \
    vertex.setMaterialIndex(matnr++);         \
  }                                           \
  else if (mbind == PER_VERTEX_INDEXED) {     \
    pointDetail.setMaterialIndex(*mindices); \
    vertex.setMaterialIndex(*mindices++); \
  }                                         \
  if (nbind == PER_VERTEX) {                \
    pointDetail.setNormalIndex(normnr);     \
    currnormal = &normals[normnr++];        \
    vertex.setNormal(*currnormal);          \
  }                                         \
  else if (nbind == PER_VERTEX_INDEXED) {   \
    pointDetail.setNormalIndex(*nindices);  \
    currnormal = &normals[*nindices++];     \
    vertex.setNormal(*currnormal);          \
  }                                        \
  if (tb.isFunction()) {                 \
    vertex.setTextureCoords(tb.get(coords->get3(idx), *currnormal)); \
    if (tb.needIndices()) pointDetail.setTextureCoordIndex(tindices ? *tindices++ : texidx++); \
  }                                         \
  else if (tbind != NONE) {                      \
    pointDetail.setTextureCoordIndex(tindices ? *tindices : texidx); \
    vertex.setTextureCoords(tb.get(tindices ? *tindices++ : texidx++)); \
  }                                         \
  vertex.setPoint(coords->get3(idx));        \
  pointDetail.setCoordinateIndex(idx);      \
  this->shapeVertex(&vertex);

void SoBrepFaceSet::generatePrimitives(SoAction * action)
{
    //TODO
#if 0
    inherited::generatePrimitives(action);
#else
    //This is highly experimental!!!

    if (this->coordIndex.getNum() < 3) return;
    SoState * state = action->getState();

    if (this->vertexProperty.getValue()) {
        state->push();
        this->vertexProperty.getValue()->doAction(action);
    }

    Binding mbind = this->findMaterialBinding(state);
    Binding nbind = this->findNormalBinding(state);

    const SoCoordinateElement * coords;
    const SbVec3f * normals;
    const int32_t * cindices;
    int numindices;
    const int32_t * nindices;
    const int32_t * tindices;
    const int32_t * mindices;
    SbBool doTextures;
    SbBool sendNormals;
    SbBool normalCacheUsed;

    sendNormals = true; // always generate normals

    this->getVertexData(state, coords, normals, cindices,
                        nindices, tindices, mindices, numindices,
                        sendNormals, normalCacheUsed);

    SoTextureCoordinateBundle tb(action, false, false);
    doTextures = tb.needCoordinates();

    if (!sendNormals) nbind = OVERALL;
    else if (normalCacheUsed && nbind == PER_VERTEX) {
        nbind = PER_VERTEX_INDEXED;
    }
    else if (normalCacheUsed && nbind == PER_FACE_INDEXED) {
        nbind = PER_FACE;
    }

    if (this->getNodeType() == SoNode::VRML1) {
        // For VRML1, PER_VERTEX means per vertex in shape, not PER_VERTEX
        // on the state.
        if (mbind == PER_VERTEX) {
            mbind = PER_VERTEX_INDEXED;
            mindices = cindices;
        }
        if (nbind == PER_VERTEX) {
            nbind = PER_VERTEX_INDEXED;
            nindices = cindices;
        }
    }

    Binding tbind = NONE;
    if (doTextures) {
        if (tb.isFunction() && !tb.needIndices()) {
            tbind = NONE;
            tindices = NULL;
        }
        // FIXME: just call inherited::areTexCoordsIndexed() instead of
        // the if-check? 20020110 mortene.
        else if (SoTextureCoordinateBindingElement::get(state) ==
                 SoTextureCoordinateBindingElement::PER_VERTEX) {
            tbind = PER_VERTEX;
            tindices = NULL;
        }
        else {
            tbind = PER_VERTEX_INDEXED;
            if (tindices == NULL) tindices = cindices;
        }
    }

    if (nbind == PER_VERTEX_INDEXED && nindices == NULL) {
        nindices = cindices;
    }
    if (mbind == PER_VERTEX_INDEXED && mindices == NULL) {
        mindices = cindices;
    }

    int texidx = 0;
    TriangleShape mode = POLYGON;
    TriangleShape newmode;
    const int32_t *viptr = cindices;
    const int32_t *viendptr = viptr + numindices;
    const int32_t *piptr = this->partIndex.getValues(0);
    int num_partindices = this->partIndex.getNum();
    const int32_t *piendptr = piptr + num_partindices;
    int32_t v1, v2, v3, v4, v5 = 0, pi; // v5 init unnecessary, but kills a compiler warning.

    SoPrimitiveVertex vertex;
    SoPointDetail pointDetail;
    SoFaceDetail faceDetail;

    vertex.setDetail(&pointDetail);

    SbVec3f dummynormal(0,0,1);
    const SbVec3f *currnormal = &dummynormal;
    if (normals) currnormal = normals;
    vertex.setNormal(*currnormal);

    int matnr = 0;
    int normnr = 0;
    int trinr = 0;
    pi = piptr < piendptr ? *piptr++ : -1;
    while (pi == 0) {
        // It may happen that a part has no triangles
        pi = piptr < piendptr ? *piptr++ : -1;
        if (mbind == PER_PART)
            matnr++;
        else if (mbind == PER_PART_INDEXED)
            mindices++;
    }

    while (viptr + 2 < viendptr) {
        v1 = *viptr++;
        v2 = *viptr++;
        v3 = *viptr++;
        if (v1 < 0 || v2 < 0 || v3 < 0) {
            break;
        }
        v4 = viptr < viendptr ? *viptr++ : -1;
        if (v4  < 0) newmode = TRIANGLES;
        else {
            v5 = viptr < viendptr ? *viptr++ : -1;
            if (v5 < 0) newmode = QUADS;
            else newmode = POLYGON;
        }
        if (newmode != mode) {
            if (mode != POLYGON) this->endShape();
            mode = newmode;
            this->beginShape(action, mode, &faceDetail);
        }
        else if (mode == POLYGON) this->beginShape(action, POLYGON, &faceDetail);

        // vertex 1 can't use DO_VERTEX
        if (mbind == PER_PART) {
            if (trinr == 0) {
                pointDetail.setMaterialIndex(matnr);
                vertex.setMaterialIndex(matnr++);
            }
        }
        else if (mbind == PER_PART_INDEXED) {
            if (trinr == 0) {
                pointDetail.setMaterialIndex(*mindices);
                vertex.setMaterialIndex(*mindices++);
            }
        }
        else if (mbind == PER_VERTEX || mbind == PER_FACE) {
            pointDetail.setMaterialIndex(matnr);
            vertex.setMaterialIndex(matnr++);
        }
        else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
            pointDetail.setMaterialIndex(*mindices);
            vertex.setMaterialIndex(*mindices++);
        }
        if (nbind == PER_VERTEX || nbind == PER_FACE) {
            pointDetail.setNormalIndex(normnr);
            currnormal = &normals[normnr++];
            vertex.setNormal(*currnormal);
        }
        else if (nbind == PER_FACE_INDEXED || nbind == PER_VERTEX_INDEXED) {
            pointDetail.setNormalIndex(*nindices);
            currnormal = &normals[*nindices++];
            vertex.setNormal(*currnormal);
        }

        if (tb.isFunction()) {
            vertex.setTextureCoords(tb.get(coords->get3(v1), *currnormal));
            if (tb.needIndices()) pointDetail.setTextureCoordIndex(tindices ? *tindices++ : texidx++); 
        }
        else if (tbind != NONE) {
            pointDetail.setTextureCoordIndex(tindices ? *tindices : texidx);
            vertex.setTextureCoords(tb.get(tindices ? *tindices++ : texidx++));
        }
        pointDetail.setCoordinateIndex(v1);
        vertex.setPoint(coords->get3(v1));
        this->shapeVertex(&vertex);

        DO_VERTEX(v2);
        DO_VERTEX(v3);

        if (mode != TRIANGLES) {
            DO_VERTEX(v4);
            if (mode == POLYGON) {
                DO_VERTEX(v5);
                v1 = viptr < viendptr ? *viptr++ : -1;
                while (v1 >= 0) {
                    DO_VERTEX(v1);
                    v1 = viptr < viendptr ? *viptr++ : -1;
                }
                this->endShape();
            }
        }
        faceDetail.incFaceIndex();
        if (mbind == PER_VERTEX_INDEXED) {
            mindices++;
        }
        if (nbind == PER_VERTEX_INDEXED) {
            nindices++;
        }
        if (tindices) tindices++;

        trinr++;
        if (pi == trinr) {
            pi = piptr < piendptr ? *piptr++ : -1;
            while (pi == 0) {
                // It may happen that a part has no triangles
                pi = piptr < piendptr ? *piptr++ : -1;
                if (mbind == PER_PART)
                    matnr++;
                else if (mbind == PER_PART_INDEXED)
                    mindices++;
            }
            trinr = 0;
        }
    }
    if (mode != POLYGON) this->endShape();

    if (normalCacheUsed) {
        this->readUnlockNormalCache();
    }

    if (this->vertexProperty.getValue()) {
        state->pop();
    }
#endif
}

#undef DO_VERTEX

void SoBrepFaceSet::renderHighlight(SoGLRenderAction *action)
{
    SoState * state = action->getState();
    state->push();

    SoLazyElement::setEmissive(state, &this->highlightColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, true);
#if 0 // disables shading effect
    // sendNormals will be false
    SoLazyElement::setDiffuse(state, this,1, &this->highlightColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, true);
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
    SoTextureCoordinateBundle tb(action, true, false);
    doTextures = tb.needCoordinates();
    SbBool sendNormals = !mb.isColorOnly() || tb.isFunction();

    this->getVertexData(state, coords, normals, cindices,
                        nindices, tindices, mindices, numindices,
                        sendNormals, normalCacheUsed);

    mb.sendFirst(); // make sure we have the correct material

    int32_t id = this->highlightIndex.getValue();
    if (id >= this->partIndex.getNum()) {
        SoDebugError::postWarning("SoBrepFaceSet::renderHighlight", "highlightIndex out of range");
    }
    else {
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
        doTextures = false;
	vbo_available=0;
	// update_vbo=1;
        renderShape(static_cast<const SoGLCoordinateElement*>(coords), &(cindices[start]), length,
            &(pindices[id]), 1, normals, nindices, &mb, mindices, &tb, tindices, nbind, mbind, doTextures?1:0);
	vbo_available=1;
    }
    state->pop();
    
    if(normalCacheUsed)
        this->readUnlockNormalCache();
}

void SoBrepFaceSet::renderSelection(SoGLRenderAction *action)
{
    int numSelected =  this->selectionIndex.getNum();
    const int32_t* selected = this->selectionIndex.getValues(0);
    if (numSelected == 0) return;

    SoState * state = action->getState();
    state->push();

    SoLazyElement::setEmissive(state, &this->selectionColor);
    SoOverrideElement::setEmissiveColorOverride(state, this, true);
#if 0 // disables shading effect
    SoLazyElement::setDiffuse(state, this,1, &this->selectionColor,&this->colorpacker);
    SoOverrideElement::setDiffuseColorOverride(state, this, true);
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
    SoTextureCoordinateBundle tb(action, true, false);
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
    doTextures = false;

    for (int i=0; i<numSelected; i++) {
        int id = selected[i];
        if (id >= this->partIndex.getNum()) {
            SoDebugError::postWarning("SoBrepFaceSet::renderSelection", "selectionIndex out of range");
            break;
        }

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
	vbo_available=0;
        renderShape(static_cast<const SoGLCoordinateElement*>(coords), &(cindices[start]), length,
            &(pindices[id]), 1, normals_s, nindices_s, &mb, mindices, &tb, tindices, nbind, mbind, doTextures?1:0);
	vbo_available=1;
    }
    state->pop();
    
    if(normalCacheUsed)
        this->readUnlockNormalCache();
}

void SoBrepFaceSet::renderShape(const SoGLCoordinateElement * const vertexlist,
                                const int32_t *vertexindices,
                                int num_indices,
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
    SbVec3f * cur_coords3d = NULL;
    SbColor  mycolor1,mycolor2,mycolor3;
    coords3d = vertexlist->getArrayPtr3();
    cur_coords3d = ( SbVec3f *)coords3d;

    const int32_t *viptr = vertexindices;
    const int32_t *viendptr = viptr + num_indices;
    const int32_t *piptr = partindices;
    const int32_t *piendptr = piptr + num_partindices;
    int32_t v1, v2, v3, v4, pi;
    SbVec3f dummynormal(0,0,1);
    int numverts = vertexlist->getNum();

    const SbVec3f *currnormal = &dummynormal;
    if (normals) currnormal = normals;

    int matnr = 0;
    int trinr = 0;

    

    /* This code detect if the user activated VBO through the preference menu */
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view;
    if ( doc != NULL )
	  view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    else
          view = NULL;
    bool ViewerVBO=false;
    if ( view != NULL )
    {
	    Gui::View3DInventorViewer* viewer = view->getViewer();
	    ViewerVBO=viewer->get_vbo_state();
    }

    /*
	vbo_available is used to determine if vbo is an avilable extension on the system .
	This is not because end user wants VBO that it is available.

    */

    if (( vbo_available ) && ViewerVBO )
    {
	    float * vertex_array = NULL;
	    GLuint * index_array = NULL;
	    SbVec3f *mynormal1,*mynormal2,*mynormal3;
	    int indice=0;
	    uint32_t RGBA,R,G,B,A;
	    float Rf,Gf,Bf,Af; 

	// vbo loaded is defining if we must pre-load data into the VBO. When the variable is set to 0
	// it means that the VBO has not been initialized 
	// update_vbo is tracking the need to update the content of the VBO which act as a buffer within
	// the graphic card
	// TODO FINISHING THE COLOR SUPPORT !

	    if (( vbo_loaded == 0 ) || update_vbo )
	    {
		    if ( ( update_vbo ) && vbo_loaded )
		    {
			// TODO
			// We must remember the buffer size ... If it has to be extended we must
			// take care of that
	
#ifdef FC_OS_WIN32
#if QT_VERSION < 0x50000
                const QGLContext* gl = QGLContext::currentContext();
#else
                QOpenGLContext* gl = QOpenGLContext::currentContext();
#endif
                PFNGLBINDBUFFERARBPROC glBindBufferARB = (PFNGLBINDBUFFERARBPROC) gl->getProcAddress(_PROC("glBindBufferARB"));
                PFNGLMAPBUFFERARBPROC glMapBufferARB = (PFNGLMAPBUFFERARBPROC) gl->getProcAddress(_PROC("glMapBufferARB"));
#endif
                glBindBufferARB(GL_ARRAY_BUFFER_ARB, myvbo[0]);
		        glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, myvbo[1]);
			vertex_array=(float*)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
			index_array=(GLuint *)glMapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);
			indice_array=0;
		    }
		    else
		    {
			// We are allocating local buffer to transfer initial VBO content
			    vertex_array = ( float * ) malloc ( sizeof(float) * num_indices *10 );
			    index_array = ( GLuint *) malloc ( sizeof(GLuint) * num_indices *3 );
		    }
	
		    // Get the initial colors
            	    mycolor1=SoLazyElement::getDiffuse(current_state,0);
	            mycolor2=SoLazyElement::getDiffuse(current_state,0);
	            mycolor3=SoLazyElement::getDiffuse(current_state,0);

	            pi = piptr < piendptr ? *piptr++ : -1;
	            while (pi == 0) {
		           // It may happen that a part has no triangles
		               pi = piptr < piendptr ? *piptr++ : -1;
		               if (mbind == PER_PART)
		                   matnr++;
		               else if (mbind == PER_PART_INDEXED)
		                   matindices++;
            	    }
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
		        (void)v4;

			if (mbind == PER_PART) {
		            if (trinr == 0)
		            {
		                materials->send(matnr++, true);
				mycolor1=SoLazyElement::getDiffuse(current_state,matnr-1);
				mycolor2=mycolor1;
				mycolor3=mycolor1;
			    }
	        	}
		        else if (mbind == PER_PART_INDEXED) {
		            if (trinr == 0)
		                materials->send(*matindices++, true);
	        	}
		        else if (mbind == PER_VERTEX || mbind == PER_FACE) {
		            materials->send(matnr++, true);
	        	}
		        else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
		            materials->send(*matindices++, true);
	        	}
			if (normals) {
		            if (nbind == PER_VERTEX || nbind == PER_FACE) {
		                currnormal = normals++;
		                mynormal1=(SbVec3f *)currnormal;
		            }
		            else if (nbind == PER_VERTEX_INDEXED || nbind == PER_FACE_INDEXED) {
		                currnormal = &normals[*normalindices++];
		                mynormal1 =(SbVec3f *) currnormal;
		            }
		        }
			if (mbind == PER_VERTEX)
		            materials->send(matnr++, true);
        		else if (mbind == PER_VERTEX_INDEXED)
		            materials->send(*matindices++, true);
			if (normals) {
		            if (nbind == PER_VERTEX) {
		                currnormal = normals++;
		                mynormal2 = (SbVec3f *)currnormal;
            		    }
		            else if (nbind == PER_VERTEX_INDEXED) {
               			 currnormal = &normals[*normalindices++];
		                mynormal2 = (SbVec3f *)currnormal;
            		    	}
        		}
			if (mbind == PER_VERTEX)
		            materials->send(matnr++, true);
		        else if (mbind == PER_VERTEX_INDEXED)
		            materials->send(*matindices++, true);
			if (normals) {
		            if (nbind == PER_VERTEX) {
		                currnormal = normals++;
		                mynormal3 =(SbVec3f *)currnormal;
		            }
		            else if (nbind == PER_VERTEX_INDEXED) {
		                currnormal = &normals[*normalindices++];
		                mynormal3 = (SbVec3f *)currnormal;
		            }
        		}
			if (nbind == PER_VERTEX_INDEXED)
            			normalindices++;

			/* We building the Vertex dataset there and push it to a VBO */
			/* The Vertex array shall contain per element vertex_coordinates[3], 
			   normal_coordinates[3], color_value[3] (RGBA format) */

		        index_array[indice_array] = indice_array;
		        index_array[indice_array+1] = indice_array+1;
		        index_array[indice_array+2] = indice_array+2;
		        indice_array+=3;	
 
			((SbVec3f *)(cur_coords3d+v1 ))->getValue(vertex_array[indice+0], 
								  vertex_array[indice+1],
								  vertex_array[indice+2]);
			((SbVec3f *)(mynormal1))->getValue(vertex_array[indice+3], 
							   vertex_array[indice+4],
							   vertex_array[indice+5]);
		
			/* We decode the Vertex1 color */

			RGBA = mycolor1.getPackedValue();
			R = ( RGBA & 0xFF000000 ) >> 24 ;
	                G = ( RGBA & 0xFF0000 ) >> 16;
	                B = ( RGBA & 0xFF00 ) >> 8;
	                A = ( RGBA & 0xFF );

			Rf = (((float )R) / 255.0);
			Gf = (((float )G) / 255.0);
			Bf = (((float )B) / 255.0);
			Af = (((float )A) / 255.0);
	
			vertex_array[indice+6] = Rf;
			vertex_array[indice+7] = Gf;
			vertex_array[indice+8] = Bf;
			vertex_array[indice+9] = Af;
			indice+=10;

			((SbVec3f *)(cur_coords3d+v2))->getValue(vertex_array[indice+0], 
								 vertex_array[indice+1],
								 vertex_array[indice+2]);
			((SbVec3f *)(mynormal2))->getValue(vertex_array[indice+3], 
							   vertex_array[indice+4],
							   vertex_array[indice+5]);

			RGBA = mycolor2.getPackedValue();
			R = ( RGBA & 0xFF000000 ) >> 24 ;
	                G = ( RGBA & 0xFF0000 ) >> 16;
	                B = ( RGBA & 0xFF00 ) >> 8;
	                A = ( RGBA & 0xFF );


	                Rf = (((float )R) / 255.0);
	                Gf = (((float )G) / 255.0);
	                Bf = (((float )B) / 255.0);
	                Af = (((float )A) / 255.0);


	     	        vertex_array[indice+6] = Rf;
	                vertex_array[indice+7] = Gf;
	                vertex_array[indice+8] = Bf;
	                vertex_array[indice+9] = Af;
	                indice+=10;

			((SbVec3f *)(cur_coords3d+v3))->getValue(vertex_array[indice+0], 
								 vertex_array[indice+1],
								 vertex_array[indice+2]);
			((SbVec3f *)(mynormal3))->getValue(vertex_array[indice+3], 
							   vertex_array[indice+4],
							   vertex_array[indice+5]);

			RGBA = mycolor3.getPackedValue();
	                R = ( RGBA & 0xFF000000 ) >> 24 ;
	                G = ( RGBA & 0xFF0000 ) >> 16;
	                B = ( RGBA & 0xFF00 ) >> 8;
	                A = ( RGBA & 0xFF );

	                Rf = (((float )R) / 255.0);
	                Gf = (((float )G) / 255.0);
	                Bf = (((float )B) / 255.0);
	                Af = (((float )A) / 255.0);

	                vertex_array[indice+6] = Rf;
	                vertex_array[indice+7] = Gf;
	                vertex_array[indice+8] = Bf;
	                vertex_array[indice+9] = Af;
	                indice+=10;

			/* ============================================================ */
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
		if ((  ! update_vbo ) || (!vbo_loaded) )
		{
			// Push the content to the VBO
#ifdef FC_OS_WIN32
#if QT_VERSION < 0x50000
            const QGLContext* gl = QGLContext::currentContext();
#else
            QOpenGLContext* gl = QOpenGLContext::currentContext();
#endif
            PFNGLBINDBUFFERARBPROC glBindBufferARB = (PFNGLBINDBUFFERARBPROC)gl->getProcAddress(_PROC("glBindBufferARB"));
            PFNGLBUFFERDATAARBPROC glBufferDataARB = (PFNGLBUFFERDATAARBPROC)gl->getProcAddress(_PROC("glBufferDataARB"));
#endif

		        glBindBufferARB(GL_ARRAY_BUFFER_ARB, myvbo[0]);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof(float) * indice , vertex_array, GL_DYNAMIC_DRAW_ARB);

			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, myvbo[1]);
		        glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, sizeof(GLuint) * indice_array , &index_array[0], GL_DYNAMIC_DRAW_ARB);

			glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
			glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

			vbo_loaded=1;
			update_vbo=0;
			free(vertex_array);
			free(index_array);
		}
		else
		{
#ifdef FC_OS_WIN32
#if QT_VERSION < 0x50000
            const QGLContext* gl = QGLContext::currentContext();
#else
            QOpenGLContext* gl = QOpenGLContext::currentContext();
#endif
            PFNGLUNMAPBUFFERARBPROC glUnmapBufferARB = (PFNGLUNMAPBUFFERARBPROC)gl->getProcAddress(_PROC("glUnmapBufferARB"));
#endif
            glUnmapBufferARB(GL_ARRAY_BUFFER_ARB);
			glUnmapBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB);
			update_vbo=0;
		}
    	}

	// This is the VBO rendering code
#ifdef FC_OS_WIN32
#if QT_VERSION < 0x50000
        const QGLContext* gl = QGLContext::currentContext();
#else
        QOpenGLContext* gl = QOpenGLContext::currentContext();
#endif
   PFNGLBINDBUFFERARBPROC glBindBufferARB = (PFNGLBINDBUFFERARBPROC)gl->getProcAddress(_PROC("glBindBufferARB"));
#endif

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, myvbo[0]);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, myvbo[1]);
	glEnableClientState(GL_VERTEX_ARRAY); 
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);


	glVertexPointer(3,GL_FLOAT,10*sizeof(GLfloat),0);
	glNormalPointer(GL_FLOAT,10*sizeof(GLfloat),(GLvoid *)(3*sizeof(GLfloat)));
	glColorPointer(4,GL_FLOAT,10*sizeof(GLfloat),(GLvoid *)(6*sizeof(GLfloat)));   

	glDrawElements(GL_TRIANGLES, indice_array, GL_UNSIGNED_INT, (void *)0); 

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	    // The data is within the VBO we can clear it at application level
    	return;
    }

    // Legacy code without VBO support
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
	if (v1 < 0 || v2 < 0 || v3 < 0 ||
            v1 >= numverts || v2 >= numverts || v3 >= numverts) {
            break;
        }
        v4 = viptr < viendptr ? *viptr++ : -1;
        (void)v4;
        /* vertex 1 *********************************************************/
        if (mbind == PER_PART) {
            if (trinr == 0)
	    {
                materials->send(matnr++, true);
	    }
        }
        else if (mbind == PER_PART_INDEXED) {
            if (trinr == 0)
	    {
                materials->send(*matindices++, true);
	    }
        }
        else if (mbind == PER_VERTEX || mbind == PER_FACE) {
            materials->send(matnr++, true);
        }
        else if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
            materials->send(*matindices++, true);
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
        {
            materials->send(matnr++, true);
        }
        else if (mbind == PER_VERTEX_INDEXED)
        {
            materials->send(*matindices++, true);
        }

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
        {
            materials->send(matnr++, true);
        }
        else if (mbind == PER_VERTEX_INDEXED)
        {
            materials->send(*matindices++, true);
        }

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
