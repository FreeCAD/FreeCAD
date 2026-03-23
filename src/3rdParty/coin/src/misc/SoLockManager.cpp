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
  \class SoLockManager SoLockMgr.h Inventor/lock/SoLockMgr.h
  \brief The SoLockManager is a defunct software license lock mechanism.

  \ingroup coin_general

  This is just a dummy implementation of the TGS Inventor class used
  to provide a license locking mechanism. Since Coin can be used
  without any royalty fees or client-site license locks, no such
  mechanism is needed.

  If you are looking for information about using Coin in proprietary
  applications for commercial distribution, read about the Coin
  Professional Edition License on the Coin web-pages <a
  href="https://coin3d.github.io">here</a>.
*/


#include <Inventor/lock/SoLockMgr.h>

#include <cstring>

#include <Inventor/SbString.h>

#include "tidbitsp.h"

class SoLockManager_pimpl {
public:
  SoLockManager_pimpl(void) { this->unlockstr = NULL; }
  ~SoLockManager_pimpl() { delete[] this->unlockstr; }

  char * unlockstr;
};

static SoLockManager_pimpl * solockmanager_pimpl = NULL;

static void solockmanager_cleanup(void)
{
  delete solockmanager_pimpl;
  solockmanager_pimpl = NULL;
}

/*!
  A void method provided just for source code compatibility in client
  applications with TGS Inventor.

  It just stores the \a unlockstr argument internally to be able to
  provide it upon calls to GetUnlockString().
*/
void
SoLockManager::SetUnlockString(char * unlockstr)
{
  if (!solockmanager_pimpl) {
    solockmanager_pimpl = new SoLockManager_pimpl;
    coin_atexit((coin_atexit_f*)solockmanager_cleanup, CC_ATEXIT_NORMAL);
  }
  delete[] solockmanager_pimpl->unlockstr;
  solockmanager_pimpl->unlockstr = new char[strlen(unlockstr) + 1];
  (void)strcpy(solockmanager_pimpl->unlockstr, unlockstr);
}

/*!
  A void method provided just for source code compatibility in client
  applications with TGS Inventor.

  Returns the string set through SetUnlockString().
*/
char *
SoLockManager::GetUnlockString(void)
{
  return solockmanager_pimpl ? solockmanager_pimpl->unlockstr : NULL;
}
