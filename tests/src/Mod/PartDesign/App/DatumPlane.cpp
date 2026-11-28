// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include "src/App/InitApplication.h"

#include <algorithm>
#include <string>
#include <vector>

#include <Base/Console.h>
#include <Base/Interpreter.h>

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

namespace
{
class OpenWarningObserver: public Base::ILogger
{
public:
    OpenWarningObserver()
    {
        Base::Console().attachObserver(this);
    }

    ~OpenWarningObserver() override
    {
        Base::Console().detachObserver(this);
    }

    const char* name() override
    {
        return "DatumOpenWarningObserver";
    }

    void sendLog(
        const std::string& notifier,
        const std::string& message,
        Base::LogStyle level,
        Base::IntendedRecipient,
        Base::ContentType
    ) override
    {
        if (level == Base::LogStyle::Warning
            && message.find("Placement changed on open") != std::string::npos) {
            labels.push_back(notifier.substr(notifier.find('#') + 1));
        }
    }

    std::vector<std::string> labels;
};
}  // namespace

TEST_F(DatumPlaneTest, warnWhenAxisAttachedDatumMovesOnOpen)
{
    const std::string names = "doc_name = '" + std::string(getDocument()->getName())
        + "'\nbody_name = '" + getBody()->getNameInDocument() + "'\n";
    OpenWarningObserver observer;
    Base::Interpreter().runString((names + R"PY(
import os, re, shutil, tempfile, zipfile
import FreeCAD as App
import PartDesign, Sketcher
doc = App.getDocument(doc_name)
body = doc.getObject(body_name)
axis = next(f for f in body.Origin.OriginFeatures if f.Role == "X_Axis")
for name, type_name in [("OnAxis", "PartDesign::Plane"), ("SketchOnAxis", "Sketcher::SketchObject"), ("Kept", "PartDesign::Plane")]:
    obj = body.newObject(type_name, name)
    obj.AttachmentSupport = [(axis, "")]
    obj.MapMode = "NormalToEdge"
doc.recompute()
for name in ("OnAxis", "SketchOnAxis"):
    doc.getObject(name).Placement = App.Placement(App.Vector(1, 2, 3), App.Rotation(App.Vector(0, 1, 0), 90))
directory = tempfile.mkdtemp()
try:
    source = os.path.join(directory, "source.FCStd")
    target = os.path.join(directory, "target.FCStd")
    doc.saveCopy(source)
    with zipfile.ZipFile(source) as zin, zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as zout:
        for item in zin.infolist():
            data = zin.read(item.filename)
            if item.filename == "Document.xml":
                data = re.sub(rb'ProgramVersion="[^"]*"', b'ProgramVersion="26.3R48900 (Git)"', data, count=1)
            zout.writestr(item, data)
    App.closeDocument(App.openDocument(target).Name)
finally:
    shutil.rmtree(directory)
)PY")
                                      .c_str());
    std::ranges::sort(observer.labels);

    EXPECT_EQ(observer.labels, (std::vector<std::string> {"OnAxis", "SketchOnAxis"}));
}
