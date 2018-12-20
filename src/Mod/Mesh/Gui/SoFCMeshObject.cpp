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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <algorithm>
# include <climits>
# ifdef FC_OS_WIN32
# include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
# else
# include <GL/gl.h>
# include <GL/glu.h>
# endif
# include <Inventor/actions/SoCallbackAction.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGetPrimitiveCountAction.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/actions/SoPickAction.h>
# include <Inventor/actions/SoWriteAction.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/errors/SoReadError.h>
# include <Inventor/misc/SoState.h>
#endif

#include "SoFCMeshObject.h"
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Gui/SoFCInteractiveElement.h>
#include <Gui/SoFCSelectionAction.h>
#include <Mod/Mesh/App/Core/Algorithm.h>
#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/App/Core/Elements.h>
#include <Mod/Mesh/App/Core/Grid.h>

using namespace MeshGui;


class SoOutputStreambuf : public std::streambuf
{
public:
    SoOutputStreambuf(SoOutput* o) : out(o)
    {
    }
protected:
    int overflow(int c = EOF)
    {
        if (c != EOF) {
            char z = static_cast<char>(c);
            out->write(z);
        }
        return c;
    }
    std::streamsize xsputn (const char* s, std::streamsize num)
    {
        out->write(s);
        return num;
    }

private:
    SoOutput* out;
};

class SoOutputStream : public std::ostream
{
public:
    SoOutputStream(SoOutput* o) : std::ostream(0), buf(o)
    {
        this->rdbuf(&buf);
    }
private:
    SoOutputStreambuf buf;
};

class SoInputStreambuf : public std::streambuf
{
public:
    SoInputStreambuf(SoInput* o) : inp(o)
    {
        setg (buffer+pbSize,
              buffer+pbSize,
              buffer+pbSize);
    }
protected:
    int underflow()
    {
        if (gptr() < egptr()) {
            return *gptr();
        }

        int numPutback;
        numPutback = gptr() - eback();
        if (numPutback > pbSize) {
            numPutback = pbSize;
        }

        memcpy (buffer+(pbSize-numPutback), gptr()-numPutback, numPutback);

        int num=0;
        for (int i=0; i<bufSize; i++) {
            char c;
            SbBool ok = inp->get(c);
            if (ok) {
                num++;
                buffer[pbSize+i] = c;
                if (c == '\n')
                    break;
            }
            else if (num==0) {
                return EOF;
            }
        }

        setg (buffer+(pbSize-numPutback),
              buffer+pbSize,
              buffer+pbSize+num);

        return *gptr();
    }

private:
    static const int pbSize = 4;
    static const int bufSize = 1024;
    char buffer[bufSize+pbSize];
    SoInput* inp;
};

class SoInputStream : public std::istream
{
public:
    SoInputStream(SoInput* o) : std::istream(0), buf(o)
    {
        this->rdbuf(&buf);
    }
    ~SoInputStream()
    {
    }

private:
    SoInputStreambuf buf;
};

// Defines all required member variables and functions for a
// single-value field
SO_SFIELD_SOURCE(SoSFMeshObject, const Mesh::MeshObject*, const Mesh::MeshObject*);


void SoSFMeshObject::initClass()
{
    // This macro takes the name of the class and the name of the
    // parent class
    SO_SFIELD_INIT_CLASS(SoSFMeshObject, SoSField);
}

// This reads the value of a field from a file. It returns false if the value could not be read
// successfully.
SbBool SoSFMeshObject::readValue(SoInput *in)
{
    if (!in->isBinary()) {
        SoInputStream str(in);
        MeshCore::MeshKernel kernel;
        MeshCore::MeshInput(kernel).LoadMeshNode(str);
        value = new Mesh::MeshObject(kernel);

        // We need to trigger the notification chain here, as this function
        // can be used on a node in a scene graph in any state -- not only
        // during initial scene graph import.
        this->valueChanged();

        return true;
    }

    int32_t countPt;
    in->read(countPt);
    std::vector<float> verts(countPt);
    in->readBinaryArray(&(verts[0]),countPt);

    MeshCore::MeshPointArray rPoints;
    rPoints.reserve(countPt/3);
    for (std::vector<float>::iterator it = verts.begin();
        it != verts.end();) {
            Base::Vector3f p;
            p.x = *it; ++it;
            p.y = *it; ++it;
            p.z = *it; ++it;
            rPoints.push_back(p);
    }

    int32_t countFt;
    in->read(countFt);
    std::vector<int32_t> faces(countFt);
    in->readBinaryArray(&(faces[0]),countFt);

    MeshCore::MeshFacetArray rFacets;
    rFacets.reserve(countFt/3);
    for (std::vector<int32_t>::iterator it = faces.begin();
        it != faces.end();) {
            MeshCore::MeshFacet f;
            f._aulPoints[0] = *it; ++it;
            f._aulPoints[1] = *it; ++it;
            f._aulPoints[2] = *it; ++it;
            rFacets.push_back(f);
    }

    MeshCore::MeshKernel kernel;
    kernel.Adopt(rPoints, rFacets, true);
    value = new Mesh::MeshObject(kernel);

    // We need to trigger the notification chain here, as this function
    // can be used on a node in a scene graph in any state -- not only
    // during initial scene graph import.
    this->valueChanged();

    return true;
}

// This writes the value of a field to a file.
void SoSFMeshObject::writeValue(SoOutput *out) const
{
    if (!out->isBinary()) {
        SoOutputStream str(out);
        MeshCore::MeshOutput(value->getKernel()).SaveMeshNode(str);
        return;
    }

    if (!value) {
        int32_t count = 0;
        out->write(count);
        out->write(count);
        return;
    }
    const MeshCore::MeshPointArray& rPoints = value->getKernel().GetPoints();
    std::vector<float> verts;
    verts.reserve(3*rPoints.size());
    for (MeshCore::MeshPointArray::_TConstIterator it = rPoints.begin();
        it != rPoints.end(); ++it) {
        verts.push_back(it->x);
        verts.push_back(it->y);
        verts.push_back(it->z);
    }

    int32_t countPt = (int32_t)verts.size();
    out->write(countPt);
    out->writeBinaryArray(&(verts[0]),countPt);

    const MeshCore::MeshFacetArray& rFacets = value->getKernel().GetFacets();
    std::vector<uint32_t> faces;
    faces.reserve(3*rFacets.size());
    for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin();
        it != rFacets.end(); ++it) {
        faces.push_back((int32_t)it->_aulPoints[0]);
        faces.push_back((int32_t)it->_aulPoints[1]);
        faces.push_back((int32_t)it->_aulPoints[2]);
    }

    int32_t countFt = (int32_t)faces.size();
    out->write(countFt);
    out->writeBinaryArray((const int32_t*)&(faces[0]),countFt);
}

// -------------------------------------------------------

SO_ELEMENT_SOURCE(SoFCMeshObjectElement);

void SoFCMeshObjectElement::initClass()
{
    SO_ELEMENT_INIT_CLASS(SoFCMeshObjectElement, inherited);
}

void SoFCMeshObjectElement::init(SoState * state)
{
    inherited::init(state);
    this->mesh = 0;
}

SoFCMeshObjectElement::~SoFCMeshObjectElement()
{
}

void SoFCMeshObjectElement::set(SoState * const state, SoNode * const node, const Mesh::MeshObject * const mesh)
{
    SoFCMeshObjectElement * elem = (SoFCMeshObjectElement *)
        SoReplacedElement::getElement(state, classStackIndex, node);
    if (elem) {
        elem->mesh = mesh;
        elem->nodeId = node->getNodeId();
    }
}

const Mesh::MeshObject * SoFCMeshObjectElement::get(SoState * const state)
{
    return SoFCMeshObjectElement::getInstance(state)->mesh;
}

const SoFCMeshObjectElement * SoFCMeshObjectElement::getInstance(SoState * state)
{
    return (const SoFCMeshObjectElement *) SoElement::getConstElement(state, classStackIndex);
}

void SoFCMeshObjectElement::print(FILE * /* file */) const
{
}

// -------------------------------------------------------

SO_NODE_SOURCE(SoFCMeshPickNode);

/*!
  Constructor.
*/
SoFCMeshPickNode::SoFCMeshPickNode(void) : meshGrid(0)
{
    SO_NODE_CONSTRUCTOR(SoFCMeshPickNode);

    SO_NODE_ADD_FIELD(mesh, (0));
}

/*!
  Destructor.
*/
SoFCMeshPickNode::~SoFCMeshPickNode()
{
    delete meshGrid;
}

// Doc from superclass.
void SoFCMeshPickNode::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCMeshPickNode, SoNode, "Node");
}

void SoFCMeshPickNode::notify(SoNotList *list)
{
    SoField *f = list->getLastField();
    if (f == &mesh) {
        const Mesh::MeshObject* meshObject = mesh.getValue();
        if (meshObject) {
            MeshCore::MeshAlgorithm alg(meshObject->getKernel());
            float fAvgLen = alg.GetAverageEdgeLength();
            delete meshGrid;
            meshGrid = new MeshCore::MeshFacetGrid(meshObject->getKernel(), 5.0f * fAvgLen);
        }
    }
}

// Doc from superclass.
void SoFCMeshPickNode::rayPick(SoRayPickAction * /*action*/)
{
}

// Doc from superclass.
void SoFCMeshPickNode::pick(SoPickAction * action)
{
    SoRayPickAction* raypick = static_cast<SoRayPickAction*>(action);
    raypick->setObjectSpace();

    const Mesh::MeshObject* meshObject = mesh.getValue();
    MeshCore::MeshAlgorithm alg(meshObject->getKernel());

    const SbLine& line = raypick->getLine();
    const SbVec3f& pos = line.getPosition();
    const SbVec3f& dir = line.getDirection();
    Base::Vector3f pt(pos[0],pos[1],pos[2]);
    Base::Vector3f dr(dir[0],dir[1],dir[2]);
    unsigned long index;
    if (alg.NearestFacetOnRay(pt, dr, *meshGrid, pt, index)) {
        SoPickedPoint* pp = raypick->addIntersection(SbVec3f(pt.x,pt.y,pt.z));
        if (pp) {
            SoFaceDetail* det = new SoFaceDetail();
            det->setFaceIndex(index);
            pp->setDetail(det, this);
        }
    }
}

// -------------------------------------------------------

SO_NODE_SOURCE(SoFCMeshGridNode);

/*!
  Constructor.
*/
SoFCMeshGridNode::SoFCMeshGridNode(void)
{
    SO_NODE_CONSTRUCTOR(SoFCMeshGridNode);

    SO_NODE_ADD_FIELD(minGrid, (SbVec3f(0,0,0)));
    SO_NODE_ADD_FIELD(maxGrid, (SbVec3f(0,0,0)));
    SO_NODE_ADD_FIELD(lenGrid, (SbVec3s(0,0,0)));
}

/*!
  Destructor.
*/
SoFCMeshGridNode::~SoFCMeshGridNode()
{
}

// Doc from superclass.
void SoFCMeshGridNode::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCMeshGridNode, SoNode, "Node");
}

void SoFCMeshGridNode::GLRender(SoGLRenderAction * /*action*/)
{
    const SbVec3f& min = minGrid.getValue();
    const SbVec3f& max = maxGrid.getValue();
    const SbVec3s& len = lenGrid.getValue();
    short u,v,w; len.getValue(u,v,w);
    float minX, minY, minZ; min.getValue(minX, minY, minZ);
    float maxX, maxY, maxZ; max.getValue(maxX, maxY, maxZ);
    float dx = (maxX-minX)/(float)u;
    float dy = (maxY-minY)/(float)v;
    float dz = (maxZ-minZ)/(float)w;
    glColor3f(0.0f,1.0f,0.0);
    glBegin(GL_LINES);
    for (short i=0; i<u+1; i++) {
        for (short j=0; j<v+1; j++) {
            float p[3];
            p[0] = i * dx + minX;
            p[1] = j * dy + minY;
            p[2] = minZ;
            glVertex3fv(p);

            p[0] = i * dx + minX;
            p[1] = j * dy + minY;
            p[2] = maxZ;
            glVertex3fv(p);
        }
    }
    for (short i=0; i<u+1; i++) {
        for (short j=0; j<w+1; j++) {
            float p[3];
            p[0] = i * dx + minX;
            p[1] = minY;
            p[2] = j * dz + minZ;
            glVertex3fv(p);

            p[0] = i * dx + minX;
            p[1] = maxY;
            p[2] = j * dz + minZ;
            glVertex3fv(p);
        }
    }
    for (short i=0; i<v+1; i++) {
        for (short j=0; j<w+1; j++) {
            float p[3];
            p[0] = minX;
            p[1] = i * dy + minY;
            p[2] = j * dz + minZ;
            glVertex3fv(p);

            p[0] = maxX;
            p[1] = i * dy + minY;
            p[2] = j * dz + minZ;
            glVertex3fv(p);
        }
    }
    glEnd();
}

// -------------------------------------------------------

SO_NODE_SOURCE(SoFCMeshObjectNode);

/*!
  Constructor.
*/
SoFCMeshObjectNode::SoFCMeshObjectNode(void)
{
    SO_NODE_CONSTRUCTOR(SoFCMeshObjectNode);

    SO_NODE_ADD_FIELD(mesh, (0));
}

/*!
  Destructor.
*/
SoFCMeshObjectNode::~SoFCMeshObjectNode()
{
}

// Doc from superclass.
void SoFCMeshObjectNode::initClass(void)
{
    SO_NODE_INIT_CLASS(SoFCMeshObjectNode, SoNode, "Node");

    SO_ENABLE(SoGetBoundingBoxAction, SoFCMeshObjectElement);
    SO_ENABLE(SoGLRenderAction, SoFCMeshObjectElement);
    SO_ENABLE(SoPickAction, SoFCMeshObjectElement);
    SO_ENABLE(SoCallbackAction, SoFCMeshObjectElement);
    SO_ENABLE(SoGetPrimitiveCountAction, SoFCMeshObjectElement);
}

// Doc from superclass.
void SoFCMeshObjectNode::doAction(SoAction * action)
{
    SoFCMeshObjectElement::set(action->getState(), this, mesh.getValue());
}

// Doc from superclass.
void SoFCMeshObjectNode::GLRender(SoGLRenderAction * action)
{
    SoFCMeshObjectNode::doAction(action);
}

// Doc from superclass.
void SoFCMeshObjectNode::callback(SoCallbackAction * action)
{
    SoFCMeshObjectNode::doAction(action);
}

// Doc from superclass.
void SoFCMeshObjectNode::pick(SoPickAction * action)
{
    SoFCMeshObjectNode::doAction(action);
}

// Doc from superclass.
void SoFCMeshObjectNode::getBoundingBox(SoGetBoundingBoxAction * action)
{
    SoFCMeshObjectNode::doAction(action);
}

// Doc from superclass.
void SoFCMeshObjectNode::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
    SoFCMeshObjectNode::doAction(action);
}

// Helper functions: draw vertices
inline void glVertex(const MeshCore::MeshPoint& _v)  
{
    float v[3];
    v[0]=_v.x; v[1]=_v.y;v[2]=_v.z;
    glVertex3fv(v); 
}

// Helper functions: draw normal
inline void glNormal(const Base::Vector3f& _n)
{
    float n[3];
    n[0]=_n.x; n[1]=_n.y;n[2]=_n.z;
    glNormal3fv(n); 
}

// Helper functions: draw normal
inline void glNormal(float* n)
{
    glNormal3fv(n); 
}

// Helper function: convert Vec to SbVec3f
inline SbVec3f sbvec3f(const Base::Vector3f& _v)
{
    return SbVec3f(_v.x, _v.y, _v.z); 
}

SO_NODE_SOURCE(SoFCMeshObjectShape);

void SoFCMeshObjectShape::initClass()
{
    SO_NODE_INIT_CLASS(SoFCMeshObjectShape, SoShape, "Shape");
}

SoFCMeshObjectShape::SoFCMeshObjectShape()
    : renderTriangleLimit(UINT_MAX)
    , selectBuf(0)
    , updateGLArray(false)
{
    SO_NODE_CONSTRUCTOR(SoFCMeshObjectShape);
    setName(SoFCMeshObjectShape::getClassTypeId().getName());
}

SoFCMeshObjectShape::~SoFCMeshObjectShape()
{
}

void SoFCMeshObjectShape::notify(SoNotList * node)
{
    inherited::notify(node);
    updateGLArray = true;
}

#define RENDER_GLARRAYS

/**
 * Either renders the complete mesh or only a subset of the points.
 */
void SoFCMeshObjectShape::GLRender(SoGLRenderAction *action)
{
    if (shouldGLRender(action))
    {
        SoState*  state = action->getState();

        // Here we must save the model and projection matrices because
        // we need them later for picking
        glGetFloatv(GL_MODELVIEW_MATRIX, this->modelview);
        glGetFloatv(GL_PROJECTION_MATRIX, this->projection);

        SbBool mode = Gui::SoFCInteractiveElement::get(state);
        const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
        if (!mesh || mesh->countPoints() == 0) return;

        Binding mbind = this->findMaterialBinding(state);

        SoMaterialBundle mb(action);
        //SoTextureCoordinateBundle tb(action, true, false);

        SbBool needNormals = !mb.isColorOnly()/* || tb.isFunction()*/;
        mb.sendFirst();  // make sure we have the correct material
    
        SbBool ccw = true;
        if (SoShapeHintsElement::getVertexOrdering(state) == SoShapeHintsElement::CLOCKWISE) 
            ccw = false;

        if (mode == false || mesh->countFacets() <= this->renderTriangleLimit) {
            if (mbind != OVERALL) {
                drawFaces(mesh, &mb, mbind, needNormals, ccw);
            }
            else {
#ifdef RENDER_GLARRAYS
                if (updateGLArray) {
                    updateGLArray = false;
                    generateGLArrays(state);
                }
                renderFacesGLArray(action);
#else
                drawFaces(mesh, 0, mbind, needNormals, ccw);
#endif
            }
        }
        else {
#if 0 && defined (RENDER_GLARRAYS)
            renderCoordsGLArray(action);
#else
            drawPoints(mesh, needNormals, ccw);
#endif
        }

        // Disable caching for this node
        //SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
    }
}

/**
 * Translates current material binding into the internal Binding enum.
 */
SoFCMeshObjectShape::Binding SoFCMeshObjectShape::findMaterialBinding(SoState * const state) const
{
    Binding binding = OVERALL;
    SoMaterialBindingElement::Binding matbind = SoMaterialBindingElement::get(state);

    switch (matbind) {
    case SoMaterialBindingElement::OVERALL:
        binding = OVERALL;
        break;
    case SoMaterialBindingElement::PER_VERTEX:
        binding = PER_VERTEX_INDEXED;
        break;
    case SoMaterialBindingElement::PER_VERTEX_INDEXED:
        binding = PER_VERTEX_INDEXED;
        break;
    case SoMaterialBindingElement::PER_PART:
    case SoMaterialBindingElement::PER_FACE:
        binding = PER_FACE_INDEXED;
        break;
    case SoMaterialBindingElement::PER_PART_INDEXED:
    case SoMaterialBindingElement::PER_FACE_INDEXED:
        binding = PER_FACE_INDEXED;
        break;
    default:
        break;
    }
    return binding;
}

/**
 * Renders the triangles of the complete mesh.
 * FIXME: Do it the same way as Coin did to have only one implementation which is controlled by defines
 * FIXME: Implement using different values of transparency for each vertex or face
 */
void SoFCMeshObjectShape::drawFaces(const Mesh::MeshObject * mesh, SoMaterialBundle* mb,
                                    Binding bind, SbBool needNormals, SbBool ccw) const
{
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();
    bool perVertex = (mb && bind == PER_VERTEX_INDEXED);
    bool perFace = (mb && bind == PER_FACE_INDEXED);

    if (needNormals)
    {
        glBegin(GL_TRIANGLES);
        if (ccw) {
            // counterclockwise ordering
            for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it)
            {
                const MeshCore::MeshPoint& v0 = rPoints[it->_aulPoints[0]];
                const MeshCore::MeshPoint& v1 = rPoints[it->_aulPoints[1]];
                const MeshCore::MeshPoint& v2 = rPoints[it->_aulPoints[2]];

                // Calculate the normal n = (v1-v0)x(v2-v0)
                float n[3];
                n[0] = (v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y);
                n[1] = (v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z);
                n[2] = (v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x);
    
                if(perFace)
                mb->send(it-rFacets.begin(), true);
                glNormal(n);
                if(perVertex)
                mb->send(it->_aulPoints[0], true);
                glVertex(v0);
                if(perVertex)
                mb->send(it->_aulPoints[1], true);
                glVertex(v1);
                if(perVertex)
                mb->send(it->_aulPoints[2], true);
                glVertex(v2);
            }
        }
        else {
            // clockwise ordering
            for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it)
            {
                const MeshCore::MeshPoint& v0 = rPoints[it->_aulPoints[0]];
                const MeshCore::MeshPoint& v1 = rPoints[it->_aulPoints[1]];
                const MeshCore::MeshPoint& v2 = rPoints[it->_aulPoints[2]];

                // Calculate the normal n = -(v1-v0)x(v2-v0)
                float n[3];
                n[0] = -((v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y));
                n[1] = -((v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z));
                n[2] = -((v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x));

                glNormal(n);
                glVertex(v0);
                glVertex(v1);
                glVertex(v2);
            }
        }
        glEnd();
    }
    else {
        glBegin(GL_TRIANGLES);
        for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it)
        {
            glVertex(rPoints[it->_aulPoints[0]]);
            glVertex(rPoints[it->_aulPoints[1]]);
            glVertex(rPoints[it->_aulPoints[2]]);
        }
        glEnd();
    }
}

/**
 * Renders the gravity points of a subset of triangles.
 */
void SoFCMeshObjectShape::drawPoints(const Mesh::MeshObject * mesh, SbBool needNormals, SbBool ccw) const
{
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();
    int mod = rFacets.size()/renderTriangleLimit+1;

    float size = std::min<float>((float)mod,3.0f);
    glPointSize(size);

    if (needNormals)
    {
        glBegin(GL_POINTS);
        int ct=0;
        if (ccw) {
            for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it, ct++)
            {
                if (ct%mod==0) {
                    const MeshCore::MeshPoint& v0 = rPoints[it->_aulPoints[0]];
                    const MeshCore::MeshPoint& v1 = rPoints[it->_aulPoints[1]];
                    const MeshCore::MeshPoint& v2 = rPoints[it->_aulPoints[2]];

                    // Calculate the normal n = (v1-v0)x(v2-v0)
                    float n[3];
                    n[0] = (v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y);
                    n[1] = (v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z);
                    n[2] = (v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x);

                    // Calculate the center point p=(v0+v1+v2)/3
                    float p[3];
                    p[0] = (v0.x+v1.x+v2.x)/3.0f;
                    p[1] = (v0.y+v1.y+v2.y)/3.0f;
                    p[2] = (v0.z+v1.z+v2.z)/3.0f;
                    glNormal3fv(n);
                    glVertex3fv(p);
                }
            }
        }
        else {
            for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it, ct++)
            {
                if (ct%mod==0) {
                    const MeshCore::MeshPoint& v0 = rPoints[it->_aulPoints[0]];
                    const MeshCore::MeshPoint& v1 = rPoints[it->_aulPoints[1]];
                    const MeshCore::MeshPoint& v2 = rPoints[it->_aulPoints[2]];

                    // Calculate the normal n = -(v1-v0)x(v2-v0)
                    float n[3];
                    n[0] = -((v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y));
                    n[1] = -((v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z));
                    n[2] = -((v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x));
      
                    // Calculate the center point p=(v0+v1+v2)/3
                    float p[3];
                    p[0] = (v0.x+v1.x+v2.x)/3.0f;
                    p[1] = (v0.y+v1.y+v2.y)/3.0f;
                    p[2] = (v0.z+v1.z+v2.z)/3.0f;
                    glNormal3fv(n);
                    glVertex3fv(p);
                }
            }
        }
        glEnd();
    }
    else {
        glBegin(GL_POINTS);
        int ct=0;
        for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it, ct++)
        {
            if (ct%mod==0) {
                const MeshCore::MeshPoint& v0 = rPoints[it->_aulPoints[0]];
                const MeshCore::MeshPoint& v1 = rPoints[it->_aulPoints[1]];
                const MeshCore::MeshPoint& v2 = rPoints[it->_aulPoints[2]];
                // Calculate the center point p=(v0+v1+v2)/3
                float p[3];
                p[0] = (v0.x+v1.x+v2.x)/3.0f;
                p[1] = (v0.y+v1.y+v2.y)/3.0f;
                p[2] = (v0.z+v1.z+v2.z)/3.0f;
                glVertex3fv(p);
            }
        }
        glEnd();
    }
}

void SoFCMeshObjectShape::generateGLArrays(SoState * state)
{
    const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);

    this->index_array.resize(0);
    this->vertex_array.resize(0);

    std::vector<float> face_vertices;
    std::vector<int32_t> face_indices;

    const MeshCore::MeshKernel& kernel = mesh->getKernel();
    const MeshCore::MeshPointArray& cP = kernel.GetPoints();
    const MeshCore::MeshFacetArray& cF = kernel.GetFacets();

#if 0
    // Smooth shading
    face_vertices.resize(cP.size() * 6);
    face_indices.resize(3 * cF.size());

    int indexed = 0;
    for (MeshCore::MeshPointArray::const_iterator it = cP.begin(); it != cP.end(); ++it) {
        face_vertices[indexed * 6 + 3] += it->x;
        face_vertices[indexed * 6 + 4] += it->y;
        face_vertices[indexed * 6 + 5] += it->z;
        indexed++;
    }

    indexed = 0;
    for (MeshCore::MeshFacetArray::const_iterator it = cF.begin(); it != cF.end(); ++it) {
        Base::Vector3f n = kernel.GetFacet(*it).GetNormal();
        for (int i=0; i<3; i++) {
            int32_t idx = it->_aulPoints[i];
            face_vertices[idx * 6 + 0] += n.x;
            face_vertices[idx * 6 + 1] += n.y;
            face_vertices[idx * 6 + 2] += n.z;

            face_indices[indexed++] = idx;
        }
    }
#else
    // Flat shading
    face_vertices.reserve(3 * cF.size() * 6); // duplicate each vertex
    face_indices.resize(3 * cF.size());

    int indexed = 0;
    for (MeshCore::MeshFacetArray::const_iterator it = cF.begin(); it != cF.end(); ++it) {
        Base::Vector3f n = kernel.GetFacet(*it).GetNormal();
        for (int i=0; i<3; i++) {
            face_vertices.push_back(n.x);
            face_vertices.push_back(n.y);
            face_vertices.push_back(n.z);
            const Base::Vector3f& v = cP[it->_aulPoints[i]];
            face_vertices.push_back(v.x);
            face_vertices.push_back(v.y);
            face_vertices.push_back(v.z);

            face_indices[indexed] = indexed;
            indexed++;
        }
    }
#endif

    this->index_array.swap(face_indices);
    this->vertex_array.swap(face_vertices);
}

void SoFCMeshObjectShape::renderFacesGLArray(SoGLRenderAction *action)
{
    (void)action;
    GLsizei cnt = static_cast<GLsizei>(index_array.size());

    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glInterleavedArrays(GL_N3F_V3F, 0, &(vertex_array[0]));
    glDrawElements(GL_TRIANGLES, cnt, GL_UNSIGNED_INT, &(index_array[0]));

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

void SoFCMeshObjectShape::renderCoordsGLArray(SoGLRenderAction *action)
{
    (void)action;
    int cnt = index_array.size();

    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    glInterleavedArrays(GL_N3F_V3F, 0, &(vertex_array[0]));
    glDrawElements(GL_POINTS, cnt, GL_UNSIGNED_INT, &(index_array[0]));

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}

void SoFCMeshObjectShape::doAction(SoAction * action)
{
    if (action->getTypeId() == Gui::SoGLSelectAction::getClassTypeId()) {
        SoNode* node = action->getNodeAppliedTo();
        if (!node) return; // on no node applied

        // The node we have is the parent of this node and the coordinate node
        // thus we search there for it.
        SoSearchAction sa;
        sa.setInterest(SoSearchAction::FIRST);
        sa.setSearchingAll(false);
        sa.setType(SoFCMeshObjectNode::getClassTypeId(), 1);
        sa.apply(node);
        SoPath * path = sa.getPath();
        if (!path) return;

        // make sure we got the node we wanted
        SoNode* coords = path->getNodeFromTail(0);
        if (!(coords && coords->getTypeId().isDerivedFrom(SoFCMeshObjectNode::getClassTypeId())))
            return;
        const Mesh::MeshObject* mesh = static_cast<SoFCMeshObjectNode*>(coords)->mesh.getValue();
        startSelection(action, mesh);
        renderSelectionGeometry(mesh);
        stopSelection(action, mesh);
    }

    inherited::doAction(action);
}

void SoFCMeshObjectShape::startSelection(SoAction * action, const Mesh::MeshObject* mesh)
{
    Gui::SoGLSelectAction *doaction = static_cast<Gui::SoGLSelectAction*>(action);
    const SbViewportRegion& vp = doaction->getViewportRegion();
    int x = vp.getViewportOriginPixels()[0];
    int y = vp.getViewportOriginPixels()[1];
    int w = vp.getViewportSizePixels()[0];
    int h = vp.getViewportSizePixels()[1];

    unsigned int bufSize = 5*mesh->countFacets(); // make the buffer big enough
    this->selectBuf = new GLuint[bufSize];

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
    // See https://www.opengl.org/discussion_boards/showthread.php/184308-gluPickMatrix-Implementation?p=1259884&viewfull=1#post1259884
    //gluPickMatrix(x, y, w, h, viewport);
    if (w > 0 && h > 0) {
        glTranslatef((viewport[2] - 2 * (x - viewport[0])) / w, (viewport[3] - 2 * (y - viewport[1])) / h, 0);
        glScalef(viewport[2] / w, viewport[3] / h, 1.0);
    }
    glMultMatrixf(/*mp*/this->projection);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(this->modelview);
}

void SoFCMeshObjectShape::stopSelection(SoAction * action, const Mesh::MeshObject* mesh)
{
    // restoring the original projection matrix
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glFlush();

    // returning to normal rendering mode
    GLint hits = glRenderMode(GL_RENDER);

    unsigned int bufSize = 5*mesh->countFacets();
    std::vector< std::pair<double,unsigned int> > hit;
    GLuint index=0;
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

void SoFCMeshObjectShape::renderSelectionGeometry(const Mesh::MeshObject* mesh)
{
    int fcnt=0;
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();
    MeshCore::MeshFacetArray::_TConstIterator it_end = rFacets.end();
    for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != it_end; ++it) {
        const MeshCore::MeshPoint& v0 = rPoints[it->_aulPoints[0]];
        const MeshCore::MeshPoint& v1 = rPoints[it->_aulPoints[1]];
        const MeshCore::MeshPoint& v2 = rPoints[it->_aulPoints[2]];
        glLoadName(fcnt);
        glBegin(GL_TRIANGLES);
        glVertex(v0);
        glVertex(v1);
        glVertex(v2);
        glEnd();
        fcnt++;
    }
}

// test bbox intersection
//static SbBool
//SoFCMeshObjectShape_ray_intersect(SoRayPickAction * action, const SbBox3f & box)
//{
//    if (box.isEmpty()) return false;
//    return action->intersect(box, true);
//}

/**
 * Calculates picked point based on primitives generated by subclasses.
 */
void
SoFCMeshObjectShape::rayPick(SoRayPickAction * action)
{
    //if (this->shouldRayPick(action)) {
    //    this->computeObjectSpaceRay(action);

    //    const SoBoundingBoxCache* bboxcache = getBoundingBoxCache();
    //    if (!bboxcache || !bboxcache->isValid(action->getState()) ||
    //        SoFCMeshObjectShape_ray_intersect(action, bboxcache->getProjectedBox())) {
    //        this->generatePrimitives(action);
    //    }
    //}
    inherited::rayPick(action);
}

/** Sets the point indices, the geometric points and the normal for each triangle.
 * If the number of triangles exceeds \a renderTriangleLimit then only a triangulation of
 * a rough model is filled in instead. This is due to performance issues.
 * \see createTriangleDetail().
 */
void SoFCMeshObjectShape::generatePrimitives(SoAction* action)
{
    SoState*  state = action->getState();
    const Mesh::MeshObject* mesh = SoFCMeshObjectElement::get(state);
    if (!mesh)
        return;
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();
    if (rPoints.size() < 3)
        return;
    if (rFacets.size() < 1)
        return;

    // get material binding
    Binding mbind = this->findMaterialBinding(state);

    // Create the information when moving over or picking into the scene
    SoPrimitiveVertex vertex;
    SoPointDetail pointDetail;
    SoFaceDetail faceDetail;

    vertex.setDetail(&pointDetail);

    beginShape(action, TRIANGLES, &faceDetail);
    try 
    {
        for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it)
        {
            const MeshCore::MeshPoint& v0 = rPoints[it->_aulPoints[0]];
            const MeshCore::MeshPoint& v1 = rPoints[it->_aulPoints[1]];
            const MeshCore::MeshPoint& v2 = rPoints[it->_aulPoints[2]];

            // Calculate the normal n = (v1-v0)x(v2-v0)
            SbVec3f n;
            n[0] = (v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y);
            n[1] = (v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z);
            n[2] = (v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x);

            // Set the normal
            vertex.setNormal(n);

            // Vertex 0
            if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
                pointDetail.setMaterialIndex(it->_aulPoints[0]);
                vertex.setMaterialIndex(it->_aulPoints[0]);
            }
            pointDetail.setCoordinateIndex(it->_aulPoints[0]);
            vertex.setPoint(sbvec3f(v0));
            shapeVertex(&vertex);

            // Vertex 1
            if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
                pointDetail.setMaterialIndex(it->_aulPoints[1]);
                vertex.setMaterialIndex(it->_aulPoints[1]);
            }
            pointDetail.setCoordinateIndex(it->_aulPoints[1]);
            vertex.setPoint(sbvec3f(v1));
            shapeVertex(&vertex);

            // Vertex 2
            if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
                pointDetail.setMaterialIndex(it->_aulPoints[2]);
                vertex.setMaterialIndex(it->_aulPoints[2]);
            }
            pointDetail.setCoordinateIndex(it->_aulPoints[2]);
            vertex.setPoint(sbvec3f(v2));
            shapeVertex(&vertex);

            // Increment for the next face
            faceDetail.incFaceIndex();
        }
    }
    catch (const Base::MemoryException&) {
        Base::Console().Log("Not enough memory to generate primitives\n");
    }

    endShape();
}

/**
 * If the number of triangles exceeds \a renderTriangleLimit 0 is returned.
 * This means that the client programmer needs to implement itself to get the
 * index of the picked triangle. If the number of triangles doesn't exceed
 * \a renderTriangleLimit SoShape::createTriangleDetail() gets called.
 * Against the default OpenInventor implementation which returns 0 as well
 * Coin3d fills in the point and face indices.
 */
SoDetail * SoFCMeshObjectShape::createTriangleDetail(SoRayPickAction * action,
                                              const SoPrimitiveVertex * v1,
                                              const SoPrimitiveVertex * v2,
                                              const SoPrimitiveVertex * v3,
                                              SoPickedPoint * pp)
{
    SoDetail* detail = inherited::createTriangleDetail(action, v1, v2, v3, pp);
    return detail;
}

/**
 * Sets the bounding box of the mesh to \a box and its center to \a center.
 */
void SoFCMeshObjectShape::computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center)
{
    SoState*  state = action->getState();
    const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
    if (mesh && mesh->countPoints() > 0) {
        Base::BoundBox3f cBox = mesh->getKernel().GetBoundBox();
        box.setBounds(SbVec3f(cBox.MinX,cBox.MinY,cBox.MinZ),
                      SbVec3f(cBox.MaxX,cBox.MaxY,cBox.MaxZ));
        Base::Vector3f mid = cBox.GetCenter();
        center.setValue(mid.x,mid.y,mid.z);
    }
    else {
        box.setBounds(SbVec3f(0,0,0), SbVec3f(0,0,0));
        center.setValue(0.0f,0.0f,0.0f);
    }
}

/**
 * Adds the number of the triangles to the \a SoGetPrimitiveCountAction.
 */
void SoFCMeshObjectShape::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
    if (!this->shouldPrimitiveCount(action)) return;
    SoState*  state = action->getState();
    const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
    action->addNumTriangles(mesh->countFacets());
    action->addNumPoints(mesh->countPoints());
}

/**
 * Counts the number of triangles. If a mesh is not set yet it returns 0.
 */
unsigned int SoFCMeshObjectShape::countTriangles(SoAction * action) const
{
    SoState*  state = action->getState();
    const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
    return (unsigned int)mesh->countFacets();
}

// -------------------------------------------------------

SO_NODE_SOURCE(SoFCMeshSegmentShape);

void SoFCMeshSegmentShape::initClass()
{
    SO_NODE_INIT_CLASS(SoFCMeshSegmentShape, SoShape, "Shape");
}

SoFCMeshSegmentShape::SoFCMeshSegmentShape() : renderTriangleLimit(UINT_MAX)
{
    SO_NODE_CONSTRUCTOR(SoFCMeshSegmentShape);
    SO_NODE_ADD_FIELD(index, (0));
}

/**
 * Either renders the complete mesh or only a subset of the points.
 */
void SoFCMeshSegmentShape::GLRender(SoGLRenderAction *action)
{
    if (shouldGLRender(action))
    {
        SoState*  state = action->getState();

        SbBool mode = Gui::SoFCInteractiveElement::get(state);
        const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
        if (!mesh) return;

        Binding mbind = this->findMaterialBinding(state);

        SoMaterialBundle mb(action);
        //SoTextureCoordinateBundle tb(action, true, false);

        SbBool needNormals = !mb.isColorOnly()/* || tb.isFunction()*/;
        mb.sendFirst();  // make sure we have the correct material
    
        SbBool ccw = true;
        if (SoShapeHintsElement::getVertexOrdering(state) == SoShapeHintsElement::CLOCKWISE) 
            ccw = false;

        if (mode == false || mesh->countFacets() <= this->renderTriangleLimit) {
            if (mbind != OVERALL)
                drawFaces(mesh, &mb, mbind, needNormals, ccw);
            else
                drawFaces(mesh, 0, mbind, needNormals, ccw);
        }
        else {
            drawPoints(mesh, needNormals, ccw);
        }

        // Disable caching for this node
        //SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
    }
}

/**
 * Translates current material binding into the internal Binding enum.
 */
SoFCMeshSegmentShape::Binding SoFCMeshSegmentShape::findMaterialBinding(SoState * const state) const
{
    Binding binding = OVERALL;
    SoMaterialBindingElement::Binding matbind = SoMaterialBindingElement::get(state);

    switch (matbind) {
    case SoMaterialBindingElement::OVERALL:
        binding = OVERALL;
        break;
    case SoMaterialBindingElement::PER_VERTEX:
        binding = PER_VERTEX_INDEXED;
        break;
    case SoMaterialBindingElement::PER_VERTEX_INDEXED:
        binding = PER_VERTEX_INDEXED;
        break;
    case SoMaterialBindingElement::PER_PART:
    case SoMaterialBindingElement::PER_FACE:
        binding = PER_FACE_INDEXED;
        break;
    case SoMaterialBindingElement::PER_PART_INDEXED:
    case SoMaterialBindingElement::PER_FACE_INDEXED:
        binding = PER_FACE_INDEXED;
        break;
    default:
        break;
    }
    return binding;
}

/**
 * Renders the triangles of the complete mesh.
 * FIXME: Do it the same way as Coin did to have only one implementation which is controlled by defines
 * FIXME: Implement using different values of transparency for each vertex or face
 */
void SoFCMeshSegmentShape::drawFaces(const Mesh::MeshObject * mesh, SoMaterialBundle* mb,
                                    Binding bind, SbBool needNormals, SbBool ccw) const
{
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();
    if (mesh->countSegments() <= this->index.getValue())
        return;
    const std::vector<unsigned long> rSegm = mesh->getSegment
        (this->index.getValue()).getIndices();
    bool perVertex = (mb && bind == PER_VERTEX_INDEXED);
    bool perFace = (mb && bind == PER_FACE_INDEXED);

    if (needNormals)
    {
        glBegin(GL_TRIANGLES);
        if (ccw) {
            // counterclockwise ordering
            for (std::vector<unsigned long>::const_iterator it = rSegm.begin(); it != rSegm.end(); ++it)
            {
                const MeshCore::MeshFacet& f = rFacets[*it];
                const MeshCore::MeshPoint& v0 = rPoints[f._aulPoints[0]];
                const MeshCore::MeshPoint& v1 = rPoints[f._aulPoints[1]];
                const MeshCore::MeshPoint& v2 = rPoints[f._aulPoints[2]];

                // Calculate the normal n = (v1-v0)x(v2-v0)
                float n[3];
                n[0] = (v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y);
                n[1] = (v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z);
                n[2] = (v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x);
    
                if(perFace)
                mb->send(*it, true);
                glNormal(n);
                if(perVertex)
                mb->send(f._aulPoints[0], true);
                glVertex(v0);
                if(perVertex)
                mb->send(f._aulPoints[1], true);
                glVertex(v1);
                if(perVertex)
                mb->send(f._aulPoints[2], true);
                glVertex(v2);
            }
        }
        else {
            // clockwise ordering
            for (std::vector<unsigned long>::const_iterator it = rSegm.begin(); it != rSegm.end(); ++it)
            {
                const MeshCore::MeshFacet& f = rFacets[*it];
                const MeshCore::MeshPoint& v0 = rPoints[f._aulPoints[0]];
                const MeshCore::MeshPoint& v1 = rPoints[f._aulPoints[1]];
                const MeshCore::MeshPoint& v2 = rPoints[f._aulPoints[2]];

                // Calculate the normal n = -(v1-v0)x(v2-v0)
                float n[3];
                n[0] = -((v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y));
                n[1] = -((v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z));
                n[2] = -((v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x));

                glNormal(n);
                glVertex(v0);
                glVertex(v1);
                glVertex(v2);
            }
        }
        glEnd();
    }
    else {
        glBegin(GL_TRIANGLES);
        for (std::vector<unsigned long>::const_iterator it = rSegm.begin(); it != rSegm.end(); ++it)
        {
            const MeshCore::MeshFacet& f = rFacets[*it];
            glVertex(rPoints[f._aulPoints[0]]);
            glVertex(rPoints[f._aulPoints[1]]);
            glVertex(rPoints[f._aulPoints[2]]);
        }
        glEnd();
    }
}

/**
 * Renders the gravity points of a subset of triangles.
 */
void SoFCMeshSegmentShape::drawPoints(const Mesh::MeshObject * mesh, SbBool needNormals, SbBool ccw) const
{
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();
    if (mesh->countSegments() <= this->index.getValue())
        return;
    const std::vector<unsigned long> rSegm = mesh->getSegment
        (this->index.getValue()).getIndices();
    int mod = rSegm.size()/renderTriangleLimit+1;

    float size = std::min<float>((float)mod,3.0f);
    glPointSize(size);

    if (needNormals)
    {
        glBegin(GL_POINTS);
        int ct=0;
        if (ccw) {
            for (std::vector<unsigned long>::const_iterator it = rSegm.begin(); it != rSegm.end(); ++it, ct++)
            {
                if (ct%mod==0) {
                    const MeshCore::MeshFacet& f = rFacets[*it];
                    const MeshCore::MeshPoint& v0 = rPoints[f._aulPoints[0]];
                    const MeshCore::MeshPoint& v1 = rPoints[f._aulPoints[1]];
                    const MeshCore::MeshPoint& v2 = rPoints[f._aulPoints[2]];

                    // Calculate the normal n = (v1-v0)x(v2-v0)
                    float n[3];
                    n[0] = (v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y);
                    n[1] = (v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z);
                    n[2] = (v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x);

                    // Calculate the center point p=(v0+v1+v2)/3
                    float p[3];
                    p[0] = (v0.x+v1.x+v2.x)/3.0f;
                    p[1] = (v0.y+v1.y+v2.y)/3.0f;
                    p[2] = (v0.z+v1.z+v2.z)/3.0f;
                    glNormal3fv(n);
                    glVertex3fv(p);
                }
            }
        }
        else {
            for (std::vector<unsigned long>::const_iterator it = rSegm.begin(); it != rSegm.end(); ++it, ct++)
            {
                if (ct%mod==0) {
                    const MeshCore::MeshFacet& f = rFacets[*it];
                    const MeshCore::MeshPoint& v0 = rPoints[f._aulPoints[0]];
                    const MeshCore::MeshPoint& v1 = rPoints[f._aulPoints[1]];
                    const MeshCore::MeshPoint& v2 = rPoints[f._aulPoints[2]];

                    // Calculate the normal n = -(v1-v0)x(v2-v0)
                    float n[3];
                    n[0] = -((v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y));
                    n[1] = -((v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z));
                    n[2] = -((v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x));
      
                    // Calculate the center point p=(v0+v1+v2)/3
                    float p[3];
                    p[0] = (v0.x+v1.x+v2.x)/3.0f;
                    p[1] = (v0.y+v1.y+v2.y)/3.0f;
                    p[2] = (v0.z+v1.z+v2.z)/3.0f;
                    glNormal3fv(n);
                    glVertex3fv(p);
                }
            }
        }
        glEnd();
    }
    else {
        glBegin(GL_POINTS);
        int ct=0;
        for (std::vector<unsigned long>::const_iterator it = rSegm.begin(); it != rSegm.end(); ++it, ct++)
        {
            if (ct%mod==0) {
                const MeshCore::MeshFacet& f = rFacets[*it];
                const MeshCore::MeshPoint& v0 = rPoints[f._aulPoints[0]];
                const MeshCore::MeshPoint& v1 = rPoints[f._aulPoints[1]];
                const MeshCore::MeshPoint& v2 = rPoints[f._aulPoints[2]];
                // Calculate the center point p=(v0+v1+v2)/3
                float p[3];
                p[0] = (v0.x+v1.x+v2.x)/3.0f;
                p[1] = (v0.y+v1.y+v2.y)/3.0f;
                p[2] = (v0.z+v1.z+v2.z)/3.0f;
                glVertex3fv(p);
            }
        }
        glEnd();
    }
}

/** Sets the point indices, the geometric points and the normal for each triangle.
 * If the number of triangles exceeds \a renderTriangleLimit then only a triangulation
 * of a rough model is filled in instead. This is due to performance issues.
 * \see createTriangleDetail().
 */
void SoFCMeshSegmentShape::generatePrimitives(SoAction* action)
{
    SoState*  state = action->getState();
    const Mesh::MeshObject* mesh = SoFCMeshObjectElement::get(state);
    if (!mesh)
        return;
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();
    if (rPoints.size() < 3)
        return;
    if (rFacets.size() < 1)
        return;
    if (mesh->countSegments() <= this->index.getValue())
        return;
    const std::vector<unsigned long> rSegm = mesh->getSegment
        (this->index.getValue()).getIndices();

    // get material binding
    Binding mbind = this->findMaterialBinding(state);

    // Create the information when moving over or picking into the scene
    SoPrimitiveVertex vertex;
    SoPointDetail pointDetail;
    SoFaceDetail faceDetail;

    vertex.setDetail(&pointDetail);

    beginShape(action, TRIANGLES, &faceDetail);
    try 
    {
        for (std::vector<unsigned long>::const_iterator it = rSegm.begin(); it != rSegm.end(); ++it)
        {
            const MeshCore::MeshFacet& f = rFacets[*it];
            const MeshCore::MeshPoint& v0 = rPoints[f._aulPoints[0]];
            const MeshCore::MeshPoint& v1 = rPoints[f._aulPoints[1]];
            const MeshCore::MeshPoint& v2 = rPoints[f._aulPoints[2]];

            // Calculate the normal n = (v1-v0)x(v2-v0)
            SbVec3f n;
            n[0] = (v1.y-v0.y)*(v2.z-v0.z)-(v1.z-v0.z)*(v2.y-v0.y);
            n[1] = (v1.z-v0.z)*(v2.x-v0.x)-(v1.x-v0.x)*(v2.z-v0.z);
            n[2] = (v1.x-v0.x)*(v2.y-v0.y)-(v1.y-v0.y)*(v2.x-v0.x);

            // Set the normal
            vertex.setNormal(n);

            // Vertex 0
            if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
                pointDetail.setMaterialIndex(f._aulPoints[0]);
                vertex.setMaterialIndex(f._aulPoints[0]);
            }
            pointDetail.setCoordinateIndex(f._aulPoints[0]);
            vertex.setPoint(sbvec3f(v0));
            shapeVertex(&vertex);

            // Vertex 1
            if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
                pointDetail.setMaterialIndex(f._aulPoints[1]);
                vertex.setMaterialIndex(f._aulPoints[1]);
            }
            pointDetail.setCoordinateIndex(f._aulPoints[1]);
            vertex.setPoint(sbvec3f(v1));
            shapeVertex(&vertex);

            // Vertex 2
            if (mbind == PER_VERTEX_INDEXED || mbind == PER_FACE_INDEXED) {
                pointDetail.setMaterialIndex(f._aulPoints[2]);
                vertex.setMaterialIndex(f._aulPoints[2]);
            }
            pointDetail.setCoordinateIndex(f._aulPoints[2]);
            vertex.setPoint(sbvec3f(v2));
            shapeVertex(&vertex);

            // Increment for the next face
            faceDetail.incFaceIndex();
        }
    }
    catch (const Base::MemoryException&) {
        Base::Console().Log("Not enough memory to generate primitives\n");
    }

    endShape();
}

/**
 * Sets the bounding box of the mesh to \a box and its center to \a center.
 */
void SoFCMeshSegmentShape::computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center)
{
    box.setBounds(SbVec3f(0,0,0), SbVec3f(0,0,0));
    center.setValue(0.0f,0.0f,0.0f);

    SoState*  state = action->getState();
    const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
    if (mesh && mesh->countSegments() > this->index.getValue()) {
        const Mesh::Segment& segm = mesh->getSegment(this->index.getValue());
        const std::vector<unsigned long>& indices = segm.getIndices();
        Base::BoundBox3f cBox;
        if (!indices.empty()) {
            const MeshCore::MeshPointArray& rPoint = mesh->getKernel().GetPoints();
            const MeshCore::MeshFacetArray& rFaces = mesh->getKernel().GetFacets();

            for (std::vector<unsigned long>::const_iterator it = indices.begin();
                it != indices.end(); ++it) {
                    const MeshCore::MeshFacet& face = rFaces[*it];
                    cBox.Add(rPoint[face._aulPoints[0]]);
                    cBox.Add(rPoint[face._aulPoints[1]]);
                    cBox.Add(rPoint[face._aulPoints[2]]);
            }
            
            box.setBounds(SbVec3f(cBox.MinX,cBox.MinY,cBox.MinZ),
                          SbVec3f(cBox.MaxX,cBox.MaxY,cBox.MaxZ));
            Base::Vector3f mid = cBox.GetCenter();
            center.setValue(mid.x,mid.y,mid.z);
        }
    }
}

/**
 * Adds the number of the triangles to the \a SoGetPrimitiveCountAction.
 */
void SoFCMeshSegmentShape::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
    if (!this->shouldPrimitiveCount(action)) return;
    SoState*  state = action->getState();
    const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
    if (mesh && mesh->countSegments() > this->index.getValue()) {
        const Mesh::Segment& segm = mesh->getSegment(this->index.getValue());
        action->addNumTriangles(segm.getIndices().size());
    }
}

// -------------------------------------------------------

SO_NODE_SOURCE(SoFCMeshObjectBoundary);

void SoFCMeshObjectBoundary::initClass()
{
    SO_NODE_INIT_CLASS(SoFCMeshObjectBoundary, SoShape, "Shape");
}

SoFCMeshObjectBoundary::SoFCMeshObjectBoundary()
{
    SO_NODE_CONSTRUCTOR(SoFCMeshObjectBoundary);
}

/**
 * Renders the open edges only.
 */
void SoFCMeshObjectBoundary::GLRender(SoGLRenderAction *action)
{
    if (shouldGLRender(action))
    {
        SoState*  state = action->getState();
        const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
        if (!mesh) return;

        SoMaterialBundle mb(action);
        SoTextureCoordinateBundle tb(action, true, false);
        SoLazyElement::setLightModel(state, SoLazyElement::BASE_COLOR);
        mb.sendFirst();  // make sure we have the correct material

        drawLines(mesh);

        // Disable caching for this node
        //SoGLCacheContextElement::shouldAutoCache(state, SoGLCacheContextElement::DONT_AUTO_CACHE);
    }
}

/**
 * Renders the triangles of the complete mesh.
 */
void SoFCMeshObjectBoundary::drawLines(const Mesh::MeshObject * mesh) const
{
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();

    // When rendering open edges use the given line width * 3 
    GLfloat lineWidth;
    glGetFloatv(GL_LINE_WIDTH, &lineWidth);
    glLineWidth(3.0f*lineWidth);

    // Use the data structure directly and not through MeshFacetIterator as this
    // class is quite slowly (at least for rendering)
    glBegin(GL_LINES);
    for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it) {
        for (int i=0; i<3; i++) {
            if (it->_aulNeighbours[i] == ULONG_MAX) {
                glVertex(rPoints[it->_aulPoints[i]]);
                glVertex(rPoints[it->_aulPoints[(i+1)%3]]);
            }
        }
    }

    glEnd();
}

void SoFCMeshObjectBoundary::generatePrimitives(SoAction* action)
{
    // do not create primitive information as an SoFCMeshObjectShape
    // should already be used that delivers the information
    SoState*  state = action->getState();
    const Mesh::MeshObject* mesh = SoFCMeshObjectElement::get(state);
    if (!mesh)
        return;
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    const MeshCore::MeshFacetArray & rFacets = mesh->getKernel().GetFacets();

    // Create the information when moving over or picking into the scene
    SoPrimitiveVertex vertex;
    SoPointDetail pointDetail;
    SoLineDetail lineDetail;

    vertex.setDetail(&pointDetail);

    beginShape(action, LINES, &lineDetail);
    for (MeshCore::MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it)
    {
        for (int i=0; i<3; i++) {
            if (it->_aulNeighbours[i] == ULONG_MAX) {
                const MeshCore::MeshPoint& v0 = rPoints[it->_aulPoints[i]];
                const MeshCore::MeshPoint& v1 = rPoints[it->_aulPoints[(i+1)%3]];

                // Vertex 0
                pointDetail.setCoordinateIndex(it->_aulPoints[i]);
                vertex.setPoint(sbvec3f(v0));
                shapeVertex(&vertex);

                // Vertex 1
                pointDetail.setCoordinateIndex(it->_aulPoints[(i+1)%3]);
                vertex.setPoint(sbvec3f(v1));
                shapeVertex(&vertex);

                // Increment for the next open edge
                lineDetail.incLineIndex();
            }
        }
    }

    endShape();
}

/**
 * Sets the bounding box of the mesh to \a box and its center to \a center.
 */
void SoFCMeshObjectBoundary::computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center)
{
    SoState*  state = action->getState();
    const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
    if (!mesh)
        return;
    const MeshCore::MeshPointArray & rPoints = mesh->getKernel().GetPoints();
    if (rPoints.size() > 0) {
        Base::BoundBox3f cBox;
        for (MeshCore::MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it)
            cBox.Add(*it);
        box.setBounds(SbVec3f(cBox.MinX,cBox.MinY,cBox.MinZ),
                      SbVec3f(cBox.MaxX,cBox.MaxY,cBox.MaxZ));
        Base::Vector3f mid = cBox.GetCenter();
        center.setValue(mid.x,mid.y,mid.z);
    }
    else {
        box.setBounds(SbVec3f(0,0,0), SbVec3f(0,0,0));
        center.setValue(0.0f,0.0f,0.0f);
    }
}

/**
 * Adds the number of the triangles to the \a SoGetPrimitiveCountAction.
 */
void SoFCMeshObjectBoundary::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
    if (!this->shouldPrimitiveCount(action)) return;
    SoState*  state = action->getState();
    const Mesh::MeshObject * mesh = SoFCMeshObjectElement::get(state);
    if (!mesh)
        return;
    const MeshCore::MeshFacetArray & rFaces = mesh->getKernel().GetFacets();

    // Count number of open edges first
    int ctEdges=0;
    for (MeshCore::MeshFacetArray::_TConstIterator jt = rFaces.begin(); jt != rFaces.end(); ++jt) {
        for (int i=0; i<3; i++) {
            if (jt->_aulNeighbours[i] == ULONG_MAX) {
                ctEdges++;
            }
        }
    }

    action->addNumLines(ctEdges);
}
