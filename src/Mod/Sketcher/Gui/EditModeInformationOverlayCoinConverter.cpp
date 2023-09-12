/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>
#endif  // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/UnitsApi.h>

#include "EditModeCoinManagerParameters.h"
#include "EditModeInformationOverlayCoinConverter.h"
#include "ViewProviderSketchCoinAttorney.h"


using namespace SketcherGui;

EditModeInformationOverlayCoinConverter::EditModeInformationOverlayCoinConverter(
    ViewProviderSketch& vp,
    SoGroup* infogroup,
    OverlayParameters& overlayparameters,
    DrawingParameters& drawingparameters)
    : viewProvider(vp)
    , infoGroup(infogroup)
    , overlayParameters(overlayparameters)
    , drawingParameters(drawingparameters)
    , nodeId(0) {

      };

void EditModeInformationOverlayCoinConverter::convert(const Part::Geometry* geometry, int geoid)
{

    if (geometry->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
        // at this point all calculations relate to BSplineCurves
        calculate<CalculationType::BSplineDegree>(geometry, geoid);
        calculate<CalculationType::BSplineControlPolygon>(geometry, geoid);
        calculate<CalculationType::BSplineCurvatureComb>(geometry, geoid);
        calculate<CalculationType::BSplineKnotMultiplicity>(geometry, geoid);
        calculate<CalculationType::BSplinePoleWeight>(geometry, geoid);

        addUpdateNode(degree);
        addUpdateNode(controlPolygon);
        addUpdateNode(curvatureComb);
        addUpdateNode(knotMultiplicity);
        addUpdateNode(poleWeights);
    }
    else if (geometry->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
        // at this point all calculations relate to ArcOfCircle
        calculate<CalculationType::ArcCircleHelper>(geometry, geoid);
        addUpdateNode(circleHelper);
    }
}

void EditModeInformationOverlayCoinConverter::addToInfoGroup(SoSwitch* sw)
{
    infoGroup->addChild(sw);
    nodeId++;
}

template<EditModeInformationOverlayCoinConverter::CalculationType calculation>
void EditModeInformationOverlayCoinConverter::calculate(const Part::Geometry* geometry,
                                                        [[maybe_unused]] int geoid)
{

    if constexpr (calculation == CalculationType::ArcCircleHelper) {
        const Part::GeomArcOfCircle* arc = static_cast<const Part::GeomArcOfCircle*>(geometry);
        clearCalculation(circleHelper);

        Base::Vector3d center = arc->getCenter();
        double radius = arc->getRadius();
        Part::GeomCircle circle;
        circle.setRadius(radius);
        circle.setCenter(center);

        const int ndiv = drawingParameters.curvedEdgeCountSegments;

        circleHelper.coordinates.reserve(ndiv);
        for (int i = 0; i < ndiv; i++) {
            double param = i * std::atan(1) * 8 / ndiv;
            circleHelper.coordinates.emplace_back(circle.value(param));
        }
        circleHelper.coordinates.emplace_back(circle.value(0.0));
        circleHelper.indices.push_back(ndiv + 1);
    }
    else {
        const Part::GeomBSplineCurve* spline = static_cast<const Part::GeomBSplineCurve*>(geometry);

        if constexpr (calculation == CalculationType::BSplineDegree) {
            clearCalculation(degree);

            std::vector<Base::Vector3d> poles = spline->getPoles();

            degree.strings.clear();
            degree.positions.clear();

            Base::Vector3d midp = Base::Vector3d(0, 0, 0);

            for (auto val : poles) {
                midp += val;
            }

            midp /= poles.size();

            degree.strings.emplace_back(std::to_string(spline->getDegree()));
            degree.positions.emplace_back(midp);
        }
        else if constexpr (calculation == CalculationType::BSplineControlPolygon) {

            clearCalculation(controlPolygon);

            std::vector<Base::Vector3d> poles = spline->getPoles();

            controlPolygon.coordinates.clear();
            controlPolygon.indices.clear();

            size_t nvertices;

            if (spline->isPeriodic()) {
                nvertices = poles.size() + 1;
            }
            else {
                nvertices = poles.size();
            }

            controlPolygon.coordinates.reserve(nvertices);

            for (auto& v : poles) {
                controlPolygon.coordinates.emplace_back(v);
            }

            if (spline->isPeriodic()) {
                controlPolygon.coordinates.emplace_back(poles[0]);
            }

            controlPolygon.indices.push_back(
                nvertices);  // single continuous polygon starting at index 0
        }
        else if constexpr (calculation == CalculationType::BSplineCurvatureComb) {

            clearCalculation(curvatureComb);
            // curvature graph --------------------------------------------------------

            // based on python source
            // https://github.com/tomate44/CurvesWB/blob/master/freecad/Curves/ParametricComb.py
            // by FreeCAD user Chris_G

            std::vector<Base::Vector3d> poles = spline->getPoles();
            auto knots = spline->getKnots();
            auto mults = spline->getMultiplicities();

            const int ndivPerPiece = 64;  // heuristic of number of division to fill in
            const int ndiv = ndivPerPiece * (knots.size() - 1);

            std::vector<Base::Vector3d> pointatcurvelist;
            std::vector<double> curvaturelist;
            std::vector<Base::Vector3d> normallist;

            pointatcurvelist.reserve(ndiv);
            curvaturelist.reserve(ndiv);
            normallist.reserve(ndiv);

            // go through the polynomial pieces (i.e. from one knot to next)
            for (size_t k = 0; k < knots.size() - 1; ++k) {
                // first and last params are a little off to account for possible discontinuity at
                // knots
                double firstparam =
                    knots[k] + Precision::Approximation() * (knots[k + 1] - knots[k]);
                double lastparam =
                    knots[k + 1] - Precision::Approximation() * (knots[k + 1] - knots[k]);

                // TODO: Maybe this can be improved, specifically adapted for each piece
                double step = (lastparam - firstparam) / (ndivPerPiece - 1);

                for (int i = 0; i < ndivPerPiece; ++i) {
                    double param = firstparam + i * step;
                    pointatcurvelist.emplace_back(spline->value(param));

                    try {
                        curvaturelist.emplace_back(spline->curvatureAt(param));
                    }
                    catch (Base::CADKernelError& e) {
                        // it is "just" a visualisation matter OCC could not calculate the curvature
                        // terminating here would mean that the other shapes would not be drawn.
                        // Solution: Report the issue and set dummy curvature to 0
                        e.ReportException();
                        Base::Console().DeveloperError(
                            "EditModeInformationOverlayCoinConverter",
                            "Curvature graph for B-Spline with GeoId=%d could not be calculated.\n",
                            geoid);
                        curvaturelist.emplace_back(0);
                    }

                    Base::Vector3d normal;
                    try {
                        spline->normalAt(param, normal);
                        normallist.emplace_back(normal);
                    }
                    catch (Base::Exception&) {
                        normallist.emplace_back(0, 0, 0);
                    }
                }
            }

            std::vector<Base::Vector3d> pointatcomblist;
            pointatcomblist.reserve(ndiv);

            for (int i = 0; i < ndiv; i++) {
                pointatcomblist.emplace_back(
                    pointatcurvelist[i]
                    - overlayParameters.currentBSplineCombRepresentationScale * curvaturelist[i]
                        * normallist[i]);
            }

            curvatureComb.coordinates.reserve(3 * ndiv);  // 2*ndiv +1 points of ndiv separate
                                                          // segments + ndiv points for last segment
            curvatureComb.indices.reserve(
                ndiv + 1);  // ndiv separate segments of radials + 1 segment connecting at comb end

            auto zInfoH = ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                * drawingParameters.zInfo;

            for (int i = 0; i < ndiv; i++) {
                // note emplace emplaces on the position BEFORE the iterator given.
                curvatureComb.coordinates.emplace_back(pointatcurvelist[i].x,
                                                       pointatcurvelist[i].y,
                                                       zInfoH);  // radials
                curvatureComb.coordinates.emplace_back(pointatcomblist[i].x,
                                                       pointatcomblist[i].y,
                                                       zInfoH);  // radials

                curvatureComb.indices.emplace_back(2);  // line
            }

            for (int i = 0; i < ndiv; i++) {
                curvatureComb.coordinates.emplace_back(pointatcomblist[i].x,
                                                       pointatcomblist[i].y,
                                                       zInfoH);  // // comb endpoint closing segment
            }

            curvatureComb.indices.emplace_back(ndiv);  // Comb line
        }
        else if constexpr (calculation == CalculationType::BSplineKnotMultiplicity) {

            clearCalculation(knotMultiplicity);
            std::vector<double> knots = spline->getKnots();
            std::vector<int> mult = spline->getMultiplicities();

            for (size_t i = 0; i < knots.size(); i++) {
                knotMultiplicity.positions.emplace_back(spline->pointAtParameter(knots[i]));

                std::ostringstream stringStream;
                stringStream << "(" << mult[i] << ")";

                knotMultiplicity.strings.emplace_back(stringStream.str());
            }
        }
        else if constexpr (calculation == CalculationType::BSplinePoleWeight) {

            clearCalculation(poleWeights);
            std::vector<Base::Vector3d> poles = spline->getPoles();
            auto weights = spline->getWeights();

            for (size_t i = 0; i < poles.size(); i++) {
                poleWeights.positions.emplace_back(poles[i]);

                QString WeightString =
                    QString::fromLatin1("[%1]").arg(weights[i],
                                                    0,
                                                    'f',
                                                    Base::UnitsApi::getDecimals());

                poleWeights.strings.emplace_back(WeightString.toStdString());
            }
        }
    }
}

template<typename Result>
void EditModeInformationOverlayCoinConverter::addUpdateNode(const Result& result)
{

    if (overlayParameters.rebuildInformationLayer) {
        addNode(result);
    }
    else {
        updateNode(result);
    }
}

template<EditModeInformationOverlayCoinConverter::CalculationType calculation>
bool EditModeInformationOverlayCoinConverter::isVisible()
{
    if constexpr (calculation == CalculationType::BSplineDegree) {
        return overlayParameters.bSplineDegreeVisible;
    }
    else if constexpr (calculation == CalculationType::BSplineControlPolygon) {
        return overlayParameters.bSplineControlPolygonVisible;
    }
    else if constexpr (calculation == CalculationType::BSplineCurvatureComb) {
        return overlayParameters.bSplineCombVisible;
    }
    else if constexpr (calculation == CalculationType::BSplineKnotMultiplicity) {
        return overlayParameters.bSplineKnotMultiplicityVisible;
    }
    else if constexpr (calculation == CalculationType::BSplinePoleWeight) {
        return overlayParameters.bSplinePoleWeightVisible;
    }
    else if constexpr (calculation == CalculationType::ArcCircleHelper) {
        return overlayParameters.arcCircleHelperVisible;
    }
}

template<typename Result>
void EditModeInformationOverlayCoinConverter::setPolygon(const Result& result,
                                                         SoLineSet* polygonlineset,
                                                         SoCoordinate3* polygoncoords)
{

    polygoncoords->point.setNum(result.coordinates.size());
    polygonlineset->numVertices.setNum(result.indices.size());

    int32_t* index = polygonlineset->numVertices.startEditing();
    SbVec3f* vts = polygoncoords->point.startEditing();

    for (size_t i = 0; i < result.coordinates.size(); i++) {
        vts[i].setValue(result.coordinates[i].x,
                        result.coordinates[i].y,
                        ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                            * drawingParameters.zInfo);
    }

    for (size_t i = 0; i < result.indices.size(); i++) {
        index[i] = result.indices[i];
    }

    polygoncoords->point.finishEditing();
    polygonlineset->numVertices.finishEditing();
}

template<int line>
void EditModeInformationOverlayCoinConverter::setText(const std::string& string, SoText2* text)
{

    if constexpr (line == 1) {
        text->string = SbString(string.c_str());
    }
    else {
        assert(line > 1);
        SoMFString label;
        for (int l = 0; l < (line - 1); l++) {
            label.set1Value(l, SbString(""));
        }

        label.set1Value(line - 1, SbString(string.c_str()));
        text->string = label;
    }
}


template<typename Result>
void EditModeInformationOverlayCoinConverter::clearCalculation(Result& result)
{
    if constexpr (Result::visualisationType == VisualisationType::Text) {
        result.positions.clear();
        result.strings.clear();
    }
    else if constexpr (Result::visualisationType == VisualisationType::Polygon) {
        result.coordinates.clear();
        result.indices.clear();
    }
}

template<typename Result>
void EditModeInformationOverlayCoinConverter::addNode(const Result& result)
{

    if constexpr (Result::visualisationType == VisualisationType::Text) {

        for (size_t i = 0; i < result.strings.size(); i++) {

            SoSwitch* sw = new SoSwitch();

            sw->whichChild = isVisible<Result::calculationType>() ? SO_SWITCH_ALL : SO_SWITCH_NONE;

            SoSeparator* sep = new SoSeparator();
            sep->ref();
            // no caching for frequently-changing data structures
            sep->renderCaching = SoSeparator::OFF;

            // every information visual node gets its own material for to-be-implemented
            // preselection and selection
            SoMaterial* mat = new SoMaterial;
            mat->ref();
            mat->diffuseColor = drawingParameters.InformationColor;

            SoTranslation* translate = new SoTranslation;

            translate->translation.setValue(
                result.positions[i].x,
                result.positions[i].y,
                ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                    * drawingParameters.zInfo);

            SoFont* font = new SoFont;
            font->name.setValue("Helvetica");
            font->size.setValue(drawingParameters.coinFontSize);

            SoText2* text = new SoText2;

            // since the first and last control point of a spline is also treated as knot and thus
            // can also have a displayed multiplicity, we must assure the multiplicity is not
            // visibly overwritten therefore be output the weight in a second line
            //
            // This could be made into a more generic form, but it is probably not worth the effort
            // at this time.
            if constexpr (Result::calculationType == CalculationType::BSplinePoleWeight) {
                setText<2>(result.strings[i], text);
            }
            else {
                setText(result.strings[i], text);
            }

            sep->addChild(mat);
            sep->addChild(font);
            sep->addChild(translate);
            sep->addChild(text);

            sw->addChild(sep);

            addToInfoGroup(sw);
            sep->unref();
            mat->unref();
        }
    }
    else if constexpr (Result::visualisationType == VisualisationType::Polygon) {

        SoSwitch* sw = new SoSwitch();

        // hGrpsk->GetBool("BSplineControlPolygonVisible", true)
        sw->whichChild = isVisible<Result::calculationType>() ? SO_SWITCH_ALL : SO_SWITCH_NONE;

        SoSeparator* sep = new SoSeparator();
        sep->ref();
        // no caching for frequently-changing data structures
        sep->renderCaching = SoSeparator::OFF;

        // every information visual node gets its own material for to-be-implemented preselection
        // and selection
        SoMaterial* mat = new SoMaterial;
        mat->ref();
        mat->diffuseColor = drawingParameters.InformationColor;

        SoLineSet* polygonlineset = new SoLineSet;
        SoCoordinate3* polygoncoords = new SoCoordinate3;

        setPolygon<Result>(result, polygonlineset, polygoncoords);

        sep->addChild(mat);
        sep->addChild(polygoncoords);
        sep->addChild(polygonlineset);

        sw->addChild(sep);

        addToInfoGroup(sw);
        sep->unref();
        mat->unref();
    }
}

template<typename Result>
void EditModeInformationOverlayCoinConverter::updateNode(const Result& result)
{

    if constexpr (Result::visualisationType == VisualisationType::Text) {

        for (size_t i = 0; i < result.strings.size(); i++) {
            SoSwitch* sw = static_cast<SoSwitch*>(infoGroup->getChild(nodeId));

            if (overlayParameters.visibleInformationChanged) {
                sw->whichChild =
                    isVisible<Result::calculationType>() ? SO_SWITCH_ALL : SO_SWITCH_NONE;
            }

            SoSeparator* sep = static_cast<SoSeparator*>(sw->getChild(0));

            static_cast<SoTranslation*>(
                sep->getChild(static_cast<int>(TextNodePosition::TextCoordinates)))
                ->translation.setValue(
                    result.positions[i].x,
                    result.positions[i].y,
                    ViewProviderSketchCoinAttorney::getViewOrientationFactor(viewProvider)
                        * drawingParameters.zInfo);

            // since the first and last control point of a spline is also treated as knot and thus
            // can also have a displayed multiplicity, we must assure the multiplicity is not
            // visibly overwritten therefore be output the weight in a second line
            //
            // This could be made into a more generic form, but it is probably not worth the effort
            // at this time.
            if constexpr (Result::calculationType == CalculationType::BSplinePoleWeight) {
                setText<2>(result.strings[i],
                           static_cast<SoText2*>(
                               sep->getChild(static_cast<int>(TextNodePosition::TextInformation))));
            }
            else {
                setText(result.strings[i],
                        static_cast<SoText2*>(
                            sep->getChild(static_cast<int>(TextNodePosition::TextInformation))));
            }

            nodeId++;
        }
    }
    else if constexpr (Result::visualisationType == VisualisationType::Polygon) {

        SoSwitch* sw = static_cast<SoSwitch*>(infoGroup->getChild(nodeId));

        if (overlayParameters.visibleInformationChanged) {
            sw->whichChild = isVisible<Result::calculationType>() ? SO_SWITCH_ALL : SO_SWITCH_NONE;
        }

        SoSeparator* sep = static_cast<SoSeparator*>(sw->getChild(0));

        SoCoordinate3* polygoncoords = static_cast<SoCoordinate3*>(
            sep->getChild(static_cast<int>(PolygonNodePosition::PolygonCoordinates)));

        SoLineSet* polygonlineset = static_cast<SoLineSet*>(
            sep->getChild(static_cast<int>(PolygonNodePosition::PolygonLineSet)));

        setPolygon(result, polygonlineset, polygoncoords);

        nodeId++;
    }
}
