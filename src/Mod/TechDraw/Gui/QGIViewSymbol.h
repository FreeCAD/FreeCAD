/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2014 WandererFan <wandererfan@gmail.com>                *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QByteArray>
#include <QPointF>
#include <optional>

#include "QGIView.h"
#include "QGIUserTypes.h"


namespace TechDraw {
class DrawViewSymbol;
}

namespace TechDrawGui
{
class QGCustomSvg;
class QGDisplayArea;

class TechDrawGuiExport QGIViewSymbol : public QGIView
{
public:
    QGIViewSymbol();
    ~QGIViewSymbol() override;

    enum {Type = UserType::QGIViewSymbol};
    int type() const override { return Type;}

    void updateView(bool update = false) override;
    void setViewSymbolFeature(TechDraw::DrawViewSymbol *obj);

    void draw() override;
    void rotateView() override;


protected:
    virtual void drawSvg();
    void symbolToSvg(QByteArray qba);
    double legacyScaler(TechDraw::DrawViewSymbol* feature) const;
    double symbolScaler(TechDraw::DrawViewSymbol* feature) const;

    QGDisplayArea* m_displayArea;
    QGCustomSvg *m_svgItem;

    // Anchor point in SVG viewBox coordinates.  Captured when a Draft/Arch
    // view transitions into LockPosition = true (or on first draw if already
    // locked) and used to keep already-visible content at a stable page
    // position as new objects grow the bounding box.  Preserved across a
    // lock->unlock transition so that unlocking itself never causes a visible
    // jump; cleared on the next unlocked redraw so that subsequent content
    // changes re-centre the view (legacy behaviour).
    std::optional<QPointF> m_lockedSvgAnchor;
    bool m_wasLocked = false;
};

} // namespace