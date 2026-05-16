// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Python extension module "DraftGuiExt".
 *
 * Importing this module registers a static DraftWPPickProvider with
 * Gui::TransientPlacementTargetRegistry so that TaskTransform can snap
 * the dragger to the Draft Working Plane grid.
 *
 * The provider is automatically deregistered when the module is unloaded
 * (i.e. when the shared library is closed at FreeCAD shutdown).
 */

#include <Base/Console.h>
#include <Base/PyObjectBase.h>

#include <Gui/TransientPlacementTargetProvider.h>

#include "DraftWPPickProvider.h"

namespace
{

/**
 * RAII guard that registers the provider on construction and removes it
 * on destruction, ensuring clean teardown when the .so is unloaded.
 */
struct ProviderGuard
{
    DraftGui::DraftWPPickProvider provider;

    ProviderGuard()
    {
        Gui::TransientPlacementTargetRegistry::instance().add(&provider);
    }
    ~ProviderGuard()
    {
        Gui::TransientPlacementTargetRegistry::instance().remove(&provider);
    }
};

// Constructed the first time DraftGuiExt is imported; destroyed when the
// shared library is unloaded.
ProviderGuard gProviderGuard;

}  // namespace

PyMOD_INIT_FUNC(DraftGuiExt)
{
    static struct PyModuleDef moduleDef = {
        PyModuleDef_HEAD_INIT,
        "DraftGuiExt",
        "Draft GUI extensions for Std_TransformManip (internal).",
        -1,
        nullptr
    };

    PyObject* mod = PyModule_Create(&moduleDef);
    Base::Console().log("Loading DraftGuiExt module... done\n");
    PyMOD_Return(mod);
}
