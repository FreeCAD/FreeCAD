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

#include <Handle_TDocStd_Document.hxx>
#include <Handle_XCAFDoc_ColorTool.hxx>
#include <Handle_XCAFDoc_ShapeTool.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS_Shape.hxx>
#include <climits>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <App/Material.h>

class TDF_Label;
class TopLoc_Location;

namespace App {
class Document;
class DocumentObject;
}
namespace Part {
class Feature;
}

namespace Import {

class ImportExport ImportOCAF
{
public:
    ImportOCAF(Handle_TDocStd_Document h, App::Document* d, const std::string& name);
    virtual ~ImportOCAF();
    void loadShapes();

private:
    void loadShapes(const TDF_Label& label, const TopLoc_Location&, const std::string& partname, const std::string& assembly, bool isRef);
    void createShape(const TDF_Label& label, const TopLoc_Location&, const std::string&);
    void createShape(const TopoDS_Shape& label, const TopLoc_Location&, const std::string&);
    virtual void applyColors(Part::Feature*, const std::vector<App::Color>&){}

private:
    Handle_TDocStd_Document pDoc;
    App::Document* doc;
    Handle_XCAFDoc_ShapeTool aShapeTool;
    Handle_XCAFDoc_ColorTool aColorTool;
    std::string default_name;
    std::set<int> myRefShapes;
    static const int HashUpper = INT_MAX;
};

class ImportExport ExportOCAF
{
public:
    ExportOCAF(Handle_TDocStd_Document h);
    void saveShape(Part::Feature* part, const std::vector<App::Color>&);

private:
    Handle_TDocStd_Document pDoc;
    Handle_XCAFDoc_ShapeTool aShapeTool;
    Handle_XCAFDoc_ColorTool aColorTool;
    TDF_Label rootLabel;
};


class ImportXCAF
{
public:
    ImportXCAF(Handle_TDocStd_Document h, App::Document* d, const std::string& name);
    virtual ~ImportXCAF();
    void loadShapes();

private:
    void createShape(const TopoDS_Shape& shape, bool perface=false, bool setname=false) const;
    void loadShapes(const TDF_Label& label);
    virtual void applyColors(Part::Feature*, const std::vector<App::Color>&){}

private:
    Handle_TDocStd_Document hdoc;
    App::Document* doc;
    Handle_XCAFDoc_ShapeTool aShapeTool;
    Handle_XCAFDoc_ColorTool hColors;
    std::string default_name;
    std::map<Standard_Integer, TopoDS_Shape> mySolids;
    std::map<Standard_Integer, TopoDS_Shape> myShells;
    std::map<Standard_Integer, TopoDS_Shape> myCompds;
    std::map<Standard_Integer, TopoDS_Shape> myShapes;
    std::map<Standard_Integer, Quantity_Color> myColorMap;
    std::map<Standard_Integer, std::string> myNameMap;
};

}

#endif //IMPORT_IMPORTOCAF_H
