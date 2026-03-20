// SPDX-License-Identifier: LGPL-2.1-or-later

#include <FCConfig.h>

#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include "SketcherTestHelpers.h"

namespace
{

struct TextSetup
{
    int constrIdx;
    std::vector<int> helperGeoIds;
    int helperCount;
    int textGeoCount;
    // Number of extensions on each helper's canonical geometry
    // (used to verify extensions are preserved across edits)
    std::vector<size_t> helperCanonicalExtCounts;
};

TextSetup analyzeTextConstraint(Sketcher::SketchObject* sketch, int constrIdx)
{
    TextSetup result;
    result.constrIdx = constrIdx;
    result.helperCount = 0;
    result.textGeoCount = 0;

    const auto* constr = sketch->Constraints[constrIdx];
    for (int i = 1; constr->hasElement(i); ++i) {
        int geoId = constr->getGeoId(i);
        if (geoId == Sketcher::GeoEnum::GeoUndef) {
            continue;
        }
        const auto* geo = sketch->getGeometry(geoId);
        if (!geo) {
            continue;
        }
        if (Sketcher::GeometryFacade::getHelper(geo)) {
            result.helperCount++;
            result.helperGeoIds.push_back(geoId);
        }
        else {
            result.textGeoCount++;
        }
    }
    // Count extensions on canonical helper geometry
    const auto& canon = constr->canonicalGeometry;
    for (const auto& cg : canon) {
        if (cg && Sketcher::GeometryFacade::getHelper(cg.get())) {
            result.helperCanonicalExtCounts.push_back(cg->getExtensions().size());
        }
    }
    return result;
}

TextSetup createTextConstraint(Sketcher::SketchObject* sketch, const std::string& text, int helperFlags = 16 /* MetricBaseline */)
{
    std::string font = App::Application::getResourceDir()
        + "Mod/TechDraw/Resources/fonts/osifont-lgpl3fe.ttf";

    Part::GeomLineSegment frameLine;
    frameLine.setPoints(Base::Vector3d(0, 0, 0), Base::Vector3d(50, 0, 0));
    int frameGeoId = sketch->addGeometry(&frameLine, /*construction=*/true);

    auto* textConstr = new Sketcher::Constraint();
    textConstr->Type = Sketcher::Text;
    textConstr->truncateElements(0);
    textConstr->addElement(Sketcher::GeoElementId(frameGeoId));
    textConstr->setText(text);
    textConstr->setFont(font);
    int constrIdx = sketch->addConstraint(textConstr);

    std::string mutableText = text;
    std::string mutableFont = font;
    sketch->setTextAndFont(constrIdx, mutableText, mutableFont, false, helperFlags);

    return analyzeTextConstraint(sketch, constrIdx);
}

}  // namespace

TEST_F(SketchObjectTest, testSetTextAndFontPreservesHelpers)
{
    auto* sketch = getObject();
    std::string font = App::Application::getResourceDir()
        + "Mod/TechDraw/Resources/fonts/osifont-lgpl3fe.ttf";

    auto setup = createTextConstraint(sketch, "Text");
    EXPECT_EQ(setup.helperCount, 7);
    EXPECT_GT(setup.textGeoCount, 0);

    // Change text — helpers should survive with same geoIds
    std::string text2 = "Textj";
    int err = sketch->setTextAndFont(setup.constrIdx, text2, font, false, 16);
    EXPECT_EQ(err, 0);

    auto after = analyzeTextConstraint(sketch, setup.constrIdx);
    EXPECT_EQ(after.helperCount, 7);
    EXPECT_GT(after.textGeoCount, 0);
    EXPECT_EQ(after.helperGeoIds, setup.helperGeoIds);

    // Extensions on canonical helpers must be preserved (includes visual layer)
    EXPECT_EQ(after.helperCanonicalExtCounts, setup.helperCanonicalExtCounts);

    auto* constr = sketch->Constraints[setup.constrIdx];
    EXPECT_EQ(constr->getText(), "Textj");
}

TEST_F(SketchObjectTest, testSetTextAndFontUndoRestoresText)
{
    auto* sketch = getObject();
    auto* doc = sketch->getDocument();
    std::string font = App::Application::getResourceDir()
        + "Mod/TechDraw/Resources/fonts/osifont-lgpl3fe.ttf";

    // Enable undo
    doc->setUndoMode(1);

    // Create initial text
    doc->openTransaction("Create text");
    auto setup = createTextConstraint(sketch, "Text");
    doc->commitTransaction();

    EXPECT_EQ(sketch->Constraints[setup.constrIdx]->getText(), "Text");
    int origTextGeoCount = setup.textGeoCount;

    // Change text in a new transaction
    doc->openTransaction("Edit text");
    std::string text2 = "Textj";
    sketch->setTextAndFont(setup.constrIdx, text2, font, false, 16);
    doc->commitTransaction();

    EXPECT_EQ(sketch->Constraints[setup.constrIdx]->getText(), "Textj");

    // Undo — should restore original text
    doc->undo();

    EXPECT_EQ(sketch->Constraints[setup.constrIdx]->getText(), "Text");
    auto restored = analyzeTextConstraint(sketch, setup.constrIdx);
    EXPECT_EQ(restored.helperCount, 7);
    EXPECT_EQ(restored.textGeoCount, origTextGeoCount);
}

TEST_F(SketchObjectTest, testSetTextAndFontTwoTextsEditFirst)
{
    auto* sketch = getObject();
    std::string font = App::Application::getResourceDir()
        + "Mod/TechDraw/Resources/fonts/osifont-lgpl3fe.ttf";

    // Create two text objects
    auto text1 = createTextConstraint(sketch, "Text");
    auto text2 = createTextConstraint(sketch, "Hello");

    EXPECT_EQ(text1.helperCount, 7);
    EXPECT_EQ(text2.helperCount, 7);

    // Edit first text — second should be unaffected
    std::string newText = "Textj";
    int err = sketch->setTextAndFont(text1.constrIdx, newText, font, false, 16);
    EXPECT_EQ(err, 0);

    // First text: helpers preserved, text updated
    auto after1 = analyzeTextConstraint(sketch, text1.constrIdx);
    EXPECT_EQ(after1.helperCount, 7);
    EXPECT_GT(after1.textGeoCount, 0);
    EXPECT_EQ(after1.helperGeoIds, text1.helperGeoIds);
    EXPECT_EQ(sketch->Constraints[text1.constrIdx]->getText(), "Textj");

    // Second text: completely unchanged
    auto after2 = analyzeTextConstraint(sketch, text2.constrIdx);
    EXPECT_EQ(after2.helperCount, 7);
    EXPECT_EQ(after2.textGeoCount, text2.textGeoCount);
    EXPECT_EQ(sketch->Constraints[text2.constrIdx]->getText(), "Hello");
}

TEST_F(SketchObjectTest, testSetTextAndFontDuplicateTextEditFirst)
{
    auto* sketch = getObject();
    std::string font = App::Application::getResourceDir()
        + "Mod/TechDraw/Resources/fonts/osifont-lgpl3fe.ttf";

    // Create two text objects with IDENTICAL text
    auto text1 = createTextConstraint(sketch, "Text");
    auto text2 = createTextConstraint(sketch, "Text");

    EXPECT_EQ(text1.helperCount, 7);
    EXPECT_EQ(text2.helperCount, 7);
    EXPECT_EQ(sketch->Constraints[text1.constrIdx]->getText(), "Text");
    EXPECT_EQ(sketch->Constraints[text2.constrIdx]->getText(), "Text");

    // Edit first text — should update the correct one
    std::string newText = "Textj";
    int err = sketch->setTextAndFont(text1.constrIdx, newText, font, false, 16);
    EXPECT_EQ(err, 0);

    // First text: updated
    EXPECT_EQ(sketch->Constraints[text1.constrIdx]->getText(), "Textj");
    auto after1 = analyzeTextConstraint(sketch, text1.constrIdx);
    EXPECT_EQ(after1.helperCount, 7);
    EXPECT_EQ(after1.helperGeoIds, text1.helperGeoIds);

    // Second text: unchanged (must NOT have been modified by mistake)
    EXPECT_EQ(sketch->Constraints[text2.constrIdx]->getText(), "Text");
    auto after2 = analyzeTextConstraint(sketch, text2.constrIdx);
    EXPECT_EQ(after2.helperCount, 7);
    EXPECT_EQ(after2.textGeoCount, text2.textGeoCount);
}

TEST_F(SketchObjectTest, testSetTextAndFontDuplicateTextEditSecond)
{
    auto* sketch = getObject();
    std::string font = App::Application::getResourceDir()
        + "Mod/TechDraw/Resources/fonts/osifont-lgpl3fe.ttf";

    // Create two text objects with IDENTICAL text
    auto text1 = createTextConstraint(sketch, "Text");
    auto text2 = createTextConstraint(sketch, "Text");

    // Edit SECOND text — must not match the first
    std::string newText = "Changed";
    int err = sketch->setTextAndFont(text2.constrIdx, newText, font, false, 16);
    EXPECT_EQ(err, 0);

    // First text: must be unchanged
    EXPECT_EQ(sketch->Constraints[text1.constrIdx]->getText(), "Text");
    auto after1 = analyzeTextConstraint(sketch, text1.constrIdx);
    EXPECT_EQ(after1.helperCount, 7);
    EXPECT_EQ(after1.textGeoCount, text1.textGeoCount);

    // Second text: updated with preserved helpers
    EXPECT_EQ(sketch->Constraints[text2.constrIdx]->getText(), "Changed");
    auto after2 = analyzeTextConstraint(sketch, text2.constrIdx);
    EXPECT_EQ(after2.helperCount, 7);
    EXPECT_EQ(after2.helperGeoIds, text2.helperGeoIds);
}
