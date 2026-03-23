#ifndef COIN_SBCLIP_H
#define COIN_SBCLIP_H
 
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

#include <Inventor/lists/SbList.h>
#include <Inventor/SbVec3f.h>
#include <cstddef>

class SbPlane;
class SbVec2f;

typedef void * SbClipCallback(const SbVec3f & v0, void * vdata0, 
                              const SbVec3f & v1, void * vdata1,
                              const SbVec3f & newvertex,
                              void * userdata);

class COIN_DLL_API SbClip {
public:
  SbClip(SbClipCallback * callback = NULL, void * userdata = NULL);
  
  void addVertex(const SbVec3f &v, void * vdata = NULL);  
  void reset(void);

  void clip(const SbPlane & plane);

  int getNumVertices(void) const;
  void getVertex(const int idx, SbVec3f & v, void ** vdata = NULL) const; 
  void * getVertexData(const int idx) const;
  
private:
  class SbClipData {
  public:
    SbClipData(void) {}
    SbClipData(const SbVec3f & v, void * data) 
      : vertex(v),
        data(data) {}
  public:
    void get(SbVec3f &v, void *& dataref) {
      v = this->vertex;
      dataref = this->data;
    }

    SbVec3f vertex;
    void * data;
  };

  SbClipCallback * callback;
  void * cbdata;
  SbList <SbClipData> array[2];
  int curr;
  void outputVertex(const SbVec3f &v, void * data);
};


#endif // !COIN_SBCLIP_H
