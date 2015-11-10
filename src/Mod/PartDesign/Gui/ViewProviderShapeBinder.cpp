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
# include <QMessageBox>
# include <Inventor/nodes/SoSeparator.h>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Control.h>

#include <Mod/PartDesign/App/ShapeBinder.h>

#include "ViewProviderShapeBinder.h"
#include "TaskShapeBinder.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderShapeBinder,PartGui::ViewProviderPart)

ViewProviderShapeBinder::ViewProviderShapeBinder()
{
    sPixmap = "PartDesign_ShapeBinder.svg";

    //make the viewprovider more datum like
    AngularDeflection.StatusBits.set(3, true);
    Deviation.StatusBits.set(3, true);
    DrawStyle.StatusBits.set(3, true);
    Lighting.StatusBits.set(3, true);
    LineColor.StatusBits.set(3, true);
    LineWidth.StatusBits.set(3, true);
    PointColor.StatusBits.set(3, true);
    PointSize.StatusBits.set(3, true);
    ShapeColor.StatusBits.set(3, true);
    Transparency.StatusBits.set(3, true);

    //get the datum coloring sheme
    ShapeColor.setValue(App::Color(0.9f, 0.9f, 0.13f, 0.5f));
    LineColor.setValue(App::Color(0.9f, 0.9f, 0.13f, 0.5f));
    PointColor.setValue(App::Color(0.9f, 0.9f, 0.13f, 0.5f));
    LineWidth.setValue(1);
}

ViewProviderShapeBinder::~ViewProviderShapeBinder()
{

}

bool ViewProviderShapeBinder::setEdit(int ModNum) {
    // TODO Share code with other view providers (2015-09-11, Fat-Zer)
    
    if (ModNum == ViewProvider::Default || ModNum == 1 ) {
        
        // When double-clicking on the item for this pad the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgShapeBinder *sbDlg = qobject_cast<TaskDlgShapeBinder*>(dlg);
        if (sbDlg)
            sbDlg = 0; // another pad left open its task panel
        if (dlg && !sbDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().reject();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // start the edit dialog
        if (sbDlg)
            Gui::Control().showDialog(sbDlg);
        else
            Gui::Control().showDialog(new TaskDlgShapeBinder(this,ModNum == 1));

        return true;
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

void ViewProviderShapeBinder::unsetEdit(int ModNum) {
    
    PartGui::ViewProviderPart::unsetEdit(ModNum);
}

void ViewProviderShapeBinder::highlightReferences(const bool on, bool auxillery) {
    
    Part::Feature* obj;
    std::vector<std::string> subs;
    
    if(getObject()->isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()))
        PartDesign::ShapeBinder::getFilterdReferences(&static_cast<PartDesign::ShapeBinder*>(getObject())->Support, obj, subs);
    else if(getObject()->isDerivedFrom(PartDesign::ShapeBinder2D::getClassTypeId()))
        PartDesign::ShapeBinder::getFilterdReferences(&static_cast<PartDesign::ShapeBinder2D*>(getObject())->Support, obj, subs);
    else 
        return;
        
    PartGui::ViewProviderPart* svp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(obj));
    if (svp == NULL) return;

    if (on) {        
         if (!subs.empty() && originalLineColors.empty()) {
            TopTools_IndexedMapOfShape eMap;
            TopExp::MapShapes(obj->Shape.getValue(), TopAbs_EDGE, eMap);
            originalLineColors = svp->LineColorArray.getValues();
            std::vector<App::Color> lcolors = originalLineColors;
            lcolors.resize(eMap.Extent(), svp->LineColor.getValue());

            TopExp::MapShapes(obj->Shape.getValue(), TopAbs_FACE, eMap);
            originalFaceColors = svp->DiffuseColor.getValues();
            std::vector<App::Color> fcolors = originalFaceColors;
            fcolors.resize(eMap.Extent(), svp->ShapeColor.getValue());

            for (std::string e : subs) {
                // Note: stoi may throw, but it strictly shouldn't happen
                if(e.substr(4) == "Edge") {
                    int idx = std::stoi(e.substr(4)) - 1;
                    assert ( idx>=0 );
                    if ( idx < (ssize_t) lcolors.size() )
                        lcolors[idx] = App::Color(1.0,0.0,1.0); // magenta
                }
                else if(e.substr(4) == "Face")  {
                    int idx = std::stoi(e.substr(4)) - 1;
                    assert ( idx>=0 );
                    if ( idx < (ssize_t) fcolors.size() )
                        fcolors[idx] = App::Color(1.0,0.0,1.0); // magenta
                }
            }
            svp->LineColorArray.setValues(lcolors);
            svp->DiffuseColor.setValues(fcolors);
        }
    } else {
        if (!subs.empty() && !originalLineColors.empty()) {
            svp->LineColorArray.setValues(originalLineColors);
            originalLineColors.clear();
            
            svp->DiffuseColor.setValues(originalFaceColors);
            originalFaceColors.clear();
        }
    }
}
