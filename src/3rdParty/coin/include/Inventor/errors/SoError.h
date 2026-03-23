#ifndef COIN_SOERROR_H
#define COIN_SOERROR_H

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

#include <Inventor/SbBasic.h>
#include <Inventor/SbString.h>
#include <Inventor/C/errors/error.h>
#include <Inventor/SoType.h>

class SoBase;
class SoNode;
class SoPath;
class SoEngine;

typedef void SoErrorCB(const class SoError * error, void * data);
typedef SoErrorCB * SoErrorCBPtr;


class COIN_DLL_API SoError {
public:
  SoError(void) { cc_error_init(&this->err); }
  virtual ~SoError() { cc_error_clean(&this->err); }

  static void setHandlerCallback(SoErrorCB * const func, void * const data);
  static SoErrorCB * getHandlerCallback(void);
  static void * getHandlerData(void);

  const SbString & getDebugString(void) const;

  static SoType getClassTypeId(void);
  virtual SoType getTypeId(void) const;
  SbBool isOfType(const SoType type) const;

  static void post(const char * const format, ...);

  static SbString getString(const SoNode * const node);
  static SbString getString(const SoPath * const path);
  static SbString getString(const SoEngine * const engine);

  static void initClass(void);
  static void initClasses(void);

protected:
  static void defaultHandlerCB(const SoError * error, void * userdata);
  virtual SoErrorCBPtr getHandler(void * & data) const;

  void setDebugString(const char * const str);
  void appendToDebugString(const char * const str);

  void handleError(void);

private:
  SoError(const cc_error * error);
  static void generateBaseString(SbString & str, const SoBase * const base,
                                 const char * const what);

  static void callbackForwarder(const cc_error * err, void * data);

  static SoType classTypeId;
  static SoErrorCB * callback;
  static void * callbackData;
  SbString debugstring;

  cc_error err;
};

#endif // !COIN_SOERROR_H
