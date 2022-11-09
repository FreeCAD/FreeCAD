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

    ADD_PROPERTY_TYPE(ColorPattern, (TechDraw::DrawGeomHatch::prefGeomHatchColor()),
                        vgroup, App::Prop_None, "Color of the pattern");
    ADD_PROPERTY_TYPE(WeightPattern, (0), vgroup, App::Prop_None, "GeometricHatch pattern line thickness");

    getParameters();

}

ViewProviderGeomHatch::~ViewProviderGeomHatch()
{
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
        Gui::Control().showDialog(new TaskDlgGeomHatch(getViewObject(), this, false));
    }

    return true;
}

bool ViewProviderGeomHatch::doubleClicked()
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
        if (gHatch) {
            TechDraw::DrawViewPart* parent = gHatch->getSourceView();
            if (parent) {
                parent->requestPaint();
            }
        }
    }

    Gui::ViewProviderDocumentObject::onChanged(p);
}

//for feature properties
void ViewProviderGeomHatch::updateData(const App::Property* prop)
{
    if ( prop == &(getViewObject()->FilePattern) ||
            prop == &(getViewObject()->NamePattern) ) {
        TechDraw::DrawViewPart* parent = getViewObject()->getSourceView();
        if (parent) {
            parent->requestPaint();
        }
    }

    Gui::ViewProviderDocumentObject::updateData(prop);
}

void ViewProviderGeomHatch::updateGraphic()
{
    TechDraw::DrawGeomHatch* dc = getViewObject();
    if (!dc) {
        return;
    }

    TechDraw::DrawViewPart* dvp = dc->getSourceView();
    if (!dvp) {
        return;
    }

    Gui::ViewProvider* view = Gui::Application::Instance->getDocument(dvp->getDocument())->getViewProvider(dvp);
    TechDrawGui::ViewProviderDrawingView* vpDV = dynamic_cast<TechDrawGui::ViewProviderDrawingView*>(view);
    if (!vpDV) {
        return;
    }
    vpDV->show();

    QGIView* qgiv = vpDV->getQView();
    if (!qgiv) {
        return;
    }
    qgiv->updateView(true);
}

void ViewProviderGeomHatch::getParameters()
{
    double weight = TechDraw::LineGroup::getDefaultWidth("Graphic");
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
