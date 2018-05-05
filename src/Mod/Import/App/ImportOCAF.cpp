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
# define WNT // avoid conflict with GUID
#endif
#ifndef _PreComp_
# include <gp_Trsf.hxx>
# include <gp_Ax1.hxx>
# include <NCollection_Vector.hxx>
# include <BRepBuilderAPI_MakeShape.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <Standard_Failure.hxx>
# include <TopoDS_Face.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx> // for Precision::Confusion()
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <climits>
# include <Standard_Version.hxx>
# include <BRep_Builder.hxx>
# include <TDocStd_Document.hxx>
# include <XCAFApp_Application.hxx>
# include <TDocStd_Document.hxx>
# include <XCAFApp_Application.hxx>
# include <XCAFDoc_DocumentTool.hxx>
# include <XCAFDoc_ShapeTool.hxx>
# include <XCAFDoc_ColorTool.hxx>
# include <XCAFDoc_Location.hxx>
# include <TDF_Label.hxx>
# include <TDF_LabelSequence.hxx>
# include <TDF_ChildIterator.hxx>
# include <TDataStd_Name.hxx>
# include <Quantity_Color.hxx>
# include <STEPCAFControl_Reader.hxx>
# include <STEPCAFControl_Writer.hxx>
# include <STEPControl_Writer.hxx>
# include <IGESCAFControl_Reader.hxx>
# include <IGESCAFControl_Writer.hxx>
# include <IGESControl_Controller.hxx>
# include <Interface_Static.hxx>
# include <Transfer_TransientProcess.hxx>
# include <XSControl_WorkSession.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_MapOfShape.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS_Iterator.hxx>
# include <APIHeaderSection_MakeHeader.hxx>
# include <OSD_Exception.hxx>
#if OCC_VERSION_HEX >= 0x060500
# include <TDataXtd_Shape.hxx>
# else
# include <TDataStd_Shape.hxx>
# endif
#endif

#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/Part.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/FeatureCompound.h>
#include "ImportOCAF.h"
#include <Mod/Part/App/ProgressIndicator.h>
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>

#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>

#ifdef HAVE_TBB
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/task_group.h>
#endif



using namespace Import;


#define OCAF_KEEP_PLACEMENT

ImportOCAF::ImportOCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
    : pDoc(h), doc(d), merge(true), default_name(name)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool (pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());
}

ImportOCAF::~ImportOCAF()
{
}

void ImportOCAF::loadShapes()
{
    std::vector<App::DocumentObject*> lValue;
    myRefShapes.clear();
    loadShapes(pDoc->Main(), TopLoc_Location(), default_name, "", false, lValue);
    lValue.clear();
}

void ImportOCAF::setMerge(bool merge)
{
    this->merge=merge;
}

void ImportOCAF::loadShapes(const TDF_Label& label, const TopLoc_Location& loc,
                            const std::string& defaultname, const std::string& assembly, bool isRef,
                            std::vector<App::DocumentObject*>& lValue)
{
    int hash = 0;
#ifdef HAVE_TBB
    using namespace tbb;
    task_group g;
#endif
    TopoDS_Shape aShape;

    std::vector<App::DocumentObject *> localValue;

    if (aShapeTool->GetShape(label,aShape)) {
        hash = aShape.HashCode(HashUpper);
    }

    Handle(TDataStd_Name) name;
    std::string part_name = defaultname;
    if (label.FindAttribute(TDataStd_Name::GetID(),name)) {
        TCollection_ExtendedString extstr = name->Get();
        char* str = new char[extstr.LengthOfCString()+1];
        extstr.ToUTF8CString(str);
        part_name = str;
        delete [] str;
        if (part_name.empty()) {
            part_name = defaultname;
        }
        else {
            bool ws=true;
            for (std::string::iterator it = part_name.begin(); it != part_name.end(); ++it) {
                if (*it != ' ') {
                    ws = false;
                    break;
                }
            }
            if (ws)
                part_name = defaultname;
        }
    }

    TopLoc_Location part_loc = loc;
    Handle(XCAFDoc_Location) hLoc;
    if (label.FindAttribute(XCAFDoc_Location::GetID(), hLoc)) {
        if (isRef)
            part_loc = part_loc * hLoc->Get();
        else
            part_loc = hLoc->Get();
    }

#ifdef FC_DEBUG
    Base::Console().Message("H:%d, N:%s, T:%d, A:%d, S:%d, C:%d, SS:%d, F:%d, R:%d, C:%d, SS:%d\n",
        hash,
        part_name.c_str(),
        aShapeTool->IsTopLevel(label),
        aShapeTool->IsAssembly(label),
        aShapeTool->IsShape(label),
        aShapeTool->IsCompound(label),
        aShapeTool->IsSimpleShape(label),
        aShapeTool->IsFree(label),
        aShapeTool->IsReference(label),
        aShapeTool->IsComponent(label),
        aShapeTool->IsSubShape(label)
    );
#endif

#if defined(OCAF_KEEP_PLACEMENT)
    std::string asm_name = part_name;
    (void)assembly;
#else
    std::string asm_name = assembly;
    if (aShapeTool->IsAssembly(label)) {
        asm_name = part_name;
    }
#endif

    TDF_Label ref;
    if (aShapeTool->IsReference(label) && aShapeTool->GetReferredShape(label, ref)) {
        loadShapes(ref, part_loc, part_name, asm_name, true, lValue);
    }

    if (isRef || myRefShapes.find(hash) == myRefShapes.end()) {
        TopoDS_Shape aShape;
        if (isRef && aShapeTool->GetShape(label, aShape))
            myRefShapes.insert(aShape.HashCode(HashUpper));

        if (aShapeTool->IsSimpleShape(label) && (isRef || aShapeTool->IsFree(label))) {
            if (!asm_name.empty())
                part_name = asm_name;

            // TODO: The merge parameter (last one from createShape) should become an Importer/Exporter
            // option within the FreeCAD preference menu
            // Currently it is merging STEP Compound Shape into a single Shape Part::Feature which
            // is an OpenCascade computed Compound
            if (isRef)
                createShape(label, loc, part_name, lValue, this->merge);
            else
                createShape(label, part_loc, part_name, localValue, this->merge);
        }
        else {
            if (aShapeTool->IsSimpleShape(label)) {
                // We are not creating a list of Part::Feature in that case but just
                // a single Part::Feature which has as a Shape a Compound of the Subshapes contained
                // within the global shape
                // This is standard behavior of many STEP reader and avoid to register a crazy amount of
                // Shape within the Tree as STEP file do mostly contain large assemblies
                return;
            }

            // This is probably an Assembly let's try to create a Compound with the name
            for (TDF_ChildIterator it(label); it.More(); it.Next()) {
                if (isRef)
                    loadShapes(it.Value(), part_loc, part_name, asm_name, false, localValue);
                else
                    loadShapes(it.Value(), part_loc, part_name, asm_name, isRef, localValue);
            }

            if (!localValue.empty()) {
                if (aShapeTool->IsAssembly(label)) {
                    App::Part *pcPart = NULL;
                    pcPart = static_cast<App::Part*>(doc->addObject("App::Part",asm_name.c_str()));
                    pcPart->addObjects(localValue);

                    // STEP reader is now a hierarchical reader. Node and leaf must have
                    // there local placement updated and relative to the STEP file content
                    // standard FreeCAD placement was absolute we are now moving to relative

                    gp_Trsf trf;
                    Base::Matrix4D mtrx;
                    if (part_loc.IsIdentity())
                        trf = part_loc.Transformation();
                    else
                        trf = TopLoc_Location(part_loc.FirstDatum()).Transformation();
                    Part::TopoShape::convertToMatrix(trf, mtrx);
                    Base::Placement pl;
                    pl.fromMatrix(mtrx);
                    pcPart->Placement.setValue(pl);

                    lValue.push_back(pcPart);
                }
            }
        }
    }
}

void ImportOCAF::createShape(const TDF_Label& label, const TopLoc_Location& loc, const std::string& name,
                             std::vector<App::DocumentObject*>& lValue, bool merge)
{
    const TopoDS_Shape& aShape = aShapeTool->GetShape(label);
#ifdef HAVE_TBB
    using namespace tbb;
    task_group g;
#endif

    App::Color color(0.8f,0.8f,0.8f);
    std::vector<App::Color> colors;
    if (!aShape.IsNull() && aShape.ShapeType() == TopAbs_COMPOUND) {
        TopExp_Explorer xp;
        int ctSolids = 0, ctShells = 0, ctVertices = 0, ctEdges = 0;
        std::vector<App::DocumentObject *> localValue;
        App::Part *pcPart = NULL;

        if (merge) {

            // We should do that only if there is more than a single shape inside
            // Computing Compounds takes time
            // We must keep track of the Color. If there is more than 1 Color into
            // a STEP Compound then the Merge can't be done and we cancel the operation

            BRep_Builder builder;
            TopoDS_Compound comp;
            builder.MakeCompound(comp);

/*
            std::vector<App::Color> colors;
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
                Quantity_Color aColor;
                App::Color color(0.8f,0.8f,0.8f);
                if (aColorTool->GetColor(xp.Current(), XCAFDoc_ColorGen, aColor) ||
                    aColorTool->GetColor(xp.Current(), XCAFDoc_ColorSurf, aColor) ||
                    aColorTool->GetColor(xp.Current(), XCAFDoc_ColorCurv, aColor)) {
                    color.r = (float)aColor.Red();
                    color.g = (float)aColor.Green();
                    color.b = (float)aColor.Blue();
                    colors.push_back(color);
                }
            }

            if (colors.size() > 1) {
                createShape(label, loc, name, lValue, false);
                return;
            }
*/
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_EDGE); xp.More(); xp.Next(), ctEdges++) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            for (xp.Init(aShape, TopAbs_VERTEX); xp.More(); xp.Next(), ctVertices++) {
                const TopoDS_Shape& sh = xp.Current();
                if (!sh.IsNull()) {
                    builder.Add(comp, sh);
                }
            }

            // Ok we got a Compound which is computed
            // Just need to add it to a Part::Feature and push it to lValue
            if (!comp.IsNull() && (ctSolids||ctShells||ctEdges||ctVertices)) {
                Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Feature"));
                // Let's allocate the relative placement of the Compound from the STEP file
                gp_Trsf trf;
                Base::Matrix4D mtrx;
                if ( loc.IsIdentity() )
                     trf = loc.Transformation();
                else
                     trf = TopLoc_Location(loc.FirstDatum()).Transformation();
                Part::TopoShape::convertToMatrix(trf, mtrx);
                Base::Placement pl;
                pl.fromMatrix(mtrx);
                part->Placement.setValue(pl);
                if (!loc.IsIdentity())
                    part->Shape.setValue(comp.Moved(loc));
                else
                    part->Shape.setValue(comp);
                part->Label.setValue(name);
                lValue.push_back(part);
            }
        }
        else {
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++) {
                createShape(xp.Current(), loc, name, localValue);
            }
            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++) {
                createShape(xp.Current(), loc, name, localValue);
            }
        }

        if (!localValue.empty() && !merge) {
            pcPart = static_cast<App::Part*>(doc->addObject("App::Part",name.c_str()));

            // localValue contain the objects that  must added to the local Part
            // We must add the PartOrigin and the Part itself
            pcPart->addObjects(localValue);

            // Let's compute relative placement of the Part
/*
            gp_Trsf trf;
            Base::Matrix4D mtrx;
            if ( loc.IsIdentity() )
                 trf = loc.Transformation();
            else
                 trf = TopLoc_Location(loc.FirstDatum()).Transformation();
            Part::TopoShape::convertToMatrix(trf, mtrx);
            Base::Placement pl;
            pl.fromMatrix(mtrx);
            pcPart->Placement.setValue(pl);
*/
            lValue.push_back(pcPart);
        }

        if (ctSolids > 0 || ctShells > 0)
            return;
    }
    else if (!aShape.IsNull()) {
        createShape(aShape, loc, name, lValue);
    }
}

void ImportOCAF::createShape(const TopoDS_Shape& aShape, const TopLoc_Location& loc, const std::string& name,
                             std::vector<App::DocumentObject*>& lvalue)
{
    Part::Feature* part = static_cast<Part::Feature*>(doc->addObject("Part::Feature"));

    if (!loc.IsIdentity())
        // part->Shape.setValue(aShape.Moved(TopLoc_Location(loc.FirstDatum())));
        part->Shape.setValue(aShape.Moved(loc));
    else
        part->Shape.setValue(aShape);

    part->Label.setValue(name);
    lvalue.push_back(part);

    Quantity_Color aColor;
    App::Color color(0.8f,0.8f,0.8f);
    if (aColorTool->GetColor(aShape, XCAFDoc_ColorGen, aColor) ||
        aColorTool->GetColor(aShape, XCAFDoc_ColorSurf, aColor) ||
        aColorTool->GetColor(aShape, XCAFDoc_ColorCurv, aColor)) {
        color.r = (float)aColor.Red();
        color.g = (float)aColor.Green();
        color.b = (float)aColor.Blue();
        std::vector<App::Color> colors;
        colors.push_back(color);
        applyColors(part, colors);
    }

    TopTools_IndexedMapOfShape faces;
    TopExp_Explorer xp(aShape,TopAbs_FACE);
    while (xp.More()) {
        faces.Add(xp.Current());
        xp.Next();
    }

    bool found_face_color = false;
    std::vector<App::Color> faceColors;
    faceColors.resize(faces.Extent(), color);
    xp.Init(aShape,TopAbs_FACE);
    while (xp.More()) {
        if (aColorTool->GetColor(xp.Current(), XCAFDoc_ColorGen, aColor) ||
            aColorTool->GetColor(xp.Current(), XCAFDoc_ColorSurf, aColor) ||
            aColorTool->GetColor(xp.Current(), XCAFDoc_ColorCurv, aColor)) {
            int index = faces.FindIndex(xp.Current());
            color.r = (float)aColor.Red();
            color.g = (float)aColor.Green();
            color.b = (float)aColor.Blue();
            faceColors[index-1] = color;
            found_face_color = true;
        }
        xp.Next();
    }

    if (found_face_color) {
        applyColors(part, faceColors);
    }
}

// ----------------------------------------------------------------------------

ExportOCAF::ExportOCAF(Handle(TDocStd_Document) h, bool explicitPlacement)
    : pDoc(h)
    , keepExplicitPlacement(explicitPlacement)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool(pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());

    if (keepExplicitPlacement) {
        // rootLabel = aShapeTool->NewShape();
        // TDataStd_Name::Set(rootLabel, "ASSEMBLY");
        Interface_Static::SetIVal("write.step.assembly",2);
    }
    else {
        rootLabel = TDF_TagSource::NewChild(pDoc->Main());
    }
}


// This function creates an Assembly node in an XCAF document with its relative placement information

void ExportOCAF::createNode(App::Part* part, int& root_id,
                            std::vector <TDF_Label>& hierarchical_label,
                            std::vector <TopLoc_Location>& hierarchical_loc,
                            std::vector <App::DocumentObject*>& hierarchical_part)
{
    TDF_Label shapeLabel = aShapeTool->NewShape();
    Handle(TDataStd_Name) N;
    TDataStd_Name::Set(shapeLabel, TCollection_ExtendedString(part->Label.getValue(), 1));

    Base::Placement pl = part->Placement.getValue();
    Base::Rotation rot(pl.getRotation());
    Base::Vector3d axis;

    double angle;
    rot.getValue(axis, angle);

    gp_Trsf trf;
    trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(axis.x, axis.y, axis.z)), angle);
    trf.SetTranslationPart(gp_Vec(pl.getPosition().x,pl.getPosition().y,pl.getPosition().z));
    TopLoc_Location MyLoc = TopLoc_Location(trf);
    XCAFDoc_Location::Set(shapeLabel,TopLoc_Location(trf));

    hierarchical_label.push_back(shapeLabel);
    hierarchical_loc.push_back(MyLoc);
    hierarchical_part.push_back(part);
    root_id=hierarchical_label.size();
}

int ExportOCAF::saveShape(Part::Feature* part, const std::vector<App::Color>& colors,
                          std::vector <TDF_Label>& hierarchical_label,
                          std::vector <TopLoc_Location>& hierarchical_loc,
                          std::vector <App::DocumentObject*>& hierarchical_part)
{
    const TopoDS_Shape& shape = part->Shape.getValue();
    if (shape.IsNull())
        return -1;

    TopoDS_Shape baseShape;
    TopLoc_Location aLoc;
    Handle(TDataStd_Name) N;

    Base::Placement pl = part->Placement.getValue();
    Base::Rotation rot(pl.getRotation());
    Base::Vector3d axis;
    double angle;
    rot.getValue(axis, angle);
    gp_Trsf trf;
    trf.SetRotation(gp_Ax1(gp_Pnt(0.,0.,0.), gp_Dir(axis.x, axis.y, axis.z)), angle);
    trf.SetTranslationPart(gp_Vec(pl.getPosition().x,pl.getPosition().y,pl.getPosition().z));
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

    TDataStd_Name::Set(shapeLabel, TCollection_ExtendedString(part->Label.getValue(), 1));


/*
    if (keepExplicitPlacement) {
        aShapeTool->AddComponent(aShapeTool->BaseLabel(), shapeLabel, aLoc);
        XCAFDoc_Location::Set(shapeLabel,MyLoc);
    }
*/

    // Add color information
    Quantity_Color col;

    std::set<int> face_index;
    TopTools_IndexedMapOfShape faces;
    TopExp_Explorer xp(baseShape,TopAbs_FACE);
    while (xp.More()) {
        face_index.insert(faces.Add(xp.Current()));
        xp.Next();
    }

    // define color per face?
    if (colors.size() == face_index.size()) {
        xp.Init(baseShape,TopAbs_FACE);
        while (xp.More()) {
            int index = faces.FindIndex(xp.Current());
            if (face_index.find(index) != face_index.end()) {
                face_index.erase(index);

                TDF_Label faceLabel = aShapeTool->AddSubShape(shapeLabel, xp.Current());
                // TDF_Label faceLabel= TDF_TagSource::NewChild(shapeLabel);
                aShapeTool->SetShape(faceLabel, xp.Current());

                const App::Color& color = colors[index-1];
                Standard_Real mat[3];
                mat[0] = color.r;
                mat[1] = color.g;
                mat[2] = color.b;
                col.SetValues(mat[0],mat[1],mat[2],Quantity_TOC_RGB);
                aColorTool->SetColor(faceLabel, col, XCAFDoc_ColorSurf);
            }
            xp.Next();
        }
    }
    else if (!colors.empty()) {
        App::Color color = colors.front();
        Standard_Real mat[3];
        mat[0] = color.r;
        mat[1] = color.g;
        mat[2] = color.b;
        col.SetValues(mat[0],mat[1],mat[2],Quantity_TOC_RGB);
        aColorTool->SetColor(shapeLabel, col, XCAFDoc_ColorGen);
    }

    hierarchical_label.push_back(shapeLabel);
    hierarchical_loc.push_back(MyLoc);
    hierarchical_part.push_back(part);

    return(hierarchical_label.size());
}

// This function is scanning the OCAF doc for Free Shapes and returns the label attached to it
// If this Free Shapes are regular Part::Feature, we must use absolute coordinate instead of
// allocating a placement into the hierarchy as it is not attached to a hierarchical node

void ExportOCAF::getFreeLabels(std::vector <TDF_Label>& hierarchical_label,
                               std::vector <TDF_Label>& labels,
                               std::vector <int>& label_part_id)
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

void ExportOCAF::reallocateFreeShape(std::vector <App::DocumentObject*> hierarchical_part,
                                     std::vector <TDF_Label> FreeLabels,
                                     std::vector <int> part_id,
                                     std::vector< std::vector<App::Color> >& Colors)
{
    std::size_t n = FreeLabels.size();
    for (std::size_t i = 0; i < n; i++) {
        TDF_Label label = FreeLabels.at(i);
        // hierarchical part does contain only part currently and not node I should add node
        if (hierarchical_part.at(part_id.at(i))->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            Part::Feature * part = static_cast<Part::Feature *>(hierarchical_part.at(part_id.at(i)));
            aShapeTool->SetShape(label, part->Shape.getValue());
            // Add color information
            std::vector<App::Color> colors;
            colors=Colors.at(i);
            TopoDS_Shape baseShape = part->Shape.getValue();

            // Add color information
            Quantity_Color col;

            std::set<int> face_index;
            TopTools_IndexedMapOfShape faces;
            TopExp_Explorer xp(baseShape,TopAbs_FACE);
            while (xp.More()) {
                face_index.insert(faces.Add(xp.Current()));
                xp.Next();
            }

            // define color per face?
            if (colors.size() == face_index.size()) {
                xp.Init(baseShape,TopAbs_FACE);
                while (xp.More()) {
                    int index = faces.FindIndex(xp.Current());
                    if (face_index.find(index) != face_index.end()) {
                        face_index.erase(index);
                        TDF_Label faceLabel = aShapeTool->AddSubShape(label, xp.Current());
                        // TDF_Label faceLabel= TDF_TagSource::NewChild(label);
                        aShapeTool->SetShape(faceLabel, xp.Current());
                        const App::Color& color = colors[index-1];
                        Standard_Real mat[3];
                        mat[0] = color.r;
                        mat[1] = color.g;
                        mat[2] = color.b;
                        col.SetValues(mat[0],mat[1],mat[2],Quantity_TOC_RGB);
                        aColorTool->SetColor(faceLabel, col, XCAFDoc_ColorSurf);
                    }

                    xp.Next();
                }
            }
            else if (!colors.empty()) {
                App::Color color = colors.front();
                Standard_Real mat[3];
                mat[0] = color.r;
                mat[1] = color.g;
                mat[2] = color.b;
                col.SetValues(mat[0],mat[1],mat[2],Quantity_TOC_RGB);
                aColorTool->SetColor(label, col, XCAFDoc_ColorGen);
            }
        }
    }
}




// This function is moving a "standard" node into an Assembly node within an XCAF doc

void ExportOCAF::pushNode(int root_id, int node_id, std::vector <TDF_Label>& hierarchical_label,std::vector <TopLoc_Location>& hierarchical_loc)
{
    TDF_Label root;
    TDF_Label node;
    root = hierarchical_label.at(root_id-1);
    node = hierarchical_label.at(node_id-1);

    XCAFDoc_DocumentTool::ShapeTool(root)->AddComponent(root, node, hierarchical_loc.at(node_id-1));
}

// ----------------------------------------------------------------------------

ImportXCAF::ImportXCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
    : hdoc(h), doc(d), default_name(name)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool (hdoc->Main());
    hColors = XCAFDoc_DocumentTool::ColorTool(hdoc->Main());
}

ImportXCAF::~ImportXCAF()
{
}

void ImportXCAF::loadShapes()
{
    // collect sequence of labels to display
    TDF_LabelSequence shapeLabels, colorLabels;
    aShapeTool->GetFreeShapes (shapeLabels);
    hColors->GetColors(colorLabels);

    // set presentations and show
    for (Standard_Integer i=1; i <= shapeLabels.Length(); i++ ) {
        // get the shapes and attributes
        const TDF_Label& label = shapeLabels.Value(i);
        loadShapes(label);
    }
    std::map<Standard_Integer, TopoDS_Shape>::iterator it;
    // go through solids
    for (it = mySolids.begin(); it != mySolids.end(); ++it) {
        createShape(it->second, true, true);
    }
    // go through shells
    for (it = myShells.begin(); it != myShells.end(); ++it) {
        createShape(it->second, true, true);
    }
    // go through compounds
    for (it = myCompds.begin(); it != myCompds.end(); ++it) {
        createShape(it->second, true, true);
    }
    // do the rest
    if (!myShapes.empty()) {
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        for (it = myShapes.begin(); it != myShapes.end(); ++it) {
            builder.Add(comp, it->second);
        }
        createShape(comp, true, false);
    }
}

void ImportXCAF::createShape(const TopoDS_Shape& shape, bool perface, bool setname) const
{
    Part::Feature* part;
    part = static_cast<Part::Feature*>(doc->addObject("Part::Feature", default_name.c_str()));
    part->Shape.setValue(shape);
    std::map<Standard_Integer, Quantity_Color>::const_iterator jt;
    jt = myColorMap.find(shape.HashCode(INT_MAX));

    App::Color partColor(0.8f,0.8f,0.8f);
#if 0//TODO
    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
    if (vp && vp->isDerivedFrom(PartGui::ViewProviderPart::getClassTypeId())) {
        if (jt != myColorMap.end()) {
            App::Color color;
            color.r = jt->second.Red();
            color.g = jt->second.Green();
            color.b = jt->second.Blue();
            static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.setValue(color);
        }

        partColor = static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.getValue();
    }
#endif

    // set label name if defined
    if (setname && !myNameMap.empty()) {
        std::map<Standard_Integer, std::string>::const_iterator jt;
        jt = myNameMap.find(shape.HashCode(INT_MAX));
        if (jt != myNameMap.end()) {
            part->Label.setValue(jt->second);
        }
    }

    // check for colors per face
    if (perface && !myColorMap.empty()) {
        TopTools_IndexedMapOfShape faces;
        TopExp_Explorer xp(shape,TopAbs_FACE);
        while (xp.More()) {
            faces.Add(xp.Current());
            xp.Next();
        }

        bool found_face_color = false;
        std::vector<App::Color> faceColors;
        faceColors.resize(faces.Extent(), partColor);
        xp.Init(shape,TopAbs_FACE);
        while (xp.More()) {
            jt = myColorMap.find(xp.Current().HashCode(INT_MAX));
            if (jt != myColorMap.end()) {
                int index = faces.FindIndex(xp.Current());
                App::Color color;
                color.r = (float)jt->second.Red();
                color.g = (float)jt->second.Green();
                color.b = (float)jt->second.Blue();
                faceColors[index-1] = color;
                found_face_color = true;
            }
            xp.Next();
        }

        if (found_face_color) {
#if 0//TODO
            Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
            if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
                static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.setValues(faceColors);
            }
#endif
        }
    }
}

void ImportXCAF::loadShapes(const TDF_Label& label)
{
    TopoDS_Shape aShape;
    if (aShapeTool->GetShape(label,aShape)) {
        //if (aShapeTool->IsReference(label)) {
        //    TDF_Label reflabel;
        //    if (aShapeTool->GetReferredShape(label, reflabel)) {
        //        loadShapes(reflabel);
        //    }
        //}
        if (aShapeTool->IsTopLevel(label)) {
            int ctSolids = 0, ctShells = 0, ctComps = 0;
            // add the shapes
            TopExp_Explorer xp;
            for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++)
                this->mySolids[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            for (xp.Init(aShape, TopAbs_SHELL, TopAbs_SOLID); xp.More(); xp.Next(), ctShells++)
                this->myShells[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            // if no solids and no shells were found then go for compounds
            if (ctSolids == 0 && ctShells == 0) {
                for (xp.Init(aShape, TopAbs_COMPOUND); xp.More(); xp.Next(), ctComps++)
                    this->myCompds[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            }
            if (ctComps == 0) {
                for (xp.Init(aShape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next())
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                for (xp.Init(aShape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next())
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                for (xp.Init(aShape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next())
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
                for (xp.Init(aShape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next())
                    this->myShapes[xp.Current().HashCode(INT_MAX)] = (xp.Current());
            }
        }

        // getting color
        Quantity_Color col;
        if (hColors->GetColor(label, XCAFDoc_ColorGen, col) ||
            hColors->GetColor(label, XCAFDoc_ColorSurf, col) ||
            hColors->GetColor(label, XCAFDoc_ColorCurv, col)) {
            // add defined color
            myColorMap[aShape.HashCode(INT_MAX)] = col;
        }
        else {
            // http://www.opencascade.org/org/forum/thread_17107/
            TopoDS_Iterator it;
            for (it.Initialize(aShape);it.More(); it.Next()) {
                if (hColors->GetColor(it.Value(), XCAFDoc_ColorGen, col) ||
                    hColors->GetColor(it.Value(), XCAFDoc_ColorSurf, col) ||
                    hColors->GetColor(it.Value(), XCAFDoc_ColorCurv, col)) {
                    // add defined color
                    myColorMap[it.Value().HashCode(INT_MAX)] = col;
                }
            }
        }

        // getting names
        Handle(TDataStd_Name) name;
        if (label.FindAttribute(TDataStd_Name::GetID(),name)) {
            TCollection_ExtendedString extstr = name->Get();
            char* str = new char[extstr.LengthOfCString()+1];
            extstr.ToUTF8CString(str);
            std::string label(str);
            if (!label.empty())
                myNameMap[aShape.HashCode(INT_MAX)] = label;
            delete [] str;
        }

#if 0
        // http://www.opencascade.org/org/forum/thread_15174/
        if (aShapeTool->IsAssembly(label)) {
            TDF_LabelSequence shapeLabels;
            aShapeTool->GetComponents(label, shapeLabels);
            Standard_Integer nbShapes = shapeLabels.Length();
            for (Standard_Integer i = 1; i <= nbShapes; i++) {
                loadShapes(shapeLabels.Value(i));
            }
        }
#endif

        if (label.HasChild()) {
            TDF_ChildIterator it;
            for (it.Initialize(label); it.More(); it.Next()) {
                loadShapes(it.Value());
            }
        }
    }
}
