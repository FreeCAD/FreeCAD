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
#include <QImage>

#include <Mod/TechDraw/App/HatchLine.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "QGIPrimPath.h"


namespace TechDrawGui
{
class QGCustomSvg;
class QGCustomRect;
class QGCustomImage;

    const double SVGSIZEW = 64.0;                     //width and height of standard FC SVG pattern
    const double SVGSIZEH = 64.0;
    const std::string SVGCOLDEFAULT = "#000000";

class QGIFace : public QGIPrimPath
{
public:
    explicit QGIFace(int index = -1);
    ~QGIFace();

    enum {Type = QGraphicsItem::UserType + 104};
    int type() const override { return Type;}
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    virtual void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = nullptr ) override;

public:
    enum fillMode {
        NoFill,
        FromFile,
        SvgFill,
        BitmapFill,
        GeomHatchFill,
        PlainFill
    };
    std::string SVGCOLPREFIX = ""; // will be determined on runtime

    int getProjIndex() const { return projIndex; }

    void draw();
    virtual void setPrettyNormal() override;
    virtual void setPrettyPre() override;
    virtual void setPrettySel() override;
    void setDrawEdges(bool b);
    virtual void setOutline(const QPainterPath& path);
 
    //shared fill parms
    void isHatched(bool s) {m_isHatched = s; }
    bool isHatched(void) {return m_isHatched;}
    void setFillMode(fillMode m);

    //general hatch parms & methods
    void setHatchColor(App::Color c);
    void setHatchScale(double s);
    
    //svg fill parms & methods
    void setHatchFile(std::string fileSpec);
    void loadSvgHatch(std::string fileSpec);
    void buildSvgHatch(void);
    void hideSvg(bool b);
    void clearSvg(void);

    //tiled pixmap fill from svg
    void buildPixHatch();

    //PAT fill parms & methods
    void setGeomHatchWeight(double w) { m_geomWeight = w; }
    void setLineWeight(double w);

    void clearLineSets(void);
    void addLineSet(TechDraw::LineSet& ls);
    void clearFillItems(void);

    void lineSetToFillItems(TechDraw::LineSet& ls);
    QGraphicsPathItem* geomToLine(TechDraw::BaseGeomPtr base, TechDraw::LineSet& ls);
//    QGraphicsPathItem* geomToOffsetLine(TechDraw::BaseGeomPtr base, double offset, const TechDraw::LineSet& ls);
    QGraphicsPathItem* geomToStubbyLine(TechDraw::BaseGeomPtr base, double offset, TechDraw::LineSet& ls);
    QGraphicsPathItem* lineFromPoints(Base::Vector3d start, Base::Vector3d end, TechDraw::DashSpec ds);

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
    double calcOffset(TechDraw::BaseGeomPtr g,TechDraw::LineSet ls);
    int projIndex;                              //index of face in Projection. -1 for SectionFace.
    QGCustomRect *m_rect;

    QByteArray m_svgXML;
    std::string m_svgCol;
    std::string m_fileSpec;   //for svg & bitmaps

    QGCustomImage* m_image;

    double m_fillScale;
    bool m_isHatched;
    QGIFace::fillMode m_mode;

    QPen setGeomPen(void);
    std::vector<double> decodeDashSpec(TechDraw::DashSpec d);
    std::vector<QGraphicsPathItem*> m_fillItems;
    std::vector<TechDraw::LineSet> m_lineSets;
    std::vector<TechDraw::DashSpec> m_dashSpecs;
    long int m_segCount;
    long int m_maxSeg;
    long int m_maxTile;

    bool m_hideSvgTiles;


private:
    QPixmap m_texture;                          //
 
    QPainterPath m_outline;                     //
 
    QPainterPath m_geomhatch;                  //crosshatch fill lines
 
    QColor m_geomColor;                        //color for crosshatch lines
    double m_geomWeight;                       //lineweight for crosshatch lines
    bool m_defClearFace;
    QColor m_defFaceColor;
};

}
#endif // DRAWINGGUI_QGRAPHICSITEMFACE_H
