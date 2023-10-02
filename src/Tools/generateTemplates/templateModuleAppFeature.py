#! python
# -*- coding: utf-8 -*-
# (c) 2006 Juergen Riegel

from . import template
import generateBase.generateModel_Module
import generateBase.generateTools


class TemplateFeature(template.ModelTemplate):
    def Generate(self):
        file = open(self.path + self.feature.Name + "Imp.cpp", "w")
        generateBase.generateTools.replace(self.TemplateImplement, locals(), file)
        file = open(self.path + self.feature.Name + ".cpp", "w")
        generateBase.generateTools.replace(self.TemplateModule, locals(), file)
        file = open(self.path + self.feature.Name + ".h", "w")
        generateBase.generateTools.replace(self.TemplateHeader, locals(), file)
        # file.write( generateBase.generateTools.replace(self.Template,locals()))

    TemplateHeader = """
#ifndef @self.module.Name.upper()@_FEATURE_@self.feature.Name.upper()@_H
#define @self.module.Name.upper()@_FEATURE_@self.feature.Name.upper()@_H

#include <App/PropertyStandard.h>

#include <App/Feature.h>

namespace @self.module.Name@
{


class @self.feature.Name@ : public App::Feature
{
  PROPERTY_HEADER(@self.module.Name@::@self.feature.Name@);

public:
  @self.feature.Name@();
+ for i in self.feature.Property:
  @i.Type@ @i.Name@;
-

  /** @name Methods override feature */
  //@{
  /// Recalculate the feature
  virtual int execute(void);
  /// Return the type name of the ViewProvider
  virtual const char* getViewProviderName(void) const {
    return "@self.module.Name@Gui::ViewProviderBox";
  }
  //@}
};

} //namespace @self.module.Name@

#endif // @self.module.Name.upper()@_FEATURE_@self.feature.Name.upper()@_H

"""
    TemplateModule = """
#include "PreCompiled.h"

#include "@self.feature.Name@.h"

using namespace @self.module.Name@;

PROPERTY_SOURCE(@self.module.Name@::@self.feature.Name@, App::Feature)

@self.feature.Name@::@self.feature.Name@()
{
+ for i in self.feature.Property:
  ADD_PROPERTY(@i.Name@,(0.0));
-
}
"""
    # Here's the template for the user part of the implementation. This does NOT get overwritten if it already exists.
    TemplateImplement = """
//
#include "PreCompiled.h"

#include "@self.feature.Name@.h"

using namespace @self.module.Name@;

// TODO This method implements the function of the feature
int @self.feature.Name@::execute(void)
{
   return 0;
}

"""
