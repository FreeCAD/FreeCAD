#ifndef COIN_SOJAVASCRIPTENGINE_H
#define COIN_SOJAVASCRIPTENGINE_H

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
  
#include <Inventor/misc/SoScriptEngine.h>
#include <Inventor/C/glue/spidermonkey.h>
#include <Inventor/lists/SbList.h>

// *************************************************************************

#define COIN_JAVASCRIPT_RUNTIME_MAXBYTES 4194304

// *************************************************************************

typedef void SoJSfield2jsvalFunc(JSContext *, const SoField *, jsval *);
typedef SbBool SoJSjsval2field2Func(JSContext *, const jsval, SoField *);
typedef JSObject * SoJSWrapperInitFunc(JSContext *, JSObject * obj);

// *************************************************************************

class COIN_DLL_API SoJavaScriptEngine : public SoScriptEngine {

public:
  SoJavaScriptEngine();
  virtual ~SoJavaScriptEngine();

  virtual SbBool executeScript(const SbName & name, 
                               const SbString & script) const;
  virtual SbBool executeFile(const SbName & filename) const;
  virtual SbBool executeFunction(const SbName &name, int argc, 
                                 const SoField * argv, 
                                 SoField * rval = NULL) const;

  virtual SbBool setScriptField(const SbName & name, const SoField * f) const;
  virtual SbBool unsetScriptField(const SbName & name) const;
  virtual SbBool getScriptField(const SbName & name, SoField * f) const;
  virtual SbBool hasScriptField(const SbName & name) const;

  // Everything under here is javascript specific

  static SoJavaScriptEngine *getEngine(JSContext * cx);

  SbBool field2jsval(const SoField * f, jsval * v) const;
  SbBool jsval2field(const jsval v, SoField * f) const;
  void enableAutoNodeUnref(SbBool onoff);
  SbBool getAutoNodeUnrefState(void) const;

  static SbBool init(uint32_t maxbytes = COIN_JAVASCRIPT_RUNTIME_MAXBYTES);
  static void shutdown(void);
  static SbBool debug(void);

  static JSRuntime * getRuntime(void);
  JSContext * getContext(void) const;
  JSObject * getGlobal(void) const;

  void addHandler(const SoType & type, SoJSWrapperInitFunc * init, 
                  SoJSfield2jsvalFunc * field2jsval, 
                  SoJSjsval2field2Func * jsval2field);

protected:
  static void setRuntime(JSRuntime * runtime);
  void setContext(JSContext * context);
  void setGlobal(JSObject * global);

private:
  class SoJavaScriptEngineP * pimpl; 
  friend class SoJavaScriptEngineP; 
};

#endif // !COIN_SOJAVASCRIPTENGINE_H
