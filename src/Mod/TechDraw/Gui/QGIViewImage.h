/***************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEWIMAGE_H
#define DRAWINGGUI_QGRAPHICSITEMVIEWIMAGE_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include "QGIView.h"

namespace TechDraw {
class DrawViewImage;
}

namespace TechDrawGui
{
class QGCustomImage;
class QGCustomClip;

class TechDrawGuiExport QGIViewImage : public QGIView
{
public:
    QGIViewImage();
    ~QGIViewImage() override;

    enum {Type = QGraphicsItem::UserType + 200};
    int type() const override { return Type;}

    void updateView(bool update = false) override;
    void setViewImageFeature(TechDraw::DrawViewImage *obj);

    void draw() override;
    void rotateView() override;

protected:
    virtual void drawImage();

    QGCustomImage* m_imageItem;
    QGCustomClip*  m_cliparea;
};

} // namespace
#endif // DRAWINGGUI_QGRAPHICSITEMVIEWIMAGE_H
