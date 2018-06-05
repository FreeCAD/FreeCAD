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
#include <TDF_LabelMapHasher.hxx>
#include <climits>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
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

struct ShapeHasher {
    std::size_t operator()(const TopoDS_Shape &s) const {
        return s.HashCode(INT_MAX);
    }
};

struct LabelHasher {
    std::size_t operator()(const TDF_Label &l) const {
        return TDF_LabelMapHasher::HashCode(l,INT_MAX);
    }
};

class ImportExport ImportOCAF
{
public:
    ImportOCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name);
    virtual ~ImportOCAF();
    App::DocumentObject* loadShapes();
    void setMerge(bool enable) { merge=enable;};
    void setUseLinkGroup(bool enable) { useLinkGroup=enable; }
    void setImportHiddenObject(bool enable) {importHidden=enable;}

private:
    struct Info {
        App::DocumentObject *obj = 0;
        App::PropertyPlacement *propPlacement = 0;
        App::Color faceColor;
        App::Color edgeColor;
        bool hasFaceColor = false;
        bool hasEdgeColor = false;
        int free = true;
    };

    App::DocumentObject *loadShape(TDF_Label label, const TopoDS_Shape &shape, bool isArrayElement=false);
    bool createAssembly(TDF_Label label, const TopoDS_Shape &shape, Info &info);
    bool createObject(TDF_Label label, const TopoDS_Shape &shape, Info &info);
    bool createGroup(Info &info, const TopoDS_Shape &shape,
        const std::vector<App::DocumentObject*> &children, const boost::dynamic_bitset<> &visibilities);
    bool getColor(const TopoDS_Shape &shape, Info &info, bool check=false, bool noDefault=false);
    void getSHUOColors(TDF_Label label, std::map<std::string,App::Color> &colors, bool appendFirst);

    virtual void applyEdgeColors(Part::Feature*, const std::vector<App::Color>&) {}
    virtual void applyFaceColors(Part::Feature*, const std::vector<App::Color>&) {}
    virtual void applyElementColors(App::DocumentObject*, const std::map<std::string,App::Color>&) {}
    virtual void applyLinkColor(App::DocumentObject *, int /*index*/, App::Color){}

private:
    Handle(TDocStd_Document) pDoc;
    App::Document* doc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;
    bool merge;
    std::string default_name;
    bool useLinkGroup;
    bool importHidden;

    std::unordered_map<TopoDS_Shape, Info, ShapeHasher> myShapes;

    std::unordered_map<TDF_Label, std::string, LabelHasher> myNames;

    App::Color defaultFaceColor;
    App::Color defaultEdgeColor;
};

class ImportExport ExportOCAF
{
public:
    typedef std::function<std::map<std::string,App::Color>(
            App::DocumentObject*, const char*)> GetShapeColorsFunc;
    ExportOCAF(Handle(TDocStd_Document) h, GetShapeColorsFunc func=GetShapeColorsFunc());

    void setExportHiddenObject(bool enable) {exportHidden=enable;}
    void exportObjects(std::vector<App::DocumentObject*> &objs, const char *name=0);

private:
    TDF_Label exportObject(App::DocumentObject *obj, const char *sub, TDF_Label parent, const char *name=0);
    void setupObject(TDF_Label label, App::DocumentObject *obj, 
            const Part::TopoShape &shape, const std::string &prefix, const char *name=0);
    void setName(TDF_Label label, App::DocumentObject *obj, const char *name=0);
    TDF_Label findComponent(const char *subname, TDF_Label label, TDF_LabelSequence &labels);

private:
    Handle(TDocStd_Document) pDoc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;

    std::map<App::DocumentObject *, TDF_Label> myObjects;

    std::unordered_map<TDF_Label, std::vector<std::string>, LabelHasher> myNames;

    std::set<std::pair<App::DocumentObject*,std::string> > mySetups;

    GetShapeColorsFunc getShapeColors;

    App::Color defaultColor;
    bool exportHidden;
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
