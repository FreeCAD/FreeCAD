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


#ifndef IMPORT_EXPORTOCAF_H
#define IMPORT_EXPORTOCAF_H

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
#include <Mod/Import/ImportGlobal.h>


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

class ImportExport ExportOCAF
{
public:
    ExportOCAF(Handle(TDocStd_Document) h, bool explicitPlacement);
    virtual ~ExportOCAF();
    int exportObject(App::DocumentObject* obj,
                     std::vector <TDF_Label>& hierarchical_label,
                     std::vector <TopLoc_Location>& hierarchical_loc,
                     std::vector <App::DocumentObject*>& hierarchical_part);
    int saveShape(Part::Feature* part, const std::vector<App::Color>&,
                  std::vector <TDF_Label>& hierarchical_label,
                  std::vector <TopLoc_Location>& hierarchical_loc,
                  std::vector <App::DocumentObject*>& hierarchical_part);
    void getPartColors(std::vector <App::DocumentObject*> hierarchical_part,
                       std::vector <TDF_Label> FreeLabels,
                       std::vector <int> part_id,
                       std::vector < std::vector<App::Color> >& Colors) const;
    void reallocateFreeShape(std::vector <App::DocumentObject*> hierarchical_part,
                             std::vector <TDF_Label> FreeLabels,
                             std::vector <int> part_id,
                             std::vector< std::vector<App::Color> >& Colors);
    void getFreeLabels(std::vector <TDF_Label>& hierarchical_label,
                       std::vector <TDF_Label>& labels,
                       std::vector <int>& label_part_id);
    void createNode(App::Part* part, int& root_it,
                    std::vector <TDF_Label>& hierarchical_label,
                    std::vector <TopLoc_Location>& hierarchical_loc,
                    std::vector <App::DocumentObject*>& hierarchical_part);
    void pushNode(int root, int node, std::vector <TDF_Label>& hierarchical_label,
                  std::vector <TopLoc_Location>& hierarchical_loc);

private:
    virtual void findColors(Part::Feature*, std::vector<App::Color>&) const {}
    std::vector<App::DocumentObject*> filterPart(App::Part* part) const;

private:
    Handle(TDocStd_Document) pDoc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;
    TDF_Label rootLabel;
    bool keepExplicitPlacement;
    bool filterBaseFeature;
};

class ImportExport ExportOCAFCmd : public ExportOCAF
{
public:
    ExportOCAFCmd(Handle(TDocStd_Document) h, bool explicitPlacement);
    void setPartColorsMap(const std::map<Part::Feature*, std::vector<App::Color> >& colors) {
        partColors = colors;
    }

private:
    virtual void findColors(Part::Feature*, std::vector<App::Color>&) const;

private:
    std::map<Part::Feature*, std::vector<App::Color> > partColors;
};


}

#endif //IMPORT_EXPORTOCAF_H
