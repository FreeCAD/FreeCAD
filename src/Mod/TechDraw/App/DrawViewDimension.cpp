/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *                 2022 WandererFan <wandererfan@gmail.com>                *
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
#include <limits>
#include <sstream>

#include <QLocale>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QStringList>

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepLProp_CLProps.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <Geom_Plane.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <ShapeAnalysis.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include <Mod/Measure/App/Measurement.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>

#include <Mod/TechDraw/App/DrawViewDimensionPy.h>  // generated from DrawViewDimensionPy.xml

#include "DrawViewDimension.h"
#include "DimensionFormatter.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "Geometry.h"
#include "GeometryMatcher.h"
#include "Preferences.h"
#include "DimensionAutoCorrect.h"
#include "DrawBrokenView.h"

using namespace TechDraw;
using namespace Part;
using DU = DrawUtil;
using RefType = DrawViewDimension::RefType;

//===========================================================================
// DrawViewDimension
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDimension, TechDraw::DrawView)

const char* DrawViewDimension::TypeEnums[] = {"Distance",
                                              "DistanceX",
                                              "DistanceY",
                                              "DistanceZ",
                                              "Radius",
                                              "Diameter",
                                              "Angle",
                                              "Angle3Pt",
                                              "Area",
                                              nullptr};

const char* DrawViewDimension::MeasureTypeEnums[] = {"True", "Projected", nullptr};

// constraint to set the step size to 0.1
static const App::PropertyQuantityConstraint::Constraints ToleranceConstraint = {
    -std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 0.1};
// constraint to force positive values
static const App::PropertyQuantityConstraint::Constraints PositiveConstraint = {
    0.0, std::numeric_limits<double>::max(), 0.1};

DrawViewDimension::DrawViewDimension()
{
    // create the formatter since it will be needed to set default property values
    m_formatter = new DimensionFormatter(this);

    ADD_PROPERTY_TYPE(References2D,
                      (nullptr, nullptr),
                      "",
                      (App::Prop_None),
                      "Projected Geometry References");
    References2D.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(References3D,
                      (nullptr, nullptr),
                      "",
                      (App::Prop_None),
                      "3D Geometry References");
    References3D.setScope(App::LinkScope::Global);

    ADD_PROPERTY_TYPE(FormatSpec,
                      (getDefaultFormatSpec()),
                      "Format",
                      App::Prop_Output,
                      "Dimension format");
    ADD_PROPERTY_TYPE(FormatSpecOverTolerance,
                      (getDefaultFormatSpec(true)),
                      "Format",
                      App::Prop_Output,
                      "Dimension overtolerance format");
    ADD_PROPERTY_TYPE(FormatSpecUnderTolerance,
                      (getDefaultFormatSpec(true)),
                      "Format",
                      App::Prop_Output,
                      "Dimension undertolerance format");
    ADD_PROPERTY_TYPE(Arbitrary, (false), "Format", App::Prop_Output, "Value overridden by user");
    ADD_PROPERTY_TYPE(ArbitraryTolerances,
                      (false),
                      "Format",
                      App::Prop_Output,
                      "Tolerance values overridden by user");

    Type.setEnums(TypeEnums);  // dimension type: length, radius etc
    ADD_PROPERTY(Type, ((long)0));
    MeasureType.setEnums(MeasureTypeEnums);
    ADD_PROPERTY(MeasureType, ((long)1));  // Projected (or True) measurement
    ADD_PROPERTY_TYPE(TheoreticalExact,
                      (false),
                      "",
                      App::Prop_Output,
                      "If theoretical exact (basic) dimension");
    ADD_PROPERTY_TYPE(EqualTolerance,
                      (true),
                      "",
                      App::Prop_Output,
                      "If over- and undertolerance are equal");

    ADD_PROPERTY_TYPE(OverTolerance,
                      (0.0),
                      "",
                      App::Prop_Output,
                      "Overtolerance value\nIf 'Equal Tolerance' is true this is also\nthe negated "
                      "value for 'Under Tolerance'");
    OverTolerance.setUnit(Base::Unit::Length);
    OverTolerance.setConstraints(&ToleranceConstraint);
    ADD_PROPERTY_TYPE(UnderTolerance,
                      (0.0),
                      "",
                      App::Prop_Output,
                      "Undertolerance value\nIf 'Equal Tolerance' is true it will be replaced\nby "
                      "negative value of 'Over Tolerance'");
    UnderTolerance.setUnit(Base::Unit::Length);
    UnderTolerance.setConstraints(&ToleranceConstraint);
    ADD_PROPERTY_TYPE(Inverted,
                      (false),
                      "",
                      App::Prop_Output,
                      "The dimensional value is displayed inverted");

    ADD_PROPERTY_TYPE(AngleOverride,
                      (false),
                      "Override",
                      App::Prop_Output,
                      "User specified angles");
    ADD_PROPERTY_TYPE(LineAngle, (0.0), "Override", App::Prop_Output, "Dimension line angle");
    ADD_PROPERTY_TYPE(ExtensionAngle, (0.0), "Override", App::Prop_Output, "Extension line angle");

    ADD_PROPERTY_TYPE(SavedGeometry,
                      (),
                      "References",
                      (App::PropertyType)(App::Prop_None),
                      "Reference Geometry");
    SavedGeometry.setOrderRelevant(true);

    ADD_PROPERTY_TYPE(
        BoxCorners,
        (),
        "References",
        (App::Prop_None),
        "Feature bounding box corners as of last reference update.  Used by autocorrect");

    // changing the references in the property editor will only cause problems
    References2D.setStatus(App::Property::ReadOnly, true);
    References3D.setStatus(App::Property::ReadOnly, true);

    // hide the DrawView properties that don't apply to Dimensions
    ScaleType.setStatus(App::Property::ReadOnly, true);
    ScaleType.setStatus(App::Property::Hidden, true);
    Scale.setStatus(App::Property::ReadOnly, true);
    Scale.setStatus(App::Property::Hidden, true);
    Rotation.setStatus(App::Property::ReadOnly, true);
    Rotation.setStatus(App::Property::Hidden, true);
    Caption.setStatus(App::Property::Hidden, true);
    LockPosition.setStatus(App::Property::Hidden, true);

    // by default EqualTolerance is true, thus make UnderTolerance read-only
    UnderTolerance.setStatus(App::Property::ReadOnly, true);
    FormatSpecUnderTolerance.setStatus(App::Property::ReadOnly, true);

    // legacy behaviour if this is false
    ADD_PROPERTY_TYPE(UseActualArea, (true), "Area", App::Prop_Output,
                      "If true, area dimensions return the area of the face minus the areas of any enclosed faces. \
                       If false, the area of the face's outer boundary is returned.");

    ADD_PROPERTY_TYPE(ShowUnits, (Preferences::showUnits()), "Format", App::Prop_None,
                                          "Show or hide the units.");
    measurement = new Measure::Measurement();
    // TODO: should have better initial datumLabel position than (0, 0) in the DVP?? something
    // closer to the object being measured?

    // initialize the descriptive geometry.
    // TODO: should this be more like DVP with a "geometry object"?
    resetLinear();
    resetAngular();
    resetArc();
    m_hasGeometry = false;
    m_matcher = new GeometryMatcher();
    m_referencesCorrect = true;
    m_corrector = new DimensionAutoCorrect(this);
}

DrawViewDimension::~DrawViewDimension()
{
    delete measurement;
    measurement = nullptr;
    delete m_formatter;
    delete m_matcher;
    delete m_corrector;
}

void DrawViewDimension::resetLinear()
{
    m_linearPoints.first(Base::Vector3d(0, 0, 0));
    m_linearPoints.second(Base::Vector3d(0, 0, 0));
}

void DrawViewDimension::resetAngular()
{
    m_anglePoints.first(Base::Vector3d(0, 0, 0));
    m_anglePoints.second(Base::Vector3d(0, 0, 0));
    m_anglePoints.vertex(Base::Vector3d(0, 0, 0));
}

void DrawViewDimension::resetArc()
{
    m_arcPoints.isArc = false;
    m_arcPoints.center = Base::Vector3d(0, 0, 0);
    m_arcPoints.onCurve.first(Base::Vector3d(0, 0, 0));
    m_arcPoints.onCurve.second(Base::Vector3d(0, 0, 0));
    m_arcPoints.arcEnds.first(Base::Vector3d(0, 0, 0));
    m_arcPoints.arcEnds.second(Base::Vector3d(0, 0, 0));
    m_arcPoints.midArc = Base::Vector3d(0, 0, 0);
    m_arcPoints.arcCW = false;
}

void DrawViewDimension::resetArea()
{
    m_areaPoint.center = Base::Vector3d(0, 0, 0);
    m_areaPoint.area = 0.0;
    m_areaPoint.actualArea = 0.0;
}

void DrawViewDimension::onChanged(const App::Property* prop)
{
    if (prop == &References3D) {
        // have to rebuild the Measurement object
        clear3DMeasurements();  // Measurement object
        if (!(References3D.getValues()).empty()) {
            setAll3DMeasurement();
        }
    }

    if (isRestoring()) {
        DrawView::onChanged(prop);
        return;
    }

    if (prop == &References2D) {
        updateSavedGeometry();
    }
    else if (prop == &References3D) {
        // remove the old measurement object
        clear3DMeasurements();
        if (!(References3D.getValues()).empty()) {
            // rebuild the Measurement object
            setAll3DMeasurement();
        }
        else if (MeasureType.isValue("True")) {  // empty 3dRefs, but True
            MeasureType.touch();                 // run MeasureType logic for this case
        }
        updateSavedGeometry();
    }
    else if (prop == &Type) {
        FormatSpec.setValue(getDefaultFormatSpec().c_str());
        auto type = static_cast<DimensionType>(Type.getValue());
        if (type == DimensionType::Angle || type == DimensionType::Angle3Pt) {
            OverTolerance.setUnit(Base::Unit::Angle);
            UnderTolerance.setUnit(Base::Unit::Angle);
        }
        else {
            OverTolerance.setUnit(Base::Unit::Length);
            UnderTolerance.setUnit(Base::Unit::Length);
        }
    }
    else if (prop == &TheoreticalExact) {
        // if TheoreticalExact disable tolerances and set them to zero
        if (TheoreticalExact.getValue()) {
            OverTolerance.setValue(0.0);
            UnderTolerance.setValue(0.0);
            OverTolerance.setReadOnly(true);
            UnderTolerance.setReadOnly(true);
            FormatSpecOverTolerance.setReadOnly(true);
            FormatSpecUnderTolerance.setReadOnly(true);
            ArbitraryTolerances.setValue(false);
            ArbitraryTolerances.setReadOnly(true);
        }
        else {
            OverTolerance.setReadOnly(false);
            FormatSpecOverTolerance.setReadOnly(false);
            ArbitraryTolerances.setReadOnly(false);
            if (!EqualTolerance.getValue()) {
                UnderTolerance.setReadOnly(false);
                FormatSpecUnderTolerance.setReadOnly(false);
            }
        }
    }
    else if (prop == &EqualTolerance) {
        // if EqualTolerance set negated overtolerance for untertolerance
        // then also the OverTolerance must be positive
        if (EqualTolerance.getValue()) {
            // if OverTolerance is negative or zero, first set it to zero
            if (OverTolerance.getValue() < 0) {
                OverTolerance.setValue(0.0);
            }
            OverTolerance.setConstraints(&PositiveConstraint);
            UnderTolerance.setValue(-1.0 * OverTolerance.getValue());
            UnderTolerance.setUnit(OverTolerance.getUnit());
            UnderTolerance.setReadOnly(true);
            FormatSpecUnderTolerance.setValue(FormatSpecOverTolerance.getValue());
            FormatSpecUnderTolerance.setReadOnly(true);
        }
        else {
            OverTolerance.setConstraints(&ToleranceConstraint);
            if (!TheoreticalExact.getValue()) {
                UnderTolerance.setReadOnly(false);
                FormatSpecUnderTolerance.setReadOnly(false);
            }
        }
    }
    else if (prop == &OverTolerance) {
        // if EqualTolerance set negated overtolerance for untertolerance
        if (EqualTolerance.getValue()) {
            UnderTolerance.setValue(-1.0 * OverTolerance.getValue());
            UnderTolerance.setUnit(OverTolerance.getUnit());
        }
    }
    else if (prop == &FormatSpecOverTolerance) {
        if (EqualTolerance.getValue() && !ArbitraryTolerances.getValue()) {
            FormatSpecUnderTolerance.setValue(FormatSpecOverTolerance.getValue());
        }
    }
    else if (prop == &FormatSpecUnderTolerance) {
        if (EqualTolerance.getValue() && !ArbitraryTolerances.getValue()) {
            FormatSpecOverTolerance.setValue(FormatSpecUnderTolerance.getValue());
        }
    }

    DrawView::onChanged(prop);
}

void DrawViewDimension::Restore(Base::XMLReader& reader)
// Old drawings did not have the equal tolerance options.
// We cannot just introduce it as being set to true because that would e.g. destroy tolerances like
// +1-2 Therefore set it to false for existing documents
{
    EqualTolerance.setValue(false);
    DrawView::Restore(reader);
}

void DrawViewDimension::onDocumentRestored()
{
    if (has3DReferences()) {
        setAll3DMeasurement();
    }

    auto type = static_cast<DimensionType>(Type.getValue());
    if (type == DimensionType::Angle || type == DimensionType::Angle3Pt) {
        OverTolerance.setUnit(Base::Unit::Angle);
        UnderTolerance.setUnit(Base::Unit::Angle);
    }
}

void DrawViewDimension::handleChangedPropertyType(Base::XMLReader& reader,
                                                  const char* TypeName,
                                                  App::Property* prop)
{
    if (prop == &OverTolerance && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat value;
        value.Restore(reader);
        OverTolerance.setValue(value.getValue());
    }
    else if (prop == &UnderTolerance && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat value;
        value.Restore(reader);
        UnderTolerance.setValue(value.getValue());
    }
    else {
        TechDraw::DrawView::handleChangedPropertyType(reader, TypeName, prop);
    }

    // Over/Undertolerance were further changed from App::PropertyQuantity to
    // App::PropertyQuantityConstraint
    if (prop == &OverTolerance && strcmp(TypeName, "App::PropertyQuantity") == 0) {
        App::PropertyQuantity OverToleranceProperty;
        // restore the PropertyQuantity to be able to set its value
        OverToleranceProperty.Restore(reader);
        OverTolerance.setValue(OverToleranceProperty.getValue());
    }
    else if (prop == &UnderTolerance && strcmp(TypeName, "App::PropertyQuantity") == 0) {
        App::PropertyQuantity UnderToleranceProperty;
        // restore the PropertyQuantity to be able to set its value
        UnderToleranceProperty.Restore(reader);
        UnderTolerance.setValue(UnderToleranceProperty.getValue());
    }
}

short DrawViewDimension::mustExecute() const
{
    if (!isRestoring()) {
        if (References2D.isTouched() || References3D.isTouched() || Type.isTouched()) {
            return 1;
        }
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn* DrawViewDimension::execute()
{
    if (!okToProceed()) {
        // if we set an error here, it will be triggered many times during
        // document load.
        return  DrawView::execute();
    }

    m_referencesCorrect = true;
    if (Preferences::autoCorrectDimRefs()) {
        m_referencesCorrect = autocorrectReferences();
    }
    if (!m_referencesCorrect) {
        // this test needs Phase 2 of auto correct to be useful
        Base::Console().log("The references for %s have changed and autocorrect could not match the geometry\n", Label.getValue());
    }

    resetLinear();
    resetAngular();
    resetArc();
    resetArea();

    // we have either or both valid References3D and References2D
    ReferenceVector references = getEffectiveReferences();

    if (Type.isValue("Distance") || Type.isValue("DistanceX") || Type.isValue("DistanceY")) {
        if (getRefType() == RefType::oneEdge) {
            m_linearPoints = getPointsOneEdge(references);
        }
        else if (getRefType() == RefType::twoEdge) {
            m_linearPoints = getPointsTwoEdges(references);
        }
        else if (getRefType() == RefType::twoVertex) {
            m_linearPoints = getPointsTwoVerts(references);
        }
        else if (getRefType() == RefType::vertexEdge) {
            m_linearPoints = getPointsEdgeVert(references);
        }
        m_hasGeometry = true;
    }
    else if (Type.isValue("Radius") ||
             Type.isValue("Diameter") ) {
        m_arcPoints = getArcParameters(references);
        m_hasGeometry = true;
    }
    else if (Type.isValue("Angle")) {
        if (getRefType() != RefType::twoEdge) {
            throw Base::RuntimeError("Angle dimension has non-edge references");
        }
        m_anglePoints = getAnglePointsTwoEdges(references);
        m_hasGeometry = true;
    }
    else if (Type.isValue("Angle3Pt")) {
        if (getRefType() != RefType::threeVertex) {
            throw Base::RuntimeError("3 point angle dimension has non-vertex references");
        }
        m_anglePoints = getAnglePointsThreeVerts(references);
        m_hasGeometry = true;
    }
    else if (Type.isValue("Area")) {
        if (getRefType() != RefType::oneFace) {
            throw Base::RuntimeError("area dimension has non-face references");
        }
        m_areaPoint = getAreaParameters(references);
        m_hasGeometry = true;
    }

    overrideKeepUpdated(false);
    return DrawView::execute();
}

// true if we have enough information to execute, false otherwise
bool DrawViewDimension::okToProceed()
{
    if (!keepUpdated()) {
        return false;
    }
    DrawViewPart* dvp = getViewPart();
    if (!dvp) {
        // TODO: translate these messages and figure out how to present them to
        // the user since we can't pop up a message box here.
        // this case is probably temporary during restore
        // Base::Console().message("DVD::okToProceed - no view for dimension\n");
        return false;
    }

    if (!(has2DReferences() || has3DReferences())) {
        // no references, can't do anything
        // Base::Console().message("DVD::okToProceed - Dimension object has no valid references\n");
        return false;
    }

    if (!getViewPart()->hasGeometry()) {
        // can't do anything until Source has geometry
        // Base::Console().message("DVD::okToProceed - Dimension object has no geometry\n");
        return false;
    }

    // is this check still relevant or is it replaced by the autocorrect and
    // validate methods?
    if (References3D.getValues().empty() && !checkReferences2D()) {
        // Base::Console().warning("%s has invalid 2D References\n", getNameInDocument());
        return false;
    }
    return validateReferenceForm();
}

//! check if geometry pointed to by references matches the saved version. If
//! everything matches, we don't need to correct anything.
bool DrawViewDimension::autocorrectReferences()
{
    // TODO: check for saved geometry here.  If we don't have saved geometry, we can't
    // successfully auto correct in phase 1.  This check is currently in
    // referencesHaveValidGeometry.
    std::vector<bool> referenceState;
    bool refsAreValid = m_corrector->referencesHaveValidGeometry(referenceState);
    if (!refsAreValid) {
        m_corrector->set3dObjectCache(m_3dObjectCache);
        ReferenceVector repairedRefs;
        refsAreValid = m_corrector->autocorrectReferences(referenceState, repairedRefs);
        if (!refsAreValid) {
            // references are broken and we can not fix them
            return false;

        }
        if (repairedRefs.front().is3d()) {
            setReferences3d(repairedRefs);
        }
        else {
            setReferences2d(repairedRefs);
        }
    }
    return true;
}

bool DrawViewDimension::isMultiValueSchema() const
{
    return m_formatter->isMultiValueSchema();
}

std::string
DrawViewDimension::formatValue(qreal value, const QString& qFormatSpec, DimensionFormatter::Format partial, bool isDim)
{
    return m_formatter->formatValue(value, qFormatSpec, partial, isDim);
}

bool DrawViewDimension::haveTolerance()
{
    // if a numeric tolerance is specified AND
    // tolerances are NOT arbitrary
    return (!DrawUtil::fpCompare(OverTolerance.getValue(), 0.0) ||
            !DrawUtil::fpCompare(UnderTolerance.getValue(), 0.0)) &&
            !ArbitraryTolerances.getValue();
}

std::string DrawViewDimension::getFormattedToleranceValue(DimensionFormatter::Format partial)
{
    return m_formatter->getFormattedToleranceValue(partial);
}

////get over and under tolerances
std::pair<std::string, std::string> DrawViewDimension::getFormattedToleranceValues(DimensionFormatter::Format partial)
{
    return m_formatter->getFormattedToleranceValues(partial);
}

////partial = 2 unit only
std::string DrawViewDimension::getFormattedDimensionValue(DimensionFormatter::Format partial)
{
    return m_formatter->getFormattedDimensionValue(partial);
}

QStringList DrawViewDimension::getPrefixSuffixSpec(const QString &fSpec)
{
    return m_formatter->getPrefixSuffixSpec(fSpec);
}

//! NOTE: this returns the Dimension value in internal units (ie mm)!!!!
double DrawViewDimension::getDimValue()
{
    constexpr double CircleDegrees{360.0};
    double result = 0.0;
    if (!has2DReferences() && !has3DReferences()) {
        // nothing to measure
        return result;
    }
    if (!getViewPart()) {
        return result;
    }

    if (!getViewPart()->hasGeometry()) {  // happens when loading saved document
        return result;
    }

    if (MeasureType.isValue("True")) {
        // True Values
        if (!measurement->has3DReferences()) {
            Base::Console().warning("%s - True dimension has no 3D References\n",
                                    getNameInDocument());
            return result;
        }
        result = getTrueDimValue();
    }
    else {
        // Projected Values
        if (!checkReferences2D()) {
            Base::Console().warning("DVD::getDimValue - %s - 2D references are corrupt (5)\n",
                                    getNameInDocument());
            return result;
        }
        result = getProjectedDimValue();
    }

    result = fabs(result);
    if (Inverted.getValue()) {
        if (Type.isValue("Angle") || Type.isValue("Angle3Pt")) {
            result = CircleDegrees - result;
        }
        else {
            result = -result;
        }
    }
    return result;
}

//! retrieve the dimension value for "true" dimensions. The returned value is in internal units (mm).
double DrawViewDimension::getTrueDimValue() const
{
    double result = 0.0;

    if (Type.isValue("Distance") || Type.isValue("DistanceX") || Type.isValue("DistanceY")) {
        result = measurement->length();
    }
    else if (Type.isValue("Radius")) {
        result = measurement->radius();
    }
    else if (Type.isValue("Diameter")) {
        result = 2 * measurement->radius();
    }
    else if (Type.isValue("Angle") || Type.isValue("Angle3Pt")) {
        result = measurement->angle();
    }
    else if (Type.isValue("Area")) {
        result = measurement->area();
    }
    else {  // tarfu
        throw Base::ValueError("getDimValue() - Unknown Dimension Type (3)");
    }
    return result;
}

//! retrieve the dimension value for "projected" (2d) dimensions. The returned value is in internal units (mm).
double DrawViewDimension::getProjectedDimValue() const
{
    double result = 0.0;
    double scale = getViewPart()->getScale();

    if (Type.isValue("Distance") || Type.isValue("DistanceX") || Type.isValue("DistanceY")) {
        pointPair pts = getLinearPoints();
        auto dbv = freecad_cast<DrawBrokenView*>(getViewPart());
        if (dbv)  {
            // raw pts from view are inverted Y, so we need to un-invert them before mapping
            // raw pts are scaled, so we need to unscale them for mapPoint2dFromView
            // then rescale them for the distance calculation below
            // centers are right side up
            // if both points are on the expanded side  of the last (rightmost/upmost) break
            // then we should not move the points.
            //
            pts.invertY();
            // unscale the points, map them to the broken view then rescale them to draw.
            pts.scale(1 / scale);
            pts.first(dbv->mapPoint2dFromView(pts.first()));
            pts.second(dbv->mapPoint2dFromView(pts.second()));
            pts.invertY();
            pts.scale(scale);
        }
        Base::Vector3d dimVec = pts.first() - pts.second();
        if (Type.isValue("Distance")) {
            result = dimVec.Length() / scale;
        }
        else if (Type.isValue("DistanceX")) {
            result = fabs(dimVec.x) / scale;
        }
        else {
            result = fabs(dimVec.y) / scale;
        }
    }
    else if (Type.isValue("Radius")) {
        // Projected BaseGeom is scaled for drawing
        result = m_arcPoints.radius / scale;
    }
    else if (Type.isValue("Diameter")) {
        arcPoints pts = m_arcPoints;
        result = (pts.radius * 2) / scale; // Projected BaseGeom is scaled for drawing
    }
    else if (Type.isValue("Angle") || Type.isValue("Angle3Pt")) {  // same as case "Angle"?
        anglePoints pts = m_anglePoints;
        Base::Vector3d vertex = pts.vertex();
        Base::Vector3d leg0 = pts.first() - vertex;
        Base::Vector3d leg1 = pts.second() - vertex;
        double legAngle = Base::toDegrees(leg0.GetAngle(leg1));
        result = legAngle;
    }
    else if (Type.isValue("Area")) {
        // 2d reference makes scaled values in areaPoint
        // 3d reference makes actual values in areaPoint :p
        double divisor{scale / scale};
        if (has3DReferences()) {
            divisor = 1.0;
        }
        if (UseActualArea.getValue()) {
           result = m_areaPoint.actualArea / divisor;
        } else {
            result = m_areaPoint.area / divisor;
        }
    }

    return result;
}


pointPair DrawViewDimension::getLinearPoints() const
{
    Base::Vector3d stdY{0, 1, 0};
    if (Type.isValue("Distance")) {
        // if the dimVec points the wrong way on generally vertical dims, the dim text will be
        // placed on the wrong side of the dim line.
        auto dimVec = m_linearPoints.second() - m_linearPoints.first();
        dimVec.Normalize();
        auto dotY = stdY.Dot(dimVec);
        if (dotY > 0) {
            // dimVec points up (ish) so the dim text will be to right of dim line and readable from
            // left side of the page.  Dimensions should always be readable from the bottom-right, so
            // we flip the points.
            return {m_linearPoints.second(), m_linearPoints.first()};
        }
    }
    return m_linearPoints;
}

pointPair DrawViewDimension::getPointsOneEdge(ReferenceVector references)
{
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement = DrawUtil::getIndexFromName(references.front().getSubName());
    if (refObject->isDerivedFrom<TechDraw::DrawViewPart>()
        && !references.at(0).getSubName().empty()) {
        // TODO: Notify if not straight line Edge?
        // this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr geom = getViewPart()->getGeomByIndex(iSubelement);
        if (!geom) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " can not find geometry for 2d reference (1)";
            throw Base::RuntimeError(ssMessage.str());
        }
        if (geom->getGeomType() != GeomType::GENERIC) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " 2d reference is a " << geom->geomTypeName();
            throw Base::RuntimeError(ssMessage.str());
        }
        TechDraw::GenericPtr generic = std::static_pointer_cast<TechDraw::Generic>(geom);
        // these points are from 2d geometry, so they are scaled and rotated
        return {generic->points[0], generic->points[1]};
    }

    // this is a 3d object
    // get the endpoints of the edge in the DVP's coordinates
    TopoDS_Shape geometry = references.front().getGeometry();
    if (geometry.IsNull() || geometry.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    const TopoDS_Edge& edge = TopoDS::Edge(geometry);
    gp_Pnt gEnd0 = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
    gp_Pnt gEnd1 = BRep_Tool::Pnt(TopExp::LastVertex(edge));

    pointPair pts(Base::convertTo<Base::Vector3d>(gEnd0), Base::convertTo<Base::Vector3d>(gEnd1));
    pts.move(getViewPart()->getCurrentCentroid());
    pts.project(getViewPart());
    return pts;
}

pointPair DrawViewDimension::getPointsTwoEdges(ReferenceVector references)
{
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(1).getSubName());
    if (refObject->isDerivedFrom<TechDraw::DrawViewPart>()
        && !references.at(0).getSubName().empty()) {
        // this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr geom0 = getViewPart()->getGeomByIndex(iSubelement0);
        TechDraw::BaseGeomPtr geom1 = getViewPart()->getGeomByIndex(iSubelement1);
        if (!geom0 || !geom1) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " can not find geometry for 2d reference (2)";
            throw Base::RuntimeError(ssMessage.str());
        }
        return closestPoints(geom0->getOCCEdge(), geom1->getOCCEdge());
    }

    // this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() || geometry0.ShapeType() != TopAbs_EDGE
        || geometry1.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }

    pointPair pts = closestPoints(geometry0, geometry1);
    pts.move(getViewPart()->getCurrentCentroid());
    pts.project(getViewPart());
    return pts;
}

pointPair DrawViewDimension::getPointsTwoVerts(ReferenceVector references)
{
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(1).getSubName());
    if (refObject->isDerivedFrom<TechDraw::DrawViewPart>()
        && !references.at(0).getSubName().empty()) {
        // this is a 2d object (a DVP + subelements)
        TechDraw::VertexPtr v0 = getViewPart()->getProjVertexByIndex(iSubelement0);
        TechDraw::VertexPtr v1 = getViewPart()->getProjVertexByIndex(iSubelement1);
        if (!v0 || !v1) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " can not find geometry for 2d reference (3)";
            throw Base::RuntimeError(ssMessage.str());
        }

        return {v0->point(), v1->point()};
    }

    // this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() || geometry0.ShapeType() != TopAbs_VERTEX
        || geometry1.ShapeType() != TopAbs_VERTEX) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    const TopoDS_Vertex& vertex0 = TopoDS::Vertex(geometry0);
    const TopoDS_Vertex& vertex1 = TopoDS::Vertex(geometry1);
    gp_Pnt gPoint0 = BRep_Tool::Pnt(vertex0);
    gp_Pnt gPoint1 = BRep_Tool::Pnt(vertex1);

    pointPair pts(Base::convertTo<Base::Vector3d>(gPoint0), Base::convertTo<Base::Vector3d>(gPoint1));
    pts.move(getViewPart()->getCurrentCentroid());
    pts.project(getViewPart());
    return pts;
}

pointPair DrawViewDimension::getPointsEdgeVert(ReferenceVector references)
{
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(1).getSubName());
    if (refObject->isDerivedFrom<TechDraw::DrawViewPart>()
        && !references.at(0).getSubName().empty()) {
        // this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr edge;
        TechDraw::VertexPtr vertex;
        if (DrawUtil::getGeomTypeFromName(references.at(0).getSubName()) == "Edge") {
            edge = getViewPart()->getGeomByIndex(iSubelement0);
            vertex = getViewPart()->getProjVertexByIndex(iSubelement1);
        }
        else {
            edge = getViewPart()->getGeomByIndex(iSubelement1);
            vertex = getViewPart()->getProjVertexByIndex(iSubelement0);
        }
        if (!vertex || !edge) {
            throw Base::RuntimeError("Missing geometry for dimension (4)");
        }

        // get curve from edge
        double start{0.0};  // curve parameters
        double end{0.0};  // curve parameters
        const Handle(Geom_Surface) hplane = new Geom_Plane(gp_Ax3());
        auto const occCurve =
            BRep_Tool::CurveOnPlane(edge->getOCCEdge(), hplane, TopLoc_Location(), start, end);
        auto const occPoint = gp_Pnt2d(vertex->x(), vertex->y());
        // project point on curve
        auto projector = Geom2dAPI_ProjectPointOnCurve(occPoint, occCurve);
        if (projector.NbPoints() > 0) {
            auto p1 = Base::Vector3d(vertex->x(), vertex->y(), 0.0);
            auto p2 =
                Base::Vector3d(projector.NearestPoint().X(), projector.NearestPoint().Y(), 0.0);
            pointPair result = pointPair(p1, p2);
            result.setExtensionLine(closestPoints(edge->getOCCEdge(), vertex->getOCCVertex()));
            return result;
        }
            // unable to project
            return closestPoints(edge->getOCCEdge(), vertex->getOCCVertex());
    }

    // this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() || geometry0.ShapeType() != TopAbs_VERTEX
        || geometry1.ShapeType() != TopAbs_VERTEX) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }

    pointPair pts = closestPoints(geometry0, geometry1);
    pts.move(getViewPart()->getCurrentCentroid());
    pts.project(getViewPart());
    return pts;
}

arcPoints DrawViewDimension::getArcParameters(ReferenceVector references)
{
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement = DrawUtil::getIndexFromName(references.front().getSubName());
    if (refObject->isDerivedFrom<TechDraw::DrawViewPart>()
        && !references.at(0).getSubName().empty()) {
        // this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr geom = getViewPart()->getGeomByIndex(iSubelement);
        if (!geom) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " can not find geometry for 2d reference (4)";
            throw Base::RuntimeError(ssMessage.str());
        }
        return arcPointsFromBaseGeom(geom);
    }

    // this is a 3d reference
    TopoDS_Shape geometry = references.front().getGeometry();
    if (geometry.IsNull() || geometry.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    const TopoDS_Edge& edge = TopoDS::Edge(geometry);
    arcPoints pts = arcPointsFromEdge(edge);
    pts.move(getViewPart()->getCurrentCentroid());
    pts.project(getViewPart());
    return pts;
}

arcPoints DrawViewDimension::arcPointsFromBaseGeom(const TechDraw::BaseGeomPtr& base)
{
    TechDraw::CirclePtr circle;
    arcPoints pts;
    pts.center = Base::Vector3d(0.0, 0.0, 0.0);
    pts.radius = 0.0;
    if ((base && base->getGeomType() == GeomType::CIRCLE)
        || (base && base->getGeomType() == GeomType::ARCOFCIRCLE)) {
        circle = std::static_pointer_cast<TechDraw::Circle>(base);
        pts.center = Base::Vector3d(circle->center.x, circle->center.y, 0.0);
        pts.radius = circle->radius;
        if (base->getGeomType() == GeomType::ARCOFCIRCLE) {
            TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(circle);
            pts.isArc = true;
            pts.onCurve.first(Base::Vector3d(aoc->midPnt.x, aoc->midPnt.y, 0.0));
            pts.midArc = Base::Vector3d(aoc->midPnt.x, aoc->midPnt.y, 0.0);
            pts.arcEnds.first(Base::Vector3d(aoc->startPnt.x, aoc->startPnt.y, 0.0));
            pts.arcEnds.second(Base::Vector3d(aoc->endPnt.x, aoc->endPnt.y, 0.0));
            pts.arcCW = aoc->cw;
        }
        else {
            pts.isArc = false;
            pts.onCurve.first(
                pts.center + Base::Vector3d(1, 0, 0) * circle->radius);  // arbitrary point on edge
            pts.onCurve.second(
                pts.center + Base::Vector3d(-1, 0, 0) * circle->radius);  // arbitrary point on edge
        }
    }
    else if ((base && base->getGeomType() == GeomType::ELLIPSE)
             || (base && base->getGeomType() == GeomType::ARCOFELLIPSE)) {
        TechDraw::EllipsePtr ellipse = std::static_pointer_cast<TechDraw::Ellipse>(base);
        if (ellipse->closed()) {
            double r1 = ellipse->minor;
            double r2 = ellipse->major;
            double rAvg = (r1 + r2) / 2;
            pts.center = Base::Vector3d(ellipse->center.x, ellipse->center.y, 0.0);
            pts.radius = rAvg;
            pts.isArc = false;
            pts.onCurve.first(pts.center
                              + Base::Vector3d(1, 0, 0) * rAvg);  // arbitrary point on edge
            pts.onCurve.second(pts.center
                               + Base::Vector3d(-1, 0, 0) * rAvg);  // arbitrary point on edge
        }
        else {
            TechDraw::AOEPtr aoe = std::static_pointer_cast<TechDraw::AOE>(base);
            double r1 = aoe->minor;
            double r2 = aoe->major;
            double rAvg = (r1 + r2) / 2;
            pts.isArc = true;
            pts.center = Base::Vector3d(aoe->center.x, aoe->center.y, 0.0);
            pts.radius = rAvg;
            pts.arcEnds.first(Base::Vector3d(aoe->startPnt.x, aoe->startPnt.y, 0.0));
            pts.arcEnds.second(Base::Vector3d(aoe->endPnt.x, aoe->endPnt.y, 0.0));
            pts.midArc = Base::Vector3d(aoe->midPnt.x, aoe->midPnt.y, 0.0);
            pts.arcCW = aoe->cw;
            pts.onCurve.first(Base::Vector3d(aoe->midPnt.x, aoe->midPnt.y, 0.0));  // for radius
            //            pts.onCurve.first(pts.center + Base::Vector3d(1, 0,0) * rAvg);   //for
            //            diameter
            pts.onCurve.second(pts.center
                               + Base::Vector3d(-1, 0, 0) * rAvg);  // arbitrary point on edge
        }
    }
    else if (base && base->getGeomType() == GeomType::BSPLINE) {
        TechDraw::BSplinePtr spline = std::static_pointer_cast<TechDraw::BSpline>(base);
        if (spline->isCircle()) {
            bool arc{false};
            double rad{0};
            Base::Vector3d center;
            // bool circ =
            GeometryUtils::getCircleParms(spline->getOCCEdge(), rad, center, arc);
            pts.center = Base::Vector3d(center.x, center.y, 0.0);
            pts.radius = rad;
            pts.arcEnds.first(Base::Vector3d(spline->startPnt.x, spline->startPnt.y, 0.0));
            pts.arcEnds.second(Base::Vector3d(spline->endPnt.x, spline->endPnt.y, 0.0));
            pts.midArc = Base::Vector3d(spline->midPnt.x, spline->midPnt.y, 0.0);
            pts.isArc = arc;
            pts.arcCW = spline->cw;
            if (arc) {
                pts.onCurve.first(Base::Vector3d(spline->midPnt.x, spline->midPnt.y, 0.0));
            }
            else {
                pts.onCurve.first(pts.center
                                  + Base::Vector3d(1, 0, 0) * rad);  // arbitrary point on edge
                pts.onCurve.second(pts.center
                                   + Base::Vector3d(-1, 0, 0) * rad);  // arbitrary point on edge
            }
        }
        else {
            // fubar - can't have non-circular spline as target of Diameter dimension, but this is
            // already checked, so something has gone badly wrong.
            Base::Console().error("%s: can not make a Circle from this B-spline edge\n",
                                  getNameInDocument());
            throw Base::RuntimeError("Bad B-spline geometry for arc dimension");
        }
    }
    else {
        std::stringstream ssMessage;
        ssMessage << getNameInDocument() << " 2d reference is a " << base->geomTypeName();
        throw Base::RuntimeError(ssMessage.str());
    }
    return pts;
}

arcPoints DrawViewDimension::arcPointsFromEdge(const TopoDS_Edge& occEdge)
{
    arcPoints pts;
    pts.isArc = !BRep_Tool::IsClosed(occEdge);
    pts.arcCW = false;

    // get all the common information for circle, ellipse and bspline conversions
    BRepAdaptor_Curve adapt(occEdge);
    double pFirst = adapt.FirstParameter();
    double pLast = adapt.LastParameter();
    double pMid = (pFirst + pLast) / 2;
    BRepLProp_CLProps props(adapt, pFirst, 0, Precision::Confusion());
    pts.arcEnds.first(Base::convertTo<Base::Vector3d>(props.Value()));
    props.SetParameter(pLast);
    pts.arcEnds.second(Base::convertTo<Base::Vector3d>(props.Value()));
    props.SetParameter(pMid);
    pts.onCurve.first(Base::convertTo<Base::Vector3d>(props.Value()));
    pts.onCurve.second(Base::convertTo<Base::Vector3d>(props.Value()));
    pts.midArc = Base::convertTo<Base::Vector3d>(props.Value());

    if (adapt.GetType() == GeomAbs_Circle) {
        gp_Circ circle = adapt.Circle();
        pts.center = Base::convertTo<Base::Vector3d>(circle.Location());
        pts.radius = circle.Radius();
        if (pts.isArc) {
            // part of circle
            gp_Ax1 axis = circle.Axis();
            auto startVec = Base::convertTo<gp_Vec>(pts.arcEnds.first() - pts.center);
            auto endVec = Base::convertTo<gp_Vec>(pts.arcEnds.second() - pts.center);
            double angle = startVec.AngleWithRef(endVec, axis.Direction().XYZ());
            pts.arcCW = (angle < 0.0);
        }
        else {
            // full circle
            pts.onCurve.first(pts.center
                              + Base::Vector3d(1, 0, 0) * pts.radius);  // arbitrary point on edge
            pts.onCurve.second(pts.center
                               + Base::Vector3d(-1, 0, 0) * pts.radius);  // arbitrary point on edge
        }
    }
    else if (adapt.GetType() == GeomAbs_Ellipse) {
        gp_Elips ellipse = adapt.Ellipse();
        pts.center = Base::convertTo<Base::Vector3d>(ellipse.Location());
        pts.radius = (ellipse.MajorRadius() + ellipse.MinorRadius()) / 2;
        if (pts.isArc) {
            // part of ellipse
            gp_Ax1 axis = ellipse.Axis();
            auto startVec = Base::convertTo<gp_Vec>(pts.arcEnds.first() - pts.center);
            auto endVec = Base::convertTo<gp_Vec>(pts.arcEnds.second() - pts.center);
            double angle = startVec.AngleWithRef(endVec, axis.Direction().XYZ());
            pts.arcCW = (angle < 0.0);
        }
        else {
            // full ellipse
            pts.onCurve.first(pts.center
                              + Base::Vector3d(1, 0, 0) * pts.radius);  // arbitrary point on edge
            pts.onCurve.second(pts.center
                               + Base::Vector3d(-1, 0, 0) * pts.radius);  // arbitrary point on edge
        }
    }
    else if (adapt.GetType() == GeomAbs_BSplineCurve) {
        if (GeometryUtils::isCircle(occEdge)) {
            bool isArc(false);
            TopoDS_Edge circleEdge = GeometryUtils::asCircle(occEdge, isArc);
            pts.isArc = isArc;
            BRepAdaptor_Curve adaptCircle(circleEdge);
            if (adaptCircle.GetType() != GeomAbs_Circle) {
                throw Base::RuntimeError("failed to get circle from B-spline");
            }
            gp_Circ circle = adapt.Circle();
            // TODO: same code as above. reuse opportunity.
            pts.center = Base::convertTo<Base::Vector3d>(circle.Location());
            pts.radius = circle.Radius();
            if (pts.isArc) {
                // part of circle
                gp_Ax1 axis = circle.Axis();
                auto startVec = Base::convertTo<gp_Vec>(pts.arcEnds.first() - pts.center);
                auto endVec = Base::convertTo<gp_Vec>(pts.arcEnds.second() - pts.center);
                double angle = startVec.AngleWithRef(endVec, axis.Direction().XYZ());
                pts.arcCW = (angle < 0.0);
            }
            else {
                // full circle
                pts.onCurve.first(
                    pts.center + Base::Vector3d(1, 0, 0) * pts.radius);  // arbitrary point on edge
                pts.onCurve.second(
                    pts.center + Base::Vector3d(-1, 0, 0) * pts.radius);  // arbitrary point on edge
            }
        }
        else {
            throw Base::RuntimeError("failed to make circle from B-spline");
        }
    }
    else {
        throw Base::RuntimeError("can not get arc points from this edge");
    }

    return pts;
}

anglePoints DrawViewDimension::getAnglePointsTwoEdges(ReferenceVector references)
{
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(1).getSubName());
    if (refObject->isDerivedFrom<TechDraw::DrawViewPart>()
        && !references.at(0).getSubName().empty()) {
        // this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr geom0 = getViewPart()->getGeomByIndex(iSubelement0);
        TechDraw::BaseGeomPtr geom1 = getViewPart()->getGeomByIndex(iSubelement1);
        if (!geom0 || !geom1) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " can not find geometry for 2d reference (5)";
            throw Base::RuntimeError(ssMessage.str());
        }
        if (geom0->getGeomType() != GeomType::GENERIC) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " first 2d reference is a "
                      << geom0->geomTypeName();
            throw Base::RuntimeError(ssMessage.str());
        }
        if (geom1->getGeomType() != GeomType::GENERIC) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " second 2d reference is a "
                      << geom0->geomTypeName();
            throw Base::RuntimeError(ssMessage.str());
        }
        auto generic0 = std::static_pointer_cast<TechDraw::Generic>(geom0);
        auto generic1 = std::static_pointer_cast<TechDraw::Generic>(geom1);
        Base::Vector3d apex = generic0->apparentInter(generic1);

        Base::Vector3d farPoint0{generic0->getEndPoint()};
        if ((generic0->getStartPoint() - apex).Length()
            > (generic0->getEndPoint() - apex).Length()) {
            farPoint0 = generic0->getStartPoint();
        }


        // pick the end of generic1 farthest from the apex
        Base::Vector3d farPoint1{generic1->getEndPoint()};
        if ((generic1->getStartPoint() - apex).Length()
            > (generic1->getEndPoint() - apex).Length()) {
            farPoint1 = (generic1->getStartPoint());
        }

        Base::Vector3d leg0Dir = (farPoint0 - apex).Normalize();
        Base::Vector3d leg1Dir = (farPoint1 - apex).Normalize();
        Base::Vector3d extenPoint0 = farPoint0;  // extension line points
        Base::Vector3d extenPoint1 = farPoint1;


        double extenRadius = std::min(extenPoint0.Length(),
                                      extenPoint1.Length());
        if (extenRadius == 0) {
            // one of the legs has 0 length??
            throw Base::RuntimeError("No extension point radius!!");
        }

        anglePoints pts;
        pts.first(apex + leg0Dir * extenRadius);
        pts.second(apex + leg1Dir * extenRadius);
        pts.vertex(apex);
        return pts;
    }

    // this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() || geometry0.ShapeType() != TopAbs_EDGE
        || geometry1.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    TopoDS_Edge edge0 = TopoDS::Edge(geometry0);
    BRepAdaptor_Curve adapt0(edge0);
    TopoDS_Edge edge1 = TopoDS::Edge(geometry1);
    BRepAdaptor_Curve adapt1(edge1);

    if (adapt0.GetType() != GeomAbs_Line || adapt1.GetType() != GeomAbs_Line) {
        throw Base::RuntimeError("Geometry for angle dimension must be lines.");
    }
    gp_Pnt gStart0 = BRep_Tool::Pnt(TopExp::FirstVertex(edge0));
    gp_Pnt gEnd0 = BRep_Tool::Pnt(TopExp::LastVertex(edge0));
    gp_Vec gDir0(gEnd0.XYZ() - gStart0.XYZ());
    gp_Pnt gStart1 = BRep_Tool::Pnt(TopExp::FirstVertex(edge1));
    gp_Pnt gEnd1 = BRep_Tool::Pnt(TopExp::LastVertex(edge1));
    gp_Vec gDir1(gEnd1.XYZ() - gStart1.XYZ());
    Base::Vector3d vApex;
    bool haveIntersection = DrawUtil::intersect2Lines3d(Base::convertTo<Base::Vector3d>(gStart0),
                                                        Base::convertTo<Base::Vector3d>(gDir0),
                                                        Base::convertTo<Base::Vector3d>(gStart1),
                                                        Base::convertTo<Base::Vector3d>(gDir1),
                                                        vApex);
    if (!haveIntersection) {
        throw Base::RuntimeError("Geometry for 3d angle dimension does not intersect");
    }
    auto gApex = Base::convertTo<gp_Pnt>(vApex);

    gp_Pnt gFar0 = gEnd0;
    if (gStart0.Distance(gApex) > gEnd0.Distance(gApex)) {
        gFar0 = gStart0;
    }

    gp_Pnt gFar1 = gEnd1;
    if (gStart1.Distance(gApex) > gEnd1.Distance(gApex)) {
        gFar1 = gStart1;
    }
    anglePoints pts(Base::convertTo<Base::Vector3d>(gApex),
                    Base::convertTo<Base::Vector3d>(gFar0),
                    Base::convertTo<Base::Vector3d>(gFar1));
    pts.move(getViewPart()->getCurrentCentroid());
    pts.project(getViewPart());
    return pts;
}

// TODO: this makes assumptions about the order of references (p - v - p). is this checked
// somewhere?
anglePoints DrawViewDimension::getAnglePointsThreeVerts(ReferenceVector references)
{
    if (references.size() < 3) {
        throw Base::RuntimeError("Not enough references to make angle dimension");
    }
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(1).getSubName());
    int iSubelement2 = DrawUtil::getIndexFromName(references.at(2).getSubName());
    if (refObject->isDerivedFrom<TechDraw::DrawViewPart>()
        && !references.at(0).getSubName().empty()) {
        // this is a 2d object (a DVP + subelements)
        TechDraw::VertexPtr vert0 = getViewPart()->getProjVertexByIndex(iSubelement0);
        TechDraw::VertexPtr vert1 = getViewPart()->getProjVertexByIndex(iSubelement1);
        TechDraw::VertexPtr vert2 = getViewPart()->getProjVertexByIndex(iSubelement2);
        if (!vert0 || !vert1 || !vert2) {
            throw Base::RuntimeError("References for three point angle dimension are not vertices");
        }
        anglePoints pts(vert1->point(), vert0->point(), vert2->point());
        return pts;
    }

    // this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    TopoDS_Shape geometry2 = references.at(2).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() || geometry2.IsNull()
        || geometry0.ShapeType() != TopAbs_VERTEX || geometry1.ShapeType() != TopAbs_VERTEX
        || geometry2.ShapeType() != TopAbs_VERTEX) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    TopoDS_Vertex vertex0 = TopoDS::Vertex(geometry0);
    gp_Pnt point0 = BRep_Tool::Pnt(vertex0);
    TopoDS_Vertex vertex1 = TopoDS::Vertex(geometry1);
    gp_Pnt point1 = BRep_Tool::Pnt(vertex1);
    TopoDS_Vertex vertex2 = TopoDS::Vertex(geometry2);
    gp_Pnt point2 = BRep_Tool::Pnt(vertex2);
    anglePoints pts(Base::convertTo<Base::Vector3d>(point1),
                    Base::convertTo<Base::Vector3d>(point0),
                    Base::convertTo<Base::Vector3d>(point2));
    pts.move(getViewPart()->getCurrentCentroid());
    pts.project(getViewPart());
    return pts;
}

areaPoint DrawViewDimension::getAreaParameters(ReferenceVector references)
{
    areaPoint pts;

    App::DocumentObject* refObject = references.front().getObject();
    if (refObject->isDerivedFrom<DrawViewPart>() && !references[0].getSubName().empty()) {
        // this is a 2d object (a DVP + subelements)
        TechDraw::FacePtr face = getViewPart()->getFace(references[0].getSubName());
        if (!face) {
            std::stringstream ssMessage;
            ssMessage << getNameInDocument() << " can not find geometry for 2d reference (4)";
            throw Base::RuntimeError(ssMessage.str());
        }
        auto dvp = static_cast<DrawViewPart*>(refObject);

        auto filteredFaces  = GeometryUtils::findHolesInFace(dvp, references.front().getSubName());
        auto perforatedFace = GeometryUtils::makePerforatedFace(face, filteredFaces);

        // these areas are scaled because the source geometry is scaled, but it makes no sense to
        // report a scaled area.
        auto unscale = getViewPart()->getScale() * getViewPart()->getScale();
        pts.area = face->getArea() / unscale;     // this will be the 2d area as projected onto the page? not really filled area?
        pts.actualArea = getActualArea(perforatedFace)  / unscale;
        pts.center = getFaceCenter(perforatedFace);
        pts.invertY();      // geometry class is over, back to -Y up/.
    }
    else {
        // this is a 3d reference. perforations should be handled for us by OCC
        TopoDS_Shape geometry = references[0].getGeometry();
        if (geometry.IsNull() || geometry.ShapeType() != TopAbs_FACE) {
            throw Base::RuntimeError("Geometry for dimension reference is null.");
        }
        const TopoDS_Face& face = TopoDS::Face(geometry);

        // these areas are unscaled as the source is 3d geometry.
        pts.area = getFilledArea(face);
        pts.actualArea = getActualArea(face);
        pts.center = getFaceCenter(face);
        pts.move(getViewPart()->getCurrentCentroid());
        pts.project(getViewPart());
    }

    return pts;
}


//! returns the center of mass of a face (density = k)
Base::Vector3d DrawViewDimension::getFaceCenter(const TopoDS_Face& face)
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face, props);
    auto center = Base::convertTo<Base::Vector3d>(props.CentreOfMass());
    return center;
}


//! returns the "net" area of a face (area of the face's outer boundary less the area of any holes)
double DrawViewDimension::getActualArea(const TopoDS_Face& face)
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face, props);
    return props.Mass();
}


//! returns the "gross" area of a face (area of the face's outer boundary)
double DrawViewDimension::getFilledArea(const TopoDS_Face& face)
{
    TopoDS_Wire outerwire = ShapeAnalysis::OuterWire(face);
    if (outerwire.IsNull()) {
        return 0.0;
    }

    double area = ShapeAnalysis::ContourArea(outerwire);
    return area;
}


DrawViewPart* DrawViewDimension::getViewPart() const
{
    if (References2D.getValues().empty()) {
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawViewPart*>(References2D.getValues().at(0));
}

// return the references controlling this dimension. 3d references are used when available
// otherwise 2d references are returned. no checking is performed. Result is pairs of (object,
// subName)
ReferenceVector DrawViewDimension::getEffectiveReferences() const
{
    const std::vector<App::DocumentObject*>& objects3d = References3D.getValues();
    const std::vector<std::string>& subElements3d = References3D.getSubValues();
    const std::vector<App::DocumentObject*>& objects = References2D.getValues();
    const std::vector<std::string>& subElements = References2D.getSubValues();
    ReferenceVector effectiveRefs;

    // note that 3d references can be destroyed without our notice if the object
    // is deleted.
    if (objects3d.empty()) {
        // use 2d references
        size_t refCount = objects.size();
        for (size_t i = 0; i < refCount; i++) {
            if (subElements.empty()) {
                // the 3d references have likely been nulled out by an object
                // deletion.
                ReferenceEntry ref(objects.at(i), std::string());
                effectiveRefs.push_back(ref);
            }
            else {
                // normal 2d reference
                ReferenceEntry ref(objects.at(i), subElements.at(i));
                effectiveRefs.push_back(ref);
            }
        }
    }
    else {
        // use 3d references
        size_t refCount = objects3d.size();
        for (size_t i = 0; i < refCount; i++) {
            ReferenceEntry ref(objects3d.at(i), std::string(subElements3d.at(i)));
            effectiveRefs.push_back(ref);
        }
    }
    return effectiveRefs;
}


// what configuration of references do we have - Vertex-Vertex, Edge-Vertex, Edge, ...
RefType DrawViewDimension::getRefType() const
{
    if (isExtentDim()) {
        return RefType::extent;
    }

    ReferenceVector refs = getEffectiveReferences();
    std::vector<std::string> subNames;

    // std::vector<std::string> subNames = getEffectiveSubNames();   //???
    for (auto& ref : refs) {
        if (ref.getSubName().empty()) {
            // skip this one
            continue;
        }
        subNames.push_back(ref.getSubName());
    }

    if (subNames.empty()) {
        // something went wrong, there were no subNames.
        Base::Console().message("DVD::getRefType - %s - there are no subNames.\n",
                                getNameInDocument());
        return RefType::invalidRef;
    }

    return getRefTypeSubElements(subNames);
}

// TODO: Gui/DimensionValidators.cpp has almost the same code
// decide what the reference configuration is by examining the names of the sub elements
RefType DrawViewDimension::getRefTypeSubElements(const std::vector<std::string>& subElements)
{
    RefType refType{RefType::invalidRef};
    int refEdges{0};
    int refVertices{0};
    int refFaces{0};

    for (const auto& se : subElements) {
        if (DrawUtil::getGeomTypeFromName(se) == "Vertex") {
            refVertices++;
        }
        if (DrawUtil::getGeomTypeFromName(se) == "Edge") {
            refEdges++;
        }
        if (DrawUtil::getGeomTypeFromName(se) == "Face") {
            refFaces++;
        }
    }

    if (refEdges == 0 && refVertices == 2 && refFaces == 0) {
        refType = RefType::twoVertex;
    }
    if (refEdges == 0 && refVertices == 3 && refFaces == 0) {
        refType = RefType::threeVertex;
    }
    if (refEdges == 1 && refVertices == 0 && refFaces == 0) {
        refType = RefType::oneEdge;
    }
    if (refEdges == 1 && refVertices == 1 && refFaces == 0) {
        refType = RefType::vertexEdge;
    }
    if (refEdges == 2 && refVertices == 0 && refFaces == 0) {
        refType = RefType::twoEdge;
    }
    if (refEdges == 0 && refVertices == 0 && refFaces == 1) {
        refType = RefType::oneFace;
    }

    return refType;
}

//! validate 2D references - only checks if the target exists
bool DrawViewDimension::checkReferences2D() const
{
    const std::vector<App::DocumentObject*>& objects = References2D.getValues();
    if (objects.empty()) {
        return false;
    }

    const std::vector<std::string>& subElements = References2D.getSubValues();
    if (subElements.empty()) {
        // must have at least 1 null string entry to balance DVP
        return false;
    }

    if (subElements.front().empty() && !References3D.getValues().empty()) {
        // this is (probably) a dim with 3d refs
        return true;
    }

    for (auto& sub : subElements) {
        if (sub.empty()) {
            return false;
        }

        int idx = DrawUtil::getIndexFromName(sub);
        if (DrawUtil::getGeomTypeFromName(sub) == "Edge") {
            TechDraw::BaseGeomPtr geom = getViewPart()->getGeomByIndex(idx);
            if (!geom) {
                return false;
            }
        }
        else if (DrawUtil::getGeomTypeFromName(sub) == "Vertex") {
            TechDraw::VertexPtr vert = getViewPart()->getProjVertexByIndex(idx);
            if (!vert) {
                return false;
            }
        }
    }

    return true;
}

//! detect the state where 3d references have been nulled out due to
//! object deletion and the reference will need to be rebuilt.
bool DrawViewDimension::hasBroken3dReferences() const
{
    const std::vector<App::DocumentObject*>& objects3d = References3D.getValues();
    const std::vector<App::DocumentObject*>& objects = References2D.getValues();
    const std::vector<std::string>& subElements = References2D.getSubValues();

    // if we have the reference to the View, but no 2d subelements or 3d objects
    // this means that the 3d references have been nulled out due to
    // object deletion and the reference will need to be rebuilt.
    return (objects.size() == 1 &&
            objects3d.empty() &&
            subElements.empty());
}


void DrawViewDimension::updateSavedGeometry()
{
    ReferenceVector references = getEffectiveReferences();
    if (references.empty()) {
        // no references to save
        return;
    }
    std::vector<TopoShape> newGeometry;
    const std::vector<TopoShape> oldGeometry = SavedGeometry.getValues();
    // need to clean up old saved geometry objects here?

    for (auto& entry : references) {
        if (entry.getSubName().empty()) {
            // view only reference has no geometry.
            continue;
        }
        if (entry.hasGeometry()) {
            newGeometry.emplace_back(entry.asCanonicalTopoShape());
        }
        else {
            // have to put something in the vector so SavedGeometry and references stay in sync.
                newGeometry.emplace_back(Part::TopoShape());
        }
    }
    if (!newGeometry.empty()) {
        SavedGeometry.setValues(newGeometry);
        saveFeatureBox();
    }
}


// based on Part::TopoShapePyImp::getShapes. Produces a vector of unique edges within the shape
std::vector<TopoShape> DrawViewDimension::getEdges(const TopoShape& inShape)
{
    std::vector<TopoShape> ret;
    TopTools_IndexedMapOfShape shapeMap;
    TopExp_Explorer Ex(inShape.getShape(), TopAbs_EDGE);
    while (Ex.More()) {
        shapeMap.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= shapeMap.Extent(); k++) {
        const TopoDS_Shape& shape = shapeMap(k);
        ret.emplace_back(TopoShape(shape));
    }

    return ret;
}

// based on Part::TopoShapePyImp::getShapes
std::vector<TopoShape> DrawViewDimension::getVertexes(const TopoShape& inShape)
{
    std::vector<TopoShape> ret;
    TopTools_IndexedMapOfShape shapeMap;
    TopExp_Explorer Ex(inShape.getShape(), TopAbs_VERTEX);
    while (Ex.More()) {
        shapeMap.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= shapeMap.Extent(); k++) {
        const TopoDS_Shape& shape = shapeMap(k);
        ret.emplace_back(TopoShape(shape));
    }

    return ret;
}

//! returns the angle subtended by an arc from 3 points.
double DrawViewDimension::getArcAngle(Base::Vector3d center, Base::Vector3d startPoint, Base::Vector3d endPoint)
{
    auto leg0 = startPoint - center;
    auto leg1 = endPoint - startPoint;
    auto referenceDirection = leg0.Cross(leg1);
    gp_Ax1 axis{Base::convertTo<gp_Pnt>(center), Base::convertTo<gp_Vec>(referenceDirection)};
    auto startVec = Base::convertTo<gp_Vec>(leg0);
    auto endVec = Base::convertTo<gp_Vec>(leg1);
    double angle = startVec.AngleWithRef(endVec, axis.Direction().XYZ());
    return angle;
}


pointPair DrawViewDimension::closestPoints(const TopoDS_Shape& s1, const TopoDS_Shape& s2) const
{
    pointPair result;
    BRepExtrema_DistShapeShape extss(s1, s2);
    if (!extss.IsDone()) {
        throw Base::RuntimeError("DVD::closestPoints - BRepExtrema_DistShapeShape failed");
    }
    int count = extss.NbSolution();
    if (count != 0) {
        gp_Pnt point = extss.PointOnShape1(1);
        result.first(Base::Vector3d(point.X(), point.Y(), point.Z()));
        point = extss.PointOnShape2(1);
        result.second(Base::Vector3d(point.X(), point.Y(), point.Z()));
    }  // TODO: else { explode }

    return result;
}

// set the reference property from a reference vector
void DrawViewDimension::setReferences2d(const ReferenceVector& refsAll)
{
    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subNames;
    if (objects.size() != subNames.size()) {
        throw Base::IndexError("DVD::setReferences2d - objects and subNames do not match.");
    }

    for (auto& ref : refsAll) {
        objects.push_back(ref.getObject());
        subNames.push_back(ref.getSubName());
    }

    References2D.setValues(objects, subNames);
    m_referencesCorrect = true;
}

// set the reference property from a reference vector
void DrawViewDimension::setReferences3d(const ReferenceVector &refsAll)
{
    if (refsAll.empty() && !References3D.getValues().empty()) {
        // clear the property of any old links
        References3D.setValue(nullptr, nullptr);
        return;
    }
    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subNames;
    if (objects.size() != subNames.size()) {
        throw Base::IndexError("DVD::setReferences3d - objects and subNames do not match.");
    }

    for (auto& ref : refsAll) {
        objects.push_back(ref.getObject());
        subNames.push_back(ref.getSubName(true));
        // cache the referenced object
        m_3dObjectCache.insert(ref.getObject()->getNameInDocument());
        // cache the parent object if available.  Ideally, we would handle deletion
        // of a reference object in a slot for DocumentObject::signalDeletedObject,
        // but by the time we get the signal the document will have severed any links
        // between our object and its parents. So we need to cache the parent here while
        // we still have the link
        App::DocumentObject* firstParent = ref.getObject()->getFirstParent();
        if (firstParent) {
            m_3dObjectCache.insert(firstParent->getNameInDocument());
        }
    }

    References3D.setValues(objects, subNames);
    m_referencesCorrect = true;
}

//! add Dimension 3D references to measurement
void DrawViewDimension::setAll3DMeasurement()
{
    measurement->clear();
    const std::vector<App::DocumentObject*>& Objs = References3D.getValues();
    const std::vector<std::string>& Subs = References3D.getSubValues();
    size_t end = Objs.size();
    size_t iObject = 0;
    for (; iObject < end; iObject++) {
        static_cast<void>(measurement->addReference3D(Objs.at(iObject), Subs.at(iObject)));
        // cache the referenced object
        m_3dObjectCache.insert(Objs.at(iObject)->getNameInDocument());
        // cache the parent object if available.  Ideally, we would handle deletion
        // of a reference object in a slot for DocumentObject::signalDeletedObject,
        // but by the time we get the signal the document will have severed any links
        // between our object and its parents. So we need to cache the parent here while
        // we still have the link
        App::DocumentObject* firstParent = Objs.at(iObject)->getFirstParent();
        if (firstParent) {
            m_3dObjectCache.insert(firstParent->getNameInDocument());
        }
    }
}

//! check the effective references have the correct number and type for this
//! dimension.
bool DrawViewDimension::validateReferenceForm() const
{
   // we have either or both valid References3D and References2D
    ReferenceVector references = getEffectiveReferences();
    if (references.empty()) {
        return false;
    }

    if (Type.isValue("Distance") || Type.isValue("DistanceX") || Type.isValue("DistanceY")) {
        if (getRefType() == RefType::oneEdge) {
            if (references.size() != 1) {
                return false;
            }
            std::string subGeom = DrawUtil::getGeomTypeFromName(references.front().getSubName());
            return subGeom == "Edge";
        }
        if (getRefType() == RefType::twoEdge) {
            if (references.size() != 2) {
                return false;
            }
            std::string subGeom0 = DrawUtil::getGeomTypeFromName(references.front().getSubName());
            std::string subGeom1 = DrawUtil::getGeomTypeFromName(references.back().getSubName());
            return (subGeom0 == "Edge" && subGeom1 == "Edge");
        }

        if (getRefType() == RefType::twoVertex) {
            if (references.size() != 2) {
                return false;
            }
            std::string subGeom0 = DrawUtil::getGeomTypeFromName(references.front().getSubName());
            std::string subGeom1 = DrawUtil::getGeomTypeFromName(references.back().getSubName());
            return (subGeom0 == "Vertex" && subGeom1 == "Vertex");
        }

        if (getRefType() == RefType::vertexEdge) {
            if (references.size() != 2) {
                return false;
            }
            std::string subGeom0 = DrawUtil::getGeomTypeFromName(references.front().getSubName());
            std::string subGeom1 = DrawUtil::getGeomTypeFromName(references.back().getSubName());
            return ( (subGeom0 == "Vertex" && subGeom1 == "Edge") ||
                     (subGeom0 == "Edge" && subGeom1 == "Vertex") );
        }
    }

    if (Type.isValue("Radius")) {
        if (references.size() != 1) {
            return false;
        }
        std::string subGeom = DrawUtil::getGeomTypeFromName(references.front().getSubName());
        return subGeom == "Edge";
    }

    if (Type.isValue("Diameter")) {
        if (references.size() != 1) {
            return false;
        }
        std::string subGeom = DrawUtil::getGeomTypeFromName(references.front().getSubName());
        return (subGeom == "Edge");
    }

    if (Type.isValue("Angle")) {
        if (references.size() != 2) {
            return false;
        }
        std::string subGeom0 = DrawUtil::getGeomTypeFromName(references.front().getSubName());
        std::string subGeom1 = DrawUtil::getGeomTypeFromName(references.back().getSubName());
        return (subGeom0 == "Edge" && subGeom1 == "Edge");
    }

    if (Type.isValue("Angle3Pt")) {
        if (references.size() != 3) {
            return false;
        }
        std::string subGeom0 = DrawUtil::getGeomTypeFromName(references.at(0).getSubName());
        std::string subGeom1 = DrawUtil::getGeomTypeFromName(references.at(1).getSubName());
        std::string subGeom2 = DrawUtil::getGeomTypeFromName(references.at(2).getSubName());
        return (subGeom0 == "Vertex" &&  subGeom1 == "Vertex" && subGeom2 == "Vertex");
    }

    if (Type.isValue("Area")) {
        if (references.size() != 1) {
            return false;
        }
        std::string subGeom = DrawUtil::getGeomTypeFromName(references.front().getSubName());
        return (subGeom == "Face");
    }

    return false;
}

// delete all previous measurements
void DrawViewDimension::clear3DMeasurements()
{
    // set sublinklist to empty?
    measurement->clear();
}

void DrawViewDimension::dumpRefs2D(const char* text) const
{
    Base::Console().message("DUMP - %s\n", text);
    const std::vector<App::DocumentObject*>& objects = References2D.getValues();
    const std::vector<std::string>& subElements = References2D.getSubValues();
    auto objIt = objects.begin();
    auto subIt = subElements.begin();
    int i = 0;
    for (; objIt != objects.end(); objIt++, subIt++, i++) {
        Base::Console().message("DUMP - ref: %d object: %s subElement: %s\n",
                                i,
                                (*objIt)->getNameInDocument(),
                                (*subIt).c_str());
    }
}

// TODO: this should go into DrawUtil or ShapeUtil or ??
double DrawViewDimension::dist2Segs(Base::Vector3d s1,
                                    Base::Vector3d e1,
                                    Base::Vector3d s2,
                                    Base::Vector3d e2) const
{
    gp_Pnt start(s1.x, s1.y, 0.0);
    gp_Pnt end(e1.x, e1.y, 0.0);
    TopoDS_Vertex v1 = BRepBuilderAPI_MakeVertex(start);
    TopoDS_Vertex v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge1(v1, v2);
    TopoDS_Edge edge1 = makeEdge1.Edge();

    start = gp_Pnt(s2.x, s2.y, 0.0);
    end = gp_Pnt(e2.x, e2.y, 0.0);
    v1 = BRepBuilderAPI_MakeVertex(start);
    v2 = BRepBuilderAPI_MakeVertex(end);
    BRepBuilderAPI_MakeEdge makeEdge2(v1, v2);
    TopoDS_Edge edge2 = makeEdge2.Edge();

    BRepExtrema_DistShapeShape extss(edge1, edge2);
    if (!extss.IsDone()) {
        throw Base::RuntimeError("DVD::dist2Segs - BRepExtrema_DistShapeShape failed");
    }
    int count = extss.NbSolution();
    double minDist = 0.0;
    if (count != 0) {
        minDist = extss.Value();
    }  // TODO: else { explode }

    return minDist;
}

bool DrawViewDimension::leaderIntersectsArc(Base::Vector3d s, Base::Vector3d pointOnCircle)
{
    bool result = false;
    const std::vector<std::string>& subElements = References2D.getSubValues();
    int idx = DrawUtil::getIndexFromName(subElements[0]);
    TechDraw::BaseGeomPtr base = getViewPart()->getGeomByIndex(idx);
    if (base && base->getGeomType() == GeomType::ARCOFCIRCLE) {
        TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC>(base);
        if (aoc->intersectsArc(s, pointOnCircle)) {
            result = true;
        }
    }
    else if (base && base->getGeomType() == GeomType::BSPLINE) {
        TechDraw::BSplinePtr spline = std::static_pointer_cast<TechDraw::BSpline>(base);
        if (spline->isCircle()) {
            if (spline->intersectsArc(s, pointOnCircle)) {
                result = true;
            }
        }
    }

    return result;
}

void DrawViewDimension::saveArrowPositions(const Base::Vector2d positions[])
{
    if (!positions) {
        m_arrowPositions.first(Base::Vector3d(0.0, 0.0, 0.0));
        m_arrowPositions.second(Base::Vector3d(0.0, 0.0, 0.0));
    }
    else {
        double scale = getViewPart()->getScale();
        m_arrowPositions.first(Base::Vector3d(positions[0].x, positions[0].y, 0.0) / scale);
        m_arrowPositions.second(Base::Vector3d(positions[1].x, positions[1].y, 0.0) / scale);
    }
}

// return the 2d references as a ReferenceVector
ReferenceVector DrawViewDimension::getReferences2d() const
{
    const std::vector<App::DocumentObject*>& objects = References2D.getValues();
    const std::vector<std::string>& subElements = References2D.getSubValues();
    ReferenceVector refs2d;
    size_t refCount = objects.size();
    for (size_t i = 0; i < refCount; i++) {
        ReferenceEntry ref(objects.at(i), subElements.at(i));
        refs2d.push_back(ref);
    }
    return refs2d;
}

// return the 3d references as a ReferenceVector
ReferenceVector DrawViewDimension::getReferences3d() const
{
    const std::vector<App::DocumentObject*>& objects3d = References3D.getValues();
    const std::vector<std::string>& subElements3d = References3D.getSubValues();
    ReferenceVector refs3d;
    size_t refCount = objects3d.size();
    for (size_t i = 0; i < refCount; i++) {
        ReferenceEntry ref(objects3d.at(i), subElements3d.at(i));
        refs3d.push_back(ref);
    }
    return refs3d;
}


// return position within parent view of dimension arrow heads/dimline endpoints
// note positions are in apparent coord (inverted y).
pointPair DrawViewDimension::getArrowPositions()
{
    return m_arrowPositions;
}

bool DrawViewDimension::has2DReferences() const
{
    const std::vector<App::DocumentObject*>& objects = References2D.getValues();
    const std::vector<std::string>& subNames = References2D.getSubValues();
    if (objects.empty()) {
        // we don't even have a DVP
        return false;
    }

    if (subNames.front().empty()) {
        // this is ok, as we must have a null string entry to balance DVP in first object position
        return true;
    }

    // we have a reference to a DVP and at least 1 subName entry, so we have 2d references
    return true;
}

// there is no special structure to 3d references, so anything > 0 is good
bool DrawViewDimension::has3DReferences() const
{
    return (References3D.getSize() > 0);
}

// has arbitrary or nonzero tolerance
bool DrawViewDimension::hasOverUnderTolerance() const
{
    return (ArbitraryTolerances.getValue() ||
            !DrawUtil::fpCompare(OverTolerance.getValue(), 0.0) ||
            !DrawUtil::fpCompare(UnderTolerance.getValue(), 0.0));
}

bool DrawViewDimension::showUnits() const
{
    return ShowUnits.getValue();
}

bool DrawViewDimension::useDecimals() const
{
    return Preferences::useGlobalDecimals();
}

std::string DrawViewDimension::getPrefixForDimType() const
{
    if (Type.isValue("Radius")) {
        return "R";
    }

    if (Type.isValue("Diameter")) {
        return std::string(Preferences::getPreferenceGroup("Dimensions")
                               ->GetASCII("DiameterSymbol", "\xe2\x8c\x80"));  // Diameter symbol
    }

    return "";
}

std::string DrawViewDimension::getDefaultFormatSpec(bool isToleranceFormat) const
{
    return m_formatter->getDefaultFormatSpec(isToleranceFormat);
}

bool DrawViewDimension::isExtentDim() const
{
    constexpr int DimExtentLength{9};
    std::string name(getNameInDocument());
    return (name.substr(0, DimExtentLength) == "DimExtent");
}


PyObject* DrawViewDimension::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewDimensionPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}


//! store the corners of this dimension's base view for use by phase 2 of the auto correct process.
void DrawViewDimension::saveFeatureBox()
{
    std::vector<Base::Vector3d> bbxCorners;
    auto bbx = getFeatureBox();
    bbxCorners.push_back(bbx.GetMinimum());
    bbxCorners.push_back(bbx.GetMaximum());
    BoxCorners.setValues(bbxCorners);
}

Base::BoundBox3d DrawViewDimension::getSavedBox() const
{
    std::vector<Base::Vector3d> bbxCorners = BoxCorners.getValues();
    if (bbxCorners.empty()) {
        // need to advise caller if BoxCorners not filled in yet.  zero length
        // diagonal?
        Base::Console().message("DVD::getSavedBox - no corners!\n");
        return Base::BoundBox3d();
    }
    return Base::BoundBox3d(bbxCorners.front().x,
                            bbxCorners.front().y,
                            bbxCorners.front().z,
                            bbxCorners.back().x,
                            bbxCorners.back().y,
                            bbxCorners.back().z);
}

Base::BoundBox3d DrawViewDimension::getFeatureBox() const
{
    if (getViewPart() && getViewPart()->getBoundingBox().IsValid()) {
        return getViewPart()->getBoundingBox();
    }
    return Base::BoundBox3d();
}
