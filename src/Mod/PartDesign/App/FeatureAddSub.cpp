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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_Failure.hxx>
#endif

#include <App/FeaturePythonPyImp.h>
#include <Mod/Part/App/modelRefine.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureAddSub.h"
#include "FeaturePy.h"


using namespace PartDesign;

namespace PartDesign {

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
    if (Refine.isTouched())
        return 1;
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

void FeatureAddSub::updatePreviewShape()
{
    const auto notifyWarning = [](const QString& message) {
        Base::Console().userTranslatedNotification(
            tr("Failure while computing preview: %1. That usually indicates an error with model.")
                .arg(message)
                .toUtf8());
    };

    // for subtractive shapes we want to also showcase removed volume, not only the tool
    if (addSubType == Subtractive) {
        TopoShape base = getBaseTopoShape(true).moved(getLocation().Inverted());

        try {
            base.makeElementBoolean(Part::OpCodes::Common, { base, AddSubShape.getShape() });
        } catch (Standard_Failure& e) {
            notifyWarning(QString::fromUtf8(e.GetMessageString()));
        }

        if (base.isValid() && !base.isNull()) {
            PreviewShape.setValue(base);
            return;
        }

        if (base.isNull()) {
            notifyWarning(tr("Preview shape is empty"));
        }

        if (!base.isValid()) {
            notifyWarning(tr("Preview shape is invalid"));
        }
    }

    PreviewShape.setValue(AddSubShape.getShape());
}

}  // namespace PartDesign

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesign::FeatureAddSubPython, PartDesign::FeatureAddSub)
template<> const char* PartDesign::FeatureAddSubPython::getViewProviderName() const {
    return "PartDesignGui::ViewProviderPython";
}
template<> PyObject* PartDesign::FeatureAddSubPython::getPyObject() {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<PartDesign::FeaturePy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class PartDesignExport FeaturePythonT<PartDesign::FeatureAddSub>;
}


namespace PartDesign {

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

}
