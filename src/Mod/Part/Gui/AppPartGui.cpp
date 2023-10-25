/***************************************************************************
*   Copyright (c) 2002 Juergen Riegel <juergen.riegel@web.de>             *
*                                                                         *
*   This file is part of the FreeCAD CAx development system.              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU Lesser General Public License (LGPL)    *
*   as published by the Free Software Foundation; either version 2 of     *
*   the License, or (at your option) any later version.                   *
*   for detail see the LICENCE text file.                                 *
*                                                                         *
*   FreeCAD is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU Lesser General Public License for more details.                   *
*                                                                         *
*   You should have received a copy of the GNU Library General Public     *
*   License along with FreeCAD; if not, write to the Free Software        *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
*   USA                                                                   *
*                                                                         *
***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_math.hxx>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/DlgPreferencesImp.h>
#include <Gui/WidgetFactory.h>
#include <Gui/Language/Translator.h>

#include "AttacherTexts.h"
#include "PropertyEnumAttacherItem.h"
#include "DlgSettings3DViewPartImp.h"
#include "DlgSettingsGeneral.h"
#include "DlgSettingsMeasure.h"
#include "DlgSettingsObjectColor.h"
#include "TaskDimension.h"
#include "SoBrepEdgeSet.h"
#include "SoBrepFaceSet.h"
#include "SoBrepPointSet.h"
#include "SoFCShapeObject.h"
#include "ViewProvider.h"
#include "ViewProvider2DObject.h"
#include "ViewProviderAttachExtension.h"
#include "ViewProviderGridExtension.h"
#include "ViewProviderBoolean.h"
#include "ViewProviderBox.h"
#include "ViewProviderCircleParametric.h"
#include "ViewProviderCompound.h"
#include "ViewProviderConeParametric.h"
#include "ViewProviderCurveNet.h"
#include "ViewProviderCylinderParametric.h"
#include "ViewProviderEllipseParametric.h"
#include "ViewProviderExt.h"
#include "ViewProviderExtrusion.h"
#include "ViewProviderScale.h"
#include "ViewProviderHelixParametric.h"
#include "ViewProviderPrimitive.h"
#include "ViewProviderPython.h"
#include "ViewProviderImport.h"
#include "ViewProviderLineParametric.h"
#include "ViewProviderMirror.h"
#include "ViewProviderPlaneParametric.h"
#include "ViewProviderPointParametric.h"
#include "ViewProviderPrism.h"
#include "ViewProviderRegularPolygon.h"
#include "ViewProviderRuledSurface.h"
#include "ViewProviderSphereParametric.h"
#include "ViewProviderSpline.h"
#include "ViewProviderTorusParametric.h"
#include "Workbench.h"
#include "WorkbenchManipulator.h"


// use a different name to CreateCommand()
void CreatePartCommands();
void CreateSimplePartCommands();
void CreateParamPartCommands();
void CreatePartSelectCommands();

void loadPartResource()
{
    // add resources and reloads the translators
    Q_INIT_RESOURCE(Part);
    Q_INIT_RESOURCE(Part_translation);
    Gui::Translator::instance()->refresh();
}

namespace PartGui {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("PartGui")
    {
        initialize("This module is the PartGui module."); // register with Python
    }

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);;
}

} // namespace PartGui

PyMOD_INIT_FUNC(PartGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        PyMOD_Return(nullptr);
    }

    // load needed modules
    try {
        Base::Interpreter().runString("import Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* partGuiModule = PartGui::initModule();

    Base::Console().Log("Loading GUI of Part module... done\n");

    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/booleans"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/create"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/measure"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/parametric"));
    Gui::BitmapFactory().addPath(QString::fromLatin1(":/icons/tools"));

    static struct PyModuleDef pAttachEngineTextsModuleDef = {
        PyModuleDef_HEAD_INIT,
        "AttachEngineResources",
        "AttachEngineResources", -1,
        AttacherGui::AttacherGuiPy::Methods,
        nullptr, nullptr, nullptr, nullptr
    };
    PyObject* pAttachEngineTextsModule = PyModule_Create(&pAttachEngineTextsModuleDef);

    Py_INCREF(pAttachEngineTextsModule);
    PyModule_AddObject(partGuiModule, "AttachEngineResources", pAttachEngineTextsModule);

    // clang-format off
    PartGui::PropertyEnumAttacherItem               ::init();
    PartGui::SoBrepFaceSet                          ::initClass();
    PartGui::SoBrepEdgeSet                          ::initClass();
    PartGui::SoBrepPointSet                         ::initClass();
    PartGui::SoFCControlPoints                      ::initClass();
    PartGui::ViewProviderAttachExtension            ::init();
    PartGui::ViewProviderAttachExtensionPython      ::init();
    PartGui::ViewProviderGridExtension              ::init();
    PartGui::ViewProviderGridExtensionPython        ::init();
    PartGui::ViewProviderSplineExtension            ::init();
    PartGui::ViewProviderSplineExtensionPython      ::init();
    PartGui::ViewProviderPartExt                    ::init();
    PartGui::ViewProviderPart                       ::init();
    PartGui::ViewProviderPrimitive                  ::init();
    PartGui::ViewProviderEllipsoid                  ::init();
    PartGui::ViewProviderPython                     ::init();
    PartGui::ViewProviderBox                        ::init();
    PartGui::ViewProviderPrism                      ::init();
    PartGui::ViewProviderRegularPolygon             ::init();
    PartGui::ViewProviderWedge                      ::init();
    PartGui::ViewProviderImport                     ::init();
    PartGui::ViewProviderCurveNet                   ::init();
    PartGui::ViewProviderExtrusion                  ::init();
    PartGui::ViewProviderScale                      ::init();
    PartGui::ViewProvider2DObject                   ::init();
    PartGui::ViewProvider2DObjectPython             ::init();
    PartGui::ViewProvider2DObjectGrid               ::init();
    PartGui::ViewProviderMirror                     ::init();
    PartGui::ViewProviderFillet                     ::init();
    PartGui::ViewProviderChamfer                    ::init();
    PartGui::ViewProviderRevolution                 ::init();
    PartGui::ViewProviderLoft                       ::init();
    PartGui::ViewProviderSweep                      ::init();
    PartGui::ViewProviderOffset                     ::init();
    PartGui::ViewProviderOffset2D                   ::init();
    PartGui::ViewProviderThickness                  ::init();
    PartGui::ViewProviderRefine                     ::init();
    PartGui::ViewProviderReverse                    ::init();
    PartGui::ViewProviderCustom                     ::init();
    PartGui::ViewProviderCustomPython               ::init();
    PartGui::ViewProviderBoolean                    ::init();
    PartGui::ViewProviderMultiFuse                  ::init();
    PartGui::ViewProviderMultiCommon                ::init();
    PartGui::ViewProviderCompound                   ::init();
    PartGui::ViewProviderSpline                     ::init();
    PartGui::ViewProviderCircleParametric           ::init();
    PartGui::ViewProviderLineParametric             ::init();
    PartGui::ViewProviderPointParametric            ::init();
    PartGui::ViewProviderEllipseParametric          ::init();
    PartGui::ViewProviderHelixParametric            ::init();
    PartGui::ViewProviderSpiralParametric           ::init();
    PartGui::ViewProviderPlaneParametric            ::init();
    PartGui::ViewProviderSphereParametric           ::init();
    PartGui::ViewProviderCylinderParametric         ::init();
    PartGui::ViewProviderConeParametric             ::init();
    PartGui::ViewProviderTorusParametric            ::init();
    PartGui::ViewProviderRuledSurface               ::init();
    PartGui::ViewProviderFace                       ::init();
    PartGui::DimensionLinear                        ::initClass();
    PartGui::DimensionAngular                       ::initClass();
    PartGui::ArcEngine                              ::initClass();

    PartGui::Workbench                              ::init();
    auto manip = std::make_shared<PartGui::WorkbenchManipulator>();
    Gui::WorkbenchManipulator::installManipulator(manip);
    // clang-format on

    // instantiating the commands
    CreatePartCommands();
    CreateSimplePartCommands();
    CreateParamPartCommands();
    CreatePartSelectCommands();
    try{
        Py::Object ae = Base::Interpreter().runStringObject("__import__('AttachmentEditor.Commands').Commands");
        Py::Module(partGuiModule).setAttr(std::string("AttachmentEditor"),ae);
    } catch (Base::PyException &err){
        err.ReportException();
    }

    // register preferences pages
    Gui::Dialog::DlgPreferencesImp::setGroupData("Part/Part Design", "Part design", QObject::tr("Part and Part Design workbench"));
    (void)new Gui::PrefPageProducer<PartGui::DlgSettingsGeneral>(QT_TRANSLATE_NOOP("QObject", "Part/Part Design"));
    (void)new Gui::PrefPageProducer<PartGui::DlgSettings3DViewPart>(QT_TRANSLATE_NOOP("QObject", "Part/Part Design"));
    (void)new Gui::PrefPageProducer<PartGui::DlgSettingsObjectColor>(QT_TRANSLATE_NOOP("QObject", "Part/Part Design"));
    (void)new Gui::PrefPageProducer<PartGui::DlgSettingsMeasure>(QT_TRANSLATE_NOOP("QObject", "Part/Part Design"));
    (void)new Gui::PrefPageProducer<PartGui::DlgImportExportIges>(QT_TRANSLATE_NOOP("QObject", "Import-Export"));
    (void)new Gui::PrefPageProducer<PartGui::DlgImportExportStep>(QT_TRANSLATE_NOOP("QObject", "Import-Export"));
    Gui::ViewProviderBuilder::add(
        Part::PropertyPartShape::getClassTypeId(),
        PartGui::ViewProviderPart::getClassTypeId());

    // add resources and reloads the translators
    loadPartResource();

    // register bitmaps
    // Gui::BitmapFactoryInst& rclBmpFactory = Gui::BitmapFactory();
    // rclBmpFactory.addXPM("Part_Feature",(const char**) PartFeature_xpm);
    // rclBmpFactory.addXPM("Part_FeatureImport",(const char**) PartFeatureImport_xpm);

    PyMOD_Return(partGuiModule);
}
