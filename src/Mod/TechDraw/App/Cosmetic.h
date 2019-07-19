/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef TECHDRAW_COSMETIC_H
#define TECHDRAW_COSMETIC_H

#include <App/FeaturePython.h>

#include <Base/Persistence.h>
#include <Base/Vector3D.h>
#include <App/Material.h>

#include "Geometry.h"

class TopoDS_Edge;

namespace TechDraw {
class DrawViewPart;

class TechDrawExport LineFormat
{
public:
    LineFormat();
    LineFormat(int style,
               double weight,
               App::Color color,
               bool visible );
    ~LineFormat() = default;

    int m_style;
    double m_weight;
    App::Color m_color;
    bool m_visible;

    static double getDefEdgeWidth();
    static App::Color getDefEdgeColor();
    static int getDefEdgeStyle();

    void dump(char* title);
    std::string toString() const;
/*    bool fromCSV(std::string& lineSpec);*/
};

class TechDrawExport CosmeticVertex: public Base::Persistence, public TechDraw::Vertex
{
    TYPESYSTEM_HEADER();

public:
    CosmeticVertex();
    CosmeticVertex(const CosmeticVertex* cv);
    CosmeticVertex(Base::Vector3d loc);
    virtual ~CosmeticVertex() = default;

    std::string toString(void) const;
/*    bool fromCSV(std::string& lineSpec);*/
    void dump(char* title);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);

    virtual PyObject *getPyObject(void);
    CosmeticVertex* copy(void) const;
    CosmeticVertex* clone(void) const;

    int            linkGeom;             //connection to corresponding "real" Vertex
    App::Color     color;
    double         size;
    int            style;
    bool           visible;

protected:

};

class TechDrawExport CosmeticEdge : public Base::Persistence
{
    TYPESYSTEM_HEADER();
public:
    CosmeticEdge();
    CosmeticEdge(CosmeticEdge* ce);
    CosmeticEdge(Base::Vector3d p1, Base::Vector3d p2);
    CosmeticEdge(TopoDS_Edge e);
    CosmeticEdge(TechDraw::BaseGeom* g);
    virtual ~CosmeticEdge();

    void initialize(void);
    TechDraw::BaseGeom* scaledGeometry(double scale);

    virtual std::string toString(void) const;
/*    virtual bool fromCSV(std::string& lineSpec);*/
    void dump(char* title);

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);

    virtual PyObject *getPyObject(void);
    CosmeticEdge* copy(void) const;
    CosmeticEdge* clone(void) const;

    TechDraw::BaseGeom* m_geometry;
    LineFormat m_format;

protected:

};

class TechDrawExport CenterLine: public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    CenterLine();
    CenterLine(CenterLine* cl);
    //set m_faces after using next 2 ctors
    CenterLine(Base::Vector3d p1, Base::Vector3d p2);
    CenterLine(Base::Vector3d p1, Base::Vector3d p2,
               int m, 
               double h,
               double v,
               double r,
               double x);
    virtual ~CenterLine();

    enum CLMODE {
        VERTICAL,
        HORIZONTAL,
        ALIGNED
    };

    enum CLTYPE {
        FACE,
        EDGE,
        VERTEX
    };


    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);

    virtual PyObject *getPyObject(void);
    CenterLine* copy(void) const;
    CenterLine* clone(void) const;

    std::string toString(void) const;

    static CenterLine* CenterLineBuilder(TechDraw::DrawViewPart* partFeat,
                                         std::vector<std::string> subs,
                                         int mode = 0,
                                         bool flip = false);
    TechDraw::BaseGeom* scaledGeometry(TechDraw::DrawViewPart* partFeat);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints(
                                          TechDraw::DrawViewPart* partFeat,
                                          std::vector<std::string> faceNames,
                                          int mode, double ext,
                                          double m_hShift, double m_vShift,
                                          double rotate);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints2Lines(
                                          TechDraw::DrawViewPart* partFeat,
                                          std::vector<std::string> faceNames,
                                          int vert, double ext,
                                          double m_hShift, double m_vShift,
                                          double rotate, bool flip);
    static std::pair<Base::Vector3d, Base::Vector3d> calcEndPoints2Points(
                                          TechDraw::DrawViewPart* partFeat,
                                          std::vector<std::string> faceNames,
                                          int vert, double ext,
                                          double m_hShift, double m_vShift,
                                          double rotate, bool flip);
    void dump(char* title);
    void setShifts(double h, double v);
    double getHShift(void);
    double getVShift(void);
    void setRotate(double r);
    double getRotate(void);
    void setExtend(double e);
    double getExtend(void);
    void setFlip(bool f);
    bool getFlip(void);

    Base::Vector3d m_start;
    Base::Vector3d m_end;
    std::vector<std::string> m_faces;
    std::vector<std::string> m_edges;
    std::vector<std::string> m_verts;
    int m_type;          // 0 - face, 1 - 2 line, 2 - 2 point
    int m_mode;          // 0 - vert/ 1 - horiz/ 2 - aligned
    double m_hShift;
    double m_vShift;
    double m_rotate;
    double m_extendBy;
    LineFormat m_format;
    bool m_flip2Line;

protected:

};

class TechDrawExport GeomFormat: public Base::Persistence
{
    TYPESYSTEM_HEADER();

public:
    GeomFormat();
    GeomFormat(TechDraw::GeomFormat* gf);
    GeomFormat(int idx,
               LineFormat fmt);
    ~GeomFormat();

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);

    virtual PyObject *getPyObject(void);
    GeomFormat* copy(void) const;
    GeomFormat* clone(void) const;

    std::string toString(void) const;
/*    bool fromCSV(std::string& lineSpec);*/
    void dump(char* title) const;

    int m_geomIndex;
    LineFormat m_format;

protected:

};

} //end namespace TechDraw

#endif //TECHDRAW_COSMETIC_H
