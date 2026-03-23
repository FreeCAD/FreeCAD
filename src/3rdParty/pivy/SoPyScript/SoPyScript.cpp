/*
 * Copyright (c) 2002-2007 Systems in Motion
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Python.h>
#include <assert.h>

#include <Inventor/SoInput.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/actions/SoAudioRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/lists/SbStringList.h>
#include <Inventor/sensors/SoOneShotSensor.h>

#include "swigpyrun.h"
#include "SoPyScript.h"

// Python code snippet to load in a URL through the urllib module
#define PYTHON_URLLIB_URLOPEN "\
import urllib\n\
try:\n\
  fd = urllib.urlopen(url.split()[0])\n\
  script = fd.read()\n\
  fd.close()\n\
except:\n\
  script = None\n\
del url"

class GlobalLock {
  public:
    GlobalLock(void) : state(PyGILState_Ensure()) {}
    ~GlobalLock() { PyGILState_Release(state); }
  private:
    PyGILState_STATE state;
};

class SoPyScriptP {
  public:
    SoPyScriptP(SoPyScript * master) :
      isReading(FALSE),
      oneshotSensor(new SoOneShotSensor(SoPyScript::eval_cb, master)),
      handler_registry_dict(PyDict_New()),
      local_module_dict(PyDict_New())
    {
      if (!global_module_dict) {
        Py_SetProgramName((char *)"SoPyScript");
        Py_Initialize();
        global_module_dict = PyModule_GetDict(PyImport_AddModule("__main__"));
        
        if (PyRun_SimpleString("from pivy.coin import *")) {
          SoDebugError::postWarning("SoPyScript::initClass",
                                    "\n*Yuk!* The box containing a fierce looking python snake to drive\n"
                                    "this node has arrived but was found to be empty! The Pivy module\n"
                                    "required for the Python Scripting Node could not be successfully\n"
                                    "imported! Check your setup and fix it so that the python snake can\n"
                                    "happily wiggle and byte you in the ass...");
        }
      }    
    }
    
    ~SoPyScriptP() {
      GlobalLock lock;
      delete this->oneshotSensor;
      Py_DECREF(handler_registry_dict);
      Py_DECREF(local_module_dict);
    }

    PyObject *
    createPySwigType(SbString typeVal, void * obj) {    
      typeVal += " *";
      swig_type_info * swig_type;
      if (!(swig_type = SWIG_TypeQuery(typeVal.getString()))) {
        // try again by prefixing the typename with So
        SbString soTypeVal("So");
        soTypeVal += typeVal;
        if (!(swig_type = SWIG_TypeQuery(soTypeVal.getString()))) {
          return NULL;
        }
      }

      return SWIG_NewPointerObj(obj, swig_type, 0);
    }

    SbBool isReading;
    SoOneShotSensor * oneshotSensor;
    PyObject * handler_registry_dict;
    PyObject * local_module_dict;
    static PyObject * global_module_dict;
    SoFieldList changedFields;
};

PyObject * SoPyScriptP::global_module_dict = NULL;

#define PRIVATE(_this_) (_this_)->pimpl

SoType SoPyScript::classTypeId;

void
SoPyScript::initClass(void)
{
  if (SoType::fromName("SoPyScript").isBad()) {
    SoPyScript::classTypeId =
      SoType::createType(SoNode::getClassTypeId(),
                         SbName("SoPyScript"),
                         SoPyScript::createInstance,
                         SoNode::nextActionMethodIndex++);

#if 0 // FIXME: necessary or not necessary? 20040412 tamer.
    SoNode::setCompatibilityTypes(SoPyScript::getClassTypeId(),
                                  SoNode::COIN_2_0|SoNode::COIN_2_2|SoNode::COIN_2_3);
#endif

    SoAudioRenderAction::addMethod(SoPyScript::getClassTypeId(), SoNode::audioRenderS);
  }
}

SoPyScript::SoPyScript(void)
  : fielddata(NULL)
{
  PRIVATE(this) = new SoPyScriptP(this);
  this->isBuiltIn = FALSE;

  assert(SoPyScript::classTypeId != SoType::badType());

  this->script.setValue(NULL);
  this->script.setContainer(this);

  this->mustEvaluate.setValue(FALSE);
  this->mustEvaluate.setContainer(this);

  this->initFieldData();
}

// Doc in parent
SoType
SoPyScript::getClassTypeId(void)
{
  return SoPyScript::classTypeId;
}

// Doc in parent
SoType
SoPyScript::getTypeId(void) const
{
  return SoPyScript::classTypeId;
}

SoPyScript::~SoPyScript()
{
  delete PRIVATE(this);
  for (int i = 0; i < this->fielddata->getNumFields(); i++) {
    SoField * f = this->fielddata->getField(this, i);
    if (f != &this->script || f != &this->mustEvaluate) { delete f; }
  }
  delete this->fielddata;
}

// Doc in parent
void
SoPyScript::doPyAction(SoAction * action, const char * funcname)
{
  if (funcname && !script.isIgnored()) {
    GlobalLock lock;

    PyObject * func = PyDict_GetItemString(PRIVATE(this)->local_module_dict, funcname);

    if (func) {
      if (coin_getenv("PIVY_DEBUG")) {
        SoDebugError::postInfo("SoPyScript::doAction",
                               "%s called!", action->getTypeId().getName().getString());
      }
      
      // convert the action instance to a Python object
      SbString typeVal(action->getTypeId().getName().getString());
      
      PyObject * pyAction;
      if (!(pyAction = PRIVATE(this)->createPySwigType(typeVal, action))) {
        SoDebugError::post("SoPyScript::doAction",
                           "%s could not be created!",
                           typeVal.getString());
        inherited::doAction(action);
        return;
      }
      if (!PyCallable_Check(func)) {
        SbString errMsg(funcname);
        errMsg += " is not a callable object!";
        PyErr_SetString(PyExc_TypeError, errMsg.getString());
      } else {
        PyObject * argtuple = Py_BuildValue("(O)", pyAction);
        PyObject * result;
        if (!(result = PyObject_CallObject(func, argtuple))) {
          PyErr_Print();
        }
        Py_XDECREF(result);
        Py_DECREF(argtuple);
        Py_DECREF(pyAction);
      }
    }

    if (coin_getenv("PIVY_DEBUG")) {
      SoDebugError::postInfo("SoPyScript::doAction",
                             "funcname: %s, func: %p",
                             funcname, func);
    }
  }
  inherited::doAction(action);
}

// Doc in parent
void
SoPyScript::GLRender(SoGLRenderAction * action)
{
  SoPyScript::doPyAction(action, "GLRender");
  inherited::GLRender(action);
}

// Doc in parent
void
SoPyScript::GLRenderBelowPath(SoGLRenderAction * action)
{
  SoPyScript::doPyAction(action, "GLRenderBelowPath");
  inherited::GLRenderBelowPath(action);
}

// Doc in parent
void
SoPyScript::GLRenderInPath(SoGLRenderAction * action)
{
  SoPyScript::doPyAction(action, "GLRenderInPath");
  inherited::GLRenderInPath(action);
}

// Doc in parent
void
SoPyScript::GLRenderOffPath(SoGLRenderAction * action)
{
  SoPyScript::doPyAction(action, "GLRenderOffPath");
  inherited::GLRenderOffPath(action);
}

// Doc in parent
void
SoPyScript::callback(SoCallbackAction * action)
{
  SoPyScript::doPyAction(action, "callback");
  inherited::callback(action);
}

// Doc in parent
void
SoPyScript::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoPyScript::doPyAction(action, "getBoundingBox");
  inherited::getBoundingBox(action);
}

// Doc in parent
void
SoPyScript::getMatrix(SoGetMatrixAction * action)
{
  SoPyScript::doPyAction(action, "getMatrix");
  inherited::getMatrix(action);
}

// Doc in parent
void
SoPyScript::handleEvent(SoHandleEventAction * action)
{
  SoPyScript::doPyAction(action, "handleEvent");
  inherited::handleEvent(action);
}

// Doc in parent
void
SoPyScript::pick(SoPickAction * action)
{
  SoPyScript::doPyAction(action, "pick");
  inherited::pick(action);
}

// Doc in parent
void
SoPyScript::rayPick(SoRayPickAction * action)
{
  SoPyScript::doPyAction(action, "rayPick");
  inherited::rayPick(action);
}

// Doc in parent
void
SoPyScript::search(SoSearchAction * action)
{
  SoPyScript::doPyAction(action, "search");
  inherited::search(action);
}

// Doc in parent
void
SoPyScript::write(SoWriteAction * action)
{
  SoPyScript::doPyAction(action, "write");
  inherited::write(action);
}

// Doc in parent
void
SoPyScript::audioRender(SoAudioRenderAction * action)
{
  SoPyScript::doPyAction(action, "audioRender");
  inherited::audioRender(action);
}

// Doc in parent
void
SoPyScript::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoPyScript::doPyAction(action, "getPrimitiveCount");
  inherited::getPrimitiveCount(action);
}

// Doc in parent
void
SoPyScript::copyContents(const SoFieldContainer * from,
                         SbBool copyConn)
{
  assert(from->isOfType(SoPyScript::getClassTypeId()));

  this->initFieldData();

  const SoPyScript * fromnode = (SoPyScript*)from;
  const SoFieldData * src = from->getFieldData();

  for (int i = 0; i < src->getNumFields(); i++) {
    const SoField * f = src->getField(from, i);
    if (f != &fromnode->script &&
        f != &fromnode->mustEvaluate) {
      SoField * field = (SoField*)f->getTypeId().createInstance();
      SbString fieldname = src->getFieldName(i).getString();
      
      field->setContainer(this);
      this->fielddata->addField(this, fieldname.getString(), field);      
    }
  }
  inherited::copyContents(from, copyConn);
}

// Doc in parent
void 
SoPyScript::notify(SoNotList * list)
{
  if (!PRIVATE(this)->isReading) {
    SoField * f = list->getLastField();

    if (f == &this->mustEvaluate) {
      int pri = this->mustEvaluate.getValue() ? 
        0 : SoDelayQueueSensor::getDefaultPriority();
      PRIVATE(this)->oneshotSensor->setPriority(pri);
    }
    else if (f == &this->script) { this->executePyScript(); }
    else { 
      if (PRIVATE(this)->changedFields.find(f) == -1) {
        PRIVATE(this)->changedFields.append(f);
      }
      PRIVATE(this)->oneshotSensor->schedule();
    }
  }
  inherited::notify(list);
}

// Doc in parent
void *
SoPyScript::createInstance(void)
{
  return (void*)new SoPyScript;
}

SbBool
SoPyScript::readInstance(SoInput * in, unsigned short flags)
{
  // avoid triggering the eval cb while reading the file.
  PRIVATE(this)->isReading = TRUE;

  SbString name, typeVal;

  // read in the first string
  if (in->read(typeVal)) {
    if (typeVal != "fields") {
      in->putBack(typeVal.getString());
    } else if (in->read(typeVal) && typeVal == "[") {
      while (in->read(typeVal) && typeVal != "]") {
        SoType type = SoType::fromName(typeVal);
        
        /* if it denotes a valid type and is derived from SoField then
           read in the next string representing the name of the
           field */
        if (type != SoType::badType() &&
            type.isDerivedFrom(SoField::getClassTypeId()) &&
            in->read(name)) {
          // check for a comma at the end and strip it off
          const SbString fieldname = 
            (name[name.getLength()-1] == ',') ? name.getSubString(0, name.getLength()-2) : name;
          
          // skip the static fields
          if (fieldname == "script" || fieldname == "mustEvaluate") { continue; }
          
          /* instantiate the field and conduct similar actions as the
             SO_NODE_ADD_FIELD macro */
          SoField * field = (SoField *)type.createInstance();
          field->setContainer(this);
          this->fielddata->addField(this, fieldname.getString(), field);
        }
      }
    }
  }

  // and finally let the regular readInstance() method parse the rest
  SbBool ok = inherited::readInstance(in, flags);

  if (!ok) {
    // evaluate script
    PRIVATE(this)->oneshotSensor->schedule();
  }

  this->executePyScript();

  PRIVATE(this)->isReading = FALSE;

  return ok;
}

// Initializes the field data and adds the default fields.
void
SoPyScript::initFieldData(void)
{
  if (this->fielddata) delete this->fielddata;
  this->fielddata = new SoFieldData;
  this->fielddata->addField(this, "script", &this->script);
  this->fielddata->addField(this, "mustEvaluate", &this->mustEvaluate);
}

// Doc in parent
const SoFieldData *
SoPyScript::getFieldData(void) const
{
  return this->fielddata;
}

// loads and executes Python script contained in the script field
void
SoPyScript::executePyScript(void)
{
  // strip out possible \r's that could come from win32 line endings
  SbString src = script.getValue();
  SbString pyString;
  for (int i=0; i < src.getLength(); i++) {
    if (src[i] != '\r') { pyString += src[i]; }
  }

  GlobalLock lock;

  /* setup the local dictionary by creating a local copy of the global
     dictionary to use for executing the script */
  PyDict_Clear(PRIVATE(this)->local_module_dict);
  PyDict_Update(PRIVATE(this)->local_module_dict, PRIVATE(this)->global_module_dict);
  PyDict_Clear(PRIVATE(this)->handler_registry_dict);
      
  /* shovel the the node itself on to the Python interpreter as self
     instance */
  swig_type_info * swig_type = 0;

  if ((swig_type = SWIG_TypeQuery("SoNode *")) == 0) {
    SoDebugError::post("SoPyScript::executePyScript",
                       "SoNode type could not be found!");
  }

  // refcount ourselves for autoref'ing to work
  this->ref();

  // add the field to the global dict
  PyDict_SetItemString(PRIVATE(this)->local_module_dict, 
                       "self",
                       SWIG_NewPointerObj(this, swig_type, 0));
  
  // add the handler registry dict to the global dict
  PyDict_SetItemString(PRIVATE(this)->local_module_dict, 
                       "handler_registry",
                       PRIVATE(this)->handler_registry_dict);

  const SoFieldData * fields = this->getFieldData();
  const int n = fields->getNumFields();
  for (int i = 0; i < n; i++) {
    SoField * f = fields->getField(this, i);
    SbString typeName(f->getTypeId().getName());
    SbString fieldName(fields->getFieldName(i));
    
    // shovel the field instance on to the Python interpreter
    PyObject * pyField = NULL;
    if ((pyField = PRIVATE(this)->createPySwigType(typeName, f)) == NULL) {
      SoDebugError::post("SoPyScript::readInstance",
                         "field type %s could not be created!",
                          typeName.getString());            
      continue;
    }

    // add the field to the global dict
    PyDict_SetItemString(PRIVATE(this)->local_module_dict, 
                         fieldName.getString(),
                         pyField);

    SbString funcname("handle_");
    funcname += fieldName;

    // add the field handler to the handler registry
    PyDict_SetItemString(PRIVATE(this)->handler_registry_dict, 
                         fieldName.getString(),
#ifdef PY_2
                         PyString_FromString(funcname.getString());
#else
                         PyString_FromString(funcname.getString());
#endif
                         )
  }

  // check if the script denotes an URL or path
  /* FIXME: maybe, just maybe, we could do a little better error
     handling here??? throwing a Syntax Error at the user's face in
     case of a not resolving URL or non existent file for the reason
     of being to lame to catch a couple of exceptions is absolutely
     pathetic!  reconsider the whole approach as it smells like a
     lousy hack! 20041021 tamer. */
  if (src.getLength()) {
    // try to find a file relative to the current input directory stack
    SbStringList subdirs;
    SbString fullName = SoInput::searchForFile(pyString, SoInput::getDirectories(), subdirs);
    if (fullName != "") { pyString = fullName; }
    PyObject * url = 
#ifdef PY_2
      PyString_FromString(pyString.getString());
#else
      PyBytes_FromString(pyString.getString());
#endif

    // add the url to the global dict
    PyDict_SetItemString(PRIVATE(this)->local_module_dict, "url", url);

    PyObject * result = PyRun_String(PYTHON_URLLIB_URLOPEN,
                                     Py_file_input, 
                                     PRIVATE(this)->local_module_dict,
                                     PRIVATE(this)->local_module_dict);
    if (result) { Py_DECREF(result); }
    else { PyErr_Print(); }

    PyObject * script_new = PyDict_GetItemString(PRIVATE(this)->local_module_dict, "script");
    if (script_new != Py_None) {
      pyString.makeEmpty();
      pyString = 
#ifdef PY_2
        PyString_AsString(script_new);
#else
        PyBytes_AsString(script_new);
    }
    Py_DECREF(url);
  }

  PyObject * result = PyRun_String((char *)pyString.getString(),
                                   Py_file_input,
                                   PRIVATE(this)->local_module_dict,
                                   PRIVATE(this)->local_module_dict);
  if (result) { Py_DECREF(result); }
  else { PyErr_Print(); }

  PRIVATE(this)->changedFields.truncate(0);

  if (coin_getenv("PIVY_DEBUG")) {
    SoDebugError::postInfo("SoPyScript::executePyScript",
                           "script executed at full length!");
  }
}

// callback for oneshotSensor
void 
SoPyScript::eval_cb(void * data, SoSensor *)
{
  SoPyScript * self = (SoPyScript*)data;

  if (coin_getenv("PIVY_DEBUG")) {
    SoDebugError::postInfo("SoPyScript::eval_cb",
                           "eval_cb called!");
  }

  for (int i = 0; i < PRIVATE(self)->changedFields.getLength(); i++) {
    SoField * f = PRIVATE(self)->changedFields[i];
    SbName fieldname;
    if (self->getFieldName(f, fieldname)) {
      GlobalLock lock;

      // look up the function name in the handler registry
      PyObject * funcname = PyDict_GetItemString(PRIVATE(self)->handler_registry_dict,
                                                 fieldname.getString());
      if (!funcname) { continue; }

      // check if it is a string
#ifdef PY_2
      if (PyString_Check(funcname)) {
#else
      if (PyBytes_Check(funcname)) {
#endif
        // get the function handle in the global module dictionary if available
        PyObject * func = PyDict_GetItemString(PRIVATE(self)->local_module_dict,
#ifdef PY_2
                                               PyString_AsString(funcname));
#else
                                               PyBytes_AsString(funcname));
#endif
        if (!func) { continue; }

        if (coin_getenv("PIVY_DEBUG")) {
          SoDebugError::postInfo("SoPyScript::eval_cb",
                                 "fieldname: %s, funcname: %s, func: %p",
                                 fieldname.getString(),
#ifdef PY_2
                                 PyString_AsString(funcname),
#else
                                 PyBytes_AsString(funcname),
#endif
                                 func);
        }

        if (!PyCallable_Check(func)) {
#ifdef PY_2
          SbString errMsg(PyString_AsString(funcname));
#else
          SbString errMsg(PyBytes_AsString(funcname));
#endif
          errMsg += " is not a callable object!";
          PyErr_SetString(PyExc_TypeError, errMsg.getString());
        } else {
          PyObject * result;
          if (!(result = PyObject_CallObject(func, NULL))) {
            PyErr_Print();
          }
          Py_XDECREF(result);
        }
      } // if (self->getFieldName(...)) - lock released here
    }
  }

  PRIVATE(self)->changedFields.truncate(0);
}

#undef PRIVATE
