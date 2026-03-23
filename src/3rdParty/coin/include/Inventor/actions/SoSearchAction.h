#ifndef COIN_SOSEARCHACTION_H
#define COIN_SOSEARCHACTION_H

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

#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoSubAction.h>
#include <Inventor/tools/SbLazyPimplPtr.h>
#include <Inventor/SbName.h>
#include <Inventor/lists/SoPathList.h>

class SoSearchActionP;

class COIN_DLL_API SoSearchAction : public SoAction {
  typedef SoAction inherited;

  SO_ACTION_HEADER(SoSearchAction);

public:
  static void initClass(void);

  SoSearchAction(void);
  virtual ~SoSearchAction(void);

  enum LookFor { NODE = 1, TYPE = 2, NAME = 4 };
  enum Interest { FIRST, LAST, ALL };

  void setNode(SoNode * const node);
  SoNode * getNode(void) const;
  void setType(const SoType type, const SbBool chkderived = TRUE);
  SoType getType(SbBool & chkderived) const;
  void setName(const SbName name);
  SbName getName(void) const;
  void setFind(const int what);
  int getFind(void) const;
  void setInterest(const Interest interest);
  Interest getInterest(void) const;
  void setSearchingAll(const SbBool searchall);
  SbBool isSearchingAll(void) const;
  SoPath * getPath(void) const;
  SoPathList & getPaths(void);
  void reset(void);

  void setFound(void);
  SbBool isFound(void) const;
  void addPath(SoPath * const path);

  // Obsoleted global flag, only present for compatibility reasons
  // with old SGI / TGS Inventor application code.
  static SbBool duringSearchAll;

protected:
  virtual void beginTraversal(SoNode * node);

private:
  int lookfor;
  Interest interest;
  SbBool searchall, chkderived;
  SoNode * node;
  SoType type;
  SbName name;
  SoPath * path;
  SoPathList paths;

private:
  SbLazyPimplPtr<SoSearchActionP> pimpl;

  // NOT IMPLEMENTED:
  SoSearchAction(const SoSearchAction & rhs);
  SoSearchAction & operator = (const SoSearchAction & rhs);
}; // SoSearchAction

#endif // !COIN_SOSEARCHACTION_H
