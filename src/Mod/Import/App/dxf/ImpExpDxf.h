/***************************************************************************
 *   Copyright (c) 2015 Yorik van Havre (yorik@uncreated.net)              *
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

#ifndef IMPEXPDXF_H
#define IMPEXPDXF_H

#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <Mod/Part/App/TopoShape.h>

#include "dxf.h"


class BRepAdaptor_Curve;

namespace Import
{
class ImportExport ImpExpDxfRead: public CDxfRead
{
public:
    ImpExpDxfRead(std::string filepath, App::Document* pcDoc);

    // CDxfRead's virtual functions
    void OnReadLine(const double* s, const double* e, bool hidden) override;
    void OnReadPoint(const double* s) override;
    void OnReadText(const double* point, const double height, const char* text) override;
    void
    OnReadArc(const double* s, const double* e, const double* c, bool dir, bool hidden) override;
    void OnReadCircle(const double* s, const double* c, bool dir, bool hidden) override;
    void OnReadEllipse(const double* c,
                       double major_radius,
                       double minor_radius,
                       double rotation,
                       double start_angle,
                       double end_angle,
                       bool dir) override;
    void OnReadSpline(struct SplineData& sd) override;
    void OnReadInsert(const double* point,
                      const double* scale,
                      const char* name,
                      double rotation) override;
    void OnReadDimension(const double* s,
                         const double* e,
                         const double* point,
                         double rotation) override;
    void AddGraphics() const override;

    // FreeCAD-specific functions
    void AddObject(Part::TopoShape* shape);  // Called by OnRead functions to add Part objects
    std::string Deformat(const char* text);  // Removes DXF formatting from texts

    std::string getOptionSource()
    {
        return m_optionSource;
    }
    void setOptionSource(std::string s)
    {
        m_optionSource = s;
    }
    void setOptions();

private:
    gp_Pnt makePoint(const double* p);

protected:
    App::Document* document;
    bool optionGroupLayers;
    bool optionImportAnnotations;
    double optionScaling;
    std::map<std::string, std::vector<Part::TopoShape*>> layers;
    std::string m_optionSource;
};

class ImportExport ImpExpDxfWrite: public CDxfWrite
{
public:
    explicit ImpExpDxfWrite(std::string filepath);
    ~ImpExpDxfWrite();

    void exportShape(const TopoDS_Shape input);
    std::string getOptionSource()
    {
        return m_optionSource;
    }
    void setOptionSource(std::string s)
    {
        m_optionSource = s;
    }
    void setOptions();

    void exportText(const char* text,
                    Base::Vector3d position1,
                    Base::Vector3d position2,
                    double size,
                    int just);
    void exportLinearDim(Base::Vector3d textLocn,
                         Base::Vector3d lineLocn,
                         Base::Vector3d extLine1Start,
                         Base::Vector3d extLine2Start,
                         char* dimText,
                         int type);
    void exportAngularDim(Base::Vector3d textLocn,
                          Base::Vector3d lineLocn,
                          Base::Vector3d extLine1Start,
                          Base::Vector3d extLine2Start,
                          Base::Vector3d apexPoint,
                          char* dimText);
    void exportRadialDim(Base::Vector3d centerPoint,
                         Base::Vector3d textLocn,
                         Base::Vector3d arcPoint,
                         char* dimText);
    void exportDiametricDim(Base::Vector3d textLocn,
                            Base::Vector3d arcPoint1,
                            Base::Vector3d arcPoint2,
                            char* dimText);


    static bool gp_PntEqual(gp_Pnt p1, gp_Pnt p2);
    static bool gp_PntCompare(gp_Pnt p1, gp_Pnt p2);

protected:
    void exportCircle(BRepAdaptor_Curve& c);
    void exportEllipse(BRepAdaptor_Curve& c);
    void exportArc(BRepAdaptor_Curve& c);
    void exportEllipseArc(BRepAdaptor_Curve& c);
    void exportBSpline(BRepAdaptor_Curve& c);
    void exportBCurve(BRepAdaptor_Curve& c);
    void exportLine(BRepAdaptor_Curve& c);
    void exportLWPoly(BRepAdaptor_Curve& c);  // LWPolyline not supported in R12?
    void exportPolyline(BRepAdaptor_Curve& c);

    //        std::string m_optionSource;
    double optionMaxLength;
    bool optionPolyLine;
    bool optionExpPoints;
};

}  // namespace Import

#endif  // IMPEXPDXF_H
