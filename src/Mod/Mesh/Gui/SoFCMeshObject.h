/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESHGUI_SOFCMESHOBJECT_H
#define MESHGUI_SOFCMESHOBJECT_H

#include <Inventor/fields/SoSField.h>
#include <Inventor/fields/SoSFUInt32.h>
#include <Inventor/fields/SoSubField.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFVec3s.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/elements/SoReplacedElement.h>
#include <Mod/Mesh/App/Core/Elements.h>
#include <Mod/Mesh/App/Mesh.h>

typedef unsigned int GLuint;
typedef int GLint;
typedef float GLfloat;

namespace MeshCore { class MeshFacetGrid; }

namespace MeshGui {

class MeshGuiExport SoSFMeshObject : public SoSField {
    typedef SoSField inherited;

    SO_SFIELD_HEADER(SoSFMeshObject, const Mesh::MeshObject*, const Mesh::MeshObject*);

public:
    static void initClass(void);

private:
    SoSFMeshObject(const SoSFMeshObject&);
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshObjectElement : public SoReplacedElement {
    typedef SoReplacedElement inherited;

    SO_ELEMENT_HEADER(SoFCMeshObjectElement);

public:
    static void initClass(void);

    virtual void init(SoState * state);
    static void set(SoState * const state, SoNode * const node, const Mesh::MeshObject * const mesh);
    static const Mesh::MeshObject * get(SoState * const state);
    static const SoFCMeshObjectElement * getInstance(SoState * state);
    virtual void print(FILE * file) const;

protected:
    virtual ~SoFCMeshObjectElement();
    const Mesh::MeshObject *mesh;
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshPickNode : public SoNode {
    typedef SoNode inherited;

    SO_NODE_HEADER(SoFCMeshPickNode);

public:
    static void initClass(void);
    SoFCMeshPickNode(void);
    void notify(SoNotList *);

    SoSFMeshObject mesh;

    virtual void rayPick(SoRayPickAction * action);
    virtual void pick(SoPickAction * action);

protected:
    virtual ~SoFCMeshPickNode();

private:
    MeshCore::MeshFacetGrid* meshGrid;
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshGridNode : public SoNode {
    typedef SoNode inherited;

    SO_NODE_HEADER(SoFCMeshGridNode);

public:
    static void initClass(void);
    SoFCMeshGridNode(void);
    void GLRender(SoGLRenderAction * action);

    SoSFVec3f minGrid;
    SoSFVec3f maxGrid;
    SoSFVec3s lenGrid;

protected:
    virtual ~SoFCMeshGridNode();
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshObjectNode : public SoNode {
    typedef SoNode inherited;

    SO_NODE_HEADER(SoFCMeshObjectNode);

public:
    static void initClass(void);
    SoFCMeshObjectNode(void);

    SoSFMeshObject mesh;

    virtual void doAction(SoAction * action);
    virtual void GLRender(SoGLRenderAction * action);
    virtual void callback(SoCallbackAction * action);
    virtual void getBoundingBox(SoGetBoundingBoxAction * action);
    virtual void pick(SoPickAction * action);
    virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

protected:
    virtual ~SoFCMeshObjectNode();
};

/**
 * class SoFCMeshObjectShape
 * \brief The SoFCMeshObjectShape class is designed to render huge meshes.
 *
 * The SoFCMeshObjectShape is an Inventor shape node that is designed to render huge meshes.
 * If the mesh exceeds a certain number of triangles and the user does some intersections
 * (e.g. moving, rotating, zooming, spinning, etc.) with the mesh then the GLRender() method
 * renders only the gravity points of a subset of the triangles.
 * If there is no user interaction with the mesh then all triangles are rendered.
 * The limit of maximum allowed triangles can be specified in \a renderTriangleLimit, the
 * default value is set to 100.000.
 *
 * The GLRender() method checks the status of the SoFCInteractiveElement to decide to be in
 * interactive mode or not.
 * To take advantage of this facility the client programmer must set the status of the
 * SoFCInteractiveElement to \a true if there is a user interaction and set the status to
 * \a false if not. This can be done e.g. in the actualRedraw() method of the viewer.
 *
 * @author Werner Mayer
 */
class MeshGuiExport SoFCMeshObjectShape : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(SoFCMeshObjectShape);

public:
    static void initClass();
    SoFCMeshObjectShape();

    unsigned int renderTriangleLimit;

protected:
    virtual void doAction(SoAction * action);
    virtual void GLRender(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
    virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
    virtual void rayPick (SoRayPickAction *action);
    virtual void generatePrimitives(SoAction *action);
    virtual SoDetail * createTriangleDetail(SoRayPickAction * action,
                                            const SoPrimitiveVertex * v1,
                                            const SoPrimitiveVertex * v2,
                                            const SoPrimitiveVertex * v3,
                                            SoPickedPoint * pp);

private:
    enum Binding {
        OVERALL = 0,
        PER_FACE_INDEXED,
        PER_VERTEX_INDEXED,
        NONE = OVERALL
    };

private:
    // Force using the reference count mechanism.
    virtual ~SoFCMeshObjectShape();
    virtual void notify(SoNotList * list);
    Binding findMaterialBinding(SoState * const state) const;
    // Draw faces
    void drawFaces(const Mesh::MeshObject *, SoMaterialBundle* mb, Binding bind, 
                   SbBool needNormals, SbBool ccw) const;
    void drawPoints(const Mesh::MeshObject *, SbBool needNormals, SbBool ccw) const;
    unsigned int countTriangles(SoAction * action) const;

    void startSelection(SoAction * action, const Mesh::MeshObject*);
    void stopSelection(SoAction * action, const Mesh::MeshObject*);
    void renderSelectionGeometry(const Mesh::MeshObject*);

    void generateGLArrays(SoState * state);
    void renderFacesGLArray(SoGLRenderAction *action);
    void renderCoordsGLArray(SoGLRenderAction *action);

private:
    GLuint *selectBuf;
    GLfloat modelview[16];
    GLfloat projection[16];
    // Vertex array handling
    std::vector<int32_t> index_array;
    std::vector<float> vertex_array;
    SbBool updateGLArray;
};

class MeshGuiExport SoFCMeshSegmentShape : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(SoFCMeshSegmentShape);

public:
    static void initClass();
    SoFCMeshSegmentShape();

    SoSFUInt32 index;
    unsigned int renderTriangleLimit;

protected:
    virtual void GLRender(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
    virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
    virtual void generatePrimitives(SoAction *action);

private:
    enum Binding {
        OVERALL = 0,
        PER_FACE_INDEXED,
        PER_VERTEX_INDEXED,
        NONE = OVERALL
    };

private:
    // Force using the reference count mechanism.
    virtual ~SoFCMeshSegmentShape() {};
    Binding findMaterialBinding(SoState * const state) const;
    // Draw faces
    void drawFaces(const Mesh::MeshObject *, SoMaterialBundle* mb, Binding bind, 
                   SbBool needNormals, SbBool ccw) const;
    void drawPoints(const Mesh::MeshObject *, SbBool needNormals, SbBool ccw) const;
};

class MeshGuiExport SoFCMeshObjectBoundary : public SoShape {
    typedef SoShape inherited;

    SO_NODE_HEADER(SoFCMeshObjectBoundary);

public:
    static void initClass();
    SoFCMeshObjectBoundary();

protected:
    virtual void GLRender(SoGLRenderAction *action);
    virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
    virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
    virtual void generatePrimitives(SoAction *action);
private:
    // Force using the reference count mechanism.
    virtual ~SoFCMeshObjectBoundary() {};
    void drawLines(const Mesh::MeshObject *) const ;
};

} // namespace MeshGui


#endif // MESHGUI_SOFCMESHOBJECT_H

