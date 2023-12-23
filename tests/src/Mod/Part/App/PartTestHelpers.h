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
    std::array<Part::Box *, 6> _boxes;
    void createTestDoc();
};

}  // namespace PartTestHelpers
