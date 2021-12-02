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

#include "PreCompiled.h"

#include <Inventor/lists/SoTypeList.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoResetTransform.h>
#include <Inventor/nodes/SoTexture2Transform.h>
#include <Inventor/nodes/SoTexture3Transform.h>
#include <Inventor/nodes/SoTextureMatrixTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/VRMLnodes/SoVRMLMaterial.h>
#include <Inventor/VRMLnodes/SoVRMLTexture.h>
#include <Inventor/VRMLnodes/SoVRMLTextureTransform.h>
#include <Inventor/VRMLnodes/SoVRMLLight.h>
#include <Inventor/VRMLnodes/SoVRMLColor.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoAnnotation.h>
#include <Inventor/sensors/SoDataSensor.h>
#include <Inventor/sensors/SoIdleSensor.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/sensors/SoPathSensor.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/misc/SoTempPath.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/errors/SoDebugError.h>

#include <unordered_map>
#include <map>

#include "../ViewParams.h"
#include "../InventorBase.h"
#include "../SoFCUnifiedSelection.h"
#include "../SoFCSelection.h"

#include "SoFCVertexCache.h"
#include "SoFCRenderCache.h"
#include "SoFCRenderer.h"
#include "SoFCRenderCacheManager.h"

using namespace Gui;

// ---------------------------------------------------------------

typedef CoinPtr<SoPath> PathPtr;
typedef CoinPtr<SoFCVertexCache> VertexCachePtr;
typedef CoinPtr<SoFCRenderCache> RenderCachePtr;
typedef SoFCRenderCache::VertexCacheMap VertexCacheMap;
typedef SoFCRenderCache::Material Material;

// copied from boost::hash_combine. 
template <class T>
static inline void hash_combine(std::size_t& seed, const T& v)
{
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

struct PathHasher {
  std::size_t operator()(const PathPtr & path) const {
    if (!path) return 0;
    std::size_t seed = 0;
    for (int i=0, n=path->getLength(); i<n; ++i) {
      hash_combine(seed, path->getNode(i));
    }
    return seed;
  }

  bool operator()(const PathPtr &a, const PathPtr &b) const {
    if (a == b)
      return true;
    if (!a || !b || a->getLength() != b->getLength())
      return false;
    for (int i=0, c=a->getLength(); i<c; ++i) {
      if (a->getNode(i) != b->getNode(i))
        return false;
    }
    return true;
  }
};

struct ElementEntry {
  ElementEntry()
    : id(0), color(0)
  {}

  std::unique_ptr<SoDetail> detail;
  int id;
  uint32_t color;
  VertexCacheMap vcachemap;
};

class SelectionSensor : public SoNodeSensor {
public:
  SelectionSensor()
    :tmpPath(10)
  {
    tmpPath.ref();
  }

  ~SelectionSensor() {
    attachPath(nullptr);
    tmpPath.unrefNoDelete();
  }

  void attachPath(SoPath *path)
  {
    int adjustment;
    if (path) {
      attachPath(nullptr);
      adjustment = 1;
    } else 
      adjustment = -1;

    if (path) {
      tmpPath.append(path);
      int idx = 0;
      if (path->getLength() > 1
          && path->getHead()->isOfType(SoFCUnifiedSelection::getClassTypeId()))
          ++idx;
      attach(path->getNode(idx));
    } else
      detach();

    attachedPath = path;
    for (int i=0, c=tmpPath.getLength(); i<c; ++i) {
      auto node = tmpPath.getNode(i);
      if (node->isOfType(SoFCSwitch::getClassTypeId())) {
        auto pcSwitch = static_cast<SoFCSwitch*>(node);
        int v = pcSwitch->childNotify.getValue() + adjustment;
        if (v < 0)
          v = 0;
        pcSwitch->childNotify.enableNotify(FALSE);
        pcSwitch->childNotify = v;
        pcSwitch->childNotify.enableNotify(TRUE);
      }
    }
    if (!path)
      tmpPath.truncate(0);
  }

  void refresh(SoFCRenderer * renderer) {
    if (!attachedPath)
      return;
    SoPath *path = attachedPath;
    // rebuild path in case of any transient changes like reordered children
    if (path->getLength() && path->getLength() < tmpPath.getLength()) {
      auto node = tmpPath.getNode(path->getLength()-1);
      for (int i=path->getLength(), c=tmpPath.getLength(); i<c; ++i) {
        auto child = tmpPath.getNode(i);
        auto children = node->getChildren();
        if (!children)
          break;
        bool found = false;
        for (int j=0, n=children->getLength(); j<n; ++j) {
          if ((*children)[j] == child) {
            found = true;
            path->append(j);
            break;
          }
        }
        if (!found)
          break;
        node = child;
      }
    }

    if (this->cache) {
      this->cache.reset();
      for (auto & v : this->elements) {
        renderer->removeSelection(v.second.id);
        v.second.vcachemap.clear();
      }
    }
  }

  SoTempPath tmpPath;
  CoinPtr<SoPath> attachedPath;
  std::unordered_map<std::string, ElementEntry> elements;
  RenderCachePtr cache;
  bool ontop = false;
};

typedef std::unordered_map<PathPtr, SelectionSensor, PathHasher, PathHasher> SelectionPathMap;

class SoFCRenderCacheManagerP {
public:
  SoFCRenderCacheManagerP();
  ~SoFCRenderCacheManagerP();

  void addSelection(const char *key,
                    const SoDetail * detail,
                    uint32_t color,
                    bool ontop,
                    bool alt);

  void initAction();

  static SoCallbackAction::Response preSeparator(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postSeparator(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response preFCSel(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postFCSel(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postSep(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response preAnnotation(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postAnnotation(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response prePathAnnotation(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postPathAnnotation(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response preShape(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postShape(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postClipPlane(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response preAutoZoom(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postAutoZoom(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postLightModel(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postLight(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postVRMLLight(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postMaterial(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postVRMLMaterial(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postColor(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postVRMLColor(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postDepthBuffer(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postResetTransform(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postTextureTransform(void *, SoCallbackAction *action, const SoNode * node);
  static SoCallbackAction::Response postTexture(void *, SoCallbackAction *action, const SoNode * node);
  static void addTriangle(void *,
                          SoCallbackAction * action,
                          const SoPrimitiveVertex * v0,
                          const SoPrimitiveVertex * v1,
                          const SoPrimitiveVertex * v2);
  static void addLine(void *,
                      SoCallbackAction * action,
                      const SoPrimitiveVertex * v0,
                      const SoPrimitiveVertex * v1);
  static void addPoint(void *, SoCallbackAction * action, const SoPrimitiveVertex * v);

  static void updateSelection(void *, SoSensor *sensor);

  class NodeSensor : public SoDataSensor
  {
  public:
    NodeSensor()
      : node(NULL)
    {}

    ~NodeSensor() { detach(); }

    void attach(SoFCRenderCacheManagerP * master, const SoNode *node) {
      (void)master;
      if (this->node == node) return;
      assert(!this->node);
      this->node = const_cast<SoNode *>(node);
      this->node->addAuditor(this, SoNotRec::SENSOR);
    }

    void detach() {
      if (!this->node) return;
      this->node->removeAuditor(this, SoNotRec::SENSOR);
      this->node = NULL;
    }

    SoNode * node;
  };

  class CacheSensor : public NodeSensor
  {
  public:
    virtual void dyingReference(void) {
      SoNode * node = this->node;
      this->detach();
      for (auto &cache : caches)
        cache->resetNode();
      SoFCRenderCacheManagerP::cachetable.erase(node);
    }

    SbFCVector<RenderCachePtr> caches;
  };

  class VCacheSensor : public NodeSensor
  {
  public:
    virtual void dyingReference(void) {
      SoNode * node = this->node;
      this->detach();
      for (auto &vcache : caches)
        vcache->resetNode();
      SoFCRenderCacheManagerP::vcachetable.erase(node);
    }

    SbFCVector<VertexCachePtr> caches;
  };

  class PathCacheSensor : public SoPathSensor
  {
  public:
    PathCacheSensor()
    {
      setFunction([](void *, SoSensor *sensor) {
        auto self = static_cast<PathCacheSensor*>(sensor);
        self->detach();
        if (self->master->highlightcache == self->cache) {
          self->master->highlightcache.reset();
          self->master->renderer->clearHighlight();
        }
        self->master->pathcachetable.erase(self->path);
        return;
      });
    }

    virtual void notify(SoNotList * l) {
#if 1
      (void)l;
#else
      SoBase * firstbase = l->getLastRec()->getBase();
      SoBase * lastbase = l->getFirstRec()->getBase();
      if (lastbase != firstbase)
#endif
      {
        if (!isScheduled())
          schedule();
      }
    }

    RenderCachePtr cache;
    SoFCRenderCacheManagerP *master = nullptr;
    PathPtr path;
  };

  static FC_COIN_THREAD_LOCAL std::unordered_map<const SoNode *, CacheSensor> cachetable;
  static FC_COIN_THREAD_LOCAL std::unordered_map<const SoNode *, VCacheSensor> vcachetable;
  std::unordered_map<PathPtr, PathCacheSensor, PathHasher, PathHasher> pathcachetable;
  RenderCachePtr highlightcache;
  CoinPtr<SoPath> highlightpath;
  bool nosectionontop = false;

  SoCallbackAction *action;
  int shapetypeid;
  VertexCachePtr vcache;

  std::unordered_map<std::string, SelectionPathMap> selcaches;
  SbFCMap<int, CoinPtr<SoPath> > selpaths;

  SbFCMap<int, VertexCachePtr> sharedcache;

  int selid;
  
  SbFCVector<RenderCachePtr> stack;
  SbFCVector<SbFCUniqueId> selnodeid;
  SbFCUniqueId sceneid;
  std::unordered_set<const SoNode *> nodeset;
  const SoNode * prunenode;
  int traversedepth;
  SoFCRenderer *renderer;
  int annotation;
};

std::unordered_map<const SoNode *,
                   SoFCRenderCacheManagerP::CacheSensor> SoFCRenderCacheManagerP::cachetable;

std::unordered_map<const SoNode *,
                   SoFCRenderCacheManagerP::VCacheSensor> SoFCRenderCacheManagerP::vcachetable;

#define PRIVATE(obj) ((obj)->pimpl)

static FC_COIN_THREAD_LOCAL int _shapetypeid = -1;

static int
getMaxShapeTypeId()
{
  SoTypeList derivedtypes;
  int n = SoType::getAllDerivedFrom(SoShape::getClassTypeId(), derivedtypes);
  int res = 0;
  for (int i=0; i<n; ++i) {
    int idx = static_cast<int>(derivedtypes[i].getData());
    if (res < idx)
      res = idx;
  }
  return res;
}

SoFCRenderCacheManagerP::SoFCRenderCacheManagerP()
{
  this->prunenode = nullptr;
  this->selid = 0;
  this->sceneid = 0;
  this->annotation = 0;
  this->action = nullptr;
  this->shapetypeid = 0;

  if (_shapetypeid < 0) {
    // In case the shape node is defined in late loaded module, we need to
    // re-init SoCallbackAction (by calling initAction()), because
    // SoCallbackAction only works for existing type ID (i.e. all existing
    // SoShape derived types in our case) at the time of calling
    // addPre/PostCallback().
    SoCallbackAction::addMethod(SoShape::getClassTypeId(),
        [](SoAction *action, SoNode *node) {
            if (_shapetypeid < static_cast<int>(node->getTypeId().getData())) {
                node->touch(); // make sure to revisit
                _shapetypeid = 0;
            }
            SoNode::callbackS(action, node);
        }
    );
  }
  initAction();
  this->renderer = new SoFCRenderer;
}

void SoFCRenderCacheManagerP::initAction()
{
  if (this->action && this->shapetypeid && _shapetypeid == this->shapetypeid)
    return;
  delete this->action;
  _shapetypeid = this->shapetypeid = getMaxShapeTypeId();
  this->action = new SoCallbackAction;
  this->action->addPreCallback(SoFCSelectionRoot::getClassTypeId(), &preSeparator, this);
  this->action->addPostCallback(SoFCSelectionRoot::getClassTypeId(), &postSeparator, this);
  this->action->addPostCallback(SoSeparator::getClassTypeId(), &postSep, this);
  this->action->addPreCallback(SoFCSelection::getClassTypeId(), &preFCSel, this);
  this->action->addPostCallback(SoFCSelection::getClassTypeId(), &postFCSel, this);
  this->action->addPreCallback(SoAnnotation::getClassTypeId(), &preAnnotation, this);
  this->action->addPostCallback(SoAnnotation::getClassTypeId(), &postAnnotation, this);
  this->action->addPreCallback(SoFCPathAnnotation::getClassTypeId(), &prePathAnnotation, this);
  this->action->addPostCallback(SoFCPathAnnotation::getClassTypeId(), &postPathAnnotation, this);
  this->action->addPreCallback(SoShape::getClassTypeId(), &preShape, this);
  this->action->addPostCallback(SoShape::getClassTypeId(), &postShape, this);
  this->action->addPostCallback(SoTexture::getClassTypeId(), &postTexture, this);
  this->action->addPostCallback(SoVRMLTexture::getClassTypeId(), &postTexture, this);
  this->action->addPostCallback(SoResetTransform::getClassTypeId(), &postResetTransform, this);
  this->action->addPostCallback(SoTextureMatrixTransform::getClassTypeId(), &postTextureTransform, this);
  this->action->addPostCallback(SoTexture2Transform::getClassTypeId(), &postTextureTransform, this);
  this->action->addPostCallback(SoTexture3Transform::getClassTypeId(), &postTextureTransform, this);
  this->action->addPostCallback(SoVRMLTextureTransform::getClassTypeId(), &postTextureTransform, this);
  this->action->addPostCallback(SoLightModel::getClassTypeId(), &postLightModel, this);
  this->action->addPostCallback(SoMaterial::getClassTypeId(), &postMaterial, this);
  this->action->addPostCallback(SoVRMLMaterial::getClassTypeId(), &postVRMLMaterial, this);
  this->action->addPostCallback(SoBaseColor::getClassTypeId(), &postColor, this);
  this->action->addPostCallback(SoVRMLColor::getClassTypeId(), &postVRMLColor, this);
  this->action->addPostCallback(SoDepthBuffer::getClassTypeId(), &postDepthBuffer, this);
  this->action->addPostCallback(SoLight::getClassTypeId(), &postLight, this);
  this->action->addPostCallback(SoVRMLLight::getClassTypeId(), &postVRMLLight, this);
  this->action->addPostCallback(SoClipPlane::getClassTypeId(), &postClipPlane, this);
  this->action->addPreCallback(SoAutoZoomTranslation::getClassTypeId(), &preAutoZoom, this);
  this->action->addPostCallback(SoAutoZoomTranslation::getClassTypeId(), &postAutoZoom, this);

  this->action->addTriangleCallback(SoShape::getClassTypeId(), &addTriangle, this);
  this->action->addLineSegmentCallback(SoShape::getClassTypeId(), &addLine, this);
  this->action->addPointCallback(SoShape::getClassTypeId(), &addPoint, this);
}

SoFCRenderCacheManagerP::~SoFCRenderCacheManagerP()
{
  delete this->action;
  delete this->renderer;
}

const SbFCMap<int, CoinPtr<SoPath> > &
SoFCRenderCacheManager::getSelectionPaths() const
{
  return PRIVATE(this)->selpaths;
}

SoFCRenderCacheManager::SoFCRenderCacheManager()
  :pimpl(new SoFCRenderCacheManagerP)
{
}

SoFCRenderCacheManager::~SoFCRenderCacheManager()
{
  delete pimpl;
}

void
SoFCRenderCacheManager::clear()
{
  PRIVATE(this)->stack.clear();
  PRIVATE(this)->selnodeid.clear();
  PRIVATE(this)->nodeset.clear();
  // PRIVATE(this)->cachetable.clear();
  // PRIVATE(this)->vcachetable.clear();
  PRIVATE(this)->selcaches.clear();
  PRIVATE(this)->selpaths.clear();
  PRIVATE(this)->renderer->clear();
}

bool
SoFCRenderCacheManager::isOnTop(const std::string & key, bool altonly) const
{
  auto it = PRIVATE(this)->selcaches.find(key);
  if (it == PRIVATE(this)->selcaches.end())
    return false;
  for (auto & v : it->second) {
    if (v.second.ontop) {
      if (!altonly)
        return true;
      auto iter = v.second.elements.find(std::string());
      if (iter != v.second.elements.end()
          && (iter->second.id & SoFCRenderer::SelIdAlt))
        return true;
    }
  }
  return false;
}

SoPath *
SoFCRenderCacheManager::getHighlightPath() const
{
  return PRIVATE(this)->highlightpath;
}

void
SoFCRenderCacheManager::setHighlight(SoPath * path,
                                     const SoDetail * detail,
                                     uint32_t color,
                                     bool ontop,
                                     bool wholeontop)
{
  if (!path || path->getLength() == 0)
    return;
  SoState * state = PRIVATE(this)->action->getState();

  PRIVATE(this)->highlightpath = path;

  RenderCachePtr cache;
  if (PRIVATE(this)->nosectionontop != ViewParams::getNoSectionOnTop()) {
    PRIVATE(this)->nosectionontop = ViewParams::getNoSectionOnTop();
    PRIVATE(this)->pathcachetable.clear();
  }
  auto it = PRIVATE(this)->pathcachetable.find(path);
  if (it != PRIVATE(this)->pathcachetable.end())
    cache = it->second.cache;
  else {
    cache = new SoFCRenderCache(state, path->getHead());
    cache->open(state);
    PRIVATE(this)->stack.resize(1, cache);
    if (ontop) {
      SoFCSwitch::setOverrideSwitch(state, true);
      SoFCSwitch::pushSwitchPath(path);
    }
    PRIVATE(this)->action->apply(path);
    if (ontop) {
      SoFCSwitch::popSwitchPath();
      SoFCSwitch::setOverrideSwitch(state, false);
    }
    cache->close(state);
    PRIVATE(this)->stack.clear();
    PRIVATE(this)->selnodeid.clear();
    if (!cache->isEmpty()) {
      // Must use SoTempPath as key to avoid path changes, because we are using
      // the path as key which is supposed to be immutable.
      PathPtr tmppath = new SoTempPath(path->getLength());
      tmppath->append(path);
      auto &sensor = PRIVATE(this)->pathcachetable[tmppath];
      sensor.path = tmppath;
      sensor.master = PRIVATE(this);
      sensor.attach(path->copy());
      sensor.cache = cache;
    }
  }

  int order = ontop ? 1 : 0;
  PRIVATE(this)->highlightcache = cache;
  PRIVATE(this)->renderer->setHighlight(
        cache->buildHighlightCache(
          PRIVATE(this)->sharedcache, order, detail, color,
          SoFCRenderCache::PreselectHighlight
          | SoFCRenderCache::CheckIndices
          | (wholeontop ? SoFCRenderCache::WholeOnTop : 0)),
        wholeontop);
}

void
SoFCRenderCacheManager::clearHighlight()
{
  PRIVATE(this)->highlightpath.reset();
  PRIVATE(this)->highlightcache.reset();
  PRIVATE(this)->renderer->clearHighlight();
}

void
SoFCRenderCacheManagerP::updateSelection(void * userdata, SoSensor * _sensor)
{
  SoFCRenderCacheManagerP * self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  SelectionSensor * sensor = static_cast<SelectionSensor*>(_sensor);
  SoPath *path = sensor->attachedPath;
  if (!path)
    return;
  sensor->refresh(self->renderer);
  if (!path->getLength())
    return;

  SoState * state = self->action->getState();
  RenderCachePtr cache = new SoFCRenderCache(state, path->getHead());
  cache->open(state);
  self->stack.resize(1, cache);
  if (sensor->ontop) {
    SoFCSwitch::setOverrideSwitch(state, true);
    SoFCSwitch::pushSwitchPath(path);
  }
  self->action->apply(path);
  if (sensor->ontop) {
    SoFCSwitch::popSwitchPath();
    SoFCSwitch::setOverrideSwitch(state, false);
  }
  cache->close(state);
  self->stack.clear();
  self->selnodeid.clear();

  if (cache->isEmpty())
    return;

  sensor->cache = cache;
  for (auto & v : sensor->elements) {
    auto & elentry = v.second;
    int flags = 0;
    if (ViewParams::highlightIndicesOnFullSelect())
      flags |= SoFCRenderCache::CheckIndices;
    if (elentry.id > 0)
      flags |= SoFCRenderCache::WholeOnTop;
    elentry.vcachemap = sensor->cache->buildHighlightCache(
        self->sharedcache, elentry.id, elentry.detail.get(), elentry.color, flags);

    self->renderer->addSelection(elentry.id, elentry.vcachemap);
    if (elentry.vcachemap.size() == 1
        && elentry.vcachemap.begin()->second.size() == 1
        && !elentry.vcachemap.begin()->second[0].key)
    {
      // here means the selection shows a bounding box, so don't put it on
      // selpaths for pick list
      self->selpaths.erase(elentry.id);
    }
  }
}

void
SoFCRenderCacheManager::addSelection(const std::string & key,
                                     const std::string & element,
                                     SoPath * nodepath,
                                     SoPath * detailpath,
                                     const SoDetail * detail,
                                     uint32_t color,
                                     bool ontop,
                                     bool alt,
                                     bool implicit)
{
  if (!detailpath || !detailpath->getLength())
    return;

  bool update = false;
  PathPtr selpath;
  auto & paths = PRIVATE(this)->selcaches[key];
  SelectionSensor * sensor;
  auto it = paths.find(detailpath);
  if (it != paths.end()) {
    sensor = &it->second;
    selpath = it->first;
  }
  else {
    selpath = detailpath->copy();
    sensor = &paths[selpath];
    sensor->attachPath(selpath);
    sensor->setFunction(&SoFCRenderCacheManagerP::updateSelection);
    sensor->setData(PRIVATE(this));
    update = true;
  }

  if (sensor->ontop)
    ontop = true;

  int id = 0;
  if (alt) {
    id |= SoFCRenderer::SelIdAlt;
    ontop = true;
  }
  id += ++PRIVATE(this)->selid;
  if (!ontop)
    id = id | (-1 - SoFCRenderer::SelIdMask);

  if (id > 0 && !sensor->ontop) {
    sensor->ontop = true;
    update = true;
    if (element.size()) {
      if (nodepath != detailpath) {
        addSelection(key, "", nodepath, nodepath, nullptr, color & 0xff, true, false, true);
      } else {
        // When any element is selected on top, we shall bring the whole object on
        // top. So make sure a nil element entry exist.
        auto & elentry = sensor->elements[std::string()];
        if (!elentry.id) {
          elentry.id = id++;
          elentry.id |= SoFCRenderer::SelIdImplicit;
          ++PRIVATE(this)->selid;
        }
      }
    }
    for (auto & v : sensor->elements) {
      auto & elentry = v.second;
      if (elentry.id < 0) {
        PRIVATE(this)->renderer->removeSelection(elentry.id);
        elentry.id = id++;
        ++PRIVATE(this)->selid;
        if (!element.empty())
          elentry.id |= SoFCRenderer::SelIdPartial;
        else if (implicit)
          elentry.id |= SoFCRenderer::SelIdImplicit;
        else
          elentry.id |= SoFCRenderer::SelIdFull;
      }

      elentry.color &= 0xffffff00;
      elentry.color |= color & 0xff;

      if (v.first.empty())
        PRIVATE(this)->selpaths[elentry.id] = selpath;

      if (sensor->cache) {
        int flags = 0;
        if (ViewParams::highlightIndicesOnFullSelect())
          flags |= SoFCRenderCache::CheckIndices;
        if (elentry.id > 0)
          flags |= SoFCRenderCache::WholeOnTop;
        elentry.vcachemap = sensor->cache->buildHighlightCache(
            PRIVATE(this)->sharedcache, elentry.id, elentry.detail.get(), elentry.color, flags);
        PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
      }
    }
  }

  auto & elentry = sensor->elements[element];

  if (elentry.id & SoFCRenderer::SelIdAlt)
    id |= SoFCRenderer::SelIdAlt;
  if (id > 0 && (color & 0xffffff00)) {
    if (element.empty())
      id |= SoFCRenderer::SelIdFull;
    else
      id |= SoFCRenderer::SelIdPartial;
  }
  
  if (elentry.id && elentry.id != id) {
    if (elentry.id > 0 && element.empty())
      PRIVATE(this)->selpaths.erase(elentry.id);
    PRIVATE(this)->renderer->removeSelection(elentry.id);
  }
  if (id > 0 && element.empty())
    PRIVATE(this)->selpaths[id] = selpath;
  elentry.id = id;
  elentry.color = color;
  elentry.detail.reset(detail ? detail->copy() : nullptr);
  if (sensor->cache) {
    int flags = 0;
    if (ViewParams::highlightIndicesOnFullSelect())
      flags |= SoFCRenderCache::CheckIndices;
    if (elentry.id > 0)
      flags |= SoFCRenderCache::WholeOnTop;
    elentry.vcachemap = sensor->cache->buildHighlightCache(
        PRIVATE(this)->sharedcache, elentry.id, elentry.detail.get(), elentry.color, flags);
    PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
  }

  if (update)
    SoFCRenderCacheManagerP::updateSelection(PRIVATE(this), sensor);
}

void
SoFCRenderCacheManager::addSelection(const std::string & key,
                                     const std::string & element,
                                     SoNode * node,
                                     uint32_t color,
                                     bool ontop,
                                     bool alt)
{
  static FC_COIN_THREAD_LOCAL SoTempPath path(1);
  path.ref();
  path.truncate(0);
  path.append(node);
  addSelection(key, element, &path, &path, nullptr, color, ontop, alt);
  path.truncate(0);
  path.unrefNoDelete();
}

void
SoFCRenderCacheManager::removeSelection(const std::string & key,
                                        const std::string & element,
                                        bool alt)
{
  auto it = PRIVATE(this)->selcaches.find(key);
  if (it == PRIVATE(this)->selcaches.end())
    return;

  auto & paths = it->second;

  for (auto itpath = paths.begin(); itpath!=paths.end(); ) {
    auto & sensor = itpath->second;
    auto iter = sensor.elements.find(element);
    if (iter == sensor.elements.end()) {
      ++itpath;
      continue;
    }
    auto & elentry = iter->second;
    if (alt) {
      if (!(elentry.id & SoFCRenderer::SelIdAlt)) {
        ++itpath;
        continue;
      }
      if (element.empty())
        PRIVATE(this)->selpaths.erase(elentry.id);
      PRIVATE(this)->renderer->removeSelection(elentry.id);
      if (!(elentry.id & SoFCRenderer::SelIdSelected)) {
        sensor.elements.erase(iter);
        if (sensor.elements.empty())
          itpath = paths.erase(itpath);
        else
          ++itpath;
        continue;
      }
      elentry.id ^= SoFCRenderer::SelIdAlt;
      PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
      if (element.empty())
        PRIVATE(this)->selpaths[elentry.id] = itpath->first;
      ++itpath;
      continue;
    }
    if (element.empty())
      PRIVATE(this)->selpaths.erase(elentry.id);
    if (elentry.id & SoFCRenderer::SelIdAlt) {
      if (elentry.id & SoFCRenderer::SelIdSelected) {
        PRIVATE(this)->renderer->removeSelection(elentry.id);
        elentry.id &= ~(SoFCRenderer::SelIdSelected);
        elentry.color &= 0xff;
        elentry.vcachemap = sensor.cache->buildHighlightCache(
            PRIVATE(this)->sharedcache, elentry.id, elentry.detail.get(), elentry.color,
            elentry.id>0 ? SoFCRenderCache::WholeOnTop : 0);
        PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
      }
      ++itpath;
      continue;
    }
    PRIVATE(this)->renderer->removeSelection(elentry.id);
    sensor.elements.erase(iter);
    if (sensor.ontop && element.size()) {
      iter = sensor.elements.find(std::string());
      if (iter != sensor.elements.end()) {
        auto & elentry = iter->second;
        if (!(elentry.id & SoFCRenderer::SelIdAlt)) {
          PRIVATE(this)->selpaths.erase(elentry.id);
          PRIVATE(this)->renderer->removeSelection(elentry.id);
          sensor.elements.erase(iter);
        }
      }
    }
    if (sensor.elements.empty())
      itpath = paths.erase(itpath);
    else
      ++itpath;
    continue;
  }

  if (paths.empty())
    PRIVATE(this)->selcaches.erase(it);
}

int
SoFCRenderCacheManager::clearSelection(bool alt)
{
  int res = 0;
  auto itsel = PRIVATE(this)->selcaches.begin();
  while (itsel!=PRIVATE(this)->selcaches.end()) {
    auto & paths = itsel->second;
    for (auto itpath=paths.begin(); itpath!=paths.end(); ) {
      auto & sensor = itpath->second;
      for (auto iter=sensor.elements.begin(); iter!=sensor.elements.end(); ) {
        auto & elentry = iter->second;
        if (alt) {
          if (!(elentry.id & SoFCRenderer::SelIdAlt)) {
            ++iter;
            continue;
          }
          if (iter->first.empty())
            PRIVATE(this)->selpaths.erase(elentry.id);
          ++res;
          PRIVATE(this)->renderer->removeSelection(elentry.id);
          if (elentry.color & ~0xff) {
            elentry.id = ++PRIVATE(this)->selid;
            if (iter->first.empty())
              PRIVATE(this)->selpaths[elentry.id] = itpath->first;
            PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
            ++iter;
            continue;
          }
          iter = sensor.elements.erase(iter);
          continue;
        }
        else if (elentry.id & SoFCRenderer::SelIdAlt) {
          if (elentry.id & SoFCRenderer::SelIdSelected) {
            ++res;
            PRIVATE(this)->renderer->removeSelection(elentry.id);
            elentry.id &= ~(SoFCRenderer::SelIdSelected);
            elentry.color &= 0xff;
            if (sensor.cache) {
              elentry.vcachemap = sensor.cache->buildHighlightCache(
                  PRIVATE(this)->sharedcache, elentry.id, elentry.detail.get(), elentry.color,
                  elentry.id>0 ? SoFCRenderCache::WholeOnTop : 0);
              PRIVATE(this)->renderer->addSelection(elentry.id, elentry.vcachemap);
            }
          }
          ++iter;
          continue;
        }
        else {
          if (iter->first.empty())
            PRIVATE(this)->selpaths.erase(elentry.id);
          PRIVATE(this)->renderer->removeSelection(elentry.id);
          iter = sensor.elements.erase(iter);
          ++res;
        }
      }
      if (sensor.elements.empty())
        itpath = paths.erase(itpath);
      else
        ++itpath;
    }
    if (paths.empty())
      itsel = PRIVATE(this)->selcaches.erase(itsel);
    else
      ++itsel;
  }
  if (PRIVATE(this)->selcaches.empty())
    PRIVATE(this)->selid = 0;

  return res;
}

void
SoFCRenderCacheManager::render(SoGLRenderAction * action)
{
  SoState * state = action->getState();
  SoCacheElement::invalidate(state);
  SoGLCacheContextElement::shouldAutoCache(state,
                                           SoGLCacheContextElement::DONT_AUTO_CACHE);

  const SoPath * path = action->getCurPath();
  if (!PRIVATE(this)->sceneid || PRIVATE(this)->sceneid != path->getTail()->getNodeId()) {
    SoState * state = action->getState();
    const SoShapeStyleElement * shapestyle = SoShapeStyleElement::get(state);
    unsigned int shapestyleflags = shapestyle->getFlags();
    if (!(shapestyleflags & SoShapeStyleElement::SHADOWMAP))
      PRIVATE(this)->sceneid = path->getTail()->getNodeId();

    RenderCachePtr cache = new SoFCRenderCache(state, path->getTail());
    cache->open(state);
    // Note that we are capturing state of the SoGLRenderAction here. However,
    // we will change to use SoCallBackAction to build the rest of the render
    // cache. That's why we need to reset the internal kept action state stack
    // depth as shown below to avoid popping the initially captured state here.
    cache->resetActionStateStackDepth();
    PRIVATE(this)->stack.resize(1, cache);
    PRIVATE(this)->initAction();
    PRIVATE(this)->action->apply(path->getTail());
    cache->close(state);
    PRIVATE(this)->renderer->setScene(cache);
    PRIVATE(this)->stack.clear();
    PRIVATE(this)->selnodeid.clear();
  }

  PRIVATE(this)->renderer->render(action);
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::preSeparator(void *userdata,
                                      SoCallbackAction *action,
                                      const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->nodeset.insert(node).second) {
      self->prunenode = node;
      SoFCSelectionRoot::reportCyclicScene(action, const_cast<SoNode*>(node));
      return SoCallbackAction::PRUNE;
  }

  SoState * state = action->getState();
  SoFCRenderCache *currentcache = self->stack.empty() ? nullptr : self->stack.back();
  RenderCachePtr prevcache;
  SbFCVector<RenderCachePtr> *sensorcaches = nullptr;
  if (action->getCurPathCode() == SoAction::BELOW_PATH
      || action->getCurPathCode() == SoAction::NO_PATH) {
    CacheSensor &sensor = self->cachetable[node];
    sensor.attach(self, node);
    sensorcaches = &sensor.caches;
    for (auto it=sensorcaches->begin(); it!=sensorcaches->end();) {
      prevcache = *it;
      if (prevcache->getNodeId() != node->getNodeId()) {
        it = sensorcaches->erase(it);
        continue;
      }
      if (prevcache->isValid(state)) {
        if (currentcache)
          currentcache->addChildCache(state, prevcache);
        self->stack.push_back(prevcache);
        return SoCallbackAction::PRUNE;
      }
      ++it;
    }
  }

  state->push();

  int selectstyle = Material::Full;
  auto selroot = static_cast<const SoFCSelectionRoot*>(node);

  switch(selroot->selectionStyle.getValue()) {
  case SoFCSelectionRoot::Box:
    selectstyle = Material::Box;
    break;
  case SoFCSelectionRoot::Unpickable:
    if (action->getCurPathCode() != SoAction::IN_PATH)
      selectstyle = Material::Unpickable;
    break;
  default:
    break;
  }

  RenderCachePtr cache(new SoFCRenderCache(state, const_cast<SoNode*>(node), prevcache));

  if (sensorcaches)
    sensorcaches->push_back(cache);
  if (currentcache)
    currentcache->beginChildCaching(state, cache);
  self->stack.push_back(cache);
  cache->open(state, selectstyle, false);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postSeparator(void *userdata,
                                       SoCallbackAction *action,
                                       const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);

  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  if (self->prunenode == node) {
    self->prunenode = nullptr;
    return SoCallbackAction::PRUNE;
  }

  self->nodeset.erase(node);
  SoState * state = action->getState();
  RenderCachePtr cache(self->stack.back());

  self->stack.pop_back();
  if (SoCacheElement::getCurrentCache(state) == cache) {
    cache->close(state);
    state->pop();
    if (self->stack.size())
      self->stack.back()->endChildCaching(state, cache);
  }
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postSep(void *userdata,
                                 SoCallbackAction *action,
                                 const SoNode * node)
{
  (void)node;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);

  if (!self->stack.empty())
    self->stack.back()->checkState(action->getState());
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::preFCSel(void *userdata,
                                 SoCallbackAction *action,
                                 const SoNode * node)
{
  (void)action;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  self->selnodeid.push_back(node->getNodeId());
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postFCSel(void *userdata,
                                   SoCallbackAction *action,
                                   const SoNode * node)
{
  (void)action;
  (void)node;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  self->selnodeid.pop_back();
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postPathAnnotation(void *userdata,
                                            SoCallbackAction *action,
                                            const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  auto annotation = static_cast<const SoFCPathAnnotation*>(node);
  if (annotation->priority.getValue())
    self->stack.back()->decreaseRenderingOrder(action->getState(), annotation->priority.getValue());
  else if (annotation->getPath() && --self->annotation == 0)
    self->stack.back()->decreaseRenderingOrder(action->getState());
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::prePathAnnotation(void *userdata,
                                           SoCallbackAction *action,
                                           const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  auto annotation = static_cast<const SoFCPathAnnotation*>(node);
  if (annotation->priority.getValue())
    self->stack.back()->increaseRenderingOrder(action->getState(), annotation->priority.getValue());
  else if (annotation->getPath() && ++self->annotation == 1)
    self->stack.back()->increaseRenderingOrder(action->getState());
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::preAnnotation(void *userdata,
                                       SoCallbackAction *action,
                                       const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  (void)node;
  if (++self->annotation == 1)
    self->stack.back()->increaseRenderingOrder(action->getState());
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postAnnotation(void *userdata,
                                        SoCallbackAction *action,
                                        const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  (void)node;
  if (self->annotation && --self->annotation == 0)
    self->stack.back()->decreaseRenderingOrder(action->getState());
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postTexture(void *userdata,
                                     SoCallbackAction *action,
                                     const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node);
  self->stack.back()->addTexture(action->getState(), node);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postResetTransform(void *userdata,
                                           SoCallbackAction *action,
                                           const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoResetTransform::getClassTypeId()));
  const SoResetTransform *rs = static_cast<const SoResetTransform*>(node);
  if (!rs->whatToReset.isIgnored() &&
      (rs->whatToReset.getValue() & SoResetTransform::TRANSFORM)) {
    self->stack.back()->resetMatrix(action->getState());
  }
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postTextureTransform(void *userdata,
                                              SoCallbackAction *action,
                                              const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  self->stack.back()->addTextureTransform(action->getState(), node);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postLightModel(void *userdata,
                                        SoCallbackAction *action,
                                        const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoLightModel::getClassTypeId()));
  const SoLightModel *lightmodel = static_cast<const SoLightModel*>(node);
  self->stack.back()->setLightModel(action->getState(), lightmodel);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postMaterial(void *userdata,
                                      SoCallbackAction *action,
                                      const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoMaterial::getClassTypeId()));
  const SoMaterial *material = static_cast<const SoMaterial*>(node);
  self->stack.back()->setMaterial(action->getState(), material);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postVRMLMaterial(void *userdata,
                                          SoCallbackAction *action,
                                          const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoVRMLMaterial::getClassTypeId()));
  auto material = static_cast<const SoVRMLMaterial*>(node);
  self->stack.back()->setMaterial(action->getState(), material);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postColor(void *userdata,
                                   SoCallbackAction *action,
                                   const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoBaseColor::getClassTypeId()));
  const SoBaseColor *color = static_cast<const SoBaseColor*>(node);
  self->stack.back()->setBaseColor(action->getState(), color, color->rgb);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postVRMLColor(void *userdata,
                                       SoCallbackAction *action,
                                       const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoVRMLColor::getClassTypeId()));
  const SoVRMLColor *color = static_cast<const SoVRMLColor*>(node);
  self->stack.back()->setBaseColor(action->getState(), color, color->color);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postDepthBuffer(void *userdata,
                                         SoCallbackAction *action,
                                         const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoDepthBuffer::getClassTypeId()));
  const SoDepthBuffer *dnode = static_cast<const SoDepthBuffer*>(node);
  self->stack.back()->setDepthBuffer(action->getState(), dnode);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postLight(void *userdata,
                                  SoCallbackAction *action,
                                  const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoLight::getClassTypeId()));
  auto light = static_cast<const SoLight*>(node);
  if (light->on.getValue() && !light->on.isIgnored())
    self->stack.back()->addLight(action->getState(), light);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postVRMLLight(void *userdata,
                                       SoCallbackAction *action,
                                       const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoVRMLLight::getClassTypeId()));
  auto light = static_cast<const SoVRMLLight*>(node);
  if (light->on.getValue() && !light->on.isIgnored())
    self->stack.back()->addLight(action->getState(), light);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postClipPlane(void *userdata,
                                       SoCallbackAction *action,
                                       const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoClipPlane::getClassTypeId()));
  self->stack.back()->addClipPlane(action->getState(), static_cast<const SoClipPlane*>(node));
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::preAutoZoom(void *userdata,
                                     SoCallbackAction *action,
                                     const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  assert(node && node->isOfType(SoAutoZoomTranslation::getClassTypeId()));
  self->stack.back()->addAutoZoom(action->getState(), static_cast<const SoAutoZoomTranslation*>(node));
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postAutoZoom(void *userdata,
                                      SoCallbackAction *action,
                                      const SoNode * node)
{
  (void)node;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::CONTINUE;

  auto state = action->getState();

  // reset to identity matrix because auto zoom will dynamically adjust scale
  // factor of the current transform depending on the current view, so must
  // evaluate on each frame
  SoModelMatrixElement::makeIdentity(state, nullptr);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::preShape(void *userdata,
                                  SoCallbackAction *action,
                                  const SoNode * node)
{
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (self->stack.empty())
      return SoCallbackAction::PRUNE;

  SoState * state = action->getState();
  SoFCRenderCache *currentcache = self->stack.back();

  VCacheSensor & sensor = self->vcachetable[node];
  sensor.attach(self, node);
  CoinPtr<SoFCVertexCache> prev;
  for (auto it=sensor.caches.begin(); it!=sensor.caches.end();) {
    auto & cache = *it;
    if (!prev) prev = cache;
    if (cache->getNodeId() != node->getNodeId()) {
      it = sensor.caches.erase(it);
      continue;
    }
    if (cache->isValid(state)) {
      currentcache->addChildCache(state, cache);
      return SoCallbackAction::PRUNE;
    }
    ++it;
  }

  state->push();
  self->vcache.reset(new SoFCVertexCache(state, const_cast<SoNode*>(node), prev));
  if (self->selnodeid.size())
    self->vcache->setSelectionNodeId(self->selnodeid.back());
  sensor.caches.emplace_back(self->vcache.get());

  currentcache->beginChildCaching(state, self->vcache);
  self->vcache->open(state);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoFCRenderCacheManagerP::postShape(void *userdata,
                                   SoCallbackAction *action,
                                   const SoNode * node)
{
  (void)node;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->vcache)
    return SoCallbackAction::PRUNE;

  SoState *state = action->getState();
  state->pop();
  self->vcache->close(state);
  self->stack.back()->endChildCaching(state, self->vcache);
  self->vcache.reset();
  return SoCallbackAction::CONTINUE;
}

void
SoFCRenderCacheManagerP::addTriangle(void * userdata, SoCallbackAction *action,
                                     const SoPrimitiveVertex * v0,
                                     const SoPrimitiveVertex * v1,
                                     const SoPrimitiveVertex * v2)
{
  (void)action;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->vcache) return;

  assert(v0 < v1 && v0 < v2);
  assert(((const char*)v1- (const char*)v0) % sizeof(SoPrimitiveVertex) == 0);
  assert(((const char*)v2- (const char*)v0) % sizeof(SoPrimitiveVertex) == 0);

  int pdidx[3];
  pdidx[0] = 0;
  pdidx[1] = (int)(v1 - v0);
  pdidx[2] = (int)(v2 - v0);
  self->vcache->addTriangle(v0,v1,v2,pdidx);
}

void
SoFCRenderCacheManagerP::addLine(void * userdata, SoCallbackAction *action,
                                 const SoPrimitiveVertex * v0,
                                 const SoPrimitiveVertex * v1)
{
  (void)action;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->vcache) return;
  self->vcache->addLine(v0,v1);
}

void
SoFCRenderCacheManagerP::addPoint(void * userdata, SoCallbackAction *action,
                                  const SoPrimitiveVertex * v0)
{
  (void)action;
  SoFCRenderCacheManagerP *self = reinterpret_cast<SoFCRenderCacheManagerP*>(userdata);
  if (!self->vcache) return;
  self->vcache->addPoint(v0);
}

void
SoFCRenderCacheManager::getBoundingBox(SbBox3f & bbox) const
{
  PRIVATE(this)->renderer->getBoundingBox(bbox);
}

SbFCUniqueId
SoFCRenderCacheManager::getSceneNodeId() const
{
  return PRIVATE(this)->sceneid;
}

void
SoFCRenderCacheManager::setHatchImage(const void *dataptr, int nc, int width, int height)
{
  PRIVATE(this)->renderer->setHatchImage(dataptr, nc, width, height);
}

const char *
SoFCRenderCacheManager::getRenderStatistics() const
{
  return PRIVATE(this)->renderer->getStatistics();
}

// vim: noai:ts=2:sw=2
