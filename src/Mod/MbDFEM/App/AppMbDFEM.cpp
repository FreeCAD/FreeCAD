// SPDX-License-Identifier: LGPL-2.1-or-later

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include "MbDAssembly.h"
#include "MbDGroup.h"
#include "MbDMarker.h"
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
    PyObject* module = MbDFEM::initModule();
    MbDFEM::MbDAssembly::init();
    MbDFEM::MbDPart::init();
    MbDFEM::MbDMarker::init();
    MbDFEM::MbDGroup::init();
    Base::Console().log("Loading MbDFEM module... done\n");
    PyMOD_Return(module);
}
