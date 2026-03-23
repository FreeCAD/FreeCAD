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
  \class SoVBOCache SoVBOCache.h Inventor/caches/SoVBOCache.h
  \brief The SoVBOCache is used to organize pointers to SoVBO and SoVertexArrayIndexer instances.

  \ingroup coin_caches
*/

#include "caches/SoVBOCache.h"
#include "rendering/SoVBO.h"
#include "rendering/SoVertexArrayIndexer.h"
#include <Inventor/lists/SbList.h>

class SoVBOCacheP {
public:
  SoVBOCacheP(void) {
    this->vaindexer = NULL;
    this->coordvbo = NULL;
    this->colorvbo = NULL;
    this->normalvbo = NULL;
  }
  ~SoVBOCacheP() {
    delete this->vaindexer;
    delete this->coordvbo;
    delete this->colorvbo;
    delete this->normalvbo;
    
    for (int i = 0; i < this->texcoordvbo.getLength(); i++) {
      delete this->texcoordvbo[i];
    }
  }
  
  SoVertexArrayIndexer * vaindexer;
  SoVBO * coordvbo;
  SoVBO * colorvbo;
  SoVBO * normalvbo;
  SbList <SoVBO*> texcoordvbo;
};

/*!
  Constructor.
*/
SoVBOCache::SoVBOCache(SoState * state)
  : SoCache(state)
{
  this->pimpl = new SoVBOCacheP();
}

/*!
  Destructor.
*/
SoVBOCache::~SoVBOCache()
{
  delete this->pimpl;
}

/*!
  Returns the vertex array indexer. If \a createifnull is TRUE, a
  indexer will be created if it doesn't exist.
*/
SoVertexArrayIndexer * 
SoVBOCache::getVertexArrayIndexer(const SbBool createifnull)
{
  if (createifnull && (this->pimpl->vaindexer == NULL)) {
    this->pimpl->vaindexer = new SoVertexArrayIndexer;
  }
  return this->pimpl->vaindexer;
}

/*!
  Returns the coordinate VBO instance. If \a createifnull is TRUE, a
  VBO instance will be created if it doesn't exist.
*/
SoVBO * 
SoVBOCache::getCoordVBO(const SbBool createifnull)
{
  if (createifnull && (this->pimpl->coordvbo == NULL)) {
    this->pimpl->coordvbo = new SoVBO;
  }
  return this->pimpl->coordvbo;
}

/*!
  Returns the normal vertex VBO instance. If \a createifnull is TRUE, a
  VBO instance will be created if it doesn't exist.
*/

SoVBO * 
SoVBOCache::getNormalVBO(const SbBool createifnull)
{
  if (createifnull && (this->pimpl->normalvbo == NULL)) {
    this->pimpl->normalvbo = new SoVBO;
  }
  return this->pimpl->normalvbo;
}

/*!
  Returns the color VBO instance. If \a createifnull is TRUE, a
  VBO instance will be created if it doesn't exist.
*/
SoVBO * 
SoVBOCache::getColorVBO(const SbBool createifnull)
{
  if (createifnull && (this->pimpl->colorvbo == NULL)) {
    this->pimpl->colorvbo = new SoVBO;
  }
  return this->pimpl->colorvbo;
}

/*!
  Returns the texture coordinate VBO instance. If \a createifnull is TRUE, a
  VBO instance will be created if it doesn't exist.
*/
SoVBO * 
SoVBOCache::getTexCoordVBO(const int unit, const SbBool createifnull)
{
  if (createifnull) {
    while (this->pimpl->texcoordvbo.getLength() <= unit) {
      this->pimpl->texcoordvbo.append(NULL);
    }
    if (this->pimpl->texcoordvbo[unit] == NULL) {
      this->pimpl->texcoordvbo[unit] = new SoVBO;
    }
  }
  if (this->pimpl->texcoordvbo.getLength() > unit) {
    return this->pimpl->texcoordvbo[unit];
  }
  return NULL;
}
