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

#ifndef _PreComp_
# include <QApplication>
# include <QMenu>
# include <Inventor/nodes/SoGroup.h>
#endif

#include "ViewProviderWrap.h"
#include <Mod/PartDesign/App/FeatureWrap.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Base/Console.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderWrap,PartDesignGui::ViewProvider)

ViewProviderWrap::ViewProviderWrap()
{
    sPixmap = "PartDesign_Wrap.svg";

    ADD_PROPERTY(Display,((long)0));
    static const char* DisplayEnum[] = {"Result","Wrap",NULL};
    Display.setEnums(DisplayEnum);

    pcGroupChildren = new SoGroup;
}

ViewProviderWrap::~ViewProviderWrap()
{
}

void ViewProviderWrap::attach(App::DocumentObject* obj) {
    inherited::attach(obj);
    addDisplayMaskMode(pcGroupChildren, "Wrap");
}

void ViewProviderWrap::updateData(const App::Property* p) {
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(getObject());
    if (feat) {
        if (p == &feat->Type || p == &feat->Frozen || p == &feat->WrapFeature) {
            if (IconColor.getValue().getPackedValue() && !feat->isSolidFeature())
                IconColor.setValue(0);
            signalChangeIcon();
        }
    }
    inherited::updateData(p);
}

Gui::ViewProviderDocumentObject * ViewProviderWrap::getWrappedView() const
{
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(getObject());
    if (!feat)
        return nullptr;
    return Base::freecad_dynamic_cast<Gui::ViewProviderDocumentObject>(
            Gui::Application::Instance->getViewProvider(feat->WrapFeature.getValue()));
}

QIcon ViewProviderWrap::getIcon() const
{
    Gui::ViewProvider *vp = nullptr;
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(getObject());
    if (feat)
        vp = Gui::Application::Instance->getViewProvider(feat->WrapFeature.getValue());

    if (!vp)
        return inherited::getIcon();

    std::map<unsigned long, unsigned long> colormap;
    const unsigned long defcolor = 0xffcc20;
    if (feat->Frozen.getValue())
        colormap[defcolor] = 0x00b4ff;
    else {
        switch(feat->Type.getValue()) {
        case 1:
            colormap[defcolor] = 0xe00000;
            break;
        case 2:
            colormap[defcolor] = 0xefefef;
            break;
        }
    }

    QPixmap px = Gui::BitmapFactory().pixmapFromSvg(
            "PartDesign_Wrap_Overlay.svg", QSizeF(64,64), colormap);
    return Gui::BitmapFactory().merge(vp->getIcon().pixmap(64),
            px, Gui::BitmapFactoryInst::BottomRight);
}

void ViewProviderWrap::onChanged(const App::Property* prop) {

    inherited::onChanged(prop);

    if(prop == &Display) {
        if(Display.getValue() == 0) {
            auto vp = getBodyViewProvider();
            if(vp)
                setDisplayMode(vp->DisplayMode.getValueAsString());
            else
                setDisplayMode("Flat Lines");
        } else {
            setDisplayMaskMode("Wrap");
            setDisplayMode("Wrap");
        }
    }
}

SoGroup* ViewProviderWrap::getChildRoot(void) const
{
    return pcChildGroup;
}

std::vector<App::DocumentObject*> ViewProviderWrap::claimChildren(void) const
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(getObject());
    auto res = inherited::claimChildren();
    if(owner && owner->WrapFeature.getValue())
        res.insert(res.begin(), owner->WrapFeature.getValue());
    return res;
}

std::vector<App::DocumentObject*> ViewProviderWrap::claimChildren3D(void) const
{
    return claimChildren();
}

void ViewProviderWrap::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    inherited::setupContextMenu(menu, receiver, member);
    auto owner = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(getObject());
    if (!owner)
        return;
    auto vp = Gui::Application::Instance->getViewProvider(owner->WrapFeature.getValue());
    if (!vp)
        return;

    int count = menu->actions().size();
    vp->setupContextMenu(menu, receiver, member);
    for(auto action : menu->actions()) {
        if (--count > 0)
            continue;

        int data = action->data().toInt();
        if (data == Gui::ViewProvider::Color && (data & 0x8000))
            menu->removeAction(action);
        else
            action->setData(data & 0x8000);
    }
}

Gui::ViewProvider * ViewProviderWrap::startEditing(int mode)
{
    if (!(mode & 0x8000))
        return inherited::startEditing(mode);

    auto vp = getWrappedView();
    if (!vp)
        return nullptr;
    mode &= ~0x8000;
    return vp->startEditing(mode);
}

bool ViewProviderWrap::doubleClicked()
{
    if (QApplication::queryKeyboardModifiers()
            & (Qt::ControlModifier | Qt::ShiftModifier))
    {
        return ViewProvider::doubleClicked();
    }

    auto vp = getWrappedView();
    if (!vp)
        return false;

    auto sels = Gui::Selection().getSelectionT("*", false, true);
    if (sels.empty() || sels.front().getSubObject() != getObject())
        return false;

    Gui::Selection().clearSelection();
    auto & sel = sels.front();
    sel.setSubName(sel.getSubNameNoElement() 
            + vp->getObject()->getNameInDocument() + ".");
    Gui::Selection().addSelection(sel);
    return vp->doubleClicked();
}
