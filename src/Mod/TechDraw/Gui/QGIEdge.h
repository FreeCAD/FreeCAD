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

#ifndef DRAWINGGUI_QGRAPHICSITEMEDGE_H
#define DRAWINGGUI_QGRAPHICSITEMEDGE_H

#include "QGIPrimPath.h"

namespace TechDrawGui
{

class TechDrawGuiExport QGIEdge : public QGIPrimPath
{
public:
    explicit QGIEdge(int index);
    ~QGIEdge() {}

    enum {Type = QGraphicsItem::UserType + 103};

    int type() const override { return Type;}
    virtual QRectF boundingRect() const override;
    virtual QPainterPath shape() const override;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 ) override;

    int getProjIndex() const { return projIndex; }

    void setCosmetic(bool state);
    void setHiddenEdge(bool b);
    bool getHiddenEdge() { return(isHiddenEdge); }
    void setSmoothEdge(bool b) { isSmoothEdge = b; }
    bool getSmoothEdge() { return(isSmoothEdge); }
    virtual void setPrettyNormal() override;
    
    double getEdgeFuzz(void) const;

protected:
    int projIndex;                                                     //index of edge in Projection. must exist.

    bool isCosmetic;
    bool isHiddenEdge;
    bool isSmoothEdge;
    QColor getHiddenColor();
    Qt::PenStyle getHiddenStyle();

private:
};

}

#endif // DRAWINGGUI_QGRAPHICSITEMEDGE_H
