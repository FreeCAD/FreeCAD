// SPDX-License-Identifier: LGPL-2.1-or-later

#include <regex>
#include "PartTestHelpers.h"

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

namespace PartTestHelpers
{

double getVolume(const TopoDS_Shape& shape)
{
    GProp_GProps prop;
    BRepGProp::VolumeProperties(shape, prop);
    return abs(prop.Mass());
}

double getArea(const TopoDS_Shape& shape)
{
    GProp_GProps prop;
    BRepGProp::SurfaceProperties(shape, prop);
    return abs(prop.Mass());
}

double getLength(const TopoDS_Shape& shape)
{
    GProp_GProps prop;
    BRepGProp::LinearProperties(shape, prop);
    return abs(prop.Mass());
}


void PartTestHelperClass::createTestDoc()
{
    _docName = App::GetApplication().getUniqueDocumentName("test");
    _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
    std::array<Base::Vector3d, 6> box_origins = {
        Base::Vector3d(),                                        // First box at 0,0,0
        Base::Vector3d(0, 1, 0),                                 // Overlap with first box
        Base::Vector3d(0, 3, 0),                                 // Don't Overlap with first box
        Base::Vector3d(0, 2, 0),                                 // Touch the first box
        Base::Vector3d(0, 2 + Base::Precision::Confusion(), 0),  // Just Outside of touching
        // For the Just Inside Of Touching case, go enough that we exceed precision rounding
        Base::Vector3d(0, 2 - minimalDistance, 0)};

    for (unsigned i = 0; i < _boxes.size(); i++) {
        auto box = _boxes[i] = _doc->addObject<Part::Box>();  // NOLINT
        box->Length.setValue(1);
        box->Width.setValue(2);
        box->Height.setValue(3);
        box->Placement.setValue(
            Base::Placement(box_origins[i], Base::Rotation(), Base::Vector3d()));  // NOLINT
    }
}

std::vector<Part::FilletElement>
_getFilletEdges(const std::vector<int>& edges, double startRadius, double endRadius)
{
    std::vector<Part::FilletElement> filletElements;
    for (auto edge : edges) {
        Part::FilletElement fe = {edge, startRadius, endRadius};
        filletElements.push_back(fe);
    }
    return filletElements;
}


void ExecutePython(const std::vector<std::string>& python)
{
    Base::InterpreterSingleton is = Base::InterpreterSingleton();

    for (auto const& line : python) {
        is.runInteractiveString(line.c_str());
    }
}


void rectangle(double height, double width, const char* name)
{
    std::vector<std::string> rectstring {
        "import FreeCAD, Part",
        "V1 = FreeCAD.Vector(0, 0, 0)",
        boost::str(boost::format("V2 = FreeCAD.Vector(%d, 0, 0)") % height),
        boost::str(boost::format("V3 = FreeCAD.Vector(%d, %d, 0)") % height % width),
        boost::str(boost::format("V4 = FreeCAD.Vector(0, %d, 0)") % width),
        "P1 = Part.makePolygon([V1, V2, V3, V4],True)",
        "F1 = Part.Face(P1)",  // Make the face or the volume calc won't work right.
        boost::str(boost::format("Part.show(F1,'%s')") % name),
    };
    ExecutePython(rectstring);
}

std::tuple<TopoDS_Face, TopoDS_Wire, TopoDS_Edge, TopoDS_Edge, TopoDS_Edge, TopoDS_Edge>
CreateRectFace(float len, float wid)
{
    auto edge1 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(len, 0.0, 0.0)).Edge();
    auto edge2 = BRepBuilderAPI_MakeEdge(gp_Pnt(len, 0.0, 0.0), gp_Pnt(len, wid, 0.0)).Edge();
    auto edge3 = BRepBuilderAPI_MakeEdge(gp_Pnt(len, wid, 0.0), gp_Pnt(0.0, wid, 0.0)).Edge();
    auto edge4 = BRepBuilderAPI_MakeEdge(gp_Pnt(0.0, wid, 0.0), gp_Pnt(0.0, 0.0, 0.0)).Edge();
    auto wire1 = BRepBuilderAPI_MakeWire({edge1, edge2, edge3, edge4}).Wire();
    auto face1 = BRepBuilderAPI_MakeFace(wire1).Face();
    return {face1, wire1, edge1, edge2, edge3, edge4};
}

std::tuple<TopoDS_Face, TopoDS_Wire, TopoDS_Wire>
CreateFaceWithRoundHole(float len, float wid, float radius)
{
    auto [face1, wire1, edge1, edge2, edge3, edge4] = CreateRectFace(len, wid);
    auto circ1 =
        GC_MakeCircle(gp_Pnt(len / 2.0, wid / 2.0, 0), gp_Dir(0.0, 0.0, 1.0), radius).Value();
    auto edge5 = BRepBuilderAPI_MakeEdge(circ1).Edge();
    auto wire2 = BRepBuilderAPI_MakeWire(edge5).Wire();
    auto face2 = BRepBuilderAPI_MakeFace(face1, wire2).Face();
    // Beware:  somewhat counterintuitively, face2 is the sum of face1 and the area inside wire2,
    // not the difference.
    return {face2, wire1, wire2};
}

testing::AssertionResult
boxesMatch(const Base::BoundBox3d& b1, const Base::BoundBox3d& b2, double prec)
{
    if (abs(b1.MinX - b2.MinX) < prec && abs(b1.MinY - b2.MinY) < prec
        && abs(b1.MinZ - b2.MinZ) < prec && abs(b1.MaxX - b2.MaxX) < prec
        && abs(b1.MaxY - b2.MaxY) < prec && abs(b1.MaxZ - b2.MaxZ) < prec) {
        return testing::AssertionSuccess();
    }
    return testing::AssertionFailure()
        << "(" << b1.MinX << "," << b1.MinY << "," << b1.MinZ << " ; "
        << "(" << b1.MaxX << "," << b1.MaxY << "," << b1.MaxZ << ") != (" << b2.MinX << ","
        << b2.MinY << "," << b2.MinZ << " ; " << b2.MaxX << "," << b2.MaxY << "," << b2.MaxZ << ")";
}

std::map<IndexedName, MappedName> elementMap(const TopoShape& shape)
{
    std::map<IndexedName, MappedName> result {};
    auto elements = shape.getElementMap();
    for (auto const& entry : elements) {
        result[entry.index] = entry.name;
    }
    return result;
}

std::string mappedElementVectorToString(std::vector<MappedElement>& elements)
{
    std::stringstream output;
    output << "{";
    for (const auto& element : elements) {
        output << "\"" << element.name.toString() << "\", ";
    }
    output << "}";
    return output.str();
}

bool matchStringsWithoutClause(std::string first, std::string second, const std::string& regex)
{
    first = std::regex_replace(first, std::regex(regex), "");
    second = std::regex_replace(second, std::regex(regex), "");
    return first == second;
}

/**
 *  Check to see if the elementMap in a shape contains all the names in a list
 *  There are some sections of the name that can vary due to random numbers or
 *  memory addresses, so we use a regex to exclude those sections while still
 *  validating that the name exists and is the correct type.
 * @param shape The Shape
 * @param names The vector of names
 * @return An assertion usable by the gtest framework
 */
testing::AssertionResult elementsMatch(const TopoShape& shape,
                                       const std::vector<std::string>& names)
{
    auto elements = shape.getElementMap();
    if (!elements.empty() || !names.empty()) {
        for (const auto& name : names) {
            if (std::find_if(elements.begin(),
                             elements.end(),
                             [&, name](const Data::MappedElement& element) {
                                 return matchStringsWithoutClause(
                                     element.name.toString(),
                                     name,
                                     "(;D|;:H|;K)-?[a-fA-F0-9]+(:[0-9]+)?|(\\(.*?\\))?");
                                 // ;D ;:H and ;K are the sections of an encoded name for
                                 // Duplicate, Tag and a Face name in slices.  All three of these
                                 // can vary from run to run or platform to platform, as they are
                                 // based on either explicit random numbers or memory addresses.
                                 // Thus we remove the value from comparisons and just check that
                                 // they exist.  The full form could be something like ;:He59:53
                                 // which is what we match and remove.  We also pull out any
                                 // subexpressions wrapped in parens to keep the parse from
                                 // becoming too complex.
                             })
                == elements.end()) {
                return testing::AssertionFailure() << mappedElementVectorToString(elements);
            }
        }
    }
    return testing::AssertionSuccess();
}

testing::AssertionResult allElementsMatch(const TopoShape& shape,
                                          const std::vector<std::string>& names)
{
    auto elements = shape.getElementMap();
    if (elements.size() != names.size()) {
        return testing::AssertionFailure()
            << elements.size() << " != " << names.size()
            << " elements: " << mappedElementVectorToString(elements);
    }
    return elementsMatch(shape, names);
}

std::pair<TopoDS_Shape, TopoDS_Shape> CreateTwoCubes()
{
    auto boxMaker1 = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0);
    boxMaker1.Build();
    auto box1 = boxMaker1.Shape();

    auto boxMaker2 = BRepPrimAPI_MakeBox(1.0, 1.0, 1.0);
    boxMaker2.Build();
    auto box2 = boxMaker2.Shape();
    auto transform = gp_Trsf();
    transform.SetTranslation(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(1.0, 0.0, 0.0));
    box2.Location(TopLoc_Location(transform));

    return {box1, box2};
}

std::pair<TopoShape, TopoShape> CreateTwoTopoShapeCubes()
{
    auto [box1, box2] = CreateTwoCubes();
    std::vector<TopoShape> vec;
    long tag = 1L;
    for (TopExp_Explorer exp(box1, TopAbs_FACE); exp.More(); exp.Next()) {
        vec.emplace_back(TopoShape(exp.Current(), tag++));
    }
    TopoShape box1ts;
    box1ts.makeElementCompound(vec);
    box1ts.Tag = tag++;
    vec.clear();
    for (TopExp_Explorer exp(box2, TopAbs_FACE); exp.More(); exp.Next()) {
        vec.emplace_back(TopoShape(exp.Current(), tag++));
    }
    TopoShape box2ts;
    box2ts.Tag = tag++;
    box2ts.makeElementCompound(vec);

    return {box1ts, box2ts};
}

}  // namespace PartTestHelpers

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
