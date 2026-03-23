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
  \class SoVertexProperty SoVertexProperty.h Inventor/nodes/SoVertexProperty.h
  \brief The SoVertexProperty class collects the functionality of various appearance nodes.

  \ingroup coin_nodes

  Instead of reading data from the current state stack of the
  scene graph traversal, nodes inheriting SoVertexShape can be set up
  with an SoVertexProperty node in the SoVertexShape::vertexProperty
  field. Coordinates, normals, texture coordinates and material /
  color information will then be fetched from the vertex shape's
  SoVertexProperty node instead of from the state stack.

  The SoVertexProperty node provides fields for duplicating the
  functionality of all these other Inventor node types: SoCoordinate3,
  SoTextureCoordinate2, SoTextureCoordinate3, SoNormal, SoPackedColor,
  SoMaterialBinding and SoNormalBinding.


  The SoVertexProperty node was introduced fairly late in the design
  of the Inventor API by SGI. The idea behind it was to provide a
  means to specify the necessary data for vertex shape derived nodes
  which would be more efficient to work with than fetching the data
  from the traversal state stack.

  In practice, the effect is not at all very noticeable. Since the use
  of SoVertexProperty nodes in the SoVertexShape::vertexProperty field
  somewhat breaks with the basic design of the Open Inventor API (the
  SoVertexProperty data is not pushed to the traversal state stack),
  you might be better off design-wise by using the usual mechanisms,
  i.e. by setting up the individual nodes SoVertexProperty "collects".

  One of the drawbacks will for instance be that it is not possible to
  share \e parts of the SoVertexProperty node between several shapes,
  which is something that can easily be done when setting up
  individual state changing nodes in the scene graph.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    VertexProperty {
        vertex [  ]
        normal [  ]
        texCoord [  ]
        orderedRGBA [  ]
        texCoord3 [  ]
        normalBinding PER_VERTEX_INDEXED
        materialBinding OVERALL
    }
  \endcode

  \since SGI Inventor 2.1
  \sa SoVertexShape
  \sa SoCoordinate3, SoTextureCoordinate2, SoTextureCoordinate3, SoNormal
  \sa SoPackedColor
  \sa SoMaterialBinding, SoNormalBinding
*/
// FIXME: should have a usage-example in the class-doc. 20020109 mortene.

// FIXME: this class is really not optimally supported in Coin. Each
// vertex shape node should support this node with internal handling
// to harvest the expected efficiency gains.  Right now we just push
// its values on the stack. ????-??-?? pederb.

#include <Inventor/nodes/SoVertexProperty.h>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoGLNormalElement.h>
#include <Inventor/elements/SoGLMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"
#include "rendering/SoVBO.h"

/*!
  \enum SoVertexProperty::Binding

  The binding types available for our SoVertexProperty::normalBinding
  and SoVertexProperty::materialBinding fields.

  For a detailed explanation of each of the enumeration value binding
  types, see the documentation of the SoMaterialBinding node.
*/


/*!
  \var SoMFVec3f SoVertexProperty::vertex

  This field sets up vertex coordinates in the same manner as
  SoCoordinate3::point.

  By default the field contains no coordinates.

  \sa SoCoordinate3
*/
/*!
  \var SoMFVec2f SoVertexProperty::texCoord

  Same functionality as SoTextureCoordinate2::point.  By default the
  field contains no coordinates.

  \sa SoTextureCoordinate2
*/

/*!
  \var SoMFInt32 SoVertexProperty::textureUnit

  The texture unit(s) for the texture coordinates. By default this field
  contains one value, 0, and texture coordinates are then sent to
  texture unit 0. It's possible to supply multiple values in this field,
  and the texture coordinates in texCoord or texCoord3 will then be split
  into those units. The first totalnum/numunits coordinates will be sent
  to the first unit specified, the next totalnum/numunits coordinates will
  be sent to the second unit in this field, etc.
  
  \sa SoTextureCoordinate2, SoTextureUnit
  \since Coin 4.0
*/

// FIXME: this field was added between TGS Inventor 2.5 and 2.6, and
// between Coin 1.0 and Coin 2.0. This means it must get special
// handling when exporting .iv-files, with regard to what header we
// can put on the output. See also item #003 in the Coin/docs/todo.txt
// file. 20030519 mortene.
/*!
  \var SoMFVec3f SoVertexProperty::texCoord3

  Same functionality as SoTextureCoordinate3::point.  By default the
  field contains no coordinates.

  \sa SoTextureCoordinate3
  \since Coin 2.0
  \since TGS Inventor 2.6
*/

/*!
  \var SoMFVec3f SoVertexProperty::normal

  This field defines a set of normal vectors in the same manner as
  SoNormal::vector.  By default the field contains no vectors.

  \sa SoNormal
*/
/*!
  \var SoSFEnum SoVertexProperty::normalBinding

  Defines how to bind the normals specified in the
  SoVertexProperty::normal set to the parts of the "owner" shape.
  Must be one of the values in the SoVertexProperty::Binding enum.

  Default value of the field is SoVertexProperty::PER_VERTEX_INDEXED.

  \sa SoNormalBinding
*/
/*!
  \var SoMFUInt32 SoVertexProperty::orderedRGBA

  A set of "packed" 32-bit diffuse color plus transparency
  values. Works in the same manner as the SoPackedColor::orderedRGBA
  field.

  By default the field has no data.

  \sa SoPackedColor
*/
/*!
  \var SoSFEnum SoVertexProperty::materialBinding

  Defines how to bind the color values specified in the
  SoVertexProperty::orderedRGBA set to the parts of the "owner" shape.
  Must be one of the values in the SoVertexProperty::Binding enum.

  Default value of the field is SoVertexProperty::OVERALL.

  \sa SoMaterialBinding
*/

class SoVertexPropertyP {
 public:
  SoVertexPropertyP(void) 
    : vertexvbo(NULL),
      normalvbo(NULL),
      colorvbo(NULL)
  {
    this->checktransparent = FALSE;
    this->transparent = FALSE;
  }
  ~SoVertexPropertyP() {
    for (int i = 0; i < this->texcoordvbo.getLength(); i++) {
      delete this->texcoordvbo[i];
    }
    delete this->vertexvbo;
    delete this->normalvbo;
    delete this->colorvbo;
  }
  
  SoVertexProperty * master;
  SbBool transparent;
  SbBool checktransparent;

  SoVBO * vertexvbo;
  SoVBO * normalvbo;
  SoVBO * colorvbo;

  SbList<SoVBO*> texcoordvbo;
};

#define PRIVATE(obj) obj->pimpl

SO_NODE_SOURCE(SoVertexProperty);

/*!
  Constructor.
*/
SoVertexProperty::SoVertexProperty(void)
{
  PRIVATE(this) = new SoVertexPropertyP;

  SO_NODE_INTERNAL_CONSTRUCTOR(SoVertexProperty);

  SO_NODE_ADD_EMPTY_MFIELD(vertex);
  SO_NODE_ADD_EMPTY_MFIELD(normal);
  SO_NODE_ADD_EMPTY_MFIELD(texCoord);
  SO_NODE_ADD_EMPTY_MFIELD(orderedRGBA);
  // FIXME: this field was added in TGS Inventor 2.6 and Coin
  // 2.0. This should have repercussions for file format
  // compatibility. 20030227 mortene.
  SO_NODE_ADD_EMPTY_MFIELD(texCoord3);
  SO_NODE_ADD_FIELD(textureUnit, (0));

  SO_NODE_ADD_FIELD(normalBinding, (SoVertexProperty::PER_VERTEX_INDEXED));
  SO_NODE_ADD_FIELD(materialBinding, (SoVertexProperty::OVERALL));

  SO_NODE_DEFINE_ENUM_VALUE(Binding, OVERALL);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_PART);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_PART_INDEXED);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_FACE);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_FACE_INDEXED);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_VERTEX);
  SO_NODE_DEFINE_ENUM_VALUE(Binding, PER_VERTEX_INDEXED);

  SO_NODE_SET_SF_ENUM_TYPE(normalBinding, Binding);
  SO_NODE_SET_SF_ENUM_TYPE(materialBinding, Binding);

}

/*!
  Destructor.
*/
SoVertexProperty::~SoVertexProperty()
{
  delete PRIVATE(this);
}

// Documented in superclass.
/*!
  \copybrief SoBase::initClass(void)
*/
void
SoVertexProperty::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVertexProperty, SO_FROM_INVENTOR_2_1);

  SO_ENABLE(SoGetBoundingBoxAction, SoCoordinateElement);

  SO_ENABLE(SoGLRenderAction, SoGLCoordinateElement);
  SO_ENABLE(SoGLRenderAction, SoMaterialBindingElement);
  SO_ENABLE(SoGLRenderAction, SoNormalBindingElement);
  SO_ENABLE(SoGLRenderAction, SoGLNormalElement);
  SO_ENABLE(SoGLRenderAction, SoGLMultiTextureCoordinateElement);

  SO_ENABLE(SoPickAction, SoCoordinateElement);
  SO_ENABLE(SoPickAction, SoMaterialBindingElement);
  SO_ENABLE(SoPickAction, SoNormalBindingElement);
  SO_ENABLE(SoPickAction, SoNormalElement);
  SO_ENABLE(SoPickAction, SoMultiTextureCoordinateElement);

  SO_ENABLE(SoCallbackAction, SoCoordinateElement);
  SO_ENABLE(SoCallbackAction, SoMaterialBindingElement);
  SO_ENABLE(SoCallbackAction, SoNormalBindingElement);
  SO_ENABLE(SoCallbackAction, SoNormalElement);
  SO_ENABLE(SoCallbackAction, SoMultiTextureCoordinateElement);

  SO_ENABLE(SoGetPrimitiveCountAction, SoCoordinateElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoMaterialBindingElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoNormalBindingElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoNormalElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoMultiTextureCoordinateElement);
}

// Documented in superclass.
void
SoVertexProperty::getBoundingBox(SoGetBoundingBoxAction * action)
{
  if (vertex.getNum() > 0) {
    SoCoordinateElement::set3(action->getState(), this,
                              vertex.getNum(), vertex.getValues(0));
  }
}

// Documented in superclass.
void
SoVertexProperty::GLRender(SoGLRenderAction * action)
{
  SoVertexProperty::doAction(action);
}

#define TEST_OVERRIDE(bit, flags) ((SoOverrideElement::bit & (flags)) != 0)

// Documented in superclass.
void
SoVertexProperty::doAction(SoAction *action)
{
  SoState * state = action->getState();

  uint32_t overrideflags = SoOverrideElement::getFlags(state);
  SbBool glrender = action->isOfType(SoGLRenderAction::getClassTypeId());
  if (glrender) SoBase::staticDataLock();

  if (PRIVATE(this)->checktransparent) {
    PRIVATE(this)->checktransparent = FALSE;
    PRIVATE(this)->transparent = FALSE;
    int n = this->orderedRGBA.getNum();
    for (int i = 0; i < n; i++) {
      if ((this->orderedRGBA[i] & 0xff) != 0xff) {
        PRIVATE(this)->transparent = TRUE;
        break;
      }
    }
  }
  const SbBool shouldcreatevbo = glrender ? SoGLVBOElement::shouldCreateVBO(state, this->vertex.getNum()) : FALSE;
  this->updateVertex(state, glrender, shouldcreatevbo);
  this->updateNormal(state, overrideflags, glrender, shouldcreatevbo);
  this->updateMaterial(state, overrideflags, glrender, shouldcreatevbo);
  this->updateTexCoord(state, glrender, shouldcreatevbo);

  if (glrender) SoBase::staticDataUnlock();
}

// Documented in superclass.
void
SoVertexProperty::callback(SoCallbackAction *action)
{
  SoVertexProperty::doAction((SoAction*)action);
}

// Documented in superclass.
void
SoVertexProperty::pick(SoPickAction *action)
{
  SoVertexProperty::doAction((SoAction*)action);
}

// Documented in superclass.
void
SoVertexProperty::getPrimitiveCount(SoGetPrimitiveCountAction *action)
{
  if (vertex.getNum() > 0) {
    SoCoordinateElement::set3(action->getState(), this,
                              vertex.getNum(), vertex.getValues(0));
  }
}

// Documented in superclass. Overridden to check for transparency when
// orderedRGBA changes.
void
SoVertexProperty::notify(SoNotList *list)
{
  SoField *f = list->getLastField();
  if (f == &this->orderedRGBA) {
    PRIVATE(this)->checktransparent = TRUE;
  }
  inherited::notify(list);
}


void 
SoVertexProperty::updateVertex(SoState * state, SbBool glrender, SbBool vbo)
{
  int num = this->vertex.getNum();

  if (num > 0) {    
    SoCoordinateElement::set3(state, this, num,
                              this->vertex.getValues(0));
    if (glrender) {
      if (vbo) {
        SbBool dirty = FALSE;
        if (PRIVATE(this)->vertexvbo == NULL) {
          PRIVATE(this)->vertexvbo = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW); 
          dirty =  TRUE;
        }
        else if (PRIVATE(this)->vertexvbo->getBufferDataId() != this->getNodeId()) {
          dirty = TRUE;
        }
        if (dirty) {
          PRIVATE(this)->vertexvbo->setBufferData(this->vertex.getValues(0),
                                                  num*sizeof(SbVec3f),
                                                  this->getNodeId());
        }
      }
      else if (PRIVATE(this)->vertexvbo && PRIVATE(this)->vertexvbo->getBufferDataId()) {
        // clear buffers to deallocate VBO memory
        PRIVATE(this)->vertexvbo->setBufferData(NULL, 0, 0);
      }
      SoGLVBOElement::setVertexVBO(state, vbo ? PRIVATE(this)->vertexvbo : NULL);
    }
  }
}

void 
SoVertexProperty::updateTexCoord(SoState * state, SbBool glrender, SbBool vbo)
{
  const int numvertex = this->vertex.getNum();
  int num = this->texCoord3.getNum();
  int dim = 3;
  const SbVec3f * tc3 = num ? this->texCoord3.getValues(0) : NULL;
  const SbVec2f * tc2 = NULL;
  if (num == 0) {
    num = this->texCoord.getNum();
    dim = 2;
    if (num) {
      tc2 = this->texCoord.getValues(0);
    }
  }
  if (num > 0) {
    const int numunits = this->textureUnit.getNum();  
    const int numperunit = num / numunits;

    if ((num % numunits) != 0) {
      SoDebugError::postWarning("SoVertexProperty::updateTexCoord",
                                "Wrong number of texture coordinates. The number of texture "
                                "coordinates must be dividable by the number of units in "
                                "the textureUnit field.");
    } 
    else {
      for (int i = 0; i < numunits; i++) {
        int32_t unit = this->textureUnit[i];
        if (glrender) {
          // it's important to call this _before_ setting the coordinates
          // on the state.
          SoGLMultiTextureCoordinateElement::setTexGen(state,
                                                       this, unit, NULL);
        }
        if (dim == 2) {
          SoMultiTextureCoordinateElement::set2(state, this, unit, numperunit,
                                                tc2 + i*numperunit);
        }
        else {
          SoMultiTextureCoordinateElement::set3(state, this, unit, numperunit,
                                                tc3 + i*numperunit);
        }
      
        if (glrender) {
          SbBool setvbo = FALSE;

          if (i >= PRIVATE(this)->texcoordvbo.getLength()) {
            PRIVATE(this)->texcoordvbo.append(NULL);
          }
          if ((numperunit == numvertex) && vbo) {
            SbBool dirty = FALSE;
            setvbo = TRUE;
            if (PRIVATE(this)->texcoordvbo[i] == NULL) {
              PRIVATE(this)->texcoordvbo[i] = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW); 
              dirty =  TRUE;
            }
            else if (PRIVATE(this)->texcoordvbo[i]->getBufferDataId() != this->getNodeId()) {
              dirty = TRUE;
            }
            if (dirty) {
              if (dim == 2) {
                PRIVATE(this)->texcoordvbo[i]->setBufferData(tc2 + i * numperunit,
                                                             numperunit*sizeof(SbVec2f),
                                                             this->getNodeId());
              }
              else {
                PRIVATE(this)->texcoordvbo[i]->setBufferData(tc3 + i * numperunit,
                                                             numperunit*sizeof(SbVec3f),
                                                             this->getNodeId());
              }
            }
          }
          else if (PRIVATE(this)->texcoordvbo[i] && 
                   PRIVATE(this)->texcoordvbo[i]->getBufferDataId()) {
            // clear buffers to deallocate VBO memory
            PRIVATE(this)->texcoordvbo[i]->setBufferData(NULL, 0, 0);
          }
          SoGLVBOElement::setTexCoordVBO(state, 0, setvbo ? PRIVATE(this)->texcoordvbo[i] : NULL);
        }
      }
    }
  }
}
  
void 
SoVertexProperty::updateNormal(SoState * state, uint32_t overrideflags, SbBool glrender, SbBool vbo)
{
  const int numvertex = this->vertex.getNum();
  const int num = this->normal.getNum();
  if (num > 0 && !TEST_OVERRIDE(NORMAL_VECTOR, overrideflags)) {
    SoNormalElement::set(state, this, num,
                         this->normal.getValues(0));
    if (this->isOverride()) {
      SoOverrideElement::setNormalVectorOverride(state, this, TRUE);
    }
    if (glrender) {
      SbBool setvbo = FALSE;
      if ((num == numvertex) && vbo) {
        SbBool dirty = FALSE;
        setvbo = TRUE;
        if (PRIVATE(this)->normalvbo == NULL) {
          PRIVATE(this)->normalvbo = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW); 
          dirty =  TRUE;
        }
        else if (PRIVATE(this)->normalvbo->getBufferDataId() != this->getNodeId()) {
          dirty = TRUE;
        }
        if (dirty) {
          PRIVATE(this)->normalvbo->setBufferData(this->normal.getValues(0),
                                                  num*sizeof(SbVec3f),
                                                  this->getNodeId());
        }
      }
      else if (PRIVATE(this)->normalvbo && PRIVATE(this)->normalvbo->getBufferDataId()) {
        // clear buffers to deallocate VBO memory
        PRIVATE(this)->normalvbo->setBufferData(NULL, 0, 0);
      }
      SoGLVBOElement::setNormalVBO(state, setvbo ? PRIVATE(this)->normalvbo : NULL);
    }
  }
  if (this->normal.getNum() > 0 && !TEST_OVERRIDE(NORMAL_BINDING, overrideflags)) {
    SoNormalBindingElement::set(state, this,
                                (SoNormalBindingElement::Binding)
                                this->normalBinding.getValue());
    if (this->isOverride()) {
      SoOverrideElement::setNormalBindingOverride(state, this, TRUE);
    }
  }
}

void 
SoVertexProperty::updateMaterial(SoState * state, uint32_t overrideflags, SbBool glrender, SbBool vbo)
{
  const int numvertex = this->vertex.getNum();
  int num = this->orderedRGBA.getNum();
  if (num > 0 && 
      !TEST_OVERRIDE(DIFFUSE_COLOR, overrideflags)) {
    
    SoLazyElement::setPacked(state, this, num,
                             this->orderedRGBA.getValues(0),
                             PRIVATE(this)->transparent);
    if (this->isOverride()) {
      SoOverrideElement::setDiffuseColorOverride(state, this, TRUE);
    }
    if (glrender) {
      SbBool setvbo = FALSE;
      if ((num == numvertex) && vbo) {
        SbBool dirty = FALSE;
        setvbo = TRUE;
        if (PRIVATE(this)->colorvbo == NULL) {
          PRIVATE(this)->colorvbo = new SoVBO(GL_ARRAY_BUFFER, GL_STATIC_DRAW);
          dirty = TRUE;
        }
        else if (PRIVATE(this)->colorvbo->getBufferDataId() != this->getNodeId()) {
          dirty = TRUE;
        }
        if (dirty) {
          if (coin_host_get_endianness() == COIN_HOST_IS_BIGENDIAN) {
            PRIVATE(this)->colorvbo->setBufferData(this->orderedRGBA.getValues(0),
                                                   num*sizeof(uint32_t),
                                                   this->getNodeId());
          }
          else {
            const uint32_t * src = this->orderedRGBA.getValues(0);
            uint32_t * dst = (uint32_t*) 
              PRIVATE(this)->colorvbo->allocBufferData(num*sizeof(uint32_t), 
                                                       this->getNodeId());  
            for (int i = 0; i < num; i++) {
              uint32_t tmp = src[i];
              dst[i] = 
                (tmp << 24) |
                ((tmp & 0xff00) << 8) |
                ((tmp & 0xff0000) >> 8) |
                (tmp >> 24);
            }
          }
        }
      }
      else if (PRIVATE(this)->colorvbo) {
        PRIVATE(this)->colorvbo->setBufferData(NULL, 0, 0);
      }
      SoGLVBOElement::setColorVBO(state, setvbo ? PRIVATE(this)->colorvbo : NULL);
    }
  }
  if (num && !TEST_OVERRIDE(MATERIAL_BINDING, overrideflags)) {
    SoMaterialBindingElement::set(state, this,
                                  (SoMaterialBindingElement::Binding)
                                  this->materialBinding.getValue());
    if (this->isOverride()) {
      SoOverrideElement::setMaterialBindingOverride(state, this, TRUE);
    }
  }
}

#undef PRIVATE
#undef TEST_OVERRIDE
