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
#define WNT  // avoid conflict with GUID
#endif
#ifndef _PreComp_
#include <Quantity_ColorRGBA.hxx>
#include <Standard_Failure.hxx>
#include <Standard_Version.hxx>
#include <TDF_AttributeSequence.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDataStd_Name.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_GraphNode.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#endif

#include <XCAFDoc_ShapeMapTool.hxx>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Link.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/Interface.h>
#include <Mod/Part/App/OCAF/ImportExportSettings.h>

#include "ExportOCAF2.h"


FC_LOG_LEVEL_INIT("Import", true, true)

using namespace Import;

ExportOCAFOptions::ExportOCAFOptions()
{
    defaultColor.setPackedValue(0xCCCCCC00);
    defaultColor.a = 0;
}

ExportOCAF2::ExportOCAF2(Handle(TDocStd_Document) h, GetShapeColorsFunc func)
    : pDoc(h)
    , getShapeColors(func)
{
    aShapeTool = XCAFDoc_DocumentTool::ShapeTool(pDoc->Main());
    aColorTool = XCAFDoc_DocumentTool::ColorTool(pDoc->Main());

    Part::Interface::writeStepAssembly(Part::Interface::Assembly::Auto);
}

// ----------------------------------------------------------------------------

/*!
 * \brief ExportOCAF2::customExportOptions
 * \return options from user settings
 */
ExportOCAFOptions ExportOCAF2::customExportOptions()
{
    Part::OCAF::ImportExportSettings settings;

    ExportOCAFOptions defaultOptions;
    defaultOptions.exportHidden = settings.getExportHiddenObject();
    defaultOptions.keepPlacement = settings.getExportKeepPlacement();

    auto handle =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    defaultOptions.defaultColor.setPackedValue(
        handle->GetUnsigned("DefaultShapeColor", defaultOptions.defaultColor.getPackedValue()));
    defaultOptions.defaultColor.a = 0;

    return defaultOptions;
}

void ExportOCAF2::setName(TDF_Label label, App::DocumentObject* obj, const char* name)
{
    if (!name) {
        if (!obj) {
            return;
        }
        name = obj->Label.getValue();
    }
    TDataStd_Name::Set(label, TCollection_ExtendedString(name, true));
}

// Similar to XCAFDoc_ShapeTool::FindSHUO but return only main SHUO, i.e. SHUO
// with no upper_usage. It should not be necessary if we strictly export from
// bottom up, but let's make sure of it.
static Standard_Boolean FindSHUO(const TDF_LabelSequence& theLabels,
                                 Handle(XCAFDoc_GraphNode) & theSHUOAttr)
{
    assert(theLabels.Length() > 1);
    theSHUOAttr.Nullify();
    TDF_AttributeSequence SHUOAttrs;
    TDF_Label aCompLabel = theLabels.Value(1);
    if (!::XCAFDoc_ShapeTool::GetAllComponentSHUO(aCompLabel, SHUOAttrs)) {
        return Standard_False;
    }
    for (Standard_Integer i = 1; i <= SHUOAttrs.Length(); i++) {
        Handle(XCAFDoc_GraphNode) anSHUO = Handle(XCAFDoc_GraphNode)::DownCast(SHUOAttrs.Value(i));
        TDF_LabelSequence aUpLabels;
        // check for any upper_usage
        ::XCAFDoc_ShapeTool::GetSHUOUpperUsage(anSHUO->Label(), aUpLabels);
        if (aUpLabels.Length() > 0) {
            continue;  // reject if there is one
        }
        int j = 2;
        for (; anSHUO->NbChildren(); ++j) {
            if (j > theLabels.Length()) {
                j = 0;
                break;
            }
            anSHUO = anSHUO->GetChild(1);
            if (theLabels.Value(j) != anSHUO->Label().Father()) {
                j = 0;
                break;
            }
        }
        if (j != theLabels.Length() + 1) {
            continue;
        }

        theSHUOAttr = Handle(XCAFDoc_GraphNode)::DownCast(SHUOAttrs.Value(i));
        break;
    }
    return (!theSHUOAttr.IsNull());
}

TDF_Label
ExportOCAF2::findComponent(const char* subname, TDF_Label label, TDF_LabelSequence& labels)
{
    const char* dot = strchr(subname, '.');
    if (!dot) {
        if (labels.Length() == 1) {
            return labels.Value(1);
        }
        Handle(XCAFDoc_GraphNode) ret;
        if (labels.Length() && (FindSHUO(labels, ret) || aShapeTool->SetSHUO(labels, ret))) {
            return ret->Label();
        }
        return {};
    }
    TDF_LabelSequence components;
    TDF_Label ref;
    if (!aShapeTool->GetReferredShape(label, ref)) {
        ref = label;
    }
    if (aShapeTool->GetComponents(ref, components)) {
        for (int i = 1; i <= components.Length(); ++i) {
            auto component = components.Value(i);
            if (std::isdigit((int)subname[0])) {
                auto n = std::to_string(i - 1) + ".";
                if (boost::starts_with(subname, n)) {
                    labels.Append(component);
                    return findComponent(subname + n.size(), component, labels);
                }
            }
            auto it = myNames.find(component);
            if (it == myNames.end()) {
                continue;
            }
            for (auto& n : it->second) {
                if (boost::starts_with(subname, n)) {
                    labels.Append(component);
                    return findComponent(subname + n.size(), component, labels);
                }
            }
        }
    }
    return {};
}

void ExportOCAF2::setupObject(TDF_Label label,
                              App::DocumentObject* obj,
                              const Part::TopoShape& shape,
                              const std::string& prefix,
                              const char* name,
                              bool force)
{
    setName(label, obj, name);
    if (aShapeTool->IsComponent(label)) {
        auto& names = myNames[label];
        // The subname reference may contain several possible namings.
        if (!name) {
            // simple object internal name
            names.push_back(prefix + obj->getNameInDocument() + ".");
        }
        else {
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
        // with preceding '$'
        names.push_back(prefix + "$" + obj->Label.getValue() + ".");
    }

    if (!getShapeColors || (!force && !mySetups.emplace(obj, name ? name : "").second)) {
        return;
    }

    std::map<std::string, std::map<std::string, App::Color>> colors;
    static std::string marker(App::DocumentObject::hiddenMarker() + "*");
    static std::array<const char*, 3> keys = {"Face*", "Edge*", marker.c_str()};
    std::string childName;
    if (name) {
        childName = name;
        childName += '.';
    }
    for (auto key : keys) {
        for (auto& v : getShapeColors(obj, key)) {
            const char* subname = v.first.c_str();
            if (name) {
                if (!boost::starts_with(v.first, childName)) {
                    continue;
                }
                subname += childName.size();
            }
            const char* dot = strrchr(subname, '.');
            if (!dot) {
                colors[""].emplace(subname, v.second);
            }
            else {
                ++dot;
                colors[std::string(subname, dot - subname)].emplace(dot, v.second);
            }
        }
    }

    bool warned = false;

    for (auto& v : colors) {
        TDF_Label nodeLabel = label;
        if (!v.first.empty()) {
            TDF_LabelSequence labels;
            if (aShapeTool->IsComponent(label)) {
                labels.Append(label);
            }
            nodeLabel = findComponent(v.first.c_str(), label, labels);
            if (nodeLabel.IsNull()) {
                FC_WARN("Failed to find component " << v.first);
                continue;
            }
        }
        for (auto& vv : v.second) {
            if (vv.first == App::DocumentObject::hiddenMarker()) {
                aColorTool->SetVisibility(nodeLabel, Standard_False);
                continue;
            }
            const App::Color& c = vv.second;
            Quantity_ColorRGBA color = Tools::convertColor(c);
            auto colorType = vv.first[0] == 'F' ? XCAFDoc_ColorSurf : XCAFDoc_ColorCurv;
            if (vv.first == "Face" || vv.first == "Edge") {
                aColorTool->SetColor(nodeLabel, color, colorType);
                continue;
            }

            if (nodeLabel != label || aShapeTool->IsComponent(label)) {
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
                if (!warned) {
                    warned = true;
                    FC_WARN("Current OCCT does not support element color override, for object "
                            << obj->getFullName());
                }
                // continue;
            }

            auto subShape = shape.getSubShape(vv.first.c_str(), true);
            if (subShape.IsNull()) {
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
            if (subLabel.IsNull()) {
                FC_WARN("Failed to add subshape " << vv.first);
                continue;
            }
            aColorTool->SetColor(subLabel, color, colorType);
        }
    }
}

void ExportOCAF2::exportObjects(std::vector<App::DocumentObject*>& objs, const char* name)
{
    if (objs.empty()) {
        return;
    }
    myObjects.clear();
    myNames.clear();
    mySetups.clear();
    if (objs.size() == 1) {
        exportObject(objs.front(), nullptr, TDF_Label());
    }
    else {
        auto label = aShapeTool->NewShape();
        App::Document* doc = nullptr;
        bool sameDoc = true;
        for (auto obj : objs) {
            if (doc) {
                sameDoc = sameDoc && doc == obj->getDocument();
            }
            else {
                doc = obj->getDocument();
            }
            exportObject(obj, nullptr, label);
        }

        if (!name && doc && sameDoc) {
            name = doc->getName();
        }
        setName(label, nullptr, name);
    }

    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        Tools::dumpLabels(pDoc->Main(), aShapeTool, aColorTool);
    }

    // Update is not performed automatically anymore:
    // https://tracker.dev.opencascade.org/view.php?id=28055
    aShapeTool->UpdateAssemblies();
}

TDF_Label ExportOCAF2::exportObject(App::DocumentObject* parentObj,
                                    const char* sub,
                                    TDF_Label parent,
                                    const char* name)
{
    App::DocumentObject* obj;
    auto shape = Part::Feature::getTopoShape(parentObj, sub, false, nullptr, &obj, false, !sub);
    if (!obj || shape.isNull()) {
        if (obj) {
            FC_WARN(obj->getFullName() << " has null shape");
        }
        return {};
    }

    // sub may contain more than one hierarchy, e.g. Assembly container may use
    // getSubObjects to skip some hierarchy containing constraints and stuff
    // when exporting. We search for extra '.', and set it as prefix if found.
    // When setting SHUO's, we'll need this prefix for matching.
    std::string prefix;
    if (sub) {
        auto len = strlen(sub);
        if (len > 1) {
            --len;
            // The prefix ends with the second last '.', so search for it.
            for (int i = 0; len != 0; --len) {
                if (sub[len] == '.' && ++i == 2) {
                    prefix = std::string(sub, len + 1);
                    break;
                }
            }
        }
    }

    TDF_Label label;
    std::vector<App::DocumentObject*> links;

    int depth = 0;
    auto linked = obj;
    auto linkedShape = shape;
    while (true) {
        auto s = Part::Feature::getTopoShape(linked);
        if (s.isNull() || !s.getShape().IsPartner(shape.getShape())) {
            break;
        }
        linkedShape = s;
        // Search using our own cache. We can't rely on ShapeTool::FindShape()
        // in case this is an assembly. Because FindShape() search among its
        // own computed shape, i.e. its own created compound, and thus will
        // never match ours.
        auto it = myObjects.find(linked);
        if (it != myObjects.end()) {
            for (auto l : links) {
                myObjects.emplace(l, it->second);
            }
            // Note: OCAF does not seem to support reference of references. We
            // have to flaten all multi-level link without scales. In other
            // word, all link will all be forced to refer to the same
            // non-located shape

            // retrieve OCAF computed shape, in case the current object returns
            // a new shape every time Part::Feature::getTopoShape() is called.
            auto baseShape = aShapeTool->GetShape(it->second);
            shape.setShape(baseShape.Located(shape.getShape().Location()));
            if (!parent.IsNull()) {
                label = aShapeTool->AddComponent(parent, shape.getShape(), Standard_False);
            }
            else {
                label = aShapeTool->AddShape(shape.getShape(), Standard_False, Standard_False);
            }
            setupObject(label, name ? parentObj : obj, shape, prefix, name);
            return label;
        }
        auto next = linked->getLinkedObject(false, nullptr, false, depth++);
        if (!next || linked == next) {
            break;
        }
        linked = next;
        links.push_back(linked);
    }

    auto subs = obj->getSubObjects();
    // subs empty means obj is not a container.
    if (subs.empty()) {

        if (!parent.IsNull()) {
            // Search for non-located shape to see if we've stored the original shape before
            if (!aShapeTool->FindShape(shape.getShape(), label)) {
                auto baseShape = linkedShape;
                auto linked = links.empty() ? obj : links.back();
                baseShape.setShape(baseShape.getShape().Located(TopLoc_Location()));
                label = aShapeTool->NewShape();
                aShapeTool->SetShape(label, baseShape.getShape());
                setupObject(label, linked, baseShape, prefix);
            }

            label = aShapeTool->AddComponent(parent, shape.getShape(), Standard_False);
            setupObject(label, name ? parentObj : obj, shape, prefix, name);
        }
        else {
            // Here means we are exporting a single non-assembly object. We must
            // not call setupObject() on a non-located baseshape like above,
            // because OCCT does not respect shape style sharing when not
            // exporting assembly
            if (!options.keepPlacement || shape.getPlacement() == Base::Placement()) {
                shape.setShape(shape.getShape().Located(TopLoc_Location()));
            }
            else {
                Base::Matrix4D mat = shape.getTransform();
                shape.setShape(shape.getShape().Located(TopLoc_Location()));
                // Transform with copy to conceal the transformation
                shape.transformShape(mat, true);
                // Even if the shape has no transformation, TopoShape still sets
                // a TopLoc_Location, so we need to clear it again.
                shape.setShape(shape.getShape().Located(TopLoc_Location()));
            }
            label = aShapeTool->AddShape(shape.getShape(), Standard_False, Standard_False);
            auto o = name ? parentObj : obj;
            if (o != linked) {
                setupObject(label, linked, shape, prefix, nullptr, true);
            }
            setupObject(label, o, shape, prefix, name, true);
        }

        myObjects.emplace(obj, label);
        for (auto link : links) {
            myObjects.emplace(link, label);
        }
        return label;
    }

    if (obj->getExtensionByType<App::LinkBaseExtension>(true)
        || obj->getExtensionByType<App::GeoFeatureGroupExtension>(true)) {
        groupLinks.push_back(obj);
    }

    // Create a new assembly
    label = aShapeTool->NewShape();

    // check for link array
    auto linkArray = obj->getLinkedObject(true)->getExtensionByType<App::LinkBaseExtension>(true);
    if (linkArray && (linkArray->getShowElementValue() || !linkArray->getElementCountValue())) {
        linkArray = nullptr;
    }
    for (auto& subobj : subs) {
        App::DocumentObject* parentGrp = nullptr;
        std::string childName;
        auto sobj = obj->resolve(subobj.c_str(), &parentGrp, &childName);
        if (!sobj) {
            FC_WARN("Cannot find object " << obj->getFullName() << '.' << subobj);
            continue;
        }
        int vis = -1;
        if (parentGrp) {
            if (!groupLinks.empty()
                && parentGrp->getExtensionByType<App::GroupExtension>(true, false)) {
                vis = groupLinks.back()->isElementVisible(childName.c_str());
            }
            else {
                vis = parentGrp->isElementVisible(childName.c_str());
            }
        }

        if (vis < 0) {
            vis = sobj->Visibility.getValue() ? 1 : 0;
        }

        if (!vis && !options.exportHidden) {
            continue;
        }

        TDF_Label childLabel =
            exportObject(obj, subobj.c_str(), label, linkArray ? childName.c_str() : nullptr);
        if (childLabel.IsNull()) {
            continue;
        }

        if (!vis) {
            // Work around OCCT bug. If no color setting here, it will crash.
            // The culprit is at STEPCAFControl_Writer::1093 as shown below
            //
            // surfColor = Styles.EncodeColor(Quantity_Color(1,1,1,OCC_COLOR_SPACE),DPDCs,ColRGBs);
            // PSA = Styles.MakeColorPSA ( item, surfColor, curvColor, isComponent );
            // if ( isComponent )
            //     setDefaultInstanceColor( override, PSA);
            //
            // Can be fixed with following
            // if ( !override.IsNull() && isComponent )
            //     setDefaultInstanceColor( override, PSA);
            //
            auto childShape = aShapeTool->GetShape(childLabel);
            Quantity_ColorRGBA col;
            if (!aColorTool->GetInstanceColor(childShape, XCAFDoc_ColorGen, col)
                && !aColorTool->GetInstanceColor(childShape, XCAFDoc_ColorSurf, col)
                && !aColorTool->GetInstanceColor(childShape, XCAFDoc_ColorCurv, col)) {
                auto& c = options.defaultColor;
                aColorTool->SetColor(childLabel, Tools::convertColor(c), XCAFDoc_ColorGen);
                FC_WARN(Tools::labelName(childLabel) << " set default color");
            }
            aColorTool->SetVisibility(childLabel, Standard_False);
        }
    }

    if (!groupLinks.empty() && groupLinks.back() == obj) {
        groupLinks.pop_back();
    }

    // Finished adding components. Now retrieve the computed non-located shape
    auto baseShape = shape;
    baseShape.setShape(aShapeTool->GetShape(label));

    myObjects.emplace(obj, label);
    for (auto link : links) {
        myObjects.emplace(link, label);
    }

    if (!parent.IsNull() && !links.empty()) {
        linked = links.back();
    }
    else {
        linked = obj;
    }
    setupObject(label, linked, baseShape, prefix);

    if (!parent.IsNull()) {
        // If we are a component, swap in the base shape but keep our location.
        shape.setShape(baseShape.getShape().Located(shape.getShape().Location()));
        label = aShapeTool->AddComponent(parent, label, shape.getShape().Location());
        setupObject(label, name ? parentObj : obj, shape, prefix, name);
    }
    return label;
}

bool ExportOCAF2::canFallback(std::vector<App::DocumentObject*> objs)
{
    for (size_t i = 0; i < objs.size(); ++i) {
        auto obj = objs[i];
        if (!obj || !obj->getNameInDocument()) {
            continue;
        }
        if (obj->getExtensionByType<App::LinkBaseExtension>(true)) {
            return false;
        }
        for (auto& sub : obj->getSubObjects()) {
            objs.push_back(obj->getSubObject(sub.c_str()));
        }
    }
    return true;
}
