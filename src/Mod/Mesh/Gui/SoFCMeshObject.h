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

#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/fields/SoSFUInt32.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/fields/SoSFVec3s.h>
#include <Inventor/fields/SoSField.h>
#include <Inventor/nodes/SoShape.h>
#include <Mod/Mesh/App/Mesh.h>


using GLuint = unsigned int;
using GLint = int;
using GLfloat = float;

namespace MeshCore
{
class MeshFacetGrid;
}

namespace MeshGui
{

// NOLINTBEGIN(cppcoreguidelines-special-member-functions,cppcoreguidelines-virtual-class-destructor)
class MeshGuiExport SoSFMeshObject: public SoSField
{
    using inherited = SoSField;

    SO_SFIELD_HEADER(SoSFMeshObject,
                     Base::Reference<const Mesh::MeshObject>,
                     Base::Reference<const Mesh::MeshObject>)

public:
    static void initClass();

    SoSFMeshObject(const SoSFMeshObject&) = delete;
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshObjectElement: public SoReplacedElement
{
    using inherited = SoReplacedElement;

    SO_ELEMENT_HEADER(SoFCMeshObjectElement);

public:
    static void initClass();

    void init(SoState* state) override;
    static void set(SoState* const state, SoNode* const node, const Mesh::MeshObject* const mesh);
    static const Mesh::MeshObject* get(SoState* const state);
    static const SoFCMeshObjectElement* getInstance(SoState* state);
    void print(FILE* file) const override;

protected:
    ~SoFCMeshObjectElement() override;

private:
    const Mesh::MeshObject* mesh {};
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshPickNode: public SoNode
{
    using inherited = SoNode;

    SO_NODE_HEADER(SoFCMeshPickNode);

public:
    static void initClass();
    SoFCMeshPickNode();
    void notify(SoNotList*) override;

    SoSFMeshObject mesh;  // NOLINT

    void rayPick(SoRayPickAction* action) override;
    void pick(SoPickAction* action) override;

protected:
    ~SoFCMeshPickNode() override;

private:
    MeshCore::MeshFacetGrid* meshGrid {nullptr};
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshGridNode: public SoNode
{
    using inherited = SoNode;

    SO_NODE_HEADER(SoFCMeshGridNode);

public:
    static void initClass();
    SoFCMeshGridNode();
    void GLRender(SoGLRenderAction* action) override;

    SoSFVec3f minGrid;
    SoSFVec3f maxGrid;
    SoSFVec3s lenGrid;

protected:
    ~SoFCMeshGridNode() override;
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshObjectNode: public SoNode
{
    using inherited = SoNode;

    SO_NODE_HEADER(SoFCMeshObjectNode);

public:
    static void initClass();
    SoFCMeshObjectNode();

    SoSFMeshObject mesh;

    void doAction(SoAction* action) override;
    void GLRender(SoGLRenderAction* action) override;
    void callback(SoCallbackAction* action) override;
    void getBoundingBox(SoGetBoundingBoxAction* action) override;
    void pick(SoPickAction* action) override;
    void getPrimitiveCount(SoGetPrimitiveCountAction* action) override;

protected:
    ~SoFCMeshObjectNode() override;
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
class MeshGuiExport SoFCMeshObjectShape: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoFCMeshObjectShape);

public:
    static void initClass();
    SoFCMeshObjectShape();

    unsigned int renderTriangleLimit;  // NOLINT

protected:
    void doAction(SoAction* action) override;
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
    void getPrimitiveCount(SoGetPrimitiveCountAction* action) override;
    void rayPick(SoRayPickAction* action) override;
    void generatePrimitives(SoAction* action) override;
    SoDetail* createTriangleDetail(SoRayPickAction* action,
                                   const SoPrimitiveVertex* v1,
                                   const SoPrimitiveVertex* v2,
                                   const SoPrimitiveVertex* v3,
                                   SoPickedPoint* pp) override;
    // Force using the reference count mechanism.
    ~SoFCMeshObjectShape() override;

private:
    enum Binding
    {
        OVERALL = 0,
        PER_FACE_INDEXED,
        PER_VERTEX_INDEXED,
        NONE = OVERALL
    };

private:
    void notify(SoNotList* list) override;
    Binding findMaterialBinding(SoState* const state) const;
    // Draw faces
    void drawFaces(const Mesh::MeshObject*,
                   SoMaterialBundle* mb,
                   Binding bind,
                   SbBool needNormals,
                   SbBool ccw) const;
    void drawPoints(const Mesh::MeshObject*, SbBool needNormals, SbBool ccw) const;
    unsigned int countTriangles(SoAction* action) const;

    void startSelection(SoAction* action, const Mesh::MeshObject*);
    void stopSelection(SoAction* action, const Mesh::MeshObject*);
    void renderSelectionGeometry(const Mesh::MeshObject*);

    void generateGLArrays(SoState* state);
    void renderFacesGLArray(SoGLRenderAction* action);
    void renderCoordsGLArray(SoGLRenderAction* action);

private:
    GLuint* selectBuf {nullptr};
    GLfloat modelview[16] {};
    GLfloat projection[16] {};
    // Vertex array handling
    std::vector<int32_t> index_array;
    std::vector<float> vertex_array;
    SbBool updateGLArray {false};
};

class MeshGuiExport SoFCMeshSegmentShape: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoFCMeshSegmentShape);

public:
    static void initClass();
    SoFCMeshSegmentShape();

    SoSFUInt32 index;
    unsigned int renderTriangleLimit;

protected:
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
    void getPrimitiveCount(SoGetPrimitiveCountAction* action) override;
    void generatePrimitives(SoAction* action) override;
    // Force using the reference count mechanism.
    ~SoFCMeshSegmentShape() override = default;

private:
    enum Binding
    {
        OVERALL = 0,
        PER_FACE_INDEXED,
        PER_VERTEX_INDEXED,
        NONE = OVERALL
    };

private:
    Binding findMaterialBinding(SoState* const state) const;
    // Draw faces
    void drawFaces(const Mesh::MeshObject*,
                   SoMaterialBundle* mb,
                   Binding bind,
                   SbBool needNormals,
                   SbBool ccw) const;
    void drawPoints(const Mesh::MeshObject*, SbBool needNormals, SbBool ccw) const;
};

class MeshGuiExport SoFCMeshObjectBoundary: public SoShape
{
    using inherited = SoShape;

    SO_NODE_HEADER(SoFCMeshObjectBoundary);

public:
    static void initClass();
    SoFCMeshObjectBoundary();

protected:
    void GLRender(SoGLRenderAction* action) override;
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
    void getPrimitiveCount(SoGetPrimitiveCountAction* action) override;
    void generatePrimitives(SoAction* action) override;
    // Force using the reference count mechanism.
    ~SoFCMeshObjectBoundary() override = default;

private:
    void drawLines(const Mesh::MeshObject*) const;
};
// NOLINTEND(cppcoreguidelines-special-member-functions,cppcoreguidelines-virtual-class-destructor)

}  // namespace MeshGui


#endif  // MESHGUI_SOFCMESHOBJECT_H
