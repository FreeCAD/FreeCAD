#ifndef COIN_SOGLOBALSIMPLIFYACTION_H
#define COIN_SOGLOBALSIMPLIFYACTION_H

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

// FIXME: this class has not yet been implemented in Coin. this is a
// placeholder / stub / skeleton code, not yet built (nor installed,
// obviously).  --mortene.

#include <Inventor/actions/SoSimplifyAction.h>
#include <Inventor/tools/SbLazyPimplPtr.h>

class SoGlobalSimplifyActionP;

class COIN_DLL_API SoGlobalSimplifyAction : public SoSimplifyAction {
  typedef SoSimplifyAction inherited;

  SO_ACTION_HEADER(SoGlobalSimplifyAction);

public:
  static void initClass(void);

  SoGlobalSimplifyAction(void);
  virtual ~SoGlobalSimplifyAction(void);

protected:
  virtual void beginTraversal(SoNode * node);

private:
  SbLazyPimplPtr<SoGlobalSimplifyActionP> pimpl;

  // NOT IMPLEMENTED:
  SoGlobalSimplifyAction(const SoGlobalSimplifyAction & rhs);
  SoGlobalSimplifyAction & operator = (const SoGlobalSimplifyAction & rhs);
}; // SoGlobalSimplifyAction

#endif // !COIN_SOGLOBALSIMPLIFYACTION_H
