
#include "PartTestHelpers.h"

namespace PartTestHelpers {

double getVolume(TopoDS_Shape shape)
{
    GProp_GProps prop;
    BRepGProp::VolumeProperties(shape, prop);
    return prop.Mass();
}

void PartTestHelperClass::createTestFile()
{
        _docName = App::GetApplication().getUniqueDocumentName("test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _box1obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box2obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box3obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box4obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box5obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        _box6obj = static_cast<Part::Box*>(_doc->addObject("Part::Box"));
        for ( auto _box : {_box1obj, _box2obj, _box3obj, _box4obj, _box5obj, _box6obj} ) {
            _box->Length.setValue(1);
            _box->Width.setValue(2);
            _box->Height.setValue(3);
        }
        _box1obj->Placement.setValue(
            Base::Placement(Base::Vector3d(), Base::Rotation(), Base::Vector3d()));
        _box2obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 1, 0), Base::Rotation(), Base::Vector3d()));
        _box3obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 3, 0), Base::Rotation(), Base::Vector3d()));
        _box4obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2, 0), Base::Rotation(), Base::Vector3d()));
        _box5obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2 + Base::Precision::Confusion(), 0),
                            Base::Rotation(),
                            Base::Vector3d()));
        _box6obj->Placement.setValue(
            Base::Placement(Base::Vector3d(0, 2 - Base::Precision::Confusion() * 1000, 0),
                            Base::Rotation(),
                            Base::Vector3d()));
}

}