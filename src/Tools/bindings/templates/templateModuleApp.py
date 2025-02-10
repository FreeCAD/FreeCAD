#! python
# -*- coding: utf-8 -*-
# (c) 2007 Juergen Riegel

from . import template, templateModuleAppMain, templateModuleAppFeature
import model.generateModel_Module
import model.generateTools


class TemplateModuleApp(template.ModelTemplate):
    def Generate(self):
        AppPath = self.outputDir + "/App/"
        model.generateTools.ensureDir(AppPath)

        # the main module files
        AppMain = templateModuleAppMain.TemplateModuleAppMain()
        AppMain.outputDir = AppPath
        AppMain.module = self.module
        AppMain.Generate()

        # Features
        model.generateTools.ensureDir(AppPath + "Features/")
        for i in self.module.Content.Feature:
            AppFeature = templateModuleAppFeature.TemplateFeature()
            AppFeature.outputDir = AppPath + "Features/"
            AppFeature.module = self.module
            AppFeature.feature = i
            AppFeature.Generate()
