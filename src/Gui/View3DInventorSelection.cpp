/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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


#include <Inventor/details/SoDetail.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>


#include "Application.h"
#include "Document.h"
#include "Inventor/SoFCSwitch.h"
#include "SoFCUnifiedSelection.h"
#include "TreeParams.h"
#include "View3DInventorSelection.h"
#include "ViewProviderDocumentObject.h"
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Base/Console.h>

FC_LOG_LEVEL_INIT("3DViewerSelection", true, true)

using namespace Gui;

View3DInventorSelection::View3DInventorSelection(SoFCUnifiedSelection* root)
    : selectionRoot(root)
{
    selectionRoot->ref();

    pcGroupOnTop = new SoSeparator;
    pcGroupOnTop->ref();
    pcGroupOnTop->setName("GroupOnTop");
    root->addChild(pcGroupOnTop);

    auto pcGroupOnTopPickStyle = new SoPickStyle;
    pcGroupOnTopPickStyle->style = SoPickStyle::UNPICKABLE;
    pcGroupOnTopPickStyle->setOverride(true);
    pcGroupOnTopPickStyle->setName("GroupOnTopPickStyle");
    pcGroupOnTop->addChild(pcGroupOnTopPickStyle);

    coin_setenv("COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE", "1", TRUE);
    auto pcGroupOnTopMaterial = new SoMaterial;
    pcGroupOnTopMaterial->transparency = 0.5;
    pcGroupOnTopMaterial->diffuseColor.setIgnored(true);
    pcGroupOnTopMaterial->setOverride(true);
    pcGroupOnTopMaterial->setName("GroupOnTopMaterial");
    pcGroupOnTop->addChild(pcGroupOnTopMaterial);

    // Depth off so a previewed hidden object doesn't z-fight the visible result
    // in the transparency pass; the raw glDisable(GL_DEPTH_TEST) in
    // SoFCPathAnnotation does not survive that pass. Must live on the parent
    // group: the Sel/PreSel children are wiped by coinRemoveAllChildren() on
    // every selection change. Both fields start ignored so ordinary selection
    // and preselection keep the depth state they had before this existed; only
    // a hidden preview turns the override on.
    pcGroupOnTopDepth = new SoDepthBuffer;
    pcGroupOnTopDepth->test = FALSE;
    pcGroupOnTopDepth->write = FALSE;
    pcGroupOnTopDepth->test.setIgnored(true);
    pcGroupOnTopDepth->write.setIgnored(true);
    pcGroupOnTopDepth->setName("GroupOnTopDepthBuffer");
    pcGroupOnTop->addChild(pcGroupOnTopDepth);

    {
        auto selRoot = new SoFCSelectionRoot;
        selRoot->selectionStyle = SoFCSelectionRoot::PassThrough;
        pcGroupOnTopSel = selRoot;
        pcGroupOnTopSel->setName("GroupOnTopSel");
        pcGroupOnTopSel->ref();
        pcGroupOnTop->addChild(pcGroupOnTopSel);
    }

    {
        auto selRoot = new SoFCSelectionRoot;
        selRoot->selectionStyle = SoFCSelectionRoot::PassThrough;
        pcGroupOnTopPreSel = selRoot;
        pcGroupOnTopPreSel->setName("GroupOnTopPreSel");
        pcGroupOnTopPreSel->ref();
        pcGroupOnTop->addChild(pcGroupOnTopPreSel);
    }
}

View3DInventorSelection::~View3DInventorSelection()
{
    selectionRoot->unref();
    pcGroupOnTop->unref();
    pcGroupOnTopPreSel->unref();
    pcGroupOnTopSel->unref();
}

void View3DInventorSelection::setHiddenPreviewDepthOverride(DepthOverride state)
{
    // An ignored field makes SoDepthBuffer read the value already on the state,
    // which is exactly the behavior of not having the node here at all.
    const SbBool ignored = state == DepthOverride::Off;
    pcGroupOnTopDepth->test.setIgnored(ignored);
    pcGroupOnTopDepth->write.setIgnored(ignored);
}

void View3DInventorSelection::clearFeaturePreview()
{
    if (previewedFeature) {
        previewedFeature->showPreselectPreview(false);
        previewedFeature = nullptr;
    }
}

void View3DInventorSelection::checkGroupOnTop(const SelectionChanges& Reason)
{
    featurePreviewActive = false;
    if (Reason.Type == SelectionChanges::SetSelection
        || Reason.Type == SelectionChanges::ClrSelection) {
        clearGroupOnTop();
        if (Reason.Type == SelectionChanges::ClrSelection) {
            return;
        }
    }
    if (Reason.Type == SelectionChanges::RmvPreselect
        || Reason.Type == SelectionChanges::RmvPreselectSignal) {
        SoSelectionElementAction action(SoSelectionElementAction::None, true);
        action.apply(pcGroupOnTopPreSel);
        coinRemoveAllChildren(pcGroupOnTopPreSel);
        objectsOnTopPreSel.clear();
        setHiddenPreviewDepthOverride(DepthOverride::Off);
        clearFeaturePreview();
        return;
    }
    if (!getDocument() || !Reason.pDocName || !Reason.pDocName[0] || !Reason.pObjectName) {
        return;
    }
    auto obj = getDocument()->getDocument()->getObject(Reason.pObjectName);
    if (!obj || !obj->isAttachedToDocument()) {
        return;
    }
    std::string key(obj->getNameInDocument());
    key += '.';
    auto subname = Reason.pSubName;
    App::ElementNamePair element;
    App::GeoFeature::resolveElement(obj, Reason.pSubName, element);
    if (Data::isMappedElement(subname) && !element.oldName.empty()) {  // If we have a shortened
                                                                       // element name
        subname = element.oldName.c_str();                             // use if
    }
    if (subname) {
        key += subname;
    }
    if (Reason.Type == SelectionChanges::RmvSelection) {
        auto& objs = objectsOnTop;
        auto pcGroup = pcGroupOnTopSel;
        auto it = objs.find(key.c_str());
        if (it == objs.end()) {
            return;
        }
        int index = pcGroup->findChild(it->second);
        if (index >= 0) {
            auto node = static_cast<SoFCPathAnnotation*>(it->second);
            SoSelectionElementAction action(
                node->getDetail() ? SoSelectionElementAction::Remove : SoSelectionElementAction::None,
                true
            );
            auto path = node->getPath();
            SoTempPath tmpPath(2 + (path ? path->getLength() : 0));
            tmpPath.ref();
            tmpPath.append(pcGroup);
            tmpPath.append(node);
            tmpPath.append(node->getPath());
            action.setElement(node->getDetail());
            action.apply(&tmpPath);
            tmpPath.unrefNoDelete();
            pcGroup->removeChild(index);
            FC_LOG("remove annotation " << Reason.Type << " " << key);
        }
        else {
            FC_LOG("remove annotation object " << Reason.Type << " " << key);
        }
        objs.erase(it);
        return;
    }

    const bool isPreselect = Reason.Type == SelectionChanges::SetPreselect;
    // gated behind a preference so the hidden-object handling can be disabled
    const bool previewHidden = isPreselect && TreeParams::getPreSelectHidden();
    auto& objs = isPreselect ? objectsOnTopPreSel : objectsOnTop;
    auto pcGroup = isPreselect ? pcGroupOnTopPreSel : pcGroupOnTopSel;

    if (objs.find(key.c_str()) != objs.end()) {
        return;
    }
    auto vp = freecad_cast<ViewProviderDocumentObject*>(Application::Instance->getViewProvider(obj));
    // preselection may target hidden objects, which are rendered on top instead
    if (!vp || !vp->isSelectable() || (!previewHidden && !vp->isShow())) {
        return;
    }
    auto svp = vp;
    if (subname && *subname) {
        auto sobj = obj->getSubObject(subname);
        if (!sobj || !sobj->isAttachedToDocument()) {
            return;
        }
        if (sobj != obj) {
            svp = freecad_cast<ViewProviderDocumentObject*>(
                Application::Instance->getViewProvider(sobj)
            );
            if (!svp || !svp->isSelectable()) {
                return;
            }
        }
    }
    // with preview off, a hidden target (e.g. a sub-object of a shown parent)
    // must not be forced on top; matches upstream where it renders nothing
    if (!previewHidden && !svp->isShow()) {
        return;
    }
    // a hidden object previews as a whole; the inner items of a hidden container
    // do not, as they would float without the container around them
    if (previewHidden && svp != vp && !vp->isShow()) {
        return;
    }
    if (previewHidden) {
        // let a PartDesign feature drive its own preview instead of the generic copy
        if (previewedFeature && previewedFeature != svp) {
            clearFeaturePreview();
        }
        if (svp->showPreselectPreview(true)) {
            previewedFeature = svp;
            featurePreviewActive = true;
            return;
        }
        // Some view providers build geometry lazily and skip it while hidden;
        // the true/false pair forces a one-time rebuild (no-op without it).
        auto rebuildIfHidden = [](ViewProviderDocumentObject* provider) {
            if (!provider->isShow()) {
                provider->forceUpdate(true);
                provider->forceUpdate(false);
            }
        };
        rebuildIfHidden(vp);
        if (svp != vp) {
            rebuildIfHidden(svp);
        }
    }
    int onTop;
    // onTop==2 means on top only if whole object is selected,
    // onTop==3 means on top only if some sub-element is selected
    // onTop==1 means either
    if (vp->OnTopWhenSelected.getValue()) {
        onTop = vp->OnTopWhenSelected.getValue();
    }
    else {
        onTop = svp->OnTopWhenSelected.getValue();
    }
    if (isPreselect) {
        SoHighlightElementAction action;
        action.setHighlighted(true);
        action.setColor(selectionRoot->colorHighlight.getValue());
        action.apply(pcGroupOnTopPreSel);
        if (!onTop) {
            onTop = 2;
        }
    }
    else {
        if (!onTop) {
            return;
        }
        SoSelectionElementAction action(SoSelectionElementAction::All);
        action.setColor(selectionRoot->colorSelection.getValue());
        action.apply(pcGroupOnTopSel);
    }
    if (onTop == 2 || onTop == 3) {
        if (subname && *subname) {
            size_t len = strlen(subname);
            if (subname[len - 1] == '.') {
                // ending with '.' means whole object selection
                if (onTop == 3) {
                    return;
                }
            }
            else if (onTop == 2) {
                return;
            }
        }
        else if (onTop == 3) {
            return;
        }
    }

    std::vector<ViewProvider*> groups;
    auto grpVp = vp;
    std::set<ViewProvider*> visited;
    for (auto childVp = vp;; childVp = grpVp) {
        auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(childVp->getObject());
        if (!grp || !grp->isAttachedToDocument()) {
            break;
        }

        grpVp = freecad_cast<ViewProviderDocumentObject*>(Application::Instance->getViewProvider(grp));
        if (!grpVp) {
            break;
        }

        // avoid endless-loops
        if (!visited.insert(childVp).second) {
            break;
        }

        auto childRoot = grpVp->getChildRoot();
        auto modeSwitch = grpVp->getModeSwitch();
        auto idx = modeSwitch->whichChild.getValue();
        if (idx < 0 || idx >= modeSwitch->getNumChildren() || modeSwitch->getChild(idx) != childRoot) {
            if (!previewHidden || !childRoot) {
                FC_LOG(
                    "skip " << obj->getFullName() << '.' << (subname ? subname : "")
                            << ", hidden inside geo group"
                );
                return;
            }
            // preselect: keep the path through the hidden group; the override draws it
        }
        if (childRoot->findChild(childVp->getRoot()) < 0) {
            FC_LOG(
                "cannot find '" << childVp->getObject()->getFullName() << "' in geo group '"
                                << grp->getNameInDocument() << "'"
            );
            break;
        }
        groups.push_back(grpVp);
    }

    SoTempPath path(10);
    path.ref();

    for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
        auto grpVp = *it;
        path.append(grpVp->getRoot());
        path.append(grpVp->getModeSwitch());
        path.append(grpVp->getChildRoot());
    }

    SoDetail* det = nullptr;
    if (vp->getDetailPath(subname, &path, true, det) && path.getLength()) {
        auto node = new SoFCPathAnnotation;
        node->setPath(&path);
        // only a tree preselect may draw an object whose mode switch is off
        node->setOverrideHidden(previewHidden);
        pcGroup->addChild(node);
        if (previewHidden && (!vp->isShow() || !svp->isShow())) {
            setHiddenPreviewDepthOverride(DepthOverride::On);
        }
        if (det) {
            SoSelectionElementAction action(SoSelectionElementAction::Append, true);
            action.setElement(det);
            SoTempPath tmpPath(path.getLength() + 2);
            tmpPath.ref();
            tmpPath.append(pcGroup);
            tmpPath.append(node);
            tmpPath.append(&path);
            // reach the element even when the object is hidden
            SoFCSwitch::OverrideScope switchOverride(previewHidden ? &path : nullptr);
            action.apply(&tmpPath);
            tmpPath.unrefNoDelete();
            node->setDetail(det);
            det = nullptr;
        }
        FC_LOG("add annotation " << Reason.Type << " " << key);
        objs[key.c_str()] = node;
    }
    delete det;
    path.unrefNoDelete();
}

void View3DInventorSelection::clearGroupOnTop()
{
    clearFeaturePreview();
    if (!objectsOnTop.empty() || !objectsOnTopPreSel.empty()) {
        objectsOnTop.clear();
        objectsOnTopPreSel.clear();
        SoSelectionElementAction action(SoSelectionElementAction::None, true);
        action.apply(pcGroupOnTopPreSel);
        action.apply(pcGroupOnTopSel);
        coinRemoveAllChildren(pcGroupOnTopSel);
        coinRemoveAllChildren(pcGroupOnTopPreSel);
        setHiddenPreviewDepthOverride(DepthOverride::Off);
        FC_LOG("clear annotation");
    }
}
