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

#include <climits>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <TDF_LabelMapHasher.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

#include <Base/Sequencer.h>
#include <Mod/Part/App/TopoShape.h>

#include "ExportOCAF.h"
#include "ImportOCAF.h"
#include "Tools.h"


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

namespace Import
{

struct ImportExport ImportOCAFOptions
{
    ImportOCAFOptions();
    App::Color defaultFaceColor;
    App::Color defaultEdgeColor;
    bool merge = false;
    bool useLinkGroup = false;
    bool useBaseName = true;
    bool importHidden = true;
    bool reduceObjects = false;
    bool showProgress = false;
    bool expandCompound = false;
    int mode = 0;
};

class ImportExport ImportOCAF2
{
public:
    ImportOCAF2(Handle(TDocStd_Document) h, App::Document* d, const std::string& name);
    virtual ~ImportOCAF2();
    App::DocumentObject* loadShapes();

    static ImportOCAFOptions customImportOptions();
    void setImportOptions(ImportOCAFOptions opts);
    void setMerge(bool enable)
    {
        options.merge = enable;
    }
    void setUseLinkGroup(bool enable);
    void setBaseName(bool enable)
    {
        options.useBaseName = enable;
    }
    void setImportHiddenObject(bool enable)
    {
        options.importHidden = enable;
    }
    void setReduceObjects(bool enable)
    {
        options.reduceObjects = enable;
    }
    void setShowProgress(bool enable)
    {
        options.showProgress = enable;
    }
    void setExpandCompound(bool enable)
    {
        options.expandCompound = enable;
    }

    enum ImportMode
    {
        SingleDoc = 0,
        GroupPerDoc = 1,
        GroupPerDir = 2,
        ObjectPerDoc = 3,
        ObjectPerDir = 4,
        ModeMax,
    };
    void setMode(int m);
    int getMode() const
    {
        return options.mode;
    }

private:
    struct Info
    {
        std::string baseName;
        App::DocumentObject* obj = nullptr;
        App::PropertyPlacement* propPlacement = nullptr;
        App::Color faceColor;
        App::Color edgeColor;
        bool hasFaceColor = false;
        bool hasEdgeColor = false;
        int free = true;
    };

    App::DocumentObject* loadShape(App::Document* doc,
                                   TDF_Label label,
                                   const TopoDS_Shape& shape,
                                   bool baseOnly = false,
                                   bool newDoc = true);
    App::Document* getDocument(App::Document* doc, TDF_Label label);
    bool createAssembly(App::Document* doc,
                        TDF_Label label,
                        const TopoDS_Shape& shape,
                        Info& info,
                        bool newDoc);
    bool createObject(App::Document* doc,
                      TDF_Label label,
                      const TopoDS_Shape& shape,
                      Info& info,
                      bool newDoc);
    bool createGroup(App::Document* doc,
                     Info& info,
                     const TopoDS_Shape& shape,
                     std::vector<App::DocumentObject*>& children,
                     const boost::dynamic_bitset<>& visibilities,
                     bool canReduce = false);
    bool
    getColor(const TopoDS_Shape& shape, Info& info, bool check = false, bool noDefault = false);
    void
    getSHUOColors(TDF_Label label, std::map<std::string, App::Color>& colors, bool appendFirst);
    void setObjectName(Info& info, TDF_Label label);
    std::string getLabelName(TDF_Label label);
    App::DocumentObject*
    expandShape(App::Document* doc, TDF_Label label, const TopoDS_Shape& shape);

    virtual void applyEdgeColors(Part::Feature*, const std::vector<App::Color>&)
    {}
    virtual void applyFaceColors(Part::Feature*, const std::vector<App::Color>&)
    {}
    virtual void applyElementColors(App::DocumentObject*, const std::map<std::string, App::Color>&)
    {}
    virtual void applyLinkColor(App::DocumentObject*, int /*index*/, App::Color)
    {}

private:
    class ImportLegacy: public ImportOCAF
    {
    public:
        explicit ImportLegacy(ImportOCAF2& parent)
            : ImportOCAF(parent.pDoc, parent.pDocument, parent.default_name)
            , myParent(parent)
        {}

    private:
        void applyColors(Part::Feature* part, const std::vector<App::Color>& colors) override
        {
            myParent.applyFaceColors(part, colors);
        }

        ImportOCAF2& myParent;
    };
    friend class ImportLegacy;

    Handle(TDocStd_Document) pDoc;
    App::Document* pDocument;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;
    std::string default_name;

    ImportOCAFOptions options;
    std::string filePath;

    std::unordered_map<TopoDS_Shape, Info, ShapeHasher> myShapes;
    std::unordered_map<TDF_Label, std::string, LabelHasher> myNames;
    std::unordered_map<App::DocumentObject*, App::PropertyPlacement*> myCollapsedObjects;

    Base::SequencerLauncher* sequencer {nullptr};
};

class ImportExport ImportOCAFExt: public ImportOCAF2
{
public:
    ImportOCAFExt(Handle(TDocStd_Document) hStdDoc, App::Document* doc, const std::string& name);

    std::map<Part::Feature*, std::vector<App::Color>> partColors;

private:
    void applyFaceColors(Part::Feature* part, const std::vector<App::Color>& colors) override;
};

struct ImportExport ExportOCAFOptions
{
    ExportOCAFOptions();
    App::Color defaultColor;
    bool exportHidden = true;
    bool keepPlacement = false;
};

class ImportExport ExportOCAF2
{
public:
    using GetShapeColorsFunc =
        std::function<std::map<std::string, App::Color>(App::DocumentObject*, const char*)>;
    explicit ExportOCAF2(Handle(TDocStd_Document) h,
                         GetShapeColorsFunc func = GetShapeColorsFunc());

    static ExportOCAFOptions customExportOptions();
    void setExportOptions(ExportOCAFOptions opts)
    {
        options = opts;
    }
    void setExportHiddenObject(bool enable)
    {
        options.exportHidden = enable;
    }
    void setKeepPlacement(bool enable)
    {
        options.keepPlacement = enable;
    }
    void exportObjects(std::vector<App::DocumentObject*>& objs, const char* name = nullptr);
    bool canFallback(std::vector<App::DocumentObject*> objs);

private:
    TDF_Label exportObject(App::DocumentObject* obj,
                           const char* sub,
                           TDF_Label parent,
                           const char* name = nullptr);
    void setupObject(TDF_Label label,
                     App::DocumentObject* obj,
                     const Part::TopoShape& shape,
                     const std::string& prefix,
                     const char* name = nullptr,
                     bool force = false);
    void setName(TDF_Label label, App::DocumentObject* obj, const char* name = nullptr);
    TDF_Label findComponent(const char* subname, TDF_Label label, TDF_LabelSequence& labels);

private:
    Handle(TDocStd_Document) pDoc;
    Handle(XCAFDoc_ShapeTool) aShapeTool;
    Handle(XCAFDoc_ColorTool) aColorTool;

    std::unordered_map<App::DocumentObject*, TDF_Label> myObjects;

    std::unordered_map<TDF_Label, std::vector<std::string>, LabelHasher> myNames;

    std::set<std::pair<App::DocumentObject*, std::string>> mySetups;

    std::vector<App::DocumentObject*> groupLinks;

    GetShapeColorsFunc getShapeColors;

    ExportOCAFOptions options;
};

}  // namespace Import

#endif  // IMPORT_IMPORTOCAF2_H
