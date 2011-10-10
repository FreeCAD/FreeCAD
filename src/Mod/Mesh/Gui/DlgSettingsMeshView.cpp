/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "DlgSettingsMeshView.h"
#include "ViewProvider.h"
#include <Gui/PrefWidgets.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>

using namespace MeshGui;

/**
 *  Constructs a DlgSettingsMeshView which is a child of 'parent'.
 */
DlgSettingsMeshView::DlgSettingsMeshView(QWidget* parent)
  : PreferencePage(parent)
{
    this->setupUi(this);
    labelBackfaceColor->hide();
    buttonBackfaceColor->hide();
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsMeshView::~DlgSettingsMeshView()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsMeshView::saveSettings()
{
    checkboxRendering->onSave();
    checkboxBoundbox->onSave();
    buttonMeshColor->onSave();
    buttonLineColor->onSave();
    buttonBackfaceColor->onSave();
    spinMeshTransparency->onSave();
    spinLineTransparency->onSave();
    checkboxNormal->onSave();
    spinboxAngle->onSave();

    bool twoside = checkboxRendering->isChecked();
    double angle = 0.0;
    if (checkboxNormal->isChecked()) {
        angle = spinboxAngle->value();
    }

    // search for Mesh view providers and apply the new settings
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    for (std::vector<App::Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
        Gui::Document* doc = Gui::Application::Instance->getDocument(*it);
        std::vector<Gui::ViewProvider*> views = doc->getViewProvidersOfType(ViewProviderMesh::getClassTypeId());
        for (std::vector<Gui::ViewProvider*>::iterator jt = views.begin(); jt != views.end(); ++jt) {
            ViewProviderMesh* meshview = static_cast<ViewProviderMesh*>(*jt);
            if (twoside) meshview->Lighting.setValue(1);
            else meshview->Lighting.setValue((long)0);
            meshview->CreaseAngle.setValue(angle);
        }
    }
}

void DlgSettingsMeshView::loadSettings()
{
    Base::Reference<ParameterGrp> hGrp = Gui::WindowParameter::getDefaultParameter();
    hGrp = hGrp->GetGroup("View");
    if (!hGrp->GetBool("EnablePreselection",true) &&
        !hGrp->GetBool("EnableSelection",true))
        checkboxBoundbox->setDisabled(true);
    checkboxRendering->onRestore();
    checkboxBoundbox->onRestore();
    buttonMeshColor->onRestore();
    buttonLineColor->onRestore();
    buttonBackfaceColor->onRestore();
    spinMeshTransparency->onRestore();
    spinLineTransparency->onRestore();
    checkboxNormal->onRestore();
    spinboxAngle->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsMeshView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsMeshView.cpp"
