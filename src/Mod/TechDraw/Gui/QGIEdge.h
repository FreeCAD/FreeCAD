/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

#include "QGIPrimPath.h"
#include "QGIUserTypes.h"

namespace TechDrawGui
{

class TechDrawGuiExport QGIEdge : public QGIPrimPath
{
public:
    explicit QGIEdge(int index);
    ~QGIEdge() override = default;

    enum {Type = UserType::QGIEdge};

    int type() const override { return Type;}
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    int getProjIndex() const { return projIndex; }

    void setCosmetic(bool state);
    void setHiddenEdge(bool b);
    bool getHiddenEdge() const { return(isHiddenEdge); }
    void setSmoothEdge(bool b) { isSmoothEdge = b; }
    bool getSmoothEdge() const { return(isSmoothEdge); }
    void setPrettyNormal() override;

    double getEdgeFuzz() const;

    void setLinePen(const QPen& isoPen);

    void setSource(TechDraw::SourceType source) { m_source = source; }
    TechDraw::SourceType getSource() const { return m_source;}

protected:

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    bool multiselectEligible() override { return true; }

    QColor getHiddenColor();

private:
    int projIndex;                                                     //index of edge in Projection. must exist.

    bool isCosmetic;
    bool isHiddenEdge;
    bool isSmoothEdge;

    TechDraw::SourceType m_source{TechDraw::SourceType::GEOMETRY};
};

}