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
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
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
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoIndexedMarkerSet.h>
#include <Inventor/VRMLnodes/SoVRMLIndexedFaceSet.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoMFNode.h>
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

#include <Base/Console.h>
#include "SoFCVertexCache.h"
#include "SoFCDiffuseElement.h"
#include "SoFCVBO.h"
#include "SoFCVertexArrayIndexer.h"
#include "SoFCShapeInfo.h"
#include "COWData.h"
#include "../ViewParams.h"

FC_LOG_LEVEL_INIT("Renderer", true, true)

using namespace Gui;

static SbFCUniqueId VertexCacheId;

// *************************************************************************
typedef SoFCVertexAttribute<SbVec2f> Vec2Array;
typedef SoFCVertexAttribute<SbVec3f> Vec3Array;
typedef SoFCVertexAttribute<SbVec4f> Vec4Array;
typedef SoFCVertexAttribute<uint8_t> ByteArray;
typedef SoFCVertexAttribute<int> IntArray;
typedef CoinPtr<SoFCVertexCache> VertexCachePtr;
typedef SoFCRenderCache::VertexCacheEntry VertexCacheEntry;

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->master)

static SbName * PartIndexField;
static SbName * SeamIndicesField;
static SbName * HighlightIndicesField;
static SbName * ElementSelectableField;
static SbName * OnTopPatternField;
static SbName * ShapeInfoField;

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
    SeamIndicesField = new SbName("seamIndices");
    HighlightIndicesField = new SbName("highlightIndices");
    ElementSelectableField = new SbName("elementSelectable");
    OnTopPatternField = new SbName("onTopPattern");
    ShapeInfoField = new SbName("shapeInfo");
  }

  static void cleanup()
  {
    delete PartIndexField;
    PartIndexField = nullptr;
    delete SeamIndicesField;
    SeamIndicesField = nullptr;
    delete HighlightIndicesField;
    HighlightIndicesField = nullptr;
    delete ElementSelectableField;
    ElementSelectableField = nullptr;
    delete OnTopPatternField;
    OnTopPatternField = nullptr;
    delete ShapeInfoField;
    ShapeInfoField = nullptr;
  }

  struct Vertex {
  public:
    SbVec3f vertex;
    SbVec3f normal;
    SbVec4f texcoord0;
    SbVec2f bumpcoord;
    uint32_t color;
    int texcoordidx;
    int marker = -1;

    bool operator==(const Vertex & v) const {
      return
        (this->vertex == v.vertex) &&
        (this->normal == v.normal) &&
        (this->texcoord0 == v.texcoord0) &&
        (this->bumpcoord == v.bumpcoord) &&
        (this->texcoordidx == v.texcoordidx) &&
        (this->marker == v.marker) &&
        (this->color == v.color);
    }
  };

  struct VertexHasher {
    // copied from boost::hash_combine. 
    template <class S, class T>
    static inline void hash_combine(S& seed, const T& v)
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
      hash_combine(seed, v.marker);
      return seed;
    }
  };

  struct TempStorage {
    std::unordered_map<Vertex, int32_t, VertexHasher> vhash;

    const SbVec2f * bumpcoords;
    int numbumpcoords;

    const uint32_t * packedptr;
    const SbColor * diffuseptr;
    const float * transpptr;

    int numdiffuse;
    int numtransp;

    const SoMultiTextureCoordinateElement * multielem;
    SoState * state = nullptr;

    const int32_t *partindices = nullptr;
    int partcount = 0;

    int pointindexcount = 0;
    bool ispointindexed = false;

    SoFCVertexArrayIndexer::IndexArray prevtriangleindices;
    SoFCVertexArrayIndexer::IndexArray prevlineindices;
    SoFCVertexArrayIndexer::IndexArray prevpointindices;
  };

  SoFCVertexCacheP(SoFCVertexCache * m,
                   SoFCVertexCache * prev,
                   SbFCUniqueId id)
    : master(m),
      prevcache(prev),
      prevattached(false),
      mergeid(0),
      cacheid(++VertexCacheId),
      nodeid(id),
      diffuseid(0),
      transpid(0),
      triangleindexer(nullptr),
      lineindexer(nullptr),
      noseamindexer(nullptr),
      pointindexer(nullptr)
  {
    if (prev) {
      this->tmp = new TempStorage;
      if (PRIVATE(prev)->triangleindexer)
        this->tmp->prevtriangleindices = PRIVATE(prev)->triangleindexer->getIndexArray();
      if (PRIVATE(prev)->lineindexer)
        this->tmp->prevlineindices = PRIVATE(prev)->lineindexer->getIndexArray();
      if (PRIVATE(prev)->pointindexer)
        this->tmp->prevpointindices = PRIVATE(prev)->pointindexer->getIndexArray();
    }
  }

  ~SoFCVertexCacheP()
  {
    delete tmp;
    delete triangleindexer;
    delete lineindexer;
    delete pointindexer;
    delete noseamindexer;
  }

  uint32_t getColor(const SoFCVertexArrayIndexer * indexer, int part) const;

  void getBoundingBox(const SbMatrix * matrix,
                      SbBox3f & bbox,
                      const SoFCVertexArrayIndexer *indexer,
                      int part) const;

  void getBoundingBox(const SbMatrix * matrix, SbBox3f & bbox) const;


  template<class FacesT, class FindT> void
  addTriangles(const FacesT & faces, FindT && find)
  {
    assert(!this->triangleindexer && this->prevcache);

    this->prevattached = true;

    auto prevcache = this->prevcache;
    assert(PRIVATE(prevcache)->triangleindexer);

    auto indexer = PRIVATE(prevcache)->triangleindexer;
    this->triangleindexer = new SoFCVertexArrayIndexer(*indexer, faces, master->getNumVertices());

    if (!indexer->getNumParts() || this->triangleindexer->getNumParts() == indexer->getNumParts())
      return;

    int typesize = this->triangleindexer->useShorts() ? 2 : 4;
    const int * parts = this->triangleindexer->getPartOffsets();

    // Check if we need to adjust solid indices in case of partial rendering
    if (this->solidpartindices.size()) {
      int idx = 0;
      for (auto i : this->solidpartindices) {
        if (find(faces, i)) {
          int prev = i == 0 ? 0 : parts[i-1];
          this->solidpartarray.compareAndSet(idx, prev * typesize);
          this->solidpartcounts.compareAndSet(idx, parts[i] - prev);
          ++idx;
        }
      }
      this->solidpartarray.resize(idx);
      this->solidpartcounts.resize(idx);
    }

    // Check if we need to adjust opaque indices in case of partial rendering
    if (this->transppartindices.size()) {
      int j = 0;
      int idx = 0;
      for (int i : this->transppartindices) {
        for (;j<i; ++j) {
          if (!find(faces, j))
            continue;
          int prev = j == 0 ? 0 : parts[j-1];
          this->opaquepartarray.compareAndSet(idx, prev * typesize);
          this->opaquepartcounts.compareAndSet(idx, parts[j] - prev);
          ++idx;
        }
      }
      this->opaquepartarray.resize(idx);
      this->opaquepartcounts.resize(idx);
    }
  }

  template<class IndicesT> void
  addLines(const IndicesT & lineindices)
  {
    assert(!this->lineindexer && this->prevcache);

    this->prevattached = true;

    auto prevcache = this->prevcache;
    assert(PRIVATE(prevcache)->lineindexer);

    auto indexer = PRIVATE(prevcache)->lineindexer;
    this->lineindexer = new SoFCVertexArrayIndexer(*indexer, lineindices, master->getNumVertices());
  }

  template<class IndicesT> void
  addPoints(const IndicesT & pointindices)
  {
    assert(!this->pointindexer && this->prevcache);

    this->prevattached = true;

    auto prevcache = this->prevcache;
    assert(PRIVATE(prevcache)->pointindexer);

    auto indexer = PRIVATE(prevcache)->pointindexer;
    this->pointindexer = new SoFCVertexArrayIndexer(*indexer, pointindices, master->getNumVertices());
  }

  SbBool depthSortTriangles(SoState * state, bool fullsort, const SbPlane *sortplane);

  void addVertex(const Vertex & v);
  void initColor(int n);

  void close(SoState *);
  void finalizeTriangleIndexer();
  void checkTransparency();

  bool canMerge(const VertexCacheEntry & entry);
  bool canMergeWith(const VertexCacheEntry & other);
  void mergeTo(bool first,
               SoFCVertexCache *vcache,
               const SbMatrix & matrix,
               bool identity) const;

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

  SoFCVertexCache *master;
  CoinPtr<SoFCVertexCache> prevcache;
  bool prevattached;

  int mergeid;
  SbFCUniqueId cacheid;

  SoNode *node = nullptr;
  SbFCUniqueId nodeid;
  SbFCUniqueId diffuseid;
  SbFCUniqueId transpid;
  SbFCUniqueId selnodeid = 0;

  Vec3Array vertexarray;
  Vec3Array normalarray;
  Vec4Array texcoord0array;
  Vec2Array bumpcoordarray;
  ByteArray colorarray;
  SbFCVector<Vec4Array> multitexarray;

  TempStorage* tmp = nullptr;

  int numtranspparts;

  SoMFInt32 *markerindices = nullptr;

  bool hastransp;
  int hassolid = 0;
  bool flipnormal = false;
  int colorpervertex;
  uint32_t firstcolor;

  int lastenabled = -1;
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
  SbFCVector<SortEntry> deptharray;

  SbFCVector<intptr_t> sortedpartarray;
  SbFCVector<int32_t> sortedpartcounts;

  COWVector<int> transppartindices;
  COWVector<intptr_t> opaquepartarray;
  COWVector<int32_t> opaquepartcounts;

  COWVector<int> solidpartindices;
  COWVector<intptr_t> solidpartarray;
  COWVector<int32_t> solidpartcounts;

  COWVector<SbVec3f> partcenters;
  COWVector<int32_t> nonflatparts;
  COWVector<int32_t> seamindices;
  COWVector<int> markers;

  SoFCVertexArrayIndexer * triangleindexer;
  SoFCVertexArrayIndexer * lineindexer;
  SoFCVertexArrayIndexer * noseamindexer;
  SoFCVertexArrayIndexer * pointindexer;

  bool elementselectable;
  bool ontoppattern;

  COWVector<int> highlightindices;

  mutable SbBox3f boundbox;
};

// *************************************************************************

SoFCVertexCache::SoFCVertexCache(SoState * state, SoNode * node, SoFCVertexCache * prev)
  : SoCache(state),
    pimpl(new SoFCVertexCacheP(this, prev, node->getNodeId()))
{
  PRIVATE(this)->node = node;
  if (!PRIVATE(this)->tmp)
    PRIVATE(this)->tmp = new SoFCVertexCacheP::TempStorage;
  if (node) {
    if (node->isOfType(SoMarkerSet::getClassTypeId()))
      PRIVATE(this)->markerindices = &static_cast<SoMarkerSet*>(node)->markerIndex;
    else if (node->isOfType(SoIndexedMarkerSet::getClassTypeId())) {
      PRIVATE(this)->markerindices = &static_cast<SoIndexedMarkerSet*>(node)->markerIndex;
      PRIVATE(this)->tmp->ispointindexed = true;
    }
    else if (node->isOfType(SoVRMLIndexedFaceSet::getClassTypeId())) {
      auto faceset = static_cast<SoVRMLIndexedFaceSet*>(node);
      PRIVATE(this)->flipnormal = !faceset->ccw.getValue();
      PRIVATE(this)->hassolid = faceset->solid.getValue() ? 2 : 0;
    }
    if (PRIVATE(this)->markerindices && !PRIVATE(this)->markerindices->getNum())
      PRIVATE(this)->markerindices = nullptr;
  }
  PRIVATE(this)->tmp->state = state;
  PRIVATE(this)->elementselectable = true;
  PRIVATE(this)->ontoppattern = false;

  const SoField * field = node->getField(*PartIndexField);
  if (field && field->isOfType(SoMFInt32::getClassTypeId())) {
    const SoMFInt32 * indices = static_cast<const SoMFInt32*>(field);
    PRIVATE(this)->tmp->partindices = indices->getValues(0);
    PRIVATE(this)->tmp->partcount = indices->getNum();
  }

  field = node->getField(*SeamIndicesField);
  if (field && field->isOfType(SoMFInt32::getClassTypeId())) {
    const SoMFInt32 * indices = static_cast<const SoMFInt32*>(field);
    if (indices->getNum()) {
      PRIVATE(this)->seamindices.resize(indices->getNum());
      auto seamindices = PRIVATE(this)->seamindices.at(0);
      memcpy(seamindices, indices->getValues(0), indices->getNum()*4);
      std::sort(seamindices, seamindices + indices->getNum());
    }
  }

  field = node->getField(*HighlightIndicesField);
  if (field && field->isOfType(SoMFInt32::getClassTypeId())) {
    const SoMFInt32 * indices = static_cast<const SoMFInt32*>(field);
    const int32_t * values = indices->getValues(0);
    PRIVATE(this)->highlightindices.reserve(indices->getNum());
    for (int i = 0, c = indices->getNum(); i < c; ++i)
      PRIVATE(this)->highlightindices.push_back(values[i]);
  }

  field = node->getField(*ElementSelectableField);
  if (field && field->isOfType(SoSFBool::getClassTypeId()))
    PRIVATE(this)->elementselectable = static_cast<const SoSFBool*>(field)->getValue();

  field = node->getField(*OnTopPatternField);
  if (field && field->isOfType(SoSFBool::getClassTypeId()))
    PRIVATE(this)->ontoppattern = static_cast<const SoSFBool*>(field)->getValue();
}

SoFCVertexCache::SoFCVertexCache(SoFCVertexCache & prev)
  : SoCache(nullptr),
    pimpl(new SoFCVertexCacheP(this, &prev, PRIVATE(&prev)->nodeid))
{
  auto pprev = &prev;

  PRIVATE(this)->node = PRIVATE(pprev)->node;
  PRIVATE(this)->markerindices = PRIVATE(pprev)->markerindices;
  PRIVATE(this)->diffuseid = PRIVATE(pprev)->diffuseid;
  PRIVATE(this)->transpid = PRIVATE(pprev)->transpid;

  PRIVATE(this)->vertexarray = PRIVATE(pprev)->vertexarray;
  PRIVATE(this)->normalarray = PRIVATE(pprev)->normalarray;
  PRIVATE(this)->texcoord0array = PRIVATE(pprev)->texcoord0array;
  PRIVATE(this)->bumpcoordarray = PRIVATE(pprev)->bumpcoordarray;
  PRIVATE(this)->colorarray = PRIVATE(pprev)->colorarray;
  PRIVATE(this)->multitexarray = PRIVATE(pprev)->multitexarray;

  PRIVATE(this)->numtranspparts = PRIVATE(pprev)->numtranspparts;
  PRIVATE(this)->hastransp = PRIVATE(pprev)->hastransp;
  PRIVATE(this)->firstcolor = PRIVATE(pprev)->firstcolor;
  PRIVATE(this)->colorpervertex = PRIVATE(pprev)->colorpervertex;
  PRIVATE(this)->hassolid = PRIVATE(pprev)->hassolid;
  PRIVATE(this)->flipnormal = PRIVATE(pprev)->flipnormal;

  PRIVATE(this)->partcenters = PRIVATE(pprev)->partcenters;
  PRIVATE(this)->nonflatparts = PRIVATE(pprev)->nonflatparts;

  PRIVATE(this)->transppartindices = PRIVATE(pprev)->transppartindices;
  PRIVATE(this)->opaquepartarray = PRIVATE(pprev)->opaquepartarray;
  PRIVATE(this)->opaquepartcounts = PRIVATE(pprev)->opaquepartcounts;

  PRIVATE(this)->solidpartindices = PRIVATE(pprev)->solidpartindices;
  PRIVATE(this)->solidpartarray = PRIVATE(pprev)->solidpartarray;
  PRIVATE(this)->solidpartcounts = PRIVATE(pprev)->solidpartcounts;

  PRIVATE(this)->seamindices = PRIVATE(pprev)->seamindices;

  PRIVATE(this)->highlightindices = PRIVATE(pprev)->highlightindices;

  PRIVATE(this)->markers = PRIVATE(pprev)->markers;

  PRIVATE(this)->elementselectable = PRIVATE(pprev)->elementselectable;
  PRIVATE(this)->ontoppattern = PRIVATE(pprev)->ontoppattern;
}

SoFCVertexCache::SoFCVertexCache(const SbBox3f &bbox)
  : SoCache(nullptr),
    pimpl(new SoFCVertexCacheP(this, nullptr, 0xD2A25905))
{
  float minx, miny, minz, maxx, maxy, maxz;
  bbox.getBounds(minx, miny, minz, maxx, maxy, maxz);
  for (int i = 0; i < 8; i++) {
    PRIVATE(this)->vertexarray.append(
        SbVec3f((i&1) ? minx : maxx,
                (i&2) ? miny : maxy,
                (i&4) ? minz : maxz));

  }
  auto verts = PRIVATE(this)->vertexarray.getArrayPtr();
  static const int indices[][2] = {
    {0,1},
    {1,3},
    {3,2},
    {2,0},
    {4,5},
    {5,7},
    {7,6},
    {6,4},
    {0,4},
    {1,5},
    {2,6},
    {3,7},
  };
  for (int i=0; i<12; ++i) {
    int a = indices[i][0];
    int b = indices[i][1];
    if ((verts[a]-verts[b]).sqrLength() < 1e-12)
      continue;
    if (!PRIVATE(this)->lineindexer)
      PRIVATE(this)->lineindexer = new SoFCVertexArrayIndexer;
    PRIVATE(this)->lineindexer->addLine(a,b,0);
  }
  if (PRIVATE(this)->lineindexer) {
    PRIVATE(this)->lineindexer->close();
  } else {
    PRIVATE(this)->pointindexer = new SoFCVertexArrayIndexer;
    PRIVATE(this)->pointindexer->addPoint(0);
    PRIVATE(this)->pointindexer->close();
  }
}

SoFCVertexCache::~SoFCVertexCache()
{
  delete pimpl;
}

void
SoFCVertexCache::setFaceColors(const SbFCVector<std::pair<int, uint32_t> > &colors)
{
  if (!PRIVATE(this)->triangleindexer)
    return;

  int numparts = PRIVATE(this)->triangleindexer->getNumParts();
  if (!numparts)
    return;

  if (colors.empty()) {
    PRIVATE(this)->colorarray.truncate();
    PRIVATE(this)->colorpervertex = 0;
    return;
  }

  auto prev = PRIVATE(this)->prevcache.get();
  if (!PRIVATE(this)->colorarray || !prev || !PRIVATE(prev)->colorarray) {
    PRIVATE(this)->initColor(PRIVATE(this)->vertexarray.getLength()*4);
  } else
    PRIVATE(this)->colorarray = PRIVATE(prev)->colorarray;

  PRIVATE(this)->colorpervertex = 1;
  auto indices = PRIVATE(this)->triangleindexer->getIndices();
  const int *offsets = PRIVATE(this)->triangleindexer->getPartOffsets();
  PRIVATE(this)->hastransp = false;
  for (auto &v : colors) {
    int start, end;
    if (v.first < 0) {
      start = 0;
      end = PRIVATE(this)->vertexarray.getLength();
    } else {
      start = v.first ? offsets[v.first-1] : 0;
      end = offsets[v.first];
    }
    uint8_t r,g,b,a;
    r = (v.second >> 24) & 0xff;
    g = (v.second >> 16) & 0xff;
    b = (v.second >> 8) & 0xff;
    a = (v.second) & 0xff;
    if (a != 0xff)
      PRIVATE(this)->hastransp = true;
    auto &array = PRIVATE(this)->colorarray;
    for (int i=start; i<end; ++i) {
      int idx = indices[i] * 4;
      array.set(idx, r);
      array.set(idx+1, g);
      array.set(idx+2, b);
      array.set(idx+3, a);
    }
  }
  PRIVATE(this)->checkTransparency();
}

void
SoFCVertexCache::resetNode()
{
  PRIVATE(this)->node = nullptr;
}

bool
SoFCVertexCache::isElementSelectable() const
{
  return PRIVATE(this)->elementselectable;
}

bool
SoFCVertexCache::allowOnTopPattern() const
{
  return PRIVATE(this)->ontoppattern;
}

void
SoFCVertexCache::open(SoState * state)
{
  assert(!PRIVATE(this)->prevattached);
  assert(PRIVATE(this)->tmp);

  SoCacheElement::set(state, this);

  // TODO: It would be ideal if we can get access to the captured elements
  // indside ourself (i.e. SoCache), but it is stored in private class at the
  // moment. Because not all shapes uses SoCoordinateElement/SoNormalElement,
  // for example, primitive shapes like SoCube. In these cases, the correct
  // way is to key on shape's nodeid.

  SoFCVertexCache *prev = PRIVATE(this)->prevcache;

  auto delem = static_cast<const SoFCDiffuseElement*>(
      state->getConstElement(SoFCDiffuseElement::getClassStackIndex()));
  if (delem->getDiffuseId() || delem->getTransparencyId()) {
    // calling get() below to capture the element in cache
    PRIVATE(this)->diffuseid = SoFCDiffuseElement::get(state, &PRIVATE(this)->transpid);
  }

  if (prev) {
    PRIVATE(this)->vertexarray.init(PRIVATE(prev)->vertexarray);
    PRIVATE(this)->normalarray.init(PRIVATE(prev)->normalarray);
  }

  const SoBumpMapCoordinateElement * belem =
    SoBumpMapCoordinateElement::getInstance(state);

  PRIVATE(this)->tmp->numbumpcoords = belem->getNum();
  PRIVATE(this)->tmp->bumpcoords = belem->getArrayPtr();
  if (prev && PRIVATE(this)->tmp->numbumpcoords)
    PRIVATE(this)->bumpcoordarray.init(PRIVATE(prev)->bumpcoordarray);

  SoLazyElement * lelem = SoLazyElement::getInstance(state);

  PRIVATE(this)->tmp->numdiffuse = lelem->getNumDiffuse();
  PRIVATE(this)->numtranspparts = 0;
  PRIVATE(this)->tmp->numtransp = lelem->getNumTransparencies();
  if (lelem->isPacked()) {
    PRIVATE(this)->tmp->packedptr = lelem->getPackedPointer();
    PRIVATE(this)->tmp->diffuseptr = NULL;
    PRIVATE(this)->tmp->transpptr = NULL;
  }
  else {
    PRIVATE(this)->tmp->packedptr = NULL;
    PRIVATE(this)->tmp->diffuseptr = lelem->getDiffusePointer();
    PRIVATE(this)->tmp->transpptr = lelem->getTransparencyPointer();
  }

  // set up variables to test if we need to supply color per vertex
  if (PRIVATE(this)->tmp->numdiffuse <= 1 && PRIVATE(this)->tmp->numtransp <= 1)
    PRIVATE(this)->colorpervertex = 0;
  else {
    PRIVATE(this)->colorpervertex = -1;
    if (prev)
      PRIVATE(this)->colorarray.init(PRIVATE(prev)->colorarray);
  }

  // just store diffuse color with index 0
  if (PRIVATE(this)->tmp->packedptr) {
    PRIVATE(this)->firstcolor = PRIVATE(this)->tmp->packedptr[0];
  }
  else {
    SbColor tmpc = PRIVATE(this)->tmp->diffuseptr[0];
    float tmpt = PRIVATE(this)->tmp->transpptr[0];
    PRIVATE(this)->firstcolor = tmpc.getPackedValue(tmpt);
  }
  PRIVATE(this)->hastransp = (PRIVATE(this)->firstcolor & 0xff)!=0xff;

  // set up for multi texturing
  PRIVATE(this)->lastenabled = -1;
  SoMultiTextureEnabledElement::getEnabledUnits(state, PRIVATE(this)->lastenabled);
  PRIVATE(this)->tmp->multielem = NULL;

}

SbFCUniqueId
SoFCVertexCache::getNodeId() const
{
  return PRIVATE(this)->nodeid;
}

SbFCUniqueId
SoFCVertexCache::getSelectionNodeId() const
{
  return PRIVATE(this)->selnodeid;
}

void
SoFCVertexCache::setSelectionNodeId(SbFCUniqueId id)
{
  PRIVATE(this)->selnodeid = id;
}

SoNode *
SoFCVertexCache::getNode() const
{
  return PRIVATE(this)->node;
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
  PRIVATE(this)->close(state);
}

void
SoFCVertexCacheP::close(SoState * state)
{
  if (this->normalarray && state) {
    if (!this->triangleindexer) {
      const SoNormalElement *nelem = SoNormalElement::getInstance(state);
      if (nelem->getNum() == 0)
        this->normalarray.truncate();
    }
  }
  if (this->triangleindexer)
    this->triangleindexer->close(this->tmp->partindices, this->tmp->partcount);
  if (this->lineindexer)
    this->lineindexer->close();
  if (this->pointindexer)
    this->pointindexer->close();

  finalizeTriangleIndexer();

  delete this->tmp;
  this->tmp = nullptr;
}

void
SoFCVertexCacheP::finalizeTriangleIndexer()
{
  if (!this->triangleindexer || !this->triangleindexer->getNumParts())
    return;

  const int *parts = this->triangleindexer->getPartOffsets();
  const GLint *indices = this->triangleindexer->getIndices();
  const SbVec3f *vertices = this->vertexarray.getArrayPtr();
  int prev = 0;
  int numparts = this->triangleindexer->getNumParts();
  this->partcenters.clear();
  this->nonflatparts.clear();
  this->partcenters.reserve(numparts);
  for (int i=0; i<numparts; ++i) {
    SbBox3f bbox;
    int n = parts[i]-prev;
    for (int k=0; k<n; ++k)
      bbox.extendBy(vertices[indices[k+prev]]);
    float dx,dy,dz;
    bbox.getSize(dx, dy, dz);
    if (dx > 1e-6f && dy > 1e-6f && dz > 1e-6)
      this->nonflatparts.push_back(i);
    this->partcenters.push_back(bbox.getCenter());
    prev = parts[i];
  }

  auto field = this->node ? this->node->getField(*ShapeInfoField) : nullptr;
  if (field && field->isOfType(SoMFNode::getClassTypeId())) {
    this->solidpartindices.clear();
    auto nodes = static_cast<SoMFNode*>(field);
    int solidpartcount = 0;
    for (int i=0, c=nodes->getNum(); i<c; ++i) {
      if (!nodes->getNode(i)
          || !nodes->getNode(i)->isOfType(SoFCShapeInstance::getClassTypeId()))
        continue;
      auto instance = static_cast<SoFCShapeInstance*>(nodes->getNode(i));
      if (!instance->shapeInfo.getValue()
          || !instance->shapeInfo.getValue()->isOfType(SoFCShapeInfo::getClassTypeId()))
        continue;
      auto info = static_cast<SoFCShapeInfo*>(instance->shapeInfo.getValue());
      if (info->shapeType.getValue() == SoFCShapeInfo::SOLID)
        solidpartcount += info->partCount.getValue();
    }
    if (solidpartcount >= numparts)
      this->hassolid = 2;
    else if (solidpartcount) {
      this->hassolid = 1;
      for (int i=0, c=nodes->getNum(); i<c; ++i) {
        if (!nodes->getNode(i)
            || nodes->getNode(i)->isOfType(SoFCShapeInstance::getClassTypeId()))
          continue;
        auto instance = static_cast<SoFCShapeInstance*>(nodes->getNode(i));
        if (!instance->shapeInfo.getValue()
            || !instance->shapeInfo.getValue()->isOfType(SoFCShapeInfo::getClassTypeId()))
          continue;
        auto info = static_cast<SoFCShapeInfo*>(instance->shapeInfo.getValue());
        if (info->shapeType.getValue() != SoFCShapeInfo::SOLID)
          continue;
        int partidx = instance->partIndex.getValue();
        int count = info->partCount.getValue();
        if (count <= 0 || partidx + count > numparts)
          continue;
        if (info->shapeType.getValue() == SoFCShapeInfo::SOLID) {
          for (int j=0; j<count; ++j)
            this->solidpartindices.push_back(j+partidx);
        }
        partidx += count;
      }
    }
  }

  this->solidpartarray.clear();
  this->solidpartcounts.clear();
  int typesize = this->triangleindexer->useShorts() ? 2 : 4;
  if (this->solidpartindices.size()) {
    for (int j : this->solidpartindices) {
      int prev = j == 0 ? 0 : parts[j-1];
      this->solidpartarray.push_back(prev * typesize);
      this->solidpartcounts.push_back(parts[j] - prev);
    }
  }

  this->checkTransparency();
}

void
SoFCVertexCacheP::checkTransparency()
{
  if (!this->triangleindexer) return;
  int numparts = this->triangleindexer->getNumParts();
  if (!numparts) return;

  int transpidx = 0;
  const int *parts = this->triangleindexer->getPartOffsets();

  if (this->colorpervertex <= 0)
    this->numtranspparts = this->hastransp ? numparts : 0;
  else if (this->hastransp) {
    const GLint *indices = this->triangleindexer->getIndices();
    int prev = 0;
    for (int i=0; i<numparts; ++i) {
      bool transp = false;
      int n = parts[i]-prev;
      for (int k=0; k<n; ++k) {
        if (this->colorarray[indices[k+prev]*4 + 3] != 0xff) {
          transp = true;
          break;
        }
      }
      if (transp)
        this->transppartindices.compareAndSet(transpidx++, i);
      prev = parts[i];
    }
    this->numtranspparts = (int)this->transppartindices.size();
  }
  this->transppartindices.resize(transpidx);

  int opaqueidx = 0;
  if (this->numtranspparts && this->numtranspparts != numparts) {
    int typesize = this->triangleindexer->useShorts() ? 2 : 4;
    int prev = 0;
    for (int i : this->transppartindices) {
      if (i != prev) {
        this->opaquepartarray.compareAndSet(opaqueidx, (prev ? parts[prev-1] : 0) * typesize);
        this->opaquepartcounts.compareAndSet(opaqueidx, parts[i-1] - (prev ? parts[prev-1] : 0));
        ++opaqueidx;
      }
      prev = i+1;
    }
    if (prev < numparts) {
      this->opaquepartarray.compareAndSet(opaqueidx, (prev ? parts[prev-1] : 0) * typesize);
      this->opaquepartcounts.compareAndSet(opaqueidx, parts[numparts-1] - (prev ? parts[prev-1] : 0));
      ++opaqueidx;
    }
  }
  this->opaquepartarray.resize(opaqueidx);
  this->opaquepartcounts.resize(opaqueidx);
}

int
SoFCVertexCache::getNumNonFlatParts() const
{
  return (int)PRIVATE(this)->nonflatparts.size();
}

const int *
SoFCVertexCache::getNonFlatParts() const
{
  return getNumNonFlatParts() ? &PRIVATE(this)->nonflatparts[0] : nullptr;
}

int
SoFCVertexCache::getNumFaceParts() const
{
  if (PRIVATE(this)->triangleindexer)
    return PRIVATE(this)->triangleindexer->getNumParts();
  return 0;
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
  if (!this->vertexarray) return;
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

  int vnum = this->vertexarray.getLength();
  if (SoFCVBO::shouldCreateVBO(state, contextid, vnum)) {
    this->enableVBOs(state, glue, contextid, color, normal, texture, enabled, lastenabled);
    indexer->render(state, glue, TRUE, contextid, offsets, counts, drawcount);
    this->disableVBOs(glue, color, normal, texture, enabled, lastenabled);
  } else if (SoFCVBO::shouldRenderAsVertexArrays(state, contextid, vnum)) {
    this->enableArrays(glue, color, normal, texture, enabled, lastenabled);
    if (!drawcount)
      indexer->render(state, glue, FALSE, contextid);
    else {
      int typeshift = indexer->useShorts() ? 1 : 2;
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
      int typeshift = indexer->useShorts() ? 1 : 2;
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
  if (!PRIVATE(this)->triangleindexer) return TRUE;
  if (!PRIVATE(this)->numtranspparts) return !PRIVATE(this)->hastransp;
  return PRIVATE(this)->numtranspparts < PRIVATE(this)->triangleindexer->getNumParts();
}

SbBool
SoFCVertexCache::hasTransparency() const
{
  return PRIVATE(this)->hastransp;
}

void
SoFCVertexCache::renderTriangles(SoState * state, const int arrays, int part, const SbPlane *viewplane)
{
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->triangleindexer, arrays, part, 3);
    return;
  }

  int drawcount = 0;
  const intptr_t * offsets = NULL;
  const int32_t * counts = NULL;

  if (arrays & (SORTED_ARRAY | FULL_SORTED_ARRAY)) {
    if (PRIVATE(this)->depthSortTriangles(state, (arrays & FULL_SORTED_ARRAY) ? true : false, viewplane)) {
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
SoFCVertexCache::renderSolids(SoState * state)
{
  int drawcount = (int)PRIVATE(this)->solidpartarray.size();
  const intptr_t * offsets = NULL;
  const int32_t * counts = NULL;
  int arrays = NON_SORTED_ARRAY;
  if (drawcount) {
    offsets = &PRIVATE(this)->solidpartarray[0];
    counts = &PRIVATE(this)->solidpartcounts[0];
  }
  PRIVATE(this)->render(state, PRIVATE(this)->triangleindexer, arrays, offsets, counts, drawcount);
}

void
SoFCVertexCache::renderLines(SoState * state, const int arrays, int part, bool noseam)
{
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->lineindexer, arrays, part, 2);
    return;
  }
  if (noseam && PRIVATE(this)->seamindices.size() && PRIVATE(this)->lineindexer) {
    if (!PRIVATE(this)->noseamindexer) {
      PRIVATE(this)->noseamindexer = new SoFCVertexArrayIndexer(
          *PRIVATE(this)->lineindexer, PRIVATE(this)->seamindices.getData(), -1, true); 
    }
    PRIVATE(this)->render(state, PRIVATE(this)->noseamindexer, arrays);
  } else
    PRIVATE(this)->render(state, PRIVATE(this)->lineindexer, arrays);
}

void
SoFCVertexCache::renderPoints(SoGLRenderAction * action, const int arrays, int part)
{
  if (!PRIVATE(this)->pointindexer)
    return;
  int num = PRIVATE(this)->pointindexer->getNumIndices();
  if (!num)
    return;

  auto state = action->getState();
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->pointindexer, arrays, part, 1);
    return;
  }
  if (!PRIVATE(this)->markerindices) {
    PRIVATE(this)->render(state, PRIVATE(this)->pointindexer, arrays);
    return;
  }

  state->push();
  SoMultiTextureEnabledElement::disableAll(state);

  const SbMatrix & mat = SoModelMatrixElement::get(state);
  //const SbViewVolume & vv = SoViewVolumeElement::get(state);
  const SbViewportRegion & vp = SoViewportRegionElement::get(state);
  const SbMatrix & projmatrix = (mat * SoViewingMatrixElement::get(state) *
                                 SoProjectionMatrixElement::get(state));
  SbVec2s vpsize = vp.getViewportSizePixels();

  GLint numPlanes = 0;
  glGetIntegerv(GL_MAX_CLIP_PLANES, &numPlanes);
  SbList<SbBool> planesEnabled;
  for (GLint i = 0; i < numPlanes; ++i) {
    planesEnabled.append(glIsEnabled(GL_CLIP_PLANE0 + i));
    glDisable(GL_CLIP_PLANE0 + i);
  }

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, vpsize[0], 0, vpsize[1], -1.0f, 1.0f);

  auto vertices = PRIVATE(this)->vertexarray.getArrayPtr();
  int numvert = PRIVATE(this)->vertexarray.getLength();

  const uint8_t *colors = nullptr;
  if (PRIVATE(this)->colorpervertex > 0
      && (arrays & COLOR)
      && PRIVATE(this)->colorarray.getLength())
  {
    colors = PRIVATE(this)->colorarray.getArrayPtr();
    assert(numvert*4 == PRIVATE(this)->colorarray.getLength());
  }

  assert(PRIVATE(this)->markers.size() == num);

  auto indices = PRIVATE(this)->pointindexer->getIndices();
  for (int i = 0, cidx=0; i < num; i++, cidx+=4) {
    int32_t idx = indices[i];
    if (idx < 0 || idx >= numvert)
      continue;

    int marker = PRIVATE(this)->markers[i];
    if (marker == SoMarkerSet::NONE) { continue; }//no marker to render

    SbVec2s size;
    const unsigned char * bytes;
    SbBool isLSBFirst;

    if (marker >= SoMarkerSet::getNumDefinedMarkers()) continue;

    SbBool validMarker = SoMarkerSet::getMarker(marker, size, bytes, isLSBFirst);
    if (!validMarker) continue;

    if (colors) 
      glColor4ub(colors[cidx], colors[cidx+1], colors[cidx+2], colors[cidx+3]);

    SbVec3f point = vertices[idx];

    // OpenGL's glBitmap() will not be clipped against anything but
    // the near and far planes. We want markers to also be clipped
    // against other clipping planes, to behave like the SoPointSet
    // superclass.
    const SbBox3f bbox(point, point);
    // FIXME: if there are *heaps* of markers, this next line will
    // probably become a bottleneck. Should really partition marker
    // positions in a oct-tree data structure and cull several at
    // the same time.  20031219 mortene.
    if (SoCullElement::cullTest(state, bbox, TRUE)) { continue; }

    projmatrix.multVecMatrix(point, point);
    point[0] = (point[0] + 1.0f) * 0.5f * vpsize[0];
    point[1] = (point[1] + 1.0f) * 0.5f * vpsize[1];

    // To have the exact center point of the marker drawn at the
    // projected 3D position.  (FIXME: I haven't actually checked that
    // this is what TGS' implementation of the SoMarkerSet node does
    // when rendering, but it seems likely. 20010823 mortene.)

    point[0] = point[0] - (size[0] - 1) / 2;
    point[1] = point[1] - (size[1] - 1) / 2;

    //FIXME: this will probably fail if someone has overwritten one of the
    //built-in markers. Currently there is no way of fetching a marker's
    //alignment from outside the SoMarkerSet class though. 20090424 wiesener
    int align = (marker >= SoMarkerSet::NUM_MARKERS) ? 1 : 4;
    glPixelStorei(GL_UNPACK_ALIGNMENT, align);
    glRasterPos3f(point[0], point[1], -point[2]);
    glBitmap(size[0], size[1], 0, 0, 0, 0, bytes);
  }

  for (GLint i = 0; i < numPlanes; ++i) {
    if (planesEnabled[i]) {
      glEnable(GL_CLIP_PLANE0 + i);
    }
  }

  // FIXME: this looks wrong, shouldn't we rather reset the alignment
  // value to what it was previously?  20010824 mortene.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restore default value
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  state->pop();
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

  if (!this->tmp->multielem && this->lastenabled >= 0) {
    this->tmp->multielem = SoMultiTextureCoordinateElement::getInstance(this->tmp->state);
    if (this->prevcache)
      this->texcoord0array.init(PRIVATE(this->prevcache)->texcoord0array);

    if (this->lastenabled > 0) {
      this->multitexarray.clear();
      this->multitexarray.resize(this->lastenabled+1);
      for (int i = 1; i < this->lastenabled+1; ++i) {
        if (this->prevcache && PRIVATE(this->prevcache)->lastenabled >= i)
          this->multitexarray[i].init(PRIVATE(this->prevcache)->multitexarray[i]);
      }
    }
  }

  this->prevcache.reset();
}

void
SoFCVertexCache::addTriangles(const std::map<int, int> & faces)
{
  PRIVATE(this)->addTriangles(faces,
    [](const std::map<int, int> & faces, int idx) {
      return faces.count(idx)!=0;
    });
}

void
SoFCVertexCache::addTriangles(const std::set<int> & faces)
{
  PRIVATE(this)->addTriangles(faces,
    [](const std::set<int> & faces, int idx) {
      return faces.count(idx)!=0;
    });
}

void
SoFCVertexCache::addTriangles(const SbFCVector<int> & faces)
{
  PRIVATE(this)->addTriangles(faces,
    [](const SbFCVector<int> & faces, int idx) {
      return std::find(faces.begin(), faces.end(), idx) != faces.end();
    });
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
      if (PRIVATE(this)->tmp->packedptr) {
        v.color = PRIVATE(this)->tmp->packedptr[SbClamp(midx, 0, PRIVATE(this)->tmp->numdiffuse-1)];
      }
      else {
        SbColor tmpc = PRIVATE(this)->tmp->diffuseptr[SbClamp(midx,0,PRIVATE(this)->tmp->numdiffuse-1)];
        float tmpt = PRIVATE(this)->tmp->transpptr[SbClamp(midx,0,PRIVATE(this)->tmp->numtransp-1)];
        v.color = tmpc.getPackedValue(tmpt);
      }
      if (PRIVATE(this)->colorpervertex < 0 && v.color != PRIVATE(this)->firstcolor)
        PRIVATE(this)->colorpervertex = 1;
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
      if (PRIVATE(this)->tmp->numbumpcoords) {
        v.bumpcoord = PRIVATE(this)->tmp->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->tmp->numbumpcoords-1)];
      }
    }

    auto res = PRIVATE(this)->tmp->vhash.insert(
        std::make_pair(v, PRIVATE(this)->vertexarray.getLength()));
    if (res.second) {
      PRIVATE(this)->addVertex(v);
      // update texture coordinates for unit 1-n
      for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
        if (v.texcoordidx >= 0 &&
            (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
          PRIVATE(this)->multitexarray[j].append(PRIVATE(this)->tmp->multielem->get4(j, v.texcoordidx));
        }
        else if (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
          PRIVATE(this)->multitexarray[j].append(PRIVATE(this)->tmp->multielem->get(j, v.vertex, v.normal));
        }
        else {
          PRIVATE(this)->multitexarray[j].append(v.texcoord0);
        }
      }
    }
    triangleindices[i] = res.first->second;
  }

  if (!PRIVATE(this)->triangleindexer) {
    PRIVATE(this)->triangleindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->tmp->prevtriangleindices);
  }
  PRIVATE(this)->triangleindexer->addTriangle(triangleindices[0],
                                              triangleindices[1],
                                              triangleindices[2]);
}

void
SoFCVertexCache::addLines(const std::map<int, int> & lineindices)
{
  PRIVATE(this)->addLines(lineindices);
}

void
SoFCVertexCache::addLines(const std::set<int> & lineindices)
{
  PRIVATE(this)->addLines(lineindices);
}

void
SoFCVertexCache::addLines(const SbFCVector<int> & lineindices)
{
  PRIVATE(this)->addLines(lineindices);
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
      if (PRIVATE(this)->tmp->packedptr) {
        v.color = PRIVATE(this)->tmp->packedptr[SbClamp(midx, 0, PRIVATE(this)->tmp->numdiffuse-1)];
      }
      else {
        SbColor tmpc = PRIVATE(this)->tmp->diffuseptr[SbClamp(midx,0,PRIVATE(this)->tmp->numdiffuse-1)];
        float tmpt = PRIVATE(this)->tmp->transpptr[SbClamp(midx,0,PRIVATE(this)->tmp->numtransp-1)];
        v.color = tmpc.getPackedValue(tmpt);
      }
      if (PRIVATE(this)->colorpervertex < 0 && v.color != PRIVATE(this)->firstcolor)
        PRIVATE(this)->colorpervertex = 1;
      PRIVATE(this)->hastransp = (PRIVATE(this)->hastransp || (v.color&0xff) != 0xff);
    }

    const SoDetail * d = vp[i]->getDetail();

    if (d && d->isOfType(SoLineDetail::getClassTypeId())) {
      ld = static_cast<const SoLineDetail *>(d);
      const SoPointDetail * pd;
      if (i == 0) pd = ld->getPoint0();
      else pd = ld->getPoint1();

      int tidx  = v.texcoordidx = static_cast<const SoPointDetail *>(pd)->getTextureCoordIndex();
      if (PRIVATE(this)->tmp->numbumpcoords) {
        v.bumpcoord = PRIVATE(this)->tmp->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->tmp->numbumpcoords-1)];
      }
    }

    auto res = PRIVATE(this)->tmp->vhash.insert(
        std::make_pair(v, PRIVATE(this)->vertexarray.getLength()));
    if (res.second) {
      PRIVATE(this)->addVertex(v);
      // update texture coordinates for unit 1-n
      for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
        if (v.texcoordidx >= 0 &&
            (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
          PRIVATE(this)->multitexarray[j].append(PRIVATE(this)->tmp->multielem->get4(j, v.texcoordidx));
        }
        else if (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
          PRIVATE(this)->multitexarray[j].append(PRIVATE(this)->tmp->multielem->get(j, v.vertex, v.normal));
        }
        else {
          PRIVATE(this)->multitexarray[j].append(v.texcoord0);
        }
      }
    }
    lineindices[i] = res.first->second;
  }

  if (!PRIVATE(this)->lineindexer) {
    PRIVATE(this)->lineindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->tmp->prevlineindices);
  }
  PRIVATE(this)->lineindexer->addLine(lineindices[0], lineindices[1], ld ? ld->getLineIndex() : -1);
}

void
SoFCVertexCache::addPoints(const std::map<int, int> & pointindices)
{
  PRIVATE(this)->addPoints(pointindices);
}

void
SoFCVertexCache::addPoints(const std::set<int> & pointindices)
{
  PRIVATE(this)->addPoints(pointindices);
}

void
SoFCVertexCache::addPoints(const SbFCVector<int> & pointindices)
{
  PRIVATE(this)->addPoints(pointindices);
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
    if (PRIVATE(this)->tmp->packedptr) {
      v.color = PRIVATE(this)->tmp->packedptr[SbClamp(midx, 0, PRIVATE(this)->tmp->numdiffuse-1)];
    }
    else {
      SbColor tmpc = PRIVATE(this)->tmp->diffuseptr[SbClamp(midx,0,PRIVATE(this)->tmp->numdiffuse-1)];
      float tmpt = PRIVATE(this)->tmp->transpptr[SbClamp(midx,0,PRIVATE(this)->tmp->numtransp-1)];
      v.color = tmpc.getPackedValue(tmpt);
    }
    if (PRIVATE(this)->colorpervertex < 0 && v.color != PRIVATE(this)->firstcolor)
      PRIVATE(this)->colorpervertex = 1;
    PRIVATE(this)->hastransp = (PRIVATE(this)->hastransp || (v.color&0xff) != 0xff);
  }

  const SoDetail * d = v0->getDetail();

  if (d && d->isOfType(SoPointDetail::getClassTypeId())) {
    const SoPointDetail * pd = static_cast<const SoPointDetail *>(d);
    if (PRIVATE(this)->markerindices) {
      int markeridx;
      if (PRIVATE(this)->tmp->ispointindexed)
        markeridx = PRIVATE(this)->tmp->pointindexcount;
      else
        markeridx = pd->getCoordinateIndex();
      v.marker = (*PRIVATE(this)->markerindices)[std::min(
          markeridx, PRIVATE(this)->markerindices->getNum()-1)];
    }
    int tidx  = v.texcoordidx = pd->getTextureCoordIndex();
    if (PRIVATE(this)->tmp->numbumpcoords) {
      v.bumpcoord = PRIVATE(this)->tmp->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->tmp->numbumpcoords-1)];
    }
  }

  if (!PRIVATE(this)->pointindexer) {
    PRIVATE(this)->pointindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->tmp->prevpointindices);
  }

  auto res = PRIVATE(this)->tmp->vhash.insert(
      std::make_pair(v, PRIVATE(this)->vertexarray.getLength()));
  if (res.second) {
    PRIVATE(this)->addVertex(v);
    // update texture coordinates for unit 1-n
    for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
      if (v.texcoordidx >= 0 &&
          (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
        PRIVATE(this)->multitexarray[j].append(PRIVATE(this)->tmp->multielem->get4(j, v.texcoordidx));
      }
      else if (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
        PRIVATE(this)->multitexarray[j].append(PRIVATE(this)->tmp->multielem->get(j, v.vertex, v.normal));
      }
      else {
        PRIVATE(this)->multitexarray[j].append(v.texcoord0);
      }
    }
  }
  PRIVATE(this)->pointindexer->addPoint(res.first->second);
  if (v.marker >= 0)
    PRIVATE(this)->markers.append(v.marker);
  ++PRIVATE(this)->tmp->pointindexcount;
}

SoFCVertexCache *
SoFCVertexCache::checkHighlightIndices(int * pindex, bool newcache)
{
  switch(PRIVATE(this)->highlightindices.size()) {
  case 0:
    return this;
  case 1:
    if (PRIVATE(this)->highlightindices[0] < 0)
      return nullptr;
    if (pindex)
      *pindex = PRIVATE(this)->highlightindices[0];
    return this;
  }

  if (!newcache) {
    if (pindex)
      *pindex = -1;
    return this;
  }

  auto cache = new SoFCVertexCache(*this);
  if (PRIVATE(this)->triangleindexer)
    cache->addTriangles(PRIVATE(this)->highlightindices.getData());
  else if (PRIVATE(this)->lineindexer)
    cache->addLines(PRIVATE(this)->highlightindices.getData());
  else if (PRIVATE(this)->pointindexer)
    cache->addPoints(PRIVATE(this)->highlightindices.getData());
  return cache;
}

int
SoFCVertexCache::getNumVertices(void) const
{
  return PRIVATE(this)->vertexarray ? PRIVATE(this)->vertexarray.getLength() : 0;
}

const SbVec3f *
SoFCVertexCache::getVertexArray(void) const
{
  return PRIVATE(this)->vertexarray ? PRIVATE(this)->vertexarray.getArrayPtr() : NULL;
}

const SbVec3f *
SoFCVertexCache::getNormalArray(void) const
{
  return PRIVATE(this)->normalarray ? PRIVATE(this)->normalarray.getArrayPtr() : NULL;
}

const SbVec4f *
SoFCVertexCache::getTexCoordArray(void) const
{
  return PRIVATE(this)->texcoord0array ? PRIVATE(this)->texcoord0array.getArrayPtr() : NULL;
}

const SbVec2f *
SoFCVertexCache::getBumpCoordArray(void) const
{
  return PRIVATE(this)->bumpcoordarray ? PRIVATE(this)->bumpcoordarray.getArrayPtr() : NULL;
}

const uint8_t *
SoFCVertexCache::getColorArray(void) const
{
  return PRIVATE(this)->colorarray ? PRIVATE(this)->colorarray.getArrayPtr() : NULL;
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

int
SoFCVertexCache::hasSolid() const
{
  return PRIVATE(this)->hassolid;
}

bool
SoFCVertexCache::hasFlipNormal() const
{
  return PRIVATE(this)->flipnormal;
}

const SbVec4f *
SoFCVertexCache::getMultiTextureCoordinateArray(const int unit) const
{
  assert(unit <= PRIVATE(this)->lastenabled);
  if (!unit)
    return PRIVATE(this)->texcoord0array ? PRIVATE(this)->texcoord0array.getArrayPtr() : NULL;

  if (unit < (int)PRIVATE(this)->multitexarray.size()
      && PRIVATE(this)->multitexarray[unit]
      && PRIVATE(this)->multitexarray[unit].getArrayPtr())
    return PRIVATE(this)->multitexarray[unit].getArrayPtr();

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
SoFCVertexCacheP::depthSortTriangles(SoState * state, bool fullsort, const SbPlane *plane)
{
  if (!this->vertexarray) return FALSE;
  int numv = this->vertexarray.getLength();
  int numtri = PUBLIC(this)->getNumTriangleIndices() / 3;
  if (numv == 0 || numtri == 0) return FALSE;

  int numparts = this->triangleindexer->getNumParts();

  // must not mess up indices if there are parts
  if (numparts) {
    // Treat one part as if there is no part, so that we can sort the
    // triangles, e.g. a sphere. It's kind of strange that sphere doesn't
    // render correctly when there is shadow without sorting.
    //
    // TODO: find out why.
    if (numparts == 1)
      numparts = 0;
    if (numparts && !fullsort) {
      if (this->numtranspparts == numparts)
        fullsort = true;
      else {
        numparts = this->numtranspparts;
        if (!numparts)
          return FALSE;
      }
    }
  }

  SbPlane sortplane = plane?*plane:SoViewVolumeElement::get(state).getPlane(0.0);
  // move plane into object space
  sortplane.transform(SoModelMatrixElement::get(state).inverse());

  const SbVec3f * vptr = this->vertexarray.getArrayPtr();

  // If having parts, sort the parts (i.e. group of triangles) instead of
  // individual triangles
  if (numparts) {
    if (numparts == (int)this->deptharray.size()
        && sortplane.getNormal() == this->prevsortplane.getNormal())
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
    else if (this->triangleindexer->getPartialIndices().size()) {
      auto it = this->transppartindices.begin();
      auto itEnd = this->transppartindices.end();
      if (it != itEnd) {
        for (int i : this->triangleindexer->getPartialIndices()) {
          if (i < *it)
            continue;
          if (i == *it)
            this->deptharray.emplace_back(i);
          if (++it == itEnd)
            break;
        }
      }
    } else {
      for (int i : this->transppartindices)
        this->deptharray.emplace_back(i);
    }

    this->prevsortplane = sortplane;
    const int *parts = this->triangleindexer->getPartOffsets();
#ifdef FC_RENDER_SORT_NEAREST
    const GLint *indices = this->triangleindexer->getIndices();
    const SbVec3f *vertices = this->vertexarray.getArrayPtr();
    for(auto & entry : this->deptharray) {
      int prev = entry.index == 0 ? 0 : parts[entry.index-1];
      int n = parts[entry.index]-prev;
      entry.depth = -FLT_MAX;
      for (int k=0; k<n; ++k) {
        const SbVec3f & vertex = vertices[indices[k+prev]];
        float d = sortplane.getDistance(vertex);
        if (d > entry.depth)
          entry.depth = d;
      }
    }
#else
    for(auto & entry : this->deptharray)
      entry.depth = sortplane.getDistance(this->partcenters[entry.index]);
#endif

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
  if (numtri == (int)deptharray.size()
      && sortplane.getNormal() == this->prevsortplane.getNormal())
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
SoFCVertexCacheP::initColor(int n)
{
  uint8_t r,g,b,a;
  r = (this->firstcolor >> 24) & 0xff;
  g = (this->firstcolor >> 16) & 0xff;
  b = (this->firstcolor >> 8) & 0xff;
  a = (this->firstcolor) & 0xff;
  for (int i=0; i<n; i+=4) {
    this->colorarray.append(r);
    this->colorarray.append(g);
    this->colorarray.append(b);
    this->colorarray.append(a);
  }
}

void
SoFCVertexCacheP::addVertex(const Vertex & v)
{
  this->vertexarray.append(v.vertex);
  this->normalarray.append(v.normal);
  if (this->lastenabled >= 0) {
    this->texcoord0array.append(v.texcoord0);
    this->bumpcoordarray.append(v.bumpcoord);
  }

  if (this->colorpervertex > 0) {
    if (!this->colorarray)
      initColor(this->vertexarray.getLength()*4 - 4);
    uint8_t r,g,b,a;
    r = (v.color >> 24) & 0xff;
    g = (v.color >> 16) & 0xff;
    b = (v.color >> 8) & 0xff;
    a = (v.color) & 0xff;
    this->colorarray.append(r);
    this->colorarray.append(g);
    this->colorarray.append(b);
    this->colorarray.append(a);
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
                             this->colorarray.getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
  }

  if (texture && this->texcoord0array) {
    cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0,
                                this->texcoord0array.getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

    for (i = 1; i <= lastenabled; i++) {
      if (enabled[i] && this->multitexarray[i]) {
        cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
        cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0,
                                    this->multitexarray[i].getArrayPtr());
        cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
      }
    }
  }
  if (normal && this->normalarray) {
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0,
                              this->normalarray.getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }

  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0,
                            this->vertexarray.getArrayPtr());
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
    this->colorarray.bindBuffer(state, contextid);
    cc_glglue_glColorPointer(glue, 4, GL_UNSIGNED_BYTE, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
  }
  if (texture && this->texcoord0array) {
    this->texcoord0array.bindBuffer(state, contextid);
    cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

    for (i = 1; i <= lastenabled; i++) {
      if (!enabled[i] || !this->multitexarray[i]) continue;
      this->multitexarray[i].bindBuffer(state, contextid);
      cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
      cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0, NULL);
      cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
    }
  }
  if (normal && this->normalarray) {
    this->normalarray.bindBuffer(state, contextid);
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }

  this->vertexarray.bindBuffer(state, contextid);
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
    colorptr = this->colorarray.getArrayPtr();
  }
  if (normal && this->normalarray) {
    normalptr = this->normalarray.getArrayPtr();
  }
  if (texture && this->texcoord0array) {
    texcoordptr = this->texcoord0array.getArrayPtr();
  }
  vertexptr = this->vertexarray.getArrayPtr();

  for (int i = 0; i < numindices; i++) {
    const int idx = indices[i];
    if (normalptr) {
      glNormal3fv(reinterpret_cast<const GLfloat *>(&normalptr[idx]));
    }
    if (colorptr) {
      glColor4ubv(reinterpret_cast<const GLubyte *>(&colorptr[idx*4]));
    }
    if (texcoordptr) {
      glTexCoord4fv(reinterpret_cast<const GLfloat *>(&texcoordptr[idx]));

      for (int j = 1; j <= lastenabled; j++) {
        if (!enabled[j] || !this->multitexarray[j]) continue;
        const SbVec4f * mt = this->multitexarray[j].getArrayPtr();
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
    PRIVATE(this)->getBoundingBox(nullptr, PRIVATE(this)->boundbox);
  return PRIVATE(this)->boundbox;
}

void
SoFCVertexCache::getBoundingBox(const SbMatrix * matrix, SbBox3f & bbox) const
{
  if (PRIVATE(this)->boundbox.isEmpty())
    PRIVATE(this)->getBoundingBox(nullptr, PRIVATE(this)->boundbox);
  bbox = PRIVATE(this)->boundbox;
  if (matrix)
    bbox.transform(*matrix);
}

void
SoFCVertexCacheP::getBoundingBox(const SbMatrix * matrix, SbBox3f & bbox) const
{
  const SbVec3f *vptr = PUBLIC(this)->getVertexArray();
  if (this->prevattached) {
    // means partial indexing, we need to explicitly iterate over indices
    if (this->triangleindexer)
      this->triangleindexer->getBoundingBox(matrix, bbox, vptr);
    if (this->lineindexer)
      this->lineindexer->getBoundingBox(matrix, bbox, vptr);
    if (this->pointindexer)
      this->pointindexer->getBoundingBox(matrix, bbox, vptr);
    return;
  }

  int num = PUBLIC(this)->getNumVertices();
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

void
SoFCVertexCacheP::getBoundingBox(const SbMatrix * matrix,
                                 SbBox3f & bbox,
                                 const SoFCVertexArrayIndexer *indexer,
                                 int part) const
{
  const SbVec3f *vptr = PUBLIC(this)->getVertexArray();
  if (part < 0 || this->prevattached)
    return indexer->getBoundingBox(matrix, bbox, vptr);

  const int * indices = indexer->getIndices();
  int numindices = indexer->getNumIndices();
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
    if (part >= 0 && part < numindices) {
      SbVec3f v;
      int i = indices[part];
      if (matrix)
        matrix->multVecMatrix(vptr[i], v);
      else
        v = vptr[i];
      bbox.extendBy(v);
      return;
    }
  }
  if (part >= numparts)
    return;

  const GLint * parts = indexer->getPartOffsets();
  for (int i=part?parts[part-1]:0; i<parts[part]; ++i) {
    SbVec3f v;
    if (matrix)
      matrix->multVecMatrix(vptr[indices[i]], v);
    else
      v = vptr[indices[i]];
    bbox.extendBy(v);
  }
}

void
SoFCVertexCache::getTrianglesBoundingBox(const SbMatrix * matrix,
                                        SbBox3f & bbox,
                                        int part) const
{
  PRIVATE(this)->getBoundingBox(matrix, bbox, PRIVATE(this)->triangleindexer, part);
}

void
SoFCVertexCache::getLinesBoundingBox(const SbMatrix * matrix,
                                        SbBox3f & bbox,
                                        int part) const
{
  PRIVATE(this)->getBoundingBox(matrix, bbox, PRIVATE(this)->lineindexer, part);
}

void
SoFCVertexCache::getPointsBoundingBox(const SbMatrix * matrix,
                                       SbBox3f & bbox,
                                       int part) const
{
  PRIVATE(this)->getBoundingBox(matrix, bbox, PRIVATE(this)->pointindexer, part);
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
  if (!indexer || part < 0 || this->colorpervertex>0 || !this->colorarray)
    return color;

  const uint8_t * colors = this->colorarray.getArrayPtr();
  int colorlen = this->colorarray.getLength();
  const int * indices = indexer->getIndices();
  int numindices = indexer->getNumIndices();

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
    if (part < 0 || part >= numindices)
      return color;

    int idx = indices[part]*4;
    if (idx >= 0 && idx+3 < colorlen) {
      colors += idx;
      color = colors[0] << 24;
      color |= colors[1] << 16;
      color |= colors[2] << 8;
      color |= colors[3];
    }
    return color;
  }
  if (part >= numparts)
    return color;

  const GLint * parts = indexer->getPartOffsets();
  int idx = indices[part ? parts[part-1] : 0] * 4;
  if (idx >= 0 && idx+3 < colorlen) {
    colors += idx;
    color = colors[0] << 24;
    color |= colors[1] << 16;
    color |= colors[2] << 8;
    color |= colors[3];
  }
  return color;
}

SoFCVertexCache *
SoFCVertexCache::getWholeCache() const
{
  if (PRIVATE(this)->prevattached)
    return PRIVATE(this)->prevcache;
  return const_cast<SoFCVertexCache*>(this);
}

bool
SoFCVertexCacheP::canMerge(const VertexCacheEntry & entry)
{
  if (entry.partidx >= 0 || entry.resetmatrix)
    return false;
  auto self = PRIVATE(entry.cache);
  if (!self->vertexarray)
    return false;
  if (!self->triangleindexer
      && !self->lineindexer
      && !self->pointindexer)
    return false;
  if (self->triangleindexer
      && self->triangleindexer->getPartialIndices().size())
      return false;
  if (self->lineindexer
      && self->lineindexer->getPartialIndices().size())
    return false;
  if (self->pointindexer
      && self->pointindexer->getPartialIndices().size())
    return false;
  return true;
}

bool
SoFCVertexCacheP::canMergeWith(const VertexCacheEntry & other_entry)
{
  auto other = PRIVATE(other_entry.cache);
  if (!other->canMerge(other_entry))
    return false;

  if (this->triangleindexer) {
    if (!other->triangleindexer
        || (!!this->triangleindexer->getNumParts() != !!other->triangleindexer->getNumParts()))
      return false;
  } else if (other->triangleindexer)
    return false;

  if (this->lineindexer) {
     if (!other->lineindexer
        || (!!this->lineindexer->getNumParts() != !!other->lineindexer->getNumParts()))
       return false;
  } else if (other->lineindexer)
    return false;

  if ((this->pointindexer && !other->pointindexer)
      || (!this->pointindexer && other->pointindexer))
    return false;

  if (other->pointindexer && other->pointindexer->getPartialIndices().size())
    return false;

  if (!this->vertexarray || !other->vertexarray)
    return false;

  if ((!this->normalarray && other->normalarray)
      || (this->normalarray && !other->normalarray))
    return false;

  if ((!this->texcoord0array && other->texcoord0array)
      || (this->texcoord0array && !other->texcoord0array))
    return false;

  if ((!this->bumpcoordarray && other->bumpcoordarray)
      || (this->bumpcoordarray && !other->bumpcoordarray))
    return false;

  if ((!this->colorarray && other->colorarray)
      || (this->colorarray && !other->colorarray))
    return false;

  if (this->multitexarray.size() != other->multitexarray.size())
    return false;

  int i = -1;
  for (auto & entry : this->multitexarray) {
    ++i;
    if ((entry && !other->multitexarray[i]) || (!entry && other->multitexarray[i]))
      return false;
  }

  return true;
}

int
SoFCVertexCache::getMergeId() const
{
  return PRIVATE(this)->mergeid;
}

SbFCUniqueId
SoFCVertexCache::getCacheId() const
{
  return PRIVATE(this)->cacheid;
}

void
SoFCVertexCache::MergeMap::cleanup()
{
  ++this->mergeid;
  int count = 0;
  for (auto it=this->map.begin(); it!=this->map.end();) {
    if (it->second->getMergeId() != this->mergeid) {
      it = this->map.erase(it);
      ++count;
    } else
      ++it;
  }

  FC_TRACE("discard " << count << " merged caches");
}

VertexCachePtr
SoFCVertexCache::merge(bool allownewmerge,
                       std::shared_ptr<MergeMap> & mergemap,
                       SoFCRenderCache::VertexCacheArray & entries,
                       int idx,
                       int & mergecount)
{
  assert(this == entries[idx].cache);

  if (!allownewmerge && !mergemap)
    return nullptr;

  if (!PRIVATE(this)->canMerge(entries[idx]))
    return nullptr;

  SbFCVector<SbFCUniqueId> mergeids;
  mergecount = 0;
  int i = idx + entries[idx].mergecount + 1;
  int maxcount = ViewParams::getRenderCacheMergeCountMax();
  if (maxcount && maxcount < ViewParams::getRenderCacheMergeCount())
    maxcount = ViewParams::getRenderCacheMergeCount();

  int mcount = 1;
  for (int c=(int)entries.size(); i<c; ++i) {
    if (!PRIVATE(this)->canMergeWith(entries[i])) {
      if (!mergecount)
        return nullptr;
      break;
    }
    mergeids.push_back(PRIVATE(entries[i].cache)->cacheid);
    ++entries[i].skipcount;
    mergecount += entries[i].mergecount + 1;
    i += entries[i].mergecount;

    ++mcount;
    if (maxcount && mcount >= maxcount)
      break;
  }
  mergecount += entries[idx].mergecount + 1;
  ++entries[idx].skipcount;
  mergeids.push_back(PRIVATE(this)->cacheid);

  if (!allownewmerge) {
    if (!mergemap)
      return nullptr;
    auto it = mergemap->map.find(mergeids);
    if (it == mergemap->map.end())
      return nullptr;
    FC_TRACE("found merged cache " << mergecount);
    return it->second;
  }

  if (!mergemap)
    mergemap = std::allocate_shared<MergeMap>(SoFCAllocator<MergeMap>());

  auto & vcache = mergemap->map[mergeids];
  if (vcache) {
    PRIVATE(vcache)->mergeid = mergemap->mergeid + 1;
    FC_TRACE("reuse merged cache " << mergecount);
    return vcache;
  }

  vcache = new SoFCVertexCache(*const_cast<SoFCVertexCache*>(this));
  PRIVATE(vcache)->mergeid = mergemap->mergeid + 1;
  bool first = true;
  for (; idx<i; idx+=entries[idx].mergecount+1) {
    auto & entry = entries[idx];
    PRIVATE(entry.cache)->mergeTo(first, vcache, entry.matrix, entry.identity);
    first = false;
  }

  PRIVATE(vcache)->finalizeTriangleIndexer();
  if (PRIVATE(vcache)->lineindexer)
    PRIVATE(vcache)->lineindexer->sort_lines();

  FC_TRACE("new merged cache " << mergecount);
  return vcache;
}

void
SoFCVertexCacheP::mergeTo(bool first,
                          SoFCVertexCache *vcache,
                          const SbMatrix & matrix,
                          bool identity) const
{
  auto pvcache = PRIVATE(vcache);
  if (first) {
    if (!identity) {
      auto * vertices = pvcache->vertexarray.getWritableArrayPtr();
      for (int i=0, c=pvcache->vertexarray.getLength(); i<c; ++i)
        matrix.multVecMatrix(vertices[i], vertices[i]);

      if (pvcache->normalarray) {
        auto *normals = pvcache->normalarray.getWritableArrayPtr();
        SbVec3f origin(0,0,0);
        matrix.multVecMatrix(origin, origin);
        for (int i=0, c=pvcache->normalarray.getLength(); i<c; ++i) {
          matrix.multVecMatrix(normals[i], normals[i]);
          normals[i] -= origin;
          normals[i].normalize();
        }
      }
    }

    if (this->triangleindexer)
      vcache->addTriangles();
    if (this->lineindexer)
      vcache->addLines();
    if (this->pointindexer)
      vcache->addPoints();
    return;
  }

  int offset = pvcache->vertexarray.getLength();
  if (identity)
    pvcache->vertexarray.append(this->vertexarray);
  else {
    for (int i=0, c=this->vertexarray.getLength(); i<c; ++i) {
      SbVec3f vec;
      matrix.multVecMatrix(this->vertexarray[i], vec);
      pvcache->vertexarray.append(vec);
    }
  }
  if (pvcache->normalarray) {
    if (identity)
      pvcache->normalarray.append(this->normalarray);
    else {
      SbVec3f origin(0,0,0);
      matrix.multVecMatrix(origin, origin);
      for (int i=0, c=this->normalarray.getLength(); i<c; ++i) {
        SbVec3f vec;
        matrix.multVecMatrix(this->normalarray[i], vec);
        vec -= origin;
        vec.normalize();
        pvcache->normalarray.append(vec);
      }
    }
  }
  if (pvcache->texcoord0array)
    pvcache->texcoord0array.append(this->texcoord0array);
  if (pvcache->bumpcoordarray)
    pvcache->bumpcoordarray.append(this->bumpcoordarray);
  if (pvcache->colorarray)
    pvcache->colorarray.append(this->colorarray);
  int i = -1;
  for (auto & entry : pvcache->multitexarray) {
    ++i;
    if (entry)
      entry.append(this->multitexarray[i]);
  }

  if (pvcache->triangleindexer) {
    int numparts = pvcache->triangleindexer->getNumParts();
    int idxoffset = pvcache->triangleindexer->getNumIndices();
    pvcache->triangleindexer->append(this->triangleindexer, offset);

    if (this->hastransp)
      pvcache->hastransp = true;

    if (pvcache->hassolid > 1 && this->hassolid <= 1) {
      pvcache->hassolid = 1;
      for (int i=0; i<numparts; ++i)
        pvcache->solidpartindices.append(i);
      for (int i : this->solidpartindices)
        pvcache->solidpartindices.append(i+idxoffset);
    }
    else if (pvcache->hassolid == 1 && this->hassolid > 1) {
      for (int i=0, c=this->triangleindexer->getNumParts(); i<c; ++i)
        pvcache->solidpartindices.append(i+idxoffset);
    }
    else if (this->hassolid == 1) {
      pvcache->hassolid = 1;
      for (int i : this->solidpartindices)
        pvcache->solidpartindices.append(i+idxoffset);
    }
  }
  if (pvcache->lineindexer)
    pvcache->lineindexer->append(this->lineindexer, offset);
  if (pvcache->pointindexer)
    pvcache->pointindexer->append(this->pointindexer, offset);
}

#undef PRIVATE
#undef PUBLIC
// vim: noai:ts=2:sw=2
