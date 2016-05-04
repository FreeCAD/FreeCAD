/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
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
# include <Python.h>
# include <QFontDatabase>
#endif

#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>

#include "Workbench.h"

#include "DlgPrefsTechDrawImp.h"
#include "ViewProviderPage.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderDimension.h"
#include "ViewProviderProjGroup.h"
#include "ViewProviderProjGroupItem.h"
#include "ViewProviderTemplate.h"
#include "ViewProviderViewPart.h"
#include "ViewProviderViewSection.h"
#include "ViewProviderAnnotation.h"
#include "ViewProviderSymbol.h"
#include "ViewProviderViewClip.h"
#include "ViewProviderHatch.h"
#include "ViewProviderSpreadsheet.h"
//#include "resources/qrc_TechDraw.cpp"

// use a different name to CreateCommand()
void CreateTechDrawCommands(void);
void CreateTechDrawCommandsDims(void);
void CreateTechDrawCommandsDecorate(void);

void loadTechDrawResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(TechDraw);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
extern struct PyMethodDef TechDrawGui_Import_methods[];


/* Python entry */
extern "C" {
void TechDrawGuiExport initTechDrawGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    (void) Py_InitModule("TechDrawGui", TechDrawGui_Import_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of TechDraw module... done\n");

    // instantiating the commands
    CreateTechDrawCommands();
    CreateTechDrawCommandsDims();
    CreateTechDrawCommandsDecorate();

    TechDrawGui::Workbench::init();

    //TODO: is this platform independent?? should osifont be added by installer?
    //Load the osifont for Drawing View
    // See https://code.google.com/p/osifont/
    QFontDatabase fontDB;
    fontDB.addApplicationFont(QString::fromAscii(":/fonts/osifont.ttf"));

    TechDrawGui::ViewProviderPage::init();
    TechDrawGui::ViewProviderDrawingView::init();

    TechDrawGui::ViewProviderTemplate::init();
    TechDrawGui::ViewProviderDimension::init();
    TechDrawGui::ViewProviderViewPart::init();
    TechDrawGui::ViewProviderProjGroupItem::init();
    TechDrawGui::ViewProviderProjGroup::init();
    TechDrawGui::ViewProviderViewSection::init();
    TechDrawGui::ViewProviderViewClip::init();
    TechDrawGui::ViewProviderAnnotation::init();
    TechDrawGui::ViewProviderSymbol::init();
    TechDrawGui::ViewProviderHatch::init();
    TechDrawGui::ViewProviderSpreadsheet::init();

    // register preferences pages
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawImp> ("TechDraw");

    // add resources and reloads the translators
    loadTechDrawResource();
}

} // extern "C" {
