/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2017 WandererFan <wandererfan@gmail.com>                *
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
#endif

#include <App/DocumentObject.h>
#include <Base/Parameter.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>

#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "QGIView.h"
#include "TaskGeomHatch.h"
#include "PreferencesGui.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderGeomHatch.h"

using namespace TechDrawGui;
using namespace TechDraw;

PROPERTY_SOURCE(TechDrawGui::ViewProviderGeomHatch, Gui::ViewProviderDocumentObject)

//**************************************************************************
// Construction/Destruction

ViewProviderGeomHatch::ViewProviderGeomHatch()
{
    sPixmap = "actions/TechDraw_GeometricHatch";

    static const char *vgroup = "GeomHatch";

    ADD_PROPERTY_TYPE(ColorPattern,(TechDraw::DrawGeomHatch::prefGeomHatchColor()),
                        vgroup,App::Prop_None,"Color of the pattern");
    ADD_PROPERTY_TYPE(WeightPattern,(0),vgroup,App::Prop_None,"GeometricHatch pattern line thickness");

    getParameters();

}

ViewProviderGeomHatch::~ViewProviderGeomHatch()
{
}

void ViewProviderGeomHatch::attach(App::DocumentObject *pcFeat)
{
    // call parent attach method
    ViewProviderDocumentObject::attach(pcFeat);
}

void ViewProviderGeomHatch::setDisplayMode(const char* ModeName)
{
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

std::vector<std::string> ViewProviderGeomHatch::getDisplayModes(void) const
{
    // get the modes of the father
    std::vector<std::string> StrList = ViewProviderDocumentObject::getDisplayModes();

    return StrList;
}

bool ViewProviderGeomHatch::setEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    TaskDlgGeomHatch *projDlg = qobject_cast<TaskDlgGeomHatch *>(dlg);
    if (projDlg && (projDlg->getViewProvider() != this))
        projDlg = nullptr; // somebody left task panel open

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // start the edit dialog
    if (projDlg) {
        projDlg->setCreateMode(false);
        Gui::Control().showDialog(projDlg);
    } else {
        Gui::Control().showDialog(new TaskDlgGeomHatch(getViewObject(),this,false));
    }

    return true;
}

void ViewProviderGeomHatch::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
    }
    else {
        ViewProviderDocumentObject::unsetEdit(ModNum);
    }
}

bool ViewProviderGeomHatch::doubleClicked(void)
{
    setEdit(0);
    return true;
}

//for VP properties - but each letter/digit in property editor triggers this!
void ViewProviderGeomHatch::onChanged(const App::Property* p)
{
    if ((p == &WeightPattern)  ||
        (p == &ColorPattern) ) {
        auto gHatch = getViewObject();
        if (gHatch != nullptr) {
            TechDraw::DrawViewPart* parent = gHatch->getSourceView();
            if (parent) {
                parent->requestPaint();
            }
        }
    }

    Gui::ViewProviderDocumentObject::onChanged(p);
}

//for feature properties - but each letter/digit in property editor triggers this!
void ViewProviderGeomHatch::updateData(const App::Property* prop)
{
    Gui::ViewProviderDocumentObject::updateData(prop);
}

void ViewProviderGeomHatch::updateGraphic(void)
{
    TechDraw::DrawGeomHatch* dc = getViewObject();
    if (dc) {
        TechDraw::DrawViewPart* dvp = dc->getSourceView();
        if (dvp) {
            Gui::ViewProvider* view = Gui::Application::Instance->getDocument(dvp->getDocument())->getViewProvider(dvp);
            TechDrawGui::ViewProviderDrawingView* vpDV = dynamic_cast<TechDrawGui::ViewProviderDrawingView*>(view);
            if (vpDV) {
                vpDV->show();
                QGIView* qgiv = vpDV->getQView();
                if (qgiv) {
                    qgiv->updateView(true);
                }
            }
        }
   }
}

void ViewProviderGeomHatch::getParameters(void)
{
    int lgNumber = Preferences::lineGroup();
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgNumber);
    double weight = lg->getWeight("Graphic");
    delete lg;                                                    //Coverity CID 174667
    WeightPattern.setValue(weight);
}

bool ViewProviderGeomHatch::canDelete(App::DocumentObject *obj) const
{
    // deletion of hatches don't destroy anything
    // thus we can pass this action
    Q_UNUSED(obj)
    return true;
}

TechDraw::DrawGeomHatch* ViewProviderGeomHatch::getViewObject() const
{
    return dynamic_cast<TechDraw::DrawGeomHatch*>(pcObject);
}

Gui::MDIView *ViewProviderGeomHatch::getMDIView() const
{
    auto obj = getViewObject();
    if(!obj)
        return nullptr;
    auto vp = Gui::Application::Instance->getViewProvider(obj->getSourceView());
    if(!vp)
        return nullptr;
    return vp->getMDIView();
}
