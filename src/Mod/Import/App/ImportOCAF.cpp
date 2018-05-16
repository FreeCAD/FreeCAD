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
# include <TDF_Tool.hxx>
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
#include "ImportOCAF.h"
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

static void setObjectName(App::DocumentObject *obj, TDF_Label label) {
    if(!obj)
        return;
    auto name = labelName(label);
    if(name.size())
        obj->Label.setValue(name.c_str());
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

ImportOCAF::ImportOCAF(Handle(TDocStd_Document) h, App::Document* d, const std::string& name)
    : pDoc(h), doc(d), default_name(name)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool (pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());

    auto hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Import/hSTEP");
    merge = hGrp->GetBool("ReadShapeCompoundMode", true);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Import");
    useLinkGroup = hGrp->GetBool("UseLinkGroup",true);
    importHidden = hGrp->GetBool("ImportHiddenObject",true);

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    defaultColor.setPackedValue(hGrp->GetUnsigned("DefaultShapeColor",0xCCCCCC00));
    defaultColor.a = 0;
}

ImportOCAF::~ImportOCAF()
{
}

static void setPlacement(App::PropertyPlacement *prop, const TopoDS_Shape &shape) {
    prop->setValue(Base::Placement(Part::TopoShape::convert(shape.Location().Transformation())));
}

bool ImportOCAF::getColor(const TopoDS_Shape &shape, App::Color &color, bool check, bool noDefault) {
    Quantity_Color aColor;
    if (aColorTool->GetColor(shape, XCAFDoc_ColorSurf, aColor) || 
        (!noDefault && aColorTool->GetColor(shape, XCAFDoc_ColorGen, aColor)))
    {
        App::Color c;
        c.r = (float)aColor.Red();
        c.g = (float)aColor.Green();
        c.b = (float)aColor.Blue();
        if(!check || color!=c) {
            color = c;
            return true;
        }
    }
    if(!check)
        color = defaultColor;
    return false;
}

bool ImportOCAF::createObject(const TopoDS_Shape &shape, Info &info) {

    auto feature = static_cast<Part::Feature*>(doc->addObject("Part::Feature","Feature"));
    feature->Visibility.setValue(false);
    feature->Shape.setValue(shape);

    App::Color color;
    info.hasColor = getColor(shape,color);

    bool haveColors = false;
    // TODO: we don't support SHUO yet, so only check for original subshape color
    Part::TopoShape tshape(shape);
    std::vector<App::Color> colors(tshape.countSubShapes(TopAbs_FACE),color);
    int i = 0;
    for(auto &face : tshape.getSubShapes(TopAbs_FACE)) {
        Quantity_Color aColor;
        if (aColorTool->GetColor(face, XCAFDoc_ColorSurf, aColor) ||
            aColorTool->GetColor(face, XCAFDoc_ColorGen, aColor)) 
        {
            color.r = (float)aColor.Red();
            color.g = (float)aColor.Green();
            color.b = (float)aColor.Blue();
            colors[i] = color;
            haveColors = true;
        }
        ++i;
    }
    if(haveColors) {
        info.hasColor = true;
        applyColors(feature,colors);
    } else
        applyColors(feature,{color});

    info.propPlacement = &feature->Placement;
    info.obj = feature;
    info.color = color;
    return true;
}

bool ImportOCAF::createGroup(Info &info, const TopoDS_Shape &shape, 
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
        if(getColor(shape,info.color,false,true))
            applyLinkColor(group,-1,info.color);
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
    getColor(shape,info.color,false,true);
    return true;
}

App::DocumentObject* ImportOCAF::loadShapes()
{
    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
        dumpLabels(pDoc->Main(),aShapeTool,aColorTool);

    myObjects.clear();
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
        applyColors(feature,{});

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

App::DocumentObject *ImportOCAF::loadShape(TDF_Label label, 
        const TopoDS_Shape &shape, bool isArrayElement) 
{
    if(shape.IsNull())
        return 0;

    auto baseShape = shape.Located(TopLoc_Location());
    auto it = myObjects.find(baseShape);
    if(it == myObjects.end()) {
        Info info;
        auto baseLabel = aShapeTool->FindShape(baseShape);
        bool res;
        if(baseLabel.IsNull() || 
           (merge && !aShapeTool->IsAssembly(baseLabel)) ||
           (!merge && shape.ShapeType()!=TopAbs_COMPOUND))
        {
            res = createObject(baseShape,info);
        } else
            res = createAssembly(baseShape,info);
        if(!res)
            return 0;
        setObjectName(info.obj,baseLabel);
        it = myObjects.emplace(baseShape,info).first;
    }

    auto &info = it->second;
    App::Color color = info.color;
    if(!isArrayElement && !getColor(shape,color,true) && info.free) {
        info.free = false;
        setPlacement(info.propPlacement,shape);
        setObjectName(info.obj,label);
        return info.obj;
    }
    auto link = static_cast<App::Link*>(doc->addObject("App::Link","Link"));
    link->Visibility.setValue(false);
    link->setLink(-1,info.obj);
    if(!isArrayElement) {
        setPlacement(&link->Placement,shape);
        setObjectName(link,label);
        if(info.color!=color)
            applyLinkColor(link,-1,color);
    }
    return link;
}

bool ImportOCAF::createAssembly(const TopoDS_Shape &shape, Info &info) {
    std::vector<TDF_Label> childLabels;
    std::vector<TopoDS_Shape> childShapes;
    boost::dynamic_bitset<> visibilities;

    for(TopoDS_Iterator it(shape,0,0);it.More();it.Next()) {
        TopoDS_Shape childShape = it.Value();
        if(childShape.IsNull())
            continue;
        TDF_Label childLabel;
        aShapeTool->Search(childShape,childLabel,Standard_True,Standard_True,Standard_False);
        if(!childLabel.IsNull() && !importHidden && !aColorTool->IsVisible(childLabel))
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
        if (aColorTool->GetColor(childShape, XCAFDoc_ColorSurf, aColor))
        {
            auto idx = placements.size()-1;
            auto &color = colors[idx];
            color.r = (float)aColor.Red();
            color.g = (float)aColor.Green();
            color.b = (float)aColor.Blue();
        }
    }

    if(placements.size()>1) {
        // Okay, we are creating a link array
        App::DocumentObject *obj = loadShape(childLabels.front(),childShapes.front(),true);
        if(!obj)
            return false;
        auto link = dynamic_cast<App::Link*>(obj);
        if(!link) {
            link = static_cast<App::Link*>(doc->addObject("App::Link","Link"));
            link->setLink(-1,obj);
        }
        link->ShowElement.setValue(false);
        link->ElementCount.setValue(placements.size());
        link->PlacementList.setValue(placements);
        link->VisibilityList.setValue(visibilities);

        for(auto &v : colors)
            applyLinkColor(link,v.first,v.second);

        info.obj = link;
        info.propPlacement = &link->Placement;
        if(getColor(shape,info.color))
            applyLinkColor(link,-1,info.color);
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
        if(aShapeTool->IsComponent(childLabel))
            visibilities.push_back(aColorTool->IsVisible(childLabel));
        else
            visibilities.push_back(true);
    }
    return createGroup(info,shape,children,visibilities);
}

// ----------------------------------------------------------------------------

ExportOCAF::ExportOCAF(Handle(TDocStd_Document) h, GetShapeColorsFunc func)
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

void ExportOCAF::setName(TDF_Label label, App::DocumentObject *obj, const char *name) {
    if(!name) {
        if(!obj)
            return;
        name = obj->Label.getValue();
    }
    TDataStd_Name::Set(label, TCollection_ExtendedString(name, 1));
}

void ExportOCAF::setupObject(TDF_Label label, App::DocumentObject *obj, 
        const Part::TopoShape &shape, const char *name) 
{
    setName(label,obj,name);
    if(!getShapeColors || aShapeTool->IsAssembly(label))
        return;

    App::Color color;
    bool isRef = aShapeTool->IsReference(label);
    auto colors = getShapeColors(shape,color,obj->getDocument(),isRef);
    if (isRef) {
        // We don't support per face color of links yet
        if(colors.size()!=1)
            return;
        color = colors.front();
    }
    Quantity_Color col(color.r,color.g,color.b,Quantity_TOC_RGB);
    aColorTool->SetColor(label, col, XCAFDoc_ColorSurf);

    if(colors.size()==1 && colors.front()==color)
        colors.clear();

    if (colors.size() == shape.countSubShapes(TopAbs_FACE)) {
        int i=0;
        for(auto &face : shape.getSubTopoShapes(TopAbs_FACE)) {
            TDF_Label faceLabel = aShapeTool->AddSubShape(label, face.getShape());
            aShapeTool->SetShape(faceLabel, face.getShape());
            const App::Color& c = colors[i++];
            Quantity_Color col(c.r,c.g,c.b,Quantity_TOC_RGB);
            aColorTool->SetColor(faceLabel, col, XCAFDoc_ColorSurf);
        }
    }
}

void ExportOCAF::exportObjects(std::vector<App::DocumentObject*> &objs, const char *name) {
    if(objs.empty())
        return;
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

TDF_Label ExportOCAF::exportObject(App::DocumentObject* parentObj, 
        const char *sub, TDF_Label parent, const char *name) 
{
    App::DocumentObject *obj;
    auto shape = Part::Feature::getTopoShape(parentObj,sub,false,0,&obj,false,false);
    if(!obj || shape.isNull()) {
        FC_WARN(obj->getNameInDocument() << " has null shape");
        return TDF_Label();
    }

    TDF_Label label;
    std::vector<App::DocumentObject *> links;

    int depth = 0;
    auto link = obj;
    auto linkedShape = shape;
    while(1) {
        auto s = Part::Feature::getTopoShape(link);
        if(s.isNull() || !s.getShape().IsPartner(shape.getShape()))
            break;
        linkedShape = s;
        auto next = link->getLinkedObject(false,0,false,depth++);
        if(!next || link==next)
            break;
        auto it = myObjects.find(next);
        if(it != myObjects.end()) {
            for(auto l : links)
                myObjects.emplace(l,it->second);
            // TODO: OCAF probably supports reference of references. But don't
            // know how to add it. Currently, we'll flaten all multi-level link
            // without scales. In other word, all link will be referring to the
            // same non-located shape
            
            // retrieve OCAF computed shape, in case the current object returns
            // a new shape every time Part::Feature::getTopoShape() is called.
            auto baseShape = aShapeTool->GetShape(it->second);
            // replace the shape without reseting element map, in order to get color
            shape.setShape(baseShape.Located(shape.getShape().Location()),false);
            label = aShapeTool->AddComponent(parent,shape.getShape(),Standard_False);
            setupObject(label,name?parentObj:obj,shape,name);
            return label;
        }
        link = next;
        links.push_back(link);
    }

    auto subs = obj->getSubObjects();
    // subs empty means obj is not a container.
    if(subs.empty()) {

        // Search for non-located shape to see if we've stored the original shape before
        if(!aShapeTool->FindShape(shape.getShape(),label)) {
            auto baseShape = linkedShape;
            auto link = links.empty()?obj:links.back();
            baseShape.setShape(baseShape.getShape().Located(TopLoc_Location()),false);
            label = aShapeTool->NewShape();
            aShapeTool->SetShape(label,baseShape.getShape());
            setupObject(label,link,baseShape);
        }

        if(!parent.IsNull()) 
            label = aShapeTool->AddComponent(parent,shape.getShape(),Standard_False);
        setupObject(label,obj,shape);

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

    myObjects.emplace(obj, label);
    for(auto link : links)
        myObjects.emplace(link, label);

    if(!parent.IsNull() && links.size())
        setName(label,links.back());
    else
        setName(label,obj);

    if(!parent.IsNull()) {
        label = aShapeTool->AddComponent(parent,label,shape.getShape().Location());
        setName(label,obj,name);
    }
    return label;
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
