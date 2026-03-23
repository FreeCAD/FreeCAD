#ifndef COIN_SOBYTESTREAM_H
#define COIN_SOBYTESTREAM_H

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

#include <Inventor/system/inttypes.h>
#include <Inventor/SbBasic.h>

class SoNode;
class SoPath;
class SoPathList;


class COIN_DLL_API SoByteStream {

public:
  SoByteStream(void);
  ~SoByteStream();

  void convert(SoNode * node, SbBool binary = TRUE);
  void convert(SoPath * path, SbBool binary = TRUE);
  void convert(SoPathList * pl, SbBool binary = TRUE);

  void * getData(void);
  uint32_t getNumBytes(void);

  static SoPathList * unconvert(SoByteStream * stream);
  static SoPathList * unconvert(void * data, uint32_t bytesinstream);

  void copy(void * d, size_t len);
  SbBool isRawData(void) const;

private:
  // Placeholder for any data for the instance. Just added for the
  // sake of making it possible to implement this class after release
  // without breaking ABI compatibility.
  void * pimpl;
};

#endif // !COIN_SOBYTESTREAM_H
