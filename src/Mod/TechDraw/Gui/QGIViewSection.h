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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEWSECTION_H
#define DRAWINGGUI_QGRAPHICSITEMVIEWSECTION_H

#include "QGIViewPart.h"

namespace TechDraw {
class DrawViewSection;
}

namespace TechDrawGeometry {
class BaseGeom;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGIViewSection : public QGIViewPart
{
    Q_OBJECT

public:

    explicit QGIViewSection(const QPoint &position, QGraphicsScene *scene);
    ~QGIViewSection();

    virtual void draw() override;
    void updateView(bool update = false);
    enum {Type = QGraphicsItem::UserType + 108};
    int type() const { return Type;}

Q_SIGNALS:
    void dirty();

protected:
    void drawSectionFace();
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEWPART_H
