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
#include <BRepAdaptor_CompCurve.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GeomAPI_Interpolate.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Line.hxx>
#include <Geom_BSplineCurve.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Elips.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>
#endif

#include <fstream>
#include <App/Annotation.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/DocumentObjectPy.h>
#include <App/FeaturePythonPyImp.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Matrix.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>
#include <Base/PlacementPy.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/FeatureCompound.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/FeaturePartCircle.h>
#include <App/Link.h>
#include <App/FeaturePython.h>
#include <Base/Tools.h>

#include "ImpExpDxf.h"


using namespace Import;

#if OCC_VERSION_HEX >= 0x070600
using BRepAdaptor_HCurve = BRepAdaptor_Curve;
#endif

namespace
{

Part::Circle* createCirclePrimitive(const TopoDS_Edge& edge, App::Document* doc, const char* name);
Part::Line* createLinePrimitive(const TopoDS_Edge& edge, App::Document* doc, const char* name);
Part::Ellipse*
createEllipsePrimitive(const TopoDS_Edge& edge, App::Document* doc, const char* name);
Part::Vertex*
createVertexPrimitive(const TopoDS_Vertex& vertex, App::Document* doc, const char* name);
Part::Feature*
createGenericShapeFeature(const TopoDS_Shape& shape, App::Document* doc, const char* name);

}  // namespace

namespace
{

// Helper function to create and configure a Part::Ellipse primitive from a TopoDS_Edge
Part::Ellipse* createEllipsePrimitive(const TopoDS_Edge& edge, App::Document* doc, const char* name)
{
    auto* p = doc->addObject<Part::Ellipse>(name);
    if (!p) {
        return nullptr;
    }

    TopLoc_Location loc;
    Standard_Real first, last;
    Handle(Geom_Curve) aCurve = BRep_Tool::Curve(edge, loc, first, last);

    if (aCurve->IsInstance(Geom_Ellipse::get_type_descriptor())) {
        Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(aCurve);

        // Set parametric properties
        p->MajorRadius.setValue(ellipse->MajorRadius());
        p->MinorRadius.setValue(ellipse->MinorRadius());

        // The axis contains the full transformation (location and orientation).
        // It's crucial to apply the TopLoc_Location transformation from the edge.
        gp_Ax2 axis = ellipse->Position().Transformed(loc.Transformation());
        gp_Pnt center = axis.Location();
        gp_Dir xDir = axis.XDirection();  // Major Axis Direction
        gp_Dir yDir = axis.YDirection();  // Minor Axis Direction
        gp_Dir zDir = axis.Direction();   // Normal

        Base::Placement plc;
        plc.setPosition(Base::Vector3d(center.X(), center.Y(), center.Z()));
        plc.setRotation(
            Base::Rotation::makeRotationByAxes(Base::Vector3d(xDir.X(), xDir.Y(), xDir.Z()),
                                               Base::Vector3d(yDir.X(), yDir.Y(), yDir.Z()),
                                               Base::Vector3d(zDir.X(), zDir.Y(), zDir.Z())));
        p->Placement.setValue(plc);

        // Set angles for arcs, converting from radians (OCC) to degrees (PropertyAngle)
        BRep_Tool::Range(edge, first, last);
        p->Angle1.setValue(Base::toDegrees(first));
        p->Angle2.setValue(Base::toDegrees(last));
    }
    return p;
}

// Helper function to create and configure a Part::Circle primitive from a TopoDS_Edge
Part::Circle* createCirclePrimitive(const TopoDS_Edge& edge, App::Document* doc, const char* name)
{
    auto* p = doc->addObject<Part::Circle>(name);
    if (!p) {
        return nullptr;
    }

    TopLoc_Location loc;
    Standard_Real first, last;
    Handle(Geom_Curve) aCurve = BRep_Tool::Curve(edge, loc, first, last);

    if (aCurve->IsInstance(Geom_Circle::get_type_descriptor())) {
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(aCurve);
        p->Radius.setValue(circle->Radius());

        // The axis contains the full transformation (location and orientation).
        gp_Ax2 axis = circle->Position().Transformed(loc.Transformation());
        gp_Pnt center = axis.Location();
        gp_Dir xDir = axis.XDirection();
        gp_Dir yDir = axis.YDirection();
        gp_Dir zDir = axis.Direction();

        Base::Placement plc;
        plc.setPosition(Base::Vector3d(center.X(), center.Y(), center.Z()));
        plc.setRotation(
            Base::Rotation::makeRotationByAxes(Base::Vector3d(xDir.X(), xDir.Y(), xDir.Z()),
                                               Base::Vector3d(yDir.X(), yDir.Y(), yDir.Z()),
                                               Base::Vector3d(zDir.X(), zDir.Y(), zDir.Z())));
        p->Placement.setValue(plc);

        // Set angles for arcs
        BRep_Tool::Range(edge, first, last);
        p->Angle1.setValue(Base::toDegrees(first));
        p->Angle2.setValue(Base::toDegrees(last));
    }
    return p;
}

// Helper function to create and configure a Part::Line primitive from a TopoDS_Edge
Part::Line* createLinePrimitive(const TopoDS_Edge& edge, App::Document* doc, const char* name)
{
    auto* p = doc->addObject<Part::Line>(name);
    if (!p) {
        return nullptr;
    }

    TopoDS_Vertex v1, v2;
    TopExp::Vertices(edge, v1, v2);
    gp_Pnt p1 = BRep_Tool::Pnt(v1);
    gp_Pnt p2 = BRep_Tool::Pnt(v2);

    p->X1.setValue(p1.X());
    p->Y1.setValue(p1.Y());
    p->Z1.setValue(p1.Z());
    p->X2.setValue(p2.X());
    p->Y2.setValue(p2.Y());
    p->Z2.setValue(p2.Z());

    return p;
}

// Helper function to create and configure a Part::Vertex primitive from a TopoDS_Vertex
Part::Vertex*
createVertexPrimitive(const TopoDS_Vertex& vertex, App::Document* doc, const char* name)
{
    auto* p = doc->addObject<Part::Vertex>(name);
    if (p) {
        gp_Pnt pnt = BRep_Tool::Pnt(vertex);
        p->X.setValue(pnt.X());
        p->Y.setValue(pnt.Y());
        p->Z.setValue(pnt.Z());
    }
    return p;
}

// Helper function to create a generic Part::Feature for any non-parametric shape
Part::Feature*
createGenericShapeFeature(const TopoDS_Shape& shape, App::Document* doc, const char* name)
{
    auto* p = doc->addObject<Part::Feature>(name);
    if (p) {
        p->Shape.setValue(shape);
    }
    return p;
}

}  // namespace

TopoDS_Wire ImpExpDxfRead::BuildWireFromPolyline(std::list<VertexInfo>& vertices, int flags)
{
    BRepBuilderAPI_MakeWire wireBuilder;
    bool is_closed = ((flags & 1) != 0);
    if (vertices.empty()) {
        return wireBuilder.Wire();
    }

    auto it = vertices.begin();
    auto prev_it = it++;

    while (it != vertices.end()) {
        const VertexInfo& start_vertex = *prev_it;
        const VertexInfo& end_vertex = *it;
        TopoDS_Edge edge;

        if (start_vertex.bulge == 0.0) {
            edge = BRepBuilderAPI_MakeEdge(makePoint(start_vertex.location),
                                           makePoint(end_vertex.location))
                       .Edge();
        }
        else {
            double cot = ((1.0 / start_vertex.bulge) - start_vertex.bulge) / 2.0;
            double center_x = ((start_vertex.location.x + end_vertex.location.x)
                               - (end_vertex.location.y - start_vertex.location.y) * cot)
                / 2.0;
            double center_y = ((start_vertex.location.y + end_vertex.location.y)
                               + (end_vertex.location.x - start_vertex.location.x) * cot)
                / 2.0;
            double center_z = (start_vertex.location.z + end_vertex.location.z) / 2.0;
            Base::Vector3d center(center_x, center_y, center_z);

            gp_Pnt p0 = makePoint(start_vertex.location);
            gp_Pnt p1 = makePoint(end_vertex.location);
            gp_Dir up(0, 0, 1);
            if (start_vertex.bulge < 0) {
                up.Reverse();
            }
            gp_Pnt pc = makePoint(center);
            gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
            if (circle.Radius() > 1e-9) {
                edge = BRepBuilderAPI_MakeEdge(circle, p0, p1).Edge();
            }
        }

        if (!edge.IsNull()) {
            wireBuilder.Add(edge);
        }
        prev_it = it++;
    }

    if (is_closed && vertices.size() > 1) {
        const VertexInfo& start_vertex = vertices.back();
        const VertexInfo& end_vertex = vertices.front();
        TopoDS_Edge edge;

        if (start_vertex.bulge == 0.0) {
            edge = BRepBuilderAPI_MakeEdge(makePoint(start_vertex.location),
                                           makePoint(end_vertex.location))
                       .Edge();
        }
        else {
            double cot = ((1.0 / start_vertex.bulge) - start_vertex.bulge) / 2.0;
            double center_x = ((start_vertex.location.x + end_vertex.location.x)
                               - (end_vertex.location.y - start_vertex.location.y) * cot)
                / 2.0;
            double center_y = ((start_vertex.location.y + end_vertex.location.y)
                               + (end_vertex.location.x - start_vertex.location.x) * cot)
                / 2.0;
            double center_z = (start_vertex.location.z + end_vertex.location.z) / 2.0;
            Base::Vector3d center(center_x, center_y, center_z);

            gp_Pnt p0 = makePoint(start_vertex.location);
            gp_Pnt p1 = makePoint(end_vertex.location);
            gp_Dir up(0, 0, 1);
            if (start_vertex.bulge < 0) {
                up.Reverse();
            }
            gp_Pnt pc = makePoint(center);
            gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
            if (circle.Radius() > 1e-9) {
                edge = BRepBuilderAPI_MakeEdge(circle, p0, p1).Edge();
            }
        }
        if (!edge.IsNull()) {
            wireBuilder.Add(edge);
        }
    }

    return wireBuilder.Wire();
}

Part::Feature* ImpExpDxfRead::createFlattenedPolylineFeature(const TopoDS_Wire& wire,
                                                             const char* name)
{
    auto* p = document->addObject<Part::Feature>(document->getUniqueObjectName(name).c_str());
    if (p) {
        p->Shape.setValue(wire);
        IncrementCreatedObjectCount();
    }
    return p;
}

Part::Compound* ImpExpDxfRead::createParametricPolylineCompound(const TopoDS_Wire& wire,
                                                                const char* name)
{
    auto* p = document->addObject<Part::Compound>(document->getUniqueObjectName(name).c_str());
    IncrementCreatedObjectCount();

    std::vector<App::DocumentObject*> segments;
    TopExp_Explorer explorer(wire, TopAbs_EDGE);

    for (; explorer.More(); explorer.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(explorer.Current());
        App::DocumentObject* segment = nullptr;
        BRepAdaptor_Curve adaptor(edge);

        if (adaptor.GetType() == GeomAbs_Line) {
            segment = createLinePrimitive(edge, document, "Segment");
        }
        else if (adaptor.GetType() == GeomAbs_Circle) {
            segment = createCirclePrimitive(edge, document, "Arc");
        }

        if (segment) {
            IncrementCreatedObjectCount();
            segment->Visibility.setValue(false);
            // We apply styles later, depending on the context
            segments.push_back(segment);
        }
    }
    p->Links.setValues(segments);
    return p;
}

void ImpExpDxfRead::CreateFlattenedPolyline(const TopoDS_Wire& wire, const char* name)
{
    Part::Feature* p = createFlattenedPolylineFeature(wire, name);

    // Perform the context-specific action of adding it to the collector
    if (p) {
        Collector->AddObject(p, name);
    }
}

void ImpExpDxfRead::CreateParametricPolyline(const TopoDS_Wire& wire, const char* name)
{
    Part::Compound* p = createParametricPolylineCompound(wire, name);

    // Perform the context-specific actions (applying styles and adding to the document)
    if (p) {
        // Style the child segments
        for (App::DocumentObject* segment : p->Links.getValues()) {
            ApplyGuiStyles(static_cast<Part::Feature*>(segment));
        }
        // Add the final compound object to the document
        Collector->AddObject(p, name);
    }
}

std::map<std::string, int> ImpExpDxfRead::PreScan(const std::string& filepath)
{
    std::map<std::string, int> counts;
    std::ifstream ifs(filepath);
    if (!ifs) {
        // Could throw an exception or log an error
        return counts;
    }

    std::string line;
    bool next_is_entity_name = false;

    while (std::getline(ifs, line)) {
        // Simple trim for Windows-style carriage returns
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (next_is_entity_name) {
            // The line after a "  0" group code is the entity type
            counts[line]++;
            next_is_entity_name = false;
        }
        else if (line == "  0") {
            next_is_entity_name = true;
        }
    }
    return counts;
}

//******************************************************************************
// reading
ImpExpDxfRead::ImpExpDxfRead(const std::string& filepath, App::Document* pcDoc)
    : CDxfRead(filepath)
    , document(pcDoc)
{
    setOptionSource("User parameter:BaseApp/Preferences/Mod/Draft");
    setOptions();
}

void ImpExpDxfRead::StartImport()
{
    CDxfRead::StartImport();
    // Create a hidden group to store the base objects for block definitions
    m_blockDefinitionGroup = static_cast<App::DocumentObjectGroup*>(
        document->addObject("App::DocumentObjectGroup", "_BlockDefinitions"));
    m_blockDefinitionGroup->Visibility.setValue(false);
    // Create a hidden group to store unreferenced blocks
    m_unreferencedBlocksGroup = static_cast<App::DocumentObjectGroup*>(
        document->addObject("App::DocumentObjectGroup", "_UnreferencedBlocks"));
    m_unreferencedBlocksGroup->Visibility.setValue(false);
}

bool ImpExpDxfRead::ReadEntitiesSection()
{
    // After parsing the BLOCKS section, compose all block definitions
    // into FreeCAD objects before processing the ENTITIES section.
    ComposeBlocks();

    DrawingEntityCollector collector(*this);
    if (m_importMode == ImportMode::FusedShapes) {
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

TopoDS_Shape ImpExpDxfRead::CombineShapesToCompound(const std::list<TopoDS_Shape>& shapes) const
{
    if (shapes.empty()) {
        return TopoDS_Shape();
    }
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    for (const auto& sh : shapes) {
        if (!sh.IsNull()) {
            builder.Add(comp, sh);
        }
    }
    return comp;
}

void ImpExpDxfRead::setOptions()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath(getOptionSource().c_str());
    m_stats.importSettings.clear();

    m_preserveLayers = hGrp->GetBool("dxfUseDraftVisGroups", true);
    m_stats.importSettings["Use layers"] = m_preserveLayers ? "Yes" : "No";

    m_preserveColors = hGrp->GetBool("dxfGetOriginalColors", true);
    m_stats.importSettings["Use colors from the DXF file"] = m_preserveColors ? "Yes" : "No";

    // Read the new master import mode parameter, set the default.
    int mode = hGrp->GetInt("DxfImportMode", static_cast<int>(ImportMode::IndividualShapes));
    m_importMode = static_cast<ImportMode>(mode);

    // TODO: joingeometry should give an intermediate between MergeShapes and SingleShapes which
    // will merge shapes that happen to join end-to-end. As such it should be in the radio button
    // set, except that the legacy importer can do joining either for sketches or for shapes. What
    // this really means is there should be an "Import as sketch" checkbox, and only the
    // MergeShapes, JoinShapes, and SingleShapes radio buttons should be allowed, i.e. Draft Objects
    // would be ignored.
    // Update: The "Join geometry" option is now a checkbox that is only enabled for the legacy
    // importer. Whether the modern importer should support this is still up for debate.
    bool joinGeometry = hGrp->GetBool("joingeometry", false);
    m_stats.importSettings["Join geometry"] = joinGeometry ? "Yes" : "No";

    double scaling = hGrp->GetFloat("dxfScaling", 1.0);
    SetAdditionalScaling(scaling);
    m_stats.importSettings["Manual scaling factor"] = std::to_string(scaling);

    m_importAnnotations = hGrp->GetBool("dxftext", false);
    m_stats.importSettings["Import texts and dimensions"] = m_importAnnotations ? "Yes" : "No";

    m_importPoints = hGrp->GetBool("dxfImportPoints", true);
    m_stats.importSettings["Import points"] = m_importPoints ? "Yes" : "No";

    m_importPaperSpaceEntities = hGrp->GetBool("dxflayout", false);
    m_stats.importSettings["Import layout objects"] = m_importPaperSpaceEntities ? "Yes" : "No";

    m_importHiddenBlocks = hGrp->GetBool("dxfstarblocks", false);
    m_stats.importSettings["Import hidden blocks"] = m_importHiddenBlocks ? "Yes" : "No";

    // TODO: There is currently no option for this: m_importFrozenLayers =
    // hGrp->GetBool("dxffrozenLayers", false);
    // TODO: There is currently no option for this: m_importHiddenLayers =
    // hGrp->GetBool("dxfhiddenLayers", true);
}

void ImpExpDxfRead::ComposeFlattenedBlock(const std::string& blockName,
                                          std::set<std::string>& composed)
{
    // 1. Base Case: If already composed, do nothing.
    if (composed.count(blockName)) {
        return;
    }

    // 2. Find the raw block data.
    auto it = this->Blocks.find(blockName);
    if (it == this->Blocks.end()) {
        ImportError("Block '%s' is referenced but not defined. Skipping.", blockName.c_str());
        return;
    }
    const Block& blockData = it->second;

    // 3. Collect all geometry shapes for this block.
    std::list<TopoDS_Shape> shapeCollection;

    // 4. Process primitive geometry.
    for (const auto& [attributes, builderList] : blockData.GeometryBuilders) {
        for (const auto& builder : builderList) {
            shapeCollection.push_back(builder.shape);
        }
    }

    // 5. Process nested inserts recursively.
    for (const auto& insertAttrPair : blockData.Inserts) {
        for (const auto& nestedInsert : insertAttrPair.second) {
            // Ensure the nested block is composed first.
            ComposeFlattenedBlock(nestedInsert.Name, composed);
            // Mark the nested block as referenced so it's not moved to the "Unreferenced" group.
            m_referencedBlocks.insert(nestedInsert.Name);

            // Retrieve the final, flattened shape of the nested block.
            auto shape_it = m_flattenedBlockShapes.find(nestedInsert.Name);
            if (shape_it != m_flattenedBlockShapes.end()) {
                if (!shape_it->second.IsNull()) {
                    // Use the Part::TopoShape wrapper to access the transformShape method.
                    Part::TopoShape nestedShape(shape_it->second);
                    // Apply the insert's transformation.
                    Base::Placement pl(
                        nestedInsert.Point,
                        Base::Rotation(Base::Vector3d(0, 0, 1), nestedInsert.Rotation));
                    Base::Matrix4D transform = pl.toMatrix();
                    transform.scale(nestedInsert.Scale);
                    nestedShape.transformShape(transform, true, true);  // Use copy=true
                    shapeCollection.push_back(nestedShape.getShape());
                }
            }
        }
    }

    // 6. Build the final merged shape.
    TopoDS_Shape finalShape = CombineShapesToCompound(shapeCollection);
    m_flattenedBlockShapes[blockName] = finalShape;  // Cache the result.

    // 7. Create the final Part::Feature object.
    if (!finalShape.IsNull()) {
        std::string featureName = "BLOCK_" + blockName;
        auto blockFeature = document->addObject<Part::Feature>(
            document->getUniqueObjectName(featureName.c_str()).c_str());
        blockFeature->Shape.setValue(finalShape);
        blockFeature->Visibility.setValue(false);
        m_blockDefinitionGroup->addObject(blockFeature);
        this->m_blockDefinitions[blockName] = blockFeature;
    }

    // 8. Mark this block as composed.
    composed.insert(blockName);
}

void ImpExpDxfRead::ComposeParametricBlock(const std::string& blockName,
                                           std::set<std::string>& composed)
{
    // 1. Base Case: If this block has already been composed, we're done.
    if (composed.count(blockName)) {
        return;
    }

    // 2. Find the raw block data from the parsing phase.
    auto it = this->Blocks.find(blockName);
    if (it == this->Blocks.end()) {
        ImportError("Block '%s' is referenced but not defined. Skipping.", blockName.c_str());
        return;
    }
    const Block& blockData = it->second;

    // 3. Create the master Part::Compound for this block definition.
    std::string compName = "BLOCK_" + blockName;
    auto blockCompound = document->addObject<Part::Compound>(
        document->getUniqueObjectName(compName.c_str()).c_str());
    m_blockDefinitionGroup->addObject(blockCompound);
    IncrementCreatedObjectCount();
    blockCompound->Visibility.setValue(false);
    this->m_blockDefinitions[blockName] = blockCompound;

    std::vector<App::DocumentObject*> childObjects;

    // 4. Recursively Compose and Link Nested Inserts.
    for (const auto& insertAttrPair : blockData.Inserts) {
        for (const auto& nestedInsert : insertAttrPair.second) {
            // Ensure the dependency is composed before we try to link to it.
            ComposeParametricBlock(nestedInsert.Name, composed);
            // Mark the nested block as referenced so it's not moved to the "Unreferenced" group.
            m_referencedBlocks.insert(nestedInsert.Name);

            // Create the App::Link for this nested insert.
            auto baseObjIt = m_blockDefinitions.find(nestedInsert.Name);
            if (baseObjIt != m_blockDefinitions.end()) {
                // The link's name should be based on the block it is inserting, not the parent.
                std::string linkName = "Link_" + nestedInsert.Name;
                auto link = document->addObject<App::Link>(
                    document->getUniqueObjectName(linkName.c_str()).c_str());
                link->setLink(-1, baseObjIt->second);
                link->LinkTransform.setValue(false);

                // Apply placement and scale to the link itself.
                Base::Placement pl(nestedInsert.Point,
                                   Base::Rotation(Base::Vector3d(0, 0, 1), nestedInsert.Rotation));
                link->Placement.setValue(pl);
                link->ScaleVector.setValue(nestedInsert.Scale);
                link->Visibility.setValue(false);
                IncrementCreatedObjectCount();
                childObjects.push_back(link);
            }
        }
    }

    // 5. Create and Link Primitive Geometry from the collected builders.
    for (const auto& [attributes, builderList] : blockData.GeometryBuilders) {
        this->m_entityAttributes = attributes;  // Set attributes for layer/color handling

        for (const auto& builder : builderList) {
            App::DocumentObject* newObject = nullptr;
            switch (builder.type) {
                // Existing cases for other primitives
                case GeometryBuilder::PrimitiveType::Line: {
                    newObject = createLinePrimitive(TopoDS::Edge(builder.shape), document, "Line");
                    break;
                }
                case GeometryBuilder::PrimitiveType::Point: {
                    newObject =
                        createVertexPrimitive(TopoDS::Vertex(builder.shape), document, "Point");
                    break;
                }
                case GeometryBuilder::PrimitiveType::Circle:
                case GeometryBuilder::PrimitiveType::Arc: {
                    const char* name =
                        (builder.type == GeometryBuilder::PrimitiveType::Circle) ? "Circle" : "Arc";
                    auto* p = createCirclePrimitive(TopoDS::Edge(builder.shape), document, name);
                    if (!p) {
                        break;
                    }
                    if (builder.type == GeometryBuilder::PrimitiveType::Circle) {
                        p->Angle1.setValue(0.0);
                        p->Angle2.setValue(360.0);
                    }
                    newObject = p;
                    break;
                }
                case GeometryBuilder::PrimitiveType::Ellipse: {
                    newObject =
                        createEllipsePrimitive(TopoDS::Edge(builder.shape), document, "Ellipse");
                    break;
                }
                case GeometryBuilder::PrimitiveType::Spline: {
                    // Splines are generic Part::Feature as no Part primitive exists
                    auto* p = document->addObject<Part::Feature>("Spline");
                    p->Shape.setValue(builder.shape);
                    newObject = p;
                    break;
                }
                case GeometryBuilder::PrimitiveType::PolylineFlattened: {
                    // This creates a simple Part::Feature wrapping the wire, which is standard for
                    // block children.
                    newObject =
                        createFlattenedPolylineFeature(TopoDS::Wire(builder.shape), "Polyline");
                    break;
                }
                case GeometryBuilder::PrimitiveType::PolylineParametric: {
                    // This creates a Part::Compound containing line/arc segments.
                    newObject =
                        createParametricPolylineCompound(TopoDS::Wire(builder.shape), "Polyline");
                    // No styling needed here, as the block's instance will control appearance.
                    break;
                }
                case GeometryBuilder::PrimitiveType::None:  // Default/fallback if not handled
                default: {
                    // Generic shape, e.g., 3DFACE
                    newObject = createGenericShapeFeature(builder.shape, document, "Shape");
                    break;
                }
            }

            if (newObject) {
                IncrementCreatedObjectCount();
                newObject->Visibility.setValue(false);  // Children of blocks are hidden by default
                // Layer and color are applied by the block itself (Part::Compound) or its children
                // if overridden.
                ApplyGuiStyles(
                    static_cast<Part::Feature*>(newObject));  // Apply style to the child object
                childObjects.push_back(newObject);  // Add to the block's main children list
            }
        }
    }

    // 6. Finalize the Part::Compound.
    if (!childObjects.empty()) {
        blockCompound->Links.setValues(childObjects);
    }

    // 7. Mark this block as composed.
    composed.insert(blockName);
}

void ImpExpDxfRead::ComposeBlocks()
{
    std::set<std::string> composedBlocks;

    if (m_importMode == ImportMode::FusedShapes) {
        // User wants flattened geometry for performance.
        for (const auto& pair : this->Blocks) {
            if (composedBlocks.find(pair.first) == composedBlocks.end()) {
                ComposeFlattenedBlock(pair.first, composedBlocks);
            }
        }
    }
    else {
        // User wants a parametric, editable structure.
        for (const auto& pair : this->Blocks) {
            if (composedBlocks.find(pair.first) == composedBlocks.end()) {
                ComposeParametricBlock(pair.first, composedBlocks);
            }
        }
    }
}

void ImpExpDxfRead::FinishImport()
{
    // This function runs after all blocks have been parsed and composed.
    // It sorts all created block definitions into two groups: those that are
    // actively referenced in the drawing, and those that are not.

    std::vector<App::DocumentObject*> referenced;
    std::vector<App::DocumentObject*> unreferenced;

    for (const auto& pair : m_blockDefinitions) {
        const std::string& blockName = pair.first;
        App::DocumentObject* blockObj = pair.second;

        bool is_referenced = (m_referencedBlocks.find(blockName) != m_referencedBlocks.end());

        // A block is considered "referenced" if it was explicitly inserted
        // or if it is an anonymous system block (e.g., for dimensions).
        // All other named blocks are considered unreferenced if not found in the set.
        if (is_referenced || (blockName.rfind('*', 0) == 0)) {
            referenced.push_back(blockObj);
        }
        else {
            unreferenced.push_back(blockObj);
        }
    }

    // Re-assign the group contents by setting the PropertyLinkList for each group.
    // This correctly re-parents the objects in the document's dependency graph.
    m_blockDefinitionGroup->Group.setValues(referenced);
    m_unreferencedBlocksGroup->Group.setValues(unreferenced);

    // Final cleanup: If the unreferenced group is empty, remove it to avoid
    // unnecessary clutter in the document tree. Otherwise, ensure it's hidden.
    if (unreferenced.empty()) {
        try {
            document->removeObject(m_unreferencedBlocksGroup->getNameInDocument());
        }
        catch (const Base::Exception& e) {
            // It's not critical if removal fails, but we should log it.
            e.reportException();
        }
    }
    else {
        m_unreferencedBlocksGroup->Visibility.setValue(false);
    }

    // If no blocks were defined in the file at all, remove the main definitions
    // group as well to keep the document clean.
    if (m_blockDefinitionGroup && m_blockDefinitionGroup->Group.getValues().empty()) {
        try {
            document->removeObject(m_blockDefinitionGroup->getNameInDocument());
        }
        catch (const Base::Exception& e) {
            e.reportException();
        }
    }

    // call the base class implementation if it has one
    CDxfRead::FinishImport();
}

bool ImpExpDxfRead::OnReadBlock(const std::string& name, int flags)
{
    // Step 1: Check for external references first. This is a critical check.
    if ((flags & 0x04) != 0) {  // Block is an Xref
        UnsupportedFeature("External (xref) BLOCK");
        return SkipBlockContents();
    }

    // Step 2: Check if the block is anonymous/system.
    bool isAnonymous = (name.find('*') == 0);
    if (isAnonymous) {
        if (name.size() > 1) {
            char type = std::toupper(name[1]);
            if (type == 'D') {
                m_stats.systemBlockCounts["Dimension-related (*D)"]++;
            }
            else if (type == 'H' || type == 'X') {
                m_stats.systemBlockCounts["Hatch-related (*H, *X)"]++;
            }
            else {
                m_stats.systemBlockCounts["Other System Blocks"]++;
            }
        }
        else {
            m_stats.systemBlockCounts["Other System Blocks"]++;
        }

        if (!m_importHiddenBlocks) {
            return SkipBlockContents();
        }
    }
    else {
        m_stats.entityCounts["BLOCK"]++;
    }

    // Step 3: Check for duplicates to prevent errors.
    if (this->Blocks.count(name)) {
        ImportError("Duplicate block name '%s' found. Ignoring subsequent definition.",
                    name.c_str());
        return SkipBlockContents();
    }

    // Step 4: Use the temporary Block struct and Collector to parse all contents into memory.
    // The .emplace method is slightly more efficient here.
    auto& temporaryBlock = Blocks.emplace(std::make_pair(name, Block(name, flags))).first->second;
    BlockDefinitionCollector blockCollector(*this,
                                            temporaryBlock.GeometryBuilders,
                                            temporaryBlock.Inserts);
    if (!ReadBlockContents()) {
        return false;  // Abort on parsing error
    }

    // That's it. The block is now parsed into this->Blocks.
    // Composition will happen later in ComposeBlocks().
    return true;
}

void ImpExpDxfRead::OnReadLine(const Base::Vector3d& start,
                               const Base::Vector3d& end,
                               bool /*hidden*/)
{
    if (shouldSkipEntity()) {
        return;
    }

    gp_Pnt p0 = makePoint(start);
    gp_Pnt p1 = makePoint(end);
    if (p0.IsEqual(p1, 1e-8)) {
        return;
    }
    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(p0, p1).Edge();
    GeometryBuilder builder(edge);

    // CORRECTED: Set PrimitiveType conditionally based on m_importMode
    switch (m_importMode) {
        case ImportMode::EditableDraft:
        case ImportMode::EditablePrimitives:
            // For these modes, we want a specific Part primitive (Part::Line)
            builder.type = GeometryBuilder::PrimitiveType::Line;
            break;
        case ImportMode::IndividualShapes:
        case ImportMode::FusedShapes:
            // For these modes, we want a generic Part::Feature wrapping the TopoDS_Shape.
            // PrimitiveType::None will lead to a generic Part::Feature in AddGeometry.
            builder.type = GeometryBuilder::PrimitiveType::None;
            break;
    }

    Collector->AddGeometry(builder);
}


void ImpExpDxfRead::OnReadPoint(const Base::Vector3d& start)
{
    if (shouldSkipEntity()) {
        return;
    }
    TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(makePoint(start)).Vertex();
    GeometryBuilder builder(vertex);

    switch (m_importMode) {
        case ImportMode::EditableDraft:
        case ImportMode::EditablePrimitives:
            builder.type = GeometryBuilder::PrimitiveType::Point;
            break;
        case ImportMode::IndividualShapes:
        case ImportMode::FusedShapes:
            builder.type = GeometryBuilder::PrimitiveType::None;  // Generic Part::Feature
            break;
    }
    Collector->AddGeometry(builder);
}


void ImpExpDxfRead::OnReadArc(const Base::Vector3d& start,
                              const Base::Vector3d& end,
                              const Base::Vector3d& center,
                              bool dir,
                              bool /*hidden*/)
{
    if (shouldSkipEntity()) {
        return;
    }

    gp_Pnt p0 = makePoint(start);
    gp_Pnt p1 = makePoint(end);
    gp_Dir up(0, 0, 1);
    if (!dir) {
        up.Reverse();
    }
    gp_Pnt pc = makePoint(center);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    if (circle.Radius() < 1e-9) {
        Base::Console().warning("ImpExpDxf - ignore degenerate arc of circle\n");
        return;
    }

    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(circle, p0, p1).Edge();
    GeometryBuilder builder(edge);  // Instantiate builder once

    switch (m_importMode) {
        case ImportMode::EditableDraft:
        case ImportMode::EditablePrimitives:
            builder.type = GeometryBuilder::PrimitiveType::Arc;
            break;
        case ImportMode::IndividualShapes:
        case ImportMode::FusedShapes:
            builder.type = GeometryBuilder::PrimitiveType::None;  // Generic Part::Feature
            break;
    }
    Collector->AddGeometry(builder);
}


void ImpExpDxfRead::OnReadCircle(const Base::Vector3d& start,
                                 const Base::Vector3d& center,
                                 bool dir,
                                 bool /*hidden*/)
{
    if (shouldSkipEntity()) {
        return;
    }

    gp_Pnt p0 = makePoint(start);
    gp_Dir up(0, 0, 1);
    if (!dir) {
        up.Reverse();
    }
    gp_Pnt pc = makePoint(center);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    if (circle.Radius() < 1e-9) {
        Base::Console().warning("ImpExpDxf - ignore degenerate circle\n");
        return;
    }

    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(circle).Edge();
    GeometryBuilder builder(edge);  // Instantiate builder once

    switch (m_importMode) {
        case ImportMode::EditableDraft:
        case ImportMode::EditablePrimitives:
            builder.type = GeometryBuilder::PrimitiveType::Circle;
            break;
        case ImportMode::IndividualShapes:
        case ImportMode::FusedShapes:
            builder.type = GeometryBuilder::PrimitiveType::None;  // Generic Part::Feature
            break;
    }
    Collector->AddGeometry(builder);
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

    if (shouldSkipEntity()) {
        return;
    }

    try {
        Handle(Geom_BSplineCurve) geom;
        if (sd.control_points > 0) {
            geom = getSplineFromPolesAndKnots(sd);
        }
        else if (sd.fit_points > 0) {
            geom = getInterpolationSpline(sd);
        }

        if (!geom.IsNull()) {
            TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(geom).Edge();
            GeometryBuilder builder(edge);  // Instantiate builder once

            switch (m_importMode) {
                case ImportMode::EditableDraft:
                case ImportMode::EditablePrimitives:
                    builder.type = GeometryBuilder::PrimitiveType::Spline;
                    break;
                case ImportMode::IndividualShapes:
                case ImportMode::FusedShapes:
                    builder.type = GeometryBuilder::PrimitiveType::None;  // Generic Part::Feature
                    break;
            }
            Collector->AddGeometry(builder);
        }
    }
    catch (const Standard_Failure&) {
        Base::Console().warning("ImpExpDxf - failed to create bspline\n");
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
    if (shouldSkipEntity()) {
        return;
    }

    gp_Dir up(0, 0, 1);
    if (!dir) {
        up.Reverse();
    }
    gp_Pnt pc = makePoint(center);
    gp_Elips ellipse(gp_Ax2(pc, up), major_radius, minor_radius);
    ellipse.Rotate(gp_Ax1(pc, up), rotation);
    if (ellipse.MinorRadius() < 1e-9) {
        Base::Console().warning("ImpExpDxf - ignore degenerate ellipse\n");
        return;
    }

    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(ellipse).Edge();
    GeometryBuilder builder(edge);  // Pass the shape to the builder

    switch (m_importMode) {
        case ImportMode::EditableDraft:
        case ImportMode::EditablePrimitives:
            // Tag this geometry so the collector knows to create a Part::Ellipse primitive
            builder.type = GeometryBuilder::PrimitiveType::Ellipse;
            break;
        case ImportMode::IndividualShapes:
        case ImportMode::FusedShapes:
        default:
            // For other modes, create a generic shape (Part:Feature), which is the existing
            // behavior.
            builder.type = GeometryBuilder::PrimitiveType::None;
            break;
    }
    Collector->AddGeometry(builder);
}

void ImpExpDxfRead::OnReadText(const Base::Vector3d& point,
                               const double height,
                               const std::string& text,
                               const double rotation)
{
    if (shouldSkipEntity() || !m_importAnnotations) {
        return;
    }

    auto* p = static_cast<App::FeaturePython*>(document->addObject("App::FeaturePython", "Text"));
    if (p) {
        p->addDynamicProperty("App::PropertyString",
                              "DxfEntityType",
                              "Internal",
                              "DXF entity type");
        static_cast<App::PropertyString*>(p->getPropertyByName("DxfEntityType"))->setValue("TEXT");

        p->addDynamicProperty("App::PropertyStringList", "Text", "Data", "Text content");
        // Explicitly create the vector to resolve ambiguity
        std::vector<std::string> text_values = {text};
        static_cast<App::PropertyStringList*>(p->getPropertyByName("Text"))->setValues(text_values);

        p->addDynamicProperty("App::PropertyFloat",
                              "DxfTextHeight",
                              "Internal",
                              "Original text height");
        static_cast<App::PropertyFloat*>(p->getPropertyByName("DxfTextHeight"))->setValue(height);

        p->addDynamicProperty("App::PropertyPlacement", "Placement", "Base", "Object placement");
        Base::Placement pl;
        pl.setPosition(point);
        pl.setRotation(Base::Rotation(Base::Vector3d(0, 0, 1), Base::toRadians(rotation)));
        static_cast<App::PropertyPlacement*>(p->getPropertyByName("Placement"))->setValue(pl);

        Collector->AddObject(p, "Text");
    }
}


void ImpExpDxfRead::OnReadInsert(const Base::Vector3d& point,
                                 const Base::Vector3d& scale,
                                 const std::string& name,
                                 double rotation)
{
    if (shouldSkipEntity()) {
        return;
    }

    // Delegate the action to the currently active collector.
    // If the BlockDefinitionCollector is active, it will just store the data.
    // If the DrawingEntityCollector is active, it will create the App::Link.
    Collector->AddInsert(point, scale, name, rotation);
}


void ImpExpDxfRead::OnReadDimension(const Base::Vector3d& start,
                                    const Base::Vector3d& end,
                                    const Base::Vector3d& point,
                                    int dimensionType,
                                    double rotation)
{
    if (shouldSkipEntity() || !m_importAnnotations) {
        return;
    }

    auto* p =
        static_cast<App::FeaturePython*>(document->addObject("App::FeaturePython", "Dimension"));
    if (p) {
        p->addDynamicProperty("App::PropertyString",
                              "DxfEntityType",
                              "Internal",
                              "DXF entity type");
        static_cast<App::PropertyString*>(p->getPropertyByName("DxfEntityType"))
            ->setValue("DIMENSION");

        p->addDynamicProperty("App::PropertyVector", "Start", "Data", "Start point of dimension");
        static_cast<App::PropertyVector*>(p->getPropertyByName("Start"))->setValue(start);

        p->addDynamicProperty("App::PropertyVector", "End", "Data", "End point of dimension");
        static_cast<App::PropertyVector*>(p->getPropertyByName("End"))->setValue(end);

        p->addDynamicProperty("App::PropertyVector", "Dimline", "Data", "Point on dimension line");
        static_cast<App::PropertyVector*>(p->getPropertyByName("Dimline"))->setValue(point);

        p->addDynamicProperty("App::PropertyInteger",
                              "DxfDimensionType",
                              "Internal",
                              "Original dimension type flag");
        static_cast<App::PropertyInteger*>(p->getPropertyByName("DxfDimensionType"))
            ->setValue(dimensionType);

        p->addDynamicProperty("App::PropertyAngle",
                              "DxfRotation",
                              "Internal",
                              "Original dimension rotation");
        // rotation is already in radians from the caller
        static_cast<App::PropertyAngle*>(p->getPropertyByName("DxfRotation"))->setValue(rotation);

        p->addDynamicProperty("App::PropertyPlacement", "Placement", "Base", "Object placement");
        Base::Placement pl;
        // Correctly construct the rotation directly from the 4x4 matrix.
        // The Base::Rotation constructor will extract the rotational part.
        pl.setRotation(Base::Rotation(OCSOrientationTransform));
        static_cast<App::PropertyPlacement*>(p->getPropertyByName("Placement"))->setValue(pl);

        Collector->AddObject(p, "Dimension");
    }
}

void ImpExpDxfRead::OnReadPolyline(std::list<VertexInfo>& vertices, int flags)
{
    if (shouldSkipEntity()) {
        return;
    }

    if (vertices.size() < 2 && (flags & 1) == 0) {
        return;  // Not enough vertices for an open polyline
    }

    TopoDS_Wire wire = BuildWireFromPolyline(vertices, flags);
    if (wire.IsNull()) {
        return;
    }

    if (m_importMode == ImportMode::EditableDraft) {
        GeometryBuilder builder(wire);
        builder.type = GeometryBuilder::PrimitiveType::PolylineFlattened;
        Collector->AddGeometry(builder);
    }
    else if (m_importMode == ImportMode::EditablePrimitives) {
        GeometryBuilder builder(wire);
        builder.type = GeometryBuilder::PrimitiveType::PolylineParametric;
        Collector->AddGeometry(builder);
    }
    else {
        Collector->AddObject(wire, "Polyline");
    }
}

void ImpExpDxfRead::DrawingEntityCollector::AddGeometry(const GeometryBuilder& builder)
{
    App::DocumentObject* newDocObj = nullptr;

    switch (builder.type) {
        case GeometryBuilder::PrimitiveType::Line: {
            newDocObj = createLinePrimitive(TopoDS::Edge(builder.shape), Reader.document, "Line");
            break;
        }
        case GeometryBuilder::PrimitiveType::Circle: {
            auto* p = createCirclePrimitive(TopoDS::Edge(builder.shape), Reader.document, "Circle");
            if (p) {
                p->Angle1.setValue(0.0);
                p->Angle2.setValue(360.0);  // Ensure it's a full circle if it's a circle entity
            }
            newDocObj = p;
            break;
        }
        case GeometryBuilder::PrimitiveType::Arc: {
            newDocObj = createCirclePrimitive(TopoDS::Edge(builder.shape), Reader.document, "Arc");
            break;
        }
        case GeometryBuilder::PrimitiveType::Point: {
            newDocObj =
                createVertexPrimitive(TopoDS::Vertex(builder.shape), Reader.document, "Point");
            break;
        }
        case GeometryBuilder::PrimitiveType::Ellipse: {
            newDocObj =
                createEllipsePrimitive(TopoDS::Edge(builder.shape), Reader.document, "Ellipse");
            break;
        }
        case GeometryBuilder::PrimitiveType::Spline: {
            newDocObj = createGenericShapeFeature(builder.shape, Reader.document, "Spline");
            break;
        }
        case GeometryBuilder::PrimitiveType::PolylineFlattened: {
            Reader.CreateFlattenedPolyline(TopoDS::Wire(builder.shape), "Polyline");
            newDocObj = nullptr;  // Object handled by helper
            break;
        }
        case GeometryBuilder::PrimitiveType::PolylineParametric: {
            Reader.CreateParametricPolyline(TopoDS::Wire(builder.shape), "Polyline");
            newDocObj = nullptr;  // Object handled by helper
            break;
        }
        case GeometryBuilder::PrimitiveType::None:  // Fallback for generic shapes (e.g., 3DFACE)
        default: {
            newDocObj = createGenericShapeFeature(builder.shape, Reader.document, "Shape");
            break;
        }
    }

    // Common post-creation steps for objects NOT handled by helper functions
    if (newDocObj) {
        Reader.IncrementCreatedObjectCount();
        Reader._addOriginalLayerProperty(newDocObj);
        Reader.MoveToLayer(newDocObj);
        Reader.ApplyGuiStyles(static_cast<Part::Feature*>(newDocObj));
    }
}

ImpExpDxfRead::Layer::Layer(const std::string& name,
                            ColorIndex_t color,
                            std::string&& lineType,
                            PyObject* drawingLayer)
    : CDxfRead::Layer(name, color, std::move(lineType))
    , DraftLayerView(drawingLayer == nullptr ? Py_None
                                             : PyObject_GetAttrString(drawingLayer, "ViewObject"))
    , GroupContents(drawingLayer == nullptr
                        ? nullptr
                        : dynamic_cast<App::PropertyLinkListHidden*>(
                              (((App::FeaturePythonPyT<App::DocumentObjectPy>*)drawingLayer)
                                   ->getPropertyContainerPtr())
                                  ->getDynamicPropertyByName("Group")))
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
    if (DraftLayerView != Py_None && Hidden) {
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
        Base::Color appColor = ObjectColor(color);
        PyObject* draftModule = nullptr;
        PyObject* layer = nullptr;
        draftModule = getDraftModule();
        if (draftModule != nullptr) {
            // After the colours, I also want to pass the draw_style, but there is an
            // intervening line-width parameter. It is easier to just pass that parameter's
            // default value than to do the handstands to pass a named parameter.
            // TODO: Pass the appropriate draw_style (from "Solid" "Dashed" "Dotted" "DashDot")
            // This needs an ObjectDrawStyleName analogous to ObjectColor but at the
            // ImpExpDxfGui level.
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
        if (result->DraftLayerView != Py_None) {
            // Get the correct boolean value based on the user's preference.
            PyObject* overrideValue = m_preserveColors ? Py_True : Py_False;
            PyObject_SetAttrString(result->DraftLayerView,
                                   "OverrideLineColorChildren",
                                   overrideValue);
            PyObject_SetAttrString(result->DraftLayerView,
                                   "OverrideShapeAppearanceChildren",
                                   overrideValue);
        }

        // We make our own layer class even if we could not make a layer. MoveToLayer will
        // ignore such layers but we have to do this because it is not a polymorphic type so we
        // can't tell what we pull out of m_entityAttributes.m_Layer.
        return result;
    }
    return CDxfRead::MakeLayer(name, color, std::move(lineType));
}
void ImpExpDxfRead::MoveToLayer(App::DocumentObject* object) const
{
    if (m_preserveLayers) {
        static_cast<Layer*>(m_entityAttributes.m_Layer)->Contents.push_back(object);
    }
    // TODO: else Hide the object if it is in a Hidden layer? That won't work because we've
    // cleared out m_entityAttributes.m_Layer
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

void ImpExpDxfRead::_addOriginalLayerProperty(App::DocumentObject* obj)
{
    if (obj && m_entityAttributes.m_Layer) {
        obj->addDynamicProperty("App::PropertyString",
                                "OriginalLayer",
                                "Internal",
                                "Layer name from the original DXF file.",
                                App::Property::Hidden);
        static_cast<App::PropertyString*>(obj->getPropertyByName("OriginalLayer"))
            ->setValue(m_entityAttributes.m_Layer->Name.c_str());
    }
}

void ImpExpDxfRead::DrawingEntityCollector::AddObject(const TopoDS_Shape& shape,
                                                      const char* nameBase)
{
    auto pcFeature = Reader.document->addObject<Part::Feature>(nameBase);

    if (pcFeature) {
        Reader.IncrementCreatedObjectCount();
        pcFeature->Shape.setValue(shape);
        Reader._addOriginalLayerProperty(pcFeature);
        Reader.MoveToLayer(pcFeature);
        Reader.ApplyGuiStyles(pcFeature);
    }
}

void ImpExpDxfRead::DrawingEntityCollector::AddObject(App::DocumentObject* obj,
                                                      const char* /*nameBase*/)
{
    Reader.MoveToLayer(obj);
    Reader._addOriginalLayerProperty(obj);

    // Safely apply styles by checking the object's actual type (only for objects not replaced
    // by Python)
    if (auto feature = dynamic_cast<Part::Feature*>(obj)) {
        Reader.ApplyGuiStyles(feature);
    }
    else if (auto pyFeature = dynamic_cast<App::FeaturePython*>(obj)) {
        Reader.ApplyGuiStyles(pyFeature);
    }
    else if (auto link = dynamic_cast<App::Link*>(obj)) {
        Reader.ApplyGuiStyles(link);
    }
}

void ImpExpDxfRead::DrawingEntityCollector::AddObject(FeaturePythonBuilder shapeBuilder)
{
    Reader.IncrementCreatedObjectCount();
    App::FeaturePython* shape = shapeBuilder(Reader.OCSOrientationTransform);
    if (shape != nullptr) {
        Reader._addOriginalLayerProperty(shape);
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
            Base::Console().warning("ImpExpDxf - unknown curve type: %d\n",
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

    // 2*pi = 6.28319 is invalid(doesn't display in LibreCAD), but 2PI = 6.28318 is valid!
    // writeEllipse(center, major, minor, rotation, 0.0, 2 * std::numbers::pi, true );
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

    double startAngle = fmod(f, 2.0 * std::numbers::pi);  // revolutions
    double endAngle = fmod(l, 2.0 * std::numbers::pi);
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
            Base::Console().message("DxfWrite::exportBSpline - result not within tolerance\n");
        }
        else {
            f = c.FirstParameter();
            l = c.LastParameter();
            s = c.Value(f);
            ePt = c.Value(l);
            Base::Console().message(
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
    Base::Console().message("BCurve dxf export not yet supported\n");
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

Py::Object ImpExpDxfRead::getStatsAsPyObject()
{
    // Create a Python dictionary to hold all import statistics.
    Py::Dict statsDict;

    // Populate the dictionary with general information about the import.
    statsDict.setItem("dxfVersion", Py::String(m_stats.dxfVersion));
    statsDict.setItem("dxfEncoding", Py::String(m_stats.dxfEncoding));
    statsDict.setItem("scalingSource", Py::String(m_stats.scalingSource));
    statsDict.setItem("fileUnits", Py::String(m_stats.fileUnits));
    statsDict.setItem("finalScalingFactor", Py::Float(m_stats.finalScalingFactor));
    statsDict.setItem("importTimeSeconds", Py::Float(m_stats.importTimeSeconds));
    statsDict.setItem("totalEntitiesCreated", Py::Long(m_stats.totalEntitiesCreated));

    // Create a nested dictionary for the counts of each DXF entity type read.
    Py::Dict entityCountsDict;
    for (const auto& pair : m_stats.entityCounts) {
        entityCountsDict.setItem(pair.first.c_str(), Py::Long(pair.second));
    }
    statsDict.setItem("entityCounts", entityCountsDict);

    // Create a nested dictionary for the import settings used for this session.
    Py::Dict importSettingsDict;
    for (const auto& pair : m_stats.importSettings) {
        importSettingsDict.setItem(pair.first.c_str(), Py::String(pair.second));
    }
    statsDict.setItem("importSettings", importSettingsDict);

    // Create a nested dictionary for any unsupported DXF features encountered.
    Py::Dict unsupportedFeaturesDict;
    for (const auto& pair : m_stats.unsupportedFeatures) {
        Py::List occurrencesList;
        for (const auto& occurrence : pair.second) {
            Py::Tuple infoTuple(2);
            infoTuple.setItem(0, Py::Long(occurrence.first));
            infoTuple.setItem(1, Py::String(occurrence.second));
            occurrencesList.append(infoTuple);
        }
        unsupportedFeaturesDict.setItem(pair.first.c_str(), occurrencesList);
    }
    statsDict.setItem("unsupportedFeatures", unsupportedFeaturesDict);

    // Create a nested dictionary for the counts of system blocks encountered.
    Py::Dict systemBlockCountsDict;
    for (const auto& pair : m_stats.systemBlockCounts) {
        systemBlockCountsDict.setItem(pair.first.c_str(), Py::Long(pair.second));
    }
    statsDict.setItem("systemBlockCounts", systemBlockCountsDict);

    // Return the fully populated statistics dictionary to the Python caller.
    return statsDict;
}
