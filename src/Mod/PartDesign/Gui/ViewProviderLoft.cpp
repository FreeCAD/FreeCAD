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
#endif

#include "ViewProviderLoft.h"
//#include "TaskLoftParameters.h"
#include "Workbench.h"
#include "TaskLoftParameters.h"
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/FeatureLoft.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderLoft,PartDesignGui::ViewProvider)

ViewProviderLoft::ViewProviderLoft()
{
}

ViewProviderLoft::~ViewProviderLoft()
{
}

std::vector<App::DocumentObject*> ViewProviderLoft::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;
    App::DocumentObject* sketch = static_cast<PartDesign::Loft*>(getObject())->Sketch.getValue();
    if (sketch != NULL)
        temp.push_back(sketch);

    return temp;
}

void ViewProviderLoft::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{/*
    QAction* act;
    act = menu->addAction(QObject::tr("Edit pad"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);*/
}

bool ViewProviderLoft::doubleClicked(void)
{
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().setEdit('%s',0)",this->pcObject->getNameInDocument());
    return true;
}

bool ViewProviderLoft::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default || ModNum == 1 ) {
        
        setPreviewDisplayMode(true);
        /*
        // When double-clicking on the item for this pad the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgLoftParameters *padDlg = qobject_cast<TaskDlgLoftParameters *>(dlg);
        if (padDlg && padDlg->getLoftView() != this)
            padDlg = 0; // another pad left open its task panel
        if (dlg && !padDlg) {
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

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog
        if (padDlg)
            Gui::Control().showDialog(padDlg);
        else
            Gui::Control().showDialog(new TaskDlgLoftParameters(this,ModNum == 1));
        */
        return true;
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

void ViewProviderLoft::unsetEdit(int ModNum) {
    setPreviewDisplayMode(false);
    PartDesignGui::ViewProvider::unsetEdit(ModNum);
}


bool ViewProviderLoft::onDelete(const std::vector<std::string> &s)
{/*
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());

    // get the Sketch
    Sketcher::SketchObject *pcSketch = 0;
    if (pcLoft->Sketch.getValue())
        pcSketch = static_cast<Sketcher::SketchObject*>(pcLoft->Sketch.getValue());

    // if abort command deleted the object the sketch is visible again
    if (pcSketch && Gui::Application::Instance->getViewProvider(pcSketch))
        Gui::Application::Instance->getViewProvider(pcSketch)->show();

    return ViewProvider::onDelete(s);*/
}



void ViewProviderLoft::highlightReferences(const bool on, bool auxillery)
{/*
    PartDesign::Loft* pcLoft = static_cast<PartDesign::Loft*>(getObject());
    Part::Feature* base;
    if(!auxillery)
        base = static_cast<Part::Feature*>(pcLoft->Spine.getValue());
    else 
        base = static_cast<Part::Feature*>(pcLoft->AuxillerySpine.getValue());
    
    if (base == NULL) return;
    PartGui::ViewProviderPart* svp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(base));
    if (svp == NULL) return;

    std::vector<std::string> edges;
    if(!auxillery)
        edges = pcLoft->Spine.getSubValuesStartsWith("Edge");
    else 
        edges = pcLoft->AuxillerySpine.getSubValuesStartsWith("Edge");

    if (on) {        
         if (!edges.empty() && originalLineColors.empty()) {
            TopTools_IndexedMapOfShape eMap;
            TopExp::MapShapes(base->Shape.getValue(), TopAbs_EDGE, eMap);
            originalLineColors = svp->LineColorArray.getValues();
            std::vector<App::Color> colors = originalLineColors;
            colors.resize(eMap.Extent(), svp->LineColor.getValue());

            for (std::string e : edges) {
                int idx = atoi(e.substr(4).c_str()) - 1;
                if (idx < colors.size())
                    colors[idx] = App::Color(1.0,0.0,1.0); // magenta
            }
            svp->LineColorArray.setValues(colors);
        }
    } else {
        if (!edges.empty() && !originalLineColors.empty()) {
            svp->LineColorArray.setValues(originalLineColors);
            originalLineColors.clear();
        }
    }*/
}

QIcon ViewProviderLoft::getIcon(void) const {
    QString str = QString::fromAscii("PartDesign_");
    auto* prim = static_cast<PartDesign::Loft*>(getObject());
    if(prim->getAddSubType() == PartDesign::FeatureAddSub::Additive)
        str += QString::fromAscii("Additive_");
    else
        str += QString::fromAscii("Subtractive_");
 
    str += QString::fromAscii("Loft.svg");
    return Gui::BitmapFactory().pixmap(str.toStdString().c_str());
}

