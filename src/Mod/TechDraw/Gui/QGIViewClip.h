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

#ifndef DRAWINGGUI_QGRAPHICSITEMCLIP_H
#define DRAWINGGUI_QGRAPHICSITEMCLIP_H

#include <QObject>
#include <QPainter>

#include "QGIView.h"

namespace TechDrawGui
{
class QGCustomRect;
class QGCustomClip;

class TechDrawGuiExport QGIViewClip : public QGIView
{
public:

    QGIViewClip();
    ~QGIViewClip() = default;

    enum {Type = QGraphicsItem::UserType + 123};
    int type() const override { return Type;}

    virtual void updateView(bool update = false) override;

    virtual void draw() override;
    QGCustomRect* getFrame(void) {return m_frame;}
    QGCustomClip* getClipArea(void) {return m_cliparea;}

protected:
    void drawClip();
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private:
    QGCustomRect* m_frame;
    QGCustomClip* m_cliparea;

};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMCLIP_H
