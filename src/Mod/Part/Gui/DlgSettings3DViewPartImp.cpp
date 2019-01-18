/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Python.h>
#include <QMessageBox>
#include "ViewProvider.h"
#include "DlgSettings3DViewPartImp.h"
#include "ui_DlgSettings3DViewPart.h"

#include <Gui/PrefWidgets.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>

using namespace PartGui;

/**
 *  Constructs a DlgSettings3DViewPart which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettings3DViewPart::DlgSettings3DViewPart(QWidget* parent)
  : PreferencePage(parent), ui(new Ui_DlgSettings3DViewPart), checkValue(false)
{
    ui->setupUi(this);
    ParameterGrp::handle hPart = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Mod/Part");
    double lowerLimit = hPart->GetFloat("MinimumDeviation", ui->maxDeviation->minimum());
    ui->maxDeviation->setMinimum(lowerLimit);
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettings3DViewPart::~DlgSettings3DViewPart()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettings3DViewPart::on_maxDeviation_valueChanged(double v)
{
    if (!this->isVisible())
        return;
    if (v < 0.01 && !checkValue) {
        checkValue = true;
        QMessageBox::warning(this, tr("Deviation"),
            tr("Setting a too small deviation causes the tessellation to take longer"
               "and thus freezes or slows down the GUI."));
    }
}

void DlgSettings3DViewPart::saveSettings()
{
    ui->maxDeviation->onSave();
    ui->maxAngularDeflection->onSave();

    // search for Part view providers and apply the new settings
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (std::vector<App::Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(*it);
        std::vector<Gui::ViewProvider*> views = doc->getViewProvidersOfType(ViewProviderPart::getClassTypeId());
        for (std::vector<Gui::ViewProvider*>::iterator jt = views.begin(); jt != views.end(); ++jt) {
            static_cast<ViewProviderPart*>(*jt)->reload();
        }
    }
}

void DlgSettings3DViewPart::loadSettings()
{
    ui->maxDeviation->onRestore();
    ui->maxAngularDeflection->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettings3DViewPart::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettings3DViewPartImp.cpp"
