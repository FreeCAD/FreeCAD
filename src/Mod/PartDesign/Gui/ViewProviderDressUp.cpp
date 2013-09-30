/***************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net>        *
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
# include <QVariant>
# include <QMenu>
# include <QAction>
# include <QMessageBox>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
#endif

#include "ViewProviderDressUp.h"
#include "TaskDressUpParameters.h"
#include <Mod/PartDesign/App/FeatureDressUp.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDressUp,PartDesignGui::ViewProvider)

void ViewProviderDressUp::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr((std::string("Edit ") + featureName + " feature").c_str()), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

const bool ViewProviderDressUp::checkDlgOpen(TaskDlgDressUpParameters* dressUpDlg) {
    // When double-clicking on the item for this feature the
    // object unsets and sets its edit mode without closing
    // the task panel
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    dressUpDlg = qobject_cast<TaskDlgDressUpParameters *>(dlg);

    if ((dressUpDlg != NULL) && (dressUpDlg->getDressUpView() != this))
        dressUpDlg = NULL; // another transformed feature left open its task panel

    if ((dlg != NULL) && (dressUpDlg == NULL)) {
        QMessageBox msgBox;
        msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
            Gui::Control().closeDialog();
        else
            return false;
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // Continue (usually in virtual method setEdit())
    return true;
}

bool ViewProviderDressUp::onDelete(const std::vector<std::string> &s)
{
    return ViewProvider::onDelete(s);
}

void ViewProviderDressUp::highlightReferences(const bool on)
{
    PartDesign::DressUp* pcDressUp = static_cast<PartDesign::DressUp*>(getObject());
    Part::Feature* base = static_cast<Part::Feature*>(pcDressUp->Base.getValue());
    if (base == NULL) return;
    PartGui::ViewProviderPart* vp = dynamic_cast<PartGui::ViewProviderPart*>(
                Gui::Application::Instance->getViewProvider(base));
    if (vp == NULL) return;

    if (on) {
        std::vector<std::string> SubVals = pcDressUp->Base.getSubValuesStartsWith("Face");
        if (SubVals.size() == 0) return;

        TopTools_IndexedMapOfShape fMap;
        TopExp::MapShapes(base->Shape.getValue(), TopAbs_FACE, fMap);

        originalColors = vp->DiffuseColor.getValues();
        std::vector<App::Color> colors = originalColors;
        colors.resize(fMap.Extent(), ShapeColor.getValue());

        for (std::vector<std::string>::const_iterator f = SubVals.begin(); f != SubVals.end(); f++) {
            int idx = atoi(f->substr(4).c_str()) - 1;
            // TODO: Find a better colour
            colors[idx] = App::Color(0.2,1,0.2);
        }
        vp->DiffuseColor.setValues(colors);
    } else {
        vp->DiffuseColor.setValues(originalColors);
    }
}

