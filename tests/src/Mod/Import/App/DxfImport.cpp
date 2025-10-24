// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <functional>
#include <App/Document.h>
#include <Mod/Import/App/dxf/ImpExpDxf.h>

class DXFImportTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

TEST_F(DXFImportTest, testDXFImportCPPIssue20195)
{
    auto& app = App::GetApplication();
    ParameterGrp::handle hGrp =
        app.GetParameterGroupByPath("User parameter:BaseApp/Preferences/Draft");

    // Set options, doing our best to restore them:
    auto wasShowDialog = hGrp->GetBool("dxfShowDialog", true);
    auto wasUseLegacyImporter = hGrp->GetBool("dxfUseLegacyImporter", false);
    auto wasUseLayers = hGrp->GetBool("dxfUseDraftVisGroups", true);
    auto wasImportMode = hGrp->GetInt("DxfImportMode", 2);
    auto wasCreateSketch = hGrp->GetBool("dxfCreateSketch", false);
    auto wasImportAnonymousBlocks = hGrp->GetBool("dxfstarblocks", false);

    auto doc = app.newDocument();

    class xxx
    {
    public:
        xxx(std::function<void()> callOnDTor)
            : CallOnDTor(callOnDTor)
        {}
        ~xxx()
        {
            CallOnDTor();
        }

    private:
        std::function<void()> CallOnDTor;
    };
    xxx cleanup([&]() {
        hGrp->SetBool("dxfShowDialog", wasShowDialog);
        hGrp->SetBool("dxfUseLegacyImporter", wasUseLegacyImporter);
        hGrp->SetBool("dxfUseDraftVisGroups", wasUseLayers);
        hGrp->SetInt("DxfImportMode", wasImportMode);
        hGrp->SetBool("dxfCreateSketch", wasCreateSketch);
        hGrp->SetBool("dxfstarblocks", wasImportAnonymousBlocks);
    });

    // disable Preferences dialog in gui mode(avoids popup prompt to user)
    hGrp->SetBool("dxfShowDialog", false);
    // Use the new C++ importer-- that's where the bug was
    hGrp->SetBool("dxfUseLegacyImporter", false);
    // Preserve the DXF layers(makes the checking of document contents easier)
    hGrp->SetBool("dxfUseDraftVisGroups", true);
    // create simple part shapes(2 params)
    // This is required to display the bug because creation of Draft objects clears out the
    // pending exception this test is looking for, whereas creation of the simple shape object
    // actually throws on the pending exception so the entity is absent from the document.
    hGrp->SetInt("DxfImportMode", 2);
    hGrp->SetBool("dxfCreateSketch", false);
    hGrp->SetBool("dxfstarblocks", false);
    auto name(app.getHomePath() + "tests/TestData/Issue24314.dxf");
    Import::ImpExpDxfRead dxf_file(name, doc);

    dxf_file.DoRead(false);
    doc->recompute();

    // This doc should have 3 objects : The Layers container, the DXF layer called 0, and one Line
    EXPECT_EQ(doc->getObjects().size(), 3);
    app.closeDocument(doc->getName());
}
