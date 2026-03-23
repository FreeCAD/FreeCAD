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
  \class SoMemoryError SoMemoryError.h Inventor/errors/SoMemoryError.h
  \brief The SoMemoryError class is used to inform of problems with memory allocation.

  \ingroup coin_errors

  Modern operating systems takes care of handling most out of memory
  conditions for you, but in certain situations it can be wise to do
  some manual checking and intervention. This class is provided as an
  aid to help out in these situations.

  The basic design of the Coin library is to pass on the
  responsibility for handling failed attempts at memory allocation to
  the application programmer. If you want to detect and take care of
  these, you should compile Coin with the C++ exception throwing on
  and wrap your code within \c try{} and \c catch{} blocks. The most
  you can do if you get a failed allocation is typically to notify the
  user and then exit the application, though, and this is something
  most operating systems will do for you, so you probably need not
  consider this at all.

  So, where does the SoMemoryError class come in handy? There are
  certain things which could happen within the library which are best
  taken care of by internally handling failed attempts at memory
  allocation. An example: the user tries to load a model file which
  contains a filename pointer to a huge bitmap file with a texture
  map. The end-user's system does not provide enough memory to load
  the file and prepare the texture image for rendering, though. This
  is a case where it is possible to just emit a warning and
  continue. The warning will then be passed through this class.

  Note that SoMemoryError is probably not of much use to the
  application programmer.
*/

// *************************************************************************

#include <Inventor/errors/SoMemoryError.h>

#include <cstdio>

#include <Inventor/SoType.h>
#include <Inventor/SbName.h>

// *************************************************************************

SoType SoMemoryError::classTypeId STATIC_SOTYPE_INIT;
SoErrorCB * SoMemoryError::callback = SoError::defaultHandlerCB;
void * SoMemoryError::callbackData = NULL;

// *************************************************************************

/*!
  \copydetails SoError::initClass(void)
*/
void
SoMemoryError::initClass(void)
{
  SoMemoryError::callback = SoError::defaultHandlerCB;
  SoMemoryError::callbackData = NULL;
  SoMemoryError::classTypeId =
    SoType::createType(SoError::getClassTypeId(), "MemoryError");
}

/*!
  \copydetails SoError::getClassTypeId(void)
*/
SoType
SoMemoryError::getClassTypeId(void)
{
  return SoMemoryError::classTypeId;
}

// Documented for parent class.
SoType
SoMemoryError::getTypeId(void) const
{
  return SoMemoryError::classTypeId;
}

/*!
  \copydetails SoError::setHandlerCallback(SoErrorCB * const function, void * const data)
*/
void
SoMemoryError::setHandlerCallback(SoErrorCB * const function,
                                  void * const data)
{
  /* FIXME: Overriding the error handler for subclasses of SoError
     doesn't work yet. Use SoError::setHandlerCallback() instead as a
     workaround, but note that this will stop working when callback
     override is implemented properly. 2003-01-22 thammer.
  */
  SoMemoryError::callback = function;
  SoMemoryError::callbackData = data;
}

/*!
  \copydetails SoError::getHandlerCallback(void)
*/
SoErrorCB *
SoMemoryError::getHandlerCallback(void)
{
  return SoMemoryError::callback;
}

/*!
  \copydetails SoError::getHandlerData(void)
*/
void *
SoMemoryError::getHandlerData(void)
{
  return SoMemoryError::callbackData;
}

/*!
  Posts a warning about a failed memory allocation. \a whatWasAllocated
  should contain a description of what we tried to allocate.
*/
void
SoMemoryError::post(const char * const whatWasAllocated)
{
  SoMemoryError error;
  error.setDebugString("ERROR allocating '");
  error.appendToDebugString(whatWasAllocated);
  error.appendToDebugString("'.");
  error.handleError();
}

// Documented for parent class.
SoErrorCB * SoMemoryError::getHandler(void * & data) const
{
  data = SoMemoryError::callbackData;
  return SoMemoryError::callback;
}
