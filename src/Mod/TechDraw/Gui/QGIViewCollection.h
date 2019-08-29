/***************************************************************************
 *   Copyright (c) 2012-2013 Luke Parry <l.parry@warwick.ac.uk>            *
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

#ifndef _DRAWINGGUI_QGRAPHICSITEMVIEWCOLLECTION_H
#define _DRAWINGGUI_QGRAPHICSITEMVIEWCOLLECTION_H

#include <QGraphicsItemGroup>
#include <QObject>

#include "QGIView.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport  QGIViewCollection : public QGIView
{
public:
    QGIViewCollection();
    ~QGIViewCollection() = default;

    enum {Type = QGraphicsItem::UserType + 110};
    int type() const override { return Type;}

    virtual void updateView(bool update = false) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override;

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

};

} // namespace MDIViewPageGui

#endif // _DRAWINGGUI_QGRAPHICSITEMVIEWCOLLECTION_H
