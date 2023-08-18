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

#ifndef DRAWINGGUI_QGRAPHICSITEMCMARK_H
#define DRAWINGGUI_QGRAPHICSITEMCMARK_H

#include <Mod/TechDraw/TechDrawGlobal.h>

# include "QGIVertex.h"

namespace TechDrawGui
{

class TechDrawGuiExport QGICMark : public QGIVertex
{
public:
    explicit QGICMark(int index);
    ~QGICMark() override {}

    enum {Type = QGraphicsItem::UserType + 171};
    int type() const override { return Type;}
    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    void draw(void);
    float getSize() { return m_size; }
    void setSize(float s);
    float getThick() { return m_width; }
    void setThick(float t);
    void setPrettyNormal() override;

    double getMarkFuzz(void) const;

protected:
    QColor getCMarkColor();

private:
    float m_size;
};

}

#endif // DRAWINGGUI_QGRAPHICSITEMCMARK_H
