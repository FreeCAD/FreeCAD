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

#ifndef PIVY_SOPYSCRIPT_H
#define PIVY_SOPYSCRIPT_H

#if defined(_WIN32) && defined(PYSCRIPT_DLL)
  #ifdef PYSCRIPT_EXPORTS
    #define PYSCRIPT_API __declspec(dllexport)
  #else
    #define PYSCRIPT_API __declspec(dllimport)
  #endif
#else
  #define PYSCRIPT_API
#endif


#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/nodes/SoSubNode.h>

class SoPyScript;
class SoPyScriptP;
class SoSensor;

typedef void SoPyScriptEvaluateCB(void * closure, SoPyScript * node);

class PYSCRIPT_API SoPyScript : public SoNode {
  typedef SoNode inherited;
    
public:
  static void initClass(void);
  SoPyScript(void);
  
  static SoType getClassTypeId(void);
  virtual SoType getTypeId(void) const;

  SoSFString script;     // holds the Python script
  SoSFBool mustEvaluate; // immediate or delayed evaluation

protected:
  virtual ~SoPyScript();

  virtual void doPyAction(SoAction * action, const char * funcname);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void GLRenderBelowPath(SoGLRenderAction * action);
  virtual void GLRenderInPath(SoGLRenderAction * action);
  virtual void GLRenderOffPath(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void getMatrix(SoGetMatrixAction * action);
  virtual void handleEvent(SoHandleEventAction * action);
  virtual void pick(SoPickAction * action);
  virtual void rayPick(SoRayPickAction * action);
  virtual void search(SoSearchAction * action);
  virtual void write(SoWriteAction * action);
  virtual void audioRender(SoAudioRenderAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

  virtual void copyContents(const SoFieldContainer * from, SbBool copyconn);
  virtual void notify(SoNotList * list);

private:
  static SoType classTypeId;
  static void * createInstance(void);

  virtual SbBool readInstance(SoInput * in, unsigned short flags);

  SoFieldData * fielddata;
  void initFieldData(void);
  virtual const SoFieldData * getFieldData(void) const;

  void executePyScript(void);

  static void eval_cb(void * data, SoSensor *);

  SoPyScriptP * pimpl;
  friend class SoPyScriptP;
};

#endif // !PIVY_SOPYSCRIPT_H
