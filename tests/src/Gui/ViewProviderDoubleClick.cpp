// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <Inventor/SoDB.h>
#include <App/DocumentObserver.h>
#include <Gui/SoFCDB.h>
#include <Gui/ViewProvider.h>
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
        SoDB::init();
        Gui::SoFCDB::init();
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
}  // namespace
