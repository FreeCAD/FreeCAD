/****************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#ifndef PARTGUI_BBOX_RAYPICK_H
#define PARTGUI_BBOX_RAYPICK_H

#include <memory>
#include <Inventor/SbVec3f.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/actions/SoRayPickAction.h>

namespace PartGui {

/// Provides a fast data struture for ray picking of bounding box
class PartGuiExport BoundBoxRayPick {
public:
    BoundBoxRayPick();
    BoundBoxRayPick(BoundBoxRayPick &&other);
    virtual ~BoundBoxRayPick();

    void init(const std::vector<SbBox3f> &bboxes, bool delay = true);
    void init(std::vector<SbBox3f> &&bboxes, bool delay = true);
    void clear();
    bool empty() const;
    std::size_t size() const;
    void rayPick(SoRayPickAction *action, std::vector<int> &results);
    const std::vector<SbBox3f> & getBoundBoxes() const;

private:
    class Private;
    std::unique_ptr<Private> pimpl;
};

typedef std::shared_ptr<BoundBoxRayPick> BoundBoxRayPickPtr;

} // namespace PartGui

#endif
