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

#ifndef DRAWINGGUI_QGRAPHICSITEMFACE_H
#define DRAWINGGUI_QGRAPHICSITEMFACE_H

#include <Qt>
#include <QGraphicsItem>
#include <QSvgRenderer>
#include <QByteArray>
#include <QBrush>
#include <QPixmap>
//#include <QVector>

#include <Mod/TechDraw/App/HatchLine.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "QGIPrimPath.h"

using namespace TechDraw;

namespace TechDrawGui
{
class QGCustomSvg;
class QGCustomRect;

    const double SVGSIZEW = 64.0;                     //width and height of standard FC SVG pattern
    const double SVGSIZEH = 64.0;
    const std::string  SVGCOLPREFIX = "stroke:";
    const std::string  SVGCOLDEFAULT = "#000000";

class QGIFace : public QGIPrimPath
{
public:
    explicit QGIFace(int index = -1);
    ~QGIFace();

    enum {Type = QGraphicsItem::UserType + 104};
    int type() const { return Type;}
    QRectF boundingRect() const;
    QPainterPath shape() const;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

public:
    enum fillMode {
        NoFill,
        FromFile,
        SvgFill,
        BitmapFill,
        GeomHatchFill,
        PlainFill
    };


    int getProjIndex() const { return projIndex; }

    void draw();
    void setPrettyNormal();
    void setPrettyPre();
    void setPrettySel();
    void setDrawEdges(bool b);
    virtual void setOutline(const QPainterPath& path);
 
    //shared fill parms
    void isHatched(bool s) {m_isHatched = s; }
    bool isHatched(void) {return m_isHatched;}
    void setFillMode(fillMode m);

    //plain color fill parms
    void setFill(QColor c, Qt::BrushStyle s);
    void setFill(QBrush b);
    void resetFill();

    //general hatch parms & methods
    void setHatchColor(App::Color c);
    void setHatchScale(double s);
    
    //svg fill parms & methods
    void setHatchFile(std::string fileSpec);
    void loadSvgHatch(std::string fileSpec);
    void buildSvgHatch(void);
    void toggleSvg(bool b);
    void clearSvg(void);
    
    //PAT fill parms & methods
    void setGeomHatchWeight(double w) { m_geomWeight = w; }
    void setLineWeight(double w);

    void clearLineSets(void);
    void addLineSet(LineSet& ls);
    void clearFillItems(void);

    void lineSetToFillItems(LineSet& ls);
    QGraphicsPathItem* geomToLine(TechDraw::BaseGeom* base,LineSet& ls);
//    QGraphicsPathItem* geomToOffsetLine(TechDraw::BaseGeom* base, double offset, const LineSet& ls);
    QGraphicsPathItem* geomToStubbyLine(TechDraw::BaseGeom* base, double offset, LineSet& ls);
    QGraphicsPathItem* lineFromPoints(Base::Vector3d start, Base::Vector3d end, DashSpec ds);

    //bitmap texture fill parms method
    QPixmap textureFromBitmap(std::string fileSpec);
    QPixmap textureFromSvg(std::string fillSpec);

protected:
    void makeMark(double x, double y);
    double getXForm(void);
    void getParameters(void);

    std::vector<double> offsetDash(const std::vector<double> dv, const double offset);
    QPainterPath dashedPPath(const std::vector<double> dv, const Base::Vector3d start, const Base::Vector3d end);
    double dashRemain(const std::vector<double> dv, const double offset);
    double calcOffset(TechDraw::BaseGeom* g,LineSet ls);
    int projIndex;                              //index of face in Projection. -1 for SectionFace.
    QGCustomRect *m_rect;

    QGCustomSvg *m_svg;
    QByteArray m_svgXML;
    std::string m_svgCol;
    std::string m_fileSpec;   //for svg & bitmaps

    double m_fillScale;
    bool m_isHatched;
    QGIFace::fillMode m_mode;

    QPen setGeomPen(void);
    std::vector<double> decodeDashSpec(DashSpec d);
    std::vector<QGraphicsPathItem*> m_fillItems;
    std::vector<LineSet> m_lineSets;
    std::vector<DashSpec> m_dashSpecs;
    long int m_segCount;
    long int m_maxSeg;
    long int m_maxTile;


private:
    QBrush m_brush;
    Qt::BrushStyle m_fillStyle;                 //current fill style
    QColor m_fillColor;                         //current fill color

    QColor m_colDefFill;                        //"no color" default normal fill color
    QColor m_colNormalFill;                     //current Normal fill color
    Qt::BrushStyle m_styleDef;                  //default Normal fill style
    Qt::BrushStyle m_styleNormal;               //current Normal fill style
    Qt::BrushStyle m_styleSelect;               //Select/preSelect fill style
 
    QPixmap m_texture;                          //
 
    QPainterPath m_outline;                     //
 
    QPainterPath m_geomhatch;                  //crosshatch fill lines
 
    QColor m_geomColor;                        //color for crosshatch lines
    double m_geomWeight;                       //lineweight for crosshatch lines
};

}
#endif // DRAWINGGUI_QGRAPHICSITEMFACE_H
