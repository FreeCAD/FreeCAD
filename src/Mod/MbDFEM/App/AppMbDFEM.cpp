// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include "MbDAction.h"
#include "MbDAssembly.h"
#include "MbDFolders.h"
#include "MbDItemIJ.h"
#include "MbDJoint.h"
#include "MbDMarker.h"
#include "MbDMotion.h"
#include "MbDParameters.h"
#include "MbDPart.h"

namespace MbDFEM
{

class Module: public Py::ExtensionModule<Module>
{
public:
    Module()
        : Py::ExtensionModule<Module>("MbDFEM")
    {
        initialize("The MbDFEM module.");
    }
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

}  // namespace MbDFEM

PyMOD_INIT_FUNC(MbDFEM)
{
    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }

    PyObject* module = MbDFEM::initModule();
    MbDFEM::MbDAssembly::init();
    MbDFEM::MbDPart::init();
    MbDFEM::MbDMarker::init();
    MbDFEM::MbDItemIJ::init();
    MbDFEM::MbDJoint::init();
    MbDFEM::MbDMotion::init();
    MbDFEM::MbDAction::init();
    MbDFEM::MbDGravity::init();
    MbDFEM::MbDSimulationParameters::init();
    MbDFEM::MbDAnimationParameters::init();
    MbDFEM::MbDAssembliesFolder::init();
    MbDFEM::MbDPartsFolder::init();
    MbDFEM::MbDFixedPartsFolder::init();
    MbDFEM::MbDMarkersFolder::init();
    MbDFEM::MbDJointsFolder::init();
    MbDFEM::MbDMotionsFolder::init();
    MbDFEM::MbDActionsFolder::init();
    Base::Console().log("Loading MbDFEM module... done\n");
    PyMOD_Return(module);
}
