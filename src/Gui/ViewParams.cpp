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
#include <App/Application.h>
#include "ViewProvider.h"
#include "ViewParams.h"
#include "Selection.h"
#include "OverlayWidgets.h"

using namespace Gui;

ViewParams::ViewParams() {
    handle = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
    handle->Attach(this);
#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def,_doc) \
    _name = handle->Get##_type(#_name,_def);\
    funcs[#_name] = &ViewParams::update##_name;

#undef FC_VIEW_PARAM2
#define FC_VIEW_PARAM2 FC_VIEW_PARAM
    FC_VIEW_PARAMS
}

ViewParams::~ViewParams() {
}

#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def,_doc) \
const char *ViewParams::doc##_name() { return _doc; }

FC_VIEW_PARAMS

void ViewParams::OnChange(Base::Subject<const char*> &, const char* sReason) {
    if(!sReason)
        return;
    auto it = funcs.find(sReason);
    if(it == funcs.end())
        return;
    it->second(this);
}

ViewParams *ViewParams::instance() {
    static ViewParams *inst;
    if(!inst)
        inst = new ViewParams;
    return inst;
}

void ViewParams::onShowSelectionOnTopChanged() {
    Selection().clearCompleteSelection();
    if(getMapChildrenPlacement())
        setMapChildrenPlacement(false);
}

void ViewParams::onMapChildrenPlacementChanged() {
    ViewProvider::clearBoundingBoxCache();
    if(!getShowSelectionOnTop())
        setShowSelectionOnTop(true);
}

void ViewParams::onDockOverlayAutoViewChanged() {
    OverlayManager::instance()->refresh();
}

void ViewParams::onDockOverlayExtraStateChanged() {
    OverlayManager::instance()->refresh(nullptr, true);
}

void ViewParams::onCornerNaviCubeChanged() {
    OverlayManager::instance()->refresh();
}

void ViewParams::onDockOverlayCheckNaviCubeChanged() {
    OverlayManager::instance()->refresh();
}

void ViewParams::onDockOverlayHideTabBarChanged() {
    OverlayManager::instance()->refresh(nullptr, true);
}

