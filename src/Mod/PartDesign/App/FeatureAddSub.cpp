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
    ADD_PROPERTY_TYPE(Outside, (false),"Part Design", App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "If set, the result will be the intersection of the profile and the preexisting body."));
}

void FeatureAddSub::onChanged(const App::Property* property)
{
    Feature::onChanged(property);
}

bool FeatureAddSub::isAdditive()
{
    return addSubType == Additive;
}

bool FeatureAddSub::isSubtractive()
{
    return addSubType == Subtractive;
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

TopoDS_Shape FeatureAddSub::subtractiveOp(const TopoDS_Shape &baseShape, const TopoDS_Shape &opShape)
{
    Outside.setStatus(App::Property::Hidden, ! isSubtractive());    // Set this after creation, like here.
    TopoDS_Shape result;
    if (  Outside.getValue() ) {
        BRepAlgoAPI_Common mkCom(baseShape, opShape);
        if (!mkCom.IsDone())
            throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Intersection of base feature failed"));
        result = mkCom.Shape();
    } else {
        BRepAlgoAPI_Cut mkCut(baseShape, opShape);
        if (!mkCut.IsDone())
            throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Cut out of base feature failed"));
        result = mkCut.Shape();
    }
    return result;
}

App::DocumentObjectExecReturn* FeatureAddSub::addSubOp(const TopoDS_Shape &baseShape, const TopoDS_Shape &opShape)
{
    AddSubShape.setValue(primitiveShape);

    TopoShape boolOp(0);
    TopoShape workingShape = opShape;

    const char* maker;
    if ( isAdditive() ) {
        maker = Part::OpCodes::Fuse;
    } else if ( isSubtractive() ) {
        if (Outside.getValue()) {
            maker = Part::OpCodes::Cut;
            workingShape = opShape.Reversed();
        }
        else {
            maker = Part::OpCodes::Cut;
        }
    } else {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Unknown operation type"));
    }
    try {
        boolOp.makeElementBoolean(maker, {baseShape, workingShape});
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Failed to perform boolean operation"));
    }
    boolOp = this->getSolid(boolOp);
    // lets check if the result is a solid
    if (boolOp.isNull()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));
    }
    boolOp = refineShapeIfActive(boolOp);
    Shape.setValue(getSolid(boolOp));
    AddSubShape.setValue(opShape);

    return App::DocumentObject::StdReturn;
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

        if (const TopoShape& addSubShape = AddSubShape.getShape(); !addSubShape.isEmpty()) {
            try {
                base.makeElementBoolean(Part::OpCodes::Common, {base, addSubShape});
            }
            catch (Standard_Failure& e) {
                notifyWarning(QString::fromUtf8(e.GetMessageString()));
            }
            catch (Base::Exception& e) {
                notifyWarning(QString::fromStdString(e.getMessage()));
            }

            if (base.isEmpty()) {
                notifyWarning(
                    tr("Resulting shape is empty. That may indicate that no material will be "
                       "removed or a problem with the model.")
                );
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
