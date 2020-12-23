/***************************************************************************
 *   Copyright (c) 2012-2014 Luke Parry <l.parry@warwick.ac.uk>            *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.           *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWINGGUI_QGRAPHICSITEMDRAWINGTEMPLATE_H
#define DRAWINGGUI_QGRAPHICSITEMDRAWINGTEMPLATE_H

#include "QGITemplate.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsPathItem;
QT_END_NAMESPACE

namespace TechDraw {
class DrawParametricTemplate;
}

namespace TechDrawGui
{

class TechDrawGuiExport  QGIDrawingTemplate : public QGITemplate
{
    Q_OBJECT

public:
    QGIDrawingTemplate(QGraphicsScene *);
    ~QGIDrawingTemplate();

    enum {Type = QGraphicsItem::UserType + 151};
    int type() const { return Type;}

    void clearContents();
    void draw();
    virtual void updateView(bool update = false);

protected:
  TechDraw::DrawParametricTemplate * getParametricTemplate();

protected:
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
  QGraphicsPathItem *pathItem;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMDRAWINGTEMPLATE_H
