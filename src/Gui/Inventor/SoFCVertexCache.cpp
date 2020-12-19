/**************************************************************************\
 * Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
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

#include "PreCompiled.h"

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cfloat>

#include <Inventor/C/glue/gl.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/elements/SoBumpMapCoordinateElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoCacheHintElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/lists/SbPList.h>
#include <Inventor/system/gl.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/threads/SbMutex.h>

#include "SoFCVertexCache.h"
#include "SoFCDiffuseElement.h"
#include "SoFCVBO.h"
#include "SoFCVertexArrayIndexer.h"

using namespace Gui;

// *************************************************************************
typedef SoFCVertexAttribute<SbVec2f> Vec2Array;
typedef SoFCVertexAttribute<SbVec3f> Vec3Array;
typedef SoFCVertexAttribute<SbVec4f> Vec4Array;
typedef SoFCVertexAttribute<uint8_t> ByteArray;

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->master)

static SbName * PartIndexField;

class SoFCVertexCacheP {
public:
  enum Arrays {
    NORMAL = SoFCVertexCache::NORMAL,
    TEXCOORD = SoFCVertexCache::TEXCOORD,
    COLOR = SoFCVertexCache::COLOR,
    SORTED_ARRAY = SoFCVertexCache::SORTED_ARRAY,
    FULL_SORTED_ARRAY = SoFCVertexCache::FULL_SORTED_ARRAY,
    NON_SORTED_ARRAY = SoFCVertexCache::NON_SORTED_ARRAY,
    ALL = SoFCVertexCache::ALL,
  };

  static void initClass()
  {
    PartIndexField = new SbName("partIndex");
  }

  static void cleanup()
  {
    delete PartIndexField;
    PartIndexField = NULL;
  }

  SoFCVertexCacheP(SoFCVertexCache * m,
                   SoFCVertexCache * prev,
                   SbFCUniqueId id)
    : master(m),
      prevcache(prev),
      prevattached(false),
      nodeid(id),
      diffuseid(0),
      transpid(0),
      triangleindexer(nullptr),
      lineindexer(nullptr),
      pointindexer(nullptr),
      partindices(nullptr),
      partcount(0)
  {
    if (prev) {
      if (PRIVATE(prev)->triangleindexer)
        this->prevtriangleindices = PRIVATE(prev)->triangleindexer->getIndexArray();
      if (PRIVATE(prev)->lineindexer)
        this->prevlineindices = PRIVATE(prev)->lineindexer->getIndexArray();
      if (PRIVATE(prev)->pointindexer)
        this->prevpointindices = PRIVATE(prev)->pointindexer->getIndexArray();
    }
  }

  ~SoFCVertexCacheP()
  {
    delete triangleindexer;
    delete lineindexer;
    delete pointindexer;
  }

  uint32_t getColor(const SoFCVertexArrayIndexer * indexer, int part) const;

  struct Vertex {
  public:
    SbVec3f vertex;
    SbVec3f normal;
    SbVec4f texcoord0;
    SbVec2f bumpcoord;
    uint32_t color;
    int texcoordidx;

    bool operator==(const Vertex & v) const {
      return
        (this->vertex == v.vertex) &&
        (this->normal == v.normal) &&
        (this->texcoord0 == v.texcoord0) &&
        (this->bumpcoord == v.bumpcoord) &&
        (this->texcoordidx == v.texcoordidx) &&
        (this->color == v.color);
    }
  };

  struct VertexHasher {
    // copied from boost::hash_combine. 
    template <class T>
    static inline void hash_combine(std::size_t& seed, const T& v)
    {
      std::hash<T> hasher;
      seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    unsigned long operator()(const Vertex &v) const {
      unsigned long seed = 0;
      hash_combine(seed, v.color);
      hash_combine(seed, v.vertex[0]);
      hash_combine(seed, v.vertex[1]);
      hash_combine(seed, v.vertex[2]);
      hash_combine(seed, v.normal[0]);
      hash_combine(seed, v.normal[1]);
      hash_combine(seed, v.normal[2]);
      hash_combine(seed, v.texcoord0[0]);
      hash_combine(seed, v.texcoord0[1]);
      hash_combine(seed, v.texcoord0[2]);
      hash_combine(seed, v.texcoord0[3]);
      hash_combine(seed, v.bumpcoord[0]);
      hash_combine(seed, v.bumpcoord[1]);
      hash_combine(seed, v.texcoordidx);
      return seed;
    }
  };

  SbBool depthSortTriangles(SoState * state, bool fullsort);

  SoFCVertexCache *master;
  CoinPtr<SoFCVertexCache> prevcache;
  bool prevattached;

  SbFCUniqueId nodeid;
  SbFCUniqueId diffuseid;
  SbFCUniqueId transpid;

  CoinPtr<Vec3Array> vertexarray;
  CoinPtr<Vec3Array> normalarray;
  CoinPtr<Vec4Array> texcoord0array;
  CoinPtr<Vec2Array> bumpcoordarray;
  CoinPtr<ByteArray> colorarray;
  CoinPtr<ByteArray> prevcolorarray;
  std::vector<CoinPtr<Vec4Array> > multitexarray;

  std::unordered_map<Vertex, int32_t, VertexHasher> vhash;

  const SbVec2f * bumpcoords;
  int numbumpcoords;

  const uint32_t * packedptr;
  const SbColor * diffuseptr;
  const float * transpptr;

  const SoLazyElement * lazyelement;
  int numdiffuse;
  int numtransp;
  int numtranspparts;
  bool hastransp;
  int colorpervertex;
  uint32_t firstcolor;

  const SbBool * enabledunits;
  int lastenabled;
  const SoMultiTextureCoordinateElement * multielem;
  SoState * state;
  SbPlane prevsortplane;

  struct SortEntry {
    float depth;
    int index;
    int index2;
    int index3;
    SortEntry(int i)
      :index(i)
    {}
    SortEntry(int i, int j, int k)
      :index(i), index2(j), index3(k)
    {}
  };
  std::vector<SortEntry> deptharray;

  std::vector<intptr_t> sortedpartarray;
  std::vector<int32_t> sortedpartcounts;

  std::vector<intptr_t> opaquepartarray;
  std::vector<int32_t> opaquepartcounts;

  std::vector<int> transppartindices;

  std::vector<SbVec3f> partcenters;

  SoFCVertexArrayIndexer * triangleindexer;
  SoFCVertexArrayIndexer * lineindexer;
  SoFCVertexArrayIndexer * pointindexer;

  CoinPtr<SoFCVertexArrayIndexer::IndexArray> prevtriangleindices;
  CoinPtr<SoFCVertexArrayIndexer::IndexArray> prevlineindices;
  CoinPtr<SoFCVertexArrayIndexer::IndexArray> prevpointindices;

  const int32_t *partindices;
  int partcount;

  SbBox3f boundbox;

  void addVertex(const Vertex & v);

  void checkTransparency();

  void render(SoState *state,
              SoFCVertexArrayIndexer *indexer,
              int arrays,
              int part,
              int unit);

  void render(SoState * state,
              SoFCVertexArrayIndexer *indexer,
              const int arrays,
              const intptr_t * offsets = NULL,
              const int32_t * counts = NULL,
              int32_t drawcount = 0);

  void renderImmediate(const cc_glglue * glue,
                       const GLint * indices,
                       const int numindices,
                       const SbBool color, const SbBool normal,
                       const SbBool texture, const SbBool * enabled,
                       const int lastenabled);

  void enableArrays(const cc_glglue * glue,
                    const SbBool color, const SbBool normal,
                    const SbBool texture, const SbBool * enabled,
                    const int lastenabled);

  void disableArrays(const cc_glglue * glue,
                     const SbBool color, const SbBool normal,
                     const SbBool texture, const SbBool * enabled,
                     const int lastenabled);

  void enableVBOs(SoState * state,
                  const cc_glglue * glue,
                  const uint32_t contextid,
                  const SbBool color, const SbBool normal,
                  const SbBool texture, const SbBool * enabled,
                  const int lastenabled);

  void disableVBOs(const cc_glglue * glue,
                   const SbBool color, const SbBool normal,
                   const SbBool texture, const SbBool * enabled,
                   const int lastenabled);

  unsigned long countVBOSize(const cc_glglue * glue,
                             const uint32_t contextid,
                             const SbBool color, const SbBool normal,
                             const SbBool texture, const SbBool * enabled,
                             const int lastenabled);

  void prepare();
};

// *************************************************************************

SoFCVertexCache::SoFCVertexCache(SoState * state, const SoNode * node, SoFCVertexCache * prev)
  : SoCache(state),
    pimpl(new SoFCVertexCacheP(this, prev, node->getNodeId()))
{
  PRIVATE(this)->state = state;
  PRIVATE(this)->diffuseid = 0;
  PRIVATE(this)->transpid = 0;

  const SoField * field = node->getField(*PartIndexField);
  if (field && field->isOfType(SoMFInt32::getClassTypeId())) {
    const SoMFInt32 * partindices = static_cast<const SoMFInt32*>(field);
    PRIVATE(this)->partindices = partindices->getValues(0);
    PRIVATE(this)->partcount = partindices->getNum();
  }
}

SoFCVertexCache::SoFCVertexCache(SoFCVertexCache & prev)
  : SoCache(nullptr),
    pimpl(new SoFCVertexCacheP(this, &prev, PRIVATE(&prev)->nodeid))
{
  PRIVATE(this)->state = nullptr;

  auto pprev = &prev;

  PRIVATE(this)->diffuseid = PRIVATE(pprev)->diffuseid;
  PRIVATE(this)->transpid = PRIVATE(pprev)->transpid;

  PRIVATE(this)->vertexarray = PRIVATE(pprev)->vertexarray;
  PRIVATE(this)->normalarray = PRIVATE(pprev)->normalarray;
  PRIVATE(this)->texcoord0array = PRIVATE(pprev)->texcoord0array;
  PRIVATE(this)->bumpcoordarray = PRIVATE(pprev)->bumpcoordarray;
  PRIVATE(this)->colorarray = PRIVATE(pprev)->colorarray;
  PRIVATE(this)->prevcolorarray.reset();
  PRIVATE(this)->multitexarray = PRIVATE(pprev)->multitexarray;

  PRIVATE(this)->numtranspparts = PRIVATE(pprev)->numtranspparts;
  PRIVATE(this)->hastransp = PRIVATE(pprev)->hastransp;
  PRIVATE(this)->colorpervertex = PRIVATE(pprev)->colorpervertex;

  PRIVATE(this)->transppartindices = PRIVATE(pprev)->transppartindices;
  PRIVATE(this)->partcenters = PRIVATE(pprev)->partcenters;
}

SoFCVertexCache::~SoFCVertexCache()
{
  delete pimpl;
}

void
SoFCVertexCache::open(SoState * state)
{
  assert(!PRIVATE(this)->prevattached);

  SoCacheElement::set(state, this);

  // TODO: It would be ideal if we can get access to the captured elements
  // indside ourself (i.e. SoCache), but it is stored in private class at the
  // moment. Because not all shapes uses SoCoordinateElement/SoNormalElement,
  // for example, primitive shapes like SoCube. In these cases, the correct
  // way is to key on shape's nodeid.

  SbFCUniqueId id;
  SoFCVertexCache *prev = PRIVATE(this)->prevcache;

  auto delem = static_cast<const SoFCDiffuseElement*>(
      state->getConstElement(SoFCDiffuseElement::getClassStackIndex()));
  if (delem->getDiffuseId() || delem->getTransparencyId()) {
    // calling get() below to capture the element in cache
    PRIVATE(this)->diffuseid = SoFCDiffuseElement::get(state, &PRIVATE(this)->transpid);
  }

  const SoCoordinateElement *celem = SoCoordinateElement::getInstance(state);
  id = celem->getNum() ? celem->getNodeId() : (getNodeId() + 0xc9edfc95);
  PRIVATE(this)->vertexarray = new Vec3Array(id, prev ? PRIVATE(prev)->vertexarray : NULL);

  const SoNormalElement *nelem = SoNormalElement::getInstance(state);
  id = nelem->getNum() ? nelem->getNodeId() : (getNodeId() + 0xc3e4eff4d);
  PRIVATE(this)->normalarray =  new Vec3Array(id, prev ? PRIVATE(prev)->normalarray : NULL);

  const SoBumpMapCoordinateElement * belem =
    SoBumpMapCoordinateElement::getInstance(state);

  PRIVATE(this)->numbumpcoords = belem->getNum();
  PRIVATE(this)->bumpcoords = belem->getArrayPtr();
  if (PRIVATE(this)->numbumpcoords) {
    PRIVATE(this)->bumpcoordarray =
      new Vec2Array(belem->getNodeId(), prev ? PRIVATE(prev)->bumpcoordarray : NULL);
  }

  SoLazyElement * lelem = SoLazyElement::getInstance(state);

  PRIVATE(this)->numdiffuse = lelem->getNumDiffuse();
  PRIVATE(this)->numtranspparts = 0;
  PRIVATE(this)->numtransp = lelem->getNumTransparencies();
  if (lelem->isPacked()) {
    PRIVATE(this)->packedptr = lelem->getPackedPointer();
    PRIVATE(this)->diffuseptr = NULL;
    PRIVATE(this)->transpptr = NULL;
  }
  else {
    PRIVATE(this)->packedptr = NULL;
    PRIVATE(this)->diffuseptr = lelem->getDiffusePointer();
    PRIVATE(this)->transpptr = lelem->getTransparencyPointer();
  }

  // set up variables to test if we need to supply color per vertex
  if (PRIVATE(this)->numdiffuse <= 1 && PRIVATE(this)->numtransp <= 1)
    PRIVATE(this)->colorpervertex = 0;
  else {
    PRIVATE(this)->colorpervertex = -1;
    if (prev)
      PRIVATE(this)->prevcolorarray = PRIVATE(prev)->colorarray;
  }

  // just store diffuse color with index 0
  if (PRIVATE(this)->packedptr) {
    PRIVATE(this)->firstcolor = PRIVATE(this)->packedptr[0];
  }
  else {
    SbColor tmpc = PRIVATE(this)->diffuseptr[0];
    float tmpt = PRIVATE(this)->transpptr[0];
    PRIVATE(this)->firstcolor = tmpc.getPackedValue(tmpt);
  }
  PRIVATE(this)->hastransp = (PRIVATE(this)->firstcolor & 0xff)!=0xff;

  // set up for multi texturing
  PRIVATE(this)->lastenabled = -1;
  PRIVATE(this)->enabledunits =
    SoMultiTextureEnabledElement::getEnabledUnits(state, PRIVATE(this)->lastenabled);
  PRIVATE(this)->multielem = NULL;

}

SbFCUniqueId
SoFCVertexCache::getNodeId() const
{
  return PRIVATE(this)->nodeid;
}

SbBool 
SoFCVertexCache::isValid(const SoState * state) const
{
  if (PRIVATE(this)->prevattached && PRIVATE(this)->prevcache)
    return PRIVATE(this)->prevcache->isValid(state);
  return inherited::isValid(state);
}

void 
SoFCVertexCache::close(SoState * state)
{
  (void)state;
  PRIVATE(this)->vhash.clear();
  if (PRIVATE(this)->vertexarray)
    PRIVATE(this)->vertexarray = PRIVATE(this)->vertexarray->attach();
  if (PRIVATE(this)->normalarray) {
    if (!PRIVATE(this)->triangleindexer) {
      const SoNormalElement *nelem = SoNormalElement::getInstance(state);
      if (nelem->getNum() == 0)
        PRIVATE(this)->normalarray.reset();
    }
    if (PRIVATE(this)->normalarray)
      PRIVATE(this)->normalarray = PRIVATE(this)->normalarray->attach();
  }
  if (PRIVATE(this)->texcoord0array)
    PRIVATE(this)->texcoord0array = PRIVATE(this)->texcoord0array->attach();
  if (PRIVATE(this)->bumpcoordarray)
    PRIVATE(this)->bumpcoordarray = PRIVATE(this)->bumpcoordarray->attach();
  if (PRIVATE(this)->colorarray)
    PRIVATE(this)->colorarray = PRIVATE(this)->colorarray->attach();
  for (auto & entry : PRIVATE(this)->multitexarray) {
    if (entry)
      entry = entry->attach();
  }
  if (PRIVATE(this)->triangleindexer)
    PRIVATE(this)->triangleindexer->close(PRIVATE(this)->partindices, PRIVATE(this)->partcount);
  if (PRIVATE(this)->lineindexer)
    PRIVATE(this)->lineindexer->close();
  if (PRIVATE(this)->pointindexer)
    PRIVATE(this)->pointindexer->close();

  PRIVATE(this)->checkTransparency();
}

void
SoFCVertexCacheP::checkTransparency()
{
  if (!this->triangleindexer) return;
  int numparts = this->triangleindexer->getNumParts();
  if (!numparts) return;

  const int *parts = this->triangleindexer->getPartOffsets();
  const GLint *indices = this->triangleindexer->getIndices();
  this->partcenters.reserve(numparts);
  const SbVec3f *vertices = this->vertexarray->getArrayPtr();
  int prev = 0;
  std::vector<int> transpparts;
  for (int i=0; i<numparts; ++i) {
    SbVec3d v(0,0,0);
    int n = parts[i]-prev;
    for (int k=0; k<n; ++k) {
      const SbVec3f & vertex = vertices[indices[k+prev]];
      v[0] += vertex[0];
      v[1] += vertex[1];
      v[2] += vertex[2];
    }
    this->partcenters.emplace_back((float)(v[0]/n),
                                   (float)(v[1]/n),
                                   (float)(v[2]/n));
    if (this->hastransp && this->colorpervertex> 0) {
      bool transp = false;
      for (int k=0; k<n; ++k) {
        if ((*this->colorarray)[indices[k+prev]*4 + 3] != 0xff) {
          transp = true;
          break;
        }
      }
      if (transp)
        transpparts.push_back(i);
    }
    prev = parts[i];
  }
  
  if (this->colorpervertex <= 0)
    this->numtranspparts = numparts;
  else
    this->numtranspparts = (int)transpparts.size();

  if (this->numtranspparts != numparts) {
    this->transppartindices = std::move(transpparts);
    this->opaquepartarray.reserve(numparts - this->numtranspparts);
    this->opaquepartcounts.reserve(numparts - this->numtranspparts);
    int typesize = this->triangleindexer->useShorts() ? 2 : 4;
    int prev = 0;
    for (int i : this->transppartindices) {
      if (i != prev) {
        this->opaquepartarray.push_back((prev ? parts[prev-1] : 0) * typesize);
        this->opaquepartcounts.push_back(parts[i-1] - (prev ? parts[prev-1] : 0));
      }
      prev = i+1;
    }
    if (prev < numparts) {
      this->opaquepartarray.push_back((prev ? parts[prev-1] : 0) * typesize);
      this->opaquepartcounts.push_back(parts[numparts-1] - parts[prev-1]);
    }
  }
}

void
SoFCVertexCacheP::render(SoState *state,
                         SoFCVertexArrayIndexer *indexer,
                         int arrays,
                         int part,
                         int unit)
{
  if (!indexer)
    return;

  intptr_t offset;
  int32_t count;

  if (!indexer->getNumParts()) {
    count = unit;
    if ((part+1) * unit > indexer->getNumIndices())
      return;
    offset = part * unit * (indexer->useShorts() ? 2 : 4);
  }
  else if (part >= indexer->getNumParts())
    return;
  else {
    const int * parts = indexer->getPartOffsets();
    if (part == 0) {
      count = parts[0];
      offset = 0;
    }
    else {
      count = parts[part] - parts[part-1];
      offset = parts[part-1];
      offset *= indexer->useShorts() ? 2 : 4;
    }
  }
  render(state, indexer, arrays, &offset, &count, 1);
}

void
SoFCVertexCacheP::render(SoState * state,
                         SoFCVertexArrayIndexer *indexer,
                         const int arrays,
                         const intptr_t * offsets,
                         const int32_t * counts,
                         int32_t drawcount)
{
  if (!indexer || !indexer->getNumIndices()) return;
  if (!this->vertexarray || !this->vertexarray->getLength()) return;
  int lastenabled = -1;

  const SbBool * enabled = NULL;
  const SbBool normal = (arrays & NORMAL) != 0;
  const SbBool texture = (arrays & TEXCOORD) != 0;
  const SbBool color = PUBLIC(this)->colorPerVertex() && ((arrays & COLOR) != 0);
  if (texture) {
    enabled = SoMultiTextureEnabledElement::getEnabledUnits(state, lastenabled);
    if (lastenabled > this->lastenabled)
      lastenabled = this->lastenabled;
  }

  const uint32_t contextid = SoGLCacheContextElement::get(state);
  const cc_glglue * glue = cc_glglue_instance(static_cast<int>(contextid));

  int vnum = this->vertexarray->getLength();
  if (SoFCVBO::shouldCreateVBO(state, contextid, vnum)) {
    this->enableVBOs(state, glue, contextid, color, normal, texture, enabled, lastenabled);
    indexer->render(state, glue, TRUE, contextid, offsets, counts, drawcount);
    this->disableVBOs(glue, color, normal, texture, enabled, lastenabled);
  } else
  if (SoFCVBO::shouldRenderAsVertexArrays(state, contextid, vnum)) {
    this->enableArrays(glue, color, normal, texture, enabled, lastenabled);
    if (!drawcount)
      indexer->render(state, glue, FALSE, contextid);
    else {
      int typeshift = this->triangleindexer->useShorts() ? 1 : 2;
      for (int i=0; i<drawcount; ++i) {
        int32_t count = counts[i];
        intptr_t offset = offsets[i] >> typeshift;
        offset = (intptr_t)(indexer->getIndices() + offset);
        indexer->render(state, glue, FALSE, contextid, &offset, &count, 1);
      }
    }
    this->disableArrays(glue, color, normal, texture, enabled, lastenabled);
  }
  else {
    // fall back to immediate mode rendering
    glBegin(indexer->getTarget());
    if (!drawcount) {
      this->renderImmediate(glue,
                            indexer->getIndices(),
                            indexer->getNumIndices(),
                            color, normal, texture, enabled, lastenabled);
    }
    else {
      int typeshift = this->triangleindexer->useShorts() ? 1 : 2;
      for (int i=0; i<drawcount; ++i) {
        int count = counts[i];
        intptr_t offset = offsets[i] >> typeshift;
        this->renderImmediate(glue,
                              indexer->getIndices() + offset, count,
                              color, normal, texture, enabled, lastenabled);
      }
    }
    glEnd();
  }
}

SbBool
SoFCVertexCache::hasOpaqueParts() const
{
  if (!PRIVATE(this)->triangleindexer) return FALSE;
  if (!PRIVATE(this)->numtranspparts) return !PRIVATE(this)->hastransp;
  return PRIVATE(this)->numtranspparts < PRIVATE(this)->triangleindexer->getNumParts();
}

SbBool
SoFCVertexCache::hasTransparency() const
{
  return PRIVATE(this)->hastransp;
}

void
SoFCVertexCache::renderTriangles(SoState * state, const int arrays, int part)
{
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->triangleindexer, arrays, part, 3);
    return;
  }

  int drawcount = 0;
  const intptr_t * offsets = NULL;
  const int32_t * counts = NULL;

  if (arrays & (SORTED_ARRAY | FULL_SORTED_ARRAY)) {
    if (PRIVATE(this)->depthSortTriangles(state, (arrays & FULL_SORTED_ARRAY) ? true : false)) {
      offsets = &PRIVATE(this)->sortedpartarray[0];
      counts = &PRIVATE(this)->sortedpartcounts[0];
      drawcount = (int)PRIVATE(this)->sortedpartarray.size();
    }
  }
  else if (!(arrays & NON_SORTED_ARRAY) && PRIVATE(this)->opaquepartarray.size()) {
    offsets = &PRIVATE(this)->opaquepartarray[0];
    counts = &PRIVATE(this)->opaquepartcounts[0];
    drawcount = (int)PRIVATE(this)->opaquepartarray.size();
  }

  PRIVATE(this)->render(state, PRIVATE(this)->triangleindexer, arrays, offsets, counts, drawcount);
}

void
SoFCVertexCache::renderLines(SoState * state, const int arrays, int part)
{
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->lineindexer, arrays, part, 2);
    return;
  }
  PRIVATE(this)->render(state, PRIVATE(this)->lineindexer, arrays);
}

void
SoFCVertexCache::renderPoints(SoState * state, const int arrays, int part)
{
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->pointindexer, arrays, part, 1);
    return;
  }
  PRIVATE(this)->render(state, PRIVATE(this)->pointindexer, arrays);
}

class MyMultiTextureCoordinateElement : public SoMultiTextureCoordinateElement
{
public:
  SbFCUniqueId getNodeId(int unit) const { return getUnitData(unit).nodeid; }
};

inline void
SoFCVertexCacheP::prepare()
{
  assert(!this->prevattached);

  if (!this->multielem && this->lastenabled >= 0) {
    this->multielem = SoMultiTextureCoordinateElement::getInstance(this->state);
    SbFCUniqueId id = static_cast<const MyMultiTextureCoordinateElement*>(this->multielem)->getNodeId(0);
    this->texcoord0array =
      new Vec4Array(id, this->prevcache ? PRIVATE(this->prevcache)->texcoord0array : NULL);

    if (this->lastenabled > 0) {
      this->multitexarray.resize(this->lastenabled+1);
      for (int i = 1; i < this->lastenabled+1; ++i) {
        id = static_cast<const MyMultiTextureCoordinateElement*>(this->multielem)->getNodeId(i);
        Vec4Array *prev = NULL;
        if (this->prevcache && PRIVATE(this->prevcache)->lastenabled >= i)
          prev = PRIVATE(this->prevcache)->multitexarray[i];
        this->multitexarray[i] = new Vec4Array(id, prev);
      }
    }
  }

  this->prevcache.reset();
}

void
SoFCVertexCache::addTriangles(const std::set<int> & faces)
{
  assert(PRIVATE(this)->prevattached);
  assert(!PRIVATE(this)->triangleindexer && PRIVATE(this)->prevcache);

  auto prevcache = PRIVATE(this)->prevcache;
  assert(PRIVATE(prevcache)->triangleindexer);

  auto indexer = PRIVATE(prevcache)->triangleindexer;
  PRIVATE(this)->triangleindexer = new SoFCVertexArrayIndexer(*indexer, faces, getNumVertices());
  PRIVATE(this)->prevtriangleindices.reset();

  if (!indexer->getNumParts() || !PRIVATE(this)->numtranspparts)
    return;

  if (PRIVATE(this)->numtranspparts == indexer->getNumParts()) {
    PRIVATE(this)->numtranspparts = static_cast<int>(faces.size());
    return;
  }

  std::vector<int> transppartindices;
  transppartindices.reserve(
      std::max(PRIVATE(this)->numtranspparts, static_cast<int>(faces.size())));
  for (int i : PRIVATE(prevcache)->transppartindices) {
    if (faces.count(i))
      transppartindices.push_back(i);
  }
  PRIVATE(this)->numtranspparts = static_cast<int>(transppartindices.size());
  if (transppartindices.size() && faces.size() > transppartindices.size()) {

    PRIVATE(this)->transppartindices = std::move(transppartindices);

    int typesize = PRIVATE(this)->triangleindexer->useShorts() ? 2 : 4;
    const int * parts = PRIVATE(this)->triangleindexer->getPartOffsets();
    int j = 0;
    for (int i : PRIVATE(prevcache)->transppartindices) {
      for (;j<i; ++j) {
        if (!faces.count(j))
          continue;
        int prev = j == 0 ? 0 : parts[j-1];
        PRIVATE(this)->opaquepartarray.push_back(prev * typesize);
        PRIVATE(this)->opaquepartcounts.push_back(parts[j] - prev);
      }
    }
  }
}

void
SoFCVertexCache::addTriangle(const SoPrimitiveVertex * v0,
                             const SoPrimitiveVertex * v1,
                             const SoPrimitiveVertex * v2,
                             const int * pointdetailidx)
{
  PRIVATE(this)->prepare();

  const SoPrimitiveVertex *vp[3] = { v0, v1, v2 };

  int32_t triangleindices[3];

  const SoFaceDetail *fd = nullptr;

  for (int i = 0; i < 3; i++) {
    SoFCVertexCacheP::Vertex v;
    v.vertex = vp[i]->getPoint();
    v.normal = vp[i]->getNormal();
    const SbVec4f & tmp = vp[i]->getTextureCoords();
    v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
    v.texcoord0 = tmp;
    v.texcoordidx = -1;

    if (PRIVATE(this)->colorpervertex == 0) {
      v.color = PRIVATE(this)->firstcolor;
    }
    else {
      int midx = vp[i]->getMaterialIndex();
      if (PRIVATE(this)->packedptr) {
        v.color = PRIVATE(this)->packedptr[SbClamp(midx, 0, PRIVATE(this)->numdiffuse-1)];
      }
      else {
        SbColor tmpc = PRIVATE(this)->diffuseptr[SbClamp(midx,0,PRIVATE(this)->numdiffuse-1)];
        float tmpt = PRIVATE(this)->transpptr[SbClamp(midx,0,PRIVATE(this)->numtransp-1)];
        v.color = tmpc.getPackedValue(tmpt);
      }
      if (v.color != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = 1;
      PRIVATE(this)->hastransp = (PRIVATE(this)->hastransp || (v.color&0xff) != 0xff);
    }

    const SoDetail * d = vp[i]->getDetail();

    if (d && d->isOfType(SoFaceDetail::getClassTypeId()) && pointdetailidx) {
      fd = static_cast<const SoFaceDetail *>(d);
      assert(pointdetailidx[i] < fd->getNumPoints());
      const SoPointDetail * pd = static_cast<const SoPointDetail *>(
        fd->getPoint(pointdetailidx[i])
       );

      int tidx  = v.texcoordidx = pd->getTextureCoordIndex();
      if (PRIVATE(this)->numbumpcoords) {
        v.bumpcoord = PRIVATE(this)->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->numbumpcoords-1)];
      }
    }

    auto res = PRIVATE(this)->vhash.insert(
        std::make_pair(v, PRIVATE(this)->vertexarray->getLength()));
    if (res.second) {
      PRIVATE(this)->addVertex(v);
      // update texture coordinates for unit 1-n
      for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
        if (v.texcoordidx >= 0 &&
            (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
          PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->multielem->get4(j, v.texcoordidx));
        }
        else if (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
          PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->multielem->get(j, v.vertex, v.normal));
        }
        else {
          PRIVATE(this)->multitexarray[j]->append(v.texcoord0);
        }
      }
    }
    triangleindices[i] = res.first->second;
  }

  if (!PRIVATE(this)->triangleindexer) {
    PRIVATE(this)->triangleindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->nodeid, PRIVATE(this)->prevtriangleindices);
    PRIVATE(this)->prevtriangleindices = NULL;
  }
  PRIVATE(this)->triangleindexer->addTriangle(triangleindices[0],
                                              triangleindices[1],
                                              triangleindices[2]);
}

void
SoFCVertexCache::addLines(const std::set<int> & lineindices)
{
  assert(PRIVATE(this)->prevattached);
  assert(!PRIVATE(this)->lineindexer && PRIVATE(this)->prevcache);

  auto prevcache = PRIVATE(this)->prevcache;
  assert(PRIVATE(prevcache)->lineindexer);

  auto indexer = PRIVATE(prevcache)->lineindexer;
  PRIVATE(this)->lineindexer = new SoFCVertexArrayIndexer(* indexer, lineindices, getNumVertices());
  PRIVATE(this)->prevlineindices.reset();
}

void
SoFCVertexCache::addLine(const SoPrimitiveVertex * v0,
                         const SoPrimitiveVertex * v1)
{
  PRIVATE(this)->prepare();

  const SoPrimitiveVertex *vp[2] = { v0,v1 };

  int32_t lineindices[2];

  const SoLineDetail * ld = nullptr;

  for (int i = 0; i < 2; i++) {
    SoFCVertexCacheP::Vertex v;
    v.vertex = vp[i]->getPoint();
    v.normal = vp[i]->getNormal();
    const SbVec4f & tmp = vp[i]->getTextureCoords();
    v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
    v.texcoord0 = tmp;
    v.texcoordidx = -1;

    if (PRIVATE(this)->colorpervertex == 0) {
      v.color = PRIVATE(this)->firstcolor;
    }
    else {
      int midx = vp[i]->getMaterialIndex();
      if (PRIVATE(this)->packedptr) {
        v.color = PRIVATE(this)->packedptr[SbClamp(midx, 0, PRIVATE(this)->numdiffuse-1)];
      }
      else {
        SbColor tmpc = PRIVATE(this)->diffuseptr[SbClamp(midx,0,PRIVATE(this)->numdiffuse-1)];
        float tmpt = PRIVATE(this)->transpptr[SbClamp(midx,0,PRIVATE(this)->numtransp-1)];
        v.color = tmpc.getPackedValue(tmpt);
      }
      if (v.color != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = 1;
      PRIVATE(this)->hastransp = (PRIVATE(this)->hastransp || (v.color&0xff) != 0xff);
    }

    const SoDetail * d = vp[i]->getDetail();

    if (d && d->isOfType(SoLineDetail::getClassTypeId())) {
      ld = static_cast<const SoLineDetail *>(d);
      const SoPointDetail * pd;
      if (i == 0) pd = ld->getPoint0();
      else pd = ld->getPoint1();

      int tidx  = v.texcoordidx = static_cast<const SoPointDetail *>(pd)->getTextureCoordIndex();
      if (PRIVATE(this)->numbumpcoords) {
        v.bumpcoord = PRIVATE(this)->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->numbumpcoords-1)];
      }
    }

    auto res = PRIVATE(this)->vhash.insert(
        std::make_pair(v, PRIVATE(this)->vertexarray->getLength()));
    if (res.second) {
      PRIVATE(this)->addVertex(v);
      // update texture coordinates for unit 1-n
      for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
        if (v.texcoordidx >= 0 &&
            (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
          PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->multielem->get4(j, v.texcoordidx));
        }
        else if (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
          PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->multielem->get(j, v.vertex, v.normal));
        }
        else {
          PRIVATE(this)->multitexarray[j]->append(v.texcoord0);
        }
      }
    }
    lineindices[i] = res.first->second;
  }

  if (!PRIVATE(this)->lineindexer) {
    PRIVATE(this)->lineindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->nodeid, PRIVATE(this)->prevlineindices);
    PRIVATE(this)->prevlineindices = NULL;
  }
  PRIVATE(this)->lineindexer->addLine(lineindices[0], lineindices[1], ld ? ld->getLineIndex() : -1);
}

void
SoFCVertexCache::addPoints(const std::set<int> & pointindices)
{
  assert(PRIVATE(this)->prevattached);
  assert(!PRIVATE(this)->pointindexer && PRIVATE(this)->prevcache);

  auto prevcache = PRIVATE(this)->prevcache;
  assert(PRIVATE(prevcache)->pointindexer);

  auto indexer = PRIVATE(prevcache)->pointindexer;
  PRIVATE(this)->pointindexer = new SoFCVertexArrayIndexer(*indexer, pointindices, getNumVertices());
  PRIVATE(this)->prevpointindices.reset();
}

void
SoFCVertexCache::addPoint(const SoPrimitiveVertex * v0)
{
  PRIVATE(this)->prepare();

  SoFCVertexCacheP::Vertex v;
  v.vertex = v0->getPoint();
  v.normal = v0->getNormal();
  const SbVec4f & tmp = v0->getTextureCoords();
  v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
  v.texcoord0 = tmp;
  v.texcoordidx = -1;

  if (PRIVATE(this)->colorpervertex == 0) {
    v.color = PRIVATE(this)->firstcolor;
  }
  else {
    int midx = v0->getMaterialIndex();
    if (PRIVATE(this)->packedptr) {
      v.color = PRIVATE(this)->packedptr[SbClamp(midx, 0, PRIVATE(this)->numdiffuse-1)];
    }
    else {
      SbColor tmpc = PRIVATE(this)->diffuseptr[SbClamp(midx,0,PRIVATE(this)->numdiffuse-1)];
      float tmpt = PRIVATE(this)->transpptr[SbClamp(midx,0,PRIVATE(this)->numtransp-1)];
      v.color = tmpc.getPackedValue(tmpt);
    }
    if (v.color != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = 1;
    PRIVATE(this)->hastransp = (PRIVATE(this)->hastransp || (v.color&0xff) != 0xff);
  }

  const SoDetail * d = v0->getDetail();

  if (d && d->isOfType(SoPointDetail::getClassTypeId())) {
    const SoPointDetail * pd = static_cast<const SoPointDetail *>(d);
    int tidx  = v.texcoordidx = pd->getTextureCoordIndex();
    if (PRIVATE(this)->numbumpcoords) {
      v.bumpcoord = PRIVATE(this)->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->numbumpcoords-1)];
    }
  }

  if (!PRIVATE(this)->pointindexer) {
    PRIVATE(this)->pointindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->nodeid, PRIVATE(this)->prevpointindices);
    PRIVATE(this)->prevpointindices = NULL;
  }

  auto res = PRIVATE(this)->vhash.insert(
      std::make_pair(v, PRIVATE(this)->vertexarray->getLength()));
  if (res.second) {
    PRIVATE(this)->addVertex(v);
    // update texture coordinates for unit 1-n
    for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
      if (v.texcoordidx >= 0 &&
          (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
        PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->multielem->get4(j, v.texcoordidx));
      }
      else if (PRIVATE(this)->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
        PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->multielem->get(j, v.vertex, v.normal));
      }
      else {
        PRIVATE(this)->multitexarray[j]->append(v.texcoord0);
      }
    }
  }
  PRIVATE(this)->pointindexer->addPoint(res.first->second);
}

int
SoFCVertexCache::getNumVertices(void) const
{
  return PRIVATE(this)->vertexarray ? PRIVATE(this)->vertexarray->getLength() : 0;
}

const SbVec3f *
SoFCVertexCache::getVertexArray(void) const
{
  return PRIVATE(this)->vertexarray ? PRIVATE(this)->vertexarray->getArrayPtr() : NULL;
}

const SbVec3f *
SoFCVertexCache::getNormalArray(void) const
{
  return PRIVATE(this)->normalarray ? PRIVATE(this)->normalarray->getArrayPtr() : NULL;
}

const SbVec4f *
SoFCVertexCache::getTexCoordArray(void) const
{
  return PRIVATE(this)->texcoord0array ? PRIVATE(this)->texcoord0array->getArrayPtr() : NULL;
}

const SbVec2f *
SoFCVertexCache::getBumpCoordArray(void) const
{
  return PRIVATE(this)->bumpcoordarray ? PRIVATE(this)->bumpcoordarray->getArrayPtr() : NULL;
}

const uint8_t *
SoFCVertexCache::getColorArray(void) const
{
  return PRIVATE(this)->colorarray ? PRIVATE(this)->colorarray->getArrayPtr() : NULL;
}

int
SoFCVertexCache::getNumTriangleIndices(void) const
{
  return PRIVATE(this)->triangleindexer ? PRIVATE(this)->triangleindexer->getNumIndices() : 0;
}

const GLint *
SoFCVertexCache::getTriangleIndices(void) const
{
  assert(PRIVATE(this)->triangleindexer);
  return PRIVATE(this)->triangleindexer->getIndices();
}

int32_t
SoFCVertexCache::getTriangleIndex(const int idx) const
{
  assert(PRIVATE(this)->triangleindexer);
  return PRIVATE(this)->triangleindexer->getIndices()[idx];
}

SbBool
SoFCVertexCache::colorPerVertex(void) const
{
  return PRIVATE(this)->colorpervertex > 0;
}

const SbVec4f *
SoFCVertexCache::getMultiTextureCoordinateArray(const int unit) const
{
  assert(unit <= PRIVATE(this)->lastenabled);
  if (!unit)
    return PRIVATE(this)->texcoord0array ? PRIVATE(this)->texcoord0array->getArrayPtr() : NULL;

  if (unit < (int)PRIVATE(this)->multitexarray.size()
      && PRIVATE(this)->multitexarray[unit]
      && PRIVATE(this)->multitexarray[unit]->getArrayPtr())
    return PRIVATE(this)->multitexarray[unit]->getArrayPtr();

  return NULL;
}

int
SoFCVertexCache::getNumLineIndices(void) const
{
  return PRIVATE(this)->lineindexer ? PRIVATE(this)->lineindexer->getNumIndices() : 0;
}

int
SoFCVertexCache::getNumPointIndices(void) const
{
  return PRIVATE(this)->pointindexer ? PRIVATE(this)->pointindexer->getNumIndices() : 0;
}


const GLint *
SoFCVertexCache::getLineIndices(void) const
{
  assert(PRIVATE(this)->lineindexer);
  return PRIVATE(this)->lineindexer->getIndices();
}

const GLint *
SoFCVertexCache::getPointIndices(void) const
{
  assert(PRIVATE(this)->pointindexer);
  return PRIVATE(this)->pointindexer->getIndices();
}

SbBool
SoFCVertexCacheP::depthSortTriangles(SoState * state, bool fullsort)
{
  if (!this->vertexarray) return FALSE;
  int numv = this->vertexarray->getLength();
  int numtri = PUBLIC(this)->getNumTriangleIndices() / 3;
  if (numv == 0 || numtri == 0) return FALSE;

  int numparts = this->triangleindexer->getNumParts();

  // must not mess up indices if there are parts
  if (numparts) {
    if (numparts == 1)
      return FALSE;
    if (!fullsort) {
      if (this->numtranspparts == numparts)
        fullsort = true;
      else {
        numparts = this->numtranspparts;
        if (!numparts)
          return FALSE;
      }
    }
  }

  SbPlane sortplane = SoViewVolumeElement::get(state).getPlane(0.0);
  // move plane into object space
  sortplane.transform(SoModelMatrixElement::get(state).inverse());

  const SbVec3f * vptr = this->vertexarray->getArrayPtr();

  // If having parts, sort the parts (i.e. group of triangles) instead of
  // individual triangles
  if (numparts) {
    if (numparts == (int)this->deptharray.size() && sortplane == this->prevsortplane)
      return TRUE;

    this->deptharray.clear();
    this->deptharray.reserve(numparts);
    if (fullsort) {
      if (this->triangleindexer->getPartialIndices().size()) {
        for (int i : this->triangleindexer->getPartialIndices())
          this->deptharray.emplace_back(i);
      }
      else {
        for (int i=0; i<numparts; ++i)
          this->deptharray.emplace_back(i);
      }
    }
    else {
      for (int i : this->transppartindices)
        this->deptharray.emplace_back(i);
    }

    this->prevsortplane = sortplane;
    const int *parts = this->triangleindexer->getPartOffsets();
    for(auto & entry : this->deptharray)
      entry.depth = sortplane.getDistance(this->partcenters[entry.index]);

    std::sort(this->deptharray.begin(), this->deptharray.end(),
        [] (const SortEntry &a, const SortEntry &b) {
          return a.depth < b.depth;
        });

    this->sortedpartarray.resize(this->deptharray.size());
    this->sortedpartcounts.resize(this->deptharray.size());
    int i=0;
    int typesize = this->triangleindexer->useShorts() ? 2 : 4;
    for (auto & entry : this->deptharray) {
      int start = entry.index == 0 ? 0 : parts[entry.index-1];
      int end = parts[entry.index];
      this->sortedpartarray[i] = start * typesize;
      this->sortedpartcounts[i++] = end - start;
    }
    return TRUE;
  }

  // normal sorting without parts
  if (numtri == (int)deptharray.size() && sortplane == this->prevsortplane)
    return FALSE;

  GLint * iptr = this->triangleindexer->getWriteableIndices();

  this->deptharray.clear();
  this->deptharray.reserve(numtri);
  for (int i=0; i<numtri; ++i) {
    this->deptharray.emplace_back(iptr[i*3], iptr[i*3+1], iptr[i*3+2]);
    double acc = 0.0;
    acc += sortplane.getDistance(vptr[iptr[i*3]]);
    acc += sortplane.getDistance(vptr[iptr[i*3+1]]);
    acc += sortplane.getDistance(vptr[iptr[i*3+2]]);
    this->deptharray.back().depth = (float) (acc / 3.0);
  }

  std::sort(this->deptharray.begin(), this->deptharray.end(),
      [] (const SortEntry &a, const SortEntry &b) {
        return a.depth < b.depth;
      });

  int i = 0;
  for(auto & entry : this->deptharray) {
    iptr[i++] = entry.index;
    iptr[i++] = entry.index2;
    iptr[i++] = entry.index3;
  }
  return FALSE;
}

void
SoFCVertexCacheP::addVertex(const Vertex & v)
{
  this->vertexarray->append(v.vertex);
  this->normalarray->append(v.normal);
  if (this->texcoord0array) this->texcoord0array->append(v.texcoord0);
  if (this->bumpcoordarray) this->bumpcoordarray->append(v.bumpcoord);

  if (!this->colorarray && this->colorpervertex > 0) {
    SbFCUniqueId id = this->diffuseid + this->transpid + 0xb68cfe55;
    this->colorarray = new ByteArray(id, this->prevcolorarray);
    this->prevcolorarray.reset();

    uint8_t r,g,b,a;
    r = (this->firstcolor >> 24) & 0xff;
    g = (this->firstcolor >> 16) & 0xff;
    b = (this->firstcolor >> 8) & 0xff;
    a = (this->firstcolor) & 0xff;
    for (int i=0, n=this->vertexarray->getLength()*4-4; i<n; i+=4) {
      this->colorarray->append(r);
      this->colorarray->append(g);
      this->colorarray->append(b);
      this->colorarray->append(a);
    }
  }

  if (this->colorarray) {
    uint8_t r,g,b,a;
    r = (v.color >> 24) & 0xff;
    g = (v.color >> 16) & 0xff;
    b = (v.color >> 8) & 0xff;
    a = (v.color) & 0xff;
    this->colorarray->append(r);
    this->colorarray->append(g);
    this->colorarray->append(b);
    this->colorarray->append(a);
  }
}

void
SoFCVertexCacheP::enableArrays(const cc_glglue * glue,
                               const SbBool color, const SbBool normal,
                               const SbBool texture, const SbBool * enabled,
                               const int lastenabled)
{
  int i;
  if (color && this->colorarray) {
    cc_glglue_glColorPointer(glue, 4, GL_UNSIGNED_BYTE, 0,
                             this->colorarray->getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
  }

  if (texture && this->texcoord0array) {
    cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0,
                                this->texcoord0array->getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

    for (i = 1; i <= lastenabled; i++) {
      if (enabled[i] && this->multitexarray[i]) {
        cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
        cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0,
                                    this->multitexarray[i]->getArrayPtr());
        cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
      }
    }
  }
  if (normal && this->normalarray) {
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0,
                              this->normalarray->getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }

  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0,
                            this->vertexarray->getArrayPtr());
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);
}


void
SoFCVertexCacheP::disableArrays(const cc_glglue * glue,
                                const SbBool color, const SbBool normal,
                                const SbBool texture, const SbBool * enabled,
                                const int lastenabled)
{
  int i;
  if (normal && this->normalarray) {
    cc_glglue_glDisableClientState(glue, GL_NORMAL_ARRAY);
  }
  if (texture && this->texcoord0array) {
    for (i = 1; i <= lastenabled; i++) {
      if (enabled[i] && this->multitexarray[i]) {
        cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
        cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
      }
    }
    if (lastenabled >= 1) {
      // reset to default
      cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0);
    }
    cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
  }
  if (color && this->colorarray) {
    cc_glglue_glDisableClientState(glue, GL_COLOR_ARRAY);
  }
  cc_glglue_glDisableClientState(glue, GL_VERTEX_ARRAY);
}

void
SoFCVertexCacheP::enableVBOs(SoState *state,
                             const cc_glglue * glue,
                             uint32_t contextid,
                             const SbBool color, const SbBool normal,
                             const SbBool texture, const SbBool * enabled,
                             const int lastenabled)
{
  if (!this->vertexarray) return;

#if 0
  if (!SoGLDriverDatabase::isSupported(glue, SO_GL_VBO_IN_DISPLAYLIST)) {
    SoCacheElement::invalidate(state);
    SoGLCacheContextElement::shouldAutoCache(state,
                                             SoGLCacheContextElement::DONT_AUTO_CACHE);
    state = NULL;
  }
#endif

  int i;
  if (color && this->colorarray) {
    this->colorarray->bindBuffer(state, contextid);
    cc_glglue_glColorPointer(glue, 4, GL_UNSIGNED_BYTE, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
  }
  if (texture && this->texcoord0array) {
    this->texcoord0array->bindBuffer(state, contextid);
    cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

    for (i = 1; i <= lastenabled; i++) {
      if (!enabled[i] || !this->multitexarray[i]) continue;
      this->multitexarray[i]->bindBuffer(state, contextid);
      cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
      cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0, NULL);
      cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
    }
  }
  if (normal && this->normalarray) {
    this->normalarray->bindBuffer(state, contextid);
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }

  this->vertexarray->bindBuffer(state, contextid);
  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0, NULL);
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);
}

void
SoFCVertexCacheP::disableVBOs(const cc_glglue * glue,
                              const SbBool color, const SbBool normal,
                              const SbBool texture, const SbBool * enabled,
                              const int lastenabled)
{
  this->disableArrays(glue, color, normal, texture, enabled, lastenabled);
  cc_glglue_glBindBuffer(glue, GL_ARRAY_BUFFER, 0); // Reset VBO binding
}

void
SoFCVertexCacheP::renderImmediate(const cc_glglue * glue,
                                  const GLint * indices,
                                  const int numindices,
                                  const SbBool color, const SbBool normal,
                                  const SbBool texture, const SbBool * enabled,
                                  const int lastenabled)
{
  if (!this->vertexarray) return;

  const unsigned char * colorptr = NULL;
  const SbVec3f * normalptr = NULL;
  const SbVec3f * vertexptr = NULL;
  const SbVec4f * texcoordptr = NULL;
  
  if (color && this->colorarray) {
    colorptr = this->colorarray->getArrayPtr();
  }
  if (normal && this->normalarray) {
    normalptr = this->normalarray->getArrayPtr();
  }
  if (texture && this->texcoord0array) {
    texcoordptr = this->texcoord0array->getArrayPtr();
  }
  vertexptr = this->vertexarray->getArrayPtr();

  for (int i = 0; i < numindices; i++) {
    const int idx = indices[i];
    if (normalptr) {
      glNormal3fv(reinterpret_cast<const GLfloat *>(&normalptr[idx]));
    }
    if (colorptr) {
      glColor3ubv(reinterpret_cast<const GLubyte *>(&colorptr[idx*4]));
    }
    if (texcoordptr) {
      glTexCoord4fv(reinterpret_cast<const GLfloat *>(&texcoordptr[idx]));

      for (int j = 1; j <= lastenabled; j++) {
        if (!enabled[j] || !this->multitexarray[j]) continue;
        const SbVec4f * mt = this->multitexarray[j]->getArrayPtr();
        cc_glglue_glMultiTexCoord4fv(glue,
                                    GL_TEXTURE0 + j,
                                    reinterpret_cast<const GLfloat *>(&mt[idx]));
      }
    }
    glVertex3fv(reinterpret_cast<const GLfloat *>(&vertexptr[idx]));
  }
}

void
SoFCVertexCache::initClass()
{
  SoFCVertexArrayIndexer::initClass();
  SoFCVertexCacheP::initClass();
  SoFCVBO::init();
}

void
SoFCVertexCache::cleanup()
{
  SoFCVertexArrayIndexer::cleanup();
  SoFCVertexCacheP::cleanup();
}

SbVec3f
SoFCVertexCache::getCenter() const
{
  return getBoundingBox().getCenter();
}

const SbBox3f &
SoFCVertexCache::getBoundingBox() const
{
  if (PRIVATE(this)->boundbox.isEmpty())
    getBoundingBox(nullptr, PRIVATE(this)->boundbox);
  return PRIVATE(this)->boundbox;
}

void
SoFCVertexCache::getBoundingBox(const SbMatrix * matrix, SbBox3f & bbox) const
{
  const SbVec3f *vptr = getVertexArray();
  if (PRIVATE(this)->prevattached) {
    // means partial indexing, we need to explicitly iterate over indices
    if (PRIVATE(this)->triangleindexer)
      PRIVATE(this)->triangleindexer->getBoundingBox(matrix, bbox, vptr);
    if (PRIVATE(this)->lineindexer)
      PRIVATE(this)->lineindexer->getBoundingBox(matrix, bbox, vptr);
    if (PRIVATE(this)->pointindexer)
      PRIVATE(this)->pointindexer->getBoundingBox(matrix, bbox, vptr);
    return;
  }

  int num = getNumVertices();
  if (matrix) {
    for (int i=0; i<num; ++i) {
      SbVec3f v;
      matrix->multVecMatrix(vptr[i], v);
      bbox.extendBy(v);
    }
  }
  else {
    for (int i=0; i<num; ++i)
      bbox.extendBy(vptr[i]);
  }
}

uint32_t
SoFCVertexCache::getFaceColor(int part) const
{
  return PRIVATE(this)->getColor(PRIVATE(this)->triangleindexer, part);
}

uint32_t
SoFCVertexCache::getLineColor(int part) const
{
  return PRIVATE(this)->getColor(PRIVATE(this)->lineindexer, part);
}

uint32_t
SoFCVertexCache::getPointColor(int part) const
{
  return PRIVATE(this)->getColor(PRIVATE(this)->pointindexer, part);
}

uint32_t
SoFCVertexCacheP::getColor(const SoFCVertexArrayIndexer * indexer, int part) const
{
  uint32_t color = this->firstcolor;
  if (!indexer || part < 0 || !this->colorpervertex)
    return color;

  const uint8_t * colors = this->colorarray->getArrayPtr();
  const int * indices = indexer->getIndices();

  int numparts = indexer->getNumParts();
  if (!numparts) {
    int unit;
    switch(indexer->getTarget()) {
    case GL_POINTS:
      unit = 1;
      break;
    case GL_LINES:
      unit = 2;
      break;
    default:
      unit = 3;
    }
    part *= unit;
    if (part >= indexer->getNumIndices())
      return color;

    colors += indices[part]*4;
    color = colors[0] << 24;
    color |= colors[1] << 16;
    color |= colors[2] << 8;
    color |= colors[3];
    return color;
  }
  if (part >= numparts)
    return color;

  const GLint * parts = indexer->getPartOffsets();
  colors += indices[part ? parts[part-1] : 0] * 4;
  color = colors[0] << 24;
  color |= colors[1] << 16;
  color |= colors[2] << 8;
  color |= colors[3];
  return color;
}

SoFCVertexCache *
SoFCVertexCache::getWholeCache() const
{
  if (PRIVATE(this)->prevattached)
    return PRIVATE(this)->prevcache;
  return const_cast<SoFCVertexCache*>(this);
}

#undef PRIVATE
#undef PUBLIC
// vim: noai:ts=2:sw=2
