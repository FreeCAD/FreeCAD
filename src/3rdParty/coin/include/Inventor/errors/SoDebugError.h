#ifndef COIN_SODEBUGERROR_H
#define COIN_SODEBUGERROR_H

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

#include <Inventor/errors/SoError.h>
#include <Inventor/C/errors/debugerror.h>

// Avoid problem with Microsoft Win32 API headers (yes, they actually
// #define ERROR -- in wingdi.h).
#if defined(ERROR)
#define SODEBUGERROR_STORE_ERROR_DEF
#pragma push_macro("ERROR")
#undef ERROR
#endif /* ERROR */


class COIN_DLL_API SoDebugError : public SoError {
  typedef SoError inherited;

public:
  enum Severity { ERROR, WARNING, INFO };

  static void setHandlerCallback(SoErrorCB * const function,
                                 void * const data);
  static SoErrorCB * getHandlerCallback(void);
  static void * getHandlerData(void);

  static SoType getClassTypeId(void);
  virtual SoType getTypeId(void) const;

  SoDebugError::Severity getSeverity(void) const;

  static void post(const char * const source, const char * const format, ...);
  static void postWarning(const char * const source, const char * const format, ...);
  static void postInfo(const char * const source, const char * const format, ...);

  static void initClass(void);

protected:
  virtual SoErrorCBPtr getHandler(void * & data) const;

private:
  static void callbackForwarder(const struct cc_debugerror * error, void * data);
  static void commonPostHandling(Severity severity, const char * type,
                                 const char * source, const SbString & s);

  static SoType classTypeId;
  static SoErrorCB * callback;
  static void * callbackData;
  Severity severity;
};

// Avoid problem with Microsoft Win32 API headers (see above).
#if defined(SODEBUGERROR_STORE_ERROR_DEF)
#pragma pop_macro("ERROR")
#undef SODEBUGERROR_STORE_ERROR_DEF
#endif /* SODEBUGERROR_STORE_ERROR_DEF */

#endif // !COIN_SODEBUGERROR_H
