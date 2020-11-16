/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_Failure.hxx>
#endif


#include <Base/Console.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/FeaturePythonPyImp.h>
#include <App/Document.h>
#include "FeatureWrap.h"
#include "FeaturePy.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true);

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::FeatureWrap, PartDesign::FeatureAddSub)

FeatureWrap::FeatureWrap()
{
    ADD_PROPERTY_TYPE(WrapFeature,(0),"Part Design",(App::PropertyType)(App::Prop_Hidden),
            "Wrapped feature");
    WrapFeature.setScope(App::LinkScope::Global);

    ADD_PROPERTY_TYPE(Type,((long)0),"Part Design",(App::PropertyType)(App::Prop_None),
            "Additive: fuse the wrapped feature with the base feature.\n"
            "Subtractive: cut the wrapped feature from the base feature.\n"
            "Standalone: standalone feature, which may or may not be a solid.\n");
    static const char *TypeEnums[] = {"Additive", "Subtractive", "Standalone"};
    Type.setEnums(TypeEnums);

    ADD_PROPERTY_TYPE(Frozen,(false),"Part Design",(App::PropertyType)(App::Prop_None),
            "Whether to update shape on change of wrapped feature");

    AddSubShape.setStatus(App::Property::Transient, true);
}

void FeatureWrap::onChanged(const App::Property * prop)
{
    if (prop == &Type) {
        switch(Type.getValue()) {
        case 1 :
            this->addSubType = Subtractive;
            break;
        default:
            this->addSubType = Additive;
        }
    } else if (prop == &Frozen || prop == &WrapFeature) {
        auto wrap = Base::freecad_dynamic_cast<Part::Feature>(WrapFeature.getValue());
        AddSubShape.setStatus(App::Property::Transient, wrap && !Frozen.getValue());
    }

    FeatureAddSub::onChanged(prop);
}

App::DocumentObjectExecReturn * FeatureWrap::execute(void)
{
    auto feat = WrapFeature.getValue();
    if (!feat || !feat->getNameInDocument())
        return new App::DocumentObjectExecReturn("No wrap feature");

    TopoShape base;
    try {
        base = getBaseShape();
    } catch (const Base::Exception&) {
    }

    try {
        TopoShape shape;

        if (Frozen.getValue())
            shape = AddSubShape.getValue();
        else
            shape = Part::Feature::getTopoShape(feat);

        if (shape.isNull())
            return new App::DocumentObjectExecReturn("No shape from wrap feature");

        Part::Feature* feat = getBaseObject(/* silent = */ true);
        if (feat)
            this->Placement.setValue(feat->Placement.getValue());
        else
            this->Placement.setValue(shape.getPlacement());

        TopLoc_Location invObjLoc = this->getLocation().Inverted();
        if(!base.isNull())
            base.move(invObjLoc);
            
        if (!Frozen.getValue())
            AddSubShape.setValue(shape);

        if(base.isNull()) {
            Shape.setValue(getSolid(shape));
            return App::DocumentObject::StdReturn;
        }

        TopoShape boolOp(0,getDocument()->getStringHasher());

        if(getAddSubType() == FeatureAddSub::Additive) {
            try {
                boolOp.makEFuse({base,shape});
            }catch(Standard_Failure &e) {
                FC_ERR(getFullName() << ": " << e.GetMessageString());
                return new App::DocumentObjectExecReturn("Failed to fuse with base feature");
            }
            boolOp = this->getSolid(boolOp);
            // lets check if the result is a solid
            if (boolOp.isNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }
        else if(getAddSubType() == FeatureAddSub::Subtractive) {
            try {
                boolOp.makECut({base,shape});
            }catch(Standard_Failure & e) {
                FC_ERR(getFullName() << ": " << e.GetMessageString());
                return new App::DocumentObjectExecReturn("Failed to cut from base feature");
            }
            // we have to get the solids (fuse sometimes creates compounds)
            boolOp = this->getSolid(boolOp);
            // lets check if the result is a solid
            if (boolOp.isNull())
                return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

            boolOp = refineShapeIfActive(boolOp);
            Shape.setValue(getSolid(boolOp));
        }

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    } catch (const Base::Exception & e) {
        e.ReportException();
        return new App::DocumentObjectExecReturn(e.what());
    } catch (...) {
        return new App::DocumentObjectExecReturn("Unknown error");
    }
}

void FeatureWrap::onDocumentRestored()
{
    if (AddSubShape.testStatus(App::Property::Transient)) {
        TopoShape shape = Part::Feature::getTopoShape(WrapFeature.getValue());
        if (!shape.isNull())
            AddSubShape.setValue(shape);
    }
    FeatureAddSub::onDocumentRestored();
}

bool FeatureWrap::isElementGenerated(const TopoShape &shape, const char *name) const
{
    auto feat = WrapFeature.getValue();
    if (!feat)
        return FeatureAddSub::isElementGenerated(shape, name);
    long wrapTag = feat->getID();
    bool res = false;
    shape.traceElement(name,
        [&] (const std::string &, size_t, long tag2) {
            if (tag2 == wrapTag) {
                res = true;
                return true;
            }
            return false;
        });

    return res;
}

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesign::FeatureWrapPython, PartDesign::FeatureWrap)
template<> const char* PartDesign::FeatureWrapPython::getViewProviderName(void) const {
    return "PartDesignGui::ViewProviderWrapPython";
}
template class PartDesignExport FeaturePythonT<PartDesign::FeatureWrap>;
}
