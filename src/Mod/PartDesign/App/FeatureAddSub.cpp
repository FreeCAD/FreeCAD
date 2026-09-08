// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#include <Standard_Failure.hxx>


#include <App/FeaturePythonPyImp.h>
#include <Mod/Part/App/modelRefine.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>

#include "FeatureAddSub.h"
#include "FeaturePy.h"

#include <Mod/Part/App/Tools.h>


using namespace PartDesign;

namespace PartDesign
{

extern bool getPDRefineModelParameter();

PROPERTY_SOURCE(PartDesign::FeatureAddSub, PartDesign::FeatureRefine)

FeatureAddSub::FeatureAddSub()
{
    ADD_PROPERTY(AddSubShape, (TopoDS_Shape()));
    ADD_PROPERTY_TYPE(Operation, (false), "Part Design", App::Prop_None, "Boolean operation to compute");
    Operation.setEnums({"Union", "Subtraction", "Common"});
}


FeatureAddSub::Type FeatureAddSub::getAddSubType()
{
    return addSubType;
}

FeatureAddSub::BooleanOperation FeatureAddSub::getBooleanOperation()
{
    return booleanOperation;
}

void FeatureAddSub::defineAdditive()
{
    addSubType = FeatureAddSub::Type::Additive;
    Operation.setStatus(App::Property::Status::Hidden, true);
    Operation.setEnums({"Union"});
    Operation.setValue("Union");
}

void FeatureAddSub::defineSubtractive()
{
    addSubType = FeatureAddSub::Type::Subtractive;
    Operation.setStatus(App::Property::Status::Hidden, false);
    Operation.setEnums({"Subtraction", "Common"});
    Operation.setValue("Subtraction");
}

const char* FeatureAddSub::getBooleanMaker() const
{
    switch (booleanOperation) {
        case BooleanOperation::Subtraction:
            return Part::OpCodes::Cut;
        case BooleanOperation::Common:
            return Part::OpCodes::Common;
        default:
            return Part::OpCodes::Fuse;
    }
}

void FeatureAddSub::onChanged(const App::Property* property)
{
    if (property == &Operation) {
        const char* strOp = Operation.getValueAsString();
        if (strcmp(strOp, "Subtraction") == 0) {
            booleanOperation = BooleanOperation::Subtraction;
            addSubType = Type::Subtractive;
        }
        else if (strcmp(strOp, "Common") == 0) {
            booleanOperation = BooleanOperation::Common;
            addSubType = Type::Subtractive;
        }
        else {
            booleanOperation = BooleanOperation::Union;
            addSubType = Type::Additive;
        }
    }

    Feature::onChanged(property);
}

short FeatureAddSub::mustExecute() const
{
    if (Refine.isTouched()) {
        return 1;
    }
    return PartDesign::Feature::mustExecute();
}

void FeatureAddSub::getAddSubShape(Part::TopoShape& addShape, Part::TopoShape& subShape)
{
    if (addSubType == Type::Additive) {
        addShape = AddSubShape.getShape();
    }
    else {
        subShape = AddSubShape.getShape();
    }
}

void FeatureAddSub::updatePreviewShape()
{
    const auto notifyWarning = [](const QString& message) {
        Base::Console().translatedUserWarning(
            "Preview",
            tr("Failure while computing removed volume preview: %1").arg(message).toUtf8()
        );
    };

    // for subtractive shapes we want to also showcase removed volume, not only the tool
    if (getBooleanMaker() != Part::OpCodes::Fuse) {
        TopoShape base = getBaseTopoShape(true).moved(getLocation().Inverted());
        const TopoShape& tool = AddSubShape.getShape();

        if (!tool.isEmpty()) {
            try {
                // Compute removed volume preview (for display)
                TopoShape common;
                common.makeElementBoolean(
                    Part::OpCodes::Common,
                    {base, tool},
                    "Preview",
                    Precision::Confusion()
                );

                // does CUT change volume?
                GProp_GProps propsBefore, propsAfter;
                BRepGProp::VolumeProperties(base.getShape(), propsBefore);

                TopoShape cut;
                cut.makeElementBoolean(
                    Part::OpCodes::Cut,
                    {base, tool},
                    "PreviewCheck",
                    Precision::Confusion()
                );

                BRepGProp::VolumeProperties(cut.getShape(), propsAfter);

                const double removed = propsBefore.Mass() - propsAfter.Mass();

                if (removed <= Precision::Confusion()) {
                    notifyWarning(
                        tr("Resulting shape is empty. That may indicate that no material will be "
                           "removed or a problem with the model.")
                    );
                }
                PreviewShape.setValue(common);
                return;
            }
            catch (Standard_Failure& e) {
                notifyWarning(QString::fromUtf8(e.GetMessageString()));
            }
            catch (Base::Exception& e) {
                notifyWarning(QString::fromStdString(e.what()));
            }
            PreviewShape.setValue(base);
            return;
        }
    }

    PreviewShape.setValue(AddSubShape.getShape());
}

}  // namespace PartDesign

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesign::FeatureAddSubPython, PartDesign::FeatureAddSub)
template<>
const char* PartDesign::FeatureAddSubPython::getViewProviderName() const
{
    return "PartDesignGui::ViewProviderPython";
}
template<>
PyObject* PartDesign::FeatureAddSubPython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<PartDesign::FeaturePy>(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class PartDesignExport FeaturePythonT<PartDesign::FeatureAddSub>;
}  // namespace App


namespace PartDesign
{

PROPERTY_SOURCE(PartDesign::FeatureAdditivePython, PartDesign::FeatureAddSubPython)

FeatureAdditivePython::FeatureAdditivePython()
{
    defineAdditive();
}

FeatureAdditivePython::~FeatureAdditivePython() = default;


PROPERTY_SOURCE(PartDesign::FeatureSubtractivePython, PartDesign::FeatureAddSubPython)

FeatureSubtractivePython::FeatureSubtractivePython()
{
    defineSubtractive();
}

FeatureSubtractivePython::~FeatureSubtractivePython() = default;

}  // namespace PartDesign
