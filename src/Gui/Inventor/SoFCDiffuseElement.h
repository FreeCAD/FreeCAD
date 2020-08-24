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

#ifndef FC_DIFFUSEELEMENT_H
#define FC_DIFFUSEELEMENT_H

#include "../InventorBase.h"
#include <Inventor/system/inttypes.h>
#include <Inventor/elements/SoElement.h>

class GuiExport SoFCDiffuseElement : public SoElement {
  typedef SoElement inherited;

  SO_ELEMENT_HEADER(SoFCDiffuseElement);
public:
  static void initClass(void);
  static void cleanup(void);

public:
  virtual void init(SoState * state);
  virtual void push(SoState * state);
  virtual SbBool matches(const SoElement * element) const;

  virtual SoElement * copyMatchInfo() const;

  SbFCUniqueId getDiffuseId() const;
  SbFCUniqueId getTransparencyId() const;

  static SbFCUniqueId get(SoState * state, SbFCUniqueId *transpid);
  static void set(SoState * state, SbFCUniqueId *diffuseid, SbFCUniqueId *transpid);

protected:
  SbFCUniqueId diffuseId;
  SbFCUniqueId transpId;
};

#endif //FC_DIFFUSEELEMENT_H
// vim: noai:ts=2:sw=2
