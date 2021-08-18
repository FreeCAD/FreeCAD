/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef FC_RENDERCACHE_H
#define FC_RENDERCACHE_H

#include <vector>
#include <map>
#include <memory>
#include <bitset>

#include <boost/container/flat_map.hpp>

#include <Inventor/SbMatrix.h>
#include <Inventor/caches/SoCache.h>

#include "../SoFCUnifiedSelection.h"
#include "COWData.h"
#include "SoFCRenderCacheManager.h"
#include "SoAutoZoomTranslation.h"

class SoFCVertexCache;
class SoFCRenderCacheP;
class SoState;
class SoTexture;
class SoLight;
class SoLightModel;
class SoMaterial;
class SoVRMLMaterial;
class SoDepthBuffer;
class SbBox3f;
class SoClipPlane;
class SoMFColor;

class GuiExport SoFCRenderCache : public SoCache {
  typedef SoCache inherited;
public:

  struct MatrixInfo {
    SbMatrix matrix;

    void combine(const MatrixInfo & other) {
      this->matrix = this->matrix.multLeft(other.matrix);
    }

    int compare(const MatrixInfo & other) const {
      for (int i=0; i<16; ++i) {
        if (this->matrix[i] < other.matrix[i]) return -1;
        if (this->matrix[i] > other.matrix[i]) return 1;
      }
      return 0;
    }

    bool operator==(const MatrixInfo &other) const {
      return compare(other) == 0;
    }

    bool operator!=(const MatrixInfo &other) const {
      return compare(other) != 0;
    }

    bool operator<(const MatrixInfo &other) const {
      return compare(other) < 0;
    }

    bool operator>(const MatrixInfo &other) const {
      return compare(other) > 0;
    }
  };

  typedef COWMap<int, MatrixInfo> TextureMatrixMap;

  struct TextureInfo {
    Gui::CoinPtr<SoNode> texture;
    SbMatrix matrix;
    bool identity = true;
    bool transparent = false;

    int compare(const TextureInfo & other) const {
      if (this->texture < other.texture) return -1;
      if (this->texture > other.texture) return 1;
      if (this->transparent < other.transparent) return -1;
      if (this->transparent > other.transparent) return 1;
      if (this->identity < other.identity) return -1;
      if (this->identity > other.identity) return 1;
      if (!this->identity) {
        for (int i=0; i<16; ++i) {
          if (this->matrix[i] < other.matrix[i]) return -1;
          if (this->matrix[i] > other.matrix[i]) return 1;
        }
      }
      return 0;
    }

    bool operator==(const TextureInfo &other) const {
      return compare(other) == 0;
    }

    bool operator!=(const TextureInfo &other) const {
      return compare(other) != 0;
    }

    bool operator<(const TextureInfo &other) const {
      return compare(other) < 0;
    }

    bool operator>(const TextureInfo &other) const {
      return compare(other) > 0;
    }
  };

  typedef COWMap<int, TextureInfo> TextureMap;

  struct NodeInfo {
    Gui::CoinPtr<SoNode> node;
    SbMatrix matrix;
    bool identity = true;
    bool resetmatrix = false;;

    int compare(const NodeInfo & other) const {
      if (this->node < other.node) return -1;
      if (this->node > other.node) return 1;
      if (this->identity < other.identity) return -1;
      if (this->identity > other.identity) return 1;
      if (!this->identity) {
        for (int i=0; i<16; ++i) {
          if (this->matrix[i] < other.matrix[i]) return -1;
          if (this->matrix[i] > other.matrix[i]) return 1;
        }
      }
      return 0;
    }

    bool operator==(const NodeInfo &other) const {
      return compare(other) == 0;
    }

    bool operator!=(const NodeInfo &other) const {
      return compare(other) != 0;
    }

    bool operator<(const NodeInfo &other) const {
      return compare(other) < 0;
    }

    bool operator>(const NodeInfo &other) const {
      return compare(other) > 0;
    }

    template <class T>
    T *cast() {
      assert(node && node->isOfType(T::getClassTypeId()));
      return static_cast<T*>(node.get());
    }

    template <class T>
    const T *cast() const {
      assert(node && node->isOfType(T::getClassTypeId()));
      return static_cast<T*>(node.get());
    }
  };

  typedef COWVector<NodeInfo> NodeInfoArray;

  struct Material {
    enum Type {
      Triangle,
      Line,
      Point,
    };
    enum SelectStyle {
      Full,
      Box,
      Unpickable,
    };
    enum FlagBits {
      FLAG_AMBIENT,
      FLAG_DIFFUSE,
      FLAG_DRAW_STYLE,
      FLAG_EMISSIVE,
      FLAG_LIGHT_MODEL,
      FLAG_LINE_PATTERN,
      FLAG_LINE_WIDTH,
      FLAG_MATERIAL_BINDING,
      FLAG_POINT_SIZE,
      FLAG_SHAPE_HINTS,
      FLAG_SHININESS,
      FLAG_SPECULAR,
      FLAG_TRANSPARENCY,
      FLAG_VERTEXORDERING,
      FLAG_TWOSIDE,
      FLAG_CULLING,
      FLAG_SHADE_MODEL,
      FLAG_SHADOW_STYLE,
      FLAG_DEPTH_TEST,
      FLAG_DEPTH_WRITE,
      FLAG_DEPTH_FUNC,
      FLAG_POLYGON_OFFSET,
    };

    int32_t order;
    std::bitset<32> overrideflags;
    std::bitset<32> maskflags;
    uint32_t diffuse;
    uint32_t ambient;
    uint32_t emissive;
    uint32_t specular;
    uint32_t linepattern;
    uint32_t hiddenlinecolor;
    float linewidth;
    float pointsize;
    float shininess;
    float polygonoffsetunits;
    float polygonoffsetfactor;
    int16_t annotation;
    int8_t type;
    int8_t lightmodel;
    int8_t materialbinding;
    int8_t vertexordering;
    int8_t drawstyle;
    int8_t polygonoffsetstyle;
    int8_t shadowstyle;
    int8_t depthfunc;
    int8_t partialhighlight;
    int8_t selectstyle;
    int8_t shapetype;
    bool depthtest;
    bool depthwrite;
    bool depthclamp;
    bool transptexture;
    bool pervertexcolor;
    bool culling;
    bool twoside;
    bool outline;
    bool resetclip;

    TextureMatrixMap texturematrices;
    TextureMap textures;
    NodeInfoArray lights;
    NodeInfoArray clippers;
    NodeInfoArray autozoom;

    void init(SoState * state = nullptr);

    bool isOnTop() const {
      return order > 0 || annotation > 0;
    }

    bool hasLinePattern() const {
      return (linepattern & 0xffff) != 0xffff;
    }

    inline bool operator<(const Material &other) const {
      if (order < other.order) return true;
      if (order > other.order) return false;
      if (annotation < other.annotation) return true;
      if (annotation > other.annotation) return false;
      if (clippers < other.clippers) return true;
      if (clippers > other.clippers) return false;
      if (autozoom < other.autozoom) return true;
      if (autozoom > other.autozoom) return false;
      if (type < other.type) return true;
      if (type > other.type) return false;
      if (depthtest < other.depthtest) return true;
      if (depthtest > other.depthtest) return false;
      if (depthclamp < other.depthclamp) return true;
      if (depthclamp > other.depthclamp) return false;
      if (depthfunc < other.depthfunc) return true;
      if (depthfunc > other.depthfunc) return false;
      if (depthwrite < other.depthwrite) return true;
      if (depthwrite > other.depthwrite) return false;
      if (selectstyle < other.selectstyle) return true;
      if (selectstyle > other.selectstyle) return false;
      if (this->type == Triangle) {
        if (shapetype < other.shapetype) return true;
        if (shapetype > other.shapetype) return false;
        if (lights < other.lights) return true;
        if (lights > other.lights) return false;
        if (textures < other.textures) return true;
        if (textures > other.textures) return false;
        if (shadowstyle < other.shadowstyle) return true;
        if (shadowstyle > other.shadowstyle) return false;
        if (diffuse < other.diffuse) return true;
        if (diffuse > other.diffuse) return false;
        if (hiddenlinecolor < other.hiddenlinecolor) return true;
        if (hiddenlinecolor > other.hiddenlinecolor) return false;
        if (pervertexcolor < other.pervertexcolor) return true;
        if (pervertexcolor > other.pervertexcolor) return false;
        if (ambient < other.ambient) return true;
        if (ambient > other.ambient) return false;
        if (emissive < other.emissive) return true;
        if (emissive > other.emissive) return false;
        if (specular < other.specular) return true;
        if (specular > other.specular) return false;
        if (shininess < other.shininess) return true;
        if (shininess > other.shininess) return false;
        if (lightmodel < other.lightmodel) return true;
        if (lightmodel > other.lightmodel) return false;
        if (vertexordering < other.vertexordering) return true;
        if (vertexordering > other.vertexordering) return false;
        if (outline < other.outline) return true;
        if (outline > other.outline) return false;
        if (culling < other.culling) return true;
        if (culling > other.culling) return false;
        if (twoside < other.twoside) return true;
        if (twoside > other.twoside) return false;
        if (drawstyle < other.drawstyle) return true;
        if (drawstyle > other.drawstyle) return false;
        if (polygonoffsetstyle < other.polygonoffsetstyle) return true;
        if (polygonoffsetstyle > other.polygonoffsetstyle) return false;
        if (polygonoffsetfactor < other.polygonoffsetfactor) return true;
        if (polygonoffsetfactor > other.polygonoffsetfactor) return false;
        if (polygonoffsetunits < other.polygonoffsetunits) return true;
        if (polygonoffsetunits > other.polygonoffsetunits) return false;
        if (outline) {
          if (linewidth < other.linewidth) return true;
          if (linewidth > other.linewidth) return false;
          if (pointsize < other.pointsize) return true;
          if (pointsize > other.pointsize) return false;
        }
        // no need to differentiate texture matrices. Its only used for merging to
        // upper hierarchy
        // if (texturematrices < other.texturematrices) return true;
        // if (texturematrices > other.texturematrices) return false;
      } else {
        if (diffuse < other.diffuse) return true;
        if (diffuse > other.diffuse) return false;
        if (pervertexcolor < other.pervertexcolor) return true;
        if (pervertexcolor > other.pervertexcolor) return false;
        if (this->type == Line) {
          if (linewidth < other.linewidth) return true;
          if (linewidth > other.linewidth) return false;
        } else {
          if (pointsize < other.pointsize) return true;
          if (pointsize > other.pointsize) return false;
        }
      }
      return false;
    }
  };


  typedef Gui::SoFCSelectionRoot::NodeKey CacheKey;
  typedef std::shared_ptr<CacheKey> CacheKeyPtr;

  struct CacheKeyHasher {
    std::size_t operator()(const CacheKeyPtr &key) const {
      if (!key)
        return 0;
      return key->hash();
    }
    bool operator()(const CacheKeyPtr &a, const CacheKeyPtr &b) const {
      if (a == b)
        return true;
      if (!a || !b)
        return false;
      return *a == *b;
    }
  };

  typedef std::unordered_set<CacheKeyPtr, CacheKeyHasher, CacheKeyHasher> CacheKeySet;

  struct VertexCacheEntry {
    VertexCacheEntry()
      : mergecount(0)
      , skipcount(0)
      , partidx(-1)
      , identity(true)
      , resetmatrix(false)
    {}

    VertexCacheEntry(SoFCVertexCache * c,
                     const SbMatrix &m,
                     bool iden,
                     bool reset,
                     const CacheKeyPtr &k)
      : key(k)
      , cache(c)
      , mergecount(0)
      , skipcount(0)
      , partidx(-1)
      , identity(iden)
      , resetmatrix(reset)
    {
      if (!iden) matrix = m;
    }

    VertexCacheEntry(SoFCVertexCache * c,
                     const VertexCacheEntry & other,
                     const CacheKeyPtr &k)
      : key(k)
      , cache(c)
      , mergecount(other.mergecount)
      , skipcount(other.skipcount)
      , partidx(other.partidx)
      , identity(other.identity)
      , resetmatrix(other.resetmatrix)
    {
      if (!other.identity)
        matrix = other.matrix;
    }

    VertexCacheEntry(const VertexCacheEntry &other)
      : key(other.key)
      , cache(other.cache)
      , mergecount(other.mergecount)
      , skipcount(other.skipcount)
      , partidx(other.partidx)
      , identity(other.identity)
      , resetmatrix(other.resetmatrix)
    {
      if (!identity)
        this->matrix = other.matrix;
    }

    CacheKeyPtr key;
    Gui::CoinPtr<SoFCVertexCache> cache;
    int mergecount;
    int skipcount;
    int partidx;
    SbMatrix matrix;
    bool identity;
    bool resetmatrix;
  };

  typedef SbFCVector<VertexCacheEntry> VertexCacheArray;

#ifdef _FC_RENDER_MEM_TRACE
  typedef boost::container::flat_map<Material,
                                     VertexCacheArray,
                                     std::less<Material>,
                                     SoFCAllocator<std::pair<Material, VertexCacheArray> > > VertexCacheMap;
#else
  typedef boost::container::flat_map<Material, VertexCacheArray> VertexCacheMap;
#endif

  SoFCRenderCache(SoState * state, SoNode *node, SoFCRenderCache *prev = nullptr);
  virtual ~SoFCRenderCache();

  static void initClass();
  static void cleanup();

  static long getCacheEntryCount();

  SbFCUniqueId getNodeId() const;

  virtual SbBool isValid(const SoState * state) const;

  SbBool isEmpty() const;

  void addTexture(SoState * state, const SoNode * texture);
  void addTextureTransform(SoState * state, const SoNode *);

  void addClipPlane(SoState * state, const SoClipPlane * light);

  void addAutoZoom(SoState * state, const Gui::SoAutoZoomTranslation * node);

  void addLight(SoState * state, const SoNode * light);

  void setLightModel(SoState *state, const SoLightModel *);

  void setMaterial(SoState *state, const SoMaterial *);
  void setMaterial(SoState *state, const SoVRMLMaterial *);
  void setBaseColor(SoState *state, const SoNode *, const SoMFColor &);

  void setDepthBuffer(SoState *state, const SoDepthBuffer *);

  const VertexCacheMap & getVertexCaches(bool canmerge, int depth=0);

  enum HighlightFlag {
    PreselectHighlight = 1,
    CheckIndices = 2,
    WholeOnTop = 4,
  };
  VertexCacheMap buildHighlightCache(SbFCMap<int, Gui::CoinPtr<SoFCVertexCache> > &sharedcache,
                                     int order,
                                     const SoDetail * detail,
                                     uint32_t color,
                                     int flags = 0);

  void open(SoState * state,
            int selectstyle = Material::Full,
            bool initmaterial = true);
  void close(SoState * state);

  void resetNode();

  void beginChildCaching(SoState * state, SoFCRenderCache * cache);
  void beginChildCaching(SoState * state, SoFCVertexCache * cache);

  void endChildCaching(SoState * state, SoFCRenderCache * cache);
  void endChildCaching(SoState * state, SoFCVertexCache * vcache);

  void addChildCache(SoState * state, SoFCRenderCache * cache);
  void addChildCache(SoState * state, SoFCVertexCache * cache);

  void increaseRenderingOrder(SoState *state, int priority=0);
  void decreaseRenderingOrder(SoState *state, int priority=0);

  const char * getRenderStatistics() const;

  void resetMatrix(SoState *state);

  const SbBox3f & getBoundingBox() const;

private:
  friend class SoFCRenderCacheP;
  SoFCRenderCacheP * pimpl;

  SoFCRenderCache(const SoFCRenderCache & rhs); // N/A
  SoFCRenderCache & operator = (const SoFCRenderCache & rhs); // N/A

  static long CacheEntryCount;
  static long CacheEntryFreeCount;
};

// support for CoinPtr<SoFCRenderCache>
inline void intrusive_ptr_add_ref(SoFCRenderCache * obj) { obj->ref(); }
inline void intrusive_ptr_release(SoFCRenderCache * obj) { obj->unref(); }

#endif // FC_RENDERCACHE_H
// vim: noai:ts=2:sw=2
