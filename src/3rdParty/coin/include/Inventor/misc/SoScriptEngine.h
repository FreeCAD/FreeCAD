#ifndef COIN_SOSCRIPTENGINE_H
#define COIN_SOSCRIPTENGINE_H

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
#include <Inventor/SbName.h>
#include <Inventor/fields/SoField.h>

class COIN_DLL_API SoScriptEngine {

public:
  // FIXME: should all these functions be const? 20050719 erikgors.
  // FIXME: what about the return value from executing a script?
  // 20050719 erikgors.
  SoScriptEngine() { } 
  virtual ~SoScriptEngine() { } 
  virtual SbBool executeScript(const SbName & name, 
                               const SbString & script) const = 0;
  virtual SbBool executeFile(const SbName & filename) const = 0;
  virtual SbBool executeFunction(const SbName & name, int argc, 
                                 const SoField * argv, 
                                 SoField * rval = NULL) const = 0;

  virtual SbBool setScriptField(const SbName & name, 
                                const SoField * f) const = 0;
  virtual SbBool getScriptField(const SbName & name, SoField * f) const = 0;
  virtual SbBool unsetScriptField(const SbName & name) const = 0;
  virtual SbBool hasScriptField(const SbName & name) const = 0;
};

#endif // !COIN_SOSCRIPTENGINE_H
