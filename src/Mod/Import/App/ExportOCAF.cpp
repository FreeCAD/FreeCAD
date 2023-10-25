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

#include "PreCompiled.h"
#if defined(__MINGW32__)
#define WNT  // avoid conflict with GUID
#endif
#ifndef _PreComp_
#include <Quantity_ColorRGBA.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_Location.hxx>
#include <climits>
#include <gp_Ax1.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>  // for Precision::Confusion()
#include <gp_Trsf.hxx>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>
#include <Mod/Part/App/Interface.h>
#include <Mod/Part/App/PartFeature.h>

#include "ExportOCAF.h"


#if OCC_VERSION_HEX >= 0x070500
// See https://dev.opencascade.org/content/occt-3d-viewer-becomes-srgb-aware
#define OCC_COLOR_SPACE Quantity_TOC_sRGB
#else
#define OCC_COLOR_SPACE Quantity_TOC_RGB
#endif

static inline Quantity_ColorRGBA convertColor(const App::Color& c)
{
    return Quantity_ColorRGBA(Quantity_Color(c.r, c.g, c.b, OCC_COLOR_SPACE), 1.0 - c.a);
}

using namespace Import;


ExportOCAF::ExportOCAF(Handle(TDocStd_Document) hDoc, bool explicitPlacement)
    : pDoc(hDoc)
    , keepExplicitPlacement(explicitPlacement)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool(pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());

    if (keepExplicitPlacement) {
        // rootLabel = aShapeTool->NewShape();
        // TDataStd_Name::Set(rootLabel, "ASSEMBLY");
        Part::Interface::writeStepAssembly(Part::Interface::Assembly::Auto);
    }
    else {
        rootLabel = TDF_TagSource::NewChild(pDoc->Main());
    }
}

ExportOCAF::~ExportOCAF() = default;

std::vector<App::DocumentObject*> ExportOCAF::filterPart(App::Part* part) const
{
    // Ignore shape of a Part that is referenced by a FeatureBase (#0003807)
    //
    std::vector<App::DocumentObject*> entries = part->Group.getValues();

    // get FeatureBases of the out-lists of the features of the Part
    Base::Type featureBase = Base::Type::fromName("PartDesign::FeatureBase");
    std::vector<App::DocumentObject*> filterType;
    for (auto it : entries) {
        std::vector<App::DocumentObject*> outList = it->getOutList();
        for (auto jt : outList) {
            if (jt->getTypeId() == featureBase) {
                filterType.push_back(jt);
            }
        }
    }

    // now check if for a feature of the Part it must be filtered
    if (!filterType.empty()) {
        std::vector<App::DocumentObject*> keepObjects;
        for (auto it : entries) {
            std::vector<App::DocumentObject*> inList = it->getInList();
            bool accept = true;
            for (auto jt : inList) {
                auto kt = std::find(filterType.begin(), filterType.end(), jt);
                if (kt != filterType.end()) {
                    accept = false;
                    break;
                }
            }

            if (accept) {
                keepObjects.push_back(it);
            }
        }

        entries.swap(keepObjects);
    }

    return entries;
}

void ExportOCAF::exportObjects(std::vector<App::DocumentObject*>& objs)
{
    // That stuff is exporting a list of selected objects into FreeCAD Tree
    std::vector<TDF_Label> hierarchical_label;
    std::vector<TopLoc_Location> hierarchical_loc;
    std::vector<App::DocumentObject*> hierarchical_part;
    for (auto obj : objs) {
        exportObject(obj, hierarchical_label, hierarchical_loc, hierarchical_part);
    }

    // Free Shapes must have absolute placement and not explicit
    std::vector<TDF_Label> FreeLabels;
    std::vector<int> part_id;
    getFreeLabels(hierarchical_label, FreeLabels, part_id);

    std::vector<std::vector<App::Color>> Colors;
    getPartColors(hierarchical_part, FreeLabels, part_id, Colors);
    reallocateFreeShape(hierarchical_part, FreeLabels, part_id, Colors);

    // Update is not performed automatically anymore:
    // https://tracker.dev.opencascade.org/view.php?id=28055
    XCAFDoc_DocumentTool::ShapeTool(pDoc->Main())->UpdateAssemblies();
}

int ExportOCAF::exportObject(App::DocumentObject* obj,
                             std::vector<TDF_Label>& hierarchical_label,
                             std::vector<TopLoc_Location>& hierarchical_loc,
                             std::vector<App::DocumentObject*>& hierarchical_part)
{
    std::vector<int> local_label;
    int root_id;
    int return_label = -1;

    if (obj->getTypeId().isDerivedFrom(App::Part::getClassTypeId())) {
        App::Part* part = static_cast<App::Part*>(obj);
        // I shall recursively select the elements and call back
        std::vector<App::DocumentObject*> entries = part->Group.getValues();
        std::vector<App::DocumentObject*>::iterator it;

        if (filterBaseFeature) {
            entries = filterPart(part);
        }

        for (it = entries.begin(); it != entries.end(); ++it) {
            int new_label = 0;
            new_label =
                exportObject((*it), hierarchical_label, hierarchical_loc, hierarchical_part);
            local_label.push_back(new_label);
        }

        createNode(part, root_id, hierarchical_label, hierarchical_loc, hierarchical_part);
        std::vector<int>::iterator label_it;
        for (label_it = local_label.begin(); label_it != local_label.end(); ++label_it) {
            pushNode(root_id, (*label_it), hierarchical_label, hierarchical_loc);
        }

        return_label = root_id;
    }

    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        Part::Feature* part = static_cast<Part::Feature*>(obj);
        std::vector<App::Color> colors;
        findColors(part, colors);

        return_label =
            saveShape(part, colors, hierarchical_label, hierarchical_loc, hierarchical_part);
    }

    return return_label;
}

// This function creates an Assembly node in an XCAF document with its relative placement
// information
void ExportOCAF::createNode(App::Part* part,
                            int& root_id,
                            std::vector<TDF_Label>& hierarchical_label,
                            std::vector<TopLoc_Location>& hierarchical_loc,
                            std::vector<App::DocumentObject*>& hierarchical_part)
{
    TDF_Label shapeLabel = aShapeTool->NewShape();
    Handle(TDataStd_Name) N;
    TDataStd_Name::Set(shapeLabel, TCollection_ExtendedString(part->Label.getValue(), true));

    Base::Placement pl = part->Placement.getValue();
    Base::Rotation rot(pl.getRotation());
    Base::Vector3d axis;

    double angle;
    rot.getValue(axis, angle);

    gp_Trsf trf;
    trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(axis.x, axis.y, axis.z)), angle);
    trf.SetTranslationPart(gp_Vec(pl.getPosition().x, pl.getPosition().y, pl.getPosition().z));
    TopLoc_Location MyLoc = TopLoc_Location(trf);
    XCAFDoc_Location::Set(shapeLabel, TopLoc_Location(trf));

    hierarchical_label.push_back(shapeLabel);
    hierarchical_loc.push_back(MyLoc);
    hierarchical_part.push_back(part);
    root_id = hierarchical_label.size();
}

int ExportOCAF::saveShape(Part::Feature* part,
                          const std::vector<App::Color>& colors,
                          std::vector<TDF_Label>& hierarchical_label,
                          std::vector<TopLoc_Location>& hierarchical_loc,
                          std::vector<App::DocumentObject*>& hierarchical_part)
{
    const TopoDS_Shape& shape = part->Shape.getValue();
    if (shape.IsNull()) {
        return -1;
    }

    TopoDS_Shape baseShape;
    TopLoc_Location aLoc;
    Handle(TDataStd_Name) N;

    Base::Placement pl = part->Placement.getValue();
    Base::Rotation rot(pl.getRotation());
    Base::Vector3d axis;
    double angle;
    rot.getValue(axis, angle);
    gp_Trsf trf;
    trf.SetRotation(gp_Ax1(gp_Pnt(0., 0., 0.), gp_Dir(axis.x, axis.y, axis.z)), angle);
    trf.SetTranslationPart(gp_Vec(pl.getPosition().x, pl.getPosition().y, pl.getPosition().z));
    TopLoc_Location MyLoc = TopLoc_Location(trf);

    if (keepExplicitPlacement) {
        // http://www.opencascade.org/org/forum/thread_18813/?forum=3
        aLoc = shape.Location();
        baseShape = shape.Located(TopLoc_Location());
    }
    else {
        baseShape = shape;
    }

    // Add shape and name
    TDF_Label shapeLabel = aShapeTool->NewShape();
    aShapeTool->SetShape(shapeLabel, baseShape);

    TDataStd_Name::Set(shapeLabel, TCollection_ExtendedString(part->Label.getValue(), true));


    /*
        if (keepExplicitPlacement) {
            aShapeTool->AddComponent(aShapeTool->BaseLabel(), shapeLabel, aLoc);
            XCAFDoc_Location::Set(shapeLabel,MyLoc);
        }
    */

    // Add color information
    Quantity_ColorRGBA col;

    std::set<int> face_index;
    TopTools_IndexedMapOfShape faces;
    TopExp_Explorer xp(baseShape, TopAbs_FACE);
    while (xp.More()) {
        face_index.insert(faces.Add(xp.Current()));
        xp.Next();
    }

    // define color per face?
    if (colors.size() == face_index.size()) {
        xp.Init(baseShape, TopAbs_FACE);
        while (xp.More()) {
            int index = faces.FindIndex(xp.Current());
            if (face_index.find(index) != face_index.end()) {
                face_index.erase(index);

                // If the baseShape is a face then since OCCT 7.3 AddSubShape() returns
                // a null label.
                // If faceLabel is null we check if for the current face a label already
                // exists. If yes then faceLabel is equal to shapeLabel.
                TDF_Label faceLabel = aShapeTool->AddSubShape(shapeLabel, xp.Current());
                // TDF_Label faceLabel= TDF_TagSource::NewChild(shapeLabel);
                if (!faceLabel.IsNull()) {
                    aShapeTool->SetShape(faceLabel, xp.Current());
                }
                else {
                    aShapeTool->FindShape(xp.Current(), faceLabel);
                }

                if (!faceLabel.IsNull()) {
                    const App::Color& color = colors[index - 1];
                    col = convertColor(color);
                    aColorTool->SetColor(faceLabel, col, XCAFDoc_ColorSurf);
                }
            }
            xp.Next();
        }
    }
    else if (!colors.empty()) {
        App::Color color = colors.front();
        col = convertColor(color);
        aColorTool->SetColor(shapeLabel, col, XCAFDoc_ColorGen);
    }

    hierarchical_label.push_back(shapeLabel);
    hierarchical_loc.push_back(MyLoc);
    hierarchical_part.push_back(part);

    return (hierarchical_label.size());
}

// This function is scanning the OCAF doc for Free Shapes and returns the label attached to it
// If this Free Shapes are regular Part::Feature, we must use absolute coordinate instead of
// allocating a placement into the hierarchy as it is not attached to a hierarchical node

void ExportOCAF::getFreeLabels(std::vector<TDF_Label>& hierarchical_label,
                               std::vector<TDF_Label>& labels,
                               std::vector<int>& label_part_id)
{
    TDF_LabelSequence FreeLabels;
    aShapeTool->GetFreeShapes(FreeLabels);
    int n = FreeLabels.Length();
    for (int i = 1; i <= n; i++) {
        TDF_Label label = FreeLabels.Value(i);
        for (std::size_t j = 0; j < hierarchical_label.size(); j++) {
            if (label == hierarchical_label.at(j)) {
                labels.push_back(label);
                label_part_id.push_back(j);
            }
        }
    }
}

void ExportOCAF::getPartColors(std::vector<App::DocumentObject*> hierarchical_part,
                               std::vector<TDF_Label> FreeLabels,
                               std::vector<int> part_id,
                               std::vector<std::vector<App::Color>>& Colors) const
{
    // I am seeking for the colors of each parts
    std::size_t n = FreeLabels.size();
    for (std::size_t i = 0; i < n; i++) {
        std::vector<App::Color> colors;
        Part::Feature* part = static_cast<Part::Feature*>(hierarchical_part.at(part_id.at(i)));
        findColors(part, colors);
        Colors.push_back(colors);
    }
}

void ExportOCAF::reallocateFreeShape(std::vector<App::DocumentObject*> hierarchical_part,
                                     std::vector<TDF_Label> FreeLabels,
                                     std::vector<int> part_id,
                                     std::vector<std::vector<App::Color>>& Colors)
{
    std::size_t n = FreeLabels.size();
    for (std::size_t i = 0; i < n; i++) {
        TDF_Label label = FreeLabels.at(i);
        // hierarchical part does contain only part currently and not node I should add node
        if (hierarchical_part.at(part_id.at(i))
                ->getTypeId()
                .isDerivedFrom(Part::Feature::getClassTypeId())) {
            Part::Feature* part = static_cast<Part::Feature*>(hierarchical_part.at(part_id.at(i)));
            aShapeTool->SetShape(label, part->Shape.getValue());
            // Add color information
            std::vector<App::Color> colors;
            colors = Colors.at(i);
            TopoDS_Shape baseShape = part->Shape.getValue();

            // Add color information
            Quantity_ColorRGBA col;

            std::set<int> face_index;
            TopTools_IndexedMapOfShape faces;
            TopExp_Explorer xp(baseShape, TopAbs_FACE);
            while (xp.More()) {
                face_index.insert(faces.Add(xp.Current()));
                xp.Next();
            }

            // define color per face?
            if (colors.size() == face_index.size()) {
                xp.Init(baseShape, TopAbs_FACE);
                while (xp.More()) {
                    int index = faces.FindIndex(xp.Current());
                    if (face_index.find(index) != face_index.end()) {
                        face_index.erase(index);

                        // If the baseShape is a face then since OCCT 7.3 AddSubShape() returns
                        // a null label.
                        // If faceLabel is null we check if for the current face a label already
                        // exists. If yes then faceLabel is equal to label.
                        TDF_Label faceLabel = aShapeTool->AddSubShape(label, xp.Current());
                        // TDF_Label faceLabel= TDF_TagSource::NewChild(label);
                        if (!faceLabel.IsNull()) {
                            aShapeTool->SetShape(faceLabel, xp.Current());
                        }
                        else {
                            aShapeTool->FindShape(xp.Current(), faceLabel);
                        }

                        if (!faceLabel.IsNull()) {
                            const App::Color& color = colors[index - 1];
                            col = convertColor(color);
                            aColorTool->SetColor(faceLabel, col, XCAFDoc_ColorSurf);
                        }
                    }

                    xp.Next();
                }
            }
            else if (!colors.empty()) {
                App::Color color = colors.front();
                col = convertColor(color);
                aColorTool->SetColor(label, col, XCAFDoc_ColorGen);
            }
        }
    }
}

// This function is moving a "standard" node into an Assembly node within an XCAF doc
void ExportOCAF::pushNode(int root_id,
                          int node_id,
                          std::vector<TDF_Label>& hierarchical_label,
                          std::vector<TopLoc_Location>& hierarchical_loc)
{
    TDF_Label root;
    TDF_Label node;
    root = hierarchical_label.at(root_id - 1);
    node = hierarchical_label.at(node_id - 1);

    XCAFDoc_DocumentTool::ShapeTool(root)->AddComponent(root,
                                                        node,
                                                        hierarchical_loc.at(node_id - 1));
}

// ----------------------------------------------------------------------------

ExportOCAFCmd::ExportOCAFCmd(Handle(TDocStd_Document) h, bool explicitPlacement)
    : ExportOCAF(h, explicitPlacement)
{}

void ExportOCAFCmd::findColors(Part::Feature* part, std::vector<App::Color>& colors) const
{
    std::map<Part::Feature*, std::vector<App::Color>>::const_iterator it = partColors.find(part);
    if (it != partColors.end()) {
        colors = it->second;
    }
}
