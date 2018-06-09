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

#include "PreCompiled.h"
#if defined(__MINGW32__)
# define WNT // avoid conflict with GUID
#endif
#ifndef _PreComp_
# include <gp_Trsf.hxx>
# include <TopExp_Explorer.hxx>
# include <Standard_Failure.hxx>
# include <Standard_Version.hxx>
# include <XCAFApp_Application.hxx>
# include <XCAFDoc_DocumentTool.hxx>
# include <XCAFDoc_ShapeTool.hxx>
# include <XCAFDoc_ColorTool.hxx>
# include <XCAFDoc_Location.hxx>
# include <XCAFDoc_GraphNode.hxx>
# include <TDF_Label.hxx>
# include <TDF_Tool.hxx>
# include <TDF_LabelSequence.hxx>
# include <TDF_ChildIterator.hxx>
# include <TDataStd_Name.hxx>
# include <Quantity_Color.hxx>
# include <TopoDS_Iterator.hxx>
# include <Interface_Static.hxx>
#endif

#include <boost/algorithm/string.hpp>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/Part.h>
#include <App/Link.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/FeatureCompound.h>
#include "ImportOCAF2.h"
#include <Mod/Part/App/ProgressIndicator.h>
#include <Mod/Part/App/ImportIges.h>
#include <Mod/Part/App/ImportStep.h>

#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>

FC_LOG_LEVEL_INIT("Import",true,true)

using namespace Import;

/////////////////////////////////////////////////////////////////////

static std::string labelName(TDF_Label label) {
    std::string txt;
    Handle(TDataStd_Name) name;
    if (!label.IsNull() && label.FindAttribute(TDataStd_Name::GetID(),name)) {
        TCollection_ExtendedString extstr = name->Get();
        char* str = new char[extstr.LengthOfCString()+1];
        extstr.ToUTF8CString(str);
        txt = str;
        delete[] str;
        boost::trim(txt);
    }
    return txt;
}

static void printLabel(TDF_Label label, Handle(XCAFDoc_ShapeTool) aShapeTool,
    Handle(XCAFDoc_ColorTool) aColorTool, const char *msg = 0) 
{
    if(label.IsNull() || !FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
        return;
    if(!msg) msg = "Label: ";
    TCollection_AsciiString entry;
    TDF_Tool::Entry(label,entry);
    std::ostringstream ss;
    ss << msg << entry << ", " << labelName(label)
       << (aShapeTool->IsShape(label)?", shape":"")
       << (aShapeTool->IsTopLevel(label)?", topLevel":"")
       << (aShapeTool->IsFree(label)?", free":"")
       << (aShapeTool->IsAssembly(label)?", assembly":"")
       << (aShapeTool->IsSimpleShape(label)?", simple":"")
       << (aShapeTool->IsCompound(label)?", compound":"")
       << (aShapeTool->IsReference(label)?", reference":"")
       << (aShapeTool->IsComponent(label)?", component":"")
       << (aShapeTool->IsSubShape(label)?", subshape":"");
    if(aShapeTool->IsShape(label)) {
        Quantity_Color c;
        if(aColorTool->GetColor(label,XCAFDoc_ColorGen,c))
            ss << ", gc: " << c.StringName(c.Name());
        if(aColorTool->GetColor(label,XCAFDoc_ColorSurf,c))
            ss << ", sc: " << c.StringName(c.Name());
        if(aColorTool->GetColor(label,XCAFDoc_ColorCurv,c))
            ss << ", cc: " << c.StringName(c.Name());
    }

    ss << std::endl;
    Base::Console().NotifyLog(ss.str().c_str());
}

static void dumpLabels(TDF_Label label, Handle(XCAFDoc_ShapeTool) aShapeTool, 
    Handle(XCAFDoc_ColorTool) aColorTool, int depth=0)
{
    std::string indent(depth*2,' ');
    printLabel(label,aShapeTool,aColorTool,indent.c_str());
    TDF_ChildIterator it;
    for (it.Initialize(label); it.More(); it.Next())
        dumpLabels(it.Value(),aShapeTool,aColorTool,depth+1);
}

/////////////////////////////////////////////////////////////////////

ImportOCAF2::ImportOCAF2(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
    : pDoc(h), doc(d), default_name(name)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool (pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Import/hSTEP");
    merge = hGrp->GetBool("ReadShapeCompoundMode", true);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Import");
    useLinkGroup = hGrp->GetBool("UseLinkGroup",true);
    useBaseName = hGrp->GetBool("UseBaseName",true);
    importHidden = hGrp->GetBool("ImportHiddenObject",true);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    defaultFaceColor.setPackedValue(hGrp->GetUnsigned("DefaultShapeColor",0xCCCCCC00));
    defaultFaceColor.a = 0;

    defaultEdgeColor.setPackedValue(hGrp->GetUnsigned("DefaultShapeLineColor",421075455UL));
    defaultEdgeColor.a = 0;

    if(!useLinkGroup) {
        Interface_Static::SetIVal("read.stepcaf.subshapes.name",1);
        aShapeTool->SetAutoNaming(Standard_False);
    }
}

ImportOCAF2::~ImportOCAF2()
{
}

static void setPlacement(App::PropertyPlacement *prop, const TopoDS_Shape &shape) {
    prop->setValue(Base::Placement(Part::TopoShape::convert(shape.Location().Transformation())));
}

std::string ImportOCAF2::getLabelName(TDF_Label label) {
    std::string name;
    if(label.IsNull())
        return name;
    if(!XCAFDoc_ShapeTool::IsReference(label))
        return labelName(label);
    if(!useBaseName)
        name = labelName(label);
    TDF_Label ref;
    if(name.empty() && XCAFDoc_ShapeTool::GetReferredShape(label,ref))
        name = labelName(ref);
    return name;
}

void ImportOCAF2::setObjectName(Info &info, TDF_Label label) {
    if(!info.obj)
        return;
    info.baseName = getLabelName(label);
    if(info.baseName.size())
        info.obj->Label.setValue(info.baseName.c_str());
    else{
        auto linked = info.obj->getLinkedObject(false);
        if(!linked || linked==info.obj)
            return;
        info.obj->Label.setValue(linked->Label.getValue());
    }
}


bool ImportOCAF2::getColor(const TopoDS_Shape &shape, Info &info, bool check, bool noDefault) {
    bool ret = false;
    Quantity_Color aColor;
    if(aColorTool->GetColor(shape, XCAFDoc_ColorSurf, aColor)) {
        App::Color c(aColor.Red(),aColor.Green(),aColor.Blue());
        if(!check || info.faceColor!=c) {
            info.faceColor = c;
            info.hasFaceColor = true;
            ret = true;
        }
    }
    if(!noDefault && !info.hasFaceColor && aColorTool->GetColor(shape, XCAFDoc_ColorGen, aColor)) {
        App::Color c(aColor.Red(),aColor.Green(),aColor.Blue());
        if(!check || info.faceColor!=c) {
            info.faceColor = c;
            info.hasFaceColor = true;
            ret = true;
        }
    }
    if(aColorTool->GetColor(shape, XCAFDoc_ColorCurv, aColor)) {
        App::Color c(aColor.Red(),aColor.Green(),aColor.Blue());
        // Some STEP include a curve color with the same value of the face
        // color. And this will look weird in FC. So for shape with face
        // we'll ignore the curve color, if it is the same as the face color.
        if((c!=info.faceColor || !TopExp_Explorer(shape,TopAbs_FACE).More()) &&
           (!check || info.edgeColor!=c)) 
        {
            info.edgeColor = c;
            info.hasEdgeColor = true;
            ret = true;
        }
    }
    if(!check) {
        if(!info.hasFaceColor)
            info.faceColor = defaultFaceColor;
        if(!info.hasEdgeColor)
            info.edgeColor = defaultEdgeColor;
    }
    return ret;
}

App::DocumentObject *ImportOCAF2::expandShape(const TopoDS_Shape &shape) {
    if(shape.IsNull() || !TopExp_Explorer(shape,TopAbs_VERTEX).More())
        return 0;

    // When saved as compound, STEP file does not support instance sharing,
    // meaning that even if the source compound may contain child shapes of
    // shared instances, or multiple hierarchies, those information are lost
    // when saved to STEP, everything become flat and duplicated. So the code
    // below is not necessary.
#if 0
    auto baseShape = shape.Located(TopLoc_Location());
    auto it = myShapes.find(baseShape);
    if(it!=myShapes.end()) {
        auto link = static_cast<App::Link*>(doc->addObject("App::Link","Link"));
        link->Visibility.setValue(false);
        link->setLink(-1,it->second.obj);
        setPlacement(&link->Placement,shape);
        return link;
    }
#endif
    std::vector<App::DocumentObject*> objs;

    if(shape.ShapeType() == TopAbs_COMPOUND) {
        for(TopoDS_Iterator it(shape,0,0);it.More();it.Next()) {
            auto child = expandShape(it.Value());
            if(child) {
                objs.push_back(child);
                Info info;
                info.free = false;
                info.obj = child;
                myShapes.emplace(it.Value().Located(TopLoc_Location()),info);
            }
        }
        if(objs.empty())
            return 0;
        auto compound = static_cast<Part::Compound2*>(doc->addObject("Part::Compound2","Compound"));
        compound->Links.setValues(objs);
        compound->Visibility.setValue(false);
        setPlacement(&compound->Placement,shape);
        return compound;
    }
    auto feature = static_cast<Part::Feature*>(doc->addObject("Part::Feature",
                Part::TopoShape::shapeName(shape.ShapeType()).c_str()));
    feature->Shape.setValue(shape);
    feature->Visibility.setValue(false);
    return feature;
}

bool ImportOCAF2::createObject(TDF_Label label, const TopoDS_Shape &shape, Info &info)
{
    if(shape.IsNull() || !TopExp_Explorer(shape,TopAbs_VERTEX).More()) {
        FC_WARN(labelName(label) << " has empty shape");
        return false;
    }

    getColor(shape,info);
    bool hasFaceColors = false;
    bool hasEdgeColors = false;

    Part::TopoShape tshape(shape);
    std::vector<App::Color> faceColors;
    std::vector<App::Color> edgeColors;

    TDF_LabelSequence seq;
    if(!label.IsNull() && aShapeTool->GetSubShapes(label,seq)) {
        faceColors.assign(tshape.countSubShapes(TopAbs_FACE),info.faceColor);
        edgeColors.assign(tshape.countSubShapes(TopAbs_EDGE),info.edgeColor);
        for(int i=1;i<=seq.Length();++i) {
            TDF_Label l = seq.Value(i);
            TopoDS_Shape subShape = aShapeTool->GetShape(l);
            if(subShape.IsNull())
                continue;
            Quantity_Color aColor;
            int idx = tshape.findShape(subShape)-1;
            if(idx<0)
                continue;
            switch(subShape.ShapeType()) {
            case TopAbs_FACE:
                if(!aColorTool->GetColor(l, XCAFDoc_ColorSurf, aColor) && 
                   !aColorTool->GetColor(l, XCAFDoc_ColorGen, aColor))
                    continue;
                assert(idx < (int)faceColors.size());
                faceColors[idx] = App::Color(aColor.Red(),aColor.Green(),aColor.Blue());
                hasFaceColors = true;
                info.hasFaceColor = true;
                break;
            case TopAbs_EDGE:
                if(!aColorTool->GetColor(l, XCAFDoc_ColorCurv, aColor)) 
                    continue;
                assert(idx < (int)edgeColors.size());
                edgeColors[idx] = App::Color(aColor.Red(),aColor.Green(),aColor.Blue());
                hasEdgeColors = true;
                info.hasEdgeColor = true;
                break;
            default:
                continue;
            }
        }
    }

    Part::Feature *feature;

    if(tshape.countSubShapes(TopAbs_SOLID)>1 || 
       (tshape.countSubShapes(TopAbs_SOLID)<=1 && tshape.countSubShapes(TopAbs_SHELL)>1))
    {
        feature = dynamic_cast<Part::Feature*>(expandShape(shape));
        assert(feature);
    } else {
        feature = static_cast<Part::Feature*>(doc->addObject("Part::Feature","Feature"));
        feature->Shape.setValue(shape);
        feature->Visibility.setValue(false);
    }
    applyFaceColors(feature,{info.faceColor});
    applyEdgeColors(feature,{info.edgeColor});
    if(hasFaceColors)
        applyFaceColors(feature,faceColors);
    if(hasEdgeColors)
        applyEdgeColors(feature,edgeColors);

    info.propPlacement = &feature->Placement;
    info.obj = feature;
    return true;
}

bool ImportOCAF2::createGroup(Info &info, const TopoDS_Shape &shape, 
                             const std::vector<App::DocumentObject*> &children, 
                             const boost::dynamic_bitset<> &visibilities) 
{
    assert(children.size() == visibilities.size());
    if(children.empty())
        return false;
    if(useLinkGroup) {
        auto group = static_cast<App::LinkGroup*>(doc->addObject("App::LinkGroup","LinkGroup"));
        group->Visibility.setValue(false);
        group->ElementList.setValues(children);
        group->VisibilityList.setValue(visibilities);
        for(auto child : children) 
            child->Visibility.setValue(false);
        info.obj = group;
        info.propPlacement = &group->Placement;
        if(getColor(shape,info,false,true)) {
            if(info.hasFaceColor)
                applyLinkColor(group,-1,info.faceColor);
        }
        return true;
    }
    auto group = static_cast<App::Part*>(doc->addObject("App::Part","Part"));
    group->Visibility.setValue(false);
    group->addObjects(children);
    int i=0;
    for(auto child : children)
        child->Visibility.setValue(visibilities[i++]);
    info.obj = group;
    info.propPlacement = &group->Placement;
    return true;
}

App::DocumentObject* ImportOCAF2::loadShapes()
{
    if(!useLinkGroup) {
        ImportLegacy legacy(*this);
        legacy.setMerge(merge);
        legacy.loadShapes();
        return 0;
    }

    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
        dumpLabels(pDoc->Main(),aShapeTool,aColorTool);

    myShapes.clear();
    myNames.clear();

    std::vector<App::DocumentObject*> objs;
    TDF_LabelSequence labels;
    aShapeTool->GetFreeShapes (labels);
    boost::dynamic_bitset<> vis;
    for (Standard_Integer i=1; i <= labels.Length(); i++ ) {
        auto label = labels.Value(i);
        if(!importHidden && !aColorTool->IsVisible(label))
            continue;
        auto obj = loadShape(label, aShapeTool->GetShape(label));
        if(obj) {
            objs.push_back(obj);
            vis.push_back(aColorTool->IsVisible(label));
        }
    }
    App::DocumentObject *ret = 0;
    if(objs.size()==1) {
        ret = objs.front();
    }else {
        Info info;
        if(createGroup(info,TopoDS_Shape(),objs,vis))
            ret = info.obj;
    }
    if(ret) {
        ret->Visibility.setValue(true);
        ret->recomputeFeature(true);
    }
    if(merge && ret && !ret->isDerivedFrom(Part::Feature::getClassTypeId())) {
        auto shape = Part::Feature::getTopoShape(ret);
        auto feature = static_cast<Part::Feature*>(
                doc->addObject("Part::Feature", "Feature"));
        auto name = labelName(pDoc->Main());
        feature->Label.setValue(name.empty()?default_name.c_str():name.c_str());
        feature->Shape.setValue(shape);
        applyFaceColors(feature,{});

        std::vector<std::pair<App::Document*,std::string> > objNames;
        for(auto obj : App::Document::getDependencyList(objs,false,true))
            objNames.emplace_back(obj->getDocument(),obj->getNameInDocument());
        for(auto &v : objNames)
            v.first->removeObject(v.second.c_str());
        ret = feature;
        ret->recomputeFeature(true);
    }
    return ret;
}

void ImportOCAF2::getSHUOColors(TDF_Label label, 
        std::map<std::string,App::Color> &colors, bool appendFirst)
{
    TDF_AttributeSequence seq;
    if(label.IsNull() || !aShapeTool->GetAllComponentSHUO(label,seq))
        return;
    std::ostringstream ss;
    for(int i=1;i<=seq.Length();++i) {
        Handle(XCAFDoc_GraphNode) shuo = Handle(XCAFDoc_GraphNode)::DownCast(seq.Value(i));
        if(shuo.IsNull())
            continue;

        TDF_Label slabel = shuo->Label();

        // We only want to process the main shuo, i.e. those without upper_usage
        TDF_LabelSequence uppers;
        aShapeTool->GetSHUOUpperUsage(slabel, uppers);
        if(uppers.Length())
            continue;

        // appendFirst tells us whether we shall append the object name of the first label
        bool skipFirst = !appendFirst;
        ss.str("");
        while(1) {
            if(skipFirst)
                skipFirst = false;
            else {
                TDF_Label l = shuo->Label().Father();
                auto it = myNames.find(l);
                if(it == myNames.end()) {
                    FC_WARN("Failed to find object of label " << labelName(l));
                    ss.str("");
                    break;
                }
                ss << it->second << '.';
            }
            if(!shuo->NbChildren())
                break;
            shuo = shuo->GetChild(1);
        }
        std::string subname = ss.str();
        if(subname.empty())
            continue;
        if(!aColorTool->IsVisible(slabel)) {
            subname += App::DocumentObject::hiddenMarker();
            colors.emplace(subname,App::Color());
        } else {
            Quantity_Color aColor;
            if(aColorTool->GetColor(slabel, XCAFDoc_ColorSurf, aColor) ||
               aColorTool->GetColor(slabel, XCAFDoc_ColorGen, aColor))
            {
                colors.emplace(subname,App::Color(aColor.Red(),aColor.Green(),aColor.Blue()));
            }
        }
    }
}

App::DocumentObject *ImportOCAF2::loadShape(TDF_Label label, const TopoDS_Shape &shape, bool isArrayElement) 
{
    if(shape.IsNull())
        return 0;

    auto baseShape = shape.Located(TopLoc_Location());
    auto it = myShapes.find(baseShape);
    if(it == myShapes.end()) {
        Info info;
        auto baseLabel = aShapeTool->FindShape(baseShape);
        bool res;
        if(baseLabel.IsNull() || !aShapeTool->IsAssembly(baseLabel))
            res = createObject(baseLabel,baseShape,info);
        else
            res = createAssembly(baseLabel,baseShape,info);
        if(!res)
            return 0;
        setObjectName(info,baseLabel);
        it = myShapes.emplace(baseShape,info).first;
    }
    if(isArrayElement)
        return it->second.obj;

    std::map<std::string,App::Color> shuoColors;
    if(!useLinkGroup)
        getSHUOColors(label,shuoColors,false);

    auto info = it->second;
    getColor(shape,info,true);

    if(shuoColors.empty() && info.free) {
        it->second.free = false;
        auto name = getLabelName(label);
        if(info.faceColor!=it->second.faceColor ||
           info.edgeColor!=it->second.edgeColor ||
           (name.size() && info.baseName.size() && name!=info.baseName)) 
        {
            auto compound = static_cast<Part::Compound2*>(doc->addObject("Part::Compound2","Compound"));
            compound->Links.setValue(info.obj);
            compound->Visibility.setValue(false);
            info.propPlacement = &compound->Placement;
            if(info.faceColor!=it->second.faceColor)
                applyFaceColors(compound,{info.faceColor});
            if(info.edgeColor!=it->second.edgeColor)
                applyEdgeColors(compound,{info.edgeColor});
            info.obj = compound;
            setObjectName(info,label);
        }
        setPlacement(info.propPlacement,shape);
        myNames.emplace(label,info.obj->getNameInDocument());
        return info.obj;
    }

    auto link = static_cast<App::Link*>(doc->addObject("App::Link","Link"));
    link->Visibility.setValue(false);
    link->setLink(-1,info.obj);
    setPlacement(&link->Placement,shape);
    info.obj = link;
    setObjectName(info,label);
    if(info.faceColor!=it->second.faceColor)
        applyLinkColor(link,-1,info.faceColor);

    myNames.emplace(label,link->getNameInDocument());
    if(shuoColors.size())
        applyElementColors(link,shuoColors);
    return link;
}

bool ImportOCAF2::createAssembly(TDF_Label label, const TopoDS_Shape &shape, Info &info)
{
    (void)label;

    std::vector<TDF_Label> childLabels;
    std::vector<TopoDS_Shape> childShapes;
    boost::dynamic_bitset<> visibilities;

    for(TopoDS_Iterator it(shape,0,0);it.More();it.Next()) {
        TopoDS_Shape childShape = it.Value();
        if(childShape.IsNull())
            continue;
        TDF_Label childLabel;
        aShapeTool->Search(childShape,childLabel,Standard_True,Standard_True,Standard_False);
        if(childLabel.IsNull() && !importHidden && !aColorTool->IsVisible(childLabel))
            continue;
        childShapes.push_back(childShape);
        childLabels.push_back(childLabel);
        if(!childLabel.IsNull() && aShapeTool->IsComponent(childLabel))
            visibilities.push_back(aColorTool->IsVisible(childLabel));
        else
            visibilities.push_back(true);
    }

    if(childShapes.empty())
        return false;

    // Special check for array, i.e. check if all children refer to the same shape
    TopoDS_Shape baseShape;
    std::vector<Base::Placement> placements;
    std::map<size_t,App::Color> colors;
    for(const auto &childShape : childShapes) {
        if(baseShape.IsNull()) {
            baseShape = childShape.Located(TopLoc_Location());
        }else if(childShape.Located(TopLoc_Location())!=baseShape) {
            placements.clear();
            break;
        }
        placements.emplace_back(Part::TopoShape::convert(childShape.Location().Transformation()));

        Quantity_Color aColor;
        if (aColorTool->GetColor(childShape, XCAFDoc_ColorSurf, aColor)) {
            auto idx = placements.size()-1;
            auto &color = colors[idx];
            color.r = (float)aColor.Red();
            color.g = (float)aColor.Green();
            color.b = (float)aColor.Blue();
        }
    }

    std::map<std::string,App::Color> shuoColors;

    if(placements.size()>1) {
        // Okay, we are creating a link array
        App::DocumentObject *obj = loadShape(childLabels.front(),childShapes.front(),true);
        if(!obj)
            return false;
        auto link = static_cast<App::Link*>(doc->addObject("App::Link","Link"));
        link->Visibility.setValue(false);
        link->setLink(-1,obj);
        link->ShowElement.setValue(false);
        link->ElementCount.setValue(placements.size());
        link->PlacementList.setValue(placements);
        link->VisibilityList.setValue(visibilities);

        for(auto &v : colors)
            applyLinkColor(link,v.first,v.second);

        info.obj = link;
        info.propPlacement = &link->Placement;
        getColor(shape,info);
        if(info.hasFaceColor)
            applyLinkColor(link,-1,info.faceColor);
        
        int i=0;
        for(auto childLabel : childLabels) {
            myNames.emplace(childLabel,std::to_string(i++));
            getSHUOColors(childLabel,shuoColors,true);
        }
        if(shuoColors.size())
            applyElementColors(link,shuoColors);
        return true;
    }
    // Not a link array, create normal group
    std::vector<App::DocumentObject *> children;
    visibilities.clear();
    int i=0;
    for(auto childLabel : childLabels) {
        auto child = loadShape(childLabel,childShapes[i++]);
        if(!child)
            continue;
        children.push_back(child);
        if(!childLabel.IsNull() && aShapeTool->IsComponent(childLabel))
            visibilities.push_back(aColorTool->IsVisible(childLabel));
        else
            visibilities.push_back(true);
        if(useLinkGroup)
            getSHUOColors(childLabel,shuoColors,true);
    }

    if(children.empty())
        return false;

    if(!createGroup(info,shape,children,visibilities))
        return false;
    if(shuoColors.size())
        applyElementColors(info.obj,shuoColors);
    return true;
}

// ----------------------------------------------------------------------------

ExportOCAF2::ExportOCAF2(Handle(TDocStd_Document) h, GetShapeColorsFunc func)
    : pDoc(h) , getShapeColors(func)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool(pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Import");
    exportHidden = hGrp->GetBool("ExportHiddenObject",true);

    Interface_Static::SetIVal("write.step.assembly",2);

    auto handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    defaultColor.setPackedValue(handle->GetUnsigned("DefaultShapeColor",0xCCCCCC00));
    defaultColor.a = 0;
}

void ExportOCAF2::setName(TDF_Label label, App::DocumentObject *obj, const char *name) {
    if(!name) {
        if(!obj)
            return;
        name = obj->Label.getValue();
    }
    TDataStd_Name::Set(label, TCollection_ExtendedString(name, 1));
}

// Similar to XCAFDoc_ShapeTool::FindSHUO but return only main SHUO, i.e. SHUO
// with no upper_usage. It should not be necssary if we strictly export from
// bottom up, but let's make sure of it.
static Standard_Boolean FindSHUO (const TDF_LabelSequence& theLabels,
                                  Handle(XCAFDoc_GraphNode)& theSHUOAttr)
{
    assert(theLabels.Length()>1);
    theSHUOAttr.Nullify();
    TDF_AttributeSequence SHUOAttrs;
    TDF_Label aCompLabel = theLabels.Value(1);
    if (! ::XCAFDoc_ShapeTool::GetAllComponentSHUO( aCompLabel, SHUOAttrs ) )
        return Standard_False;
    for (Standard_Integer i = 1; i <= SHUOAttrs.Length(); i++) {
        Handle(XCAFDoc_GraphNode) anSHUO = Handle(XCAFDoc_GraphNode)::DownCast(SHUOAttrs.Value(i));
        TDF_LabelSequence aUpLabels;
        // check for any upper_usage
        ::XCAFDoc_ShapeTool::GetSHUOUpperUsage( anSHUO->Label(), aUpLabels );
        if ( aUpLabels.Length() > 0 )
            continue; // reject if there is one
        int j=2;
        for ( ; anSHUO->NbChildren() ; ++j ) {
            if ( j>theLabels.Length() ) {
                j=0;
                break;
            }
            anSHUO = anSHUO->GetChild( 1 );
            if ( theLabels.Value(j)!=anSHUO->Label().Father() ) {
                j=0;
                break;
            }
        }
        if( j!=theLabels.Length()+1 )
            continue;

        theSHUOAttr = Handle(XCAFDoc_GraphNode)::DownCast(SHUOAttrs.Value(i));
        break;
    }
    return ( !theSHUOAttr.IsNull() );
}

TDF_Label ExportOCAF2::findComponent(const char *subname, TDF_Label label, TDF_LabelSequence &labels) {
    const char *dot = strchr(subname,'.');
    if(!dot) {
        if(labels.Length()==1)
            return labels.Value(1);
        Handle(XCAFDoc_GraphNode) ret;
        if(labels.Length() && (FindSHUO(labels,ret) || aShapeTool->SetSHUO(labels,ret)))
            return ret->Label();
        return TDF_Label();
    }
    TDF_LabelSequence components;
    TDF_Label ref;
    if(!aShapeTool->GetReferredShape(label,ref))
        ref = label;
    if(aShapeTool->GetComponents(ref,components)) {
        for(int i=1;i<=components.Length();++i) {
            auto component = components.Value(i);
            auto it = myNames.find(component);
            if(it == myNames.end())
                continue;
            for(auto &n : it->second) {
                if(boost::starts_with(subname,n)) {
                    labels.Append(component);
                    return findComponent(subname+n.size(),component,labels);
                }
            }
        }
    }
    return TDF_Label();
}

void ExportOCAF2::setupObject(TDF_Label label, App::DocumentObject *obj, 
        const Part::TopoShape &shape, const std::string &prefix, const char *name)
{
    setName(label,obj,name);
    if(aShapeTool->IsComponent(label)) {
        auto &names = myNames[label];
        // The subname reference may contain several possible namings.
        if(!name) {
            // simple object internal name
            names.push_back(prefix+obj->getNameInDocument()+".");
        } else {
            // name is not NULL in case this is a collapsed link array element.
            // Collapsed means that the element is not an actual object, and
            // 'obj' here is actually the parent. The given 'name' is in fact
            // the element index
            names.push_back(prefix + name + ".");
            // In case the subname reference is created when the link array is
            // previously expanded, the element object will be named as the
            // parent object internal name + '_i<index>'
            names.push_back(prefix + obj->getNameInDocument() + "_i" + name + ".");
        }
        // Finally, the subname reference allows to use the label for naming
        // with preceeding '$'
        names.push_back(prefix + "$" + obj->Label.getValue() + ".");
    }

    if(!getShapeColors || !mySetups.emplace(obj,name?name:"").second)
        return;

    std::map<std::string, std::map<std::string,App::Color> > colors;
    static std::string marker(App::DocumentObject::hiddenMarker()+"*");
    static std::array<const char *,3> keys = {"Face*","Edge*",marker.c_str()};
    std::string childName;
    if(name) {
        childName = name;
        childName += '.';
    }
    for(auto key : keys) {
        for(auto &v : getShapeColors(obj,key)) {
            const char *subname = v.first.c_str();
            if(name) {
                if(!boost::starts_with(v.first,childName))
                    continue;
                subname += childName.size();
            }
            const char *dot = strrchr(subname,'.');
            if(!dot)
                colors[""].insert(v);
            else {
                ++dot;
                colors[std::string(subname,dot-subname)].emplace(dot,v.second);
            }
        }
    }

    bool warned = false;

    for(auto &v : colors) {
        TDF_Label nodeLabel = label;
        Handle(XCAFDoc_GraphNode) shuo;
        if(v.first.size()) {
            TDF_LabelSequence labels;
            if(aShapeTool->IsComponent(label))
                labels.Append(label);
            nodeLabel = findComponent(v.first.c_str(),label,labels);
            if(nodeLabel.IsNull()) {
                FC_WARN("Failed to find component " << v.first);
                continue;
            }
        }
        for(auto &vv : v.second) {
            if(vv.first == App::DocumentObject::hiddenMarker()) {
                aColorTool->SetVisibility(nodeLabel,Standard_False);
                continue;
            }

            auto colorType = vv.first[0]=='F'?XCAFDoc_ColorSurf:XCAFDoc_ColorCurv;
            const App::Color& c = vv.second;
            Quantity_Color color(c.r,c.g,c.b,Quantity_TOC_RGB);

            if(boost::ends_with(vv.first,"*")) {
                aColorTool->SetColor(nodeLabel, color, colorType);
                continue;
            }

            if(nodeLabel!=label || aShapeTool->IsComponent(label)) {
                // OCCT 7 seems to only support "Recommended practices for
                // model styling and organization" version 1.2
                // (https://www.cax-if.org/documents/rec_prac_styling_org_v12.pdf).
                // The SHUO described in section 5.3 does not mention the
                // capability of overriding context-depdendent element color,
                // only whole shape color. Newer version of the same document
                // (https://www.cax-if.org/documents/rec_prac_styling_org_v15.pdf)
                // does support this, in section 5.1. 
                //
                // The above observation is confirmed by further inspection of
                // OCCT code, XCAFDoc_ShapeTool.cxx and STEPCAFControl_Writer.cxx.
                if(!warned) {
                    warned = true;
                    FC_WARN("Current OCCT does not support element color override, for object "
                            << obj->getNameInDocument());
                }
                continue;
            }

            auto subShape = shape.getSubShape(vv.first.c_str(),true);
            if(subShape.IsNull()) {
                FC_WARN("Failed to get subshape " << vv.first);
                continue;
            }
            TDF_Label subLabel = aShapeTool->AddSubShape(nodeLabel, subShape);
            aColorTool->SetColor(subLabel, color, colorType);
        }
    }
}

void ExportOCAF2::exportObjects(std::vector<App::DocumentObject*> &objs, const char *name) {
    if(objs.empty())
        return;
    myObjects.clear();
    myNames.clear();
    mySetups.clear();
    if(objs.size()==1)
        exportObject(objs.front(),0,TDF_Label());
    else {
        auto label = aShapeTool->NewShape();
        App::Document *doc = 0;
        bool sameDoc = true;
        for(auto obj : objs) {
            if(doc)
                sameDoc = sameDoc && doc==obj->getDocument();
            else
                doc = obj->getDocument();
            exportObject(obj,0,label);
        }

        if(!name && doc && sameDoc)
            name = doc->getName();
        setName(label,0,name);
    }

    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
        dumpLabels(pDoc->Main(),aShapeTool,aColorTool);
}

TDF_Label ExportOCAF2::exportObject(App::DocumentObject* parentObj, 
        const char *sub, TDF_Label parent, const char *name) 
{
    App::DocumentObject *obj;
    auto shape = Part::Feature::getTopoShape(parentObj,sub,false,0,&obj,false,!sub);
    if(!obj || shape.isNull()) {
        FC_WARN(obj->getNameInDocument() << " has null shape");
        return TDF_Label();
    }

    //sub may contain more than one hierarchy, e.g. Assembly container may use
    //getSubObjects to skip some hierarchy containing constraints and stuff
    //when exporting. We search for extra '.', and set it as prefix if found.
    //When setting SHUO's, we'll need this prefix for matching.
    std::string prefix;
    if(sub) {
        auto len = strlen(sub);
        if(len>1) {
            --len;
            // The prefix ends with the second last '.', so search for it.
            for(int i=0;len!=0;--len) {
                if(sub[len]=='.' && ++i==2) {
                    prefix = std::string(sub,len+1);
                    break;
                }
            }
        }
    }

    TDF_Label label;
    std::vector<App::DocumentObject *> links;

    int depth = 0;
    auto linked = obj;
    auto linkedShape = shape;
    while(1) {
        auto s = Part::Feature::getTopoShape(linked);
        if(s.isNull() || !s.getShape().IsPartner(shape.getShape()))
            break;
        linkedShape = s;
        // Search using our own cache. We can't rely on ShapeTool::FindShape()
        // in case this is an assembly. Because FindShape() search among its
        // own computed shape, i.e. its own created compound, and thus will
        // never match ours.
        auto it = myObjects.find(linked);
        if(it != myObjects.end()) {
            for(auto l : links)
                myObjects.emplace(l,it->second);
            // Note: OCAF does not seem to support reference of references. We
            // have to flaten all multi-level link without scales. In other
            // word, all link will all be forced to refer to the same
            // non-located shape
            
            // retrieve OCAF computed shape, in case the current object returns
            // a new shape every time Part::Feature::getTopoShape() is called.
            auto baseShape = aShapeTool->GetShape(it->second);
            shape.setShape(baseShape.Located(shape.getShape().Location()),false);
            if(!parent.IsNull())
                label = aShapeTool->AddComponent(parent,shape.getShape(),Standard_False);
            else
                label = aShapeTool->AddShape(shape.getShape(),Standard_False,Standard_False);
            setupObject(label,name?parentObj:obj,shape,prefix,name);
            return label;
        }
        auto next = linked->getLinkedObject(false,0,false,depth++);
        if(!next || linked==next)
            break;
        linked = next;
        links.push_back(linked);
    }

    auto subs = obj->getSubObjects();
    // subs empty means obj is not a container.
    if(subs.empty()) {

        // Search for non-located shape to see if we've stored the original shape before
        if(!aShapeTool->FindShape(shape.getShape(),label)) {
            auto baseShape = linkedShape;
            auto linked = links.empty()?obj:links.back();
            baseShape.setShape(baseShape.getShape().Located(TopLoc_Location()),false);
            label = aShapeTool->NewShape();
            aShapeTool->SetShape(label,baseShape.getShape());
            setupObject(label,linked,baseShape,prefix);
        }

        if(!parent.IsNull()) 
            label = aShapeTool->AddComponent(parent,shape.getShape(),Standard_False);
        setupObject(label,name?parentObj:obj,shape,prefix,name);

        myObjects.emplace(obj, label);
        for(auto link : links)
            myObjects.emplace(link, label);
        return label;
    }

    // Create a new assembly
    label = aShapeTool->NewShape();

    // check for link array
    auto linkArray = obj->getLinkedObject(true)->getExtensionByType<App::LinkBaseExtension>(true);
    if(linkArray && (linkArray->getShowElementValue() || !linkArray->getElementCountValue()))
        linkArray = 0;
    for(auto &sub : subs) {
        App::DocumentObject *parent = 0;
        std::string childName;
        auto sobj = obj->resolve(sub.c_str(),&parent,&childName);
        if(!sobj) {
            FC_WARN("Cannot find object " << obj->getNameInDocument() << '.' << sub);
            continue;
        }
        int vis;
        if(!parent || (vis=parent->isElementVisible(childName.c_str()))<0)
            vis = sobj->Visibility.getValue()?1:0;

        if(!vis && !exportHidden)
            continue;

        TDF_Label childLabel = exportObject(obj,sub.c_str(),label,linkArray?childName.c_str():0);
        if(childLabel.IsNull())
            continue;

        if(!vis) {
            // Work around OCCT bug. If no color setting here, it will crash.
            // The culprit is at STEPCAFControl_Writer::1093 as shown below
            //
            // surfColor = Styles.EncodeColor(Quantity_Color(1,1,1,Quantity_TOC_RGB),DPDCs,ColRGBs);
            // PSA = Styles.MakeColorPSA ( item, surfColor, curvColor, isComponent );
            // if ( isComponent )
            //     setDefaultInstanceColor( override, PSA);
            //
            // Can be fixed with following
            // if ( !override.IsNull() && isComponent )
            //     setDefaultInstanceColor( override, PSA);
            //
            auto childShape = aShapeTool->GetShape(childLabel);
            Quantity_Color col;
            if(!aColorTool->GetInstanceColor(childShape,XCAFDoc_ColorGen,col) &&
               !aColorTool->GetInstanceColor(childShape,XCAFDoc_ColorSurf,col) &&
               !aColorTool->GetInstanceColor(childShape,XCAFDoc_ColorCurv,col)) 
            {
                auto &c = defaultColor;
                aColorTool->SetColor(childLabel, Quantity_Color(c.r,c.g,c.b,Quantity_TOC_RGB), XCAFDoc_ColorGen);
                FC_WARN(labelName(childLabel) << " set default color");
            }
            aColorTool->SetVisibility(childLabel,Standard_False);
        }
    }

    // Finished adding components. Now retrieve the computed non-located shape
    auto baseShape = shape;
    baseShape.setShape(aShapeTool->GetShape(label),false);

    myObjects.emplace(obj, label);
    for(auto link : links)
        myObjects.emplace(link, label);

    if(!parent.IsNull() && links.size())
        linked = links.back();
    else
        linked = obj;
    setupObject(label,linked,baseShape,prefix);

    if(!parent.IsNull()) {
        // If we are a component, swap in the base shape but keep our location.
        shape.setShape(baseShape.getShape().Located(shape.getShape().Location()),false);
        label = aShapeTool->AddComponent(parent,label,shape.getShape().Location());
        setupObject(label,name?parentObj:obj,shape,prefix,name);
    }
    return label;
}

bool ExportOCAF2::canFallback(std::vector<App::DocumentObject*> objs) {
    bool hasPartGroup = false;
    for(size_t i=0;i<objs.size();++i) {
        auto obj = objs[i];
        if(!obj || obj->getNameInDocument())
            continue;
        if(obj->getExtensionByType<App::LinkBaseExtension>(true))
            return false;
        if(!hasPartGroup && dynamic_cast<App::Part*>(obj))
            hasPartGroup = true;
        for(auto &sub : obj->getSubObjects()) 
            objs.push_back(obj->getSubObject(sub.c_str()));
    }
    return hasPartGroup;
}
