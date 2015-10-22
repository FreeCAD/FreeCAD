#! python
# -*- coding: utf-8 -*-
# (c) 2006 Juergen Riegel 

import template, templateModuleApp,templateModuleGui
import generateBase.generateModel_Module

class TemplateModule (template.ModelTemplate):
    def Generate(self):
        print "generateBase.generateModel_Module.Generate()\n"
        App= templateModuleApp.TemplateModuleApp()
        App.path   = self.path 
        App.module = self.module
        App.Generate()

        

