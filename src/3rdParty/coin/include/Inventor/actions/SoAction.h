#ifndef COIN_SOACTION_H
#define COIN_SOACTION_H

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
#include <Inventor/SoType.h>
#include <Inventor/misc/SoTempPath.h>
#include <Inventor/tools/SbPimplPtr.h>

// Include instead of using forward declarations to be compatible with
// Open Inventor (and besides, all the other action class definitions
// needs to know about these lists).
#include <Inventor/lists/SoActionMethodList.h>
#include <Inventor/lists/SoEnabledElementsList.h>

// defined in Inventor/SbBasic.h
#ifdef COIN_UNDEF_IN_PATH_HACK
#include <sys/unistd.h>
#undef IN_PATH
// Avoid problem with HPUX 10.20 C library API headers, which defines IN_PATH
// in <sys/unistd.h>.  That define destroys the SoAction::PathCode enum, and
// the preprocessor is so broken that we can't store/restore the define for
// the duration of this header file.
#endif // COIN_UNDEF_IN_PATH_HACK


/*!
  The SO_ENABLE macro is a convenient mechanism for letting an action
  class specify which elements it needs during its operation.
*/
#define SO_ENABLE(action, element) \
  do { \
    assert(!element::getClassTypeId().isBad()); \
    action::enableElement(element::getClassTypeId(), \
                          element::getClassStackIndex()); \
  } WHILE_0


class SoEnabledElementsList;
class SoNode;
class SoPath;
class SoPathList;
class SoState;
class SoActionP;

class COIN_DLL_API SoAction {
public:
  static void initClass(void);
  static void initClasses(void);

  enum AppliedCode { NODE = 0, PATH = 1, PATH_LIST = 2 };
  enum PathCode { NO_PATH = 0, IN_PATH = 1, BELOW_PATH = 2, OFF_PATH = 3 };

  virtual ~SoAction(void);

  static SoType getClassTypeId(void);
  virtual SoType getTypeId(void) const = 0;
  virtual SbBool isOfType(SoType type) const;

  virtual void apply(SoNode * root);
  virtual void apply(SoPath * path);
  virtual void apply(const SoPathList & pathlist, SbBool obeysrules = FALSE);
  void apply(SoAction * beingApplied);
  virtual void invalidateState(void);

  static void nullAction(SoAction * action, SoNode * node);

  AppliedCode getWhatAppliedTo(void) const;
  SoNode * getNodeAppliedTo(void) const;
  SoPath * getPathAppliedTo(void) const;
  const SoPathList * getPathListAppliedTo(void) const;
  const SoPathList * getOriginalPathListAppliedTo(void) const;

  SbBool isLastPathListAppliedTo(void) const;

  PathCode getPathCode(int & numindices, const int * & indices);

  void traverse(SoNode * const node);
  SbBool hasTerminated(void) const;

  const SoPath * getCurPath(void);
  SoState * getState(void) const;

  PathCode getCurPathCode(void) const;
  virtual SoNode * getCurPathTail(void);
  void usePathCode(int & numindices, const int * & indices);
  
  void pushCurPath(const int childindex, SoNode * node = NULL);
  void popCurPath(const PathCode prevpathcode);
  void pushCurPath(void);
  
  void popPushCurPath(const int childindex, SoNode * node = NULL);
  void popCurPath(void);

public:
  void switchToPathTraversal(SoPath * path);
  void switchToNodeTraversal(SoNode * node);

protected:
  SoAction(void);

  virtual void beginTraversal(SoNode * node);
  virtual void endTraversal(SoNode * node);
  void setTerminated(const SbBool flag);

  virtual const SoEnabledElementsList & getEnabledElements(void) const;
  virtual SbBool shouldCompactPathList(void) const;

  SoState * state;
  SoActionMethodList * traversalMethods;

  /* These two methods are not available in the original OIV API.  The
     reason they have been added is explained in SoSubAction.h for the
     SO_ACTION_HEADER macro. */
  static SoEnabledElementsList * getClassEnabledElements(void);
  static SoActionMethodList * getClassActionMethods(void);

private:
  static SoType classTypeId;
  /* The enabledElements and methods variables are protected in the
     original OIV API. This is not such a good idea, see the comments
     in SoSubAction.h for SO_ACTION_HEADER. */

  static void atexit_cleanup(void);
  static SoEnabledElementsList * enabledElements;
  static SoActionMethodList * methods;
  SoTempPath currentpath;
  PathCode currentpathcode;

private:
  SbPimplPtr<SoActionP> pimpl;

  // NOT IMPLEMENTED:
  SoAction(const SoAction & rhs);
  SoAction & operator = (const SoAction & rhs);
}; // SoAction

// inline methods

inline SoAction::PathCode
SoAction::getCurPathCode(void) const
{
  return this->currentpathcode;
}

#endif // !COIN_SOACTION_H
