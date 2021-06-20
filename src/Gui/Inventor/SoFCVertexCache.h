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
#ifndef FC_SOPEVERTEXCACHE_H
#define FC_SOPEVERTEXCACHE_H

#include <set>
#include <map>

#include <Inventor/caches/SoCache.h>
#include <Inventor/system/gl.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbVec4f.h>
#include <Inventor/SbVec2f.h>

#include "../InventorBase.h"

class SoFCVertexCacheP;
class SoPrimitiveVertex;
class SoPointDetail;
class SoState;
class SbVec3f;
class SbBox3f;

class GuiExport SoFCVertexCache : public SoCache {
  typedef SoCache inherited;
public:
  SoFCVertexCache(SoState * state, SoNode * node, SoFCVertexCache *prev=NULL);
  SoFCVertexCache(SoFCVertexCache & prev);
  SoFCVertexCache(const SbBox3f &bbox); // create a cache for rendering a wireframe cube

  virtual ~SoFCVertexCache();

  enum Arrays {
    NORMAL = 0x01,
    TEXCOORD = 0x02,
    COLOR = 0x04,
    SORTED_ARRAY = 0x08,
    FULL_SORTED_ARRAY = 0x10,
    NON_SORTED_ARRAY = 0x20,
    ALL = (NORMAL|TEXCOORD|COLOR),
    ALL_SORTED = (NORMAL|TEXCOORD|COLOR|SORTED_ARRAY),
    NON_SORTED = (NORMAL|TEXCOORD|NON_SORTED_ARRAY),
  };

  static void initClass();
  static void cleanup();

  virtual SbBool isValid(const SoState * state) const;

  void open(SoState * state);
  void close(SoState * state);

  void renderTriangles(SoState * state, const int arrays = ALL, int part = -1, const SbPlane *plane = nullptr);
  void renderLines(SoState * state, const int arrays = ALL, int part = -1, bool noseam = false);
  void renderPoints(SoState * state, const int array = ALL, int part = -1);

  void addTriangles(const std::map<int, int> & faces);
  void addTriangles(const std::set<int> & faces);
  void addTriangles(const std::vector<int> & faces = {});
  void addLines(const std::map<int, int> & lines);
  void addLines(const std::set<int> & lines);
  void addLines(const std::vector<int> & lines = {});
  void addPoints(const std::map<int, int> & points);
  void addPoints(const std::set<int> & points);
  void addPoints(const std::vector<int> & points = {});

  SoFCVertexCache * highlightIndices(int * indices = nullptr);

  void addTriangle(const SoPrimitiveVertex * v0,
                   const SoPrimitiveVertex * v1,
                   const SoPrimitiveVertex * v2,
                   const int * pointdetailidx = NULL);
  void addLine(const SoPrimitiveVertex * v0,
               const SoPrimitiveVertex * v1);
  void addPoint(const SoPrimitiveVertex * v);

  int getNumVertices(void) const;
  const SbVec3f * getVertexArray(void) const;
  const SbVec3f * getNormalArray(void) const;
  const SbVec4f * getTexCoordArray(void) const;
  const SbVec2f * getBumpCoordArray(void) const;
  const uint8_t * getColorArray(void) const;

  void setFaceColors(const std::vector<std::pair<int, uint32_t> > &colors, SbFCUniqueId id);

  int getNumTriangleIndices(void) const;
  const GLint * getTriangleIndices(void) const;
  int32_t getTriangleIndex(const int idx) const;

  SbBool colorPerVertex(void) const;
  SbBool hasTransparency(void) const;
  SbBool hasOpaqueParts(void) const;

  uint32_t getFaceColor(int part) const;
  uint32_t getLineColor(int part) const;
  uint32_t getPointColor(int part) const;

  const SbVec4f * getMultiTextureCoordinateArray(const int unit) const;

  int getNumLineIndices(void) const;
  const GLint * getLineIndices(void) const;

  int getNumPointIndices(void) const;
  const GLint * getPointIndices(void) const;

  SoNode *getNode() const;
  void resetNode();
  SbFCUniqueId getNodeId() const;
  SbFCUniqueId getDiffuseId() const;
  SbFCUniqueId getTransparencyId() const;

  SbVec3f getCenter() const;
  const SbBox3f & getBoundingBox() const;
  void getBoundingBox(const SbMatrix * matrix, SbBox3f & bbox) const;

  void getTrianglesBoundingBox(const SbMatrix *matrix, SbBox3f &bbox, int part = -1) const;
  void getLinesBoundingBox(const SbMatrix *matrix, SbBox3f &bbox, int part = -1) const;
  void getPointsBoundingBox(const SbMatrix *matrix, SbBox3f &bbox, int part = -1) const;

  SoFCVertexCache * getWholeCache() const;

  int getNumNonFlatParts(void) const; 
  const int *getNonFlatParts() const;

  int getNumFaceParts() const;

  bool isElementSelectable() const;
  bool allowOnTopPattern() const;

private:
  friend class SoFCVertexCacheP;
  SoFCVertexCacheP * pimpl;

  SoFCVertexCache & operator = (const SoFCVertexCache & rhs) = delete; // N/A

};

// support for CoinPtr
inline void intrusive_ptr_add_ref(SoFCVertexCache * obj) { obj->ref(); }
inline void intrusive_ptr_release(SoFCVertexCache * obj) { obj->unref(); }

#endif // FC_SOVEVERTEXCACHE_H
// vim: noai:ts=2:sw=2
