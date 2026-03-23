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

// FIXME: this class has not been implemented yet. 20000627 mortene.

#include <Inventor/misc/SoByteStream.h>
#include <coindefs.h> // COIN_STUB
#include <cstdlib>


SoByteStream::SoByteStream(void)
{
  COIN_STUB();
}

SoByteStream::~SoByteStream()
{
  COIN_STUB();
}

void
SoByteStream::convert(SoNode * COIN_UNUSED_ARG(node), SbBool COIN_UNUSED_ARG(binary))
{
  COIN_STUB();
}

void
SoByteStream::convert(SoPath * COIN_UNUSED_ARG(path), SbBool COIN_UNUSED_ARG(binary))
{
  COIN_STUB();
}

void
SoByteStream::convert(SoPathList * COIN_UNUSED_ARG(pl), SbBool COIN_UNUSED_ARG(binary))
{
  COIN_STUB();
}

void *
SoByteStream::getData(void)
{
  COIN_STUB();
  return NULL;
}

uint32_t
SoByteStream::getNumBytes(void)
{
  COIN_STUB();
  return 0;
}

SoPathList *
SoByteStream::unconvert(SoByteStream * COIN_UNUSED_ARG(stream))
{
  COIN_STUB();
  return NULL;
}

SoPathList *
SoByteStream::unconvert(void * COIN_UNUSED_ARG(data), uint32_t COIN_UNUSED_ARG(bytesinstream))
{
  COIN_STUB();
  return NULL;
}

void
SoByteStream::copy(void * COIN_UNUSED_ARG(d), size_t COIN_UNUSED_ARG(len))
{
  COIN_STUB();
}

SbBool
SoByteStream::isRawData(void) const
{
  COIN_STUB();
  return FALSE;
}
