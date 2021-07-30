/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#ifndef _PreComp_
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoTransform.h>
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Standard_Version.hxx>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include "ViewProviderAddSub.h"
#include <Mod/Part/Gui/PartParams.h>
#include <Mod/PartDesign/App/FeatureAddSub.h>
#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/SoFCSelectionAction.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Base/Console.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderAddSub,PartDesignGui::ViewProvider)

ViewProviderAddSub::ViewProviderAddSub()
{
    previewTransform = new SoTransform;
    auto group = new Gui::SoFCPathAnnotation;
    group->priority = -2;
    group->addChild(previewTransform);
    previewGroup = group;
}

ViewProviderAddSub::~ViewProviderAddSub()
{
}

void ViewProviderAddSub::attach(App::DocumentObject* obj) {

    ViewProvider::attach(obj);
    std::string name(obj->getNameInDocument());
    name += "_preview";
    previewGroup->setName(name.c_str());
    addDisplayMaskMode(previewGroup, "Shape preview");
}

PartGui::ViewProviderPart * ViewProviderAddSub::getAddSubView()
{
    if (pAddSubView) {
        if (pAddSubView->testStatus(Gui::Detach))
            pAddSubView->reattach(getObject());
        return pAddSubView.get();
    }

    pAddSubView.reset(new PartGui::ViewProviderPart);
    pAddSubView->setShapePropertyName("AddSubShape");
    pAddSubView->forceUpdate();
    pAddSubView->MapFaceColor.setValue(false);    
    pAddSubView->MapLineColor.setValue(false);    
    pAddSubView->MapPointColor.setValue(false);    
    pAddSubView->MapTransparency.setValue(false);    
    pAddSubView->ForceMapColors.setValue(false);
    pAddSubView->setHighlightFaceEdges(true);
    pAddSubView->Selectable.setValue(false);
    pAddSubView->Lighting.setValue(1);
    pAddSubView->enableFullSelectionHighlight(false, false, false);
    pAddSubView->setStatus(Gui::SecondaryView,true);
    if (pAddSubView->LineWidth.getValue() < 2.0f)
        pAddSubView->LineWidth.setValue(2.0f);

    pAddSubView->attach(getObject());
    pAddSubView->setDefaultMode(0);
    pAddSubView->show();
    previewGroup->addChild(pAddSubView->getRoot());
    return pAddSubView.get();
}

void ViewProviderAddSub::finishRestoring()
{
    ViewProvider::finishRestoring();
    updateAddSubShapeIndicator();
}

void ViewProviderAddSub::updateAddSubShapeIndicator()
{
    auto view = getAddSubView();
    if (view)
        view->updateVisual();
}

void ViewProviderAddSub::checkAddSubColor()
{
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
    if (!feat)
        return;
    App::Color color((uint32_t)PartGui::PartParams::PreviewAddColor());
    if (feat) {
        if (feat->isDerivedFrom(PartDesign::DressUp::getClassTypeId()))
            color = App::Color((uint32_t)PartGui::PartParams::PreviewDressColor());
        else if (feat->getAddSubType() == PartDesign::FeatureAddSub::Subtractive)
            color = App::Color((uint32_t)PartGui::PartParams::PreviewSubColor());
        else if (feat->getAddSubType() == PartDesign::FeatureAddSub::Intersecting)
            color = App::Color((uint32_t)PartGui::PartParams::PreviewIntersectColor());
        else
            color = App::Color((uint32_t)PartGui::PartParams::PreviewAddColor());
    }
    // clamp transparency between 0.1 ~ 0.8
    float t = std::max(0.1f, std::min(0.8f, 1.0f - color.a));
    if (!PartGui::PartParams::PreviewWithTransparency()) {
        t = 0.0f;
        previewGroup->priority = 0;
    } else
        previewGroup->priority = -2;
    setAddSubColor(color, t);
}

void ViewProviderAddSub::setAddSubColor(const App::Color & color, float t)
{
    auto view = getAddSubView();
    if (!view)
        return;
    view->LineColor.setValue(color);
    auto material = view->PointMaterial.getValue();
    material.diffuseColor = color;
    material.transparency = 1.0f;
    view->PointMaterial.setValue(material);
    view->ShapeColor.setValue(color);
    view->Transparency.setValue(t*100);
}

void ViewProviderAddSub::updateData(const App::Property* p) {
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
    if (feat) {
        if (p == &feat->AddSubShape)
            updateAddSubShapeIndicator();
        else if (p == &feat->AddSubType) {
            checkAddSubColor();
            signalChangeIcon();
        } else if (p == &feat->BaseFeature) {
            if (previewActive) {
                setPreviewDisplayMode(false);
                setPreviewDisplayMode(true);
            }
        }

        if (p == &feat->BaseFeature || p == &feat->Placement) {
            Base::Matrix4D matrix;
            auto base = Base::freecad_dynamic_cast<Part::Feature>(
                    feat->BaseFeature.getValue());
            if (base)
                matrix = base->Placement.getValue().inverse().toMatrix()
                         * feat->Placement.getValue().toMatrix();
            previewTransform->setMatrix(convert(matrix));
        }
    }
    PartDesignGui::ViewProvider::updateData(p);
}

bool ViewProviderAddSub::isPreviewMode() const
{
    int mode = getDefaultMode(true);
    return pcModeSwitch
        && mode >= 0
        && pcModeSwitch->getNumChildren() > mode
        && pcModeSwitch->getChild(mode) == previewGroup;
}

void ViewProviderAddSub::setPreviewDisplayMode(bool on) {
    SoFCSwitch *fcSwitch =  nullptr;
    if (pcModeSwitch->isOfType(SoFCSwitch::getClassTypeId()))
        fcSwitch = static_cast<SoFCSwitch*>(pcModeSwitch);

    if (on) {
        checkAddSubColor();

        auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
        auto base = feat ? feat->BaseFeature.getValue() : nullptr;
        if (base) {
            baseFeature = App::DocumentObjectT(base);

            // If there is a base feature, we shall inject the preview group
            // into the base feature view provider using SoFCSwitch::headChild
            // functionality, which will be shown as long as SoFCSwitch is
            // visible. See SoFCSwitch documentation for more details.
            auto baseVp = Gui::Application::Instance->getViewProvider(base);
            if (baseVp && baseVp->getModeSwitch()
                       && baseVp->getModeSwitch()->isOfType(SoFCSwitch::getClassTypeId()))
            {
                hide();
                base->Visibility.setValue(true);
                auto baseSwitch = static_cast<SoFCSwitch*>(baseVp->getModeSwitch());
                baseSwitch->addChild(previewGroup);
                baseChild = baseSwitch->headChild.getValue();
                baseSwitch->headChild = baseSwitch->getNumChildren()-1;
                previewActive = true;
                return;
            }
        }
    } else {
        previewActive = false;
        auto base = baseFeature.getObject();
        baseFeature = App::DocumentObjectT();
        if (base) {
            auto baseVp = Gui::Application::Instance->getViewProvider(base);
            if (baseVp && baseVp->getModeSwitch()
                       && baseVp->getModeSwitch()->isOfType(SoFCSwitch::getClassTypeId()))
            {
                base->Visibility.setValue(false);
                auto baseSwitch = static_cast<SoFCSwitch*>(baseVp->getModeSwitch());
                int idx = baseSwitch->findChild(previewGroup);
                if (idx >= 0)
                    baseSwitch->removeChild(idx);
                baseSwitch->headChild = baseChild;
                baseChild = -1;
                return;
            }
        }
    }

    if (on) {
        previewActive = true;
        if (!isPreviewMode()) {
            show();
            displayMode = getActiveDisplayMode();
            if (fcSwitch) {
                defaultChild = fcSwitch->defaultChild.getValue();
                fcSwitch->allowNamedOverride = false;
            }
            setDisplayMaskMode("Shape preview");
            if (fcSwitch)
                fcSwitch->defaultChild = pcModeSwitch->whichChild.getValue();
        }
    } else if (isPreviewMode()) {
        setDisplayMaskMode(displayMode.c_str());
        if (fcSwitch) {
            fcSwitch->defaultChild.setValue(defaultChild);
            fcSwitch->allowNamedOverride = true;
        }
    }
}

void ViewProviderAddSub::beforeDelete()
{
    setPreviewDisplayMode(false);
    if (pAddSubView)
        pAddSubView->beforeDelete();
    ViewProvider::beforeDelete();
}

bool ViewProviderAddSub::setEdit(int ModNum)
{
    return ViewProvider::setEdit(ModNum);
}

void ViewProviderAddSub::unsetEdit(int ModNum) {
    ViewProvider::unsetEdit(ModNum);
    if (pAddSubView)
        pAddSubView->beforeDelete();
}

bool ViewProviderAddSub::getDetailPath(const char *subname,
                                       SoFullPath *pPath,
                                       bool append,
                                       SoDetail *&det) const
{
    const std::string &prefix = PartDesign::FeatureAddSub::addsubElementPrefix();
    if (boost::starts_with(subname, prefix)) {
        auto view = const_cast<ViewProviderAddSub*>(this)->getAddSubView();
        if (!view)
            return false;
        subname += prefix.size();
        if(append) {
            pPath->append(pcRoot);
            pPath->append(pcModeSwitch);
        }
        pPath->append(previewGroup);
        return view->getDetailPath(subname, pPath, true, det);
    }
    return ViewProvider::getDetailPath(subname, pPath, append, det);
}

void ViewProviderAddSub::reattach(App::DocumentObject *obj)
{
    ViewProvider::reattach(obj);
    if (pAddSubView)
        pAddSubView->reattach(obj);
}

QIcon ViewProviderAddSub::getIcon() const
{
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
    if (!sPixmap || !feat)
        return QIcon();
    std::string name;
    static const char prefixNone[] = "PartDesign_";
    static const char prefixAdditive[] = "PartDesign_Additive";
    static const char prefixSubtractive[] = "PartDesign_Subtractive";
    static const char prefixIntersecting[] = "PartDesign_Intersecting";
    switch(feat->getAddSubType()) {
    case PartDesign::FeatureAddSub::Subtractive:
        name = prefixSubtractive;
        break;
    case PartDesign::FeatureAddSub::Intersecting:
        name = prefixIntersecting;
        break;
    default:
        name = prefixAdditive;
    }

    if (boost::starts_with(sPixmap, prefixAdditive))
        name += sPixmap + sizeof(prefixAdditive) - 1;
    else if (boost::starts_with(sPixmap, prefixNone))
        name += sPixmap + sizeof(prefixNone) -1;
    else
        return PartDesignGui::ViewProvider::getIcon();

    auto pixmap = Gui::BitmapFactory().pixmap(name.c_str(), true);
    if (pixmap.isNull())
        return PartDesignGui::ViewProvider::getIcon();
    return pixmap;
}
