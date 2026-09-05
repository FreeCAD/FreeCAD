// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <src/App/InitApplication.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>

namespace
{

class ObjectNameGate: public Gui::SelectionGate
{
public:
    explicit ObjectNameGate(const char* allowedName)
        : _allowedName(allowedName)
    {}

    bool allow(App::Document*, App::DocumentObject* obj, const char*) override
    {
        if (!obj || _allowedName != obj->getNameInDocument()) {
            notAllowedReason = "rejected by test gate";
            return false;
        }
        return true;
    }

private:
    std::string _allowedName;
};

class SelectionTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        // Keep this fixture headless even if another Gui gtest already created
        // Gui::Application in the shared binary.
        App::DocumentInitFlags createFlags;
        createFlags.createView = false;
        _docName = App::GetApplication().getUniqueDocumentName("selection_test");
        _doc = App::GetApplication().newDocument(_docName.c_str(), "testUser", createFlags);
        _allowedObject = _doc->addObject("App::FeatureTest", "Allowed");
        _rejectedObject = _doc->addObject("App::FeatureTest", "Rejected");
    }

    void TearDown() override
    {
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().clearSelection();
        if (App::GetApplication().getDocument(_docName.c_str())) {
            App::GetApplication().closeDocument(_docName.c_str());
        }
    }

    std::string _docName;
    App::Document* _doc {};
    App::DocumentObject* _allowedObject {};
    App::DocumentObject* _rejectedObject {};
};

Gui::SelectionPickPolicy::Candidate pickCandidate(
    const void* owner,
    int priority,
    bool closeToFirst,
    bool hasGate,
    bool passesGate
)
{
    Gui::SelectionPickPolicy::Candidate candidate;
    candidate.owner = owner;
    candidate.priority = priority;
    candidate.closeToFirst = closeToFirst;
    candidate.hasGate = hasGate;
    candidate.passesGate = passesGate;
    return candidate;
}

}  // namespace

TEST_F(SelectionTest, testSelectionAllowsObjectsWhenNoGateIsInstalled)
{
    EXPECT_TRUE(Gui::Selection().testSelection(_doc, _allowedObject));
}

TEST_F(SelectionTest, testSelectionUsesActiveGateWithoutKeepingRejectionReason)
{
    auto* gate = new ObjectNameGate(_allowedObject->getNameInDocument());
    gate->notAllowedReason = "existing reason";
    Gui::Selection().addSelectionGate(gate, Gui::ResolveMode::NoResolve);

    EXPECT_TRUE(Gui::Selection().testSelection(_doc, _allowedObject));
    EXPECT_EQ(gate->notAllowedReason, "existing reason");

    EXPECT_FALSE(Gui::Selection().testSelection(_doc, _rejectedObject));
    EXPECT_EQ(gate->notAllowedReason, "existing reason");
    EXPECT_FALSE(Gui::Selection().hasPreselection());
    EXPECT_FALSE(Gui::Selection().hasSelection(_docName.c_str(), Gui::ResolveMode::NoResolve));
}

TEST_F(SelectionTest, selectionFilterParsesObjectType)
{
    Gui::SelectionFilter filter("SELECT App::FeatureTest");

    ASSERT_TRUE(filter.isValid());
    const auto ast = filter.getAst();
    ASSERT_EQ(ast->Objects.size(), 1U);
    EXPECT_EQ(ast->Objects[0]->ObjectType.getName(), "App::FeatureTest");
    EXPECT_TRUE(ast->Objects[0]->SubName.empty());
    EXPECT_EQ(ast->Objects[0]->Slice, nullptr);
}

TEST_F(SelectionTest, selectionFilterParsesSubelementAndCountForms)
{
    Gui::SelectionFilter range("SELECT App::FeatureTest SUBELEMENT Edge COUNT 2..5");
    Gui::SelectionFilter openRange("SELECT App::FeatureTest COUNT 2..");
    Gui::SelectionFilter exact("SELECT App::FeatureTest COUNT 2");

    ASSERT_EQ(range.getAst()->Objects.size(), 1U);
    EXPECT_EQ(range.getAst()->Objects[0]->SubName, "Edge");
    ASSERT_NE(range.getAst()->Objects[0]->Slice, nullptr);
    EXPECT_EQ(range.getAst()->Objects[0]->Slice->Min, 2);
    EXPECT_EQ(range.getAst()->Objects[0]->Slice->Max, 5);

    ASSERT_NE(openRange.getAst()->Objects[0]->Slice, nullptr);
    EXPECT_EQ(openRange.getAst()->Objects[0]->Slice->Min, 2);
    EXPECT_EQ(openRange.getAst()->Objects[0]->Slice->Max, std::numeric_limits<int>::max());

    ASSERT_NE(exact.getAst()->Objects[0]->Slice, nullptr);
    EXPECT_EQ(exact.getAst()->Objects[0]->Slice->Min, 2);
    EXPECT_EQ(exact.getAst()->Objects[0]->Slice->Max, 2);
}

TEST_F(SelectionTest, selectionFilterParsesMultipleMatchLines)
{
    Gui::SelectionFilter filter(
        "SELECT App::FeatureTest SUBELEMENT Edge\n"
        "SELECT App::DocumentObject COUNT 0..3"
    );

    const auto ast = filter.getAst();
    ASSERT_EQ(ast->Objects.size(), 2U);
    EXPECT_EQ(ast->Objects[0]->ObjectType.getName(), "App::FeatureTest");
    EXPECT_EQ(ast->Objects[1]->ObjectType.getName(), "App::DocumentObject");
    ASSERT_NE(ast->Objects[1]->Slice, nullptr);
    EXPECT_EQ(ast->Objects[1]->Slice->Min, 0);
    EXPECT_EQ(ast->Objects[1]->Slice->Max, 3);
}

TEST_F(SelectionTest, selectionFilterRetainsIgnoredUnmatchedCharacters)
{
    Gui::SelectionFilter filter("SELECT App::FeatureTest COUNT 1...5");

    const auto ast = filter.getAst();
    ASSERT_EQ(ast->Objects.size(), 1U);
    ASSERT_NE(ast->Objects[0]->Slice, nullptr);
    EXPECT_EQ(ast->Objects[0]->Slice->Min, 1);
    EXPECT_EQ(ast->Objects[0]->Slice->Max, 5);
}

TEST_F(SelectionTest, emptySelectionFilterIsValidToStoreButHasNoAst)
{
    Gui::SelectionFilter filter("");

    EXPECT_FALSE(filter.isValid());
    EXPECT_TRUE(filter.getFilter().empty());
}

TEST_F(SelectionTest, selectionFilterRejectsMalformedGrammar)
{
    const std::vector<std::string> malformed {
        "SELECT",
        "select App::FeatureTest",
        "SELECT App::FeatureTest SUBELEMENT",
        "SELECT App::FeatureTest COUNT",
        "SELECT App::FeatureTest COUNT ..5",
        "SELECT App::FeatureTest COUNT 1..5 COUNT 2",
        "SELECT App::FeatureTest::Extra",
    };

    for (const auto& text : malformed) {
        EXPECT_THROW(Gui::SelectionFilter filter(text), Base::ParserError) << text;
    }
}

TEST(SelectionPickPolicyTest, canFinalizeSinglePickWhenNoGateIsInstalled)
{
    int owner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&owner, 0, true, false, true),
    };

    EXPECT_TRUE(Gui::SelectionPickPolicy::canFinalizeSinglePick(candidates));
}

TEST(SelectionPickPolicyTest, continuesSinglePickWhenGateRejectedAllCurrentCandidates)
{
    int owner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&owner, 0, true, true, false),
    };

    EXPECT_FALSE(Gui::SelectionPickPolicy::canFinalizeSinglePick(candidates));
}

TEST(SelectionPickPolicyTest, choosesAllowedCandidateWhenPreferredPickIsRejected)
{
    int firstOwner {};
    int secondOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&firstOwner, 1, true, true, false),
        pickCandidate(&secondOwner, 0, true, true, true),
    };

    EXPECT_EQ(Gui::SelectionPickPolicy::choosePreferredPick(candidates), 1U);
}

TEST(SelectionPickPolicyTest, preservesPriorityChoiceWithinFirstOwner)
{
    int firstOwner {};
    int secondOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&firstOwner, 1, true, false, true),
        pickCandidate(&firstOwner, 2, true, false, true),
        pickCandidate(&secondOwner, 3, true, false, true),
    };

    EXPECT_EQ(Gui::SelectionPickPolicy::choosePreferredPick(candidates), 1U);
}

TEST(SelectionPickPolicyTest, keepsPreferredPickWhenNoCandidatePassesGate)
{
    int firstOwner {};
    int secondOwner {};
    std::vector<Gui::SelectionPickPolicy::Candidate> candidates {
        pickCandidate(&firstOwner, 1, true, true, false),
        pickCandidate(&secondOwner, 0, true, true, false),
    };

    EXPECT_EQ(Gui::SelectionPickPolicy::choosePreferredPick(candidates), 0U);
}
