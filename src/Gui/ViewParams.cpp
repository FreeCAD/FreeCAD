/****************************************************************************
 *   Copyright (c) 2018 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
#include <App/Application.h>
#include "RenderPipeline.h"
#include "ViewParams.h"

using namespace Gui;

void ViewParams::setup()
{
    static_assert(
        Base::is_getter<decltype(&ViewParams::getUseNewSelection), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setUseNewSelection), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getUseSelectionRoot), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setUseSelectionRoot), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getEnableSelection), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setEnableSelection), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getRenderCache), Int::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setRenderCache), Int::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getRenderPipeline), String::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setRenderPipeline), String::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getRandomColor), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setRandomColor), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getBoundingBoxColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setBoundingBoxColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getAnnotationTextColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setAnnotationTextColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getMarkerSize), Int::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setMarkerSize), Int::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDefaultLinkColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDefaultLinkColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDefaultShapeLineColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDefaultShapeLineColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDefaultShapeVertexColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDefaultShapeVertexColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDefaultShapeColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDefaultShapeColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDefaultShapeTransparency), Int::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDefaultShapeTransparency), Int::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDefaultShapeLineWidth), Int::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDefaultShapeLineWidth), Int::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDefaultShapePointSize), Int::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDefaultShapePointSize), Int::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getCoinCycleCheck), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setCoinCycleCheck), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getEnablePropertyViewForInactiveDocument), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setEnablePropertyViewForInactiveDocument), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getShowSelectionBoundingBox), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setShowSelectionBoundingBox), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getPropertyViewTimer), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setPropertyViewTimer), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getAxisXColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setAxisXColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getAxisYColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setAxisYColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getAxisZColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setAxisZColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getOriginColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setOriginColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getNeutralColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setNeutralColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getPlacementIndicatorScale), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setPlacementIndicatorScale), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDraggerScale), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDraggerScale), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDatumScale), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDatumScale), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDatumPlaneSize), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDatumPlaneSize), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDatumLineSize), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDatumLineSize), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getDatumTemporaryScaleFactor), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setDatumTemporaryScaleFactor), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getViewSelectionExtend), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setViewSelectionExtend), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getViewSelectionExtendFactor), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setViewSelectionExtendFactor), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getSelectionLineThicken), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setSelectionLineThicken), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getSelectionLineMaxWidth), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setSelectionLineMaxWidth), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getSelectionBBoxLineWidth), Double::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setSelectionBBoxLineWidth), Double::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getMaxViewSelections), Int::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setMaxViewSelections), Int::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getSelectionColor), Unsigned::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setSelectionColor), Unsigned::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getUseTightBoundingBox), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setUseTightBoundingBox), Bool::value_type>,
        "Mismatching signature"
    );

    static_assert(
        Base::is_getter<decltype(&ViewParams::getRenderProjectedBBox), Bool::value_type>,
        "Mismatching signature"
    );
    static_assert(
        Base::is_setter<decltype(&ViewParams::setRenderProjectedBBox), Bool::value_type>,
        "Mismatching signature"
    );

    addParameter("UseNewSelection", Bool {true});
    addParameter("UseSelectionRoot", Bool {true});
    addParameter("EnableSelection", Bool {true});
    addParameter("RenderCache", Int {0});
    addParameter("CoinRenderPipeline", String {"LegacyGL"});
    addParameter("RandomColor", Bool {false});
    addParameter("BoundingBoxColor", Unsigned {4294967295UL});
    addParameter("AnnotationTextColor", Unsigned {4294967295UL});
    addParameter("MarkerSize", Int {9});
    addParameter("DefaultLinkColor", Unsigned {0x66FFFF00});
    addParameter("DefaultShapeLineColor", Unsigned {255UL});
    addParameter("DefaultShapeVertexColor", Unsigned {421075455UL});
    addParameter("DefaultShapeColor", Unsigned {0xCCCCCC00});
    addParameter("DefaultShapeTransparency", Int {0});
    addParameter("DefaultShapeLineWidth", Int {2});
    addParameter("DefaultShapePointSize", Int {2});
    addParameter("CoinCycleCheck", Bool {true});
    addParameter("EnablePropertyViewForInactiveDocument", Bool {true});
    addParameter("ShowSelectionBoundingBox", Bool {false});
    addParameter("PropertyViewTimer", Unsigned {100});
    addParameter("AxisXColor", Unsigned {0xCC333300});
    addParameter("AxisYColor", Unsigned {0x33CC3300});
    addParameter("AxisZColor", Unsigned {0x3333CC00});
    addParameter("OriginColor", Unsigned {0xFBD62900});
    addParameter("NeutralColor", Unsigned {0xB3B38000});
    addParameter("PlacementIndicatorScale", Double {40.0});
    addParameter("DraggerScale", Double {0.03});
    addParameter("DatumScale", Double {100.0});
    addParameter("DatumPlaneSize", Double {62.0});
    addParameter("DatumLineSize", Double {70.0});
    addParameter("DatumTemporaryScaleFactor", Double {2.0});
    addParameter("ViewSelectionExtend", Bool {false});
    addParameter("ViewSelectionExtendFactor", Double {0.5});
    addParameter("SelectionLineThicken", Double {1.0});
    addParameter("SelectionLineMaxWidth", Double {4.0});
    addParameter("SelectionBBoxLineWidth", Double {3.0});
    addParameter("MaxViewSelections", Int {100});
    addParameter("SelectionColor", Unsigned {0x1cad1cff});
    addParameter("UseTightBoundingBox", Bool {true});
    addParameter("RenderProjectedBBox", Bool {true});
}

ViewParams::ViewParams()
{
    attachToParameter(
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
    );
    setup();
    initParameters();
}

ViewParams* ViewParams::instance()
{
    static ViewParams view;
    return &view;
}

bool ViewParams::getUseNewSelection() const
{
    return getValue<bool>("UseNewSelection");
}

void ViewParams::setUseNewSelection(bool v)
{
    setValue("UseNewSelection", v);
}

bool ViewParams::getUseSelectionRoot() const
{
    return getValue<bool>("UseSelectionRoot");
}

void ViewParams::setUseSelectionRoot(bool v)
{
    setValue("UseSelectionRoot", v);
}

bool ViewParams::getEnableSelection() const
{
    return getValue<bool>("EnableSelection");
}

void ViewParams::setEnableSelection(bool v)
{
    setValue("EnableSelection", v);
}

long ViewParams::getRenderCache() const
{
    return getValue<long>("RenderCache");
}

void ViewParams::setRenderCache(long v)
{
    setValue("RenderCache", v);
}

std::string ViewParams::getRenderPipeline() const
{
    const auto value = getValue<std::string>("CoinRenderPipeline");
    const auto pipeline = parseRenderPipelineOrLegacy(value);
    if (value != renderPipelineName(pipeline)) {
        const_cast<ViewParams*>(this)->setRenderPipeline(
            std::string(renderPipelineName(pipeline))
        );  // NOLINT
    }
    return std::string(renderPipelineName(pipeline));
}

void ViewParams::setRenderPipeline(std::string v)
{
    const auto pipeline = parseRenderPipelineOrLegacy(v);
    setValue("CoinRenderPipeline", std::string(renderPipelineName(pipeline)));
}

bool ViewParams::getRandomColor() const
{
    return getValue<bool>("RandomColor");
}

void ViewParams::setRandomColor(bool v)
{
    setValue("RandomColor", v);
}

unsigned long ViewParams::getBoundingBoxColor() const
{
    return getValue<unsigned long>("BoundingBoxColor");
}

void ViewParams::setBoundingBoxColor(unsigned long v)
{
    setValue("BoundingBoxColor", v);
}

unsigned long ViewParams::getAnnotationTextColor() const
{
    return getValue<unsigned long>("AnnotationTextColor");
}

void ViewParams::setAnnotationTextColor(unsigned long v)
{
    setValue("AnnotationTextColor", v);
}

long ViewParams::getMarkerSize() const
{
    return getValue<long>("MarkerSize");
}

void ViewParams::setMarkerSize(long v)
{
    setValue("MarkerSize", v);
}

unsigned long ViewParams::getDefaultLinkColor() const
{
    return getValue<unsigned long>("DefaultLinkColor");
}

void ViewParams::setDefaultLinkColor(unsigned long v)
{
    setValue("DefaultLinkColor", v);
}

unsigned long ViewParams::getDefaultShapeLineColor() const
{
    return getValue<unsigned long>("DefaultShapeLineColor");
}

void ViewParams::setDefaultShapeLineColor(unsigned long v)
{
    setValue("DefaultShapeLineColor", v);
}

unsigned long ViewParams::getDefaultShapeVertexColor() const
{
    return getValue<unsigned long>("DefaultShapeVertexColor");
}

void ViewParams::setDefaultShapeVertexColor(unsigned long v)
{
    setValue("DefaultShapeVertexColor", v);
}

unsigned long ViewParams::getDefaultShapeColor() const
{
    return getValue<unsigned long>("DefaultShapeColor");
}

void ViewParams::setDefaultShapeColor(unsigned long v)
{
    setValue("DefaultShapeColor", v);
}

long ViewParams::getDefaultShapeTransparency() const
{
    return getValue<long>("DefaultShapeTransparency");
}

void ViewParams::setDefaultShapeTransparency(long v)
{
    setValue("DefaultShapeTransparency", v);
}

long ViewParams::getDefaultShapeLineWidth() const
{
    return getValue<long>("DefaultShapeLineWidth");
}

void ViewParams::setDefaultShapeLineWidth(long v)
{
    setValue("DefaultShapeLineWidth", v);
}

long ViewParams::getDefaultShapePointSize() const
{
    return getValue<long>("DefaultShapePointSize");
}

void ViewParams::setDefaultShapePointSize(long v)
{
    setValue("DefaultShapePointSize", v);
}

bool ViewParams::getCoinCycleCheck() const
{
    return getValue<bool>("CoinCycleCheck");
}

void ViewParams::setCoinCycleCheck(bool v)
{
    setValue("CoinCycleCheck", v);
}

bool ViewParams::getEnablePropertyViewForInactiveDocument() const
{
    return getValue<bool>("EnablePropertyViewForInactiveDocument");
}

void ViewParams::setEnablePropertyViewForInactiveDocument(bool v)
{
    setValue("EnablePropertyViewForInactiveDocument", v);
}

bool ViewParams::getShowSelectionBoundingBox() const
{
    return getValue<bool>("ShowSelectionBoundingBox");
}

void ViewParams::setShowSelectionBoundingBox(bool v)
{
    setValue("ShowSelectionBoundingBox", v);
}

unsigned long ViewParams::getPropertyViewTimer() const
{
    return getValue<unsigned long>("PropertyViewTimer");
}

void ViewParams::setPropertyViewTimer(unsigned long v)
{
    setValue("PropertyViewTimer", v);
}

unsigned long ViewParams::getAxisXColor() const
{
    return getValue<unsigned long>("AxisXColor");
}

void ViewParams::setAxisXColor(unsigned long v)
{
    setValue("AxisXColor", v);
}

unsigned long ViewParams::getAxisYColor() const
{
    return getValue<unsigned long>("AxisYColor");
}

void ViewParams::setAxisYColor(unsigned long v)
{
    setValue("AxisYColor", v);
}

unsigned long ViewParams::getAxisZColor() const
{
    return getValue<unsigned long>("AxisZColor");
}

void ViewParams::setAxisZColor(unsigned long v)
{
    setValue("AxisZColor", v);
}

unsigned long ViewParams::getOriginColor() const
{
    return getValue<unsigned long>("OriginColor");
}

void ViewParams::setOriginColor(unsigned long v)
{
    setValue("OriginColor", v);
}

unsigned long ViewParams::getNeutralColor() const
{
    return getValue<unsigned long>("NeutralColor");
}

void ViewParams::setNeutralColor(unsigned long v)
{
    setValue("NeutralColor", v);
}

double ViewParams::getPlacementIndicatorScale() const
{
    return getValue<double>("PlacementIndicatorScale");
}

void ViewParams::setPlacementIndicatorScale(double v)
{
    setValue("PlacementIndicatorScale", v);
}

double ViewParams::getDraggerScale() const
{
    return getValue<double>("DraggerScale");
}

void ViewParams::setDraggerScale(double v)
{
    setValue("DraggerScale", v);
}

double ViewParams::getDatumScale() const
{
    return getValue<double>("DatumScale");
}

void ViewParams::setDatumScale(double v)
{
    setValue("DatumScale", v);
}

double ViewParams::getDatumPlaneSize() const
{
    return getValue<double>("DatumPlaneSize");
}

void ViewParams::setDatumPlaneSize(double v)
{
    setValue("DatumPlaneSize", v);
}

double ViewParams::getDatumLineSize() const
{
    return getValue<double>("DatumLineSize");
}

void ViewParams::setDatumLineSize(double v)
{
    setValue("DatumLineSize", v);
}

double ViewParams::getDatumTemporaryScaleFactor() const
{
    return getValue<double>("DatumTemporaryScaleFactor");
}

void ViewParams::setDatumTemporaryScaleFactor(double v)
{
    setValue("DatumTemporaryScaleFactor", v);
}

bool ViewParams::getViewSelectionExtend() const
{
    return getValue<bool>("ViewSelectionExtend");
}

void ViewParams::setViewSelectionExtend(bool v)
{
    setValue("ViewSelectionExtend", v);
}

double ViewParams::getViewSelectionExtendFactor() const
{
    return getValue<double>("ViewSelectionExtendFactor");
}

void ViewParams::setViewSelectionExtendFactor(double v)
{
    setValue("ViewSelectionExtendFactor", v);
}

double ViewParams::getSelectionLineThicken() const
{
    return getValue<double>("SelectionLineThicken");
}

void ViewParams::setSelectionLineThicken(double v)
{
    setValue("SelectionLineThicken", v);
}

double ViewParams::getSelectionLineMaxWidth() const
{
    return getValue<double>("SelectionLineMaxWidth");
}

void ViewParams::setSelectionLineMaxWidth(double v)
{
    setValue("SelectionLineMaxWidth", v);
}

double ViewParams::getSelectionBBoxLineWidth() const
{
    return getValue<double>("SelectionBBoxLineWidth");
}

void ViewParams::setSelectionBBoxLineWidth(double v)
{
    setValue("SelectionBBoxLineWidth", v);
}

long ViewParams::getMaxViewSelections() const
{
    return getValue<long>("MaxViewSelections");
}

void ViewParams::setMaxViewSelections(long v)
{
    setValue("MaxViewSelections", v);
}

unsigned long ViewParams::getSelectionColor() const
{
    return getValue<unsigned long>("SelectionColor");
}

void ViewParams::setSelectionColor(unsigned long v)
{
    setValue("SelectionColor", v);
}

bool ViewParams::getUseTightBoundingBox() const
{
    return getValue<bool>("UseTightBoundingBox");
}

void ViewParams::setUseTightBoundingBox(bool v)
{
    setValue("UseTightBoundingBox", v);
}

bool ViewParams::getRenderProjectedBBox() const
{
    return getValue<bool>("RenderProjectedBBox");
}

void ViewParams::setRenderProjectedBBox(bool v)
{
    setValue("RenderProjectedBBox", v);
}
