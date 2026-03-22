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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include "QGIViewCollection.h"
#include "QGIUserTypes.h"


QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QEvent;
QT_END_NAMESPACE

namespace TechDraw {
    class DrawProjGroup;
}

namespace TechDrawGui
{
class QGIViewPart;

class TechDrawGuiExport QGIProjGroup : public QGIViewCollection
{
public:
    QGIProjGroup();

    // TODO: if the QGIVO is deleted, should we clean up any remaining QGIVParts??
    ~QGIProjGroup() override = default;

    enum {Type = UserType::QGIProjGroup};
    int type() const override { return Type;}

    void alignTo(QGIProjGroup *, const QString &alignment);

    void rotateView() override;

    void drawBorder() override;

    bool isMember(App::DocumentObject* dvpObj) const;
    QGIView* getAnchorQItem() const;
    TechDraw::DrawProjGroup* getPGroupFeature() const;
    QList<QGIViewPart*> secondaryQViews() const;

protected:
    bool sceneEventFilter(QGraphicsItem* watched, QEvent *event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    // Mouse handling
    void mouseMoveEvent(QGraphicsSceneMouseEvent * event ) override;
    void mousePressEvent(QGraphicsSceneMouseEvent * event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) override;

    void mouseReleaseEvent(QGIView* originator, QGraphicsSceneMouseEvent* event);

private:
    /// Convenience function
    bool autoDistributeEnabled() const;

    QGraphicsItem* m_origin;
    QPoint mousePos;
};

} // namespace MDIViewPageGui