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

#include <Inventor/SbMatrix.h>
#include <Inventor/caches/SoCache.h>
#include "COWData.h"
#include "SoFCVertexCache.h"
#include "SoFCRenderCacheManager.h"

class SoFCRenderCacheP;
class SoState;
class SoTexture;
class SoLight;
class SoLightModel;
class SoMaterial;
class SoDepthBuffer;
class SbBox3f;

// -------------------------------------------------------------

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

  typedef COWMap<std::map<int, MatrixInfo> > TextureMatrixMap;

  struct TextureInfo {
    Gui::CoinPtr<SoTexture> texture;
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

  typedef COWMap<std::map<int, TextureInfo> > TextureMap;

  struct LightInfo {
    Gui::CoinPtr<SoLight> light;
    SbMatrix matrix;
    bool identity;
    bool resetmatrix;

    int compare(const LightInfo & other) const {
      if (this->light < other.light) return -1;
      if (this->light > other.light) return 1;
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

    bool operator==(const LightInfo &other) const {
      return compare(other) == 0;
    }

    bool operator!=(const LightInfo &other) const {
      return compare(other) != 0;
    }

    bool operator<(const LightInfo &other) const {
      return compare(other) < 0;
    }

    bool operator>(const LightInfo &other) const {
      return compare(other) > 0;
    }
  };
  typedef COWVector<std::vector<LightInfo> > LightArray;

  struct Material {
    enum Type {
      Triangle,
      Line,
      Point,
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
    bool depthtest;
    bool depthwrite;
    bool transptexture;
    bool pervertexcolor;
    bool culling;
    bool twoside;
    bool selectable;

    TextureMatrixMap texturematrices;
    TextureMap textures;
    LightArray lights;

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
      if (type < other.type) return true;
      if (type > other.type) return false;
      if (depthtest < other.depthtest) return true;
      if (depthtest > other.depthtest) return false;
      if (depthfunc < other.depthfunc) return true;
      if (depthfunc > other.depthfunc) return false;
      if (depthwrite < other.depthwrite) return true;
      if (depthwrite > other.depthwrite) return false;
      if (selectable < other.selectable) return true;
      if (selectable > other.selectable) return false;
      if (this->type == Triangle) {
        if (lights < other.lights) return true;
        if (lights > other.lights) return false;
        if (textures < other.textures) return true;
        if (textures > other.textures) return false;
        if (shadowstyle < other.shadowstyle) return true;
        if (shadowstyle > other.shadowstyle) return false;
        if (diffuse < other.diffuse) return true;
        if (diffuse > other.diffuse) return false;
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

  typedef std::vector<intptr_t> CacheKey;
  typedef std::shared_ptr<CacheKey> CacheKeyPtr;

  struct VertexCacheEntry {
    VertexCacheEntry(SoFCVertexCache * c,
                     const SbMatrix &m,
                     bool iden,
                     bool reset,
                     CacheKeyPtr k)
      : key(k)
      , cache(c)
      , partidx(-1)
      , identity(iden)
      , resetmatrix(reset)
    {
      if (!iden) matrix = m;
    }

    CacheKeyPtr key;
    Gui::CoinPtr<SoFCVertexCache> cache;
    int partidx;
    SbMatrix matrix;
    bool identity;
    bool resetmatrix;
  };

  typedef std::map<Material, std::vector<VertexCacheEntry> > VertexCacheMap;

  SoFCRenderCache(SoState * state, intptr_t nodeptr, SbFCUniqueId nodeid);
  virtual ~SoFCRenderCache();

  static void initClass();
  static void cleanup();

  SbFCUniqueId getNodeId() const;

  virtual SbBool isValid(const SoState * state) const;

  void addTexture(SoState * state, const SoTexture * texture);
  void addTextureTransform(SoState * state, const SoNode *);

  void addLight(SoState * state, const SoLight * light);

  void setLightModel(SoState *state, const SoLightModel *);

  void setMaterial(SoState *state, const SoMaterial *);

  void setDepthBuffer(SoState *state, const SoDepthBuffer *);

  const VertexCacheMap & getVertexCaches(bool finalize=false);

  VertexCacheMap buildHighlightCache(int order,
                                     const SoDetail * detail,
                                     uint32_t color,
                                     bool checkindices,
                                     bool wholeontop = true);

  void open(SoState * state, bool selectable, bool initmaterial = false);
  void close(SoState * state);

  void beginChildCaching(SoState * state, SoFCRenderCache * cache);
  void beginChildCaching(SoState * state, SoFCVertexCache * cache);

  void endChildCaching(SoState * state, SoCache * cache);

  void addChildCache(SoState * state, SoFCRenderCache * cache);
  void addChildCache(SoState * state, SoFCVertexCache * cache);

  void increaseRenderingOrder();
  void decreaseRenderingOrder();

  void resetMatrix();

  const SbBox3f & getBoundingBox() const;

private:
  friend class SoFCRenderCacheP;
  SoFCRenderCacheP * pimpl;

  SoFCRenderCache(const SoFCRenderCache & rhs); // N/A
  SoFCRenderCache & operator = (const SoFCRenderCache & rhs); // N/A
};

// support for CoinPtr<SoFCRenderCache>
inline void intrusive_ptr_add_ref(SoFCRenderCache * obj) { obj->ref(); }
inline void intrusive_ptr_release(SoFCRenderCache * obj) { obj->unref(); }

#endif // FC_RENDERCACHE_H
// vim: noai:ts=2:sw=2
