#! python
# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

# (c) 2006 Juergen Riegel

from . import template, templateModuleApp


class TemplateModule(template.ModelTemplate):
    def Generate(self):
        print("model.generateModel_Module.Generate()\n")
        App = templateModuleApp.TemplateModuleApp()
        App.outputDir = self.outputDir
        App.module = self.module
        App.Generate()
