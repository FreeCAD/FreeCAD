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
    ADD_PROPERTY(AddSubColor,((long)0));

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
    onChanged(&AddSubColor);
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

void ViewProviderAddSub::onChanged(const App::Property *p)
{
    if (p == & AddSubColor)
        checkAddSubColor();
    ViewProvider::onChanged(p);
}

void ViewProviderAddSub::checkAddSubColor()
{
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
    if (!feat)
        return;
    App::Color color((uint32_t)PartGui::PartParams::PreviewAddColor());
    if (AddSubColor.getValue().getPackedValue()) {
        color = AddSubColor.getValue();
    } else {
        if (feat) {
            if (feat->isDerivedFrom(PartDesign::DressUp::getClassTypeId())) {
                color = App::Color((uint32_t)PartGui::PartParams::PreviewDressColor());
            } else if (feat->getAddSubType() == PartDesign::FeatureAddSub::Additive)
                color = App::Color((uint32_t)PartGui::PartParams::PreviewAddColor());
            else
                color = App::Color((uint32_t)PartGui::PartParams::PreviewSubColor());
        }
    }
    // clamp transparency between 0.1 ~ 0.8
    float t = std::max(0.1f, std::min(0.8f, 1.0f - color.a));
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
        else if (p == &feat->BaseFeature) {
            auto base = feat->getBaseObject(true);
            if (!base) {
                auto baseObj = baseFeature.getObject();
                if (baseObj) {
                    setPreviewDisplayMode(false);
                    baseFeature = App::DocumentObjectT();
                    baseObj->Visibility.setValue(false);
                    setPreviewDisplayMode(true);
                    getObject()->Visibility.setValue(true);
                }
            } else if (isPreviewMode())  {
                setPreviewDisplayMode(false);
                setPreviewDisplayMode(true);
            }
        }

        if (p == &feat->BaseFeature || p == &feat->Placement) {
            Base::Matrix4D matrix;
            auto base = Base::freecad_dynamic_cast<Part::Feature>(
                    feat->getBaseObject(true));
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

void ViewProviderAddSub::setPreviewDisplayMode(bool onoff) {
    Gui::SoFCSwitch *fcSwitch =  nullptr;
    if (pcModeSwitch->isOfType(Gui::SoFCSwitch::getClassTypeId()))
        fcSwitch = static_cast<Gui::SoFCSwitch*>(pcModeSwitch);

    if (onoff) {
        checkAddSubColor();

        auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
        auto base = feat ? feat->getBaseObject(true) : nullptr;
        if (base) {
            baseFeature = App::DocumentObjectT(base);

            // If there is a base feature, we shall inject the preview group
            // into the base feature view provider using SoFCSwitch::headChild
            // functionality, which will be shown as long as SoFCSwitch is
            // visible. See SoFCSwitch documentation for more details.
            auto baseVp = Gui::Application::Instance->getViewProvider(base);
            if (baseVp && baseVp->getModeSwitch()
                       && baseVp->getModeSwitch()->isOfType(Gui::SoFCSwitch::getClassTypeId()))
            {
                feat->Visibility.setValue(false);
                base->Visibility.setValue(true);
                auto baseSwitch = static_cast<Gui::SoFCSwitch*>(baseVp->getModeSwitch());
                baseSwitch->addChild(previewGroup);
                baseChild = baseSwitch->headChild.getValue();
                baseSwitch->headChild = baseSwitch->getNumChildren()-1;
                return;
            }
        }
    } else {
        auto base = baseFeature.getObject();
        baseFeature = App::DocumentObjectT();
        if (base) {
            auto baseVp = Gui::Application::Instance->getViewProvider(base);
            if (baseVp && baseVp->getModeSwitch()
                       && baseVp->getModeSwitch()->isOfType(Gui::SoFCSwitch::getClassTypeId()))
            {
                base->Visibility.setValue(false);
                auto baseSwitch = static_cast<Gui::SoFCSwitch*>(baseVp->getModeSwitch());
                int idx = baseSwitch->findChild(previewGroup);
                if (idx >= 0)
                    baseSwitch->removeChild(idx);
                baseSwitch->headChild = baseChild;
                baseChild = -1;
                return;
            }
        }
    }

    if (onoff && !isPreviewMode()) {
        displayMode = getActiveDisplayMode();
        if (fcSwitch) {
            defaultChild = fcSwitch->defaultChild.getValue();
            fcSwitch->allowNamedOverride = false;
        }
        setDisplayMaskMode("Shape preview");
        if (fcSwitch)
            fcSwitch->defaultChild = pcModeSwitch->whichChild.getValue();
    } else if (!onoff && isPreviewMode()) {
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
