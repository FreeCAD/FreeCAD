/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   J�rgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_math.hxx>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/WidgetFactory.h>

#include <Mod/Part/App/PropertyTopoShape.h>

#include "SoBrepFaceSet.h"
#include "SoBrepEdgeSet.h"
#include "SoBrepPointSet.h"
#include "SoFCShapeObject.h"
#include "ViewProvider.h"
#include "ViewProviderExt.h"
#include "ViewProviderPython.h"
#include "ViewProviderBox.h"
#include "ViewProviderCurveNet.h"
#include "ViewProviderImport.h"
#include "ViewProviderExtrusion.h"
#include "ViewProvider2DObject.h"
#include "ViewProviderMirror.h"
#include "ViewProviderBoolean.h"
#include "ViewProviderCompound.h"
#include "ViewProviderCircleParametric.h"
#include "ViewProviderLineParametric.h"
#include "ViewProviderPointParametric.h"
#include "ViewProviderEllipseParametric.h"
#include "ViewProviderHelixParametric.h"
#include "ViewProviderPlaneParametric.h"
#include "ViewProviderSphereParametric.h"
#include "ViewProviderCylinderParametric.h"
#include "ViewProviderConeParametric.h"
#include "ViewProviderTorusParametric.h"
#include "ViewProviderRuledSurface.h"
#include "ViewProviderPrism.h"
#include "ViewProviderSpline.h"
#include "ViewProviderRegularPolygon.h"
#include "TaskDimension.h"
#include "DlgSettingsGeneral.h"
#include "DlgSettingsObjectColor.h"
#include "DlgSettings3DViewPartImp.h"
#include "Workbench.h"

#include <Gui/Language/Translator.h>

#include "Resources/icons/PartFeature.xpm"
#include "Resources/icons/PartFeatureImport.xpm"

// use a different name to CreateCommand()
void CreatePartCommands(void);
void CreateSimplePartCommands(void);
void CreateParamPartCommands(void);

void loadPartResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Part);
    Gui::Translator::instance()->refresh();
}

/* registration table  */
static struct PyMethodDef PartGui_methods[] = {
    {NULL, NULL}                   /* end of table marker */
};

extern "C" {
void PartGuiExport initPartGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }

    // load needed modules
    try {
        Base::Interpreter().runString("import Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }

    (void) Py_InitModule("PartGui", PartGui_methods);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Part module... done\n");

    PartGui::SoBrepFaceSet                  ::initClass();
    PartGui::SoBrepEdgeSet                  ::initClass();
    PartGui::SoBrepPointSet                 ::initClass();
    PartGui::SoFCControlPoints              ::initClass();
    PartGui::ViewProviderPartBase           ::init();
    PartGui::ViewProviderPartExt            ::init();
    PartGui::ViewProviderPart               ::init();
    PartGui::ViewProviderEllipsoid          ::init();
    PartGui::ViewProviderPython             ::init();
    PartGui::ViewProviderBox                ::init();
    PartGui::ViewProviderPrism              ::init();
    PartGui::ViewProviderRegularPolygon     ::init();
    PartGui::ViewProviderWedge              ::init();
    PartGui::ViewProviderImport             ::init();
    PartGui::ViewProviderCurveNet           ::init();
    PartGui::ViewProviderExtrusion          ::init();
    PartGui::ViewProvider2DObject           ::init();
    PartGui::ViewProvider2DObjectPython     ::init();
    PartGui::ViewProviderMirror             ::init();
    PartGui::ViewProviderFillet             ::init();
    PartGui::ViewProviderChamfer            ::init();
    PartGui::ViewProviderRevolution         ::init();
    PartGui::ViewProviderLoft               ::init();
    PartGui::ViewProviderSweep              ::init();
    PartGui::ViewProviderOffset             ::init();
    PartGui::ViewProviderThickness          ::init();
    PartGui::ViewProviderCustom             ::init();
    PartGui::ViewProviderCustomPython       ::init();
    PartGui::ViewProviderBoolean            ::init();
    PartGui::ViewProviderMultiFuse          ::init();
    PartGui::ViewProviderMultiCommon        ::init();
    PartGui::ViewProviderCompound           ::init();
    PartGui::ViewProviderSpline             ::init();
    PartGui::ViewProviderCircleParametric   ::init();
    PartGui::ViewProviderLineParametric     ::init();
    PartGui::ViewProviderPointParametric    ::init();
    PartGui::ViewProviderEllipseParametric  ::init();
    PartGui::ViewProviderHelixParametric    ::init();
    PartGui::ViewProviderSpiralParametric   ::init();
    PartGui::ViewProviderPlaneParametric    ::init();
    PartGui::ViewProviderSphereParametric   ::init();
    PartGui::ViewProviderCylinderParametric ::init();
    PartGui::ViewProviderConeParametric     ::init();
    PartGui::ViewProviderTorusParametric    ::init();
    PartGui::ViewProviderRuledSurface       ::init();
    PartGui::DimensionLinear                ::initClass();
    PartGui::DimensionAngular               ::initClass();
    PartGui::ArcEngine                      ::initClass();

    PartGui::Workbench                      ::init();

    // instantiating the commands
    CreatePartCommands();
    CreateSimplePartCommands();
    CreateParamPartCommands();

    // register preferences pages
    (void)new Gui::PrefPageProducer<PartGui::DlgSettingsGeneral>      ( QT_TRANSLATE_NOOP("QObject","Part design") );
    (void)new Gui::PrefPageProducer<PartGui::DlgSettings3DViewPart>   ( QT_TRANSLATE_NOOP("QObject","Part design") );
    (void)new Gui::PrefPageProducer<PartGui::DlgSettingsObjectColor>  ( QT_TRANSLATE_NOOP("QObject","Display") );
    Gui::ViewProviderBuilder::add(
        Part::PropertyPartShape::getClassTypeId(),
        PartGui::ViewProviderPart::getClassTypeId());

    // add resources and reloads the translators
    loadPartResource();

    // register bitmaps
    Gui::BitmapFactoryInst& rclBmpFactory = Gui::BitmapFactory();
    rclBmpFactory.addXPM("PartFeature",(const char**) PartFeature_xpm);
    rclBmpFactory.addXPM("PartFeatureImport",(const char**) PartFeatureImport_xpm);
}
} // extern "C"
