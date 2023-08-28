#! python
# -*- coding: utf-8 -*-
# (c) 2007 Juergen Riegel

from . import template, templateModuleAppMain, templateModuleAppFeature
import generateBase.generateModel_Module
import generateBase.generateTools


class TemplateModuleApp(template.ModelTemplate):
    def Generate(self):
        AppPath = self.path + "/App/"
        generateBase.generateTools.ensureDir(AppPath)

        # the main module files
        AppMain = templateModuleAppMain.TemplateModuleAppMain()
        AppMain.path = AppPath
        AppMain.module = self.module
        AppMain.Generate()

        # Features
        generateBase.generateTools.ensureDir(AppPath + "Features/")
        for i in self.module.Content.Feature:
            AppFeature = templateModuleAppFeature.TemplateFeature()
            AppFeature.path = AppPath + "Features/"
            AppFeature.module = self.module
            AppFeature.feature = i
            AppFeature.Generate()
