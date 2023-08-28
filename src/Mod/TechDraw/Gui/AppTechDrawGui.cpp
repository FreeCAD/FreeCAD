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
#include <QFontDatabase>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/Language/Translator.h>
#include <Gui/WidgetFactory.h>

#include "DlgPrefsTechDrawAdvancedImp.h"
#include "DlgPrefsTechDrawAnnotationImp.h"
#include "DlgPrefsTechDrawColorsImp.h"
#include "DlgPrefsTechDrawDimensionsImp.h"
#include "DlgPrefsTechDrawGeneralImp.h"
#include "DlgPrefsTechDrawHLRImp.h"
#include "DlgPrefsTechDrawScaleImp.h"
#include "MDIViewPage.h"
#include "ViewProviderAnnotation.h"
#include "ViewProviderBalloon.h"
#include "ViewProviderCosmeticExtension.h"
#include "ViewProviderDimension.h"
#include "ViewProviderDrawingView.h"
#include "ViewProviderDrawingViewExtension.h"
#include "ViewProviderGeomHatch.h"
#include "ViewProviderHatch.h"
#include "ViewProviderImage.h"
#include "ViewProviderLeader.h"
#include "ViewProviderPage.h"
#include "ViewProviderPageExtension.h"
#include "ViewProviderProjGroup.h"
#include "ViewProviderProjGroupItem.h"
#include "ViewProviderRichAnno.h"
#include "ViewProviderSpreadsheet.h"
#include "ViewProviderSymbol.h"
#include "ViewProviderTemplate.h"
#include "ViewProviderTemplateExtension.h"
#include "ViewProviderTile.h"
#include "ViewProviderViewClip.h"
#include "ViewProviderViewPart.h"
#include "ViewProviderViewSection.h"
#include "ViewProviderWeld.h"
#include "Workbench.h"


// use a different name to CreateCommand()
void CreateTechDrawCommands();
void CreateTechDrawCommandsDims();
void CreateTechDrawCommandsDecorate();
void CreateTechDrawCommandsAnnotate();
void CreateTechDrawCommandsExtensionDims();
void CreateTechDrawCommandsExtensions();
void CreateTechDrawCommandsStack();

void loadTechDrawResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(TechDraw);
    Q_INIT_RESOURCE(TechDraw_translation);
    Gui::Translator::instance()->refresh();

    // add fonts
    std::string fontDir = App::Application::getResourceDir() + "Mod/TechDraw/Resources/fonts/";

    std::vector<std::string> fontsAll(
        {"osifont-lgpl3fe.ttf", "osifont-italic.ttf", "Y14.5-2018.ttf", "Y14.5-FreeCAD.ttf"});

    for (auto& font : fontsAll) {
        QString fontFile = Base::Tools::fromStdString(fontDir + font);
        int rc = QFontDatabase::addApplicationFont(fontFile);
        if (rc < 0) {
            Base::Console().Warning(
                "TechDraw failed to load font file: %d from: %s\n", rc, qPrintable(fontFile));
        }
    }
}

namespace TechDrawGui
{
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
    catch (const Base::Exception& e) {
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
    CreateTechDrawCommandsStack();

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

    TechDrawGui::ViewProviderPageExtension ::init();
    //    TechDrawGui::ViewProviderPageExtensionPython::init();
    TechDrawGui::ViewProviderDrawingViewExtension::init();
    TechDrawGui::ViewProviderTemplateExtension::init();

    TechDrawGui::ViewProviderCosmeticExtension::init();

    // register preferences pages
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawGeneralImp>(QT_TRANSLATE_NOOP("QObject", "TechDraw"));   //General
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawScaleImp>(QT_TRANSLATE_NOOP("QObject", "TechDraw"));     //Scale
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawDimensionsImp>(QT_TRANSLATE_NOOP("QObject", "TechDraw"));//Dimensions
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawAnnotationImp>(QT_TRANSLATE_NOOP("QObject", "TechDraw"));//Annotation
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawColorsImp>(QT_TRANSLATE_NOOP("QObject", "TechDraw"));    //Colors
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawHLRImp>(QT_TRANSLATE_NOOP("QObject", "TechDraw"));       //HLR
    new Gui::PrefPageProducer<TechDrawGui::DlgPrefsTechDrawAdvancedImp>(QT_TRANSLATE_NOOP("QObject", "TechDraw"));  //Advanced

    // add resources and reloads the translators
    loadTechDrawResource();

    PyMOD_Return(mod);
}
