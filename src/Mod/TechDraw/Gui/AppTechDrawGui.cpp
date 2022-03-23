/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QFontDatabase>
#endif

#include <Base/Console.h>
#include <Base/PyObjectBase.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>

#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>

#include "Workbench.h"
#include "MDIViewPage.h"

#include "DlgPrefsTechDrawGeneralImp.h"
#include "DlgPrefsTechDrawScaleImp.h"
#include "DlgPrefsTechDrawAnnotationImp.h"
#include "DlgPrefsTechDrawDimensionsImp.h"
#include "DlgPrefsTechDrawColorsImp.h"
#include "DlgPrefsTechDrawAdvancedImp.h"
#include "DlgPrefsTechDrawHLRImp.h"
#include "ViewProviderPage.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderDimension.h"
#include "ViewProviderBalloon.h"
#include "ViewProviderProjGroup.h"
#include "ViewProviderProjGroupItem.h"
#include "ViewProviderTemplate.h"
#include "ViewProviderViewPart.h"
#include "ViewProviderViewSection.h"
#include "ViewProviderAnnotation.h"
#include "ViewProviderSymbol.h"
#include "ViewProviderViewClip.h"
#include "ViewProviderHatch.h"
#include "ViewProviderGeomHatch.h"
#include "ViewProviderSpreadsheet.h"
#include "ViewProviderImage.h"
#include "ViewProviderRichAnno.h"
#include "ViewProviderLeader.h"
#include "ViewProviderTile.h"
#include "ViewProviderWeld.h"

#include "ViewProviderCosmeticExtension.h"


// use a different name to CreateCommand()
void CreateTechDrawCommands(void);
void CreateTechDrawCommandsDims(void);
void CreateTechDrawCommandsDecorate(void);
void CreateTechDrawCommandsAnnotate(void);
void CreateTechDrawCommandsExtensionDims(void);
void CreateTechDrawCommandsExtensions(void);

void loadTechDrawResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(TechDraw);
    Gui::Translator::instance()->refresh();

    // add osifont
    std::string fontDir = App::Application::getResourceDir() + "Mod/TechDraw/Resources/fonts/";
    QString fontFile = Base::Tools::fromStdString(fontDir + "osifont-lgpl3fe.ttf");
    QFontDatabase fontDB;
    int rc = fontDB.addApplicationFont(fontFile);
    if (rc) {
        Base::Console().Log("TechDraw failed to load osifont file: %d from: %s\n",rc,qPrintable(fontFile));
    }
}

namespace TechDrawGui {
    extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(TechDrawGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }
    // load dependent module
    try {
        Base::Interpreter().loadModule("TechDraw");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    PyObject* mod = TechDrawGui::initModule();

    Base::Console().Log("Loading TechDrawGui module... done\n");

    // instantiating the commands
    CreateTechDrawCommands();
    CreateTechDrawCommandsDims();
    CreateTechDrawCommandsDecorate();
    CreateTechDrawCommandsAnnotate();
    CreateTechDrawCommandsExtensionDims();
    CreateTechDrawCommandsExtensions();

    TechDrawGui::Workbench::init();
    TechDrawGui::MDIViewPage::init();
    TechDrawGui::MDIViewPagePy::init_type();

    TechDrawGui::ViewProviderPage::init();
    TechDrawGui::ViewProviderDrawingView::init();

    TechDrawGui::ViewProviderTemplate::init();
    TechDrawGui::ViewProviderDimension::init();
    TechDrawGui::ViewProviderBalloon::init();
    TechDrawGui::ViewProviderViewPart::init();
    TechDrawGui::ViewProviderProjGroupItem::init();
    TechDrawGui::ViewProviderProjGroup::init();
    TechDrawGui::ViewProviderViewSection::init();
    TechDrawGui::ViewProviderViewClip::init();
    TechDrawGui::ViewProviderAnnotation::init();
    TechDrawGui::ViewProviderSymbol::init();
    TechDrawGui::ViewProviderDraft::init();
    TechDrawGui::ViewProviderArch::init();
    TechDrawGui::ViewProviderHatch::init();
    TechDrawGui::ViewProviderGeomHatch::init();
    TechDrawGui::ViewProviderSpreadsheet::init();
    TechDrawGui::ViewProviderImage::init();
    TechDrawGui::ViewProviderLeader::init();
    TechDrawGui::ViewProviderRichAnno::init();
    TechDrawGui::ViewProviderTile::init();
    TechDrawGui::ViewProviderWeld::init();

    TechDrawGui::ViewProviderCosmeticExtension::init();

    // register preferences pages
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawGeneralImp> ("TechDraw");    //General
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawScaleImp> ("TechDraw");      //Scale
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawDimensionsImp>("TechDraw");  //Dimensions
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawAnnotationImp> ("TechDraw"); //Annotation
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawColorsImp>("TechDraw");      //Colors
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawHLRImp> ("TechDraw");        //HLR
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawAdvancedImp> ("TechDraw");   //Advanced

    // add resources and reloads the translators
    loadTechDrawResource();

    PyMOD_Return(mod);
}
