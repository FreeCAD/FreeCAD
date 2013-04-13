/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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
# include <QAction>
# include <QMenu>
#endif

#include "ViewProviderDatum.h"
#include "TaskDatumParameters.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/DatumFeature.h>
#include <Gui/Control.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatum,PartDesignGui::ViewProvider)

ViewProviderDatum::ViewProviderDatum()
{    
}

ViewProviderDatum::~ViewProviderDatum()
{
}

void ViewProviderDatum::attach(App::DocumentObject *obj)
{
    ViewProvider::attach(obj);

    PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(getObject());
    if (pcDatum->getTypeId() == PartDesign::Plane::getClassTypeId())
        datumType = QObject::tr("Plane");
    else if (pcDatum->getTypeId() == PartDesign::Line::getClassTypeId())
        datumType = QObject::tr("Line");
    else if (pcDatum->getTypeId() == PartDesign::Point::getClassTypeId())
        datumType = QObject::tr("Point");
}

void ViewProviderDatum::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit datum ") + datumType, receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderDatum::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this datum feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgDatumParameters *datumDlg = qobject_cast<TaskDlgDatumParameters *>(dlg);
        if (datumDlg && datumDlg->getDatumView() != this)
            datumDlg = 0; // another datum feature left open its task panel
        if (dlg && !datumDlg) {
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

        // start the edit dialog
        if (datumDlg)
            Gui::Control().showDialog(datumDlg);
        else
            Gui::Control().showDialog(new TaskDlgDatumParameters(this));

        return true;
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

void ViewProviderDatum::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
    }
}


