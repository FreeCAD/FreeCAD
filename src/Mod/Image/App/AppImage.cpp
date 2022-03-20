/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *   Jürgen Riegel 2002                                                    *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include "ImagePlane.h"


namespace Image {
class Module : public Py::ExtensionModule<Module>
{
public:
    Module() : Py::ExtensionModule<Module>("Image")
    {
        initialize("This module is the Image module."); // register with Python
    }

    virtual ~Module() {}

private:
};

PyObject* initModule()
{
    return Base::Interpreter().addModule(new Module);
}

} // namespace Image

/* Python entry */
PyMOD_INIT_FUNC(Image)
{
    PyObject* mod = Image::initModule();
    Base::Console().Log("Loading Image module... done\n");

    Image::ImagePlane::init();

    PyMOD_Return(mod);
}
