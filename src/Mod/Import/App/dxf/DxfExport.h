#ifndef DXFEXPORT_H
#define DXFEXPORT_H

#include <Mod/Import/ImportGlobal.h>  // For the ImportExport macro
#include <Base/PyObjectBase.h>        // For PyObject

// Forward declare the writer class to avoid including its full header here
namespace Import
{
class ImpExpDxfWrite;
}

namespace Import
{
/**
 * The core, non-GUI DXF export logic. This function is exported from the
 * App module to be shared with the Gui module.
 */
ImportExport void
executeDxfExport(PyObject* objectList, ImpExpDxfWrite& writer, PyObject* helperModule);
}  // namespace Import

#endif  // DXFEXPORT_H
