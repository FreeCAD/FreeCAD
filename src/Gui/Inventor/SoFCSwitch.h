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

#ifndef FC_SOFCSWITCH_H
#define FC_SOFCSWITCH_H

#include <bitset>

#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/fields/SoMFName.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/SbColor.h>

/// Switch node that support global visibility override
class GuiExport SoFCSwitch : public SoSwitch {
  typedef SoSwitch inherited;
  SO_NODE_HEADER(SoFCSwitch);

public:
  /// Stores the child index used in switching override mode
  SoSFInt32 defaultChild;
  /// Stores the child index that will be traversed as long as the whichChild is not -1
  SoSFInt32 tailChild;
  /// Stores the child index that will be traversed first as long as the whichChild is not -1
  SoSFInt32 headChild;
  /// If greater than zero, then any children change will trigger parent notify
  SoSFInt32 childNotify;

  /// Stores children node names, for dynamic override children override
  SoMFName childNames;

  /// Enable/disable named child switch override
  SoSFBool allowNamedOverride;

  enum OverrideSwitch {
    /// No switch override
    OverrideNone,
    /// Override this and following SoFCSwitch node to its \c defaultChild if visible
    OverrideDefault,
    /** If OverrideDefault is on by some parent SoFCSwitch node, then
     * override any (grand)child SoFCSwitch nodes even if it is invisible
     */
    OverrideVisible,
    /// Reset override mode after this node
    OverrideReset,
  };
  SoSFEnum overrideSwitch;

  static void initClass(void);
  static void finish(void);

  SoFCSwitch();

  virtual void doAction(SoAction *action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void search(SoSearchAction * action);
  virtual void callback(SoCallbackAction *action);
  virtual void pick(SoPickAction *action);
  virtual void handleEvent(SoHandleEventAction *action);
  virtual void notify(SoNotList * nl);

  /// Enables switching override for the give action
  static void switchOverride(SoAction *action, OverrideSwitch o=OverrideDefault);

  enum TraverseStateFlag {
    /// Normal traverse
    TraverseNormal          =0,
    /// One or more parent SoFCSwitch nodes have been overridden
    TraverseOverride        =1,
    /// One or more parent SoFCSwitch are supposed to be invisible, but got overridden
    TraverseInvisible       =2,
    /// The immediate parent SoFCSwitch node has been switch to its \c defaultChild
    TraverseAlternative     =4,
  };
  typedef std::bitset<32> TraverseState;
  static bool testTraverseState(TraverseStateFlag flag);

  static void setOverrideSwitch(SoState *state, bool enable);
  static void pushSwitchPath(SoPath * path);
  static void popSwitchPath();

private:
  void traverseHead(SoAction *action, int idx);
  void traverseTail(SoAction *action, int idx);
  void traverseChild(SoAction *action, int idx);
};

#endif // FC_SOFCSWITCH_H
// vim: noai:ts=2:sw=2
