// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include <src/App/InitApplication.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Selection/Selection.h>
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

class MessageProviderGate: public Gui::SelectionGate, public Gui::SelectionGateMessageProvider
{
public:
    explicit MessageProviderGate(bool allowed)
        : _allowed(allowed)
    {}

    bool allow(App::Document*, App::DocumentObject*, const char*) override
    {
        ++allowCalls;
        notAllowedReason = _allowed ? "allowed probe reason" : "rejected probe reason";
        return _allowed;
    }

    QString selectionRejectionText() const override
    {
        ++renderCalls;
        return QStringLiteral("rendered test rejection");
    }

    int allowCalls {};
    mutable int renderCalls {};

private:
    bool _allowed;
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

TEST(SelectionGateMessageProviderTest, discoversProviderThroughSelectionGateReference)
{
    MessageProviderGate gate(false);
    const Gui::SelectionGate& base = gate;

    EXPECT_EQ(
        dynamic_cast<const Gui::SelectionGateMessageProvider*>(&base),
        static_cast<const Gui::SelectionGateMessageProvider*>(&gate)
    );
    EXPECT_EQ(gate.renderCalls, 0);
}

TEST(SelectionGateMessageProviderTest, legacyGateHasNoMessageProvider)
{
    ObjectNameGate gate("Allowed");
    const Gui::SelectionGate& base = gate;

    EXPECT_EQ(dynamic_cast<const Gui::SelectionGateMessageProvider*>(&base), nullptr);
}

TEST_F(SelectionTest, allowedProviderProbeRestoresReasonWithoutRendering)
{
    auto* gate = new MessageProviderGate(true);
    Gui::Selection().addSelectionGate(gate, Gui::ResolveMode::NoResolve);
    int probeCount = 0;

    for (const auto* originalReason : {"existing reason", ""}) {
        gate->notAllowedReason = originalReason;
        EXPECT_TRUE(Gui::Selection().testSelection(_doc, _allowedObject));
        ++probeCount;
        EXPECT_EQ(gate->allowCalls, probeCount);
        EXPECT_EQ(gate->notAllowedReason, originalReason);
        EXPECT_EQ(gate->renderCalls, 0);
    }
    EXPECT_FALSE(Gui::Selection().hasPreselection());
    EXPECT_FALSE(Gui::Selection().hasSelection(_docName.c_str(), Gui::ResolveMode::NoResolve));
}

TEST_F(SelectionTest, rejectedProviderProbeRestoresReasonWithoutRendering)
{
    auto* gate = new MessageProviderGate(false);
    Gui::Selection().addSelectionGate(gate, Gui::ResolveMode::NoResolve);
    int probeCount = 0;

    for (const auto* originalReason : {"existing reason", ""}) {
        gate->notAllowedReason = originalReason;
        EXPECT_FALSE(Gui::Selection().testSelection(_doc, _rejectedObject));
        ++probeCount;
        EXPECT_EQ(gate->allowCalls, probeCount);
        EXPECT_EQ(gate->notAllowedReason, originalReason);
        EXPECT_EQ(gate->renderCalls, 0);
    }
    EXPECT_FALSE(Gui::Selection().hasPreselection());
    EXPECT_FALSE(Gui::Selection().hasSelection(_docName.c_str(), Gui::ResolveMode::NoResolve));
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
