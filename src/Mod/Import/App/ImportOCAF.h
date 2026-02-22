// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef IMPORT_IMPORTOCAF_H
#define IMPORT_IMPORTOCAF_H

#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <Quantity_ColorRGBA.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Shape.hxx>
#include <Standard_Version.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <App/Part.h>
#include <Mod/Import/ImportGlobal.h>


class TDF_Label;
class TopLoc_Location;

namespace App
{
class Document;
class DocumentObject;
}  // namespace App
namespace Part
{
class Feature;
}

#if (OCC_VERSION_MAJOR < 7 || (OCC_VERSION_MAJOR == 7 && OCC_VERSION_MINOR < 8))
// Older versions of OCCT do not provide this template specialization so we provide it here.
// On the other hand, newer versions do not have TopoDS_Shape::HashCode() so we can't always use
// this code in a custom hash class explicitly passed to the unordered_map template.
namespace std
{
template<>
struct hash<TopoDS_Shape>
{
    size_t operator()(const TopoDS_Shape& theShape) const noexcept
    {
        return theShape.HashCode(std::numeric_limits<size_t>::max());
    }
};
}  // namespace std
#endif


namespace Import
{

class ImportExport ImportOCAF
{
public:
    ImportOCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name);
    virtual ~ImportOCAF();
    void loadShapes();
    void setMerge(bool);

private:
    App::DocumentObject* loadShapes(
        const TDF_Label& label,
        const TopLoc_Location&,
        const std::string& partname,
        const std::string& assembly,
        bool isRef
    );
    App::DocumentObject* createShape(const TDF_Label& label, const TopLoc_Location&, const std::string&);
    App::DocumentObject* createShape(
        const TopoDS_Shape& shape,
        const TDF_Label& labelHint,
        const TopLoc_Location&,
        bool setPlacementFromLocation,
        const std::string&
    );
    virtual void applyColors(Part::Feature*, const std::vector<Base::Color>&)
    {}
    static void tryPlacementFromLoc(App::GeoFeature*, const TopLoc_Location&);
    bool getShapeColour(const TopoDS_Shape& shape, TDF_Label labelHint, Base::Color& foundColour);

private:
    Handle(TDocStd_Document) pDoc;
    App::Document* doc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;
    bool merge {true};
    std::string default_name;
    std::set<int> myRefShapes;
    std::unordered_map<TopoDS_Shape, TDF_Label> shapeToLabelMap;
};

class ImportExport ImportOCAFCmd: public ImportOCAF
{
public:
    ImportOCAFCmd(Handle(TDocStd_Document) h, App::Document* d, const std::string& name);
    std::map<Part::Feature*, std::vector<Base::Color>> getPartColorsMap() const
    {
        return partColors;
    }

private:
    void applyColors(Part::Feature* part, const std::vector<Base::Color>& colors) override;

private:
    std::map<Part::Feature*, std::vector<Base::Color>> partColors;
};

class ImportXCAF
{
public:
    ImportXCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name);
    virtual ~ImportXCAF();
    void loadShapes();

private:
    void createShape(const TopoDS_Shape& shape, bool perface = false, bool setname = false) const;
    void loadShapes(const TDF_Label& label);
    virtual void applyColors(Part::Feature*, const std::vector<Base::Color>&)
    {}

private:
    Handle(TDocStd_Document) hdoc;
    App::Document* doc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) hColors;
    std::string default_name;
    std::map<Standard_Integer, TopoDS_Shape> mySolids;
    std::map<Standard_Integer, TopoDS_Shape> myShells;
    std::map<Standard_Integer, TopoDS_Shape> myCompds;
    std::map<Standard_Integer, TopoDS_Shape> myShapes;
    std::map<Standard_Integer, Quantity_ColorRGBA> myColorMap;
    std::map<Standard_Integer, std::string> myNameMap;
};

}  // namespace Import

#endif  // IMPORT_IMPORTOCAF_H
