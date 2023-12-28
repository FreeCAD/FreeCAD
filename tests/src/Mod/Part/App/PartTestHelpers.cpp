
#include "PartTestHelpers.h"

namespace PartTestHelpers
{

double getVolume(TopoDS_Shape shape)
{
    GProp_GProps prop;

    BRepGProp::VolumeProperties(shape, prop); //, 1.0e-1, Standard_False, Standard_False);
    // BRepGProp::VolumePropertiesGK(shape, prop, 1.0e-5, Standard_False, Standard_False);
    return prop.Mass(); // Yes, Mass is how you get Volume in opencascade.
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
        auto box = _boxes[i] = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        box->Length.setValue(1);
        box->Width.setValue(2);
        box->Height.setValue(3);
        box->Placement.setValue(
            Base::Placement(box_origins[i], Base::Rotation(), Base::Vector3d()));
    }
}

std::vector<Part::FilletElement>
_getFilletEdges(std::vector<int> edges, double startRadius, double endRadius)
{
    std::vector<Part::FilletElement> filletElements;
    for (auto edge : edges) {
        Part::FilletElement fe = {edge, startRadius, endRadius};
        filletElements.push_back(fe);
    }
    return filletElements;
}

void executePython(std::vector<std::string> python)
{
    Base::InterpreterSingleton is = Base::InterpreterSingleton();

    for (auto line : python) {
        is.runInteractiveString(line.c_str());
    }
}


void rectangle(double height, double width, char* name)
{
    std::vector<std::string> v {
        "import FreeCAD, Part",
        "V1 = FreeCAD.Vector(0, 0, 0)",
        boost::str(boost::format("V2 = FreeCAD.Vector(%d, 0, 0)") % height),
        boost::str(boost::format("V3 = FreeCAD.Vector(%d, %d, 0)") % height % width),
        boost::str(boost::format("V4 = FreeCAD.Vector(0, %d, 0)") % width),
        "P1 = Part.makePolygon([V1, V2, V3, V4, V1])",
        "F1 = Part.Face(P1)",  // Make the face or the volume calc won't work right.
        boost::str(boost::format("Part.show(F1,'%s')") % name),
    };
    executePython(v);
}


}  // namespace PartTestHelpers
