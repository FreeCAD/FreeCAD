// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>
#include <Gui/Application.h>

#include "ViewProviderMbDAction.h"
#include "ViewProviderMbDAssembly.h"
#include "ViewProviderMbDItemIJ.h"
#include "ViewProviderMbDJoint.h"
#include "ViewProviderMbDMarker.h"
#include "ViewProviderMbDMotion.h"
#include "ViewProviderMbDPart.h"
#include "ViewProviderMbDGravity.h"

namespace MbDFEMGui
{

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("MbDFEMGui")
    {
        initialize("The MbDFEMGui module.");
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace MbDFEMGui

PyMOD_INIT_FUNC(MbDFEMGui)
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load MbDFEMGui in console application.");
        PyMOD_Return(nullptr);
    }

    // load dependent GUI module
    try {
        Base::Interpreter().runString("import PartGui");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    MbDFEMGui::ViewProviderMbDAssembly::init();
    MbDFEMGui::ViewProviderMbDPart::init();
    MbDFEMGui::ViewProviderMbDMarker::init();
    MbDFEMGui::ViewProviderMbDItemIJ::init();
    MbDFEMGui::ViewProviderMbDJoint::init();
    MbDFEMGui::ViewProviderMbDMotion::init();
    MbDFEMGui::ViewProviderMbDAction::init();
    MbDFEMGui::ViewProviderMbDGravity::init();

    PyObject* module = MbDFEMGui::initModule();
    Base::Console().log("Loading GUI of MbDFEM module... done\n");
    PyMOD_Return(module);
}
