/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <algorithm>
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
# else
# include <GL/gl.h>
# include <GL/glu.h>
# endif
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/bundles/SoMaterialBundle.h>
# include <Inventor/elements/SoCoordinateElement.h>
# include <Inventor/elements/SoGLCoordinateElement.h>
# include <Inventor/elements/SoMaterialBindingElement.h>
# include <Inventor/elements/SoProjectionMatrixElement.h>
# include <Inventor/elements/SoViewingMatrixElement.h>
#endif

#include <Gui/SoFCInteractiveElement.h>
#include <Gui/SoFCSelectionAction.h>
#include "SoFCIndexedFaceSet.h"

using namespace MeshGui;


SO_NODE_SOURCE(SoFCIndexedFaceSet);

void SoFCIndexedFaceSet::initClass()
{
    SO_NODE_INIT_CLASS(SoFCIndexedFaceSet, SoIndexedFaceSet, "IndexedFaceSet");
}

SoFCIndexedFaceSet::SoFCIndexedFaceSet() : renderTriangleLimit(100000), selectBuf(0)
{
    SO_NODE_CONSTRUCTOR(SoFCIndexedFaceSet);
    setName(SoFCIndexedFaceSet::getClassTypeId().getName());
}

/**
 * Either renders the complete mesh or only a subset of the points.
 */
void SoFCIndexedFaceSet::GLRender(SoGLRenderAction *action)
{
    if (this->coordIndex.getNum() < 3)
        return;

    if (!this->shouldGLRender(action))
        return;

    SoState * state = action->getState();
    SbBool mode = Gui::SoFCInteractiveElement::get(state);

    unsigned int num = this->coordIndex.getNum()/4;
    if (mode == false || num <= this->renderTriangleLimit) {
        inherited::GLRender(action);
    }
    else {
        SoMaterialBindingElement::Binding matbind =
            SoMaterialBindingElement::get(state);
        int32_t binding = (int32_t)(matbind);

        const SoCoordinateElement * coords;
        const SbVec3f * normals;
        const int32_t * cindices;
        int numindices;
        const int32_t * nindices;
        const int32_t * tindices;
        const int32_t * mindices;
        SbBool normalCacheUsed;

        SoMaterialBundle mb(action);

        SoTextureCoordinateBundle tb(action, TRUE, FALSE);
        SbBool sendNormals = !mb.isColorOnly() || tb.isFunction();

        this->getVertexData(state, coords, normals, cindices,
                            nindices, tindices, mindices, numindices,
                            sendNormals, normalCacheUsed);

        mb.sendFirst(); // make sure we have the correct material

        drawCoords(static_cast<const SoGLCoordinateElement*>(coords), cindices, numindices,
                   normals, nindices, &mb, mindices, binding, &tb, tindices);
        // Disable caching for this node
        SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
    }
}

void SoFCIndexedFaceSet::drawCoords(const SoGLCoordinateElement * const vertexlist,
                                    const int32_t *vertexindices,
                                    int numindices,
                                    const SbVec3f *normals,
                                    const int32_t *normalindices,
                                    SoMaterialBundle *materials,
                                    const int32_t *matindices,
                                    const int32_t binding,
                                    const SoTextureCoordinateBundle * const texcoords,
                                    const int32_t *texindices)
{
    const SbVec3f * coords3d = 0;
    coords3d = vertexlist->getArrayPtr3();

    int mod = numindices/(4*this->renderTriangleLimit)+1;
    float size = std::min<float>((float)mod,3.0f);
    glPointSize(size);

    SbBool per_face = FALSE;
    SbBool per_vert = FALSE;
    switch (binding) {
        case SoMaterialBindingElement::PER_FACE:
            per_face = TRUE;
            break;
        case SoMaterialBindingElement::PER_VERTEX:
            per_vert = TRUE;
            break;
        default:
            break;
    }

    int ct=0;
    const int32_t *viptr = vertexindices;
    int32_t v1, v2, v3;
    SbVec3f dummynormal(0,0,1);
    const SbVec3f *currnormal = &dummynormal;
    if (normals) currnormal = normals;

    glBegin(GL_POINTS);
    for (int index=0; index<numindices; ct++) {
        if (ct%mod==0) {
            if (per_face)
                materials->send(ct, TRUE);
            v1 = *viptr++; index++;
            if (per_vert)
                materials->send(v1, TRUE);
            if (normals)
                currnormal = &normals[*normalindices++];
            glNormal3fv((const GLfloat*)currnormal);
            glVertex3fv((const GLfloat*)(coords3d + v1));

            v2 = *viptr++; index++;
            if (per_vert)
                materials->send(v2, TRUE);
            if (normals)
                currnormal = &normals[*normalindices++];
            glNormal3fv((const GLfloat*)currnormal);
            glVertex3fv((const GLfloat*)(coords3d + v2));

            v3 = *viptr++; index++;
            if (per_vert)
                materials->send(v3, TRUE);
            if (normals)
                currnormal = &normals[*normalindices++];
            glNormal3fv((const GLfloat*)currnormal);
            glVertex3fv((const GLfloat*)(coords3d + v3));
        }
        else {
            viptr++; index++; normalindices++;
            viptr++; index++; normalindices++;
            viptr++; index++; normalindices++;
        }

        viptr++; index++; normalindices++;
    }
    glEnd();
}

void SoFCIndexedFaceSet::doAction(SoAction * action)
{
    if (action->getTypeId() == Gui::SoGLSelectAction::getClassTypeId()) {
        SoNode* node = action->getNodeAppliedTo();
        if (!node) return; // on no node applied

        // The node we have is the parent of this node and the coordinate node
        // thus we search there for it.
        SoSearchAction sa;
        sa.setInterest(SoSearchAction::FIRST);
        sa.setSearchingAll(FALSE);
        sa.setType(SoCoordinate3::getClassTypeId(), 1);
        sa.apply(node);
        SoPath * path = sa.getPath();
        if (!path) return;

        // make sure we got the node we wanted
        SoNode* coords = path->getNodeFromTail(0);
        if (!(coords && coords->getTypeId().isDerivedFrom(SoCoordinate3::getClassTypeId())))
            return;
        startSelection(action);
        renderSelectionGeometry(static_cast<SoCoordinate3*>(coords)->point.getValues(0));
        stopSelection(action);
    }
    else if (action->getTypeId() == Gui::SoVisibleFaceAction::getClassTypeId()) {
        SoNode* node = action->getNodeAppliedTo();
        if (!node) return; // on no node applied

        // The node we have is the parent of this node and the coordinate node
        // thus we search there for it.
        SoSearchAction sa;
        sa.setInterest(SoSearchAction::FIRST);
        sa.setSearchingAll(FALSE);
        sa.setType(SoCoordinate3::getClassTypeId(), 1);
        sa.apply(node);
        SoPath * path = sa.getPath();
        if (!path) return;

        // make sure we got the node we wanted
        SoNode* coords = path->getNodeFromTail(0);
        if (!(coords && coords->getTypeId().isDerivedFrom(SoCoordinate3::getClassTypeId())))
            return;
        startVisibility(action);
        renderVisibleFaces(static_cast<SoCoordinate3*>(coords)->point.getValues(0));
        stopVisibility(action);
    }

    inherited::doAction(action);
}

void SoFCIndexedFaceSet::startSelection(SoAction * action)
{
    Gui::SoGLSelectAction *doaction = static_cast<Gui::SoGLSelectAction*>(action);
    const SbViewportRegion& vp = doaction->getViewportRegion();
    int x = vp.getViewportOriginPixels()[0];
    int y = vp.getViewportOriginPixels()[1];
    int w = vp.getViewportSizePixels()[0];
    int h = vp.getViewportSizePixels()[1];

    int bufSize = 5*(this->coordIndex.getNum()/4); // make the buffer big enough
    this->selectBuf = new GLuint[bufSize];

    SbMatrix view = SoViewingMatrixElement::get(action->getState());
    SbMatrix proj = SoProjectionMatrixElement::get(action->getState());

    glSelectBuffer(bufSize, selectBuf);
    glRenderMode(GL_SELECT);

    glInitNames();
    glPushName(-1);

    //double mp[16];
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);
    glMatrixMode(GL_PROJECTION);
    //glGetDoublev(GL_PROJECTION_MATRIX ,mp);
    glPushMatrix();
    glLoadIdentity();
    gluPickMatrix(x, y, w, h, viewport);
    glMultMatrixf(/*mp*/(float*)proj);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf((float*)view);
}

void SoFCIndexedFaceSet::stopSelection(SoAction * action)
{
    // restoring the original projection matrix
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glFlush();

    // returning to normal rendering mode
    GLint hits = glRenderMode(GL_RENDER);

    int bufSize = 5*(this->coordIndex.getNum()/4);
    std::vector< std::pair<double,unsigned int> > hit;
    GLint index=0;
    for (GLint ii=0;ii<hits && index<bufSize;ii++) {
        GLint ct = (GLint)selectBuf[index];
        hit.push_back(std::pair<double,unsigned int>
            (selectBuf[index+1]/4294967295.0,selectBuf[index+3]));
        index = index+ct+3;
    }

    delete [] selectBuf;
    selectBuf = 0;
    bool sorted = true;
    if(sorted) std::sort(hit.begin(),hit.end());

    Gui::SoGLSelectAction *doaction = static_cast<Gui::SoGLSelectAction*>(action);
    doaction->indices.reserve(hit.size());
    for (GLint ii=0;ii<hits;ii++) {
        doaction->indices.push_back(hit[ii].second);
    }
}

void SoFCIndexedFaceSet::renderSelectionGeometry(const SbVec3f * coords3d)
{
    int numfaces = this->coordIndex.getNum()/4;
    const int32_t * cindices = this->coordIndex.getValues(0);

    int fcnt=0;
    int32_t v1, v2, v3;
    for (int index=0; index<numfaces;index++,cindices++) {
        glLoadName(fcnt);
        glBegin(GL_TRIANGLES);
            v1 = *cindices++;
            glVertex3fv((const GLfloat*)(coords3d + v1));
            v2 = *cindices++;
            glVertex3fv((const GLfloat*)(coords3d + v2));
            v3 = *cindices++;
            glVertex3fv((const GLfloat*)(coords3d + v3));
        glEnd();
        fcnt++;
    }
}

void SoFCIndexedFaceSet::startVisibility(SoAction * action)
{
    SbMatrix view = SoViewingMatrixElement::get(action->getState());
    SbMatrix proj = SoProjectionMatrixElement::get(action->getState());

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixf((float*)proj);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf((float*)view);
}

void SoFCIndexedFaceSet::stopVisibility(SoAction * action)
{
    // restoring the original projection matrix
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glFlush();
}

void SoFCIndexedFaceSet::renderVisibleFaces(const SbVec3f * coords3d)
{
    //GLint redBits, greenBits, blueBits;

    //glGetIntegerv (GL_RED_BITS, &redBits);
    //glGetIntegerv (GL_GREEN_BITS, &greenBits);
    //glGetIntegerv (GL_BLUE_BITS, &blueBits);
    glDisable (GL_BLEND);
    glDisable (GL_DITHER);
    glDisable (GL_FOG);
    glDisable (GL_LIGHTING);
    glDisable (GL_TEXTURE_1D);
    glDisable (GL_TEXTURE_2D);
    glShadeModel (GL_FLAT);

    uint32_t numfaces = this->coordIndex.getNum()/4;
    const int32_t * cindices = this->coordIndex.getValues(0);

    int32_t v1, v2, v3;
    for (uint32_t index=0; index<numfaces;index++,cindices++) {
        glBegin(GL_TRIANGLES);
            float t;
            SbColor c;
            c.setPackedValue(index<<8,t);
            glColor3f(c[0],c[1],c[2]);
            v1 = *cindices++;
            glVertex3fv((const GLfloat*)(coords3d + v1));
            v2 = *cindices++;
            glVertex3fv((const GLfloat*)(coords3d + v2));
            v3 = *cindices++;
            glVertex3fv((const GLfloat*)(coords3d + v3));
        glEnd();
    }
}
