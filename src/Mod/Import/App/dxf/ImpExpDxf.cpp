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
#include <App/DocumentObjectGroup.h>
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
#include <Mod/Part/App/FeatureCompound.h>
#include <App/Link.h>
#include <Base/Tools.h>

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

    // Default for creation type is to create draft objects.
    // The radio-button structure of the options dialog should generally prevent this condition.
    m_mergeOption = DraftObjects;
    m_stats.importSettings["Merge option"] = "Create Draft objects";  // Default
    if (hGrp->GetBool("groupLayers", true)) {
        // Group all compatible objects together
        m_mergeOption = MergeShapes;
        m_stats.importSettings["Merge option"] = "Group layers into blocks";
    }
    else if (hGrp->GetBool("dxfCreatePart", true)) {
        // Create (non-draft) Shape objects when possible
        m_mergeOption = SingleShapes;
        m_stats.importSettings["Merge option"] = "Create Part shapes";
    }
    else if (hGrp->GetBool("dxfCreateDraft", true)) {
        // Create only Draft objects, making the result closest to drawn-from-scratch
        m_mergeOption = DraftObjects;
        m_stats.importSettings["Merge option"] = "Create Draft objects";
    }
    // TODO: joingeometry should give an intermediate between MergeShapes and SingleShapes which
    // will merge shapes that happen to join end-to-end. As such it should be in the radio button
    // set, except that the legacy importer can do joining either for sketches or for shapes. What
    // this really means is there should be an "Import as sketch" checkbox, and only the
    // MergeShapes, JoinShapes, and SingleShapes radio buttons should be allowed, i.e. Draft Objects
    // would be ignored.
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
    for (const auto& [attributes, shapeList] : blockData.Shapes) {
        shapeCollection.insert(shapeCollection.end(), shapeList.begin(), shapeList.end());
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
    std::string compName = "BLOCK_";
    compName += blockName;
    auto blockCompound = document->addObject<Part::Compound>(
        document->getUniqueObjectName(compName.c_str()).c_str());
    m_blockDefinitionGroup->addObject(blockCompound);
    IncrementCreatedObjectCount();
    blockCompound->Visibility.setValue(false);
    this->m_blockDefinitions[blockName] = blockCompound;

    std::vector<App::DocumentObject*> linkedObjects;

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
                linkedObjects.push_back(link);
            }
        }
    }

    // 5. Create and Link Primitive Geometry.
    // Iterate through each attribute group (e.g., each layer within the block).
    for (const auto& [attributes, shapeList] : blockData.Shapes) {
        // Then, iterate through each shape in that group and create a separate feature for it.
        for (const auto& shape : shapeList) {
            if (!shape.IsNull()) {
                std::string cleanBlockLabel = blockName;
                if (!cleanBlockLabel.empty() && std::isdigit(cleanBlockLabel[0])) {
                    // Workaround for FreeCAD's unique name generator, which prepends an underscore
                    // to names that start with a digit. We add our own prefix.
                    cleanBlockLabel.insert(0, "_");
                }
                else if (!cleanBlockLabel.empty() && std::isdigit(cleanBlockLabel.back())) {
                    // Add a trailing underscore to prevent the unique name generator
                    // from incrementing the number in the block's name.
                    cleanBlockLabel += "_";
                }
                // Determine a more descriptive name for the primitive feature.
                std::string type_suffix = "Shape";
                if (shape.ShapeType() == TopAbs_EDGE) {
                    BRepAdaptor_Curve adaptor(TopoDS::Edge(shape));
                    switch (adaptor.GetType()) {
                        case GeomAbs_Line:
                            type_suffix = "Line";
                            break;
                        case GeomAbs_Circle:
                            type_suffix = "Circle";
                            break;
                        case GeomAbs_Ellipse:
                            type_suffix = "Ellipse";
                            break;
                        case GeomAbs_BSplineCurve:
                            type_suffix = "BSpline";
                            break;
                        case GeomAbs_BezierCurve:
                            type_suffix = "Bezier";
                            break;
                        default:
                            type_suffix = "Edge";
                            break;
                    }
                }
                else if (shape.ShapeType() == TopAbs_VERTEX) {
                    type_suffix = "Vertex";
                }
                else if (shape.ShapeType() == TopAbs_WIRE) {
                    type_suffix = "Wire";
                }
                else if (shape.ShapeType() == TopAbs_FACE) {
                    type_suffix = "Face";
                }
                else if (shape.ShapeType() == TopAbs_SHELL) {
                    type_suffix = "Shell";
                }
                else if (shape.ShapeType() == TopAbs_SOLID) {
                    type_suffix = "Solid";
                }
                else if (shape.ShapeType() == TopAbs_COMPOUND) {
                    type_suffix = "Compound";
                }

                std::string primitive_base_label = cleanBlockLabel + "_" + type_suffix;
                // Use getStandardObjectLabel to get a unique user-facing label (e.g.,
                // "block01_Line001") while keeping the internal object name clean.
                auto geomFeature = document->addObject<Part::Feature>(
                    document->getStandardObjectLabel(primitive_base_label.c_str(), 3).c_str());

                IncrementCreatedObjectCount();
                geomFeature->Shape.setValue(shape);
                geomFeature->Visibility.setValue(false);

                // Apply styling to this primitive feature using its original attributes.
                this->m_entityAttributes = attributes;
                this->ApplyGuiStyles(geomFeature);

                linkedObjects.push_back(geomFeature);
            }
        }
    }

    // TODO: Add similar logic for blockData.FeatureBuildersList if needed.

    // 6. Finalize the Part::Compound.
    if (!linkedObjects.empty()) {
        blockCompound->Links.setValues(linkedObjects);
    }

    // 7. Mark this block as composed.
    composed.insert(blockName);
}

void ImpExpDxfRead::ComposeBlocks()
{
    std::set<std::string> composedBlocks;

    if (m_mergeOption == MergeShapes) {
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
                                            temporaryBlock.Shapes,
                                            temporaryBlock.FeatureBuildersList,
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
    if (p0.IsEqual(p1, 0.00000001)) {
        // TODO: Really?? What about the people designing integrated circuits?
        return;
    }
    Collector->AddObject(BRepBuilderAPI_MakeEdge(p0, p1).Edge(), "Line");
}


void ImpExpDxfRead::OnReadPoint(const Base::Vector3d& start)
{
    if (shouldSkipEntity()) {
        return;
    }

    Collector->AddObject(BRepBuilderAPI_MakeVertex(makePoint(start)).Vertex(), "Point");
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
        up = -up;
    }
    gp_Pnt pc = makePoint(center);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    if (circle.Radius() > 0) {
        Collector->AddObject(BRepBuilderAPI_MakeEdge(circle, p0, p1).Edge(), "Arc");
    }
    else {
        Base::Console().warning("ImpExpDxf - ignore degenerate arc of circle\n");
    }
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
        up = -up;
    }
    gp_Pnt pc = makePoint(center);
    gp_Circ circle(gp_Ax2(pc, up), p0.Distance(pc));
    if (circle.Radius() > 0) {
        Collector->AddObject(BRepBuilderAPI_MakeEdge(circle).Edge(), "Circle");
    }
    else {
        Base::Console().warning("ImpExpDxf - ignore degenerate circle\n");
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

        if (geom.IsNull()) {
            throw Standard_Failure();
        }

        Collector->AddObject(BRepBuilderAPI_MakeEdge(geom).Edge(), "Spline");
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
        up = -up;
    }
    gp_Pnt pc = makePoint(center);
    gp_Elips ellipse(gp_Ax2(pc, up), major_radius, minor_radius);
    ellipse.Rotate(gp_Ax1(pc, up), rotation);
    if (ellipse.MinorRadius() > 0) {
        Collector->AddObject(BRepBuilderAPI_MakeEdge(ellipse).Edge(), "Ellipse");
    }
    else {
        Base::Console().warning("ImpExpDxf - ignore degenerate ellipse\n");
    }
}


void ImpExpDxfRead::OnReadText(const Base::Vector3d& point,
                               const double height,
                               const std::string& text,
                               const double rotation)
{
    if (shouldSkipEntity()) {
        return;
    }

    // Note that our parameters do not contain all the information needed to properly orient the
    // text. As a result the text will always appear on the XY plane
    if (m_importAnnotations) {
        auto makeText = [this, rotation, point, text, height](
                            const Base::Matrix4D& transform) -> App::FeaturePython* {
            PyObject* draftModule = getDraftModule();
            if (draftModule != nullptr) {
                Base::Matrix4D localTransform;
                localTransform.rotZ(Base::toRadians(rotation));
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
                                    double /*rotation*/)
{
    if (shouldSkipEntity()) {
        return;
    }

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
    if (shouldSkipEntity()) {
        return;
    }

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
        if (result->DraftLayerView != Py_None) {
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
    Reader.IncrementCreatedObjectCount();
    auto pcFeature = Reader.document->addObject<Part::Feature>(nameBase);
    pcFeature->Shape.setValue(shape);
    Reader.MoveToLayer(pcFeature);
    Reader.ApplyGuiStyles(pcFeature);
}

void ImpExpDxfRead::DrawingEntityCollector::AddObject(App::DocumentObject* obj,
                                                      const char* /*nameBase*/)
{
    // This overload is for C++ created objects like App::Link
    // The object is already in the document, so we just need to style it and move it to a layer.
    Reader.MoveToLayer(obj);

    // Safely apply styles by checking the object's actual type
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
