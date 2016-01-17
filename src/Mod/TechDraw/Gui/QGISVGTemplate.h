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

#ifndef DRAWINGGUI_QGRAPHICSITEMSVGTEMPLATE_H
#define DRAWINGGUI_QGRAPHICSITEMSVGTEMPLATE_H

#include "QGITemplate.h"

QT_BEGIN_NAMESPACE
class QGraphicsScene;
class QGraphicsSceneMouseEvent;
class QGraphicsSvgItem;
class QSvgRenderer;
QT_END_NAMESPACE

namespace TechDraw {
class DrawSVGTemplate;
}

namespace TechDrawGui
{

class TechDrawGuiExport QGISVGTemplate : public QGITemplate
{
    Q_OBJECT

public:
    QGISVGTemplate(QGraphicsScene *);
    ~QGISVGTemplate();

    enum {Type = QGraphicsItem::UserType + 153};
    int type() const { return Type; }

    /// Currently just frees up textFields
    void clearContents();
    void draw();
    virtual void updateView(bool update = false);

Q_SIGNALS:
    void dirty();

protected:
    void openFile(const QFile &file);
    void load (const QString & fileName);

protected:
    TechDraw::DrawSVGTemplate * getSVGTemplate();
    QGraphicsSvgItem *m_svgItem;
    QSvgRenderer *m_svgRender;
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};  // class QGISVGTemplate

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMSVGTEMPLATE_H
