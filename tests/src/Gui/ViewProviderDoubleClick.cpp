// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Inventor/SoDB.h>
#include <App/Document.h>
#include <App/DocumentObserver.h>
#include <App/Extension.h>
#include <App/GroupExtension.h>
#include <App/Link.h>
#include <Gui/SoFCDB.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <src/App/InitApplication.h>

namespace
{
class LegacyProvider: public Gui::ViewProvider
{
public:
    bool doubleClicked() override
    {
        ++legacyCalls;
        return legacyResult;
    }

    int legacyCalls = 0;
    bool legacyResult = true;
};

class OccurrenceProvider: public LegacyProvider
{
public:
    std::optional<bool> doubleClickedOccurrence(const App::SubObjectT& reference) override
    {
        receivedReference = reference;
        ++occurrenceCalls;
        return occurrenceResult;
    }

    App::SubObjectT receivedReference;
    int occurrenceCalls = 0;
    std::optional<bool> occurrenceResult = true;
};

class ViewProviderDoubleClick: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
        if (Gui::ViewProvider::getClassTypeId().isBad()) {
            Gui::ViewProvider::init();
        }
        if (Gui::ViewProviderDocumentObject::getClassTypeId().isBad()) {
            Gui::ViewProviderDocumentObject::init();
        }
        SoDB::init();
        if (!Gui::SoFCDB::isInitialized()) {
            Gui::SoFCDB::init();
        }
    }
};

TEST_F(ViewProviderDoubleClick, LegacyCallbackRunsOnce)
{
    LegacyProvider provider;
    EXPECT_TRUE(provider.doubleClickedObject(App::SubObjectT()));
    EXPECT_EQ(provider.legacyCalls, 1);
}

TEST_F(ViewProviderDoubleClick, LegacyRejectionIsPreserved)
{
    LegacyProvider provider;
    provider.legacyResult = false;
    EXPECT_FALSE(provider.doubleClickedObject(App::SubObjectT()));
    EXPECT_EQ(provider.legacyCalls, 1);
}

TEST_F(ViewProviderDoubleClick, ExplicitRejectionDoesNotFallBackToLegacyCallback)
{
    OccurrenceProvider provider;
    provider.occurrenceResult = false;
    EXPECT_FALSE(provider.doubleClickedObject(App::SubObjectT()));
    EXPECT_EQ(provider.occurrenceCalls, 1);
    EXPECT_EQ(provider.legacyCalls, 0);
}

TEST_F(ViewProviderDoubleClick, UnhandledOccurrenceUsesLegacyCallbackOnce)
{
    OccurrenceProvider provider;
    provider.occurrenceResult = std::nullopt;
    EXPECT_TRUE(provider.doubleClickedObject(App::SubObjectT()));
    EXPECT_EQ(provider.occurrenceCalls, 1);
    EXPECT_EQ(provider.legacyCalls, 1);
}

TEST_F(ViewProviderDoubleClick, KeepsTheCallersOccurrencePath)
{
    OccurrenceProvider provider;
    const App::SubObjectT reference(App::DocumentObjectT(), "Instance.Container.Array.");
    EXPECT_TRUE(provider.doubleClickedObject(reference));
    EXPECT_EQ(provider.receivedReference.getSubName(), reference.getSubName());
    EXPECT_EQ(provider.occurrenceCalls, 1);
    EXPECT_EQ(provider.legacyCalls, 0);
}

// These tests exercise model references without a GUI application or main window.
// Attaching a provider changes DisplayMode, whose normal notifications update GUI actions.
class ReferenceTestProvider: public Gui::ViewProviderDocumentObject
{
protected:
    void onBeforeChange(const App::Property*) override
    {}

    void onChanged(const App::Property*) override
    {}
};

class DefaultEditReference: public ViewProviderDoubleClick
{
protected:
    void SetUp() override
    {
        App::DocumentInitFlags flags;
        flags.createView = false;
        document = App::GetApplication().newDocument("default_edit_reference", nullptr, flags);
    }

    void TearDown() override
    {
        App::GetApplication().closeDocument(document->getName());
    }

    App::DocumentObject* addCoordinateSystem(const char* name)
    {
        // Exercise the same group extension without App::Part's origin/datum setup,
        // which emits property-editor notifications when another test has started the GUI.
        auto* object = document->addObject("App::GeoFeature", name, true, "");
        auto* group = static_cast<App::Extension*>(
            Base::Type::createInstanceByName("App::GeoFeatureGroupExtensionPython")
        );
        group->initExtension(object);  // The object owns dynamically added Python extensions.
        return object;
    }

    App::Document* document = nullptr;
};

TEST_F(DefaultEditReference, UsesModelContainersInsteadOfIncomingLinks)
{
    auto* outer = addCoordinateSystem("Outer");
    auto* inner = addCoordinateSystem("Inner");
    auto* object = document->addObject("App::FeatureTest", "Feature");
    outer->getExtensionByType<App::GroupExtension>()->addObject(inner);
    inner->getExtensionByType<App::GroupExtension>()->addObject(object);
    auto* link = freecad_cast<App::Link*>(document->addObject("App::Link", "Instance"));
    link->LinkedObject.setValue(outer);

    ReferenceTestProvider provider;
    provider.attach(object);
    const auto reference = provider.getDefaultEditReference();
    EXPECT_EQ(reference.getObject(), outer);
    EXPECT_EQ(reference.getSubName(), "Inner.Feature.");
    EXPECT_EQ(outer->getSubObject(reference.getSubName().c_str()), object);
}

TEST_F(DefaultEditReference, IncludesLinkGroupContainers)
{
    auto* group = freecad_cast<App::LinkGroup*>(document->addObject("App::LinkGroup", "Group"));
    auto* object = document->addObject("App::FeatureTest", "Feature");
    group->ElementList.setValues({object});

    ReferenceTestProvider provider;
    provider.attach(object);
    const auto reference = provider.getDefaultEditReference();
    EXPECT_EQ(reference.getObject(), group);
    EXPECT_EQ(reference.getSubName(), "Feature.");
    EXPECT_EQ(group->getSubObject(reference.getSubName().c_str()), object);
}

TEST_F(DefaultEditReference, IncomingLinkDoesNotReplaceTopLevelObject)
{
    auto* object = document->addObject("App::FeatureTest", "Feature");
    auto* link = freecad_cast<App::Link*>(document->addObject("App::Link", "Instance"));
    link->LinkedObject.setValue(object);

    ReferenceTestProvider provider;
    provider.attach(object);
    const auto reference = provider.getDefaultEditReference();
    EXPECT_EQ(reference.getObject(), object);
    EXPECT_TRUE(reference.getSubName().empty());
}
}  // namespace
