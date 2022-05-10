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
# include <Inventor/actions/SoCallbackAction.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGetPrimitiveCountAction.h>
# include <Inventor/actions/SoGLRenderAction.h>
# include <Inventor/actions/SoPickAction.h>
# include <Inventor/actions/SoWriteAction.h>
# include <Inventor/errors/SoReadError.h>
# include <Inventor/misc/SoState.h>
#endif

#include "SoFCMeshVertex.h"

using namespace MeshGui;


// Defines all required member variables and functions for a
// single-value field
SO_SFIELD_SOURCE(SoSFMeshPointArray, MeshCore::MeshPointArray*, MeshCore::MeshPointArray*);


void SoSFMeshPointArray::initClass()
{
   // This macro takes the name of the class and the name of the
   // parent class
   SO_SFIELD_INIT_CLASS(SoSFMeshPointArray, SoSField);
}

void SoSFMeshPointArray::setValue(const MeshCore::MeshPointArray& p)
{
  SoSFMeshPointArray::setValue(const_cast<MeshCore::MeshPointArray*>(&p));
}

// This reads the value of a field from a file. It returns FALSE if the value could not be read
// successfully.
SbBool SoSFMeshPointArray::readValue(SoInput *in)
{
  // This macro is convenient for reading with error detection.
#define READ_VAL(val) \
  if (!in->read(val)) { \
    SoReadError::post(in, "Premature end of file"); \
    return FALSE; \
  }

  value = new MeshCore::MeshPointArray();

  // ** Binary format ******************************************************
  if (in->isBinary()) {
    int numtoread;
    READ_VAL(numtoread);

    // Sanity checking on the value, to avoid barfing on corrupt
    // files.
    if (numtoread < 0) {
      SoReadError::post(in, "invalid number of values in field: %d",
                        numtoread);
      return FALSE;
    }

    value->resize(numtoread);
    if (!this->readBinaryValues(in, numtoread)) {
        return FALSE;
    }
  }

  // ** ASCII format *******************************************************
  else {
    char c;
    READ_VAL(c);
    if (c == '[') {
      unsigned long currentidx = 0;

      READ_VAL(c);
      if (c == ']') {
        // Zero values -- done. :^)
      }
      else {
        in->putBack(c);

        while (TRUE) {
          // makeRoom() makes sure the allocation strategy is decent.
          if (currentidx >= value->size()) value->resize(currentidx + 1);

          if (!this->read1Value(in, currentidx++))
              return FALSE;

          READ_VAL(c);
          if (c == ',') { READ_VAL(c); } // Treat trailing comma as whitespace.

          // That was the last array element, we're done.
          if (c == ']') { break; }

          if (c == '}') {
            SoReadError::post(in, "Premature end of array, got '%c'", c);
            return FALSE;
          }

          in->putBack(c);
        }
      }

      // Fit array to number of items.
      value->resize(currentidx);
    }
    else {
      in->putBack(c);
      value->resize(1);
      if (!this->read1Value(in, 0))
          return FALSE;
    }
  }

#undef READ_VAL

  // We need to trigger the notification chain here, as this function
  // can be used on a node in a scene graph in any state -- not only
  // during initial scene graph import.
  this->valueChanged();
  
  return TRUE;
}

SbBool SoSFMeshPointArray::readBinaryValues(SoInput * in, unsigned long numarg)
{
  assert(in->isBinary());
  assert(numarg >= 0);

  for (unsigned long i=0; i < numarg; i++) if (!this->read1Value(in, i)) return FALSE;
  return TRUE;
}

SbBool SoSFMeshPointArray::read1Value(SoInput * in, unsigned long idx)
{
  assert(idx < value->size());
  MeshCore::MeshPoint& v = (*value)[idx];
  return (in->read(v.x) && in->read(v.y) && in->read(v.z));
}

int SoSFMeshPointArray::getNumValuesPerLine() const
{
  return 1;
}

// This writes the value of a field to a file.
void SoSFMeshPointArray::writeValue(SoOutput *out) const
{
  if (out->isBinary()) {
    this->writeBinaryValues(out);
    return;
  }

  const unsigned long count = value->size();
  if ((count > 1) || (count == 0)) out->write("[ ");

  out->incrementIndent();

  for (unsigned long i=0; i < count; i++) {
    this->write1Value(out, i);

    if (i != count-1) {
      if (((i+1) % this->getNumValuesPerLine()) == 0) {
        out->write(",\n");
        out->indent();
        // for alignment
        out->write("  ");
      }
      else {
        out->write(", ");
      }
    }
  }
  if ((count > 1) || (count == 0)) out->write(" ]");

  out->decrementIndent();
}

void SoSFMeshPointArray::writeBinaryValues(SoOutput * out) const
{
  assert(out->isBinary());

  const unsigned int count = (unsigned int)value->size();
  out->write(count);
  for (unsigned int i=0; i < count; i++) this->write1Value(out, i);
}

void SoSFMeshPointArray::write1Value(SoOutput * out, unsigned long idx) const
{
  const MeshCore::MeshPoint& v = (*value)[idx];
  out->write(v.x);
  if (!out->isBinary()) out->write(' ');
  out->write(v.y);
  if (!out->isBinary()) out->write(' ');
  out->write(v.z);
}

// -------------------------------------------------------

SO_ELEMENT_SOURCE(SoFCMeshVertexElement);

void SoFCMeshVertexElement::initClass()
{
   SO_ELEMENT_INIT_CLASS(SoFCMeshVertexElement, inherited);
}

void SoFCMeshVertexElement::init(SoState * state)
{
  inherited::init(state);
  this->coords3D = 0;
}

SoFCMeshVertexElement::~SoFCMeshVertexElement()
{
}

void SoFCMeshVertexElement::set(SoState * const state, SoNode * const node, const MeshCore::MeshPointArray * const coords)
{
  SoFCMeshVertexElement * elem = (SoFCMeshVertexElement *)
    SoReplacedElement::getElement(state, classStackIndex, node);
  if (elem) {
    elem->coords3D = coords;
    elem->nodeId = node->getNodeId();
  }
}

const MeshCore::MeshPointArray * SoFCMeshVertexElement::get(SoState * const state)
{
  return SoFCMeshVertexElement::getInstance(state)->coords3D;
}

const SoFCMeshVertexElement * SoFCMeshVertexElement::getInstance(SoState * state)
{
  return (const SoFCMeshVertexElement *) SoElement::getConstElement(state, classStackIndex);
}

void SoFCMeshVertexElement::print(FILE * /* file */) const
{
}

// -------------------------------------------------------

SO_NODE_SOURCE(SoFCMeshVertex);

/*!
  Constructor.
*/
SoFCMeshVertex::SoFCMeshVertex(void)
{
  SO_NODE_CONSTRUCTOR(SoFCMeshVertex);

  SO_NODE_ADD_FIELD(point, (0));
}

/*!
  Destructor.
*/
SoFCMeshVertex::~SoFCMeshVertex()
{
}

// Doc from superclass.
void SoFCMeshVertex::initClass(void)
{
  SO_NODE_INIT_CLASS(SoFCMeshVertex, SoNode, "Node");

  SO_ENABLE(SoGetBoundingBoxAction, SoFCMeshVertexElement);
  SO_ENABLE(SoGLRenderAction, SoFCMeshVertexElement);
  SO_ENABLE(SoPickAction, SoFCMeshVertexElement);
  SO_ENABLE(SoCallbackAction, SoFCMeshVertexElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoFCMeshVertexElement);
}

// Doc from superclass.
void SoFCMeshVertex::doAction(SoAction * action)
{
  SoFCMeshVertexElement::set(action->getState(), this, point.getValue());
//  SoCoordinateElement::set3(action->getState(), this,
//                            point.getNum(), point.getValues(0));
}

// Doc from superclass.
void SoFCMeshVertex::GLRender(SoGLRenderAction * action)
{
  SoFCMeshVertex::doAction(action);
}

// Doc from superclass.
void SoFCMeshVertex::callback(SoCallbackAction * action)
{
  SoFCMeshVertex::doAction(action);
}

// Doc from superclass.
void SoFCMeshVertex::pick(SoPickAction * action)
{
  SoFCMeshVertex::doAction(action);
}

// Doc from superclass.
void SoFCMeshVertex::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoFCMeshVertex::doAction(action);
}

// Doc from superclass.
void SoFCMeshVertex::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoFCMeshVertex::doAction(action);
}

