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
  \class SoReadError SoReadError.h Inventor/errors/SoReadError.h
  \brief The SoReadError class is used during model import operations.

  \ingroup coin_errors

  During model file import, this class will be used to output any
  error or warning messages.

  Depending on your application, setting up your own error handler
  callbacks for SoReadError might be a good idea.
*/

// *************************************************************************

#include <Inventor/errors/SoReadError.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include <Inventor/SoType.h>
#include <Inventor/SoInput.h>
#include <Inventor/SbName.h>

// *************************************************************************

SoType SoReadError::classTypeId STATIC_SOTYPE_INIT;
SoErrorCB * SoReadError::callback = SoError::defaultHandlerCB;
void * SoReadError::callbackData = NULL;

// *************************************************************************

/*!
  \copybrief SoError::initClass(void)
*/
void
SoReadError::initClass(void)
{
  SoReadError::callback = SoError::defaultHandlerCB;
  SoReadError::callbackData = NULL;
  SoReadError::classTypeId =
    SoType::createType(SoError::getClassTypeId(), "ReadError");
}

/*!
  \copydetails SoError::getClassTypeId(void)
*/
SoType
SoReadError::getClassTypeId(void)
{
  return SoReadError::classTypeId;
}

// Documented for parent class.
SoType
SoReadError::getTypeId(void) const
{
  return SoReadError::classTypeId;
}

/*!
  \copydetails SoError::setHandlerCallback(SoErrorCB * const function, void * const data)
*/
void
SoReadError::setHandlerCallback(SoErrorCB * const function, void * const data)
{
  SoReadError::callback = function;
  SoReadError::callbackData = data;
}

/*!
  \copydetails SoError::getHandlerCallback(void)
*/
SoErrorCB *
SoReadError::getHandlerCallback(void)
{
  return SoReadError::callback;
}

/*!
  \copydetails SoError::getHandlerData(void)
*/
void *
SoReadError::getHandlerData(void)
{
  return SoReadError::callbackData;
}

/*!
  Method used from import code to post error or warning messages for
  model files which are not 100% compliant to the format specification.

  The messages will be wrapped within information on line number,
  file offset etc.
*/
void
SoReadError::post(const SoInput * const in, const char * const format, ...)
{
  va_list args;
  va_start(args, format);
  SbString formatstr;
  formatstr.vsprintf(format, args);
  va_end(args);

  SoReadError error;
  error.setDebugString("Coin read error: ");
  error.appendToDebugString(formatstr.getString());
  error.appendToDebugString("\n");

  SbString s;
  in->getLocationString(s);
  error.appendToDebugString(s.getString());

  // for some reason the getIVVersion() function is not const. We
  // therefore need to make the SoInput cast here to be able to test
  // for Inventor V1.0 files
  if (const_cast<SoInput *>(in)->getIVVersion() == 1.0f) {
    error.appendToDebugString("\nThis operation might have failed due to limited "
                              "support for old Inventor files. If this is the case, "
                              "please get in touch with Kongsberg Oil & Gas Technologies "
                              "at <coin-support@coin3d.org>, and we will "
                              "rectify the situation.");
  }

  if (SoReadError::callback != SoError::defaultHandlerCB) {
    SoReadError::callback(&error, SoReadError::callbackData);
  }
  else {
    error.handleError();
  }
}

// Documented for parent class.
SoErrorCB *
SoReadError::getHandler(void * & data) const
{
  data = SoReadError::callbackData;
  return SoReadError::callback;
}
