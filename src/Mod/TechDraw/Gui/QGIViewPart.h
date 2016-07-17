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

#ifndef DRAWINGGUI_QGRAPHICSITEMVIEWPART_H
#define DRAWINGGUI_QGRAPHICSITEMVIEWPART_H

#include <QObject>
#include <QPainter>

#include <Base/Parameter.h>
#include <Mod/TechDraw/App/Geometry.h>
#include "QGIView.h"

namespace TechDraw {
class DrawViewPart;
class DrawHatch;
}

namespace TechDrawGui
{
class QGIFace;

class TechDrawGuiExport QGIViewPart : public QGIView
{
public:

    explicit QGIViewPart();
    ~QGIViewPart();

    enum {Type = QGraphicsItem::UserType + 102};
    int type() const override { return Type;}


    void toggleCache(bool state) override;
    void toggleCosmeticLines(bool state);
    void toggleVertices(bool state);
    void setViewPartFeature(TechDraw::DrawViewPart *obj);
    virtual void updateView(bool update = false) override;
    void tidy();

    virtual void draw() override;
    virtual QRectF boundingRect() const override;

protected:
    /// Helper for pathArc()
    /*!
     * x_axis_rotation is in radian
     */
    void pathArcSegment(QPainterPath &path, double xc, double yc, double th0,
                        double th1,double rx, double ry, double xAxisRotation) const;

    /// Draws an arc using QPainterPath path
    /*!
     * x_axis_rotation is in radian
     */
    void pathArc(QPainterPath &path, double rx, double ry, double x_axis_rotation,
                                     bool large_arc_flag, bool sweep_flag,
                                     double x, double y,
                                     double curx, double cury) const;

    QPainterPath drawPainterPath(TechDrawGeometry::BaseGeom *baseGeom) const;
    void drawViewPart();
    QGIFace* drawFace(TechDrawGeometry::Face* f, int idx);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    TechDraw::DrawHatch* faceIsHatched(int i,std::vector<TechDraw::DrawHatch*> hatchObjs) const;
    void dumpPath(const char* text,QPainterPath path);
    void removePrimitives(void);

private:
    QList<QGraphicsItem*> deleteItems;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_QGRAPHICSITEMVIEWPART_H
