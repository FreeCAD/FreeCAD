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

#ifndef DRAWINGGUI_QGIProjGroup_H
#define DRAWINGGUI_QGIProjGroup_H

#include <QGraphicsItemGroup>
#include <QObject>
#include <App/PropertyLinks.h>

#include "QGIViewCollection.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QEvent;
QT_END_NAMESPACE

namespace TechDrawGui
{

class TechDrawGuiExport QGIProjGroup : public QGIViewCollection
{
public:
    QGIProjGroup();

    // TODO: if the QGIVO is deleted, should we clean up any remaining QGIVParts??
    ~QGIProjGroup() = default;

    enum {Type = QGraphicsItem::UserType + 113};
    int type() const { return Type;}

    void alignTo(QGIProjGroup *, const QString &alignment);

    virtual void updateView(bool update = false);
    virtual void drawBorder(void);

protected:
    virtual bool sceneEventFilter(QGraphicsItem* watched, QEvent *event);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    // Mouse handling
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event );
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
    QGIView * getAnchorQItem() const;

private:
    /// Convenience function
    TechDraw::DrawProjGroup * getDrawView(void) const;

    QGraphicsRectItem *m_groupBackground;
    QGraphicsItem* m_origin;
    QPoint mousePos;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGIProjGroup_H
