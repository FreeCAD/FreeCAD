
#include "PartTestHelpers.h"

namespace PartTestHelpers
{

double getVolume(TopoDS_Shape shape)
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
        auto box = _boxes[i] = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        box->Length.setValue(1);
        box->Width.setValue(2);
        box->Height.setValue(3);
        box->Placement.setValue(
            Base::Placement(box_origins[i], Base::Rotation(), Base::Vector3d()));
    }
}

}  // namespace PartTestHelpers

// https://google.github.io/googletest/advanced.html#teaching-googletest-how-to-print-your-values
namespace Part
{
void PrintTo(ShapeHistory sh, std::ostream* os)
{
    const char* types[] =
        {"Compound", "CompSolid", "Solid", "Shell", "Face", "Wire", "Edge", "Vertex", "Shape"};
    *os << "History for " << types[sh.type] << " is ";
    for (const auto& it : sh.shapeMap) {
        int old_shape_index = it.first;
        *os << " " << old_shape_index << ": ";
        if (!it.second.empty()) {
            for (auto it2 : it.second) {
                *os << it2 << " ";
            }
        }
    }
    *os << std::endl;
}
}  // namespace Part
