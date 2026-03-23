/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SoNonIndexedShape SoNonIndexedShape.h Inventor/nodes/SoNonIndexedShape.h
  \brief The SoNonIndexedShape class is the superclass for all non-indexed vertex based shapes.

  \ingroup coin_nodes
  
  It contains the (now obsoleted) startIndex field and a convenience
  method for calculating the bounding box.
*/

#include <Inventor/nodes/SoNonIndexedShape.h>

#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/elements/SoCoordinateElement.h>

#include "nodes/SoSubNodeP.h"

/*!  
  \var SoSFInt32 SoNonIndexedShape::startIndex 
  Coordinates are fetched from this index on. This field is now obsoleted, and 
  is provided only for backward compatibility.  
*/


SO_NODE_ABSTRACT_SOURCE(SoNonIndexedShape);

/*!
  Constructor.
*/
SoNonIndexedShape::SoNonIndexedShape()
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoNonIndexedShape);

  SO_NODE_ADD_FIELD(startIndex, (0));
}

/*!
  Destructor.
*/
SoNonIndexedShape::~SoNonIndexedShape()
{
}

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoNonIndexedShape::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoNonIndexedShape, SO_FROM_INVENTOR_1);
}

/*!
  This method is provided as a convenient means for the subclasses of this
  class to find their bounding box and center value.

  The returned bounding box will enclose all vertices from \a startIndex
  up to \a startIndex + \a numVertices. If \a numVertices is less than
  zero, \e all vertices in the current coordinate element or vertex property
  node will be used in the calculation.

  The \a center point will be calculated as the average of all the vertices
  in the bounding box.
 */
void
SoNonIndexedShape::computeCoordBBox(SoAction * action, int numVertices,
                                    SbBox3f & box, SbVec3f & center)
{
  const SoCoordinateElement *coordelem =
    SoCoordinateElement::getInstance(action->getState());

  SoNode *vpnode = this->vertexProperty.getValue();
  SoVertexProperty *vp = 
    (vpnode && vpnode->isOfType(SoVertexProperty::getClassTypeId())) ?
    (SoVertexProperty *)vpnode : NULL;
  SbBool vpvtx = vp && (vp->vertex.getNum() > 0);

  const int numCoords = vpvtx ?
    vp->vertex.getNum() :
    coordelem->getNum();

  int32_t startidx = this->startIndex.getValue();
  int32_t lastidx;

  if (numVertices < 0) lastidx = numCoords - 1;
  else lastidx = startidx + numVertices-1;

  if (numCoords <= 0 || lastidx >= numCoords) {
    // no need to call box.makeEmpty(). Box will be empty when
    // this method is called.
    return;
  }

  center.setValue(0.0f, 0.0f, 0.0f);
  
  if (vpvtx || coordelem->is3D()) {
    const SbVec3f * coords = vpvtx ?
      vp->vertex.getValues(0) :
      coordelem->getArrayPtr3();
    
    for (int i = startidx; i <= lastidx; i++) {
      box.extendBy(coords[i]);
      center += coords[i];
    }
  }
  else { // 4D
    SbVec3f tmp;
    const SbVec4f * coords = coordelem->getArrayPtr4();
    for (int i = startidx; i <= lastidx; i++) {
      SbVec4f h = coords[i];
      h.getReal(tmp);
      box.extendBy(tmp);
      center += tmp;
    }
  }
  if (lastidx+1 - startidx) {
    center /= float(lastidx + 1 - startidx);
  }
}

/*!  
  Convenience method that might adjust \a start and \a end
  pointers, which should point at the start and end of the numVertices
  array when calling this method. This takes care of the case where
  numVertices contains a single -1, and all coordinates in the state
  (or in the vertexProperty field) should be rendered as one
  primitive.

  \a dummyarray should be a temporary array, with room for one integer.

  Not part of the OIV API.  
*/
void
SoNonIndexedShape::fixNumVerticesPointers(SoState *state, const int32_t *&start, const int32_t *&end,
                                          int32_t *dummyarray) const
{
  if ((start + 1 == end) && (*start == -1)) {
    const SoCoordinateElement *coordelem =
      SoCoordinateElement::getInstance(state);
    SoVertexProperty * vp = (SoVertexProperty *) this->vertexProperty.getValue();
    SbBool vpvtx = vp && (vp->vertex.getNum() > 0);

    const int numCoords = vpvtx ?
      vp->vertex.getNum() :
      coordelem->getNum();

    dummyarray[0] = numCoords - startIndex.getValue();
    start = dummyarray;
    end = numCoords > 1 ? start + 1 : start;
  }
}
