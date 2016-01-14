/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                 2014 wandererfan <WandererFan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEWANNOTATION_H
#define DRAWINGGUI_QGRAPHICSITEMVIEWANNOTATION_H

#include <QObject>
#include <QPainter>

#include "QGIView.h"
#include "QGCustomText.h"

namespace TechDraw {
class DrawViewAnnotation;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGIViewAnnotation : public QGIView
{
    Q_OBJECT

public:

    explicit QGIViewAnnotation(const QPoint &position, QGraphicsScene *scene);
    ~QGIViewAnnotation();

    enum {Type = QGI::UserType + 120};
    int type() const { return Type;}

    void updateView(bool update = false);
    void setViewAnnoFeature(TechDraw::DrawViewAnnotation *obj);

    virtual void draw();
    virtual QRectF boundingRect() const;

Q_SIGNALS:
    void hover(bool state);
    void selected(bool state);

protected:
    void drawAnnotation();
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

protected:
    QGCustomText *m_textItem;
    QColor m_colNormal;
    QColor m_colSel;
    QColor m_colPre;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEWANNOTATION_H
