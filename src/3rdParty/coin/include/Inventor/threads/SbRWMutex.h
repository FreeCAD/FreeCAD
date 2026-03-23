#ifndef COIN_SBRWMUTEX_H
#define COIN_SBRWMUTEX_H

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

#include <Inventor/C/threads/rwmutex.h>

class SbRWMutex {
public:
  enum Precedence {
    READ_PRECEDENCE,
    WRITE_PRECEDENCE
  };

  SbRWMutex(Precedence policy) {
    this->rwmutex = cc_rwmutex_construct_etc(
      (policy == WRITE_PRECEDENCE)? CC_WRITE_PRECEDENCE : CC_READ_PRECEDENCE);
  }
  ~SbRWMutex(void) { cc_rwmutex_destruct(this->rwmutex); }

  int writeLock(void) { 
    return cc_rwmutex_write_lock(this->rwmutex) == CC_OK ? 0 : 1; 
  }
  SbBool tryWriteLock(void) { 
    return cc_rwmutex_write_try_lock(this->rwmutex) == CC_OK; 
  }
  int writeUnlock(void) { 
    return cc_rwmutex_write_unlock(this->rwmutex) == CC_OK ? 0 : 1; 
  }
  
  int readLock(void) { 
    return cc_rwmutex_read_lock(this->rwmutex) == CC_OK ? 0 : 1; 
  }
  int tryReadLock(void) { 
    return cc_rwmutex_read_try_lock(this->rwmutex) == CC_OK; 
  }
  int readUnlock(void) { 
    return cc_rwmutex_read_unlock(this->rwmutex) == CC_OK ? 0 : 1; 
  }

private:
  cc_rwmutex * rwmutex;
};

#endif // !COIN_SBRWMUTEX_H
