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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QByteArray>
#include <QPixmap>
#include <QSvgRenderer>

#include <App/Material.h>

#include <Mod/TechDraw/App/HatchLine.h>

#include "PATPathMaker.h"
#include "QGIPrimPath.h"


namespace TechDrawGui
{
class QGCustomSvg;
class QGCustomRect;
class QGCustomImage;

    constexpr int SVGSIZEW{64};                     //width and height of standard FC SVG pattern
    constexpr int SVGSIZEH{64};
    constexpr uint32_t COLWHITE{0xfffff};       // white
    constexpr int ALPHALOW{0};
    constexpr int ALPHAHIGH{255};
    constexpr long int MAXSEGMENT{10000L};
    constexpr long int MAXTILES{10000L};
    const std::string SVGCOLDEFAULT = "#000000";  // black

class QGIFace : public QGIPrimPath
{
public:
    explicit QGIFace(int index = -1);
    ~QGIFace() override;

    enum {Type = QGraphicsItem::UserType + 104};
    int type() const override { return Type;}
    QRectF boundingRect() const override;
    QPainterPath shape() const override;

public:
    enum class FillMode {
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
    void setPrettyNormal() override;
    void setPrettyPre() override;
    void setPrettySel() override;
    void setDrawEdges(bool state);
    virtual void setOutline(const QPainterPath& path);

    //shared fill parms
    void isHatched(bool state) {m_isHatched = state; }
    bool isHatched() {return m_isHatched;}
    void setFillMode(FillMode mode);

    //general hatch parms & methods
    void setHatchColor(Base::Color color);
    void setHatchScale(double scale);

    //svg fill parms & methods
    void setHatchFile(std::string fileSpec);
    void loadSvgHatch(std::string fileSpec);
    void buildSvgHatch();
    void hideSvg(bool state);
    void clearSvg();

    //tiled pixmap fill from svg
    void buildPixHatch();

    //PAT fill parms & methods
    void setGeomHatchWeight(double weight) { m_geomWeight = weight; }
    void setLineWeight(double weight);

    void clearLineSets();
    void addLineSet(TechDraw::LineSet& ls);
    void clearFillItems();

    void lineSetToFillItems(TechDraw::LineSet& ls);
    QGraphicsPathItem* geomToLine(TechDraw::BaseGeomPtr base, TechDraw::LineSet& ls);
    QGraphicsPathItem* geomToStubbyLine(TechDraw::BaseGeomPtr base, double offset, TechDraw::LineSet& ls);
    QGraphicsPathItem* lineFromPoints(Base::Vector3d start, Base::Vector3d end, TechDraw::DashSpec ds);

    QPixmap textureFromBitmap(std::string fileSpec) const;

    //Qt uses clockwise degrees
    void setHatchRotation(double degrees) { m_hatchRotation = -degrees; }
    double getHatchRotation() const { return -m_hatchRotation; }

    void setHatchOffset(Base::Vector3d offset) { m_hatchOffset = offset; }
    Base::Vector3d getHatchOffset() { return m_hatchOffset; }

protected:
    void makeMark(double x, double y);  // NOLINT readability-identifier-length
    double getXForm();
    void getParameters();

    std::vector<double> offsetDash(const std::vector<double> dv, const double offset);
    QPainterPath dashedPPath(const std::vector<double> dv, const Base::Vector3d start, const Base::Vector3d end);
    double dashRemain(const std::vector<double> dv, const double offset);
    double calcOffset(TechDraw::BaseGeomPtr geom, TechDraw::LineSet ls);


    QPen setGeomPen();
    std::vector<double> decodeDashSpec(TechDraw::DashSpec dash);


    bool multiselectEligible() override { return true; }

    bool exporting() const;


private:
    std::vector<QGraphicsPathItem*> m_fillItems;
    std::vector<TechDraw::LineSet> m_lineSets;
    std::vector<TechDraw::DashSpec> m_dashSpecs;
    long int m_segCount{0};
    long int m_maxSeg{0};
    long int m_maxTile{0};

    bool m_hideSvgTiles{false};
    int projIndex;                              //index of face in Projection. -1 for SectionFace.

    QGCustomRect* m_svgHatchArea;
    QGCustomImage* m_imageSvgHatchArea;

    QByteArray m_svgXML;
    std::string m_svgCol{SVGCOLDEFAULT};
    std::string m_fileSpec;   //for svg & bitmaps

    double m_fillScale{1.0};
    bool m_isHatched{false};
    QGIFace::FillMode m_mode;
    QPixmap m_texture;                          //

    QPainterPath m_outline;                     //

    QPainterPath m_geomhatch;                  //crosshatch fill lines

    QColor m_geomColor;                        //color for crosshatch lines
    double m_geomWeight{0.5};                       //lineweight for crosshatch lines

    QColor m_defFaceColor;

    double m_hatchRotation{0.0};
    Base::Vector3d m_hatchOffset;

    QSvgRenderer *m_sharedRender;

    PATPathMaker* m_patMaker;

};

}
#endif // DRAWINGGUI_QGRAPHICSITEMFACE_H
