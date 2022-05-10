/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


#include "PreCompiled.h"

#include <Base/Exception.h>
#include <Base/Unit.h>

#include "FeatureTest.h"
#include "Material.h"

#ifdef _MSC_VER
#pragma warning( disable : 4700 )
#pragma warning( disable : 4723 )
#endif

using namespace App;


PROPERTY_SOURCE(App::FeatureTest, App::DocumentObject)

const char* enums[]= {"Zero","One","Two","Three","Four",nullptr};
const PropertyIntegerConstraint::Constraints intPercent = {0,100,1};
const PropertyFloatConstraint::Constraints floatPercent = {0.0,100.0,1.0};


FeatureTest::FeatureTest()
{
  ADD_PROPERTY(Integer,(4711)  );
  ADD_PROPERTY(Float  ,(47.11f) );
  ADD_PROPERTY(Bool   ,(true)  );
  ADD_PROPERTY(BoolList,(false));
  ADD_PROPERTY(String ,("4711"));
  ADD_PROPERTY(Path   ,("c:\\temp"));
  ADD_PROPERTY(StringList ,("4711"));

  ADD_PROPERTY(Enum   ,(4));
  Enum.setEnums(enums);
  ADD_PROPERTY(ConstraintInt ,(5));
  ConstraintInt.setConstraints(&intPercent);
  ADD_PROPERTY(ConstraintFloat ,(5.0));
  ConstraintFloat.setConstraints(&floatPercent);

  App::Color c;
  App::Material mat(App::Material::GOLD);
  ADD_PROPERTY(Colour      ,(c) );
  ADD_PROPERTY(ColourList  ,(c) );
  ADD_PROPERTY(Material    ,(mat));
  ADD_PROPERTY(MaterialList,(mat));

  ADD_PROPERTY(Distance,(47.11f) );
  ADD_PROPERTY(Angle   ,(3.0f) );

  ADD_PROPERTY(IntegerList,(4711)  );
  ADD_PROPERTY(FloatList  ,(47.11f) );

  ADD_PROPERTY(Link       ,(nullptr));
  ADD_PROPERTY(LinkSub    ,(nullptr));
  ADD_PROPERTY(LinkList   ,(nullptr));
  ADD_PROPERTY(LinkSubList,(nullptr));

  ADD_PROPERTY(Vector    ,(1.0,2.0,3.0));
  ADD_PROPERTY(VectorList,(3.0,2.0,1.0));
  ADD_PROPERTY(Matrix    ,(Base::Matrix4D(1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0,10.0,11.0,12.0,13.0,14.0,15.0,16.0)));
  ADD_PROPERTY(Placement ,(Base::Placement()));

  // properties for recompute testing
  static const char* group = "Feature Test";
  ADD_PROPERTY_TYPE(Source1       ,(nullptr),group,Prop_None,"Source for testing links");
  ADD_PROPERTY_TYPE(Source2       ,(nullptr),group,Prop_None,"Source for testing links");
  ADD_PROPERTY_TYPE(SourceN       ,(nullptr),group,Prop_None,"Source for testing links");
  ADD_PROPERTY_TYPE(ExecResult    ,("empty"),group,Prop_None,"Result of the execution");
  ADD_PROPERTY_TYPE(ExceptionType ,(0),group,Prop_None,"The type of exception the execution method throws");
  ADD_PROPERTY_TYPE(ExecCount     ,(0),group,Prop_None,"Number of executions");

  // properties with types
  ADD_PROPERTY_TYPE(TypeHidden  ,(4711),group,Prop_Hidden,"An example property which has the type 'Hidden'"  );
  ADD_PROPERTY_TYPE(TypeReadOnly,(4711),group,Prop_ReadOnly ,"An example property which has the type 'ReadOnly'"  );
  ADD_PROPERTY_TYPE(TypeOutput  ,(4711),group,Prop_Output ,"An example property which has the type 'Output'"  );
  ADD_PROPERTY_TYPE(TypeTransient,(4711),group,Prop_Transient ,"An example property which has the type 'Transient'"  );
  ADD_PROPERTY_TYPE(TypeNoRecompute,(4711),group,Prop_NoRecompute,"An example property which has the type 'NoRecompute'");
  ADD_PROPERTY_TYPE(TypeAll     ,(4711),group,(App::PropertyType) (Prop_Output|Prop_ReadOnly |Prop_Hidden ),
      "An example property which has the types 'Output', 'ReadOnly' and 'Hidden'");

  ADD_PROPERTY(QuantityLength,(1.0));
  QuantityLength.setUnit(Base::Unit::Length);
  ADD_PROPERTY(QuantityOther,(5.0));
  QuantityOther.setUnit(Base::Unit(-3,1));
  //ADD_PROPERTY(QuantityMass,(1.0));
  //QuantityMass.setUnit(Base::Unit::Mass);
  //ADD_PROPERTY(QuantityAngle,(1.0));
  //QuantityAngle.setUnit(Base::Unit::Angle);

}

FeatureTest::~FeatureTest()
{

}

short FeatureTest::mustExecute() const
{
    return DocumentObject::mustExecute();
}

DocumentObjectExecReturn *FeatureTest::execute()
{
    // Enum handling
    Enumeration enumObj1 = Enum.getEnum();
    enumObj1.setValue(7, false);
    enumObj1.setValue(4, true);

    Enumeration enumObj2 = Enum.getEnum();
    enumObj2.setValue(4, true);

    Enumeration enumObj3(enumObj2);
    const char* val = enumObj3.getCStr();
    enumObj3.isValue(val);
    enumObj3.getEnumVector();

    Enumeration enumObj4("Single item");
    enumObj4.setEnums(enums);
    std::ignore = enumObj4 == enumObj2;
    enumObj4.setEnums(nullptr);
    enumObj4 = enumObj2;
    std::ignore = enumObj4 == enumObj4.getCStr();

    Enumeration enumObj5(enums, enums[3]);
    enumObj5.isValue(enums[2]);
    enumObj5.isValue(enums[3]);
    enumObj5.contains(enums[1]);

    Enumeration enumObj6;
    enumObj6.setEnums(enums);
    enumObj6.setValue(enums[1]);
    std::vector<std::string> list;
    list.emplace_back("Hello");
    list.emplace_back("World");
    enumObj6.setEnums(list);
    enumObj6.setValue(list.back());
    /*
doc=App.newDocument()
obj=doc.addObject("App::FeatureTest")

obj.ExceptionType=0 # good
doc.recompute()

obj.ExceptionType=1 # unknown exception
doc.recompute()

obj.ExceptionType=2 # Runtime error
doc.recompute()

obj.ExceptionType=3 # segfault
doc.recompute()

obj.ExceptionType=4 # segfault
doc.recompute()

obj.ExceptionType=5 # int division by zero
doc.recompute()

obj.ExceptionType=6 # float division by zero
doc.recompute()
     */
    int *i=nullptr,j;
    float f;
    void *s;
    std::string t;

    // Code analyzers may complain about some errors. This can be ignored
    // because this is done on purpose to test the error handling mechanism
    switch(ExceptionType.getValue())
    {
        case 0: break;
        case 1: throw std::runtime_error("Test Exception");
        case 2: throw Base::RuntimeError("FeatureTestException::execute(): Testexception");
#if 0 // only allow these error types on purpose
        case 3: *i=0;printf("%i",*i);break;                 // seg-fault
        case 4: t = nullptr; break;                         // seg-fault
        case 5: j=0; printf("%i",1/j); break;               // int division by zero
        case 6: f=0.0; printf("%f",1/f); break;             // float division by zero
        case 7: s = malloc(3600000000ul); free(s); break;   // out-of-memory
#else
        default: (void)i; (void)j; (void)f; (void)s; (void)t; break;
#endif
    }

    ExecCount.setValue(ExecCount.getValue() + 1);

    ExecResult.setValue("Exec");

    return DocumentObject::StdReturn;
}


PROPERTY_SOURCE(App::FeatureTestException, App::FeatureTest)


FeatureTestException::FeatureTestException()
{
    ADD_PROPERTY(ExceptionType,(Base::Exception::getClassTypeId().getKey())  );
}

DocumentObjectExecReturn *FeatureTestException::execute()
{
    //ExceptionType;
    throw Base::RuntimeError("FeatureTestException::execute(): Testexception  ;-)");

    return nullptr;
}
