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

#include <TDocStd_Document.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS_Shape.hxx>
#include <climits>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <App/Material.h>
#include <App/Part.h>
#include <Mod/Part/App/FeatureCompound.h>


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
    ImportOCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name);
    virtual ~ImportOCAF();
    void loadShapes();
    void setMerge(bool);

private:
    void loadShapes(const TDF_Label& label, const TopLoc_Location&, const std::string& partname, const std::string& assembly, bool isRef, std::vector<App::DocumentObject*> &);
    void createShape(const TDF_Label& label, const TopLoc_Location&, const std::string&, std::vector<App::DocumentObject*> &, bool);
    void createShape(const TopoDS_Shape& label, const TopLoc_Location&, const std::string&, std::vector<App::DocumentObject*> &);
    virtual void applyColors(Part::Feature*, const std::vector<App::Color>&){}

private:
    Handle(TDocStd_Document) pDoc;
    App::Document* doc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;
    bool merge;
    std::string default_name;
    std::set<int> myRefShapes;
    static const int HashUpper = INT_MAX;
};

class ImportExport ExportOCAF
{
public:
    void createNode(App::Part* part, int& root_it,
                    std::vector <TDF_Label>& hierarchical_label,
                    std::vector <TopLoc_Location>& hierarchical_loc,
                    std::vector <App::DocumentObject*>& hierarchical_part);
    ExportOCAF(Handle(TDocStd_Document) h, bool explicitPlacement);
    int saveShape(Part::Feature* part, const std::vector<App::Color>&,
                  std::vector <TDF_Label>& hierarchical_label,
                  std::vector <TopLoc_Location>& hierarchical_loc,
                  std::vector <App::DocumentObject*>& hierarchical_part);
    void reallocateFreeShape(std::vector <App::DocumentObject*> hierarchical_part,
                             std::vector <TDF_Label> FreeLabels,
                             std::vector <int> part_id,
                             std::vector< std::vector<App::Color> >& Colors);
    void getFreeLabels(std::vector <TDF_Label>& hierarchical_label,
                       std::vector <TDF_Label>& labels,
                       std::vector <int>& label_part_id);
    void pushNode(int root, int node, std::vector <TDF_Label>& hierarchical_label,
                  std::vector <TopLoc_Location>& hierarchical_loc);

private:
    Handle(TDocStd_Document) pDoc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;
    TDF_Label rootLabel;
    bool keepExplicitPlacement;
};


class ImportXCAF
{
public:
    ImportXCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name);
    virtual ~ImportXCAF();
    void loadShapes();

private:
    void createShape(const TopoDS_Shape& shape, bool perface=false, bool setname=false) const;
    void loadShapes(const TDF_Label& label);
    virtual void applyColors(Part::Feature*, const std::vector<App::Color>&){}

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
    std::map<Standard_Integer, Quantity_Color> myColorMap;
    std::map<Standard_Integer, std::string> myNameMap;
};

}

#endif //IMPORT_IMPORTOCAF_H
