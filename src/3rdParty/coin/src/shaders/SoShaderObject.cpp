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
  \class SoShaderObject SoShaderObject.h Inventor/nodes/SoShaderObject.h
  \brief The SoShaderObject class is the superclass for all shader classes in Coin.

  See \ref coin_shaders_page "Shaders in Coin" for more information
  on how to set up a scene graph with shaders.

  \ingroup coin_shaders

  \sa SoShaderProgram
*/

/*!
  \var SoSFBool SoShaderObject::isActive

  Enabled/disables the shader. Default value is TRUE.
*/

/*!
  \var SoSFString SoShaderObject::sourceProgram

  The shader program, or a file name if the shader should be loaded from a file.
  If the shader is loaded from a file, the shader type is identified by the
  file extension. .glsl for GLSL shaders, .cg for Cg shaders, and .vp and .fp
  for ARB shaders.
*/


/*!
  \var SoSFEnum SoShaderObject::sourceType

  The type of shader.
*/


/*!
  \enum SoShaderObject::SourceType

  Used for enumerating the shader types in sourceProgram.
*/

/*!
  \var SoShaderObject::SourceType SoShaderObject::ARB_PROGRAM

  Specifies an ARB shader.
*/

/*!
  \var SoShaderObject::SourceType SoShaderObject::CG_PROGRAM

  Specifies a Cg shader program.
*/

/*!
  \var SoShaderObject::SourceType SoShaderObject::GLSL_PROGRAM

  Specifies a GLSL program.
*/

/*!
  \var SoShaderObject::SourceType SoShaderObject::FILENAME

  Shader should be loaded from the file in sourceProgram.
*/


/*!
  \var SoMFNode SoShaderObject::parameter

  The shader program parameters.
*/

#include <Inventor/nodes/SoShaderObject.h>

#include <cassert>

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLShaderProgramElement.h>
#include <Inventor/elements/SoGLMultiTextureImageElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/misc/SoContextHandler.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/nodes/SoFragmentShader.h>
#include <Inventor/nodes/SoShaderParameter.h>
#include <Inventor/nodes/SoVertexShader.h>
#include <Inventor/nodes/SoGeometryShader.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/SoInput.h>
#include <Inventor/lists/SbStringList.h>

#include "nodes/SoSubNodeP.h"
#include "misc/SbHash.h"
#include "shaders/SoGLARBShaderObject.h"
#include "shaders/SoGLCgShaderObject.h"
#include "shaders/SoGLSLShaderObject.h"
#include "shaders/SoGLShaderProgram.h"

// *************************************************************************

class SoShaderObjectP
{
public:
  SoShaderObjectP(SoShaderObject *ownerptr);
  ~SoShaderObjectP();

  void GLRender(SoGLRenderAction *action);
  void render(SoState * state);

  SoGLShaderObject * getGLShaderObject(const uint32_t cachecontext) {
    SoGLShaderObject * obj = NULL;
    if (this->glshaderobjects.get(cachecontext, obj)) return obj;
    return NULL;
  }
  void setGLShaderObject(SoGLShaderObject * obj, const uint32_t cachecontext) {
    SoGLShaderObject * oldshader;
    if (this->glshaderobjects.get(cachecontext, oldshader)) {
      SoGLCacheContextElement::scheduleDeleteCallback(oldshader->getCacheContext(),
                                                      really_delete_object, oldshader);
    }
    (void) this->glshaderobjects.put(cachecontext, obj);
  }
  void deleteGLShaderObjects(void) {
    SbList <uint32_t> keylist;
    this->glshaderobjects.makeKeyList(keylist);
    for (int i = 0; i < keylist.getLength(); i++) {
      SoGLShaderObject * glshader = NULL;
      (void) this->glshaderobjects.get(keylist[i], glshader);
      SoGLCacheContextElement::scheduleDeleteCallback(glshader->getCacheContext(),
                                                      really_delete_object, glshader);
    }
    this->glshaderobjects.clear();
  }
  //
  // Callback from SoGLCacheContextElement
  //
  static void really_delete_object(void * closure, uint32_t COIN_UNUSED_ARG(contextid)) {
    SoGLShaderObject * obj = (SoGLShaderObject*) closure;
    delete obj;
  }
  //
  // callback from SoContextHandler
  //
  static void context_destruction_cb(uint32_t cachecontext, void * userdata) {
    SoShaderObjectP * thisp = (SoShaderObjectP*) userdata;

    SoGLShaderObject * oldshader;
    if (thisp->glshaderobjects.get(cachecontext, oldshader)) {
      // just delete immediately. The context is current
      delete oldshader;
      thisp->glshaderobjects.erase(cachecontext);
    }
  }

  void invalidateParameters(void) {
    SbList <uint32_t> keylist;
    this->glshaderobjects.makeKeyList(keylist);
    for (int i = 0; i < keylist.getLength(); i++) {
      SoGLShaderObject * glshader = NULL;
      (void) this->glshaderobjects.get(keylist[i], glshader);
      glshader->setParametersDirty(TRUE);
    }
  }

  SoShaderObject * owner;
  SoShaderObject::SourceType cachedSourceType;
  SbString cachedSourceProgram;
  SbBool didSetSearchDirectories;
  SbBool shouldload;
  SoNodeSensor *sensor;

  void updateParameters(const uint32_t cachecontext, int start, int num);
  void updateAllParameters(const uint32_t cachecontext);
  void updateCoinParameters(const uint32_t cachecontext, SoState * state);
  void updateStateMatrixParameters(const uint32_t cachecontext, SoState * state);
  SbBool containStateMatrixParameters(void) const;
  void setSearchDirectories(const SbStringList & list);

private:
  static void sensorCB(void *data, SoSensor *);

  SbStringList searchdirectories;
  SbHash<uint32_t, SoGLShaderObject *> glshaderobjects;

  void checkType(void); // sets cachedSourceType
  void readSource(void); // sets cachedSourceProgram depending on sourceType

  SbBool isSupported(SoShaderObject::SourceType sourceType, const cc_glglue * glue);

#if defined(SOURCE_HINT)
  SbString getSourceHint(void) const;
#endif
};

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

SO_NODE_ABSTRACT_SOURCE(SoShaderObject);

// *************************************************************************

/*!
  \copybrief SoNode::initClass(void)
*/
void SoShaderObject::initClass(void)
{
  SO_NODE_INTERNAL_INIT_ABSTRACT_CLASS(SoShaderObject,
                                       SO_FROM_COIN_2_5|SO_FROM_INVENTOR_5_0);
}

/*!
  Constructor.
*/
SoShaderObject::SoShaderObject(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoShaderObject);

  SO_NODE_ADD_FIELD(isActive, (TRUE));

  SO_NODE_DEFINE_ENUM_VALUE(SourceType, ARB_PROGRAM);
  SO_NODE_DEFINE_ENUM_VALUE(SourceType, CG_PROGRAM);
  SO_NODE_DEFINE_ENUM_VALUE(SourceType, GLSL_PROGRAM);
  SO_NODE_DEFINE_ENUM_VALUE(SourceType, FILENAME);

  SO_NODE_ADD_FIELD(sourceType, (FILENAME));
  SO_NODE_SET_SF_ENUM_TYPE(sourceType, SourceType);

  SO_NODE_ADD_FIELD(sourceProgram, (""));
  SO_NODE_ADD_FIELD(parameter, (NULL));
  this->parameter.setNum(0);
  this->parameter.setDefault(TRUE);

  PRIVATE(this) = new SoShaderObjectP(this);
}

/*!
  Destructor
*/
SoShaderObject::~SoShaderObject()
{
  delete PRIVATE(this);
}

// doc from parent
void
SoShaderObject::GLRender(SoGLRenderAction * action)
{
  PRIVATE(this)->GLRender(action);
}

// doc from parent
void
SoShaderObject::search(SoSearchAction * action)
{
  // Include this node in the search.
  SoNode::search(action);
  if (action->isFound()) return;

  // we really can't do this since this node hasn't got an SoChildList
  // instance
#if 0 // disabled, not possible to search under this node
  int numindices;
  const int * indices;
  if (action->getPathCode(numindices, indices) == SoAction::IN_PATH) {
    // FIXME: not implemented -- 20050129 martin
  }
  else { // traverse all shader parameter
    int num = this->parameter.getNum();
    for (int i=0; i<num; i++) {
      SoNode * node = this->parameter[i];
      action->pushCurPath(i, node);
      SoNodeProfiling profiling;
      profiling.preTraversal(action);
      node->search(action);
      profiling.postTraversal(action);
      action->popCurPath();
      if (action->isFound()) return;
    }
  }
#endif // disabled
}

// doc from parent
SbBool
SoShaderObject::readInstance(SoInput * in, unsigned short flags)
{
  PRIVATE(this)->sensor->detach();
  PRIVATE(this)->deleteGLShaderObjects();

  SbBool ret = inherited::readInstance(in, flags);
  if (ret) {
    PRIVATE(this)->setSearchDirectories(SoInput::getDirectories());
  }
  PRIVATE(this)->sensor->attach(this);

  return ret;
}

/*!
  Returns the shader type detected in source program.
*/
SoShaderObject::SourceType
SoShaderObject::getSourceType(void) const
{
  return PRIVATE(this)->cachedSourceType;
}

/*!
  Returns the actual shader program.
*/
SbString SoShaderObject::getSourceProgram(void) const
{
  return PRIVATE(this)->cachedSourceProgram;
}

/*!
  Used internally to update shader parameters.
*/
void
SoShaderObject::updateParameters(SoState * state)
{
  const uint32_t cachecontext = SoGLCacheContextElement::get(state);
  PRIVATE(this)->updateAllParameters(cachecontext);
  PRIVATE(this)->updateStateMatrixParameters(cachecontext, state);
  PRIVATE(this)->updateCoinParameters(cachecontext, state);
}

/* ***************************************************************************
 * *** private implementation of SoShaderObjectP ***
 * ***************************************************************************/

SoShaderObjectP::SoShaderObjectP(SoShaderObject * ownerptr)
{
  this->owner = ownerptr;
  this->sensor = new SoNodeSensor(SoShaderObjectP::sensorCB, this);
  this->sensor->setPriority(0);
  this->sensor->attach(ownerptr);

  this->cachedSourceType = SoShaderObject::FILENAME;
  this->didSetSearchDirectories = FALSE;
  this->shouldload = TRUE;

  SoContextHandler::addContextDestructionCallback(context_destruction_cb, this);
}

SoShaderObjectP::~SoShaderObjectP()
{
  SoContextHandler::removeContextDestructionCallback(context_destruction_cb, this);

  this->deleteGLShaderObjects();

  SbStringList empty;
  this->setSearchDirectories(empty);
  delete this->sensor;
}

void
SoShaderObjectP::GLRender(SoGLRenderAction * action)
{
  this->render(action ? action->getState() : NULL);
}

void
SoShaderObjectP::render(SoState * state)
{
  SbBool isactive = this->owner->isActive.getValue();
  if (!isactive || !state) return;

  SoGLShaderProgram * shaderProgram = SoGLShaderProgramElement::get(state);
  if (!shaderProgram) {
    SoDebugError::postWarning("SoShaderObject::render",
                              "SoShaderObject seems to not be under a SoShaderProgram node");
    return;
  }

  const uint32_t cachecontext = SoGLCacheContextElement::get(state);
  const cc_glglue * glue = cc_glglue_instance(cachecontext);

  SoGLShaderObject * shaderobject = this->getGLShaderObject(cachecontext);

  if (this->owner->sourceProgram.isDefault() ||
      this->owner->sourceProgram.getValue().getLength() == 0) { return; }

  if (shaderobject == NULL) {
    if (this->shouldload) {
      this->checkType(); // set this->cachedSourceType
      this->readSource(); // set this->cachedSourceProgram
      this->shouldload = FALSE;
    }
    // if file could not be read
    if (this->cachedSourceType == SoShaderObject::FILENAME) return;

    if (!this->isSupported(this->cachedSourceType, glue)) {
      SbString s;
      switch (this->cachedSourceType) {
      case SoShaderObject::ARB_PROGRAM: s = "ARB_PROGRAM"; break;
      case SoShaderObject::CG_PROGRAM: s = "CG_PROGRAM"; break;
      case SoShaderObject::GLSL_PROGRAM: s = "GLSL_PROGRAM"; break;
      default: assert(FALSE && "unknown shader");
      }
      SoDebugError::postWarning("SoShaderObjectP::render",
                                "%s is not supported", s.getString());
      return;
    }

    switch (this->cachedSourceType) {
    case SoShaderObject::ARB_PROGRAM:
      shaderobject = new SoGLARBShaderObject(cachecontext);
      break;
    case SoShaderObject::CG_PROGRAM:
      shaderobject = new SoGLCgShaderObject(cachecontext);
      break;
    case SoShaderObject::GLSL_PROGRAM:
      shaderobject = new SoGLSLShaderObject(cachecontext);
      break;
    default:
      assert(FALSE && "This shouldn't happen!");
    }

    if (this->owner->isOfType(SoVertexShader::getClassTypeId())) {
      shaderobject->setShaderType(SoGLShaderObject::VERTEX);
    }
    else if (this->owner->isOfType(SoFragmentShader::getClassTypeId())) {
      shaderobject->setShaderType(SoGLShaderObject::FRAGMENT);
    }
    else {
      assert(this->owner->isOfType(SoGeometryShader::getClassTypeId()));
      shaderobject->setShaderType(SoGLShaderObject::GEOMETRY);

      //SoGeometryShader * geomshader = (SoGeometryShader*) this->owner;

    }

#if defined(SOURCE_HINT)
    shaderobject->sourceHint = getSourceHint();
#endif
    shaderobject->load(this->cachedSourceProgram.getString());
    this->setGLShaderObject(shaderobject, cachecontext);
  }
  if (shaderobject) {
    shaderProgram->addShaderObject(shaderobject);
    shaderobject->setIsActive(isactive);
  }
}

void
SoShaderObject::render(SoState * state)
{
  PRIVATE(this)->render(state);
}

// sets this->cachedSourceType to [ARB|CG|GLSL]_PROGRAM
// this->cachedSourceType will be set to FILENAME, if sourceType is unknown
void
SoShaderObjectP::checkType(void)
{
  this->cachedSourceType =
    (SoShaderObject::SourceType)this->owner->sourceType.getValue();

  if (this->cachedSourceType != SoShaderObject::FILENAME) return;

  // determine sourceType from file extension
  SbString fileName = this->owner->sourceProgram.getValue();
  int len = fileName.getLength();
  if (len > 5) {
    SbString subStr = fileName.getSubString(len-5);
    if (subStr == ".glsl" || subStr == ".vert" || subStr == ".frag") {
      this->cachedSourceType = SoShaderObject::GLSL_PROGRAM;
      return;
    }
  }
  if (len > 3) {
    SbString subStr = fileName.getSubString(len-3);
    if (subStr == ".cg") {
      this->cachedSourceType = SoShaderObject::CG_PROGRAM;
      return;
    }
    if (subStr == ".fp") {
      this->cachedSourceType = this->owner->isOfType(SoVertexShader::getClassTypeId())
        ? SoShaderObject::FILENAME : SoShaderObject::ARB_PROGRAM;
      return;
    }
    if (subStr==".vp") {
      this->cachedSourceType = this->owner->isOfType(SoVertexShader::getClassTypeId())
        ? SoShaderObject::ARB_PROGRAM : SoShaderObject::FILENAME;
      return;
    }
  }
  SoDebugError::postWarning("SoShaderObjectP::checkType",
                            "Could not determine shader type of file '%s'!\n"
                            "Following file extensions are supported:\n"
                            "*.fp -> ARB_PROGRAM (fragment)\n"
                            "*.vp -> ARB_PROGRAM (vertex)\n"
                            "*.cg -> CG_PROGRAM (fragment|vertex)\n"
                            "*.glsl *.vert *.frag -> GLSL_PROGRAM (fragment|vertex)\n",
                            fileName.getString());
  // error: could not determine SourceType
  this->cachedSourceType = SoShaderObject::FILENAME;
}

// read the file if necessary and assign content to this->cachedSourceProgram
void
SoShaderObjectP::readSource(void)
{
  SoShaderObject::SourceType srcType =
    (SoShaderObject::SourceType)this->owner->sourceType.getValue();

  this->cachedSourceProgram.makeEmpty();

  if (this->owner->sourceProgram.isDefault())
    return;
  else if (srcType != SoShaderObject::FILENAME)
    this->cachedSourceProgram = this->owner->sourceProgram.getValue();
  else {
    if (this->cachedSourceType != SoShaderObject::FILENAME) {

      SbStringList subdirs;
      subdirs.append(new SbString("shader"));
      subdirs.append(new SbString("shaders"));
      SbString fileName = SoInput::searchForFile(this->owner->sourceProgram.getValue(),
                                                 this->searchdirectories,
                                                 subdirs);
      // delete allocated subdirs before continuing
      delete subdirs[0];
      delete subdirs[1];

      if (fileName.getLength() <= 0) {
        SoDebugError::postWarning("SoShaderObjectP::readSource",
                                  "Shader file not found: '%s'",
                                  this->owner->sourceProgram.getValue().getString());
        this->cachedSourceType = SoShaderObject::FILENAME;
        return;
      }

      FILE * f = fopen(fileName.getString(), "rb");
      SbBool readok = FALSE;
      if (f) {
        if (fseek(f, 0L, SEEK_END) == 0) {
          const long length = ftell(f);
          if ((length > 0) && (fseek(f, 0L, SEEK_SET) == 0)) {
            char * srcstr = new char[length+1];
            size_t readlen = fread(srcstr, 1, length, f);
            if (readlen == (size_t) length) {
              srcstr[length] = '\0';
              this->cachedSourceProgram = srcstr;
              readok = TRUE;
            }
            delete[] srcstr;
          }
        }
        fclose(f);
      }
      if (!readok) {
        this->cachedSourceType = SoShaderObject::FILENAME;
        SoDebugError::postWarning("SoShaderObjectP::readSource",
                                  "Could not read shader file '%s'",
                                  fileName.getString());
      }
    }
  }
}

SbBool
SoShaderObjectP::isSupported(SoShaderObject::SourceType sourceType, const cc_glglue * glue)
{
  if (this->owner->isOfType(SoVertexShader::getClassTypeId())) {
    // don't call this function. It's not context safe. pederb, 20051103
    // return SoVertexShader::isSupported(sourceType);

    if (sourceType == SoShaderObject::ARB_PROGRAM) {
      return SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_VERTEX_PROGRAM);
    }
    else if (sourceType == SoShaderObject::GLSL_PROGRAM) {
      return SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_SHADER_OBJECT);
    }
    // FIXME: Add support for detecting missing Cg support
    // (20050427 handegar)
    else if (sourceType == SoShaderObject::CG_PROGRAM) return TRUE;
    return FALSE;
  }
  else if (this->owner->isOfType(SoFragmentShader::getClassTypeId())) {
    // don't call this function. It's not context safe. pederb, 20051103
    // return SoFragmentShader::isSupported(sourceType);

    if (sourceType == SoShaderObject::ARB_PROGRAM) {
      return SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_FRAGMENT_PROGRAM);
    }
    else if (sourceType == SoShaderObject::GLSL_PROGRAM) {
      return SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_SHADER_OBJECT);
    }
    // FIXME: Add support for detecting missing Cg support (20050427
    // handegar)
    else if (sourceType == SoShaderObject::CG_PROGRAM) return TRUE;
    return FALSE;
  }
  else {
    assert(this->owner->isOfType(SoGeometryShader::getClassTypeId()));
    if (sourceType == SoShaderObject::GLSL_PROGRAM) {
      return
        SoGLDriverDatabase::isSupported(glue, "GL_EXT_geometry_shader4") &&
        SoGLDriverDatabase::isSupported(glue, SO_GL_ARB_SHADER_OBJECT);
    }
    return FALSE;
  }
}

void
SoShaderObjectP::updateParameters(const uint32_t cachecontext, int start, int num)
{

  if (!this->owner->isActive.getValue()) return;
  if (start < 0 || num < 0) return;

  SoGLShaderObject * shaderobject = this->getGLShaderObject(cachecontext);
  if ((shaderobject == NULL) || !shaderobject->getParametersDirty()) return;

  int cnt = this->owner->parameter.getNum();
  int end = start+num;

  end = (end > cnt) ? cnt : end;
  for (int i=start; i<end; i++) {
    SoUniformShaderParameter * param =
      (SoUniformShaderParameter*)this->owner->parameter[i];
    param->updateParameter(shaderobject);
  }
}

void
SoShaderObjectP::updateCoinParameters(const uint32_t cachecontext, SoState * state)
{
  int i, cnt = this->owner->parameter.getNum();

  SoGLShaderObject * shaderobject = this->getGLShaderObject(cachecontext);

  for (i = 0; i < cnt; i++) {
    SoUniformShaderParameter * param =
      (SoUniformShaderParameter*)this->owner->parameter[i];
    SbName name = param->name.getValue();
    
    if (strncmp(name.getString(), "coin_", 5) == 0) {
      if (name == "coin_texunit0_model") {
        SoMultiTextureImageElement::Model model;
        SbColor dummy;
        SbBool tex = SoGLMultiTextureImageElement::get(state, model, dummy) != NULL;
        shaderobject->updateCoinParameter(state, name, NULL, tex ? model : 0);
      }
      else if (name == "coin_texunit1_model") {
        SoMultiTextureImageElement::Model model;
        SbColor dummy;
        SbBool tex = SoGLMultiTextureImageElement::get(state, 1, model, dummy) != NULL;
        shaderobject->updateCoinParameter(state, name, NULL, tex ? model : 0);
      }
      else if (name == "coin_texunit2_model") {
        SoMultiTextureImageElement::Model model;
        SbColor dummy;
        SbBool tex = SoGLMultiTextureImageElement::get(state, 2, model, dummy) != NULL;
        shaderobject->updateCoinParameter(state, name, NULL, tex ? model : 0);
      }
      else if (name == "coin_texunit3_model") {
        SoMultiTextureImageElement::Model model;
        SbColor dummy;
        SbBool tex = SoGLMultiTextureImageElement::get(state, 3, model, dummy) != NULL;
        shaderobject->updateCoinParameter(state, name, NULL, tex ? model : 0);
      }
      else if (name == "coin_light_model") {
        shaderobject->updateCoinParameter(state, name, NULL, SoLazyElement::getLightModel(state));
      }
      else if (name == "coin_two_sided_lighting") {
        shaderobject->updateCoinParameter(state, name, NULL, SoLazyElement::getTwoSidedLighting(state));
      }
    }
  }
}


void
SoShaderObjectP::updateAllParameters(const uint32_t cachecontext)
{
  if (!this->owner->isActive.getValue()) return;

  SoGLShaderObject * shaderobject = this->getGLShaderObject(cachecontext);
  if ((shaderobject == NULL) || !shaderobject->getParametersDirty()) return;

  int i, cnt = this->owner->parameter.getNum();

  for (i=0; i<cnt; i++) {
    SoUniformShaderParameter *param =
      (SoUniformShaderParameter*)this->owner->parameter[i];
    param->updateParameter(shaderobject);
  }
  shaderobject->setParametersDirty(FALSE);
}

// Update state matrix parameters
void
SoShaderObjectP::updateStateMatrixParameters(const uint32_t cachecontext, SoState *state)
{
#define STATE_PARAM SoShaderStateMatrixParameter
  if (!this->owner->isActive.getValue()) return;

  SoGLShaderObject * shaderobject = this->getGLShaderObject(cachecontext);
  if (shaderobject == NULL) return;

  int i, cnt = this->owner->parameter.getNum();
  for (i= 0; i <cnt; i++) {
    STATE_PARAM * param = (STATE_PARAM*)this->owner->parameter[i];
    if (param->isOfType(STATE_PARAM::getClassTypeId())) {
      param->updateValue(state);
      param->updateParameter(shaderobject);
	}
  }
#undef STATE_PARAM
}

SbBool
SoShaderObjectP::containStateMatrixParameters(void) const
{
#define STATE_PARAM SoShaderStateMatrixParameter
  int i, cnt = this->owner->parameter.getNum();
  for (i = 0; i < cnt; i++) {
    if (this->owner->parameter[i]->isOfType(STATE_PARAM::getClassTypeId()))
      return TRUE;
  }
#undef STATE_PARAM
  return FALSE;
}

#if defined(SOURCE_HINT)
SbString
SoShaderObjectP::getSourceHint(void) const
{
  SoShaderObject::SourceType srcType =
    (SoShaderObject::SourceType)this->owner->sourceType.getValue();

  if (srcType == SoShaderObject::FILENAME)
    return this->owner->sourceProgram.getValue();
  else
    return ""; // FIXME: should return first line of shader source code
}
#endif

void
SoShaderObjectP::sensorCB(void *data, SoSensor *sensor)
{
  SoShaderObjectP * thisp = (SoShaderObjectP*) data;
  SoField * field = ((SoNodeSensor *)sensor)->getTriggerField();

  if (field == &thisp->owner->sourceProgram ||
      field == &thisp->owner->sourceType) {
    thisp->deleteGLShaderObjects();
    thisp->shouldload = TRUE;
  }
  else if (field == &thisp->owner->parameter) {
    thisp->invalidateParameters();
  }
  if (!thisp->didSetSearchDirectories) {
    thisp->setSearchDirectories(SoInput::getDirectories());
  }
}

void
SoShaderObjectP::setSearchDirectories(const SbStringList & list)
{
  int i;
  for (i = 0; i< this->searchdirectories.getLength(); i++) {
    delete this->searchdirectories[i];
  }

  for (i = 0; i < list.getLength(); i++) {
    this->searchdirectories.append(new SbString(*(list[i])));
  }
  this->didSetSearchDirectories = TRUE;
}

#undef PRIVATE
