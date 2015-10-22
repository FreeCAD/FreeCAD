/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <werner.wm.mayer@gmx.de>              *
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

#ifndef MESHGUI_SOFC_MESHFACESET_H
#define MESHGUI_SOFC_MESHFACESET_H

#include <Inventor/fields/SoSField.h>
#include <Inventor/fields/SoSubField.h>
#include <Mod/Mesh/App/Core/Elements.h>


#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFInt32.h>

class SoMaterialBundle;

namespace Mesh {
class Feature;
}

namespace MeshGui {

class MeshGuiExport SoSFMeshFacetArray : public SoSField {
  typedef SoSField inherited;

  SO_SFIELD_HEADER(SoSFMeshFacetArray, MeshCore::MeshFacetArray*, MeshCore::MeshFacetArray*);

public:
  static void initClass(void);
  void setValue(const MeshCore::MeshFacetArray& p);

protected:
  SbBool readBinaryValues(SoInput * in, unsigned long numarg);
  SbBool read1Value(SoInput * in, unsigned long idx);
  void writeBinaryValues(SoOutput * out) const;
  void write1Value(SoOutput * out, unsigned long idx) const;
  int getNumValuesPerLine() const;
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshFacetElement : public SoReplacedElement {
  typedef SoReplacedElement inherited;

  SO_ELEMENT_HEADER(SoFCMeshFacetElement);

public:
  static void initClass(void);

  virtual void init(SoState * state);
  static void set(SoState * const state, SoNode * const node, const MeshCore::MeshFacetArray * const coords);
  static const MeshCore::MeshFacetArray * get(SoState * const state);
  static const SoFCMeshFacetElement * getInstance(SoState * state);
  virtual void print(FILE * file) const;

protected:
  virtual ~SoFCMeshFacetElement();
  const MeshCore::MeshFacetArray *coordIndex;
};

// -------------------------------------------------------

class MeshGuiExport SoFCMeshFacet : public SoNode {
  typedef SoSField inherited;

  SO_NODE_HEADER(SoFCMeshFacet);

public:
  static void initClass(void);
  SoFCMeshFacet(void);

  SoSFMeshFacetArray coordIndex;

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void pick(SoPickAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

protected:
  virtual ~SoFCMeshFacet();
};

// -------------------------------------------------------

/**
 * \brief The SoFCMeshFaceSet class renders the mesh data structure.
 * It does basically the same as SoFCMeshNode by rendering directly the FreeCAD mesh  structure whereas this class follows more the Inventor way. 
 * While SoFCMeshFaceSet has a pointer to the mesh structure as a whole for SoFCMeshFaceSet the mesh is splitted into two nodes: 
 * an SoFCMeshVertex has a field that holds a pointer to vertex array and SoFCMeshFacet has a field that holds a pointer to the face array.
 *
 * The advantage of separating the mesh structure is higher flexibility. E.g. to render open edges the class SoFCMeshOpenEdgeSet just takes the 
 * SoFCMeshVertex and SoFCMeshFaceSet nodes from the stack and does the rendering. The client programmer just has to add the an SoFCMeshOpenEdgeSet 
 * instance to the Inventor tree -- nothing more. Another advantage is that memory is saved when writing the scene to a file. 
 * The actual data is only hold and written by SoFCMeshVertex and SoFCMeshFaceSet. Normally, no shape nodes have to save further data to the file.
 * @author Werner Mayer
 */
class MeshGuiExport SoFCMeshFaceSet : public SoShape {
  typedef SoShape inherited;

  SO_NODE_HEADER(SoFCMeshFaceSet);
    
public:
  static void initClass();
  SoFCMeshFaceSet();

  unsigned int MaximumTriangles;

protected:
  virtual void GLRender(SoGLRenderAction *action);
  virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
  virtual void  rayPick (SoRayPickAction *action);
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
  virtual ~SoFCMeshFaceSet() {};
  virtual void notify(SoNotList * list);
  Binding findMaterialBinding(SoState * const state) const;
  // Draw faces
  void drawFaces(const MeshCore::MeshPointArray *, const MeshCore::MeshFacetArray*, SoMaterialBundle* mb, Binding bind, 
                 SbBool needNormals, SbBool ccw) const;
  void drawPoints(const MeshCore::MeshPointArray *, const MeshCore::MeshFacetArray*, SbBool needNormals, SbBool ccw) const;
  unsigned int countTriangles(SoAction * action) const;
  void createProxyModel(const MeshCore::MeshPointArray *, const MeshCore::MeshFacetArray*, SbBool simplest);

private:
  bool meshChanged;
  SoMFVec3f point;
  SoMFInt32 coordIndex;
};

// ------------------------------------------------------------

class MeshGuiExport SoFCMeshOpenEdgeSet : public SoShape {
  typedef SoShape inherited;

  SO_NODE_HEADER(SoFCMeshOpenEdgeSet);
    
public:
  static void initClass();
  SoFCMeshOpenEdgeSet();

protected:
  virtual void GLRender(SoGLRenderAction *action);
  virtual void computeBBox(SoAction *action, SbBox3f &box, SbVec3f &center);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
  virtual void generatePrimitives(SoAction *action);
private:
  // Force using the reference count mechanism.
  virtual ~SoFCMeshOpenEdgeSet() {};
  void drawLines(const MeshCore::MeshPointArray *, const MeshCore::MeshFacetArray*) const ;
};

} // namespace MeshGui


#endif // MESHGUI_SOFC_MESHFACESET_H

