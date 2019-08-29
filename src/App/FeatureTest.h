/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2006     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/



#ifndef APP_FEATURETEST_H
#define APP_FEATURETEST_H


#include "DocumentObject.h"
#include "PropertyUnits.h"
#include "PropertyGeo.h"
#include "PropertyLinks.h"

namespace App
{

/// The testing feature
class FeatureTest : public DocumentObject
{
  PROPERTY_HEADER(App::FeatureTest);

public:
  FeatureTest();

  ~FeatureTest();

  // Standard Properties (PropertyStandard.h)
  App::PropertyInteger      Integer;
  App::PropertyFloat        Float;
  App::PropertyBool         Bool;
  App::PropertyBoolList     BoolList;
  App::PropertyString       String;
  App::PropertyPath         Path;
  App::PropertyStringList   StringList;

  App::PropertyColor        Colour;
  App::PropertyColorList    ColourList;
  App::PropertyMaterial     Material;
  App::PropertyMaterialList MaterialList;

  // special types
  App::PropertyDistance     Distance;
  App::PropertyAngle        Angle;

  // Constraint types
  App::PropertyEnumeration       Enum;
  App::PropertyIntegerConstraint ConstraintInt;
  App::PropertyFloatConstraint   ConstraintFloat;
 
  // Standard Properties (PrppertyStandard.h)
  App::PropertyIntegerList IntegerList;
  App::PropertyFloatList   FloatList;

  // Standard Properties (PropertyLinks.h)
  App::PropertyLink        Link;
  App::PropertyLinkSub     LinkSub;
  App::PropertyLinkList    LinkList;
  App::PropertyLinkSubList LinkSubList;

  // Standard Properties (PropertyGeo.h)
  App::PropertyMatrix     Matrix;
  App::PropertyVector     Vector;
  App::PropertyVectorList VectorList;
  App::PropertyPlacement  Placement;

  // Properties to test the Document::recompute()
  App::PropertyLink     Source1;
  App::PropertyLink     Source2;
  App::PropertyLinkList SourceN;
  App::PropertyString   ExecResult;
  App::PropertyInteger  ExceptionType;
  App::PropertyInteger  ExecCount;
  
  App::PropertyInteger   TypeHidden;
  App::PropertyInteger   TypeReadOnly;
  App::PropertyInteger   TypeOutput;
  App::PropertyInteger   TypeAll;
  App::PropertyInteger   TypeTransient;
  App::PropertyInteger   TypeNoRecompute;

  App::PropertyQuantity  QuantityLength;
  App::PropertyQuantity  QuantityOther;
  //App::PropertyQuantity  QuantityMass;
  //App::PropertyQuantity  QuantityAngle;

  /** @name methods override Feature */
  //@{
  virtual short mustExecute(void) const;
  /// recalculate the Feature
  virtual DocumentObjectExecReturn *execute(void);
  /// returns the type name of the ViewProvider
  //FIXME: Probably it makes sense to have a view provider for unittests (e.g. Gui::ViewProviderTest)
  virtual const char* getViewProviderName(void) const {
    return "Gui::ViewProviderFeature";
  }
  //@}
};

/// The exception testing feature
class FeatureTestException :public FeatureTest
{
  PROPERTY_HEADER(App::FeatureTestException);

public:
  FeatureTestException();

  /// this property defines which kind of exceptio the feature throw on you
  App::PropertyInteger ExceptionType;

  /// recalculate the Feature and throw an exception
  virtual DocumentObjectExecReturn *execute(void);
  /// returns the type name of the ViewProvider
  virtual const char* getViewProviderName(void) const {
    return "Gui::ViewProviderFeature";
  }
};



} //namespace App

#endif // APP_FEATURETEST_H
