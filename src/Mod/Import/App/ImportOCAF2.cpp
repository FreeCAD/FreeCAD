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
# include <TDF_AttributeSequence.hxx>
#endif

#include <XCAFDoc_ShapeMapTool.hxx>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/Part.h>
#include <App/Link.h>
#include <App/GroupExtension.h>
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
    if(aShapeTool->IsSubShape(label)) {
        auto shape = aShapeTool->GetShape(label);
        if(!shape.IsNull())
            ss << ", " << Part::TopoShape::shapeName(shape.ShapeType(),true);
    }
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
    : pDoc(h), pDocument(d), default_name(name), sequencer(0)
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
    reduceObjects = hGrp->GetBool("ReduceObjects",true);
    showProgress = hGrp->GetBool("ShowProgress",true);
    expandCompound = hGrp->GetBool("ExpandCompound",true);

    if(d->isSaved()) {
        Base::FileInfo fi(d->FileName.getValue());
        filePath = fi.dirPath();
    }
    mode = hGrp->GetInt("ImportMode",SingleDoc);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    defaultFaceColor.setPackedValue(hGrp->GetUnsigned("DefaultShapeColor",0xCCCCCC00));
    defaultFaceColor.a = 0;

    defaultEdgeColor.setPackedValue(hGrp->GetUnsigned("DefaultShapeLineColor",421075455UL));
    defaultEdgeColor.a = 0;

    if(useLinkGroup) {
        // Interface_Static::SetIVal("read.stepcaf.subshapes.name",1);
        aShapeTool->SetAutoNaming(Standard_False);
    }
}

ImportOCAF2::~ImportOCAF2()
{
}

void ImportOCAF2::setMode(int m) {
    if(m<0 || m>=ModeMax)
        FC_WARN("Invalid import mode " << m);
    else
        mode = m;
    if(mode!=SingleDoc) {
        if(pDocument->isSaved()) {
            Base::FileInfo fi(pDocument->FileName.getValue());
            filePath = fi.dirPath();
        }else
            FC_WARN("Diable multi-document mode because the input document is not saved.");
    }
}

static void setPlacement(App::PropertyPlacement *prop, const TopoDS_Shape &shape) {
    prop->setValue(Base::Placement(
                Part::TopoShape::convert(shape.Location().Transformation()))*prop->getValue());
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

App::DocumentObject *ImportOCAF2::expandShape(
        App::Document *doc, TDF_Label label, const TopoDS_Shape &shape) 
{
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
            TDF_Label childLabel;
            if(!label.IsNull())
                aShapeTool->FindSubShape(label,it.Value(),childLabel);
            auto child = expandShape(doc,childLabel,it.Value());
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
        // compound->Visibility.setValue(false);
        setPlacement(&compound->Placement,shape);
        return compound;
    }
    Info info;
    info.obj = 0;
    createObject(doc,label,shape,info,false);
    return info.obj;
}

bool ImportOCAF2::createObject(App::Document *doc, TDF_Label label, 
        const TopoDS_Shape &shape, Info &info, bool newDoc)
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
        // Two passes to get sub shape colors. First pass, look for solid, and
        // second pass look for face and edges. This allows lower level
        // subshape to override color of higher level ones.
        for(int j=0;j<2;++j) {
            for(int i=1;i<=seq.Length();++i) {
                TDF_Label l = seq.Value(i);
                TopoDS_Shape subShape = aShapeTool->GetShape(l);
                if(subShape.IsNull())
                    continue;
                if(subShape.ShapeType()==TopAbs_FACE || subShape.ShapeType()==TopAbs_EDGE) {
                    if(j==0)
                        continue;
                }else if(j!=0)
                    continue;

                bool foundFaceColor=false,foundEdgeColor=false;
                App::Color faceColor,edgeColor;
                Quantity_Color aColor;
                if(aColorTool->GetColor(l, XCAFDoc_ColorSurf, aColor) ||
                   aColorTool->GetColor(l, XCAFDoc_ColorGen, aColor))
                {
                    faceColor = App::Color(aColor.Red(),aColor.Green(),aColor.Blue());
                    foundFaceColor = true;
                }
                if(aColorTool->GetColor(l, XCAFDoc_ColorCurv, aColor)) {
                    edgeColor = App::Color(aColor.Red(),aColor.Green(),aColor.Blue());
                    foundEdgeColor = true;
                    if(j==0 && foundFaceColor && faceColors.size() && edgeColor==faceColor) {
                        // Do not set edge the same color as face
                        foundEdgeColor = false;
                    }
                }

                if(foundFaceColor) {
                    for(TopExp_Explorer exp(subShape,TopAbs_FACE);exp.More();exp.Next()) {
                        int idx = tshape.findShape(exp.Current())-1;
                        if(idx>=0 && idx<(int)faceColors.size()) {
                            faceColors[idx] = faceColor;
                            hasFaceColors = true;
                            info.hasFaceColor = true;
                        }else
                            assert(0);
                    }
                }
                if(foundEdgeColor) {
                    for(TopExp_Explorer exp(subShape,TopAbs_EDGE);exp.More();exp.Next()) {
                        int idx = tshape.findShape(exp.Current())-1;
                        if(idx>=0 && idx<(int)edgeColors.size()) {
                            edgeColors[idx] = edgeColor;
                            hasEdgeColors = true;
                            info.hasEdgeColor = true;
                        }
                    }
                }
            }
        }
    }

    Part::Feature *feature;

    if(newDoc && (mode==ObjectPerDoc || mode==ObjectPerDir))
        doc = getDocument(doc,label);

    if(expandCompound && 
       (tshape.countSubShapes(TopAbs_SOLID)>1 || 
        (!tshape.countSubShapes(TopAbs_SOLID) && tshape.countSubShapes(TopAbs_SHELL)>1)))
    {
        feature = dynamic_cast<Part::Feature*>(expandShape(doc,label,shape));
        assert(feature);
    } else {
        feature = static_cast<Part::Feature*>(doc->addObject("Part::Feature",tshape.shapeName().c_str()));
        feature->Shape.setValue(shape);
        // feature->Visibility.setValue(false);
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

App::Document *ImportOCAF2::getDocument(App::Document *doc, TDF_Label label) {
    if(filePath.empty() || mode==SingleDoc || merge)
        return doc;

    auto name = getLabelName(label);
    if(name.empty())
        return doc;

    auto newDoc = App::GetApplication().newDocument(name.c_str(),name.c_str(),false);
    std::ostringstream ss;
    Base::FileInfo fi(doc->FileName.getValue());
    std::string path = fi.dirPath();
    if(mode == GroupPerDir || mode == ObjectPerDir) {
        for(int i=0;i<1000;++i) {
            ss.str("");
            ss << path << '/' << fi.fileNamePure() << "_parts";
            if(i>0) 
                ss << '_' << std::setfill('0') << std::setw(3) << i;
            Base::FileInfo fi2(ss.str());
            if(fi2.exists()) {
                if(!fi2.isDir())
                    continue;
            }else if(!fi2.createDirectory()) {
                FC_WARN("Failed to create directory " << fi2.filePath());
                break;
            }
            path = fi2.filePath();
            break;
        }
    }
    for(int i=0;i<1000;++i) {
        ss.str("");
        ss << path << '/' << newDoc->getName() << ".fcstd";
        if(i>0) 
            ss << '_' << std::setfill('0') << std::setw(3) << i;
        Base::FileInfo fi(ss.str());
        if(!fi.exists()) {
            if(!newDoc->saveAs(fi.filePath().c_str()))
                break;
            return newDoc;
        }
    }

    FC_WARN("Cannot save document for part '" << name << "'");
    return doc;
}

bool ImportOCAF2::createGroup(App::Document *doc, Info &info, const TopoDS_Shape &shape, 
                             std::vector<App::DocumentObject*> &children, 
                             const boost::dynamic_bitset<> &visibilities,
                             bool canReduce) 
{
    assert(children.size() == visibilities.size());
    if(children.empty())
        return false;
    bool hasColor = getColor(shape,info,false,true);
    if(canReduce && !hasColor && reduceObjects && children.size()==1 && visibilities[0]) {
        info.obj = children.front();
        info.free = true;
        info.propPlacement = dynamic_cast<App::PropertyPlacement*>(info.obj->getPropertyByName("Placement"));
        myCollapsedObjects.emplace(info.obj,info.propPlacement);
        return true;
    }
    auto group = static_cast<App::LinkGroup*>(doc->addObject("App::LinkGroup","LinkGroup"));
    for(auto &child : children)  {
        if(child->getDocument()!=doc) {
            auto link = static_cast<App::Link*>(doc->addObject("App::Link","Link"));
            link->Label.setValue(child->Label.getValue());
            link->setLink(-1,child);
            auto pla = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                child->getPropertyByName("Placement"));
            if(pla)
                link->Placement.setValue(pla->getValue());
            child = link;
        }
        // child->Visibility.setValue(false);
    }
    // group->Visibility.setValue(false);
    group->ElementList.setValues(children);
    group->VisibilityList.setValue(visibilities);
    info.obj = group;
    info.propPlacement = &group->Placement;
    if(getColor(shape,info,false,true)) {
        if(info.hasFaceColor)
            applyLinkColor(group,-1,info.faceColor);
    }
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

    TDF_LabelSequence labels;
    aShapeTool->GetShapes(labels);
    Base::SequencerLauncher seq("Importing...",labels.Length());
    FC_MSG("free shape count " << labels.Length());
    sequencer = showProgress?&seq:0;

    labels.Clear();
    myShapes.clear();
    myNames.clear();
    myCollapsedObjects.clear();

    std::vector<App::DocumentObject*> objs;
    aShapeTool->GetFreeShapes (labels);
    boost::dynamic_bitset<> vis;
    int count = 0;
    for (Standard_Integer i=1; i <= labels.Length(); i++ ) {
        auto label = labels.Value(i);
        if(!importHidden && !aColorTool->IsVisible(label))
            continue;
        ++count;
    }
    for (Standard_Integer i=1; i <= labels.Length(); i++ ) {
        auto label = labels.Value(i);
        if(!importHidden && !aColorTool->IsVisible(label))
            continue;
        auto obj = loadShape(pDocument, label, 
                aShapeTool->GetShape(label), false, count>1);
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
        if(createGroup(pDocument,info,TopoDS_Shape(),objs,vis))
            ret = info.obj;
    }
    if(ret) {
        // ret->Visibility.setValue(true);
        ret->recomputeFeature(true);
    }
    if(merge && ret && !ret->isDerivedFrom(Part::Feature::getClassTypeId())) {
        auto shape = Part::Feature::getTopoShape(ret);
        auto feature = static_cast<Part::Feature*>(
                pDocument->addObject("Part::Feature", "Feature"));
        auto name = labelName(pDoc->Main());
        feature->Label.setValue(name.empty()?default_name.c_str():name.c_str());
        feature->Shape.setValue(shape);
        applyFaceColors(feature,{});

        std::vector<std::pair<App::Document*,std::string> > objNames;
        for(auto obj : App::Document::getDependencyList(objs,App::Document::DepSort))
            objNames.emplace_back(obj->getDocument(),obj->getNameInDocument());
        for(auto &v : objNames)
            v.first->removeObject(v.second.c_str());
        ret = feature;
        ret->recomputeFeature(true);
    }
    sequencer = 0;
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
                if(it->second.size())
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

App::DocumentObject *ImportOCAF2::loadShape(App::Document *doc, 
        TDF_Label label, const TopoDS_Shape &shape, bool baseOnly, bool newDoc) 
{
    if(shape.IsNull())
        return 0;

    auto baseShape = shape.Located(TopLoc_Location());
    auto it = myShapes.find(baseShape);
    if(it == myShapes.end()) {
        Info info;
        auto baseLabel = aShapeTool->FindShape(baseShape);
        if(sequencer && !baseLabel.IsNull() && aShapeTool->IsTopLevel(baseLabel))
            sequencer->next(true);
        bool res;
        if(baseLabel.IsNull() || !aShapeTool->IsAssembly(baseLabel))
            res = createObject(doc,baseLabel,baseShape,info,newDoc);
        else 
            res = createAssembly(doc,baseLabel,baseShape,info,newDoc);
        if(!res)
            return 0;
        setObjectName(info,baseLabel);
        it = myShapes.emplace(baseShape,info).first;
    }
    if(baseOnly)
        return it->second.obj;

    std::map<std::string,App::Color> shuoColors;
    if(!useLinkGroup)
        getSHUOColors(label,shuoColors,false);

    auto info = it->second;
    getColor(shape,info,true);

    if(shuoColors.empty() && info.free && doc==info.obj->getDocument()) {
        it->second.free = false;
        auto name = getLabelName(label);
        if(info.faceColor!=it->second.faceColor ||
           info.edgeColor!=it->second.edgeColor ||
           (name.size() && info.baseName.size() && name!=info.baseName)) 
        {
            auto compound = static_cast<Part::Compound2*>(doc->addObject("Part::Compound2","Compound"));
            compound->Links.setValue(info.obj);
            // compound->Visibility.setValue(false);
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
    // link->Visibility.setValue(false);
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

struct ChildInfo {
    std::vector<Base::Placement> plas;
    boost::dynamic_bitset<> vis;
    std::map<size_t,App::Color> colors;
    std::vector<TDF_Label> labels;
    TopoDS_Shape shape;
};

bool ImportOCAF2::createAssembly(App::Document *_doc, 
        TDF_Label label, const TopoDS_Shape &shape, Info &info, bool newDoc)
{
    (void)label;

    std::vector<App::DocumentObject*> children;
    std::map<App::DocumentObject*, ChildInfo> childrenMap;
    boost::dynamic_bitset<> visibilities;
    std::map<std::string,App::Color> shuoColors;

    auto doc = _doc;
    if(newDoc)
        doc = getDocument(_doc,label);

    for(TopoDS_Iterator it(shape,0,0);it.More();it.Next()) {
        TopoDS_Shape childShape = it.Value();
        if(childShape.IsNull())
            continue;
        TDF_Label childLabel;
        aShapeTool->Search(childShape,childLabel,Standard_True,Standard_True,Standard_False);
        if(!childLabel.IsNull() && !importHidden && !aColorTool->IsVisible(childLabel))
            continue;
        auto obj = loadShape(doc,childLabel,childShape,reduceObjects);
        if(!obj)
            continue;
        bool vis = true;
        if(!childLabel.IsNull() && aShapeTool->IsComponent(childLabel))
            vis = aColorTool->IsVisible(childLabel);
        if(!reduceObjects) {
            visibilities.push_back(vis);
            children.push_back(obj);
            getSHUOColors(childLabel,shuoColors,true);
            continue;
        } 

        auto &info = childrenMap[obj];
        if(info.plas.empty()) {
            children.push_back(obj);
            visibilities.push_back(vis);
            info.shape = childShape;
        }
        info.vis.push_back(vis);
        info.labels.push_back(childLabel);
        info.plas.emplace_back(Part::TopoShape::convert(childShape.Location().Transformation()));
        Quantity_Color aColor;
        if (aColorTool->GetColor(childShape, XCAFDoc_ColorSurf, aColor)) {
            auto &color = info.colors[info.plas.size()-1];
            color.r = (float)aColor.Red();
            color.g = (float)aColor.Green();
            color.b = (float)aColor.Blue();
        }
    }
    assert(visibilities.size() == children.size());

    if(children.empty()) {
        if(doc!=_doc)
            App::GetApplication().closeDocument(doc->getName());
        return false;
    }

    if(reduceObjects) {
        int i=-1;
        for(auto &child : children) {
            ++i;
            auto &info = childrenMap[child];
            if(info.plas.size()==1) {
                child = loadShape(doc,info.labels.front(),info.shape);
                getSHUOColors(info.labels.front(),shuoColors,true);
                continue;
            }

            visibilities[i] = true;

            // Okay, we are creating a link array
            auto link = static_cast<App::Link*>(doc->addObject("App::Link","Link"));
            // link->Visibility.setValue(false);
            link->setLink(-1,child);
            link->ShowElement.setValue(false);
            link->ElementCount.setValue(info.plas.size());
            auto it = myCollapsedObjects.find(child);
            if(it!=myCollapsedObjects.end()) {
                // child is a single component assembly that has been
                // collapsed, so we have to honour its placement
                for(auto &pla : info.plas)
                    pla *= it->second->getValue();
            }
            link->PlacementList.setValue(info.plas);
            link->VisibilityList.setValue(info.vis);

            for(auto &v : info.colors)
                applyLinkColor(link,v.first,v.second);

            int i=0;
            std::string name = link->getNameInDocument();
            name += '.';
            for(auto childLabel : info.labels) {
                myNames.emplace(childLabel,name + std::to_string(i++));
                getSHUOColors(childLabel,shuoColors,true);
            }

            child = link;
            Info objInfo;
            objInfo.obj = child;
            setObjectName(objInfo,info.labels.front());
        }
    }

    if(children.empty())
        return false;

    if(!createGroup(doc,info,shape,children,visibilities,shuoColors.empty()))
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
    keepPlacement = hGrp->GetBool("ExportKeepPlacement",false);

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
            if(std::isdigit((int)subname[0])) {
                auto n = std::to_string(i-1)+".";
                if(boost::starts_with(subname,n)) {
                    labels.Append(component);
                    return findComponent(subname+n.size(),component,labels);
                }
            }
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
        const Part::TopoShape &shape, const std::string &prefix, const char *name, bool force)
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

    if(!getShapeColors || (!force && !mySetups.emplace(obj,name?name:"").second))
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
                colors[""].emplace(subname,v.second);
            else {
                ++dot;
                colors[std::string(subname,dot-subname)].emplace(dot,v.second);
            }
        }
    }

    bool warned = false;

    for(auto &v : colors) {
        TDF_Label nodeLabel = label;
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
            const App::Color& c = vv.second;
            Quantity_Color color(c.r,c.g,c.b,Quantity_TOC_RGB);
            auto colorType = vv.first[0]=='F'?XCAFDoc_ColorSurf:XCAFDoc_ColorCurv;
            if(vv.first=="Face" || vv.first=="Edge") {
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
                            << obj->getFullName());
                }
                // continue;
            }

            auto subShape = shape.getSubShape(vv.first.c_str(),true);
            if(subShape.IsNull()) {
                FC_WARN("Failed to get subshape " << vv.first);
                continue;
            }

            // The following code is copied from OCCT 7.3 and is a work around
            // a bug in previous versions
            Handle(XCAFDoc_ShapeMapTool) A;
            if (!nodeLabel.FindAttribute(XCAFDoc_ShapeMapTool::GetID(), A)) {
                TopoDS_Shape aShape = aShapeTool->GetShape(nodeLabel);
                if (!aShape.IsNull()) {
                    A = XCAFDoc_ShapeMapTool::Set(nodeLabel);
                    A->SetShape(aShape);
                }
            }

            TDF_Label subLabel = aShapeTool->AddSubShape(nodeLabel, subShape);
            if(subLabel.IsNull()) {
                FC_WARN("Failed to add subshape " << vv.first);
                continue;
            }
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

#if OCC_VERSION_HEX >= 0x070200
    // Update is not performed automatically anymore: https://tracker.dev.opencascade.org/view.php?id=28055
    aShapeTool->UpdateAssemblies();
#endif
}

TDF_Label ExportOCAF2::exportObject(App::DocumentObject* parentObj, 
        const char *sub, TDF_Label parent, const char *name) 
{
    App::DocumentObject *obj;
    auto shape = Part::Feature::getTopoShape(parentObj,sub,false,0,&obj,false,!sub);
    if(!obj || shape.isNull()) {
        FC_WARN(obj->getFullName() << " has null shape");
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

        if(!parent.IsNull()) {
            // Search for non-located shape to see if we've stored the original shape before
            if(!aShapeTool->FindShape(shape.getShape(),label)) {
                auto baseShape = linkedShape;
                auto linked = links.empty()?obj:links.back();
                baseShape.setShape(baseShape.getShape().Located(TopLoc_Location()),false);
                label = aShapeTool->NewShape();
                aShapeTool->SetShape(label,baseShape.getShape());
                setupObject(label,linked,baseShape,prefix);
            }

            label = aShapeTool->AddComponent(parent,shape.getShape(),Standard_False);
            setupObject(label,name?parentObj:obj,shape,prefix,name);

        }else{
            // Here means we are exporting a single non-assembly object. We must
            // not call setupObject() on a non-located baseshape like above,
            // because OCCT does not respect shape style sharing when not
            // exporting assembly
            if(!keepPlacement) 
                shape.setShape(shape.getShape().Located(TopLoc_Location()),false);
            label = aShapeTool->AddShape(shape.getShape(),Standard_False, Standard_False);
            auto o = name?parentObj:obj;
            if(o!=linked)
                setupObject(label,linked,shape,prefix,0,true);
            setupObject(label,o,shape,prefix,name,true);
        }

        myObjects.emplace(obj, label);
        for(auto link : links)
            myObjects.emplace(link, label);
        return label;
    }

    if(obj->getExtensionByType<App::LinkBaseExtension>(true)
            || obj->getExtensionByType<App::GeoFeatureGroupExtension>(true))
        groupLinks.push_back(obj);

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
            FC_WARN("Cannot find object " << obj->getFullName() << '.' << sub);
            continue;
        }
        int vis = -1;
        if(parent) {
            if(groupLinks.size() 
                && parent->getExtensionByType<App::GroupExtension>(true,false))
            {
                vis = groupLinks.back()->isElementVisible(childName.c_str());
            }else
                vis = parent->isElementVisible(childName.c_str());
        }

        if(vis < 0)
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

    if(groupLinks.size() && groupLinks.back()==obj)
        groupLinks.pop_back();

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
    for(size_t i=0;i<objs.size();++i) {
        auto obj = objs[i];
        if(!obj || !obj->getNameInDocument())
            continue;
        if(obj->getExtensionByType<App::LinkBaseExtension>(true))
            return false;
        for(auto &sub : obj->getSubObjects()) 
            objs.push_back(obj->getSubObject(sub.c_str()));
    }
    return true;
}
