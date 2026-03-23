#ifndef COIN_SBFIFO_H
#define COIN_SBFIFO_H

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

#include <Inventor/C/threads/fifo.h>

class SbFifo {
public:
  SbFifo(void) { this->fifo = cc_fifo_new(); }
  ~SbFifo(void) { cc_fifo_delete(this->fifo); }

  void assign(void * ptr, uint32_t type)
    { cc_fifo_assign(this->fifo, ptr, type); }
  void retrieve(void *& ptr, uint32_t &type)
    { cc_fifo_retrieve(this->fifo, &ptr, &type); }
  SbBool tryRetrieve(void *& ptr, uint32_t & type)
    { return cc_fifo_try_retrieve(this->fifo, &ptr, &type); }

  unsigned int size(void) const { return cc_fifo_size(this->fifo); }

  void lock(void) const { cc_fifo_lock(this->fifo); }
  void unlock(void) const { cc_fifo_unlock(this->fifo); }

  // lock/unlock only needed around the following operations:
  SbBool peek(void *& item, uint32_t & type) const
    { return cc_fifo_peek(this->fifo, &item, &type); }
  SbBool contains(void * item) const
    { return cc_fifo_contains(this->fifo, item); }
  SbBool reclaim(void * item)
    { return cc_fifo_reclaim(this->fifo, item); }

private:
  cc_fifo * fifo;
};

#endif // !COIN_SBFIFO_H
