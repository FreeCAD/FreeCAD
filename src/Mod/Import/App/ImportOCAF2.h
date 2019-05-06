/****************************************************************************
 *   Copyright (c) 2018 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef IMPORT_IMPORTOCAF2_H
#define IMPORT_IMPORTOCAF2_H

#include <TDocStd_Document.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
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
#include <Base/Sequencer.h>
#include "ImportOCAF.h"
#include "ExportOCAF.h"


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

class ImportExport ImportOCAF2
{
public:
    ImportOCAF2(Handle(TDocStd_Document) h, App::Document* d, const std::string& name);
    virtual ~ImportOCAF2();
    App::DocumentObject* loadShapes();
    void setMerge(bool enable) { merge=enable;};
    void setUseLinkGroup(bool enable) { useLinkGroup=enable; }
    void setBaseName(bool enable) { useBaseName=enable; }
    void setImportHiddenObject(bool enable) {importHidden=enable;}
    void setReduceObjects(bool enable) {reduceObjects=enable;}
    void setShowProgress(bool enable) {showProgress=enable;}
    void setExpandCompound(bool enable) {expandCompound=enable;}

    enum ImportMode {
        SingleDoc = 0,
        GroupPerDoc = 1,
        GroupPerDir = 2,
        ObjectPerDoc = 3,
        ObjectPerDir = 4,
        ModeMax,
    };
    void setMode(int m);
    int getMode() const {return mode;}

private:
    struct Info {
        std::string baseName;
        App::DocumentObject *obj = 0;
        App::PropertyPlacement *propPlacement = 0;
        App::Color faceColor;
        App::Color edgeColor;
        bool hasFaceColor = false;
        bool hasEdgeColor = false;
        int free = true;
    };

    App::DocumentObject *loadShape(App::Document *doc, TDF_Label label, 
            const TopoDS_Shape &shape, bool baseOnly=false, bool newDoc=true);
    App::Document *getDocument(App::Document *doc, TDF_Label label);
    bool createAssembly(App::Document *doc, TDF_Label label, 
            const TopoDS_Shape &shape, Info &info, bool newDoc);
    bool createObject(App::Document *doc, TDF_Label label, 
            const TopoDS_Shape &shape, Info &info, bool newDoc);
    bool createGroup(App::Document *doc, Info &info, 
            const TopoDS_Shape &shape, std::vector<App::DocumentObject*> &children, 
            const boost::dynamic_bitset<> &visibilities, bool canReduce=false);
    bool getColor(const TopoDS_Shape &shape, Info &info, bool check=false, bool noDefault=false);
    void getSHUOColors(TDF_Label label, std::map<std::string,App::Color> &colors, bool appendFirst);
    void setObjectName(Info &info, TDF_Label label);
    std::string getLabelName(TDF_Label label);
    App::DocumentObject *expandShape(App::Document *doc, TDF_Label label, const TopoDS_Shape &shape);

    virtual void applyEdgeColors(Part::Feature*, const std::vector<App::Color>&) {}
    virtual void applyFaceColors(Part::Feature*, const std::vector<App::Color>&) {}
    virtual void applyElementColors(App::DocumentObject*, const std::map<std::string,App::Color>&) {}
    virtual void applyLinkColor(App::DocumentObject *, int /*index*/, App::Color){}

private:
    class ImportLegacy : public ImportOCAF {
    public:
        ImportLegacy(ImportOCAF2 &parent)
            :ImportOCAF(parent.pDoc, parent.pDocument, parent.default_name),myParent(parent)
        {}
            
    private:
        void applyColors(Part::Feature* part, const std::vector<App::Color>& colors) {
            myParent.applyFaceColors(part, colors);
        }

        ImportOCAF2 &myParent;
    };
    friend class ImportLegacy;

    Handle(TDocStd_Document) pDoc;
    App::Document* pDocument;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;
    bool merge;
    std::string default_name;
    bool useLinkGroup;
    bool useBaseName;
    bool importHidden;
    bool reduceObjects;
    bool showProgress;
    bool expandCompound;

    int mode;
    std::string filePath;

    std::unordered_map<TopoDS_Shape, Info, ShapeHasher> myShapes;
    std::unordered_map<TDF_Label, std::string, LabelHasher> myNames;
    std::unordered_map<App::DocumentObject*, App::PropertyPlacement*> myCollapsedObjects;

    App::Color defaultFaceColor;
    App::Color defaultEdgeColor;

    Base::SequencerLauncher *sequencer;
};

class ImportExport ExportOCAF2
{
public:
    typedef std::function<std::map<std::string,App::Color>(
            App::DocumentObject*, const char*)> GetShapeColorsFunc;
    ExportOCAF2(Handle(TDocStd_Document) h, GetShapeColorsFunc func=GetShapeColorsFunc());

    void setExportHiddenObject(bool enable) {exportHidden=enable;}
    void exportObjects(std::vector<App::DocumentObject*> &objs, const char *name=0);
    bool canFallback(std::vector<App::DocumentObject*> objs);

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

    std::unordered_map<App::DocumentObject *, TDF_Label> myObjects;

    std::unordered_map<TDF_Label, std::vector<std::string>, LabelHasher> myNames;

    std::set<std::pair<App::DocumentObject*,std::string> > mySetups;

    GetShapeColorsFunc getShapeColors;

    App::Color defaultColor;
    bool exportHidden;
};

}

#endif //IMPORT_IMPORTOCAF2_H
