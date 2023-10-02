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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Inventor/details/SoDetail.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include "Application.h"
#include "Document.h"
#include "SoFCUnifiedSelection.h"
#include "View3DInventorSelection.h"
#include "ViewProviderDocumentObject.h"
#include <App/Document.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Base/Console.h>

FC_LOG_LEVEL_INIT("3DViewerSelection",true,true)

using namespace Gui;

View3DInventorSelection::View3DInventorSelection(SoFCUnifiedSelection* root)
    : selectionRoot(root)
{
    selectionRoot->ref();

    pcGroupOnTop = new SoSeparator;
    pcGroupOnTop->ref();
    root->addChild(pcGroupOnTop);

    auto pcGroupOnTopPickStyle = new SoPickStyle;
    pcGroupOnTopPickStyle->style = SoPickStyle::UNPICKABLE;
    pcGroupOnTopPickStyle->setOverride(true);
    pcGroupOnTop->addChild(pcGroupOnTopPickStyle);

    coin_setenv("COIN_SEPARATE_DIFFUSE_TRANSPARENCY_OVERRIDE", "1", TRUE);
    auto pcOnTopMaterial = new SoMaterial;
    pcOnTopMaterial->transparency = 0.5;
    pcOnTopMaterial->diffuseColor.setIgnored(true);
    pcOnTopMaterial->setOverride(true);
    pcGroupOnTop->addChild(pcOnTopMaterial);

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

void View3DInventorSelection::checkGroupOnTop(const SelectionChanges &Reason)
{
    if (Reason.Type == SelectionChanges::SetSelection || Reason.Type == SelectionChanges::ClrSelection) {
        clearGroupOnTop();
        if(Reason.Type == SelectionChanges::ClrSelection)
            return;
    }
    if(Reason.Type == SelectionChanges::RmvPreselect ||
       Reason.Type == SelectionChanges::RmvPreselectSignal)
    {
        SoSelectionElementAction action(SoSelectionElementAction::None,true);
        action.apply(pcGroupOnTopPreSel);
        coinRemoveAllChildren(pcGroupOnTopPreSel);
        objectsOnTopPreSel.clear();
        return;
    }
    if(!getDocument() || !Reason.pDocName || !Reason.pDocName[0] || !Reason.pObjectName)
        return;
    auto obj = getDocument()->getDocument()->getObject(Reason.pObjectName);
    if(!obj || !obj->getNameInDocument())
        return;
    std::string key(obj->getNameInDocument());
    key += '.';
    auto subname = Reason.pSubName;
    if(subname)
        key += subname;
    if(Reason.Type == SelectionChanges::RmvSelection) {
        auto &objs = objectsOnTop;
        auto pcGroup = pcGroupOnTopSel;
        auto it = objs.find(key.c_str());
        if(it == objs.end())
            return;
        int index = pcGroup->findChild(it->second);
        if(index >= 0) {
            auto node = static_cast<SoFCPathAnnotation*>(it->second);
            SoSelectionElementAction action(node->getDetail()?
                    SoSelectionElementAction::Remove:SoSelectionElementAction::None,true);
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
        }else
            FC_LOG("remove annotation object " << Reason.Type << " " << key);
        objs.erase(it);
        return;
    }

    auto &objs = Reason.Type==SelectionChanges::SetPreselect?objectsOnTopPreSel:objectsOnTop;
    auto pcGroup = Reason.Type==SelectionChanges::SetPreselect?pcGroupOnTopPreSel:pcGroupOnTopSel;

    if(objs.find(key.c_str())!=objs.end())
        return;
    auto vp = dynamic_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(obj));
    if(!vp || !vp->isSelectable() || !vp->isShow())
        return;
    auto svp = vp;
    if(subname && *subname) {
        auto sobj = obj->getSubObject(subname);
        if(!sobj || !sobj->getNameInDocument())
            return;
        if(sobj!=obj) {
            svp = dynamic_cast<ViewProviderDocumentObject*>(
                    Application::Instance->getViewProvider(sobj));
            if(!svp || !svp->isSelectable())
                return;
        }
    }
    int onTop;
    // onTop==2 means on top only if whole object is selected,
    // onTop==3 means on top only if some sub-element is selected
    // onTop==1 means either
    if(Gui::Selection().needPickedList())
        onTop = 1;
    else if(vp->OnTopWhenSelected.getValue())
        onTop = vp->OnTopWhenSelected.getValue();
    else
        onTop = svp->OnTopWhenSelected.getValue();
    if(Reason.Type == SelectionChanges::SetPreselect) {
        SoHighlightElementAction action;
        action.setHighlighted(true);
        action.setColor(selectionRoot->colorHighlight.getValue());
        action.apply(pcGroupOnTopPreSel);
        if(!onTop)
            onTop = 2;
    }else {
        if(!onTop)
            return;
        SoSelectionElementAction action(SoSelectionElementAction::All);
        action.setColor(selectionRoot->colorHighlight.getValue());
        action.apply(pcGroupOnTopSel);
    }
    if(onTop==2 || onTop==3) {
        if(subname && *subname) {
            size_t len = strlen(subname);
            if(subname[len-1]=='.') {
                // ending with '.' means whole object selection
                if(onTop == 3)
                    return;
            }else if(onTop==2)
                return;
        }else if(onTop==3)
            return;
    }

    std::vector<ViewProvider*> groups;
    auto grpVp = vp;
    std::set<ViewProvider*> visited;
    for(auto childVp=vp;;childVp=grpVp) {
        auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(childVp->getObject());
        if (!grp || !grp->getNameInDocument()) {
            break;
        }

        grpVp = dynamic_cast<ViewProviderDocumentObject*>(
                Application::Instance->getViewProvider(grp));
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
        if(idx<0 || idx>=modeSwitch->getNumChildren() ||
           modeSwitch->getChild(idx)!=childRoot)
        {
            FC_LOG("skip " << obj->getFullName() << '.' << (subname?subname:"")
                    << ", hidden inside geo group");
            return;
        }
        if(childRoot->findChild(childVp->getRoot())<0) {
            FC_LOG("cannot find '" << childVp->getObject()->getFullName()
                    << "' in geo group '" << grp->getNameInDocument() << "'");
            break;
        }
        groups.push_back(grpVp);
    }

    SoTempPath path(10);
    path.ref();

    for(auto it=groups.rbegin();it!=groups.rend();++it) {
        auto grpVp = *it;
        path.append(grpVp->getRoot());
        path.append(grpVp->getModeSwitch());
        path.append(grpVp->getChildRoot());
    }

    SoDetail *det = nullptr;
    if(vp->getDetailPath(subname, &path,true,det) && path.getLength()) {
        auto node = new SoFCPathAnnotation;
        node->setPath(&path);
        pcGroup->addChild(node);
        if(det) {
            SoSelectionElementAction action(SoSelectionElementAction::Append,true);
            action.setElement(det);
            SoTempPath tmpPath(path.getLength()+2);
            tmpPath.ref();
            tmpPath.append(pcGroup);
            tmpPath.append(node);
            tmpPath.append(&path);
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
    if(!objectsOnTop.empty() || !objectsOnTopPreSel.empty()) {
        objectsOnTop.clear();
        objectsOnTopPreSel.clear();
        SoSelectionElementAction action(SoSelectionElementAction::None,true);
        action.apply(pcGroupOnTopPreSel);
        action.apply(pcGroupOnTopSel);
        coinRemoveAllChildren(pcGroupOnTopSel);
        coinRemoveAllChildren(pcGroupOnTopPreSel);
        FC_LOG("clear annotation");
    }
}
