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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Mod/TechDraw/App/Geometry.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "QGIView.h"


namespace TechDraw {
class DrawViewPart;
class DrawViewSection;
class DrawHatch;
class DrawGeomHatch;
class DrawViewDetail;
class DrawView;

}

namespace TechDrawGui
{
class QGIFace;
class QGIEdge;
class QGIHighlight;
class PathBuilder;

class TechDrawGuiExport QGIViewPart : public QGIView
{
public:

    explicit QGIViewPart();
    ~QGIViewPart() override;

    enum {Type = QGraphicsItem::UserType + 102};
    int type() const override { return Type;}
    void paint( QPainter * painter,
                        const QStyleOptionGraphicsItem * option,
                        QWidget * widget = nullptr ) override;


    void toggleCache(bool state) override;
    void toggleCosmeticLines(bool state);
    void setViewPartFeature(TechDraw::DrawViewPart *obj);
    void updateView(bool update = false) override;
    void tidy();
    QRectF boundingRect() const override;

    virtual void drawAllFaces();
    virtual void drawAllEdges();
    virtual void drawAllVertexes();

    bool showThisEdge(TechDraw::BaseGeomPtr geom);

    virtual void drawAllSectionLines();
    virtual void drawSectionLine(TechDraw::DrawViewSection* s, bool b);
    virtual void drawComplexSectionLine(TechDraw::DrawViewSection* viewSection, bool b);
    virtual void drawCenterLines(bool b);
    virtual void drawAllHighlights();
    virtual void drawHighlight(TechDraw::DrawViewDetail* viewDetail, bool b);
    virtual void drawMatting();
    bool showSection;

    void draw() override;
    void rotateView() override;

    virtual void highlightMoved(QGIHighlight* highlight, QPointF newPos);

    static QPainterPath geomToPainterPath(TechDraw::BaseGeomPtr baseGeom, double rotation = 0.0);
    /// Helper for pathArc()
    /*!
     * x_axis_rotation is in radian
     */
    static void pathArcSegment(QPainterPath &path, double xc, double yc, double th0,
                        double th1, double rx, double ry, double xAxisRotation);

    /// Draws an arc using QPainterPath path
    /*!
     * x_axis_rotation is in radian
     */
    static void pathArc(QPainterPath &path, double rx, double ry, double x_axis_rotation,
                                     bool large_arc_flag, bool sweep_flag,
                                     double x, double y,
                                     double curx, double cury);
    void setExporting(bool b) { m_isExporting = b; }
    bool getExporting() { return m_isExporting; }

protected:
    QPainterPath drawPainterPath(TechDraw::BaseGeomPtr baseGeom) const;
    void drawViewPart();
    QGIFace* drawFace(TechDraw::FacePtr f, int idx);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    TechDraw::DrawHatch* faceIsHatched(int i, std::vector<TechDraw::DrawHatch*> hatchObjs) const;
    TechDraw::DrawGeomHatch* faceIsGeomHatched(int i, std::vector<TechDraw::DrawGeomHatch*> geomObjs) const;
    void dumpPath(const char* text, QPainterPath path);
    void removePrimitives();
    void removeDecorations();
    bool prefFaceEdges();
    bool prefPrintCenters();

    bool formatGeomFromCosmetic(std::string cTag, QGIEdge* item);
    bool formatGeomFromCenterLine(std::string cTag, QGIEdge* item);

    bool m_isExporting;

private:
    QList<QGraphicsItem*> deleteItems;
    PathBuilder* m_pathBuilder;
};

} // namespace

#endif // DRAWINGGUI_QGRAPHICSITEMVIEWPART_H
