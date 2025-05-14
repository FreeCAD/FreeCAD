// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>
#include "Mod/Part/App/Attacher.h"
#include "Mod/PartDesign/App/Body.h"
#include "Mod/PartDesign/App/DatumPlane.h"

class DatumPlaneTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        _docName = App::GetApplication().getUniqueDocumentName("test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser");
        _body = _doc->addObject<PartDesign::Body>();
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(_docName.c_str());
    }

    App::Document* getDocument() const
    {
        return _doc;
    }

    PartDesign::Body* getBody() const
    {
        return _body;
    }

private:
    std::string _docName;
    App::Document* _doc = nullptr;
    PartDesign::Body* _body = nullptr;
};

TEST_F(DatumPlaneTest, attachDatumPlane)
{
    auto datumPlane = getDocument()->addObject<PartDesign::Plane>("Plane");
    ASSERT_TRUE(datumPlane);
    getBody()->addObject(datumPlane);
    auto origin = getBody()->getOrigin();

    App::PropertyLinkSubList support;
    std::vector<App::DocumentObject*> objs;
    std::vector<std::string> subs;
    objs.push_back(origin->getXY());
    subs.emplace_back();
    support.setValues(objs, subs);

    auto attach = datumPlane->getExtensionByType<Part::AttachExtension>();
    attach->attacher().setReferences(support);
    Attacher::SuggestResult sugr;
    attach->attacher().suggestMapModes(sugr);
    EXPECT_EQ(sugr.message, Attacher::SuggestResult::srOK);
}
