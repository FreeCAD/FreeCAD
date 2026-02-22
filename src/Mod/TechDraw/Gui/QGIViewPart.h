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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/LineGenerator.h>

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "QGIView.h"
#include "QGIUserTypes.h"

class QColor;

namespace App {
class Color;
}

namespace TechDraw {
class DrawViewPart;
class DrawViewSection;
class DrawHatch;
class DrawGeomHatch;
class DrawViewDetail;
class DrawView;
class LineGenerator;
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

    enum {Type = UserType::QGIViewPart};
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
    virtual void drawBreakLines();
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

    void addToGroupWithoutUpdate(QGraphicsItem* item);
    bool getGroupSelection() override;
    void setGroupSelection(bool isSelected) override;
    void setGroupSelection(bool isSelected, const std::vector<std::string> &subNames) override;

    virtual QGraphicsItem *getQGISubItemByName(const std::string &subName) const;

    virtual bool removeSelectedCosmetic() const;

    virtual double getLineWidth();
    virtual double getVertexSize();

    bool hideCenterMarks() const;

    void setMovableFlag() override;
    void setMovableFlagProjGroupItem();

protected:
    bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
    QPainterPath drawPainterPath(TechDraw::BaseGeomPtr baseGeom) const;
    void drawViewPart();
    QGIFace* drawFace(TechDraw::FacePtr f, int idx);

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    TechDraw::DrawHatch* faceIsHatched(int i, std::vector<TechDraw::DrawHatch*> hatchObjs) const;
    TechDraw::DrawGeomHatch* faceIsGeomHatched(int i, std::vector<TechDraw::DrawGeomHatch*> geomObjs) const;
    void dumpPath(const char* text, QPainterPath path);
    void removePrimitives();
    void removeDecorations();
    bool prefFaceEdges();
    Base::Color prefBreaklineColor();

    bool formatGeomFromCosmetic(std::string cTag, QGIEdge* item);
    bool formatGeomFromCenterLine(std::string cTag, QGIEdge* item);

    bool showCenterMarks() const;
    bool showVertices() const;

private:
    QList<QGraphicsItem*> deleteItems;
    PathBuilder* m_pathBuilder;
    TechDraw::LineGenerator* m_dashedLineGenerator;
    QMetaObject::Connection m_selectionChangedConnection;

};

} // namespace