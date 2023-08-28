#! python
# -*- coding: utf-8 -*-
# (c) 2006 Juergen Riegel

from . import template
import generateBase.generateModel_Module
import generateBase.generateTools


class TemplateModuleAppMain(template.ModelTemplate):
    def Generate(self):
        file = open(self.path + "/App" + self.module.Name + ".cpp", "w")
        generateBase.generateTools.replace(self.Template, locals(), file)
        # file.write( generateBase.generateTools.replace(self.Template,locals()))

    Template = """
/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   for detail see the LICENCE text file.                                 *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>

+ for i in self.module.Content.Feature:
#include "Feature/@i.Name@.h"
-

extern struct PyMethodDef @self.module.Name@_methods[];


extern "C" {
void App@self.module.Name@Export init@self.module.Name@() {

  Base::Console().Log("Mod: Loading @self.module.Name@ module... done\\n");
  PyObject* partModule = Py_InitModule3("@self.module.Name@", @self.module.Name@_methods, module_@self.module.Name@_doc);   /* mod name, table ptr */

+ for i in self.module.Content.Feature:
  @self.module.Name@::Feature@i.Name@::init();
-

    return;
}

} // extern "C"
"""
