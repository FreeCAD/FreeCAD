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
#include <Base/Console.h>
#include <Mod/Part/App/modelRefine.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <GProp_GProps.hxx>
#include <BRepGProp.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopoDS.hxx>

#include "FeatureAddSub.h"
#include "FeaturePy.h"

#include <Mod/Part/App/Tools.h>


FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

namespace PartDesign
{

extern bool getPDRefineModelParameter();

PROPERTY_SOURCE(PartDesign::FeatureAddSub, PartDesign::FeatureRefine)

FeatureAddSub::FeatureAddSub()
{
    ADD_PROPERTY(AddSubShape, (TopoDS_Shape()));
}

void FeatureAddSub::onChanged(const App::Property* property)
{
    Feature::onChanged(property);
}

FeatureAddSub::Type FeatureAddSub::getAddSubType()
{
    return addSubType;
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
    if (addSubType == Additive) {
        addShape = AddSubShape.getShape();
    }
    else if (addSubType == Subtractive) {
        subShape = AddSubShape.getShape();
    }
}

Part::TopoShape FeatureAddSub::getAddSubPreviewShape() const
{
    Part::TopoShape tool = AddSubShape.getShape();
    if (addSubType != Subtractive || tool.isNull()) {
        return tool;
    }
    // no base is an ordinary state here (the first feature in a body), so ask
    // for it silently
    constexpr bool silent = true;
    Part::TopoShape base = getBaseTopoShape(silent).moved(getLocation().Inverted());
    if (base.isNull()) {
        return tool;
    }
    // a tool that stays inside the base is already the removed volume
    if (tool.getBoundBox().IsInBox(base.getBoundBox())) {
        return tool;
    }
    // only trim analytic tools; a swept or spline tool (e.g. a modeled thread)
    // makes the boolean slow, so there the raw tool is the graceful fallback
    for (TopExp_Explorer it(tool.getShape(), TopAbs_FACE); it.More(); it.Next()) {
        switch (BRepAdaptor_Surface(TopoDS::Face(it.Current())).GetType()) {
            case GeomAbs_Plane:
            case GeomAbs_Cylinder:
            case GeomAbs_Cone:
            case GeomAbs_Sphere:
            case GeomAbs_Torus:
                break;
            default:
                // any other (or future) surface type is treated as non-analytic
                // and skips the boolean, so an unknown type fails safe to the tool
                return tool;
        }
    }
    try {
        Part::TopoShape common;
        common.makeElementBoolean(Part::OpCodes::Common, {base, tool}, "Preview", Precision::Confusion());
        if (!common.isNull()) {
            return common;
        }
    }
    catch (const Standard_Failure& e) {
        // The trim is only cosmetic, so a failed boolean falls back to the raw
        // tool rather than aborting the preview. Logged because a shape that
        // fails here every time is otherwise invisible in a bug report.
        FC_WARN("preselect preview trim failed for " << getFullName() << ": " << e.GetMessageString());
    }
    catch (const Base::Exception& e) {
        FC_WARN("preselect preview trim failed for " << getFullName() << ": " << e.what());
    }
    return tool;
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
    if (addSubType == Subtractive) {
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
                notifyWarning(QString::fromStdString(e.getMessage()));
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
    addSubType = Additive;
}

FeatureAdditivePython::~FeatureAdditivePython() = default;


PROPERTY_SOURCE(PartDesign::FeatureSubtractivePython, PartDesign::FeatureAddSubPython)

FeatureSubtractivePython::FeatureSubtractivePython()
{
    addSubType = Subtractive;
}

FeatureSubtractivePython::~FeatureSubtractivePython() = default;

}  // namespace PartDesign
