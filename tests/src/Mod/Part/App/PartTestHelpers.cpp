// SPDX-License-Identifier: LGPL-2.1-or-later

#include "PartTestHelpers.h"

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)

namespace PartTestHelpers
{

double getVolume(const TopoDS_Shape& shape)
{
    GProp_GProps prop;
    BRepGProp::VolumeProperties(shape, prop);
    return prop.Mass();
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
        auto box = _boxes[i] = dynamic_cast<Part::Box*>(_doc->addObject("Part::Box"));  // NOLINT
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

void executePython(const std::vector<std::string>& python)
{
    Base::InterpreterSingleton is = Base::InterpreterSingleton();

    for (auto const& line : python) {
        is.runInteractiveString(line.c_str());
    }
}


void rectangle(double height, double width, char* name)
{
    std::vector<std::string> rectstring {
        "import FreeCAD, Part",
        "V1 = FreeCAD.Vector(0, 0, 0)",
        boost::str(boost::format("V2 = FreeCAD.Vector(%d, 0, 0)") % height),
        boost::str(boost::format("V3 = FreeCAD.Vector(%d, %d, 0)") % height % width),
        boost::str(boost::format("V4 = FreeCAD.Vector(0, %d, 0)") % width),
        "P1 = Part.makePolygon([V1, V2, V3, V4],True)",
        "F1 = Part.Face(P1)",  // Make the face or the volume calc won't work right.
        // "L1 = Part.LineSegment(V1, V2)",
        // "L2 = Part.LineSegment(V2, V3)",
        // "L3 = Part.LineSegment(V3, V4)",
        // "L4 = Part.LineSegment(V4, V1)",
        // "S1 = Part.Shape([L1,L2,L3,L4])",
        // "W1 = Part.Wire(S1.Edges)",
        // "F1 = Part.Face(W1)",  // Make the face or the volume calc won't work right.
        boost::str(boost::format("Part.show(F1,'%s')") % name),
    };
    executePython(rectstring);
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

}  // namespace PartTestHelpers

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers)
