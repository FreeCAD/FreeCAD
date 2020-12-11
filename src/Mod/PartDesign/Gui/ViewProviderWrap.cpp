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
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <boost/range.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "ViewProviderWrap.h"
#include <Mod/PartDesign/App/FeatureWrap.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Base/Console.h>

typedef boost::iterator_range<const char*> CharRange;

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderWrap,PartDesignGui::ViewProviderAddSub)

ViewProviderWrap::ViewProviderWrap()
{
    this->sPixmap = "PartDesign_Wrap.svg";
    this->pcGroupChildren = new SoGroup;
    this->dispModeOverride.reset(new Gui::SoFCDisplayMode);
    this->previewGroup->addChild(this->dispModeOverride);
    this->previewGroup->addChild(this->pcGroupChildren);
}

ViewProviderWrap::~ViewProviderWrap()
{
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
}

SoGroup* ViewProviderWrap::getChildRoot(void) const
{
    return pcGroupChildren;
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
    auto owner = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(getObject());
    if(owner && owner->WrapFeature.getValue())
        return {owner->WrapFeature.getValue()};
    return {};
}

bool ViewProviderWrap::getDetailPath(
        const char *subname, SoFullPath *path, bool append, SoDetail *&det) const
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(getObject());
    auto wrapped = owner->WrapFeature.getValue();
    if (wrapped && subname
                && subname != Data::ComplexGeoData::findElementName(subname)) {
        const char * dot = strchr(subname,'.');
        if (dot && boost::equals(CharRange(subname, dot), wrapped->getNameInDocument())) {
            if(append) {
                path->append(pcRoot);
                path->append(pcModeSwitch);
            }
            path->append(previewGroup);
            path->append(pcGroupChildren);
            auto vp = Gui::Application::Instance->getViewProvider(wrapped);
            if (vp)
                return vp->getDetailPath(dot+1, path, true, det);
            return  true;
        }
    }
    return inherited::getDetailPath(subname, path, append, det);
}

void ViewProviderWrap::updateAddSubShapeIndicator()
{
}

PartGui::ViewProviderPart * ViewProviderWrap::getAddSubView()
{
    auto owner = Base::freecad_dynamic_cast<PartDesign::FeatureWrap>(getObject());
    PartGui::ViewProviderPart * vp = nullptr;
    if(owner && owner->WrapFeature.getValue())
        vp = Base::freecad_dynamic_cast<PartGui::ViewProviderPart>(
                Gui::Application::Instance->getViewProvider(
                    owner->WrapFeature.getValue()));
    if (!vp)
        return nullptr;
    return vp;
}

void ViewProviderWrap::setAddSubColor(const App::Color & color, float t)
{
    auto view = getAddSubView();
    if (!view)
        return;
    SbColor c(color.a, color.g, color.b);
    dispModeOverride->faceColor = c;
    dispModeOverride->lineColor = c;
    dispModeOverride->transparency.setValue(t);
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
