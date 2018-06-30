
#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>

#include "DrawViewSymbol.h"
#include "DrawView.h"

// inclusion of the generated files
#include <Mod/TechDraw/App/DrawViewPy.h>
#include <Mod/TechDraw/App/DrawViewSymbolPy.h>
#include <Mod/TechDraw/App/DrawViewSymbolPy.cpp>

using namespace TechDraw;

// returns a string which represents the object e.g. when printed in python
std::string DrawViewSymbolPy::representation(void) const
{
    return std::string("<DrawViewSymbol object>");
}

PyObject* DrawViewSymbolPy::dumpSymbol(PyObject *args)
{
    const char* fileSpec;
    if (!PyArg_ParseTuple(args, "s", &fileSpec)) {
       throw Py::TypeError("** dumpSymbol bad args.");
    }
    auto dvs = getDrawViewSymbolPtr();
    std::string symbolRepr;
    if (dvs != nullptr) {
        symbolRepr = dvs->Symbol.getValue();
    }

    Base::FileInfo fi(fileSpec);
    std::ofstream outfile;
    outfile.open(fi.filePath());
    outfile.write (symbolRepr.c_str(),symbolRepr.size());
    outfile.close();
    if (outfile.good()) {
        outfile.close();
    } else {
        std::string error = std::string("Can't write ");
        error += fileSpec;
        throw Py::RuntimeError(error);
    }
    Py_Return;
}

PyObject *DrawViewSymbolPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int DrawViewSymbolPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
