/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
#include <QStatusBar>
#include <QToolBar>
#include <QMenuBar>
#include <App/Application.h>
#include "Application.h"
#include "Document.h"
#include "ViewProvider.h"
#include "ViewParams.h"
#include "TreeParams.h"
#include "Selection.h"
#include "OverlayWidgets.h"
#include "Widgets.h"
#include "MainWindow.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "QSint/actionpanel/taskheader_p.h"

using namespace Gui;

ViewParams::ViewParams() {
    handle = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
    handle->Attach(this);
#undef FC_VIEW_PARAM
#define FC_VIEW_PARAM(_name,_ctype,_type,_def,_doc) \
    _##_name = handle->Get##_type(#_name,_def);\
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

void ViewParams::onTextCursorWidthChanged() {
    LineEditStyle::setupChildren(getMainWindow());
}

void ViewParams::onSectionHatchTextureChanged() {
    for (auto doc : App::GetApplication().getDocuments()) {
        Gui::Document* gdoc = Gui::Application::Instance->getDocument(doc);
        if (gdoc) {
            for (auto v : gdoc->getMDIViewsOfType(View3DInventor::getClassTypeId())) {
                auto view = static_cast<View3DInventor*>(v)->getViewer();
                view->updateHatchTexture();
            }
        }
    }
}

void ViewParams::onRenderCacheChanged() {
    onSectionHatchTextureChanged();
}

bool ViewParams::isUsingRenderer()
{
    return instance()->getRenderCache() == 3;
}

void ViewParams::useRenderer(bool enable)
{
    if (instance()->isUsingRenderer()) {
        if (!enable)
            instance()->setRenderCache(0);
    } else if (enable)
        instance()->setRenderCache(3);
}

int ViewParams::appDefaultFontSize() {
    static int defaultSize;
    if (!defaultSize) {
        QFont font;
        defaultSize = font.pointSize();
    }
    return defaultSize;
}

void ViewParams::onDefaultFontSizeChanged() {
    int defaultSize = appDefaultFontSize();
    int fontSize = instance()->getDefaultFontSize();
    if (fontSize <= 0)
        fontSize = defaultSize;
    else if (fontSize < 8)
        fontSize = 8;
    QFont font = QApplication::font();
    if (font.pointSize() != fontSize) {
        font.setPointSize(fontSize);
        QApplication::setFont(font);
        QEvent e(QEvent::ApplicationFontChange);
        for (auto w : QApplication::allWidgets())
            QApplication::sendEvent(w, &e);
    }

    if (TreeParams::FontSize() <= 0)
        TreeParams::instance()->onFontSizeChanged();
}

void ViewParams::onEnableTaskPanelKeyTranslateChanged() {
    QSint::TaskHeader::enableKeyTranslate(_EnableTaskPanelKeyTranslate);
}

void ViewParams::init() {
    instance()->onDefaultFontSizeChanged();
}

bool ViewParams::highlightPick()
{
    return Selection().needPickedList() || AutoTransparentPick();
}

bool ViewParams::hiddenLineSelectionOnTop()
{
    return HiddenLineSelectionOnTop() || highlightPick();
}
