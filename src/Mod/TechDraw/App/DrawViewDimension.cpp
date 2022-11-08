/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <QLocale>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QStringList>
#include <BRepAdaptor_Curve.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Tool.hxx>
#include <gp_Pnt.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Quantity.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Mod/Measure/App/Measurement.h>
//#include <Mod/Part/App/PartFeature.h>

#include <Mod/TechDraw/App/DrawViewDimensionPy.h>  // generated from DrawViewDimensionPy.xml

#include "DrawViewDimension.h"
//#include "DimensionGeometry.h"
//#include "DimensionReferences.h"
#include "DrawUtil.h"
#include "DrawViewPart.h"
#include "Geometry.h"
#include "Preferences.h"

using namespace TechDraw;
using DU = DrawUtil;

//===========================================================================
// DrawViewDimension
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewDimension, TechDraw::DrawView)

//namespace {
//    // keep this enum synchronized with TypeEnums
//    enum DimensionType {
//        Distance,
//        DistanceX,
//        DistanceY,
//        DistanceZ,
//        Radius,
//        Diameter,
//        Angle,
//        Angle3Pt
//    };
//}

const char* DrawViewDimension::TypeEnums[]= {"Distance",
                                             "DistanceX",
                                             "DistanceY",
                                             "DistanceZ",
                                             "Radius",
                                             "Diameter",
                                             "Angle",
                                             "Angle3Pt",
                                             nullptr};

const char* DrawViewDimension::MeasureTypeEnums[]= {"True",
                                                    "Projected",
                                                    nullptr};

// constraint to set the step size to 0.1
static const App::PropertyQuantityConstraint::Constraints ToleranceConstraint = { -DBL_MAX, DBL_MAX, 0.1 };
// constraint to force positive values
static const App::PropertyQuantityConstraint::Constraints PositiveConstraint = { 0.0, DBL_MAX, 0.1 };

DrawViewDimension::DrawViewDimension()
{
    ADD_PROPERTY_TYPE(References2D, (nullptr, nullptr), "", (App::Prop_None), "Projected Geometry References");
    References2D.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(References3D, (nullptr, nullptr), "", (App::Prop_None), "3D Geometry References");
    References3D.setScope(App::LinkScope::Global);

    ADD_PROPERTY_TYPE(FormatSpec, (getDefaultFormatSpec()), "Format", App::Prop_Output, "Dimension format");
    ADD_PROPERTY_TYPE(FormatSpecOverTolerance, (getDefaultFormatSpec(true)), "Format", App::Prop_Output, "Dimension overtolerance format");
    ADD_PROPERTY_TYPE(FormatSpecUnderTolerance, (getDefaultFormatSpec(true)), "Format", App::Prop_Output, "Dimension undertolerance format");
    ADD_PROPERTY_TYPE(Arbitrary, (false), "Format", App::Prop_Output, "Value overridden by user");
    ADD_PROPERTY_TYPE(ArbitraryTolerances, (false), "Format", App::Prop_Output, "Tolerance values overridden by user");

    Type.setEnums(TypeEnums);                                         //dimension type: length, radius etc
    ADD_PROPERTY(Type, ((long)0));
    MeasureType.setEnums(MeasureTypeEnums);
    ADD_PROPERTY(MeasureType, ((long)1));                             //Projected (or True) measurement
    ADD_PROPERTY_TYPE(TheoreticalExact, (false), "", App::Prop_Output, "If theoretical exact (basic) dimension");
    ADD_PROPERTY_TYPE(EqualTolerance, (true), "", App::Prop_Output, "If over- and undertolerance are equal");

    ADD_PROPERTY_TYPE(OverTolerance, (0.0), "", App::Prop_Output, "Overtolerance value\nIf 'Equal Tolerance' is true this is also\nthe negated value for 'Under Tolerance'");
    OverTolerance.setUnit(Base::Unit::Length);
    OverTolerance.setConstraints(&ToleranceConstraint);
    ADD_PROPERTY_TYPE(UnderTolerance, (0.0), "", App::Prop_Output, "Undertolerance value\nIf 'Equal Tolerance' is true it will be replaced\nby negative value of 'Over Tolerance'");
    UnderTolerance.setUnit(Base::Unit::Length);
    UnderTolerance.setConstraints(&ToleranceConstraint);
    ADD_PROPERTY_TYPE(Inverted, (false), "", App::Prop_Output, "The dimensional value is displayed inverted");

    ADD_PROPERTY_TYPE(AngleOverride, (false), "Override", App::Prop_Output, "User specified angles");
    ADD_PROPERTY_TYPE(LineAngle, (0.0), "Override", App::Prop_Output, "Dimension line angle");
    ADD_PROPERTY_TYPE(ExtensionAngle, (0.0), "Override", App::Prop_Output, "Extension line angle");

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

    measurement = new Measure::Measurement();
    //TODO: should have better initial datumLabel position than (0, 0) in the DVP?? something closer to the object being measured?

    //initialize the descriptive geometry.
    //TODO: should this be more like DVP with a "geometry object"?
    resetLinear();
    resetAngular();
    resetArc();
    m_hasGeometry = false;
}

DrawViewDimension::~DrawViewDimension()
{
    delete measurement;
    measurement = nullptr;
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
    m_anglePoints.vertex(Base::Vector3d(0, 0,0));
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

void DrawViewDimension::onChanged(const App::Property* prop)
{

    if (prop == &References3D) {
        //have to rebuild the Measurement object
        clear3DMeasurements();                             //Measurement object
        if (!(References3D.getValues()).empty()) {
            setAll3DMeasurement();
        }
    }
    if (isRestoring()) {
        DrawView::onChanged(prop);
        return;
    }

    if (prop == &References3D) {   //have to rebuild the Measurement object
        clear3DMeasurements();                             //Measurement object
        if (!(References3D.getValues()).empty()) {
            setAll3DMeasurement();
        } else if (MeasureType.isValue("True")) { //empty 3dRefs, but True
            MeasureType.touch();                       //run MeasureType logic for this case
        }
    }
    else if (prop == &Type) {                                    //why??
        FormatSpec.setValue(getDefaultFormatSpec().c_str());

        DimensionType type = static_cast<DimensionType>(Type.getValue());
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
        requestPaint();
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
        requestPaint();
    }
    else if (prop == &OverTolerance) {
        // if EqualTolerance set negated overtolerance for untertolerance
        if (EqualTolerance.getValue()) {
            UnderTolerance.setValue(-1.0 * OverTolerance.getValue());
            UnderTolerance.setUnit(OverTolerance.getUnit());
        }
        requestPaint();
    }
    else if (prop == &FormatSpecOverTolerance) {
        if (!ArbitraryTolerances.getValue()) {
            FormatSpecUnderTolerance.setValue(FormatSpecOverTolerance.getValue());
        }
        requestPaint();
    }
    else if (prop == &FormatSpecUnderTolerance) {
        if (!ArbitraryTolerances.getValue()) {
            FormatSpecOverTolerance.setValue(FormatSpecUnderTolerance.getValue());
        }
        requestPaint();
    }
    else if ( (prop == &FormatSpec) ||
         (prop == &Arbitrary) ||
         (prop == &ArbitraryTolerances) ||
         (prop == &MeasureType) ||
         (prop == &UnderTolerance) ||
         (prop == &Inverted) ) {
        requestPaint();
    }

    DrawView::onChanged(prop);
}

void DrawViewDimension::Restore(Base::XMLReader& reader)
// Old drawings did not have the equal tolerance options.
// We cannot just introduce it as being set to true because that would e.g. destroy tolerances like +1-2
// Therefore set it to false for existing documents
{
    EqualTolerance.setValue(false);
    DrawView::Restore(reader);
}

void DrawViewDimension::onDocumentRestored()
{
    if (has3DReferences()) {
        setAll3DMeasurement();
    }

    DimensionType type = static_cast<DimensionType>(Type.getValue());
    if (type == DimensionType::Angle || type == DimensionType::Angle3Pt) {
        OverTolerance.setUnit(Base::Unit::Angle);
        UnderTolerance.setUnit(Base::Unit::Angle);
    }
}

void DrawViewDimension::handleChangedPropertyType(Base::XMLReader &reader, const char * TypeName, App::Property * prop)
{
    if (prop == &OverTolerance && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat v;
        v.Restore(reader);
        OverTolerance.setValue(v.getValue());
    }
    else if (prop == &UnderTolerance && strcmp(TypeName, "App::PropertyFloat") == 0) {
        App::PropertyFloat v;
        v.Restore(reader);
        UnderTolerance.setValue(v.getValue());
    }
    else {
        TechDraw::DrawView::handleChangedPropertyType(reader, TypeName, prop);
    }

    // Over/Undertolerance were further changed from App::PropertyQuantity to App::PropertyQuantityConstraint
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
        if (
            References2D.isTouched() ||
            Type.isTouched() ||
            FormatSpec.isTouched() ||
            Arbitrary.isTouched() ||
            FormatSpecOverTolerance.isTouched() ||
            FormatSpecUnderTolerance.isTouched() ||
            ArbitraryTolerances.isTouched() ||
            MeasureType.isTouched() ||
            TheoreticalExact.isTouched() ||
            EqualTolerance.isTouched() ||
            OverTolerance.isTouched() ||
            UnderTolerance.isTouched() ||
            Inverted.isTouched()
        ) {
            return true;
        }
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawViewDimension::execute()
{
//    Base::Console().Message("DVD::execute() - %s\n", getNameInDocument());
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    DrawViewPart* dvp = getViewPart();
    if (!dvp)
        return App::DocumentObject::StdReturn;

    if (!has2DReferences() && !has3DReferences()) {
        //no references, cant' do anything
        return App::DocumentObject::StdReturn;
    }

    if (!getViewPart()->hasGeometry()) {
        //can't do anything until Source has geometry
        return App::DocumentObject::StdReturn;
    }

    if (References3D.getValues().empty() &&
        !checkReferences2D()) {
        Base::Console().Warning("DVD::execute - %s has invalid 2D References\n", getNameInDocument());
        return App::DocumentObject::StdReturn;
    }

    //we have either or both valid References3D and References2D
    ReferenceVector references = getEffectiveReferences();

    resetLinear();
    resetAngular();
    resetArc();

    if ( Type.isValue("Distance")  ||
         Type.isValue("DistanceX") ||
         Type.isValue("DistanceY") )  {
        if (getRefType() == oneEdge) {
            m_linearPoints = getPointsOneEdge(references);
        } else if (getRefType() == twoEdge) {
            m_linearPoints = getPointsTwoEdges(references);
        } else if (getRefType() == twoVertex) {
            m_linearPoints = getPointsTwoVerts(references);
        } else if (getRefType() == vertexEdge) {
            m_linearPoints = getPointsEdgeVert(references);
        }
        m_hasGeometry = true;
    } else if (Type.isValue("Radius")){
        m_arcPoints = getArcParameters(references);
        m_hasGeometry = true;
    } else if (Type.isValue("Diameter")){
        m_arcPoints = getArcParameters(references);
        m_hasGeometry = true;
    } else if (Type.isValue("Angle")){
        if (getRefType() != twoEdge) {
            throw Base::RuntimeError("Angle dimension has non-edge references");
        }
        m_anglePoints = getAnglePointsTwoEdges(references);
        m_hasGeometry = true;
    } else if (Type.isValue("Angle3Pt")){
        if (getRefType() != threeVertex) {
            throw Base::RuntimeError("3 point angle dimension has non-vertex references");
        }
        m_anglePoints = getAnglePointsThreeVerts(references);
        m_hasGeometry = true;
    }

    overrideKeepUpdated(false);
    return DrawView::execute();
}

//TODO: schema not report their multiValue status
bool DrawViewDimension::isMultiValueSchema() const
{
    bool angularMeasure = (Type.isValue("Angle") || Type.isValue("Angle3Pt"));

    Base::UnitSystem uniSys = Base::UnitsApi::getSchema();
    if (uniSys == Base::UnitSystem::ImperialBuilding &&
            !angularMeasure) {
        return true;
    } else if (uniSys == Base::UnitSystem::ImperialCivil &&
               !angularMeasure) {
        return true;
    }
    return false;
}

std::string DrawViewDimension::getBaseLengthUnit(Base::UnitSystem system)
{
    switch (system) {
    case Base::UnitSystem::SI1:
        return "mm";
    case Base::UnitSystem::SI2:
        return "m";
    case Base::UnitSystem::Imperial1:
        return "in";
    case Base::UnitSystem::ImperialDecimal:
        return "in";
    case Base::UnitSystem::Centimeters:
        return "cm";
    case Base::UnitSystem::ImperialBuilding:
        return "ft";
    case Base::UnitSystem::MmMin:
        return "mm";
    case Base::UnitSystem::ImperialCivil:
        return "ft";
    case Base::UnitSystem::FemMilliMeterNewton:
        return "mm";
    default:
        return "Unknown schema";
    }
}

std::string DrawViewDimension::formatValue(qreal value,
                                           QString qFormatSpec,
                                           int partial,
                                           bool isDim)
{
    QString qUserStringUnits;
    QString formattedValue;
    bool angularMeasure = false;
    QLocale loc;

    Base::Quantity asQuantity;
    asQuantity.setValue(value);
    if ( (Type.isValue("Angle")) ||
         (Type.isValue("Angle3Pt")) ) {
        angularMeasure = true;
        asQuantity.setUnit(Base::Unit::Angle);
    } else {
        asQuantity.setUnit(Base::Unit::Length);
    }

    QString qUserString = asQuantity.getUserString();  // this handles mm to inch/km/parsec etc
                                                       // and decimal positions but won't give more than
                                                       // Global_Decimals precision

    //units api: get schema to figure out if this is multi-value schema(Imperial1, ImperialBuilding, etc)
    //if it is multi-unit schema, don't even try to use Alt Decimals
    Base::UnitSystem unitSystem = Base::UnitsApi::getSchema();

    // we need to know what length unit is used by the scheme
    std::string BaseLengthUnit = getBaseLengthUnit(unitSystem);

    //get formatSpec prefix/suffix/specifier
    QStringList qsl = getPrefixSuffixSpec(qFormatSpec);
    QString formatPrefix    = qsl[0];   //FormatSpec prefix
    QString formatSuffix    = qsl[1];   //FormatSpec suffix
    QString formatSpecifier = qsl[2];   //FormatSpec specifier

    //handle multi value schemes (yd/ft/in, dms, etc)
    std::string genPrefix = getPrefix();     //general prefix - diameter, radius, etc
    QString qMultiValueStr;
    QString qGenPrefix = QString::fromUtf8(genPrefix.data(), genPrefix.size());
    if ((unitSystem == Base::UnitSystem::ImperialCivil) && angularMeasure) {
        QString dispMinute = QString::fromUtf8("\'");
        QString dispSecond = QString::fromUtf8("\"");
        QString schemeMinute = QString::fromUtf8("M");
        QString schemeSecond = QString::fromUtf8("S");
        QString displaySub = qUserString.replace(schemeMinute, dispMinute);
        displaySub = displaySub.replace(schemeSecond, dispSecond);
        qMultiValueStr = displaySub;
        if (!genPrefix.empty()) {
            // prefix + 48*30'30" + suffix
            qMultiValueStr = formatPrefix + qGenPrefix + displaySub + formatSuffix;
        }
        formattedValue = qMultiValueStr;
    } else if (isMultiValueSchema()) {
        qMultiValueStr = qUserString;
        if (!genPrefix.empty()) {
            //qUserString from Quantity includes units - prefix + R + nnn ft + suffix
            qMultiValueStr = formatPrefix + qUserString + formatSuffix;
        }
        return qMultiValueStr.toStdString();
    } else {  //not multivalue schema
        if (formatSpecifier.isEmpty()) {
            Base::Console().Warning("Warning - no numeric format in Format Spec %s - %s\n",
                                    qPrintable(qFormatSpec), getNameInDocument());
            return Base::Tools::toStdString(qFormatSpec);
        }

        // for older TD drawings the formatSpecifier "%g" was used, but the number of decimals was
        // neverheless limited. To keep old drawings, we limit the number of decimals too
        // if the TD preferences option to use the global decimal number is set
        // the formatSpecifier can have a prefix and/or suffix
        if (useDecimals() && formatSpecifier.contains(QString::fromLatin1("%g"), Qt::CaseInsensitive)) {
                int globalPrecision = Base::UnitsApi::getDecimals();
                // change formatSpecifier to e.g. "%.2f"
                QString newSpecifier = QString::fromStdString("%." + std::to_string(globalPrecision) + "f");
                formatSpecifier.replace(QString::fromLatin1("%g"), newSpecifier, Qt::CaseInsensitive);
        }

        // qUserString is the value + unit with default decimals, so extract the unit
        // we cannot just use unit.getString() because this would convert '°' to 'deg'
        QRegularExpression rxUnits(QString::fromUtf8(" \\D*$")); // space + any non digits at end of string
        QRegularExpressionMatch rxMatch;
        int pos = qUserString.indexOf(rxUnits, 0, &rxMatch);
        if (pos != -1) {
            qUserStringUnits = rxMatch.captured(0); // entire capture - non numerics at end of qUserString
        }

        // get value in the base unit with default decimals
        // for the conversion we use the same method as in DlgUnitsCalculator::valueChanged
        // get the conversion factor for the unit
        double convertValue = Base::Quantity::parse(QString::fromLatin1("1") + QString::fromStdString(BaseLengthUnit)).getValue();
        // the result is now just val / convertValue because val is always in the base unit
        // don't do this for angular values since they are not in the BaseLengthUnit
        double userVal;
        if (!angularMeasure) {
            userVal = asQuantity.getValue() / convertValue;
            // since we converted to the BaseLengthUnit we must assure it is also used for qUserStringUnits
            qUserStringUnits = QChar::fromLatin1(' ') + QString::fromStdString(BaseLengthUnit);
        }
        else {
            userVal = asQuantity.getValue();
        }

        // we reformat the value
        // the user can overwrite the decimal settings, so we must in every case use the formatSpecifier
        // the default is: if useDecimals(), then formatSpecifier = global decimals, otherwise it is %.2f
        // Also, handle the new non-standard format-specifier '%w', which has the following rules: works as %f, but no trailing zeros
        if (formatSpecifier.contains(QRegularExpression(QStringLiteral("%.*[wW]")))) {
            QString fs = formatSpecifier;
            fs.replace(QRegularExpression(QStringLiteral("%(.*)w")), QStringLiteral("%\\1f"));
            fs.replace(QRegularExpression(QStringLiteral("%(.*)W")), QStringLiteral("%\\1F"));
            formattedValue = QString::asprintf(Base::Tools::toStdString(fs).c_str(), userVal);
            // First, try to cut trailing zeros, if AFTER decimal dot there are nonzero numbers
            // Second, try to cut also decimal dot and zeros, if there are just zeros after it
            formattedValue.replace(QRegularExpression(QStringLiteral("([0-9][0-9]*\\.[0-9]*[1-9])00*$")), QStringLiteral("\\1"));
            formattedValue.replace(QRegularExpression(QStringLiteral("([0-9][0-9]*)\\.0*$")), QStringLiteral("\\1"));
        } else {
            formattedValue = QString::asprintf(Base::Tools::toStdString(formatSpecifier).c_str(), userVal);
        }

        // if abs(1 - userVal / formattedValue) > 0.1 we know that we make an error greater than 10%
        // then we need more digits
        if (abs(userVal - formattedValue.toDouble()) > 0.1 * abs(userVal)) {
            int i = 1;
            do { // increase decimals step by step until error is < 10 %
                formattedValue = QLocale().toString(userVal, 'f', i);
                ++i;
            } while (abs(userVal - loc.toDouble(formattedValue)) > 0.1 * abs(userVal));
            // We purposely don't reset the formatSpecifier.
            // Why "%.1f" is overwritten for a value of e.g. "0.001" is obvious,
            // moreover such cases only occurs when
            // changing unit schemes on existing drawings. Moreover a typical case is that
            // you accidentally used e.g. a building scheme, see your mistake and go back
            // then you would end up with e.g. "%.5f" and must manually correct this.
        }

        // replace decimal sign if necessary
        QChar dp = QChar::fromLatin1('.');
        if (loc.decimalPoint() != dp) {
            formattedValue.replace(dp, loc.decimalPoint());
        }
    }


    std::string formattedValueString = formattedValue.toStdString();

    if (partial == 0) {   //full text for multi-value schemas
        return Base::Tools::toStdString(formatPrefix) +
            Base::Tools::toStdString(qMultiValueStr) +
            Base::Tools::toStdString(formatSuffix) +
            Base::Tools::toStdString(qUserStringUnits);
    }
    else if (partial == 1)  {            // prefix number[unit] suffix
            // remove space between dimension and ° (U+00B0)
            // other units need 1 space for readability
            if ( angularMeasure &&
                 !qUserStringUnits.contains(QString::fromLatin1("deg")) ) {
                QRegularExpression space(QString::fromUtf8("\\s"));
                qUserStringUnits.remove(space);
            }
            if (angularMeasure) {
                //always insert unit after value
                return Base::Tools::toStdString(formatPrefix) +
                         formattedValueString +
                         Base::Tools::toStdString(qUserStringUnits) +
                         Base::Tools::toStdString(formatSuffix);
            } else if (showUnits()){
                if (isDim && haveTolerance()) {
                    //unit will be included in tolerance so don't repeat it here
                    return Base::Tools::toStdString(formatPrefix) +
                             formattedValueString +
                             Base::Tools::toStdString(formatSuffix);
                } else {
                    //no tolerance, so we need to include unit
                    return Base::Tools::toStdString(formatPrefix) +
                             formattedValueString +
                             Base::Tools::toStdString(qUserStringUnits) +
                             Base::Tools::toStdString(formatSuffix);
                }
            } else {
                return Base::Tools::toStdString(formatPrefix) +
                         formattedValueString +
                         Base::Tools::toStdString(formatSuffix);
            }
    }
    else if (partial == 2) {             // just the unit
        if (angularMeasure) {
            // remove leading space from unit if unit is not "deg"
            if ( !qUserStringUnits.contains(QString::fromLatin1("deg")) ) {
                QRegularExpression space(QString::fromUtf8("\\s"));
                qUserStringUnits.remove(space);
            }
            return Base::Tools::toStdString(qUserStringUnits);
        } else if (showUnits()) {
            return Base::Tools::toStdString(qUserStringUnits);
        } else {
            return "";
        }
    }

    return formattedValueString;
}

bool DrawViewDimension::haveTolerance()
{
    //if a numeric tolerance is specified AND
    //tolerances are NOT arbitrary
    if ((!DrawUtil::fpCompare(OverTolerance.getValue(), 0.0) ||
        !DrawUtil::fpCompare(UnderTolerance.getValue(), 0.0)) &&
        !ArbitraryTolerances.getValue()){
        return true;
    }
    return false;
}

std::string DrawViewDimension::getFormattedToleranceValue(int partial)
{
    QString FormatSpec = QString::fromUtf8(FormatSpecOverTolerance.getStrValue().data());
    QString ToleranceString;

    if (ArbitraryTolerances.getValue())
        ToleranceString = FormatSpec;
    else
        ToleranceString = QString::fromUtf8(formatValue(OverTolerance.getValue(),
                                                        FormatSpec,
                                                        partial,
                                                        false).c_str());

    return ToleranceString.toStdString();
}

//get over and under tolerances
std::pair<std::string, std::string> DrawViewDimension::getFormattedToleranceValues(int partial)
{
    QString underFormatSpec = QString::fromUtf8(FormatSpecUnderTolerance.getStrValue().data());
    QString overFormatSpec = QString::fromUtf8(FormatSpecOverTolerance.getStrValue().data());
    std::pair<std::string, std::string> tolerances;
    QString underTolerance, overTolerance;

    if (ArbitraryTolerances.getValue()) {
        underTolerance = underFormatSpec;
        overTolerance = overFormatSpec;
    } else {
        if (DrawUtil::fpCompare(UnderTolerance.getValue(), 0.0)) {
            underTolerance = QString::fromUtf8(formatValue(UnderTolerance.getValue(),
                                                           QString::fromUtf8("%.0f"),
                                                           partial,
                                                           false).c_str());
        }
        else {
            underTolerance = QString::fromUtf8(formatValue(UnderTolerance.getValue(),
                                                           underFormatSpec,
                                                           partial,
                                                           false).c_str());
        }
        if (DrawUtil::fpCompare(OverTolerance.getValue(), 0.0)) {
            overTolerance = QString::fromUtf8(formatValue(OverTolerance.getValue(),
                                                          QString::fromUtf8("%.0f"),
                                                          partial,
                                                          false).c_str());
        }
        else {
            overTolerance = QString::fromUtf8(formatValue(OverTolerance.getValue(),
                                                          overFormatSpec,
                                                          partial,
                                                          false).c_str());
        }
    }

    tolerances.first = underTolerance.toStdString();
    tolerances.second = overTolerance.toStdString();

    return tolerances;
}

//partial = 2 unit only
std::string DrawViewDimension::getFormattedDimensionValue(int partial)
{
    QString qFormatSpec = QString::fromUtf8(FormatSpec.getStrValue().data());

    if ( (Arbitrary.getValue() && !EqualTolerance.getValue())
        || (Arbitrary.getValue() && TheoreticalExact.getValue()) ) {
        return FormatSpec.getStrValue();
    }

    if (Arbitrary.getValue()) {
        return FormatSpec.getStrValue();
    }

    // if there is an equal over-/undertolerance (so only 1 tolerance to show with +/-) and
    // not theoretically exact (which has no tolerance), and
    // tolerance has been specified, ie
    // (OverTolerance != 0.0 (so a tolerance has been specified) or
    // ArbitraryTolerances are specified)
    // concatenate the tolerance to dimension
    if (EqualTolerance.getValue() &&
        !TheoreticalExact.getValue() &&
        (!DrawUtil::fpCompare(OverTolerance.getValue(), 0.0) || ArbitraryTolerances.getValue())) {
        QString labelText = QString::fromUtf8(formatValue(getDimValue(),
                                                          qFormatSpec,
                                                          1,
                                                          true).c_str()); //just the number pref/spec[unit]/suf
        QString unitText = QString::fromUtf8(formatValue(getDimValue(),
                                                         qFormatSpec,
                                                         2,
                                                         false).c_str()); //just the unit
        QString tolerance = QString::fromStdString(getFormattedToleranceValue(1).c_str());

        // tolerance might start with a plus sign that we don't want, so cut it off
        // note plus sign is not at pos = 0!
        QRegularExpression plus(QString::fromUtf8("^\\s*\\+"));
        tolerance.remove(plus);

        return (labelText +
                 QString::fromUtf8(" \xC2\xB1 ") +          // +/- symbol
                 tolerance).toStdString();

        if (partial == 2) {
            return unitText.toStdString();
        }

        return "";
    }

    //tolerance not specified, so just format dimension value?
    std::string formattedValue = formatValue(getDimValue(), qFormatSpec, partial, true);

    return formattedValue;
}

QStringList DrawViewDimension::getPrefixSuffixSpec(QString fSpec)
{
    QStringList result;
    //find the %x.y tag in FormatSpec
    QRegularExpression rxFormat(QStringLiteral("%[+-]?[0-9]*\\.*[0-9]*[aefgwAEFGW]")); //printf double format spec
    QRegularExpressionMatch rxMatch;
    int pos = fSpec.indexOf(rxFormat, 0, &rxMatch);
    if (pos != -1)  {
        QString match = rxMatch.captured(0);                                  //entire capture of rx
        QString formatPrefix = fSpec.left(pos);
        result.append(formatPrefix);
        QString formatSuffix = fSpec.right(fSpec.size() - pos - match.size());
        result.append(formatSuffix);
        result.append(match);
    } else {       //printf format not found!
        Base::Console().Warning("Warning - no numeric format in formatSpec %s - %s\n",
                                qPrintable(fSpec), getNameInDocument());
        result.append(QString());
        result.append(QString());
        result.append(fSpec);
    }
    return result;
}

//!NOTE: this returns the Dimension value in internal units (ie mm)!!!!
double DrawViewDimension::getDimValue()
{
//    Base::Console().Message("DVD::getDimValue()\n");
    double result = 0.0;
    if (!has2DReferences() && !has3DReferences()) {
        //nothing to measure
        return result;
    }
    if  (!getViewPart())
        return result;

    if  (!getViewPart()->hasGeometry())            //happens when loading saved document
        return result;

    if (MeasureType.isValue("True")) {
        // True Values
        if (!measurement->has3DReferences()) {
            Base::Console().Warning("%s - True dimension has no 3D References\n", getNameInDocument());
            return result;
        }
        if ( Type.isValue("Distance")  ||
             Type.isValue("DistanceX") ||
             Type.isValue("DistanceY") )  {
            result = measurement->length();
        } else if (Type.isValue("Radius")){
            result = measurement->radius();
        } else if (Type.isValue("Diameter")){
            result = 2.0 * measurement->radius();
        } else if (Type.isValue("Angle") ||
                   Type.isValue("Angle3Pt") ) {
            result = measurement->angle();
        } else {  //tarfu
            throw Base::ValueError("getDimValue() - Unknown Dimension Type (3)");
        }
    } else {
        // Projected Values
        if (!checkReferences2D()) {
            Base::Console().Warning("DVD::getDimValue - %s - 2D references are corrupt (5)\n", getNameInDocument());
            return result;
        }
        if ( Type.isValue("Distance")  ||
             Type.isValue("DistanceX") ||
             Type.isValue("DistanceY") )  {
            pointPair pts = getLinearPoints();
            Base::Vector3d dimVec = pts.first() - pts.second();
            if (Type.isValue("Distance")) {
                result = dimVec.Length() / getViewPart()->getScale();
            } else if (Type.isValue("DistanceX")) {
                result = fabs(dimVec.x) / getViewPart()->getScale();
            } else {
                result = fabs(dimVec.y) / getViewPart()->getScale();
            }
        } else if (Type.isValue("Radius")){
            arcPoints pts = m_arcPoints;
            result = pts.radius / getViewPart()->getScale();            //Projected BaseGeom is scaled for drawing
        } else if (Type.isValue("Diameter")){
            arcPoints pts = m_arcPoints;
            result = (pts.radius  * 2.0) / getViewPart()->getScale();   //Projected BaseGeom is scaled for drawing
        } else if (Type.isValue("Angle") ||
                   Type.isValue("Angle3Pt")) { //same as case "Angle"?
            anglePoints pts = m_anglePoints;
            Base::Vector3d vertex = pts.vertex();
            Base::Vector3d leg0 = pts.first() - vertex;
            Base::Vector3d leg1 = pts.second() - vertex;
            double legAngle =  leg0.GetAngle(leg1) * 180.0 / M_PI;
            result = legAngle;
        }
    }

    result = fabs(result);
    if (Inverted.getValue()) {
        if (Type.isValue("Angle") || Type.isValue("Angle3Pt")) {
            result = 360 - result;
        }
        else {
            result = -result;
        }
    }
    return result;
}

pointPair DrawViewDimension::getPointsOneEdge(ReferenceVector references)
{
//    Base::Console().Message("DVD::getPointsOneEdge()\n");
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement = DrawUtil::getIndexFromName(references.front().getSubName());
    if (refObject->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
            !references.at(0).getSubName().empty()) {
        //TODO: Notify if not straight line Edge?
        //this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr geom = getViewPart()->getGeomByIndex(iSubelement);
        if (!geom || geom->geomType != TechDraw::GeomType::GENERIC) {
            throw Base::RuntimeError("Missing geometry for dimension");
        }
        TechDraw::GenericPtr generic = std::static_pointer_cast<TechDraw::Generic>(geom);
        return { generic->points[0], generic->points[1] };
    }

    //this is a 3d object
    //get the endpoints of the edge in the DVP's coordinates
    Base::Vector3d edgeEnd0, edgeEnd1;
    TopoDS_Shape geometry = references.front().getGeometry();
    if (geometry.IsNull() ||
        geometry.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    const TopoDS_Edge& edge = TopoDS::Edge(geometry);
    gp_Pnt gEnd0 = BRep_Tool::Pnt(TopExp::FirstVertex(edge));
    gp_Pnt gEnd1 = BRep_Tool::Pnt(TopExp::LastVertex(edge));
    gp_Pnt gCentroid = DrawUtil::togp_Pnt(getViewPart()->getOriginalCentroid());
    gEnd0 = gp_Pnt(gEnd0.XYZ() - gCentroid.XYZ());
    gEnd1 = gp_Pnt(gEnd1.XYZ() - gCentroid.XYZ());
    //project points onto paperplane, centered, scaled, rotated and inverted
    edgeEnd0 = getViewPart()->projectPoint(DrawUtil::toVector3d(gEnd0)) * getViewPart()->getScale();
    edgeEnd1 = getViewPart()->projectPoint(DrawUtil::toVector3d(gEnd1)) * getViewPart()->getScale();

    return { edgeEnd0, edgeEnd1 };
}

pointPair DrawViewDimension::getPointsTwoEdges(ReferenceVector references)
{
//    Base::Console().Message("DVD::getPointsTwoEdges() - %s\n", getNameInDocument());
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    if (refObject->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
        !references.at(0).getSubName().empty()) {
        //this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr geom0 = getViewPart()->getGeomByIndex(iSubelement0);
        TechDraw::BaseGeomPtr geom1 = getViewPart()->getGeomByIndex(iSubelement1);
        if (!geom0 || !geom1) {
            throw Base::RuntimeError("Missing geometry for dimension");
        }
        return closestPoints(geom0->occEdge, geom1->occEdge);
    }

    //this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() ||
        geometry0.ShapeType() != TopAbs_EDGE || geometry1.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }

    pointPair pts = closestPoints(geometry0, geometry1);
    pts.move(getViewPart()->getOriginalCentroid());
    pts.project(getViewPart());
    pts.mapToPage(getViewPart());
    pts.invertY();
    return pts;
}

pointPair DrawViewDimension::getPointsTwoVerts(ReferenceVector references)
{
//    Base::Console().Message("DVD::getPointsTwoVerts() - %s\n", getNameInDocument());
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(1).getSubName());
    if (refObject->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
        !references.at(0).getSubName().empty()) {
        //this is a 2d object (a DVP + subelements)
        TechDraw::VertexPtr v0 = getViewPart()->getProjVertexByIndex(iSubelement0);
        TechDraw::VertexPtr v1 = getViewPart()->getProjVertexByIndex(iSubelement1);
        if (!v0 || !v1) {
            throw Base::RuntimeError("Missing geometry for dimension");
        }
        return { v0->pnt, v1->pnt };
    }

    //this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() ||
        geometry0.ShapeType() != TopAbs_VERTEX || geometry1.ShapeType() != TopAbs_VERTEX) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    const TopoDS_Vertex& vertex0 = TopoDS::Vertex(geometry0);
    const TopoDS_Vertex& vertex1 = TopoDS::Vertex(geometry1);
    gp_Pnt gPoint0 = BRep_Tool::Pnt(vertex0);
    gp_Pnt gPoint1 = BRep_Tool::Pnt(vertex1);
    gp_Pnt gCentroid = DrawUtil::togp_Pnt(getViewPart()->getOriginalCentroid());
    gPoint0 = gp_Pnt(gPoint0.XYZ() - gCentroid.XYZ());
    gPoint1 = gp_Pnt(gPoint1.XYZ() - gCentroid.XYZ());
    //project points onto paperplane, centered, scaled, rotated and inverted
    Base::Vector3d vPoint0 = getViewPart()->projectPoint(DrawUtil::toVector3d(gPoint0)) * getViewPart()->getScale();
    Base::Vector3d vPoint1 = getViewPart()->projectPoint(DrawUtil::toVector3d(gPoint1)) * getViewPart()->getScale();
    return { vPoint0, vPoint1 };
}

pointPair DrawViewDimension::getPointsEdgeVert(ReferenceVector references)
{
//    Base::Console().Message("DVD::getPointsEdgeVert() - %s\n", getNameInDocument());
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    if (refObject->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
        !references.at(0).getSubName().empty()) {
        //this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr edge;
        TechDraw::VertexPtr vertex;
        if (DrawUtil::getGeomTypeFromName(references.at(0).getSubName()) == "Edge") {
            edge = getViewPart()->getGeomByIndex(iSubelement0);
            vertex = getViewPart()->getProjVertexByIndex(iSubelement1);
        } else {
            edge = getViewPart()->getGeomByIndex(iSubelement1);
            vertex = getViewPart()->getProjVertexByIndex(iSubelement0);
        }
        if (!vertex || !edge) {
            throw Base::RuntimeError("Missing geometry for dimension");
        }
        return closestPoints(edge->occEdge, vertex->occVertex);
    }

    //this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() ||
        geometry0.ShapeType() != TopAbs_VERTEX || geometry1.ShapeType() != TopAbs_VERTEX) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }

    pointPair pts = closestPoints(geometry0, geometry1);
    pts.move(getViewPart()->getOriginalCentroid());
    pts.project(getViewPart());
    pts.mapToPage(getViewPart());
    pts.invertY();
    return pts;
}

arcPoints DrawViewDimension::getArcParameters(ReferenceVector references)
{
//    Base::Console().Message("DVD::getArcParameters()\n");
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement = DrawUtil::getIndexFromName(references.front().getSubName());
    if (refObject->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
        !references.at(0).getSubName().empty()) {
        //this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr geom = getViewPart()->getGeomByIndex(iSubelement);
        if (!geom) {
            Base::Console().Error("DVD - %s - 2D references are corrupt (1)\n",getNameInDocument());
            return arcPoints();
        }
        return arcPointsFromBaseGeom(getViewPart()->getGeomByIndex(iSubelement));
    }

    //this is a 3d reference
    TopoDS_Shape geometry = references.front().getGeometry();
    if (geometry.IsNull() ||
        geometry.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    const TopoDS_Edge& edge = TopoDS::Edge(geometry);
    arcPoints pts = arcPointsFromEdge(edge);
    pts.move(getViewPart()->getOriginalCentroid());
    pts.project(getViewPart());
    pts.mapToPage(getViewPart());
    pts.invertY();
    return pts;
}

arcPoints DrawViewDimension::arcPointsFromBaseGeom(TechDraw::BaseGeomPtr base)
{
    TechDraw::CirclePtr circle;
    arcPoints pts;
    pts.center = Base::Vector3d(0.0, 0.0, 0.0);
    pts.radius = 0.0;
    if ((base && base->geomType == TechDraw::GeomType::CIRCLE) ||
        (base && base->geomType == TechDraw::GeomType::ARCOFCIRCLE)) {
        circle = std::static_pointer_cast<TechDraw::Circle> (base);
        pts.center = Base::Vector3d(circle->center.x, circle->center.y, 0.0);
        pts.radius = circle->radius;
        if (base->geomType == TechDraw::GeomType::ARCOFCIRCLE) {
            TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC> (circle);
            pts.isArc = true;
            pts.onCurve.first(Base::Vector3d(aoc->midPnt.x, aoc->midPnt.y, 0.0));
            pts.midArc         = Base::Vector3d(aoc->midPnt.x, aoc->midPnt.y, 0.0);
            pts.arcEnds.first(Base::Vector3d(aoc->startPnt.x, aoc->startPnt.y, 0.0));
            pts.arcEnds.second(Base::Vector3d(aoc->endPnt.x, aoc->endPnt.y, 0.0));
            pts.arcCW          = aoc->cw;
        } else {
            pts.isArc = false;
            pts.onCurve.first(pts.center + Base::Vector3d(1, 0,0) * circle->radius);   //arbitrary point on edge
            pts.onCurve.second(pts.center + Base::Vector3d(-1, 0,0) * circle->radius);  //arbitrary point on edge
        }
    } else if ( (base && base->geomType == TechDraw::GeomType::ELLIPSE) ||
             (base && base->geomType == TechDraw::GeomType::ARCOFELLIPSE) )  {
        TechDraw::EllipsePtr ellipse = std::static_pointer_cast<TechDraw::Ellipse> (base);
        if (ellipse->closed()) {
            double r1 = ellipse->minor;
            double r2 = ellipse->major;
            double rAvg = (r1 + r2) / 2.0;
            pts.center = Base::Vector3d(ellipse->center.x,
                                        ellipse->center.y,
                                        0.0);
            pts.radius = rAvg;
            pts.isArc = false;
            pts.onCurve.first(pts.center + Base::Vector3d(1, 0,0) * rAvg);   //arbitrary point on edge
            pts.onCurve.second(pts.center + Base::Vector3d(-1, 0,0) * rAvg);  //arbitrary point on edge
        } else {
            TechDraw::AOEPtr aoe = std::static_pointer_cast<TechDraw::AOE> (base);
            double r1 = aoe->minor;
            double r2 = aoe->major;
            double rAvg = (r1 + r2) / 2.0;
            pts.isArc = true;
            pts.center = Base::Vector3d(aoe->center.x,
                                        aoe->center.y,
                                        0.0);
            pts.radius = rAvg;
            pts.arcEnds.first(Base::Vector3d(aoe->startPnt.x, aoe->startPnt.y, 0.0));
            pts.arcEnds.second(Base::Vector3d(aoe->endPnt.x, aoe->endPnt.y, 0.0));
            pts.midArc         = Base::Vector3d(aoe->midPnt.x, aoe->midPnt.y, 0.0);
            pts.arcCW          = aoe->cw;
            pts.onCurve.first(Base::Vector3d(aoe->midPnt.x, aoe->midPnt.y, 0.0)); //for radius
//            pts.onCurve.first(pts.center + Base::Vector3d(1, 0,0) * rAvg);   //for diameter
            pts.onCurve.second(pts.center + Base::Vector3d(-1, 0,0) * rAvg);  //arbitrary point on edge
        }
    } else if (base && base->geomType == TechDraw::GeomType::BSPLINE) {
        TechDraw::BSplinePtr spline = std::static_pointer_cast<TechDraw::BSpline> (base);
        if (spline->isCircle()) {
            bool arc;
            double rad;
            Base::Vector3d center;
            //bool circ =
            GeometryUtils::getCircleParms(spline->occEdge, rad, center, arc);
            pts.center = Base::Vector3d(center.x, center.y, 0.0);
            pts.radius = rad;
            pts.arcEnds.first(Base::Vector3d(spline->startPnt.x, spline->startPnt.y, 0.0));
            pts.arcEnds.second(Base::Vector3d(spline->endPnt.x, spline->endPnt.y, 0.0));
            pts.midArc         = Base::Vector3d(spline->midPnt.x, spline->midPnt.y, 0.0);
            pts.isArc = arc;
            pts.arcCW          = spline->cw;
            if (arc) {
                pts.onCurve.first(Base::Vector3d(spline->midPnt.x, spline->midPnt.y, 0.0));
            } else {
                pts.onCurve.first(pts.center + Base::Vector3d(1, 0,0) * rad);   //arbitrary point on edge
                pts.onCurve.second(pts.center + Base::Vector3d(-1, 0,0) * rad);  //arbitrary point on edge
            }
        } else {
            //fubar - can't have non-circular spline as target of Diameter dimension, but this is already
            //checked, so something has gone badly wrong.
            Base::Console().Error("%s: can not make a Circle from this BSpline edge\n", getNameInDocument());
            throw Base::RuntimeError("Bad BSpline geometry for arc dimension");
        }
    } else {
        Base::Console().Log("Error: DVD - %s - 2D references are corrupt\n", getNameInDocument());
        throw Base::RuntimeError("Bad geometry for arc dimension");
    }
    return pts;
}

arcPoints DrawViewDimension::arcPointsFromEdge(TopoDS_Edge occEdge)
{
    arcPoints pts;
    pts.isArc = !BRep_Tool::IsClosed(occEdge);
    pts.arcCW  = false;
    BRepAdaptor_Curve adapt(occEdge);
    double pFirst = adapt.FirstParameter();
    double pLast = adapt.LastParameter();
    double pMid = (pFirst + pLast) / 2.0;
    BRepLProp_CLProps props(adapt, pFirst, 0, Precision::Confusion());
    pts.arcEnds.first(DU::toVector3d(props.Value()));
    props.SetParameter(pLast);
    pts.arcEnds.second(DU::toVector3d(props.Value()));
    props.SetParameter(pMid);
    pts.onCurve.first(DU::toVector3d(props.Value()));
    pts.midArc = DU::toVector3d(props.Value());

    if (adapt.GetType() == GeomAbs_Circle) {
        gp_Circ circle = adapt.Circle();
        pts.center = DU::toVector3d(circle.Location());
        pts.radius = circle.Radius();
        if (pts.isArc) {
            //part of circle
            gp_Ax1 axis = circle.Axis();
            gp_Vec startVec = DU::togp_Vec(pts.arcEnds.first() - pts.center);
            gp_Vec endVec = DU::togp_Vec(pts.arcEnds.second() - pts.center);
            double angle = startVec.AngleWithRef(endVec, axis.Direction().XYZ());
            pts.arcCW = (angle < 0.0);
        } else {
            //full circle
            pts.onCurve.first(pts.center + Base::Vector3d(1, 0,0) * pts.radius);   //arbitrary point on edge
            pts.onCurve.second(pts.center + Base::Vector3d(-1, 0,0) * pts.radius);  //arbitrary point on edge
        }
    } else if (adapt.GetType() == GeomAbs_Ellipse) {
        gp_Elips ellipse = adapt.Ellipse();
        pts.center = DU::toVector3d(ellipse.Location());
        pts.radius = (ellipse.MajorRadius() + ellipse.MinorRadius()) / 2.0;
        if (pts.isArc) {
            //part of ellipse
            gp_Ax1 axis = ellipse.Axis();
            gp_Vec startVec = DU::togp_Vec(pts.arcEnds.first() - pts.center);
            gp_Vec endVec = DU::togp_Vec(pts.arcEnds.second() - pts.center);
            double angle = startVec.AngleWithRef(endVec, axis.Direction().XYZ());
            pts.arcCW = (angle < 0.0);
        } else {
            //full ellipse
            pts.onCurve.first(pts.center + Base::Vector3d(1, 0,0) * pts.radius);   //arbitrary point on edge
            pts.onCurve.second(pts.center + Base::Vector3d(-1, 0,0) * pts.radius);  //arbitrary point on edge
        }
    } else if (adapt.GetType() == GeomAbs_BSplineCurve) {
        if (GeometryUtils::isCircle(occEdge)) {
            bool isArc(false);
            TopoDS_Edge circleEdge = GeometryUtils::asCircle(occEdge, isArc);
            pts.isArc = isArc;
            BRepAdaptor_Curve adaptCircle(circleEdge);
            if (adaptCircle.GetType() != GeomAbs_Circle) {
                throw Base::RuntimeError("failed to get circle from bspline");
            }
            gp_Circ circle = adapt.Circle();
            //TODO: same code as above. reuse opportunity.
            pts.center = DU::toVector3d(circle.Location());
            pts.radius = circle.Radius();
            if (pts.isArc) {
                //part of circle
                gp_Ax1 axis = circle.Axis();
                gp_Vec startVec = DU::togp_Vec(pts.arcEnds.first() - pts.center);
                gp_Vec endVec = DU::togp_Vec(pts.arcEnds.second() - pts.center);
                double angle = startVec.AngleWithRef(endVec, axis.Direction().XYZ());
                pts.arcCW = (angle < 0.0);
            } else {
                //full circle
                pts.onCurve.first(pts.center + Base::Vector3d(1, 0,0) * pts.radius);   //arbitrary point on edge
                pts.onCurve.second(pts.center + Base::Vector3d(-1, 0,0) * pts.radius);  //arbitrary point on edge
            }
        } else {
            throw Base::RuntimeError("failed to make circle from bspline");
        }
    } else {
        throw Base::RuntimeError("can not get arc points from this edge");
    }

    return pts;
}

anglePoints DrawViewDimension::getAnglePointsTwoEdges(ReferenceVector references)
{
//    Base::Console().Message("DVD::getAnglePointsTwoEdges() - %s\n", getNameInDocument());
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(1).getSubName());
    if (refObject->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
        !references.at(0).getSubName().empty()) {
        //this is a 2d object (a DVP + subelements)
        TechDraw::BaseGeomPtr geom0 = getViewPart()->getGeomByIndex(iSubelement0);
        TechDraw::BaseGeomPtr geom1 = getViewPart()->getGeomByIndex(iSubelement1);
        if (!geom0 || !geom1) {
            throw Base::RuntimeError("Missing geometry for dimension");
        }
        if (!geom0 || geom0->geomType != TechDraw::GeomType::GENERIC) {
            throw Base::RuntimeError("Missing geometry for dimension");
        }
        if (!geom1 || geom1->geomType != TechDraw::GeomType::GENERIC) {
            throw Base::RuntimeError("Missing geometry for dimension");
        }
        TechDraw::GenericPtr generic0 = std::static_pointer_cast<TechDraw::Generic>(geom0);
        TechDraw::GenericPtr generic1 = std::static_pointer_cast<TechDraw::Generic>(geom1);
        Base::Vector3d apex = generic0->apparentInter(generic1);
        Base::Vector3d farPoint0, farPoint1;
        //pick the end of generic0 farthest from the apex
        if ((generic0->getStartPoint() - apex).Length() >
            (generic0->getEndPoint() - apex).Length()) {
            farPoint0 = generic0->getStartPoint();
        } else {
            farPoint0 = generic0->getEndPoint();
        }
        //pick the end of generic1 farthest from the apex
        if ((generic1->getStartPoint() - apex).Length() >
            (generic1->getEndPoint() - apex).Length()) {
            farPoint1 = generic1->getStartPoint();
        } else {
            farPoint1 = generic1->getEndPoint();
        }
        Base::Vector3d leg0Dir = (generic0->getStartPoint() - generic0->getEndPoint()).Normalize();
        Base::Vector3d leg1Dir = (generic1->getStartPoint() - generic1->getEndPoint()).Normalize();
        if (DU::fpCompare(fabs(leg0Dir.Dot(leg1Dir)), 1.0)) {
            //legs of the angle are parallel.
            throw Base::RuntimeError("Can not make angle from parallel edges");
        }
        Base::Vector3d extenPoint0 = farPoint0;     //extension line points
        Base::Vector3d extenPoint1 = farPoint1;
        if (!DU::fpCompare(fabs(leg0Dir.Dot(leg1Dir)), 0.0)) {
            //legs of the angle are skew
            //project farthest points onto opposite edge
            Base::Vector3d projFar0OnLeg1 = farPoint0.Perpendicular(apex, leg1Dir);
            Base::Vector3d projFar1OnLeg0 = farPoint1.Perpendicular(apex, leg0Dir);
            if (DrawUtil::isBetween(projFar0OnLeg1,
                                    generic1->getStartPoint(),
                                    generic1->getEndPoint())) {
                extenPoint1 = projFar0OnLeg1;
            } else if (DrawUtil::isBetween(projFar1OnLeg0,
                                         generic0->getStartPoint(),
                                         generic0->getEndPoint())) {
                extenPoint0 = projFar1OnLeg0;
            }

            anglePoints pts;
            pts.first(extenPoint0);
            pts.second(extenPoint1);
            pts.vertex(apex);
            return pts;
        }
    }

    //this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() ||
        geometry0.ShapeType() != TopAbs_EDGE || geometry1.ShapeType() != TopAbs_EDGE) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    TopoDS_Edge edge0 = TopoDS::Edge(geometry0);
    BRepAdaptor_Curve adapt0(edge0);
    auto curve0 = adapt0.Curve().Curve();
    TopoDS_Edge edge1 = TopoDS::Edge(geometry1);
    BRepAdaptor_Curve adapt1(edge1);
    auto curve1 = adapt1.Curve().Curve();
    Base::Vector3d apex;
    if (!DU::apparentIntersection(curve0, curve1, apex)) {
        //no intersection
        throw Base::RuntimeError("Edges for angle dimension can not intersect");
    }
    gp_Pnt gStart0 = BRep_Tool::Pnt(TopExp::FirstVertex(edge0));
    gp_Pnt gEnd0 = BRep_Tool::Pnt(TopExp::LastVertex(edge0));
    gp_Pnt gFar0 = gEnd0;
    if (gStart0.Distance(DU::togp_Pnt(apex)) > gEnd0.Distance(DU::togp_Pnt(apex)) ) {
        gFar0 = gStart0;
    }
    gp_Pnt gStart1 = BRep_Tool::Pnt(TopExp::FirstVertex(edge1));
    gp_Pnt gEnd1 = BRep_Tool::Pnt(TopExp::LastVertex(edge1));
    gp_Pnt gFar1 = gEnd1;
    if (gStart1.Distance(DU::togp_Pnt(apex)) > gEnd1.Distance(DU::togp_Pnt(apex)) ) {
        gFar1 = gStart1;
    }
    Base::Vector3d farPoint0 = DU::toVector3d(gFar0);
    Base::Vector3d farPoint1 = DU::toVector3d(gFar1);
    Base::Vector3d leg0Dir = DU::toVector3d(gFar0) - apex;
    Base::Vector3d leg1Dir = DU::toVector3d(gFar1) - apex;
    if (DU::fpCompare(fabs(leg0Dir.Dot(leg1Dir)), 1.0)) {
        //legs of the angle are parallel.
        throw Base::RuntimeError("Can not make angle from parallel edges");
    }
    Base::Vector3d extenPoint0 = DU::toVector3d(gFar0);     //extension line points
    Base::Vector3d extenPoint1 = DU::toVector3d(gFar1);
    if (!DU::fpCompare(fabs(leg0Dir.Dot(leg1Dir)), 0.0)) {
        //legs of the angle are skew
        //project farthest points onto opposite edge
        Base::Vector3d projFar0OnLeg1 = farPoint0.Perpendicular(apex, leg1Dir);
        Base::Vector3d projFar1OnLeg0 = farPoint1.Perpendicular(apex, leg0Dir);
        if (DrawUtil::isBetween(projFar0OnLeg1,
                                DU::toVector3d(gStart0),
                                DU::toVector3d(gEnd0)) ) {
            extenPoint1 = projFar0OnLeg1;
        } else if (DrawUtil::isBetween(projFar1OnLeg0,
                                       DU::toVector3d(gStart1),
                                       DU::toVector3d(gEnd1)) ) {

            extenPoint0 = projFar1OnLeg0;
        }
    }

    anglePoints pts(apex, extenPoint0, extenPoint1);
    pts.move(getViewPart()->getOriginalCentroid());
    pts.project(getViewPart());
    pts.mapToPage(getViewPart());
    pts.invertY();
    return pts;
}

//TODO: this makes assumptions about the order of references (p - v - p). is this checked somewhere?
anglePoints DrawViewDimension::getAnglePointsThreeVerts(ReferenceVector references)
{
//    Base::Console().Message("DVD::getAnglePointsThreeVerts() - %s\n", getNameInDocument());
    if (references.size() < 3) {
        throw Base::RuntimeError("Not enough references to make angle dimension");
    }
    App::DocumentObject* refObject = references.front().getObject();
    int iSubelement0 = DrawUtil::getIndexFromName(references.at(0).getSubName());
    int iSubelement1 = DrawUtil::getIndexFromName(references.at(1).getSubName());
    int iSubelement2 = DrawUtil::getIndexFromName(references.at(2).getSubName());
    if (refObject->isDerivedFrom(TechDraw::DrawViewPart::getClassTypeId()) &&
        !references.at(0).getSubName().empty()) {
        //this is a 2d object (a DVP + subelements)
        TechDraw::VertexPtr vert0 = getViewPart()->getProjVertexByIndex(iSubelement0);
        TechDraw::VertexPtr vert1 = getViewPart()->getProjVertexByIndex(iSubelement1);
        TechDraw::VertexPtr vert2 = getViewPart()->getProjVertexByIndex(iSubelement2);
        if (!vert0 || !vert1 || !vert2) {
            throw Base::RuntimeError("References for three point angle dimension are not vertices");
        }
        anglePoints pts(vert1->point(), vert0->point(), vert2->point());
        return pts;
    }

    //this is a 3d object
    TopoDS_Shape geometry0 = references.at(0).getGeometry();
    TopoDS_Shape geometry1 = references.at(1).getGeometry();
    TopoDS_Shape geometry2 = references.at(2).getGeometry();
    if (geometry0.IsNull() || geometry1.IsNull() || geometry2.IsNull() ||
        geometry0.ShapeType() != TopAbs_VERTEX || geometry1.ShapeType() != TopAbs_VERTEX || geometry2.ShapeType() != TopAbs_VERTEX) {
        throw Base::RuntimeError("Geometry for dimension reference is null.");
    }
    TopoDS_Vertex vertex0 = TopoDS::Vertex(geometry0);
    gp_Pnt point0 = BRep_Tool::Pnt(vertex0);
    TopoDS_Vertex vertex1 = TopoDS::Vertex(geometry1);
    gp_Pnt point1 = BRep_Tool::Pnt(vertex1);
    TopoDS_Vertex vertex2 = TopoDS::Vertex(geometry2);
    gp_Pnt point2 = BRep_Tool::Pnt(vertex2);
    anglePoints pts(DU::toVector3d(point1), DU::toVector3d(point0), DU::toVector3d(point2));
    pts.move(getViewPart()->getOriginalCentroid());
    pts.project(getViewPart());
    pts.mapToPage(getViewPart());
    pts.invertY();
    return pts;
}

DrawViewPart* DrawViewDimension::getViewPart() const
{
    if (References2D.getValues().empty()) {
        return nullptr;
    }
    return dynamic_cast<TechDraw::DrawViewPart * >(References2D.getValues().at(0));
}


//return the references controlling this dimension. 3d references are used when available
//otherwise 2d references are returned. no checking is performed. Result is pairs of (object, subName)
ReferenceVector DrawViewDimension::getEffectiveReferences() const
{
    const std::vector<App::DocumentObject*>& objects3d = References3D.getValues();
    const std::vector<std::string>& subElements3d = References3D.getSubValues();
    const std::vector<App::DocumentObject*>& objects = References2D.getValues();
    const std::vector<std::string>& subElements = References2D.getSubValues();
    ReferenceVector effectiveRefs;
    if (!objects3d.empty()) {
        //use 3d references by preference
        int refCount = objects3d.size();
        for (int i = 0; i < refCount; i++) {
            ReferenceEntry ref(objects3d.at(i), std::string(subElements3d.at(i)));
            effectiveRefs.push_back(ref);
        }
    } else {
        //use 2d references if necessary
        int refCount = objects.size();
        for (int i = 0; i < refCount; i++) {
            ReferenceEntry ref(objects.at(i), subElements.at(i));
            effectiveRefs.push_back(ref);
        }
    }
    return effectiveRefs;
}

//what configuration of references do we have - Vertex-Vertex, Edge-Vertex, Edge, ...
int DrawViewDimension::getRefType() const
{
    if (isExtentDim()) {
        return RefType::extent;
    }

    ReferenceVector refs = getEffectiveReferences();
    std::vector<std::string> subNames;

    //std::vector<std::string> subNames = getEffectiveSubNames();   //???
    for (auto& ref : refs) {
        if (ref.getSubName().empty()) {
            //skip this one
            continue;
        }
        subNames.push_back(ref.getSubName());
    }

    if (subNames.empty()) {
        //something went wrong, there were no subNames.
        Base::Console().Message("DVD::getRefType - %s - there are no subNames.\n", getNameInDocument());
        return 0;
    }

    return getRefTypeSubElements(subNames);
}

//TODO: Gui/DimensionValidators.cpp has almost the same code
//decide what the reference configuration is by examining the names of the sub elements
int DrawViewDimension::getRefTypeSubElements(const std::vector<std::string> &subElements)
{
    int refType = invalidRef;
    int refEdges = 0, refVertices = 0;

    for (const auto& se: subElements) {
        if (DrawUtil::getGeomTypeFromName(se) == "Vertex") { refVertices++; }
        if (DrawUtil::getGeomTypeFromName(se) == "Edge") { refEdges++; }
    }

    if (refEdges == 0 && refVertices == 2) { refType = twoVertex; }
    if (refEdges == 0 && refVertices == 3) { refType = threeVertex; }
    if (refEdges == 1 && refVertices == 0) { refType = oneEdge; }
    if (refEdges == 1 && refVertices == 1) { refType = vertexEdge; }
    if (refEdges == 2 && refVertices == 0) { refType = twoEdge; }

    return refType;
}

//! validate 2D references - only checks if the target exists
bool DrawViewDimension::checkReferences2D() const
{
//    Base::Console().Message("DVD::checkReferences2d() - %s\n", getNameInDocument());
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    if (objects.empty()) {
        return false;
    }

    const std::vector<std::string> &subElements = References2D.getSubValues();
    if (subElements.empty()) {
        //must have at least 1 null string entry to balance DVP
        return false;
    }

    if (subElements.front().empty() &&
        !References3D.getValues().empty()) {
        //this is (probably) a dim with 3d refs
        return true;
    }

    for (auto& s: subElements) {
        if (s.empty()) {
            return false;
        }

        int idx = DrawUtil::getIndexFromName(s);
        if (DrawUtil::getGeomTypeFromName(s) == "Edge") {
            TechDraw::BaseGeomPtr geom = getViewPart()->getGeomByIndex(idx);
            if (!geom) {
                return false;
            }
        } else if (DrawUtil::getGeomTypeFromName(s) == "Vertex") {
            TechDraw::VertexPtr v = getViewPart()->getProjVertexByIndex(idx);
            if (!v) {
                return false;
            }
        }
    }

    return true;
}

pointPair DrawViewDimension::closestPoints(TopoDS_Shape s1,
                                           TopoDS_Shape s2) const
{
    pointPair result;
    BRepExtrema_DistShapeShape extss(s1, s2);
    if (!extss.IsDone()) {
        throw Base::RuntimeError("DVD::closestPoints - BRepExtrema_DistShapeShape failed");
    }
    int count = extss.NbSolution();
    if (count != 0) {
        gp_Pnt p = extss.PointOnShape1(1);
        result.first(Base::Vector3d(p.X(), p.Y(), p.Z()));
        p = extss.PointOnShape2(1);
        result.second(Base::Vector3d(p.X(), p.Y(), p.Z()));
    } //TODO: else { explode }

    return result;
}

//set the reference property from a reference vector
void DrawViewDimension::setReferences2d(ReferenceVector refs)
{
    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subNames;
    if ( objects.size() != subNames.size() ) {
        throw Base::IndexError("DVD::setReferences2d - objects and subNames do not match.");
    }

    for ( size_t iRef = 0; iRef < refs.size(); iRef++) {
        objects.push_back(refs.at(iRef).getObject());
        subNames.push_back(refs.at(iRef).getSubName());
    }

    References2D.setValues(objects, subNames);
}

//set the reference property from a reference vector
void DrawViewDimension::setReferences3d(ReferenceVector refs)
{
    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subNames;
    if ( objects.size() != subNames.size() ) {
        throw Base::IndexError("DVD::setReferences3d - objects and subNames do not match.");
    }

    for ( size_t iRef = 0; iRef < refs.size(); iRef++) {
        objects.push_back(refs.at(iRef).getObject());
        subNames.push_back(refs.at(iRef).getSubName());
    }

    References3D.setValues(objects, subNames);
}

//!add Dimension 3D references to measurement
void DrawViewDimension::setAll3DMeasurement()
{
//    Base::Console().Message("DVD::setAll3dMeasurement()\n");
    measurement->clear();
    const std::vector<App::DocumentObject*> &Objs = References3D.getValues();
    const std::vector<std::string> &Subs      = References3D.getSubValues();
    int end = Objs.size();
    int i = 0;
    for ( ; i < end; i++) {
        static_cast<void> (measurement->addReference3D(Objs.at(i), Subs.at(i)));
    }
}

//delete all previous measurements
void DrawViewDimension::clear3DMeasurements()
{
    //set sublinklist to empty?
    measurement->clear();
}

void DrawViewDimension::dumpRefs2D(const char* text) const
{
    Base::Console().Message("DUMP - %s\n", text);
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string> &subElements = References2D.getSubValues();
    std::vector<App::DocumentObject*>::const_iterator objIt = objects.begin();
    std::vector<std::string>::const_iterator subIt = subElements.begin();
    int i = 0;
    for( ;objIt != objects.end();objIt++, subIt++, i++) {
        Base::Console().Message("DUMP - ref: %d object: %s subElement: %s\n", i,(*objIt)->getNameInDocument(), (*subIt).c_str());
    }
}

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
    } //TODO: else { explode }

    return minDist;
}

bool DrawViewDimension::leaderIntersectsArc(Base::Vector3d s, Base::Vector3d pointOnCircle) {
    bool result = false;
    const std::vector<std::string> &subElements      = References2D.getSubValues();
    int idx = DrawUtil::getIndexFromName(subElements[0]);
    TechDraw::BaseGeomPtr base = getViewPart()->getGeomByIndex(idx);
    if ( base && base->geomType == TechDraw::GeomType::ARCOFCIRCLE )  {
        TechDraw::AOCPtr aoc = std::static_pointer_cast<TechDraw::AOC> (base);
        if (aoc->intersectsArc(s, pointOnCircle)) {
            result = true;
        }
    } else if ( base && base->geomType == TechDraw::GeomType::BSPLINE )  {
        TechDraw::BSplinePtr spline = std::static_pointer_cast<TechDraw::BSpline> (base);
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
    } else {
        double scale = getViewPart()->getScale();
        m_arrowPositions.first(Base::Vector3d(positions[0].x, positions[0].y, 0.0) / scale);
        m_arrowPositions.second(Base::Vector3d(positions[1].x, positions[1].y, 0.0) / scale);
    }
}

//return position within parent view of dimension arrow heads/dimline endpoints
//note positions are in apparent coord (inverted y).
pointPair DrawViewDimension::getArrowPositions()
{
    return m_arrowPositions;
}

bool DrawViewDimension::has2DReferences() const
{
//    Base::Console().Message("DVD::has2DReferences() - %s\n",getNameInDocument());
    const std::vector<App::DocumentObject*> &objects = References2D.getValues();
    const std::vector<std::string>& subNames         = References2D.getSubValues();
    if (objects.empty()) {
        //we don't even have a DVP
        return false;
    }

    if (subNames.front().empty()) {
        //this is ok, as we must have a null string entry to balance DVP in first object position
        return true;
    }

    //we have a reference to a DVP and at least 1 subName entry, so we have 2d references
    return true;
}

//there is no special structure to 3d references, so anything > 0 is good
bool DrawViewDimension::has3DReferences() const
{
    return (References3D.getSize() > 0);
}

//has arbitrary or nonzero tolerance
bool DrawViewDimension::hasOverUnderTolerance() const
{
    if (ArbitraryTolerances.getValue() ||
            !DrawUtil::fpCompare(OverTolerance.getValue(), 0.0) ||
            !DrawUtil::fpCompare(UnderTolerance.getValue(), 0.0)) {
        return true;
    }
    return false;
}

bool DrawViewDimension::showUnits() const
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    return hGrp->GetBool("ShowUnits", false);
}

bool DrawViewDimension::useDecimals() const
{
    return Preferences::useGlobalDecimals();
}

std::string DrawViewDimension::getPrefix() const
{
    if (Type.isValue("Radius")){
        return "R";
    } else if (Type.isValue("Diameter")){
        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
            .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
        return std::string(hGrp->GetASCII("DiameterSymbol", "\xe2\x8c\x80")); // Diameter symbol
    }

    return "";
}

std::string DrawViewDimension::getDefaultFormatSpec(bool isToleranceFormat) const
{
    std::string prefFormat = Preferences::formatSpec();
    QString formatSpec;
    QString qPrefix;
    if (prefFormat.empty()) {
        QString format1 = Base::Tools::fromStdString("%.");
        QString format2 = Base::Tools::fromStdString("f");
        int precision;
        if (useDecimals()) {
            precision = Base::UnitsApi::getDecimals();
        } else {
            precision = Preferences::altDecimals();
        }
        QString formatPrecision = QString::number(precision);

        std::string prefix = getPrefix();

        if (!prefix.empty()) {
            qPrefix = QString::fromUtf8(prefix.data(), prefix.size());
        }

        formatSpec = qPrefix + format1 + formatPrecision + format2;
    } else {

        std::string prefix = getPrefix();
        qPrefix = QString::fromUtf8(prefix.data(), prefix.size());
        formatSpec = qPrefix + QString::fromStdString(prefFormat);

    }

    if (isToleranceFormat) {
        formatSpec.replace(QString::fromUtf8("%"), QString::fromUtf8("%+"));
    }

    return Base::Tools::toStdString(formatSpec);
}

bool DrawViewDimension::isExtentDim() const
{
    std::string name(getNameInDocument());
    if (name.substr(0, 9) == "DimExtent") {
        return true;
    }
    return false;
}


PyObject *DrawViewDimension::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawViewDimensionPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
