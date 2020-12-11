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

#include <boost/range.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/FeaturePythonPyImp.h>
#include <App/Document.h>
#include "Body.h"
#include "FeatureWrap.h"
#include "FeaturePy.h"

typedef boost::iterator_range<const char*> CharRange;

FC_LOG_LEVEL_INIT("PartDesign", true, true);

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::FeatureWrap, PartDesign::FeatureAddSub)

FeatureWrap::FeatureWrap()
{
    ADD_PROPERTY_TYPE(WrapFeature,(0),"Part Design",(App::PropertyType)(App::Prop_Hidden),
            "Wrapped feature");
    WrapFeature.setScope(App::LinkScope::Global);

    ADD_PROPERTY_TYPE(Type,((long)3),"Part Design",(App::PropertyType)(App::Prop_None),
            "Additive: fuse the wrapped feature with the base feature.\n"
            "Subtractive: cut the wrapped feature from the base feature.\n"
            "Standalone: standalone feature, which may or may not be a solid.\n"
            "AutoDetect: auto detect feature type based on resulting shape.\n");
    static const char *TypeEnums[] = {"Additive", "Subtractive", "Standalone", "Auto Detect", nullptr};
    Type.setEnums(TypeEnums);

    ADD_PROPERTY_TYPE(Frozen,(false),"Part Design",(App::PropertyType)(App::Prop_None),
            "Whether to update shape on change of wrapped feature");

    AddSubShape.setStatus(App::Property::Transient, true);
}

void FeatureWrap::setWrappedLinkScope()
{
    auto wrapped = WrapFeature.getValue();
    if (!wrapped)
        return;
    std::vector<App::Property*> props;
    wrapped->getPropertyList(props);
    for (auto prop : props) {
        auto propLink = Base::freecad_dynamic_cast<App::PropertyLinkBase>(prop);
        if (propLink && propLink->getScope() == App::LinkScope::Local)
            propLink->setScope(App::LinkScope::Global);
    }
}

void FeatureWrap::onNewSolidChanged()
{
    auto body = Body::findBodyOf(this);
    if (!body)
        return;

    if (Type.getValue() == 2) {
        this->addSubType = Additive;
        BaseFeature.setValue(nullptr);
    } else {
        this->addSubType = Type.getValue() == 1 ? Subtractive : Additive;
        if (!NewSolid.getValue()) {
            BaseFeature.setValue(nullptr);
        } else {
            auto base = body->getPrevSolidFeature(this);
            BaseFeature.setValue(base);
        }
    }

    if (body->Tip.getValue() == this) {
        if (!isSolidFeature()) {
            auto feat = body->getPrevSolidFeature(this);
            if (!feat)
                feat = body->getNextSolidFeature(this);
            if (feat)
                body->setTip(feat);
        }
    } else if (!body->getNextSolidFeature(this))
        body->setTip(this);
}

void FeatureWrap::onChanged(const App::Property * prop)
{
    if (!this->isRestoring() 
            && this->getDocument()
            && !this->getDocument()->isPerformingTransaction()) {
        if (prop == &Type) {
            onNewSolidChanged();
        } else if (prop == &Frozen || prop == &WrapFeature) {
            if (prop == &WrapFeature)
                setWrappedLinkScope();
            AddSubShape.setStatus(App::Property::Transient, WrapFeature.getValue() && !Frozen.getValue());
        }
    }

    FeatureAddSub::onChanged(prop);
}

App::DocumentObjectExecReturn * FeatureWrap::execute(void)
{
    auto feat = WrapFeature.getValue();
    if (!feat || !feat->getNameInDocument())
        return new App::DocumentObjectExecReturn("No wrap feature");

    try {
        TopoShape shape;

        if (Frozen.getValue())
            shape = AddSubShape.getValue();
        else
            shape = Part::Feature::getTopoShape(feat);

        if (shape.isNull())
            return new App::DocumentObjectExecReturn("No shape from wrap feature");

        if (Type.getValue() == 3) {
            Type.setValue((long)2);
            NewSolid.setValue(!getSolid(shape).isNull());
        }

        TopoShape base;
        base = getBaseShape(true);

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

        if(base.isNull() || Type.getValue() > 1) {
            if (Type.getValue() < 2)
                return new App::DocumentObjectExecReturn("No base shape");
            if (Type.getValue() != 2) {
                shape = getSolid(shape);
                if (shape.isNull())
                    return new App::DocumentObjectExecReturn("No solid shape");
            }
            Shape.setValue(shape);
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
            boolOp = getSolid(boolOp);
            if (boolOp.isNull())
                return new App::DocumentObjectExecReturn("Resulting shape is null");
            Shape.setValue(boolOp);
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
    setWrappedLinkScope();
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

bool FeatureWrap::isSolidFeature() const
{
    return NewSolid.getValue() || (BaseFeature.getValue() && Type.getValue() <= 1);
}

App::DocumentObject *FeatureWrap::getSubObject(const char *subname, 
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const
{
    auto wrapped = WrapFeature.getValue();
    if (wrapped && subname
                && subname != Data::ComplexGeoData::findElementName(subname)) {
        const char * dot = strchr(subname,'.');
        if (dot && boost::equals(CharRange(subname, dot), wrapped->getNameInDocument())) {
            Base::Matrix4D _mat;
            if (!transform) {
                // If no transform is request, we must counter the
                // placement of this feature, because
                // PartDesign::Feature is not suppose to transform its
                // children
                _mat = Placement.getValue().inverse().toMatrix();
                if (pmat)
                    *pmat *= _mat; 
                else
                    pmat = &_mat;
            }
            return wrapped->getSubObject(dot+1, pyObj, pmat, true, depth+1);
        }
    }
    return PartDesign::Feature::getSubObject(subname, pyObj, pmat, transform, depth);
}


namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesign::FeatureWrapPython, PartDesign::FeatureWrap)
template<> const char* PartDesign::FeatureWrapPython::getViewProviderName(void) const {
    return "PartDesignGui::ViewProviderWrapPython";
}
template class PartDesignExport FeaturePythonT<PartDesign::FeatureWrap>;
}
