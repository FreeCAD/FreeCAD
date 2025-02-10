#! python
# -*- coding: utf-8 -*-
# (c) 2006 Juergen Riegel

from . import template, templateModuleApp


class TemplateModule(template.ModelTemplate):
    def Generate(self):
        print("model.generateModel_Module.Generate()\n")
        App = templateModuleApp.TemplateModuleApp()
        App.outputDir = self.outputDir
        App.module = self.module
        App.Generate()
