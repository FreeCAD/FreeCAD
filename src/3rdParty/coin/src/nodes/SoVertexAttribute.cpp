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

#include <Inventor/nodes/SoVertexAttribute.h>
#include <Inventor/elements/SoGLVertexAttributeElement.h>

#include <memory>

#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/fields/SoFieldData.h>
#include <Inventor/fields/SoMFFloat.h>
#include <Inventor/fields/SoMFVec2f.h>
#include <Inventor/fields/SoMFVec3f.h>
#include <Inventor/fields/SoMFVec4f.h>
#include <Inventor/fields/SoMFShort.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/misc/SoGLDriverDatabase.h>

#include "rendering/SoVBO.h"
#include "nodes/SoSubNodeP.h"
#include "elements/SoVertexAttributeData.h"

/*!
  \class SoVertexAttribute SoVertexAttribute.h Inventor/nodes/SoVertexAttribute.h
  \brief A generic node for providing GL vertex attributes of various types.

  The SoVertexAttribute nodes are used with the SoShaderProgram node to
  send vertex attributes to the vertex shader.

  \verbatim
  #Inventor V2.1 ascii

  Separator {
     ShaderProgram {
        shaderObject [
                      VertexShader {
                         sourceProgram "vertex.glsl"
                      }
                   ]
     }

     Coordinate3 {
        point [ 0 0 0, 1 0 0, 1 1 0 ]
     }

     VertexAttribute {
        typeName "SoMFVec3f"
        name "color"
        values [ 1.0 0.0 0.0,
                 0.0 1.0 0.0,
                 0.0 0.0 1.0 ]
     }

     IndexedFaceSet {
        coordIndex [ 0, 1, 2, -1 ]
     }
  }
  \endverbatim

The vertex shader (vertex.glsl)
\code
  attribute vec3 color;

  void main(void)
  {
    gl_Position = ftransform();
    gl_FrontColor = vec4(color, 1.0);
  }
  \endcode

  \sa SoVertexAttributeBinding
  \ingroup coin_shaders
  \COIN_CLASS_EXTENSION
  \since Coin 3.0
*/

class SoVertexAttributeP {
public:
  void setDataPtr(void);

  int size;
  GLenum gltype;
  SbBool isreading;
  SoMFFloat dummyfield;
  std::unique_ptr<SoMField> valuesfield;
  std::unique_ptr<SoFieldData> fielddata;
  SoVertexAttributeData * attributedata;
  SoVertexAttribute * publ;
}; // SoVertexAttributeP

// *************************************************************************

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->publ)
//#define DMSG SoDebugError::postInfo
#define DMSG nop

inline void nop(const char *, ...) { }

// *************************************************************************

// overridden to return the private fielddata
const SoFieldData *
SoVertexAttribute::getFieldData(void) const
{
  return PRIVATE(this)->fielddata.get();
}

/*!
  Creates a new instance of the class type corresponding to the SoType object.
*/
void *
SoVertexAttribute::createInstance(void)
{
  return new SoVertexAttribute;
}
// SO_NODE_SOURCE()
// *************************************************************************

SoType SoVertexAttribute::classTypeId STATIC_SOTYPE_INIT;

/*!
  \copybrief SoNode::initClass(void)
*/
void
SoVertexAttribute::initClass(void)
{
  SoVertexAttribute::classTypeId =
    SoType::createType(SoNode::getClassTypeId(),
                       SbName("VertexAttribute"),
                       SoVertexAttribute::createInstance,
                       SoNode::nextActionMethodIndex++);

  SoNode::setCompatibilityTypes(SoVertexAttribute::getClassTypeId(), SO_FROM_COIN_3_0);
  SO_ENABLE(SoGLRenderAction, SoGLVertexAttributeElement);
}

/*!
  Constructor.
*/
SoVertexAttribute::SoVertexAttribute(void)
{
  // We don't use SO_NODE_INTERNAL_CONSTRUCTOR(SoVertexAttribute) here because
  // the fieldData setup has to be overridden in custom ways for this node...

  this->setNodeType(SoNode::COIN_3_0);
  this->isBuiltIn = TRUE;
  assert(SoVertexAttribute::classTypeId != SoType::badType());

  this->name.setContainer(this);
  this->typeName.setContainer(this);

  this->initFieldData();

  // initialize attribute data
  PRIVATE(this)->publ = this;
  PRIVATE(this)->isreading = FALSE;
  SoVertexAttributeData* attributedata = new SoVertexAttributeData;
  attributedata->name = SbName::empty();
  attributedata->index = -1;
  attributedata->state = NULL;
  attributedata->data = NULL;
  attributedata->shaderobj = 0;
  attributedata->nodeid = NULL;
  attributedata->vbo = NULL;
  PRIVATE(this)->attributedata = attributedata;
}

/*!
  Destructor.
*/
SoVertexAttribute::~SoVertexAttribute(void)
{
  delete PRIVATE(this)->attributedata;
}

/*!
  This static method returns the SoType object associated with
  objects of this class.
*/
SoType
SoVertexAttribute::getClassTypeId(void)
{
  return SoVertexAttribute::classTypeId;
}

// Doc in superclass
SoType
SoVertexAttribute::getTypeId(void) const
{
  return SoVertexAttribute::classTypeId;
}

void
SoVertexAttribute::initFieldData(void)
{
  // initialize fielddata
  PRIVATE(this)->fielddata.reset(new SoFieldData);
  PRIVATE(this)->fielddata->addField(this, "name", &this->name);
  PRIVATE(this)->fielddata->addField(this, "typeName", &this->typeName);
}

void
SoVertexAttribute::doAction(SoAction * action)
{
  PRIVATE(this)->attributedata->name = this->name.getValue();
  PRIVATE(this)->attributedata->data = PRIVATE(this)->valuesfield.get();
  PRIVATE(this)->attributedata->state = action->getState();
  PRIVATE(this)->attributedata->nodeid = this;

  SoVertexAttributeElement::add(action->getState(), PRIVATE(this)->attributedata);
}

void
SoVertexAttribute::GLRender(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  const cc_glglue * glue = cc_glglue_instance(SoGLCacheContextElement::get(state));

  // check for gl vertex attribute support
  SbBool vertex_shader_supported = SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_VERTEX_SHADER);
  SbBool opengl_version_match = cc_glglue_glversion_matches_at_least(glue, 2, 0, 0);

  if (!vertex_shader_supported || !opengl_version_match) {
    static SbBool first = TRUE;
    if (first) {
      first = FALSE;
      SbString msg("\nUnable to use Vertex Attributes:\n");
      if (!opengl_version_match) msg += "OpenGL version < 2.0\n";
      if (!vertex_shader_supported) msg += "GL_ARB_vertex_shader extension not supported\n";
      SoDebugError::post("SoVertexAttribute::GLRender", msg.getString());
    }
    return;
  }

  // check if there was an SoShaderProgram node before this node in
  // the scene graph
  SoGLShaderProgram * shaderprogram =
    static_cast<SoGLShaderProgram *>(SoGLShaderProgramElement::get(state));

  if (!shaderprogram) {
    SoDebugError::post("SoVertexAttribute::GLRender",
                       "SoShaderProgram node not found in scene");
    return;
  }

  // update element
  PRIVATE(this)->setDataPtr();
  SoVertexAttribute::doAction((SoAction *) action);

  // check if vbo rendering should be used and create vbo if yes
  SbBool setvbo = FALSE;
  // get data length but make sure that field has been created before (after typeName was set)
  int num = PRIVATE(this)->attributedata->data ? PRIVATE(this)->attributedata->data->getNum() : 0;
  SoBase::staticDataLock();
  if (SoGLVBOElement::shouldCreateVBO(state, num)) {
    SbBool dirty = FALSE;
    setvbo = TRUE;
    if (PRIVATE(this)->attributedata->vbo == NULL) {
      PRIVATE(this)->attributedata->vbo = new SoVBO;
      dirty = TRUE;
    }
    else if (PRIVATE(this)->attributedata->vbo->getBufferDataId()
             != this->getNodeId()) {
      dirty = TRUE;
    }
    if (dirty) {
      PRIVATE(this)->attributedata->vbo->setBufferData(PRIVATE(this)->attributedata->dataptr,
                                                       PRIVATE(this)->attributedata->size,
                                                       this->getNodeId());
    }
  }
  else if (PRIVATE(this)->attributedata->vbo &&
           PRIVATE(this)->attributedata->vbo->getBufferDataId()) {
    // clear buffers to deallocate VBO memory
    PRIVATE(this)->attributedata->vbo->setBufferData(NULL, 0, 0);
  }
  SoBase::staticDataUnlock();
}

SoMField *
SoVertexAttribute::getValuesField(void) const
{
  return PRIVATE(this)->valuesfield.get();
}

void
SoVertexAttribute::write(SoWriteAction * action)
{
  SoOutput * out = action->getOutput();

  if (out->getStage() == SoOutput::COUNT_REFS) {
    this->addWriteReference(out, FALSE);
  }
  else if (out->getStage() == SoOutput::WRITE) {
    if (this->writeHeader(out, FALSE, FALSE)) return;

     this->typeName.write(out, "typeName");
     this->name.write(out, "name");
     SoMField * values = PRIVATE(this)->valuesfield.get();
     if (values) {
       values->write(out, "values");
     }
     this->writeFooter(out);
  }
}

SbBool
SoVertexAttribute::readInstance(SoInput * in, unsigned short COIN_UNUSED_ARG(flags))
{
  PRIVATE(this)->isreading = TRUE;
  // avoid triggering in the notify()-function while reading the file.

  static const SbName typenamekey("typeName");
  static const SbName namekey("name");
  static const SbName valueskey("values");

  SbName fieldname(SbName::empty());
  SbBool ok = in->read(fieldname, TRUE);
  SbBool err = FALSE;

  while (ok && !err) {
    DMSG("SoVertexAttribute::readInstance", "found fieldname '%s'",
         fieldname.getString());

    if (fieldname == typenamekey) {
      err = !this->typeName.read(in, typenamekey);
      if (!err) {
        if (PRIVATE(this)->valuesfield.get()) {
          PRIVATE(this)->valuesfield.reset(NULL);
        }
        SoType datatype = SoType::fromName(this->typeName.getValue());
        PRIVATE(this)->valuesfield.reset((SoMField *)datatype.createInstance());
        if (PRIVATE(this)->valuesfield.get()) {
          PRIVATE(this)->fielddata->addField(this, valueskey,
                                             PRIVATE(this)->valuesfield.get());
        }
      }
    }
    else if (fieldname == namekey) {
      err = !this->name.read(in, namekey);
      if (!err) {
        DMSG("SoVertexAttribute::readInstance", "read the value '%s'",
             this->name.getValue().getString());
      }
    }
    else if (fieldname == valueskey) {
      if (PRIVATE(this)->valuesfield.get()) {
        err = !PRIVATE(this)->valuesfield->read(in, valueskey);
        if (!err) {
          DMSG("SoVertexAttribute::readInstance", "read in %d values",
               PRIVATE(this)->valuesfield->getNum());
        }
      }
    }

    if (err) {
      DMSG("SoVertexAttribute::readInstance",
           "error reading field contents for field %s", fieldname.getString());
    }
    else {
      ok = in->read(fieldname, TRUE);
    }
  }

  PRIVATE(this)->isreading = FALSE;

  if (!err) {
    if (fieldname != "") in->putBack(fieldname.getString());
  }

  return !err;
}

//
// overridden to update the valuesfield and fielddata whenever the
// type field changes
//
void
SoVertexAttribute::notify(SoNotList * list)
{
  if (PRIVATE(this)->isreading) return; // ignore notification while reading

  SoField * field = list->getLastField();
  if (field == &this->typeName) {
    SoType datatype = SoType::fromName(this->typeName.getValue());
    PRIVATE(this)->valuesfield.reset((SoMField *)datatype.createInstance());
    PRIVATE(this)->fielddata->addField(this, "values",
                                       PRIVATE(this)->valuesfield.get());
  }
  inherited::notify(list);
}

// Doc in superclass
void
SoVertexAttribute::copyContents(const SoFieldContainer * from,
                                SbBool copyConn)
{
  assert(from->isOfType(SoVertexAttribute::getClassTypeId()));

  // copy fields
  const SoFieldData * src = from->getFieldData();
  const int n = src->getNumFields();
  for (int i = 0; i < n; i++) {
    const SoField * f = src->getField(from, i);
    SoField * cp = (SoField*) f->getTypeId().createInstance();
    cp->setFieldType(f->getFieldType());
    cp->setContainer(this);
    PRIVATE(this)->fielddata->addField(this, src->getFieldName(i), cp);
  }
  // disable notification while copying the field values to avoid that
  // the values field is recreated.
  this->enableNotify(false);
  inherited::copyContents(from, copyConn);
  this->enableNotify(true);
}

void
SoVertexAttributeP::setDataPtr(void)
{
  SoType datatype = SoType::fromName(PUBLIC(this)->typeName.getValue().getString());
  this->attributedata->type = datatype;

  if (datatype == SoMFFloat::getClassTypeId()) {
    SoMFFloat * mfield = static_cast<SoMFFloat *>(this->valuesfield.get());
    this->attributedata->size = mfield->getNum() * sizeof(float);
    this->attributedata->gltype = GL_FLOAT;
    this->attributedata->num = 1;
    this->attributedata->dataptr = mfield->getValues(0);

  } else if (datatype == SoMFVec2f::getClassTypeId()) {
    SoMFVec2f * mfield = static_cast<SoMFVec2f *>(this->valuesfield.get());
    this->attributedata->size = mfield->getNum() * sizeof(SbVec2f);
    this->attributedata->gltype = GL_FLOAT;
    this->attributedata->num = 2;
    this->attributedata->dataptr = mfield->getValues(0);

  } else if (datatype == SoMFVec3f::getClassTypeId()) {
    SoMFVec3f * mfield = static_cast<SoMFVec3f *>(this->valuesfield.get());
    this->attributedata->size = mfield->getNum() * sizeof(SbVec3f);
    this->attributedata->gltype = GL_FLOAT;
    this->attributedata->num = 3;
    this->attributedata->dataptr = mfield->getValues(0);

  } else if (datatype == SoMFVec4f::getClassTypeId()) {
    SoMFVec4f * mfield = static_cast<SoMFVec4f *>(this->valuesfield.get());
    this->attributedata->size = mfield->getNum() * sizeof(SbVec4f);
    this->attributedata->gltype = GL_FLOAT;
    this->attributedata->num = 4;
    this->attributedata->dataptr = mfield->getValues(0);

  } else if (datatype == SoMFShort::getClassTypeId()) {
    SoMFShort * mfield = static_cast<SoMFShort *>(this->valuesfield.get());
    this->attributedata->size = mfield->getNum() * sizeof(short);
    this->attributedata->gltype = GL_SHORT;
    this->attributedata->num = 1;
    this->attributedata->dataptr = mfield->getValues(0);

  } else {
    SoDebugError::postInfo("SoVertexAttributeP::setDataPtr",
                           "Typename '%s' is not a supported type.",
                           PUBLIC(this)->typeName.getValue().getString());
  }
}

#undef PRIVATE
#undef PUBLIC
#undef DMSG
