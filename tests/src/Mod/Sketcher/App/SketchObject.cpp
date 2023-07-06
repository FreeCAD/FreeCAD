// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Sketcher/App/SketchObject.h>

class SketchObjectTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (App::Application::GetARGC() == 0) {
            int argc = 1;
            char* argv[] = {"FreeCAD"};
            App::Application::Config()["ExeName"] = "FreeCAD";
            App::Application::init(argc, argv);
        }
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        auto _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        // TODO: Do we add a body first, or is just adding sketch sufficient for this test?
        _sketchobj = static_cast<Sketcher::SketchObject*>(_doc->addObject("Sketcher::SketchObject"));
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

private:
    // TODO: use shared_ptr or something else here?
    Sketcher::SketchObject* _sketchobj;
    std::string _docName;
};

TEST_F(SketchObjectTest, createSketchObject) // NOLINT
{
    // Arrange

    // Act

    // Assert
}
