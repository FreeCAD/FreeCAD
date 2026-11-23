// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <string>

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Expression.h>
#include <App/ObjectIdentifier.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/Groups.h>
#include <src/App/InitApplication.h>

class AssemblyObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        auto _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _assemblyObj = _doc->addObject<Assembly::AssemblyObject>();
        _jointGroupObj = _assemblyObj->addObject<Assembly::JointGroup>("jointGroupTest");
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    Assembly::AssemblyObject* getObject()
    {
        return _assemblyObj;
    }

private:
    // TODO: use shared_ptr or something else here?
    Assembly::AssemblyObject* _assemblyObj;
    Assembly::JointGroup* _jointGroupObj;
    std::string _docName;
};

TEST_F(AssemblyObjectTest, createAssemblyObject)  // NOLINT
{
    // Arrange

    // Act

    // Assert
}

namespace
{
class HasherMismatchObserver: public Base::ILogger
{
public:
    HasherMismatchObserver()
    {
        Base::Console().attachObserver(this);
    }

    ~HasherMismatchObserver() override
    {
        Base::Console().detachObserver(this);
    }

    const char* name() override
    {
        return "AssemblyHasherMismatchObserver";
    }

    void sendLog(
        const std::string&,
        const std::string& message,
        Base::LogStyle,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        if (message.find("hasher mismatch") != std::string::npos) {
            ++mismatches;
        }
    }

    int mismatches = 0;
};
}  // namespace

TEST_F(AssemblyObjectTest, jointSnappingToExternalBody)
{
    HasherMismatchObserver observer;
    ASSERT_NO_THROW(Base::Interpreter().runString(R"PY(
import os
import tempfile
import FreeCAD as App
import Part
import Sketcher
import UtilsAssembly

with tempfile.TemporaryDirectory() as directory:
    source = App.newDocument()
    target = App.newDocument()
    try:
        body = source.addObject("PartDesign::Body", "Body")
        sketch = body.newObject("Sketcher::SketchObject", "Sketch")
        points = [App.Vector(0, 0, 0), App.Vector(2, 0, 0),
                  App.Vector(2, 3, 0), App.Vector(0, 3, 0)]
        sketch.addGeometry([Part.LineSegment(points[i], points[(i + 1) % 4])
                            for i in range(4)], False)
        pad = body.newObject("PartDesign::Pad", "Pad")
        pad.Profile = sketch
        pad.Length = 7
        source.recompute()
        source.saveAs(os.path.join(directory, "Source.FCStd"))
        source_map = body.Shape.ElementMap
        assert source_map

        assembly = target.addObject("Assembly::AssemblyObject", "Assembly")
        target.saveAs(os.path.join(directory, "Assembly.FCStd"))
        link = assembly.newObject("App::Link", "Link")
        link.LinkedObject = body
        link.Placement = App.Placement(App.Vector(10, 20, 30), App.Rotation(15, 25, 35))
        target.recompute()
        link_map = link.Shape.ElementMap
        shape = Part.getShape(body, noElementMap=True)
        assert shape.isPartner(body.Shape)
        owner_shape = UtilsAssembly.getReferenceOwnerShape([assembly, [link.Name + ".Face1"]])
        assert owner_shape is not None
        assert owner_shape.isPartner(link.Shape)

        for token in ("Face1", "Edge1", "Vertex1"):
            element = shape.getElement(token)
            point = element.Point if token == "Vertex1" else element.Vertexes[0].Point
            ref = [link, [token]]
            mouse = UtilsAssembly.getGlobalPlacement(ref).multVec(point)
            vertex = UtilsAssembly.findElementClosestVertex(ref, mouse)
            assert vertex == "Vertex1", (token, vertex)
            ref = UtilsAssembly.addVertexToReference(ref, vertex)
            placement = UtilsAssembly.findPlacement(ref)
            assert placement.Base.isEqual(point, 1e-7), (token, placement.Base, point)
            vertex_shape = Part.getShape(link, noElementMap=True).getElement(vertex)
            assert UtilsAssembly.findVertexNameInObject(vertex_shape, link) == vertex

        assert body.Shape.ElementMap == source_map
        assert link.Shape.ElementMap == link_map
    finally:
        App.closeDocument(target.Name)
        App.closeDocument(source.Name)
)PY"));
    EXPECT_EQ(observer.mismatches, 0);
}
