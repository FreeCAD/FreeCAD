#include <App/Application.h>
#include <App/Document.h>
#include <Base/Precision.h>
#include "Mod/Part/App/FeaturePartBox.h"
#include "Mod/Part/App/FeaturePartFuse.h"
#include "Mod/Part/App/FeatureFillet.h"
#include <BRepGProp.hxx>

namespace PartTestHelpers
{

double getVolume(TopoDS_Shape shape);

class PartTestHelperClass
{
public:
    App::Document* _doc;
    std::string _docName;
    Part::Box *_box1obj, *_box2obj, *_box3obj, *_box4obj, *_box5obj, *_box6obj;

    void createTestFile();
};

}  // namespace PartTestHelpers
