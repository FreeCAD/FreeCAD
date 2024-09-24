/***************************************************************************
 *   Copyright (c) 2015 Yorik van Havre (yorik@uncreated.net)              *
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
#include <Standard_Version.hxx>
#if OCC_VERSION_HEX < 0x070600
#include <BRepAdaptor_HCurve.hxx>
#endif
#include <Approx_Curve3d.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Builder.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#endif

#include <App/Annotation.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/FeaturePythonPyImp.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Matrix.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>
#include <Base/PlacementPy.h>
#include <Base/VectorPy.h>
#include <Mod/Part/App/PartFeature.h>

#include "ImpExpDxf.h"


using namespace Import;

#if OCC_VERSION_HEX >= 0x070600
using BRepAdaptor_HCurve = BRepAdaptor_Curve;
#endif


//******************************************************************************
// reading
ImpExpDxfRead::ImpExpDxfRead(const std::string& filepath, App::Document* pcDoc)
    : CDxfRead(filepath)
    , document(pcDoc)
{
    setOptionSource("User parameter:BaseApp/Preferences/Mod/Draft");
    setOptions();
}

bool ImpExpDxfRead::ReadEntitiesSection()
{
    DrawingEntityCollector collector(*this);
    if (m_mergeOption < SingleShapes) {
        std::map<CDxfRead::CommonEntityAttributes, std::list<TopoDS_Shape>> ShapesToCombine;
        {
            ShapeSavingEntityCollector savingCollector(*this, ShapesToCombine);
            if (!CDxfRead::ReadEntitiesSection()) {
                return false;
            }
        }

        // Merge the contents of ShapesToCombine and AddObject the result(s)
        // TODO: We do end-to-end joining or complete merging as selected by the options.
        for (auto& shapeSet : ShapesToCombine) {
            m_entityAttributes = shapeSet.first;
            CombineShapes(shapeSet.second,
                          m_entityAttributes.m_Layer == nullptr
                              ? "Compound"
                              : m_entityAttributes.m_Layer->Name.c_str());
        }
    }
    else {
        if (!CDxfRead::ReadEntitiesSection()) {
            return false;
        }
    }
    if (m_preserveLayers) {
        for (auto& layerEntry : Layers) {
            ((Layer*)layerEntry.second)->FinishLayer();
        }
    }
    return true;
}

void ImpExpDxfRead::CombineShapes(std::list<TopoDS_Shape>& shapes, const char* nameBase) const
{
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    for (const auto& sh : shapes) {
        if (!sh.IsNull()) {
            builder.Add(comp, sh);
        }
    }
    if (!comp.IsNull()) {
        Collector->AddObject(comp, nameBase);
    }
}

void ImpExpDxfRead::setOptions()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath(getOptionSource().c_str());
    m_preserveLayers = hGrp->GetBool("dxfUseDraftVisGroups", true);
    m_preserveColors = hGrp->GetBool("dxfGetOriginalColors", true);
    // Default for creation type is to create draft objects.
    // The radio-button structure of the options dialog should generally prevent this condition.
    m_mergeOption = DraftObjects;
    if (hGrp->GetBool("groupLayers", true)) {
        // Group all compatible objects together
        m_mergeOption = MergeShapes;
    }
    else if (hGrp->GetBool("dxfCreatePart", true)) {
        // Create (non-draft) Shape objects when possible
        m_mergeOption = SingleShapes;
    }
    else if (hGrp->GetBool("dxfCreateDraft", true)) {
        // Create only Draft objects, making the result closest to drawn-from-scratch
        m_mergeOption = DraftObjects;
    }
    // TODO: joingeometry should give an intermediate between MergeShapes and SingleShapes which
    // will merge shapes that happen to join end-to-end. As such it should be in the radio button
    // set, except that the legacy importer can do joining either for sketches or for shapes. What
    // this really means is there should be an "Import as sketch" checkbox, and only the
    // MergeShapes, JoinShapes, and SingleShapes radio buttons should be allowed, i.e. Draft Objects
    // would be ignored.
    SetAdditionalScaling(hGrp->GetFloat("dxfScaling", 1.0));

    m_importAnnotations = hGrp->GetBool("dxftext", false);
    m_importPoints = hGrp->GetBool("dxfImportPoints", true);
    m_importPaperSpaceEntities = hGrp->GetBool("dxflayout", false);
    m_importHiddenBlocks = hGrp->GetBool("dxfstarblocks", false);
    // TODO: There is currently no option for this: m_importFrozenLayers =
    // hGrp->GetBool("dxffrozenLayers", false);
    // TODO: There is currently no option for this: m_importHiddenLayers =
    // hGrp->GetBool("dxfhiddenLayers", true);
}

bool ImpExpDxfRead::OnReadBlock(const std::string& name, int flags)
{
    if ((flags & 0x04) != 0) {
        // Note that this doesn't mean there are not entities in the block. I don't
        // know if the external reference can be cached because there are two other bits
        // here, 0x10 and 0x20, that seem to handle "resolved" external references.
        UnsupportedFeature("External (xref) BLOCK");
    }
    else if (!m_importHiddenBlocks && (flags & 0x01) != 0) {
        // It is an anonymous block used to build dimensions, hatches, etc so we don't need it
        // and don't want to be complaining about unhandled entity types.
        // Note that if it *is* for a hatch we could actually import it and use it to draw a hatch.
    }
    else if (Blocks.count(name) > 0) {
        ImportError("Duplicate block name '%s'\n", name);
    }
    else {
        Block& block = Blocks.insert(std::make_pair(name, Block(name, flags))).first->second;
        BlockDefinitionCollector blockCollector(*this,
                                                block.Shapes,
                                                block.FeatureBuildersList,
                                                block.Inserts);
        return ReadBlockContents();
    }
    return SkipBlockContents();
}

void ImpExpDxfRead::OnReadLine(const Base::Vector3d& start,
                               const Base::Vector3d& end,
                               bool /*hidden*/)
{
    gp_Pnt p0 = makePoint(start);
    gp_Pnt p1 = makePoint(end);
    if (p0.IsEqual(p1, 0.00000001)) {
        // TODO: Really?? What about the people designing integrated circuits?
        return;
    }
    Collector->AddObject(BRepBuilderAPI_MakeEdge(p0, p1).Edge(), "Line");
}


void ImpExpDxfRead::OnReadPoint(const Base::Vector3d& start)
{
    Collector->AddObject(BRepBuilderAPI_MakeVertex(makePoint(start)).Vertex(), "Point");
}


void ImpExpDxfRead::OnReadArc(const Base::Vector3d& start,
                              const Base::Vector3d& end,
                              const Base::Vector3d& center,
                              bool dir,
                              bool /*hidden*/)
{
    gp_Pnt p0 = makePoint(start);
    gp_Pnt p1 = makePoint(end);
    gp_Dir up(0, 0, 1);
    if (!dir) {
        up = -up;
    }
    gp_Pnt pc = makePoint(center);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    if (circle.Radius() > 0) {
        Collector->AddObject(BRepBuilderAPI_MakeEdge(circle, p0, p1).Edge(), "Arc");
    }
    else {
        Base::Console().Warning("ImpExpDxf - ignore degenerate arc of circle\n");
    }
}


void ImpExpDxfRead::OnReadCircle(const Base::Vector3d& start,
                                 const Base::Vector3d& center,
                                 bool dir,
                                 bool /*hidden*/)
{
    gp_Pnt p0 = makePoint(start);
    gp_Dir up(0, 0, 1);
    if (!dir) {
        up = -up;
    }
    gp_Pnt pc = makePoint(center);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    if (circle.Radius() > 0) {
        Collector->AddObject(BRepBuilderAPI_MakeEdge(circle).Edge(), "Circle");
    }
    else {
        Base::Console().Warning("ImpExpDxf - ignore degenerate circle\n");
    }
}


Handle(Geom_BSplineCurve) getSplineFromPolesAndKnots(struct SplineData& sd)
{
    std::size_t numPoles = sd.control_points;
    if (sd.controlx.size() > numPoles || sd.controly.size() > numPoles
        || sd.controlz.size() > numPoles || sd.weight.size() > numPoles) {
        return nullptr;
    }

    // handle the poles
    TColgp_Array1OfPnt occpoles(1, sd.control_points);
    int index = 1;
    for (auto coordinate : sd.controlx) {
        occpoles(index++).SetX(coordinate);
    }

    index = 1;
    for (auto coordinate : sd.controly) {
        occpoles(index++).SetY(coordinate);
    }

    index = 1;
    for (auto coordinate : sd.controlz) {
        occpoles(index++).SetZ(coordinate);
    }

    // handle knots and mults
    std::set<double> unique;
    unique.insert(sd.knot.begin(), sd.knot.end());

    int numKnots = int(unique.size());
    TColStd_Array1OfInteger occmults(1, numKnots);
    TColStd_Array1OfReal occknots(1, numKnots);
    index = 1;
    for (auto knot : unique) {
        occknots(index) = knot;
        occmults(index) = (int)std::count(sd.knot.begin(), sd.knot.end(), knot);
        index++;
    }

    // handle weights
    TColStd_Array1OfReal occweights(1, sd.control_points);
    if (sd.weight.size() == std::size_t(sd.control_points)) {
        index = 1;
        for (auto weight : sd.weight) {
            occweights(index++) = weight;
        }
    }
    else {
        // non-rational
        for (int i = occweights.Lower(); i <= occweights.Upper(); i++) {
            occweights(i) = 1.0;
        }
    }

    Standard_Boolean periodic = sd.flag == 2;
    Handle(Geom_BSplineCurve) geom =
        new Geom_BSplineCurve(occpoles, occweights, occknots, occmults, sd.degree, periodic);
    return geom;
}

Handle(Geom_BSplineCurve) getInterpolationSpline(struct SplineData& sd)
{
    std::size_t numPoints = sd.fit_points;
    if (sd.fitx.size() > numPoints || sd.fity.size() > numPoints || sd.fitz.size() > numPoints) {
        return nullptr;
    }

    // handle the poles
    Handle(TColgp_HArray1OfPnt) fitpoints = new TColgp_HArray1OfPnt(1, sd.fit_points);
    int index = 1;
    for (auto coordinate : sd.fitx) {
        fitpoints->ChangeValue(index++).SetX(coordinate);
    }

    index = 1;
    for (auto coordinate : sd.fity) {
        fitpoints->ChangeValue(index++).SetY(coordinate);
    }

    index = 1;
    for (auto coordinate : sd.fitz) {
        fitpoints->ChangeValue(index++).SetZ(coordinate);
    }

    Standard_Boolean periodic = sd.flag == 2;
    GeomAPI_Interpolate interp(fitpoints, periodic, Precision::Confusion());
    interp.Perform();
    return interp.Curve();
}

void ImpExpDxfRead::OnReadSpline(struct SplineData& sd)
{
    // https://documentation.help/AutoCAD-DXF/WS1a9193826455f5ff18cb41610ec0a2e719-79e1.htm
    // Flags:
    // 1: Closed, 2: Periodic, 4: Rational, 8: Planar, 16: Linear

    try {
        Handle(Geom_BSplineCurve) geom;
        if (sd.control_points > 0) {
            geom = getSplineFromPolesAndKnots(sd);
        }
        else if (sd.fit_points > 0) {
            geom = getInterpolationSpline(sd);
        }

        if (geom.IsNull()) {
            throw Standard_Failure();
        }

        Collector->AddObject(BRepBuilderAPI_MakeEdge(geom).Edge(), "Spline");
    }
    catch (const Standard_Failure&) {
        Base::Console().Warning("ImpExpDxf - failed to create bspline\n");
    }
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters)
void ImpExpDxfRead::OnReadEllipse(const Base::Vector3d& center,
                                  double major_radius,
                                  double minor_radius,
                                  double rotation,
                                  double /*start_angle*/,
                                  double /*end_angle*/,
                                  bool dir)
// NOLINTEND(bugprone-easily-swappable-parameters)
{
    gp_Dir up(0, 0, 1);
    if (!dir) {
        up = -up;
    }
    gp_Pnt pc = makePoint(center);
    gp_Elips ellipse(gp_Ax2(pc, up), major_radius, minor_radius);
    ellipse.Rotate(gp_Ax1(pc, up), rotation);
    if (ellipse.MinorRadius() > 0) {
        Collector->AddObject(BRepBuilderAPI_MakeEdge(ellipse).Edge(), "Ellipse");
    }
    else {
        Base::Console().Warning("ImpExpDxf - ignore degenerate ellipse\n");
    }
}


void ImpExpDxfRead::OnReadText(const Base::Vector3d& point,
                               const double height,
                               const std::string& text,
                               const double rotation)
{
    // Note that our parameters do not contain all the information needed to properly orient the
    // text. As a result the text will always appear on the XY plane
    if (m_importAnnotations) {
        auto makeText = [this, rotation, point, text, height](
                            const Base::Matrix4D& transform) -> App::FeaturePython* {
            PyObject* draftModule = getDraftModule();
            if (draftModule != nullptr) {
                Base::Matrix4D localTransform;
                localTransform.rotZ(rotation);
                localTransform.move(point);
                PyObject* placement =
                    new Base::PlacementPy(Base::Placement(transform * localTransform));
                // returns a wrapped App::FeaturePython
                auto builtText = dynamic_cast<App::FeaturePythonPyT<App::DocumentObjectPy>*>(
                    // NOLINTNEXTLINE(readability/nolint)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                    (Base::PyObjectBase*)PyObject_CallMethod(draftModule,
                                                             "make_text",
                                                             "sOif",
                                                             text.c_str(),
                                                             placement,
                                                             0,
                                                             height));
                Py_DECREF(placement);
                if (builtText != nullptr) {
                    return dynamic_cast<App::FeaturePython*>(builtText->getDocumentObjectPtr());
                }
            }
            return nullptr;
        };
        Collector->AddObject((FeaturePythonBuilder)makeText);
    }
}


void ImpExpDxfRead::OnReadInsert(const Base::Vector3d& point,
                                 const Base::Vector3d& scale,
                                 const std::string& name,
                                 double rotation)
{
    Collector->AddInsert(point, scale, name, rotation);
}
void ImpExpDxfRead::ExpandInsert(const std::string& name,
                                 const Base::Matrix4D& transform,
                                 const Base::Vector3d& point,
                                 double rotation,
                                 const Base::Vector3d& scale)
{
    if (Blocks.count(name) == 0) {
        ImportError("Reference to undefined or external block '%s'\n", name);
        return;
    }
    Block& block = Blocks.at(name);
    // Apply the scaling, rotation, and move before the OCSEnttityTransform and place the result io
    // BaseEntityTransform,
    Base::Matrix4D localTransform;
    localTransform.scale(scale.x, scale.y, scale.z);
    localTransform.rotZ(rotation);
    localTransform.move(point[0], point[1], point[2]);
    localTransform = transform * localTransform;
    CommonEntityAttributes mainAttributes = m_entityAttributes;
    for (const auto& [attributes, shapes] : block.Shapes) {
        // Put attributes into m_entityAttributes after using the latter to set byblock values in
        // the former.
        m_entityAttributes = attributes;
        m_entityAttributes.ResolveByBlockAttributes(mainAttributes);

        for (const TopoDS_Shape& shape : shapes) {
            // TODO???: See the comment in TopoShape::makeTransform regarding calling
            // Moved(identityTransform) on the new shape
            Collector->AddObject(
                BRepBuilderAPI_Transform(shape,
                                         Part::TopoShape::convert(localTransform),
                                         Standard_True)
                    .Shape(),
                "InsertPart");  // TODO: The collection should contain the nameBase to use
        }
    }
    for (const auto& [attributes, featureBuilders] : block.FeatureBuildersList) {
        // Put attributes into m_entityAttributes after using the latter to set byblock values in
        // the former.
        m_entityAttributes = attributes;
        m_entityAttributes.ResolveByBlockAttributes(mainAttributes);

        for (const FeaturePythonBuilder& featureBuilder : featureBuilders) {
            // TODO: Any non-identity transform from the original entity record needs to be applied
            // before OCSEntityTransform (which includes this INSERT's transform followed by the
            // transform for the INSERT's context (i.e. from an outeer INSERT)
            // TODO: Perhaps pass a prefix ("Insert") to the builder to make the object name so
            // Draft objects in a block get named similarly to Shapes.
            App::FeaturePython* feature = featureBuilder(localTransform);
            if (feature != nullptr) {
                // Note that the featureBuilder has already placed this object in the drawing as a
                // top-level object, so we don't have to add them but we must place it in its layer
                // and set its gui styles
                MoveToLayer(feature);
                ApplyGuiStyles(feature);
            }
        }
    }
    for (const auto& [attributes, inserts] : block.Inserts) {
        // Put attributes into m_entityAttributes after using the latter to set byblock values in
        // the former.
        m_entityAttributes = attributes;
        m_entityAttributes.ResolveByBlockAttributes(mainAttributes);

        for (const Block::Insert& insert : inserts) {
            // TODO: Apply the OCSOrientationTransform saved with the Insert statement to
            // localTransform. (pass localTransform*insert.OCSDirectionTransform)
            ExpandInsert(insert.Name, localTransform, insert.Point, insert.Rotation, insert.Scale);
        }
    }
}


void ImpExpDxfRead::OnReadDimension(const Base::Vector3d& start,
                                    const Base::Vector3d& end,
                                    const Base::Vector3d& point,
                                    double /*rotation*/)
{
    if (m_importAnnotations) {
        auto makeDimension =
            [this, start, end, point](const Base::Matrix4D& transform) -> App::FeaturePython* {
            PyObject* draftModule = getDraftModule();
            if (draftModule != nullptr) {
                // TODO: Capture and apply OCSOrientationTransform to OCS coordinates
                // Note, some of the locations in the DXF are OCS and some are UCS, but UCS doesn't
                // mean UCS when in a block expansion, it means 'transform'
                // So we want transform*vector for "UCS" coordinates and transform*ocdCapture*vector
                // for "OCS" coordinates
                //
                // We implement the transform by mapping all the points from OCS to UCS
                // TODO: Set the Normal property to transform*(0,0,1,0)
                // TODO: Set the Direction property to transform*(the desired direction).
                // By default this is parallel to (start-end).
                PyObject* startPy = new Base::VectorPy(transform * start);
                PyObject* endPy = new Base::VectorPy(transform * end);
                PyObject* lineLocationPy = new Base::VectorPy(transform * point);
                // returns a wrapped App::FeaturePython
                auto builtDim = dynamic_cast<App::FeaturePythonPyT<App::DocumentObjectPy>*>(
                    // NOLINTNEXTLINE(readability/nolint)
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                    (Base::PyObjectBase*)PyObject_CallMethod(draftModule,
                                                             "make_linear_dimension",
                                                             "OOO",
                                                             startPy,
                                                             endPy,
                                                             lineLocationPy));
                Py_DECREF(startPy);
                Py_DECREF(endPy);
                Py_DECREF(lineLocationPy);
                if (builtDim != nullptr) {
                    return dynamic_cast<App::FeaturePython*>(builtDim->getDocumentObjectPtr());
                }
            }
            return nullptr;
        };
        Collector->AddObject((FeaturePythonBuilder)makeDimension);
    }
}
void ImpExpDxfRead::OnReadPolyline(std::list<VertexInfo>& vertices, int flags)
{
    std::map<CDxfRead::CommonEntityAttributes, std::list<TopoDS_Shape>> ShapesToCombine;
    {
        // TODO: Currently ExpandPolyline calls OnReadArc etc to generate the pieces, and these
        // create TopoShape objects which ShapeSavingEntityCollector can gather up.
        // Eventually when m_mergeOption being DraftObjects is implemented OnReadArc etc might
        // generate Draft objects which ShapeSavingEntityCollector does not save.
        // We need either a collector that collects everything (and we have to figure out
        // how to join Draft objects) or we need to temporarily set m_mergeOption to SingleShapes
        // if it is set to DraftObjects (and safely restore it on exceptions)
        // A clean way would be to give the collector a "makeDraftObjects" property,
        // and our special collector could give this the value 'false' whereas the main
        // collector would base this on the option setting.
        // Also ShapeSavingEntityCollector classifies by entityAttributes which is not needed here
        // because they are constant throughout.
        ShapeSavingEntityCollector savingCollector(*this, ShapesToCombine);
        ExplodePolyline(vertices, flags);
    }
    // Join the shapes.
    if (!ShapesToCombine.empty()) {
        // TODO: If we want Draft objects and all segments are straight lines we can make a draft
        // wire.
        CombineShapes(ShapesToCombine.begin()->second, "Polyline");
    }
}


ImpExpDxfRead::Layer::Layer(const std::string& name,
                            ColorIndex_t color,
                            std::string&& lineType,
                            PyObject* drawingLayer)
    : CDxfRead::Layer(name, color, std::move(lineType))
    , GroupContents(
          drawingLayer == nullptr
              ? nullptr
              : (App::PropertyLinkListHidden*)(((App::FeaturePythonPyT<App::DocumentObjectPy>*)
                                                    drawingLayer)
                                                   ->getPropertyContainerPtr())
                    ->getDynamicPropertyByName("Group"))
    , DraftLayerView(drawingLayer == nullptr ? nullptr
                                             : PyObject_GetAttrString(drawingLayer, "ViewObject"))
{}
ImpExpDxfRead::Layer::~Layer()
{
    Py_XDECREF(DraftLayerView);
}

void ImpExpDxfRead::Layer::FinishLayer() const
{
    if (GroupContents != nullptr) {
        // We have to move the object to layer->DraftLayer
        // The DraftLayer will have a Proxy attribute which has a addObject attribute which we
        // call with (draftLayer, draftObject) Checking from python, the layer is a
        // App::FeaturePython, and its Proxy is a draftobjects.layer.Layer
        GroupContents->setValue(Contents);
    }
    if (DraftLayerView != nullptr && Hidden) {
        // Hide the Hidden layers if possible (if GUI exists)
        // We do this now rather than when the layer is created so all objects
        // within the layers also become hidden.
        PyObject_CallMethod(DraftLayerView, "hide", nullptr);
    }
}

CDxfRead::Layer*
ImpExpDxfRead::MakeLayer(const std::string& name, ColorIndex_t color, std::string&& lineType)
{
    if (m_preserveLayers) {
        // Hidden layers are implemented in the wrapup code after the entire file has been read.
        App::Color appColor = ObjectColor(color);
        PyObject* draftModule = nullptr;
        PyObject* layer = nullptr;
        draftModule = getDraftModule();
        if (draftModule != nullptr) {
            // After the colours, I also want to pass the draw_style, but there is an intervening
            // line-width parameter. It is easier to just pass that parameter's default value than
            // to do the handstands to pass a named parameter.
            // TODO: Pass the appropriate draw_style (from "Solid" "Dashed" "Dotted" "DashDot")
            // This needs an ObjectDrawStyleName analogous to ObjectColor but at the ImpExpDxfGui
            // level.
            layer =
                // NOLINTNEXTLINE(readability/nolint)
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                (Base::PyObjectBase*)PyObject_CallMethod(draftModule,
                                                         "make_layer",
                                                         "s(fff)(fff)fs",
                                                         name.c_str(),
                                                         appColor.r,
                                                         appColor.g,
                                                         appColor.b,
                                                         appColor.r,
                                                         appColor.g,
                                                         appColor.b,
                                                         2.0,
                                                         "Solid");
        }
        auto result = new Layer(name, color, std::move(lineType), layer);
        if (result->DraftLayerView != nullptr) {
            PyObject_SetAttrString(result->DraftLayerView, "OverrideLineColorChildren", Py_False);
            PyObject_SetAttrString(result->DraftLayerView,
                                   "OverrideShapeAppearanceChildren",
                                   Py_False);
        }

        // We make our own layer class even if we could not make a layer. MoveToLayer will ignore
        // such layers but we have to do this because it is not a polymorphic type so we can't tell
        // what we pull out of m_entityAttributes.m_Layer.
        return result;
    }
    return CDxfRead::MakeLayer(name, color, std::move(lineType));
}
void ImpExpDxfRead::MoveToLayer(App::DocumentObject* object) const
{
    if (m_preserveLayers) {
        static_cast<Layer*>(m_entityAttributes.m_Layer)->Contents.push_back(object);
    }
    // TODO: else Hide the object if it is in a Hidden layer? That won't work because we've cleared
    // out m_entityAttributes.m_Layer
}


std::string ImpExpDxfRead::Deformat(const char* text)
{
    // this function removes DXF formatting from texts
    std::stringstream ss;
    bool escape = false;      // turned on when finding an escape character
    bool longescape = false;  // turned on for certain escape codes that expect additional chars
    for (unsigned int i = 0; i < strlen(text); i++) {
        char ch = text[i];
        if (ch == '\\') {
            escape = true;
        }
        else if (escape) {
            if (longescape) {
                if (ch == ';') {
                    escape = false;
                    longescape = false;
                }
            }
            else if ((ch == 'H') || (ch == 'h') || (ch == 'Q') || (ch == 'q') || (ch == 'W')
                     || (ch == 'w') || (ch == 'F') || (ch == 'f') || (ch == 'A') || (ch == 'a')
                     || (ch == 'C') || (ch == 'c') || (ch == 'T') || (ch == 't')) {
                longescape = true;
            }
            else {
                if ((ch == 'P') || (ch == 'p')) {
                    ss << "\n";
                }
                escape = false;
            }
        }
        else if ((ch != '{') && (ch != '}')) {
            ss << ch;
        }
    }
    return ss.str();
}

void ImpExpDxfRead::DrawingEntityCollector::AddObject(const TopoDS_Shape& shape,
                                                      const char* nameBase)
{
    auto pcFeature =
        dynamic_cast<Part::Feature*>(Reader.document->addObject("Part::Feature", nameBase));
    pcFeature->Shape.setValue(shape);
    Reader.MoveToLayer(pcFeature);
    Reader.ApplyGuiStyles(pcFeature);
}
void ImpExpDxfRead::DrawingEntityCollector::AddObject(FeaturePythonBuilder shapeBuilder)
{
    App::FeaturePython* shape = shapeBuilder(Reader.OCSOrientationTransform);
    if (shape != nullptr) {
        Reader.MoveToLayer(shape);
        Reader.ApplyGuiStyles(shape);
    }
}

//******************************************************************************
// writing

void gPntToTuple(double result[3], gp_Pnt& p)
{
    result[0] = p.X();
    result[1] = p.Y();
    result[2] = p.Z();
}

point3D gPntTopoint3D(gp_Pnt& p)
{
    point3D result = {p.X(), p.Y(), p.Z()};
    return result;
}

ImpExpDxfWrite::ImpExpDxfWrite(std::string filepath)
    : CDxfWrite(filepath.c_str())
{
    setOptionSource("User parameter:BaseApp/Preferences/Mod/Import");
    setOptions();
}

ImpExpDxfWrite::~ImpExpDxfWrite() = default;

void ImpExpDxfWrite::setOptions()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath(getOptionSource().c_str());
    optionMaxLength = hGrp->GetFloat("maxsegmentlength", 5.0);
    optionExpPoints = hGrp->GetBool("ExportPoints", false);
    m_version = hGrp->GetInt("DxfVersionOut", 14);
    optionPolyLine = hGrp->GetBool("DiscretizeEllipses", false);
    m_polyOverride = hGrp->GetBool("DiscretizeEllipses", false);
    setDataDir(App::Application::getResourceDir() + "Mod/Import/DxfPlate/");
}

void ImpExpDxfWrite::exportShape(const TopoDS_Shape input)
{
    // export Edges
    TopExp_Explorer edges(input, TopAbs_EDGE);
    for (int i = 1; edges.More(); edges.Next(), i++) {
        const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() == GeomAbs_Circle) {
            double f = adapt.FirstParameter();
            double l = adapt.LastParameter();
            gp_Pnt start = adapt.Value(f);
            gp_Pnt e = adapt.Value(l);
            if (fabs(l - f) > 1.0 && start.SquareDistance(e) < 0.001) {
                exportCircle(adapt);
            }
            else {
                exportArc(adapt);
            }
        }
        else if (adapt.GetType() == GeomAbs_Ellipse) {
            double f = adapt.FirstParameter();
            double l = adapt.LastParameter();
            gp_Pnt start = adapt.Value(f);
            gp_Pnt e = adapt.Value(l);
            if (fabs(l - f) > 1.0 && start.SquareDistance(e) < 0.001) {
                if (m_polyOverride) {
                    if (m_version >= 14) {
                        exportLWPoly(adapt);
                    }
                    else {  // m_version < 14
                        exportPolyline(adapt);
                    }
                }
                else if (optionPolyLine) {
                    if (m_version >= 14) {
                        exportLWPoly(adapt);
                    }
                    else {  // m_version < 14
                        exportPolyline(adapt);
                    }
                }
                else {  // no overrides, do what's right!
                    if (m_version < 14) {
                        exportPolyline(adapt);
                    }
                    else {
                        exportEllipse(adapt);
                    }
                }
            }
            else {  // it's an arc
                if (m_polyOverride) {
                    if (m_version >= 14) {
                        exportLWPoly(adapt);
                    }
                    else {  // m_version < 14
                        exportPolyline(adapt);
                    }
                }
                else if (optionPolyLine) {
                    if (m_version >= 14) {
                        exportLWPoly(adapt);
                    }
                    else {  // m_version < 14
                        exportPolyline(adapt);
                    }
                }
                else {  // no overrides, do what's right!
                    if (m_version < 14) {
                        exportPolyline(adapt);
                    }
                    else {
                        exportEllipseArc(adapt);
                    }
                }
            }
        }
        else if (adapt.GetType() == GeomAbs_BSplineCurve) {
            if (m_polyOverride) {
                if (m_version >= 14) {
                    exportLWPoly(adapt);
                }
                else {  // m_version < 14
                    exportPolyline(adapt);
                }
            }
            else if (optionPolyLine) {
                if (m_version >= 14) {
                    exportLWPoly(adapt);
                }
                else {  // m_version < 14
                    exportPolyline(adapt);
                }
            }
            else {  // no overrides, do what's right!
                if (m_version < 14) {
                    exportPolyline(adapt);
                }
                else {
                    exportBSpline(adapt);
                }
            }
        }
        else if (adapt.GetType() == GeomAbs_BezierCurve) {
            exportBCurve(adapt);
        }
        else if (adapt.GetType() == GeomAbs_Line) {
            exportLine(adapt);
        }
        else {
            Base::Console().Warning("ImpExpDxf - unknown curve type: %d\n",
                                    static_cast<int>(adapt.GetType()));
        }
    }

    if (optionExpPoints) {
        TopExp_Explorer verts(input, TopAbs_VERTEX);
        std::vector<gp_Pnt> duplicates;
        for (int i = 1; verts.More(); verts.Next(), i++) {
            const TopoDS_Vertex& v = TopoDS::Vertex(verts.Current());
            gp_Pnt p = BRep_Tool::Pnt(v);
            duplicates.push_back(p);
        }

        std::sort(duplicates.begin(), duplicates.end(), ImpExpDxfWrite::gp_PntCompare);
        auto newEnd =
            std::unique(duplicates.begin(), duplicates.end(), ImpExpDxfWrite::gp_PntEqual);
        std::vector<gp_Pnt> uniquePts(duplicates.begin(), newEnd);
        for (auto& p : uniquePts) {
            double point[3] = {0, 0, 0};
            gPntToTuple(point, p);
            writePoint(point);
        }
    }
}

bool ImpExpDxfWrite::gp_PntEqual(gp_Pnt p1, gp_Pnt p2)
{
    bool result = false;
    if (p1.IsEqual(p2, Precision::Confusion())) {
        result = true;
    }
    return result;
}

// is p1 "less than" p2?
bool ImpExpDxfWrite::gp_PntCompare(gp_Pnt p1, gp_Pnt p2)
{
    bool result = false;
    if (!(p1.IsEqual(p2, Precision::Confusion()))) {              // ie v1 != v2
        if (!(fabs(p1.X() - p2.X()) < Precision::Confusion())) {  // x1 != x2
            result = p1.X() < p2.X();
        }
        else if (!(fabs(p1.Y() - p2.Y()) < Precision::Confusion())) {  // y1 != y2
            result = p1.Y() < p2.Y();
        }
        else {
            result = p1.Z() < p2.Z();
        }
    }
    return result;
}


void ImpExpDxfWrite::exportCircle(BRepAdaptor_Curve& c)
{
    gp_Circ circ = c.Circle();
    gp_Pnt p = circ.Location();
    double center[3] = {0, 0, 0};
    gPntToTuple(center, p);

    double radius = circ.Radius();

    writeCircle(center, radius);
}

void ImpExpDxfWrite::exportEllipse(BRepAdaptor_Curve& c)
{
    gp_Elips ellp = c.Ellipse();
    gp_Pnt p = ellp.Location();
    double center[3] = {0, 0, 0};
    gPntToTuple(center, p);

    double major = ellp.MajorRadius();
    double minor = ellp.MinorRadius();

    gp_Dir xaxis = ellp.XAxis().Direction();  // direction of major axis
    // rotation appears to be the clockwise(?) angle between major & +Y??
    double rotation = xaxis.AngleWithRef(gp_Dir(0, 1, 0), gp_Dir(0, 0, 1));

    // 2*M_PI = 6.28319 is invalid(doesn't display in LibreCAD), but 2PI = 6.28318 is valid!
    // writeEllipse(center, major, minor, rotation, 0.0, 2 * M_PI, true );
    writeEllipse(center, major, minor, rotation, 0.0, 6.28318, true);
}

void ImpExpDxfWrite::exportArc(BRepAdaptor_Curve& c)
{
    gp_Circ circ = c.Circle();
    gp_Pnt p = circ.Location();
    double center[3] = {0, 0, 0};
    gPntToTuple(center, p);

    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    double start[3];
    gPntToTuple(start, s);
    gp_Pnt m = c.Value((l + f) / 2.0);
    gp_Pnt e = c.Value(l);
    double end[3] = {0, 0, 0};
    gPntToTuple(end, e);

    gp_Vec v1(m, s);
    gp_Vec v2(m, e);
    gp_Vec v3(0, 0, 1);
    double a = v3.DotCross(v1, v2);

    bool dir = (a < 0) ? true : false;
    writeArc(start, end, center, dir);
}

void ImpExpDxfWrite::exportEllipseArc(BRepAdaptor_Curve& c)
{
    gp_Elips ellp = c.Ellipse();
    gp_Pnt p = ellp.Location();
    double center[3] = {0, 0, 0};
    gPntToTuple(center, p);

    double major = ellp.MajorRadius();
    double minor = ellp.MinorRadius();

    gp_Dir xaxis = ellp.XAxis().Direction();  // direction of major axis
    // rotation appears to be the clockwise angle between major & +Y??
    double rotation = xaxis.AngleWithRef(gp_Dir(0, 1, 0), gp_Dir(0, 0, 1));

    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    gp_Pnt m = c.Value((l + f) / 2.0);
    gp_Pnt e = c.Value(l);

    gp_Vec v1(m, s);
    gp_Vec v2(m, e);
    gp_Vec v3(0, 0, 1);
    double a = v3.DotCross(v1, v2);  // a = v3 dot (v1 cross v2)
                                     // relates to "handedness" of 3 vectors
                                     // a > 0 ==> v2 is CCW from v1 (righthanded)?
                                     // a < 0 ==> v2 is CW from v1 (lefthanded)?

    double startAngle = fmod(f, 2.0 * M_PI);  // revolutions
    double endAngle = fmod(l, 2.0 * M_PI);
    bool endIsCW = (a < 0) ? true : false;  // if !endIsCW swap(start,end)
    // not sure if this is a hack or not. seems to make valid arcs.
    if (!endIsCW) {
        startAngle = -startAngle;
        endAngle = -endAngle;
    }

    writeEllipse(center, major, minor, rotation, startAngle, endAngle, endIsCW);
}

void ImpExpDxfWrite::exportBSpline(BRepAdaptor_Curve& c)
{
    SplineDataOut sd;
    Handle(Geom_BSplineCurve) spline;
    double f, l;
    gp_Pnt s, ePt;

    Standard_Real tol3D = 0.001;
    Standard_Integer maxDegree = 3, maxSegment = 200;
    Handle(BRepAdaptor_HCurve) hCurve = new BRepAdaptor_HCurve(c);
    Approx_Curve3d approx(hCurve, tol3D, GeomAbs_C0, maxSegment, maxDegree);
    if (approx.IsDone() && approx.HasResult()) {
        spline = approx.Curve();
    }
    else {
        if (approx.HasResult()) {  // result, but not within tolerance
            spline = approx.Curve();
            Base::Console().Message("DxfWrite::exportBSpline - result not within tolerance\n");
        }
        else {
            f = c.FirstParameter();
            l = c.LastParameter();
            s = c.Value(f);
            ePt = c.Value(l);
            Base::Console().Message(
                "DxfWrite::exportBSpline - no result- from:(%.3f,%.3f) to:(%.3f,%.3f)\n",
                s.X(),
                s.Y(),
                ePt.X(),
                ePt.Y());
            TColgp_Array1OfPnt controlPoints(0, 1);
            controlPoints.SetValue(0, s);
            controlPoints.SetValue(1, ePt);
            spline = GeomAPI_PointsToBSpline(controlPoints, 1).Curve();
        }
    }
    // WF? norm of surface containing curve??
    sd.norm.x = 0.0;
    sd.norm.y = 0.0;
    sd.norm.z = 1.0;

    sd.flag = spline->IsClosed();
    sd.flag += spline->IsPeriodic() * 2;
    sd.flag += spline->IsRational() * 4;
    sd.flag += 8;  // planar spline

    sd.degree = spline->Degree();
    sd.control_points = spline->NbPoles();
    sd.knots = spline->NbKnots();
    gp_Pnt p;
    spline->D0(spline->FirstParameter(), p);
    sd.starttan = gPntTopoint3D(p);
    spline->D0(spline->LastParameter(), p);
    sd.endtan = gPntTopoint3D(p);

    // next bit is from DrawingExport.cpp (Dan Falk?).
    Standard_Integer m = 0;
    if (spline->IsPeriodic()) {
        m = spline->NbPoles() + 2 * spline->Degree() - spline->Multiplicity(1) + 2;
    }
    else {
        for (int i = 1; i <= spline->NbKnots(); i++) {
            m += spline->Multiplicity(i);
        }
    }
    TColStd_Array1OfReal knotsequence(1, m);
    spline->KnotSequence(knotsequence);
    for (int i = knotsequence.Lower(); i <= knotsequence.Upper(); i++) {
        sd.knot.push_back(knotsequence(i));
    }
    sd.knots = knotsequence.Length();

    TColgp_Array1OfPnt poles(1, spline->NbPoles());
    spline->Poles(poles);
    for (int i = poles.Lower(); i <= poles.Upper(); i++) {
        sd.control.push_back(gPntTopoint3D(poles(i)));
    }
    // OCC doesn't have separate lists for control points and fit points.

    writeSpline(sd);
}

void ImpExpDxfWrite::exportBCurve(BRepAdaptor_Curve& c)
{
    (void)c;
    Base::Console().Message("BCurve dxf export not yet supported\n");
}

void ImpExpDxfWrite::exportLine(BRepAdaptor_Curve& c)
{
    double f = c.FirstParameter();
    double l = c.LastParameter();
    gp_Pnt s = c.Value(f);
    double start[3] = {0, 0, 0};
    gPntToTuple(start, s);
    gp_Pnt e = c.Value(l);
    double end[3] = {0, 0, 0};
    gPntToTuple(end, e);
    writeLine(start, end);
}

void ImpExpDxfWrite::exportLWPoly(BRepAdaptor_Curve& c)
{
    LWPolyDataOut pd;
    pd.Flag = c.IsClosed();
    pd.Elev = 0.0;
    pd.Thick = 0.0;
    pd.Extr.x = 0.0;
    pd.Extr.y = 0.0;
    pd.Extr.z = 1.0;
    pd.nVert = 0;

    GCPnts_UniformAbscissa discretizer;
    discretizer.Initialize(c, optionMaxLength);
    std::vector<point3D> points;
    if (discretizer.IsDone() && discretizer.NbPoints() > 0) {
        int nbPoints = discretizer.NbPoints();
        for (int i = 1; i <= nbPoints; i++) {
            gp_Pnt p = c.Value(discretizer.Parameter(i));
            pd.Verts.push_back(gPntTopoint3D(p));
        }
        pd.nVert = discretizer.NbPoints();
        writeLWPolyLine(pd);
    }
}

void ImpExpDxfWrite::exportPolyline(BRepAdaptor_Curve& c)
{
    LWPolyDataOut pd;
    pd.Flag = c.IsClosed();
    pd.Elev = 0.0;
    pd.Thick = 0.0;
    pd.Extr.x = 0.0;
    pd.Extr.y = 0.0;
    pd.Extr.z = 1.0;
    pd.nVert = 0;

    GCPnts_UniformAbscissa discretizer;
    discretizer.Initialize(c, optionMaxLength);
    std::vector<point3D> points;
    if (discretizer.IsDone() && discretizer.NbPoints() > 0) {
        int nbPoints = discretizer.NbPoints();
        for (int i = 1; i <= nbPoints; i++) {
            gp_Pnt p = c.Value(discretizer.Parameter(i));
            pd.Verts.push_back(gPntTopoint3D(p));
        }
        pd.nVert = discretizer.NbPoints();
        writePolyline(pd);
    }
}

void ImpExpDxfWrite::exportText(const char* text,
                                Base::Vector3d position1,
                                Base::Vector3d position2,
                                double size,
                                int just)
{
    double location1[3] = {0, 0, 0};
    location1[0] = position1.x;
    location1[1] = position1.y;
    location1[2] = position1.z;
    double location2[3] = {0, 0, 0};
    location2[0] = position2.x;
    location2[1] = position2.y;
    location2[2] = position2.z;

    writeText(text, location1, location2, size, just);
}

void ImpExpDxfWrite::exportLinearDim(Base::Vector3d textLocn,
                                     Base::Vector3d lineLocn,
                                     Base::Vector3d extLine1Start,
                                     Base::Vector3d extLine2Start,
                                     char* dimText,
                                     int type)
{
    double text[3] = {0, 0, 0};
    text[0] = textLocn.x;
    text[1] = textLocn.y;
    text[2] = textLocn.z;
    double line[3] = {0, 0, 0};
    line[0] = lineLocn.x;
    line[1] = lineLocn.y;
    line[2] = lineLocn.z;
    double ext1[3] = {0, 0, 0};
    ext1[0] = extLine1Start.x;
    ext1[1] = extLine1Start.y;
    ext1[2] = extLine1Start.z;
    double ext2[3] = {0, 0, 0};
    ext2[0] = extLine2Start.x;
    ext2[1] = extLine2Start.y;
    ext2[2] = extLine2Start.z;
    writeLinearDim(text, line, ext1, ext2, dimText, type);
}

void ImpExpDxfWrite::exportAngularDim(Base::Vector3d textLocn,
                                      Base::Vector3d lineLocn,
                                      Base::Vector3d extLine1End,
                                      Base::Vector3d extLine2End,
                                      Base::Vector3d apexPoint,
                                      char* dimText)
{
    double text[3] = {0, 0, 0};
    text[0] = textLocn.x;
    text[1] = textLocn.y;
    text[2] = textLocn.z;
    double line[3] = {0, 0, 0};
    line[0] = lineLocn.x;
    line[1] = lineLocn.y;
    line[2] = lineLocn.z;
    double ext1[3] = {0, 0, 0};
    ext1[0] = extLine1End.x;
    ext1[1] = extLine1End.y;
    ext1[2] = extLine1End.z;
    double ext2[3] = {0, 0, 0};
    ext2[0] = extLine2End.x;
    ext2[1] = extLine2End.y;
    ext2[2] = extLine2End.z;
    double apex[3] = {0, 0, 0};
    apex[0] = apexPoint.x;
    apex[1] = apexPoint.y;
    apex[2] = apexPoint.z;
    writeAngularDim(text, line, apex, ext1, apex, ext2, dimText);
}

void ImpExpDxfWrite::exportRadialDim(Base::Vector3d centerPoint,
                                     Base::Vector3d textLocn,
                                     Base::Vector3d arcPoint,
                                     char* dimText)
{
    double center[3] = {0, 0, 0};
    center[0] = centerPoint.x;
    center[1] = centerPoint.y;
    center[2] = centerPoint.z;
    double text[3] = {0, 0, 0};
    text[0] = textLocn.x;
    text[1] = textLocn.y;
    text[2] = textLocn.z;
    double arc[3] = {0, 0, 0};
    arc[0] = arcPoint.x;
    arc[1] = arcPoint.y;
    arc[2] = arcPoint.z;
    writeRadialDim(center, text, arc, dimText);
}

void ImpExpDxfWrite::exportDiametricDim(Base::Vector3d textLocn,
                                        Base::Vector3d arcPoint1,
                                        Base::Vector3d arcPoint2,
                                        char* dimText)
{
    double text[3] = {0, 0, 0};
    text[0] = textLocn.x;
    text[1] = textLocn.y;
    text[2] = textLocn.z;
    double arc1[3] = {0, 0, 0};
    arc1[0] = arcPoint1.x;
    arc1[1] = arcPoint1.y;
    arc1[2] = arcPoint1.z;
    double arc2[3] = {0, 0, 0};
    arc2[0] = arcPoint2.x;
    arc2[1] = arcPoint2.y;
    arc2[2] = arcPoint2.z;
    writeDiametricDim(text, arc1, arc2, dimText);
}
