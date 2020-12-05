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
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <BRepMesh_IncrementalMesh.hxx>
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Standard_Version.hxx>
#endif

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

    previewGroup = new SoSeparator();
    previewGroup->ref();
}

ViewProviderAddSub::~ViewProviderAddSub()
{
    previewGroup->unref();
}

void ViewProviderAddSub::attach(App::DocumentObject* obj) {

    ViewProvider::attach(obj);

    pAddSubView.reset(new PartGui::ViewProviderPart);
    pAddSubView->setShapePropertyName("AddSubShape");
    pAddSubView->forceUpdate();
    pAddSubView->MapFaceColor.setValue(false);    
    pAddSubView->MapLineColor.setValue(false);    
    pAddSubView->MapPointColor.setValue(false);    
    pAddSubView->MapTransparency.setValue(false);    
    pAddSubView->ForceMapColors.setValue(false);
    pAddSubView->Selectable.setValue(false);
    pAddSubView->Lighting.setValue(1);
    pAddSubView->enableFullSelectionHighlight(false, false, false);
    pAddSubView->setStatus(Gui::SecondaryView,true);

    pAddSubView->attach(obj);

    pAddSubView->setDefaultMode(0);
    pAddSubView->show();

    onChanged(&AddSubColor);

    previewGroup->addChild(pAddSubView->getRoot());
    addDisplayMaskMode(previewGroup, "Shape preview");
}

void ViewProviderAddSub::finishRestoring()
{
    ViewProvider::finishRestoring();
    updateAddSubShapeIndicator();
}

void ViewProviderAddSub::updateAddSubShapeIndicator()
{
    pAddSubView->updateVisual();
}

void ViewProviderAddSub::onChanged(const App::Property *p)
{
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
    if (p == & AddSubColor) {
        App::Color color((uint32_t)PartGui::PartParams::PreviewAddColor());
        float t = 1.0f - color.a;
        if (AddSubColor.getValue().getPackedValue()) {
            color = AddSubColor.getValue();
            t = 1.0 - color.a;
            if (t < 0.1)
                t = 0.7;
            if (t > 0.9)
                t = 0.7;
        } else {
            if (feat) {
                if (feat->isDerivedFrom(PartDesign::DressUp::getClassTypeId())) {
                    color = App::Color((uint32_t)PartGui::PartParams::PreviewDressColor());
                } else if (feat->getAddSubType() == PartDesign::FeatureAddSub::Additive)
                    color = App::Color((uint32_t)PartGui::PartParams::PreviewAddColor());
                else
                    color = App::Color((uint32_t)PartGui::PartParams::PreviewSubColor());
                t = 1.0f - color.a;
            }
        }
        auto material = pAddSubView->ShapeMaterial.getValue();
        material.diffuseColor = color;
        material.transparency = t;
        pAddSubView->ShapeMaterial.setValue(material);
        pAddSubView->LineMaterial.setValue(material);
        material.transparency = 1.0f;
        pAddSubView->PointMaterial.setValue(material);
    }
    ViewProvider::onChanged(p);
}

void ViewProviderAddSub::updateData(const App::Property* p) {
    auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
    if (feat) {
        if (p == &feat->AddSubShape)
            updateAddSubShapeIndicator();
        else if (p == &feat->BaseFeature) {
            auto base = baseFeature.getObject();
            if (base) {
                baseFeature = App::DocumentObjectT();
                base->Visibility.setValue(false);
            }
            if (isPreviewMode())  {
                base = feat->BaseFeature.getValue();
                baseFeature = base;
                if (base)
                    base->Visibility.setValue(true);
            }
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
        auto feat = Base::freecad_dynamic_cast<PartDesign::FeatureAddSub>(getObject());
        if (feat && feat->BaseFeature.getValue()) {
            auto base = feat->BaseFeature.getValue();
            baseFeature = App::DocumentObjectT(base);
            base->Visibility.setValue(true);

            // If there is a base feature, we shall inject the preview group
            // into the base feature view provider using SoFCSwitch::tailChild
            // functionality, which will be shown as long as SoFCSwitch is
            // visible. See SoFCSwitch documentation for more details.
            auto baseVp = Gui::Application::Instance->getViewProvider(base);
            if (baseVp && baseVp->getModeSwitch()
                       && baseVp->getModeSwitch()->isOfType(Gui::SoFCSwitch::getClassTypeId()))
            {
                auto baseSwitch = static_cast<Gui::SoFCSwitch*>(baseVp->getModeSwitch());
                baseSwitch->addChild(previewGroup);
                baseTail = baseSwitch->tailChild.getValue();
                baseSwitch->tailChild = baseSwitch->getNumChildren()-1;
                return;
            }

            // If there is no base feature, we'll make us temporarily visible,
            // and switch to preview display mode below.
            makeTemporaryVisible(true);
        }
    } else if (!onoff) {
        auto base = baseFeature.getObject();
        baseFeature = App::DocumentObjectT();
        if (base) {
            base->Visibility.setValue(false);

            auto baseVp = Gui::Application::Instance->getViewProvider(base);
            if (baseVp && baseVp->getModeSwitch()
                       && baseVp->getModeSwitch()->isOfType(Gui::SoFCSwitch::getClassTypeId()))
            {
                auto baseSwitch = static_cast<Gui::SoFCSwitch*>(baseVp->getModeSwitch());
                int idx = baseSwitch->findChild(previewGroup);
                if (idx >= 0)
                    baseSwitch->removeChild(idx);
                baseSwitch->tailChild = baseTail;
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
        if (!Visibility.getValue())
            Gui::ViewProvider::hide();
        if (fcSwitch) {
            fcSwitch->defaultChild.setValue(defaultChild);
            fcSwitch->allowNamedOverride = true;
        }
    }
}

void ViewProviderAddSub::hide(void)
{
    ViewProvider::hide();
    if(isPreviewMode())
        makeTemporaryVisible(true);
}

bool ViewProviderAddSub::setEdit(int ModNum)
{
    if (ViewProvider::setEdit(ModNum)) {
        if (ModNum == ViewProvider::Default)
            setPreviewDisplayMode(true);
        return true;
    }
    return false;
}

void ViewProviderAddSub::unsetEdit(int ModNum) {
    if (ModNum == ViewProvider::Default)
        setPreviewDisplayMode(false);
    ViewProvider::unsetEdit(ModNum);
}
