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

// FIXME: idea from thammer; make all the various operations connect
// through callback-plugins, for maximum flexibility. 20050606 mortene.

// FIXME: another idea; factor out all the internal Javascript
// handling, connecting up to fields, etc. to a more generic
// internal interface -- as thammer's project would like to connect
// Javascript programs/routines to any scene graph field. 20050606 mortene.

// *************************************************************************

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

// FIXME: is this really necessary? Coin/src/Makefile.am stops
// traversal into src/vrml97/, so it seems
// superfluous. Investigate. 20050526 mortene.
#ifdef HAVE_VRML97

// *************************************************************************

/*!
  \class SoVRMLScript SoVRMLScript.h Inventor/VRMLnodes/SoVRMLScript.h
  \brief The SoVRMLScript class is used to control the scene using scripts.

  \ingroup coin_VRMLnodes

  \WEB3DCOPYRIGHT

  \verbatim
  Script {
    exposedField MFString url           []
    field        SFBool   directOutput  FALSE
    field        SFBool   mustEvaluate  FALSE
    # And any number of:
    eventIn      eventType eventName
    field        fieldType fieldName initialValue
    eventOut     eventType eventName
  }
  \endverbatim

  The Script node is used to program behaviour in a scene. Script nodes
  typically

  - signify a change or user action;
  - receive events from other nodes;
  - contain a program module that performs some computation;
  - effect change somewhere else in the scene by sending events.

  Each Script node has associated programming language code,
  referenced by the url field, that is executed to carry out the
  Script node's function. That code is referred to as the "script" in
  the rest of this description. Details on the url field can be found
  in 4.5, VRML and the World Wide Web
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.5>).

  Browsers are not required to support any specific language. Detailed
  information on scripting languages is described in 4.12, Scripting
  (<http://www.web3d.org/documents/specifications/14772/V2.0/part1/concepts.html#4.12>).

  Browsers supporting a scripting language for which a language
  binding is specified shall adhere to that language binding.
  Sometime before a script receives the first event it shall be
  initialized (any language-dependent or user-defined initialize() is
  performed).

  The script is able to receive and process events that are sent to
  it. Each event that can be received shall be declared in the Script
  node using the same syntax as is used in a prototype definition:

  \verbatim
  eventIn type name
  \endverbatim

  The type can be any of the standard VRML fields
  (as defined in 5, Field and event reference). Name shall be an
  identifier that is unique for this Script node.

  The Script node is able to generate events in response to the
  incoming events. Each event that may be generated shall be declared
  in the Script node using the following syntax:

  \verbatim
  eventOut type name
  \endverbatim

  With the exception of the url field, exposedFields are not allowed
  in Script nodes.

  If the Script node's \e mustEvaluate field is \c FALSE, the browser
  may delay sending input events to the script until its outputs are
  needed by the browser. If the \e mustEvaluate field is TRUE, the
  browser shall send input events to the script as soon as possible,
  regardless of whether the outputs are needed. The \e mustEvaluate
  field shall be set to TRUE only if the Script node has effects that
  are not known to the browser (such as sending information across the
  network). Otherwise, poor performance may result.

  Once the script has access to a VRML node (via an SoSFNode or
  SoMFNode value either in one of the Script node's fields or passed
  in as an eventIn), the script is able to read the contents of that
  node's exposed fields.

  If the Script node's \e directOutput field is \c TRUE, the script
  may also send events directly to any node to which it has access,
  and may dynamically establish or break routes.

  If directOutput is \c FALSE (the default), the script may only
  affect the rest of the world via events sent through its
  eventOuts. The results are undefined if directOutput is \c FALSE and
  the script sends events directly to a node to which it has access.

  A script is able to communicate directly with the VRML browser to
  get information such as the current time and the current world
  URL. This is strictly defined by the API for the specific scripting
  language being used.  The location of the Script node in the scene
  graph has no affect on its operation. For example, if a parent of a
  Script node is a Switch node with whichChoice set to "-1" (i.e.,
  ignore its children), the Script node continues to operate as
  specified (i.e., it receives and sends events).
*/

// *************************************************************************

#include <Inventor/VRMLnodes/SoVRMLScript.h>
#include "coindefs.h"

#include <cassert>

#include <Inventor/VRMLnodes/SoVRMLMacros.h>
#include <Inventor/SbName.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/misc/SoProto.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/sensors/SoOneShotSensor.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/misc/SoJavaScriptEngine.h>

#include "nodes/SoSubNodeP.h"
#include "tidbitsp.h"

// *************************************************************************

static SoVRMLScriptEvaluateCB * sovrmlscript_eval_cb = NULL;
static void * sovrmlscript_eval_closure = NULL;

class SoVRMLScriptP {
public:
  SoVRMLScriptP(SoVRMLScript * m)
  {
    this->master = m;
    this->oneshotsensor = new SoOneShotSensor(SoVRMLScript::eval_cb, master);
    this->isreading = FALSE;
    this->isevaluating = FALSE;
#ifdef COIN_HAVE_JAVASCRIPT
    this->engine = NULL;
#endif // !COIN_HAVE_JAVASCRIPT
  }

  ~SoVRMLScriptP()
  {
    delete this->oneshotsensor;

#ifdef COIN_HAVE_JAVASCRIPT
    // FIXME: this needs to be done in a nicer way. 20050720 erikgors.
    if (this->engine != NULL)
      this->shutdown();
#endif // !COIN_HAVE_JAVASCRIPT
  }

  static SbBool debug(void);
  static void cleanup(void);

#ifdef COIN_HAVE_JAVASCRIPT
  void initialize(void);
  void shutdown(void);

  static SbBool allowSpiderMonkey(void);
  static SbBool useSpiderMonkey(void);
#endif // !COIN_HAVE_JAVASCRIPT
  void evaluate(void);

  SbBool isreading, isevaluating;
  SoOneShotSensor * oneshotsensor;

  SbList<SbName> fieldnotifications, eventoutfields, eventinfields;
  void executeFunctions(void);

#ifdef COIN_HAVE_JAVASCRIPT
  SoJavaScriptEngine * engine;
#endif // !COIN_HAVE_JAVASCRIPT

  static SbBool spidermonkey_init_failed;

private:
  SoVRMLScript * master;
};

#define PUBLIC(p) ((p)->master)
#define PRIVATE(p) ((p)->pimpl)

SbBool SoVRMLScriptP::spidermonkey_init_failed = FALSE;

// *************************************************************************

void
SoVRMLScriptP::cleanup(void)
{
#ifdef COIN_HAVE_JAVASCRIPT
  // FIXME: need to make sure this is added to atexit only after
  // the engine has started. 20050720 erikgors.
  if (SoJavaScriptEngine::getRuntime() == NULL)
    return;

  if (SoVRMLScriptP::useSpiderMonkey()) {
    // Destroy javascript runtime
    
    SoJavaScriptEngine::shutdown();
  }
#endif // !COIN_HAVE_JAVASCRIPT

  // reset static var
  SoVRMLScriptP::spidermonkey_init_failed = FALSE;
  sovrmlscript_eval_cb = NULL;
  sovrmlscript_eval_closure = NULL;
}

// *************************************************************************

SbBool
SoVRMLScriptP::debug(void)
{
  static int d = -1;
  if (d == -1) {
    const char * env = coin_getenv("COIN_DEBUG_VRMLSCRIPT");
    d = (env && (atoi(env) > 0)) ? 1 : 0;

  }
  return d ? TRUE : FALSE;
}

#ifdef COIN_HAVE_JAVASCRIPT

// The Javascript support is far from being compliant with the VRML
// specification, and has so far been developed just for internal SIM
// use, so one needs to explicitly activate it for now.
SbBool
SoVRMLScriptP::allowSpiderMonkey(void)
{
  static int d = -1;
  if (d == -1) {
    const char * env = coin_getenv("COIN_ALLOW_SPIDERMONKEY");
    d = (env && (atoi(env) > 0)) ? 1 : 0;
  }
  return d ? TRUE : FALSE;
}

SbBool
SoVRMLScriptP::useSpiderMonkey(void)
{
  if (!SoVRMLScriptP::allowSpiderMonkey()) { return FALSE; }
  if (!spidermonkey()->available) { return FALSE; }
  if (SoJavaScriptEngine::getRuntime() == NULL) { return FALSE; }
  return TRUE;
}

#endif // !COIN_HAVE_JAVASCRIPT

// *************************************************************************

SoType SoVRMLScript::classTypeId STATIC_SOTYPE_INIT;

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLScript::initClass(void) // static
{
  SoVRMLScript::classTypeId =
    SoType::createType(SoNode::getClassTypeId(),
                       SbName("VRMLScript"),
                       SoVRMLScript::createInstance,
                       SoNode::nextActionMethodIndex++);
  SoNode::setCompatibilityTypes(SoVRMLScript::getClassTypeId(), SO_VRML97_NODE_TYPE);
}

// *************************************************************************

/*!
  Constructor.
*/
SoVRMLScript::SoVRMLScript(void)
  : fielddata(NULL)
{
  coin_atexit((coin_atexit_f *)SoVRMLScriptP::cleanup, CC_ATEXIT_NORMAL);

#ifdef COIN_HAVE_JAVASCRIPT
  if (SoVRMLScriptP::allowSpiderMonkey() &&
      !SoVRMLScriptP::spidermonkey_init_failed &&
      // FIXME: next line is a hack-ish way of checking whether init()
      // has already been done on the SoJavaScriptEngine class.
      // 20060207 mortene.
      (SoJavaScriptEngine::getRuntime() == NULL)) {
    SbBool ok = SoJavaScriptEngine::init();
    if (!ok) { SoVRMLScriptP::spidermonkey_init_failed = TRUE; }
  }
#endif // !COIN_HAVE_JAVASCRIPT

  PRIVATE(this) = new SoVRMLScriptP(this);
  this->setNodeType(SoNode::VRML2);

  this->isBuiltIn = TRUE;
  assert(SoVRMLScript::classTypeId != SoType::badType());

  this->url.setNum(0);
  this->url.setContainer(this);

  // FIXME: directOutput should default be FALSE, according to the
  // VRML97 spec doc. 20050526 mortene.
  // Looks more like a hint than a rule to me. Something we can
  // safely ignore. 20050712 erikgors.
  this->directOutput.setValue(FALSE);
  this->directOutput.setContainer(this);

  // FIXME: shouldn't mustEvaluate be default FALSE? Seems like it
  // from the VRML97 spec doc. 20050526 mortene.
  this->mustEvaluate.setValue(FALSE);
  this->mustEvaluate.setContainer(this);

  this->initFieldData();
}

/*!
  Destructor.
*/
SoVRMLScript::~SoVRMLScript()
{
  delete PRIVATE(this);

  const int n = this->fielddata->getNumFields();
  for (int i = 0; i < n; i++) {
    SoField * f = this->fielddata->getField(this, i);
    if (f != &this->directOutput &&
        f != &this->url &&
        f != &this->mustEvaluate) delete f;
  }
  delete this->fielddata;
}

// *************************************************************************

/*!
  \copydetails SoNode::getClassTypeId(void)
*/
SoType
SoVRMLScript::getClassTypeId(void)
{
  return SoVRMLScript::classTypeId;
}

/*!
  \copydetails SoNode::getTypeId(void) const
*/
SoType
SoVRMLScript::getTypeId(void) const
{
  return SoVRMLScript::classTypeId;
}

// *************************************************************************

/*!  
  Sets the callback that will be called when the script needs to be
  evaluated.  
*/
void 
SoVRMLScript::setScriptEvaluateCB(SoVRMLScriptEvaluateCB * cb,
                                  void * closure)
{
  sovrmlscript_eval_cb = cb;
  sovrmlscript_eval_closure = closure;
}

// *************************************************************************

// Doc in superclass
void
SoVRMLScript::doAction(SoAction * COIN_UNUSED_ARG(action))
{
}

// Doc in superclass
void
SoVRMLScript::callback(SoCallbackAction * action)
{
  SoVRMLScript::doAction((SoAction*) action);
}

// Doc in superclass
void
SoVRMLScript::GLRender(SoGLRenderAction * action)
{
  SoVRMLScript::doAction((SoAction*) action);
}

// Doc in superclass
void
SoVRMLScript::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoVRMLScript::doAction((SoAction*) action);
}

// Doc in superclass
void
SoVRMLScript::pick(SoPickAction * action)
{
  SoVRMLScript::doAction((SoAction*) action);
}

// Doc in superclass
void
SoVRMLScript::handleEvent(SoHandleEventAction * action)
{
  SoVRMLScript::doAction((SoAction*) action);
}

// *************************************************************************

// Doc in superclass
void
SoVRMLScript::write(SoWriteAction * action)
{
  int i;
  const SbName URL("url");
  const SbName DIRECTOUTPUT("directOutput");
  const SbName MUSTEVALUATE("mustEvaluate");
  const SoFieldData * fd = this->getFieldData();

  SoOutput * out = action->getOutput();
  if (out->getStage() == SoOutput::COUNT_REFS) {
    // We will always write NORMAL and EXPOSED fields, so do a
    // setDefault(FALSE) on them. We will not write other field types,
    // so do a setDefault(TRUE) on them.
    for (i = 0; i < fd->getNumFields(); i++) {
      const SoField * f = fd->getField(this, i);
      SbName fieldname = fd->getFieldName(i);
      if (fieldname != URL && fieldname != DIRECTOUTPUT &&
          fieldname != MUSTEVALUATE) {
        if ((f->getFieldType() == SoField::NORMAL_FIELD) ||
            (f->getFieldType() == SoField::EXPOSED_FIELD)) {
          ((SoField*)f)->setDefault(FALSE);
        }
        else ((SoField*)f)->setDefault(TRUE);
      }
    }
    inherited::write(action);
  }
  else if (out->getStage() == SoOutput::WRITE) {
    if (this->writeHeader(out, FALSE, FALSE))
      return;
    for (i = 0; i < fd->getNumFields(); i++) {
      const SoField * f = fd->getField(this, i);
      SbName fieldname = fd->getFieldName(i);
      SbBool writevalue = FALSE;
      if (fieldname != URL && fieldname != DIRECTOUTPUT &&
          fieldname != MUSTEVALUATE) {
        out->indent();
        switch (f->getFieldType()) {
        case SoField::NORMAL_FIELD:        
          out->write("field ");
          writevalue = TRUE;
          break;
        case SoField::EVENTIN_FIELD:
          out->write("eventIn ");
          break;
        case SoField::EVENTOUT_FIELD:
          out->write("eventOut ");
          break;
        case SoField::EXPOSED_FIELD:
          out->write("exposedField ");
          writevalue = TRUE;
          break;
        default:
          break;
        }
        out->write(f->getTypeId().getName().getString());
        out->write(' ');

        if (writevalue) {
          f->write(out, fieldname);
        }
        else {
          out->write(fieldname.getString());
          // the write() call below is needed to resolve
          // ROUTES. SoField::write() will not write anything since if
          // we get here, the field is either an EVENTIN or an
          // EVENTOUT field
          f->write(out, fieldname); 
        }
        out->write("\n");
      }
      else if (f->shouldWrite()){
        f->write(out, fieldname);
      }
    }
    this->writeFooter(out);
  }
  else assert(0 && "unknown stage");
}

// *************************************************************************

// Doc in superclass
void
SoVRMLScript::copyContents(const SoFieldContainer * from,
                           SbBool copyConn)
{
  assert(from->isOfType(SoVRMLScript::getClassTypeId()));

  const SoVRMLScript * fromnode = (SoVRMLScript*) from;

  const SoFieldData * src = from->getFieldData();
  const int n = src->getNumFields();
  for (int i = 0; i < n; i++) {
    const SoField * f = src->getField(from, i);
    if (f != &fromnode->directOutput &&
        f != &fromnode->url &&
        f != &fromnode->mustEvaluate) {
      SoField * cp = (SoField*) f->getTypeId().createInstance();
      cp->setFieldType(f->getFieldType());
      cp->setContainer(this);
      this->fielddata->addField(this, src->getFieldName(i), cp);
    }
  }
  inherited::copyContents(from, copyConn);
}

// *************************************************************************

// Doc in superclass
void 
SoVRMLScript::notify(SoNotList * l)
{
  const SoField * f = l->getLastField();

  if (!PRIVATE(this)->isreading && !PRIVATE(this)->isevaluating) {
    if (f == &this->mustEvaluate) {
      int pri = this->mustEvaluate.getValue() ? 0 : 
        SoDelayQueueSensor::getDefaultPriority();
      PRIVATE(this)->oneshotsensor->setPriority(pri);
    }
    else {
      SbName name;
      SbBool ok = this->getFieldName(f, name);
      assert(ok);

      // We silently ignore events for non-eventIn fields
      // FIXME: This will happen when we get a fieldnotification from a
      // reference. Should we post a warning? 20050712 erikgors.
      if (PRIVATE(this)->eventinfields.find(name) != -1) {
        if (PRIVATE(this)->fieldnotifications.find(name) == -1) {
          PRIVATE(this)->fieldnotifications.append(name);
        }

        if (!PRIVATE(this)->oneshotsensor->isScheduled()) {
          PRIVATE(this)->oneshotsensor->schedule();
        }
      }
    }
  }

#ifdef COIN_HAVE_JAVASCRIPT
  if (f == &this->url) {
    PRIVATE(this)->initialize();
  }
#endif // !COIN_HAVE_JAVASCRIPT

  inherited::notify(l);
}

// *************************************************************************

// Doc in superclass
void *
SoVRMLScript::createInstance(void)
{
  return new SoVRMLScript;
}

// Doc in superclass
const SoFieldData *
SoVRMLScript::getFieldData(void) const
{
  return this->fielddata;
}

// *************************************************************************

// Doc in superclass
SbBool
SoVRMLScript::readInstance(SoInput * in, unsigned short COIN_UNUSED_ARG(flags))
{
  // avoid triggering the eval cb while reading the file.
  PRIVATE(this)->isreading = TRUE;

  SbName name(SbName::empty());
  SbBool ok;

  ok = in->read(name, TRUE);

  const SbName URL("url");
  const SbName DIRECTOUTPUT("directOutput");
  const SbName MUSTEVALUATE("mustEvaluate");
  const SbName EVENTIN("eventIn");
  const SbName EVENTOUT("eventOut");
  const SbName FIELD("field");
  const SbName EXPOSEDFIELD("exposedField");

  SbBool err = FALSE;

  SoField * builtinfield;

  while (!err && ok) {
    if (name == EVENTIN ||
        name == EVENTOUT ||
        name == FIELD ||
        name == EXPOSEDFIELD) {
      SbName ftype, fname;
      err = ! (in->read(ftype, TRUE) && in->read(fname, TRUE));
      if (!err) {
        SoType type = SoType::fromName(ftype);
        if (type.isDerivedFrom(SoField::getClassTypeId()) && type.canCreateInstance()) {
          SoField * f = (SoField*) type.createInstance();

          if (name == EVENTIN) {
            f->setFieldType(SoField::EVENTIN_FIELD);
            PRIVATE(this)->eventinfields.append(fname);
          }
          else if (name == EVENTOUT) {
            f->setFieldType(SoField::EVENTOUT_FIELD);
            PRIVATE(this)->eventoutfields.append(fname);
          }
          else if (name == EXPOSEDFIELD) {
            f->setFieldType(SoField::EXPOSED_FIELD);
          }
          f->setContainer(this);
          this->fielddata->addField(this, fname, f);
          if (name == FIELD || name == EXPOSEDFIELD) { // only read field values for fields

            err = ! f->read(in, fname);
            if (err) {
              SoReadError::post(in, "Unable to read default value for '%s'.", 
                                fname.getString());
            }
          }
          else {
            (void) in->checkISReference(this, fname, err);
            err = !err;
            if (err) {
              SoReadError::post(in, "Error while parsing IS reference for '%s'.", 
                                fname.getString());
            }
          }
        }
        else {
          err = TRUE;
          SoReadError::post(in, "Unknown field type.");
        }
      }
      else {
        SoReadError::post(in, "Unable to read field name.");
      }
      if (!err) {
        name = "";
        ok = in->read(name, TRUE);
      }
    }
    else if ((builtinfield = this->getField(name)) != NULL) {
      err = !builtinfield->read(in, name);
      if (!err) {
        name = "";
        ok = in->read(name, TRUE);
      }
      else {
        SoReadError::post(in, "Error while reading field '%s'.", 
                          name.getString());
      }
    }
    else ok = FALSE;
  }
  PRIVATE(this)->isreading = FALSE;
  
  if (!err) {
    if (name != "") in->putBack(name.getString());
    // evaluate script
    PRIVATE(this)->oneshotsensor->schedule();
  }
  return !err;
}

// *************************************************************************

//
// Private method that initializes the field data and adds the default
// fields.
//
void
SoVRMLScript::initFieldData(void)
{
  delete this->fielddata;
  this->fielddata = new SoFieldData;
  this->fielddata->addField(this, "url", &this->url);
  this->fielddata->addField(this, "directOutput", &this->directOutput);
  this->fielddata->addField(this, "mustEvaluate", &this->mustEvaluate);
}

// *************************************************************************

#ifdef COIN_HAVE_JAVASCRIPT

void
SoVRMLScriptP::initialize(void)
{
  if (this->engine != NULL) {
    if (SoVRMLScriptP::debug()) {
      SoDebugError::postInfo("SoVRMLScriptP::initialize",
                             "restarting script engine");
    }
    this->shutdown();
  }

  SbString script;

  for (int index = 0; index < PUBLIC(this)->url.getNum(); ++index) {
    SbString s(PUBLIC(this)->url[index].getString());

    // javascript support
    const char jsPrefix[] = "javascript:";
    const char jsPrefix2[] = "vrmlscript:";
    const size_t jsPrefixlen = sizeof(jsPrefix) - 1;
    const size_t jsPrefixlen2 = sizeof(jsPrefix2) - 1;
    if (s.getLength() > (int)jsPrefixlen && 
        ((s.getSubString(0, jsPrefixlen -1) == jsPrefix) ||
         (s.getSubString(0, jsPrefixlen2 -1) == jsPrefix2))) {
      // starting javascript engine
      if (!SoVRMLScriptP::useSpiderMonkey()) {
        if (SoVRMLScriptP::debug()) {
          SoDebugError::postInfo("SoVRMLScriptP::initialize",
                                 "Only the SpiderMonkey Javascript engine "
                                 "is currently supported.");
        }
        continue;
      }
      assert(this->engine == NULL);
      this->engine = new SoJavaScriptEngine;
      script = s.getSubString(jsPrefixlen);
      break;
    }
  }

  if (this->engine == NULL) {
    static int first = 1;
    if (first) {
      SoDebugError::postWarning("SoVRMLScript::initialize",
                                "No script language evaluation engine available.");
      first = 0;
    }
    return;
  }

  // FIXME: should scriptFields be set before or after the script has been
  // executed? After script feels most correct from a language independent
  // viewpoint.  200507011 erikgors.

  // FIXME: is getName() a sensiable identificator for script? 20050719 erikgors.
  SbName name = PUBLIC(this)->getName();
  this->engine->executeScript(name, script.getString());

  int numFields = PUBLIC(this)->fielddata->getNumFields();
  for (int i=0; i<numFields; ++i) {
    const SbName & name = PUBLIC(this)->fielddata->getFieldName(i);
    const SoField * f = PUBLIC(this)->fielddata->getField(PUBLIC(this), i);
    if (!(f->getFieldType() == SoField::NORMAL_FIELD ||
        f->getFieldType() == SoField::EVENTOUT_FIELD)) {
      if (SoVRMLScriptP::debug()) {
        SoDebugError::postInfo("SoVRMLScriptP::initialize",
                               "skipping scriptField %s", name.getString());
      }
      continue;
    }
    if (SoVRMLScriptP::debug()) {
      SoDebugError::postInfo("SoVRMLScriptP::initialize",
                             "setting scriptField %s", name.getString());
    }
    this->engine->setScriptField(name, f);
  }

  // Adding TRUE and FALSE to be bug-compatible. 20050719 erikgors.
  SoSFBool * boolean = (SoSFBool *)SoSFBool::createInstance();
  boolean->setValue(TRUE);
  this->engine->setScriptField(SbName("TRUE"), boolean);
  boolean->setValue(FALSE);
  this->engine->setScriptField(SbName("FALSE"), boolean);
  delete boolean;

  // Run initialize function if it exists
  SbName initialize("initialize");
  if (this->engine->hasScriptField(initialize)) {
    if (SoVRMLScriptP::debug()) {
      SoDebugError::postInfo("SoVRMLScriptP::initialize",
                             "executing script function \"%s\"", initialize.getString());
    }
    this->engine->executeFunction(initialize, 0, NULL);
  }
}

void
SoVRMLScriptP::shutdown(void)
{
  assert(this->engine != NULL);

  SbName shutdown("shutdown");
  if (this->engine->hasScriptField(shutdown)) {
    if (SoVRMLScriptP::debug()) {
      SoDebugError::postInfo("SoVRMLScriptP::initialize",
                             "executing script function \"%s\"", shutdown.getString());
    }
    this->engine->executeFunction(shutdown, 0, NULL);
  }

  delete this->engine;
  this->engine = NULL;
}

#endif // !COIN_HAVE_JAVASCRIPT

// *************************************************************************

void 
SoVRMLScriptP::evaluate(void)
{
  if (SoVRMLScriptP::debug()) {
    SoDebugError::postInfo("SoVRMLScript::eval_cb", "invoked");
    for (int i = 0; i < this->fieldnotifications.getLength(); i++) {
      SoDebugError::postInfo("SoVRMLScriptP::evaluate",
                             "notification on field '%s'",
                             this->fieldnotifications[i].getString());
    }
  }
  
  if (sovrmlscript_eval_cb) {
    sovrmlscript_eval_cb(sovrmlscript_eval_closure, PUBLIC(this));
  }
#ifdef COIN_HAVE_JAVASCRIPT
  else if (this->engine != NULL) {
    this->executeFunctions();
  }
#endif // !COIN_HAVE_JAVASCRIPT
  else {
    // FIXME: improve on this to be of more informational value to
    // both the app programmer and end-user. 20050526 mortene.
    static SbBool first = TRUE;
    if (first) {
      first = FALSE;
      SoDebugError::postWarning("SoVRMLScript::eval_cb",
                                "No script language evaluation engine available.");
    }
  }
}

// callback for oneshotsensor
void 
SoVRMLScript::eval_cb(void * data, SoSensor *)
{
  SoVRMLScript * thisp = (SoVRMLScript *)data;

  // FIXME: this wrapping is too simple, we can loose important new
  // input events. What we need to do is ignoring any writes to the
  // output field(s) that are updated. 20050602 mortene.
  PRIVATE(thisp)->isevaluating = TRUE;
  PRIVATE(thisp)->evaluate();
  PRIVATE(thisp)->isevaluating = FALSE;
}

// *************************************************************************

void
SoVRMLScriptP::executeFunctions(void)
{
  int i;

  if (this->fieldnotifications.getLength() == 0) {
    return;
  }
  
  // Execute all functions for input fields that have been changed.

  for (i = 0; i < this->fieldnotifications.getLength(); i++) {
    const SbName & n = this->fieldnotifications[i];
    const SoField * f = PUBLIC(this)->getField(n);
    assert(f);
#ifdef COIN_HAVE_JAVASCRIPT
    if (!this->engine->executeFunction(n, 1, f)) {
      SoDebugError::postWarning("SoVRMLScriptP::executeFunctions", 
                                "could not execute function %s",
                                n.getString());
    }
#endif // !COIN_HAVE_JAVASCRIPT
  }
  this->fieldnotifications.truncate(0);

  // Call eventsProcessed
#ifdef COIN_HAVE_JAVASCRIPT
  static SbName eventsProcessed("eventsProcessed");
  if (this->engine->hasScriptField(eventsProcessed)) {
    this->engine->executeFunction(eventsProcessed, 0, NULL);
  }
#endif // !COIN_HAVE_JAVASCRIPT

  // Then, pick up all new eventOut values.

  for (i = 0; i < this->eventoutfields.getLength(); i++) {
    const SbName & name = this->eventoutfields[i];
#ifdef COIN_HAVE_JAVASCRIPT
    if (!this->engine->hasScriptField(name)) {
      if (debug()) {
        SoDebugError::postInfo("SoVRMLScriptP::executeFunctions",
                               "No new value of eventOut '%s'...",
                               name.getString());
      }
      continue;
    }
#endif // !COIN_HAVE_JAVASCRIPT
    SoField * f = PUBLIC(this)->getEventOut(name);
    assert(f);

    if (debug()) {
      SoDebugError::postInfo("SoVRMLScriptP::executeFunctions",
                             "Picking up new value of eventOut '%s'...",
                             name.getString());
    }

    // FIXME: should probably rather compare new value with old before
    // actually pushing it. Perhaps there's something about this in
    // the spec. 20050606 mortene.
#ifdef COIN_HAVE_JAVASCRIPT
    if (!this->engine->getScriptField(name, f)) {
      SoDebugError::postWarning("SoVRMLScriptP::executeFunctions", 
                                "could not convert eventOut field %s",
                                name.getString());
    }
#endif // !COIN_HAVE_JAVASCRIPT

    // FIXME: We could simply unset the field, but then a lot of
    // "buggy" javascripts will start to complain. 20050721 erikgors.
    // this->engine->unsetScriptField(name);
  }
}

#undef PRIVATE
#undef PUBLIC

#endif // HAVE_VRML97
