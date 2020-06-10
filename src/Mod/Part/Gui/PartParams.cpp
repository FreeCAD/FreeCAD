/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include "ViewProvider.h"
#include "PartParams.h"

using namespace PartGui;

PartParams::PartParams() {
    handle = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Part");
    handle->Attach(this);
#undef FC_PART_PARAM
#define FC_PART_PARAM(_name,_ctype,_type,_def) \
    _##_name = handle->Get##_type(#_name,_def);\
    funcs[#_name] = &PartParams::update##_name;

#undef FC_PART_PARAM2
#define FC_PART_PARAM2 FC_PART_PARAM
    FC_PART_PARAMS

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(updateVisual()));
}

PartParams::~PartParams() {
}

void PartParams::OnChange(Base::Subject<const char*> &, const char* sReason) {
    if(!sReason)
        return;
    auto it = funcs.find(sReason);
    if(it == funcs.end())
        return;
    it->second(this);
}

PartParams *PartParams::instance() {
    static PartParams *inst;
    if(!inst)
        inst = new PartParams;
    return inst;
}

void PartParams::onMeshDeviationChanged() {
    timer.start(100);
}

void PartParams::onMeshAngularDeflectionChanged() {
    timer.start(100);
}

void PartParams::onMinimumDeviationChanged() {
    timer.start(100);
}

void PartParams::onMinimumAngularDeflectionChanged() {
    timer.start(100);
}

void PartParams::onOverrideTessellationChanged() {
    timer.start(100);
}

void PartParams::updateVisual() {
    timer.stop();
    // search for Part view providers and apply the new settings
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (std::vector<App::Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(*it);
        std::vector<Gui::ViewProvider*> views = doc->getViewProvidersOfType(ViewProviderPart::getClassTypeId());
        for (std::vector<Gui::ViewProvider*>::iterator jt = views.begin(); jt != views.end(); ++jt) {
            static_cast<ViewProviderPart*>(*jt)->reload();
        }
    }
}

#include "moc_PartParams.cpp"
