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

#ifndef GUI_SOFCRENDERCACHEMANAGER_H
#define GUI_SOFCRENDERCACHEMANAGER_H

#include <map>
#include "../InventorBase.h"

class SoGLRenderAction;
class SoGroup;
class SoFCRenderCache;
class SoFCRenderCacheManagerP;
class SoPath;
class SoDetail;

class GuiExport SoFCRenderCacheManager
{
public:
  SoFCRenderCacheManager();
  virtual ~SoFCRenderCacheManager();

  void render(SoGLRenderAction *action);
  void clear();

  void setHighlight(SoPath * path, const SoDetail * detail, uint32_t color, bool ontop = false);

  void clearHighlight();

  void addSelection(const std::string & key,
                    const std::string & element,
                    SoPath * path,
                    const SoDetail * detail,
                    uint32_t color,
                    bool ontop = false,
                    bool alt = false);

  void addSelection(const std::string & key,
                    const std::string & element,
                    SoNode * node,
                    uint32_t color,
                    bool ontop = false,
                    bool alt = false);

  void removeSelection(const std::string & key,
                       const std::string & element,
                       bool alt = false);

  void clearSelection(bool alt = false);

  bool isOnTop(const std::string & key);

  const std::map<int, SoPath*> & getSelectionPaths() const;

  void getBoundingBox(SbBox3f & bbox) const;

  SbFCUniqueId getSceneNodeId() const;

private:
  friend class SoFCRenderCacheManagerP;
  SoFCRenderCacheManagerP * pimpl;
};

#endif // GUI_SOFCRENDERCACHEMANAGER_H 
// vim: noai:ts=2:sw=2
