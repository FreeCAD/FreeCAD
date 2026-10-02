// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <vector>

#include <App/Application.h>
#include <App/Document.h>
#include <App/PropertyLinks.h>
#include <src/App/InitApplication.h>

#include "PartTestHelpers.h"

namespace
{

class RecordingLinkSub: public App::PropertyLinkSub
{
public:
    void updateElementReference(App::DocumentObject* feature, bool reverse, bool notify) override
    {
        if (feature) {
            ++*updateCount;
            const bool destroyedItself = onUpdate && onUpdate();
            if (destroyedItself) {
                return;
            }
        }
        PropertyLinkSub::updateElementReference(feature, reverse, notify);
    }

    std::shared_ptr<int> updateCount = std::make_shared<int>(0);  // NOLINT
    std::function<bool()> onUpdate;                               // NOLINT
};

}  // namespace

class PropertyLinkElementReferencesTest: public ::testing::Test,
                                         public PartTestHelpers::PartTestHelperClass
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        createTestDoc();
        _doc->recompute();
    }

    void TearDown() override
    {
        _links.clear();
        App::GetApplication().closeDocument(_docName.c_str());
    }

    RecordingLinkSub* addLink(App::DocumentObject* target)
    {
        auto link = std::make_unique<RecordingLinkSub>();
        link->setScope(App::LinkScope::Hidden);
        // Registering without a container avoids change notifications for an unnamed property
        link->setValue(target, std::vector<std::string> {"Face1"});
        link->setContainer(_boxes[5]);
        EXPECT_TRUE(App::PropertyLinkBase::getElementReferences(target).contains(link.get()));
        return _links.emplace_back(std::move(link)).get();
    }

    void destroyLink(RecordingLinkSub* link)
    {
        std::erase_if(_links, [link](const auto& owned) { return owned.get() == link; });
    }

    std::vector<std::unique_ptr<RecordingLinkSub>> _links;  // NOLINT
};

TEST_F(PropertyLinkElementReferencesTest, everyRegisteredPropertyIsUpdatedOnce)
{
    // Arrange
    auto first = addLink(_boxes[0]);
    auto second = addLink(_boxes[0]);
    auto third = addLink(_boxes[1]);

    // Act
    App::PropertyLinkBase::updateAllElementReferences();

    // Assert
    EXPECT_EQ(*first->updateCount, 1);
    EXPECT_EQ(*second->updateCount, 1);
    EXPECT_EQ(*third->updateCount, 1);
}

TEST_F(PropertyLinkElementReferencesTest, propertyUnregisteredByEarlierUpdateIsSkipped)
{
    // Arrange
    auto first = addLink(_boxes[0]);
    auto second = addLink(_boxes[0]);
    first->onUpdate = [second] {
        second->unregisterElementReference();
        return false;
    };
    second->onUpdate = [first] {
        first->unregisterElementReference();
        return false;
    };

    // Act
    App::PropertyLinkBase::updateAllElementReferences();

    // Assert
    EXPECT_EQ(*first->updateCount + *second->updateCount, 1);
}

TEST_F(PropertyLinkElementReferencesTest, propertyDestroyedByEarlierUpdateIsSkipped)
{
    // Arrange
    auto first = addLink(_boxes[0]);
    auto second = addLink(_boxes[0]);
    auto firstCount = first->updateCount;
    auto secondCount = second->updateCount;
    first->onUpdate = [this, second] {
        destroyLink(second);
        return false;
    };
    second->onUpdate = [this, first] {
        destroyLink(first);
        return false;
    };

    // Act
    App::PropertyLinkBase::updateAllElementReferences();

    // Assert
    EXPECT_EQ(*firstCount + *secondCount, 1);
    EXPECT_EQ(_links.size(), 1);
}

TEST_F(PropertyLinkElementReferencesTest, propertyDestroyedDuringItsOwnUpdateIsUnregistered)
{
    // Arrange
    auto doomed = addLink(_boxes[2]);
    auto survivor = addLink(_boxes[3]);
    doomed->onUpdate = [this, doomed] {
        destroyLink(doomed);
        return true;
    };

    // Act
    App::PropertyLinkBase::updateAllElementReferences();

    // Assert
    EXPECT_TRUE(App::PropertyLinkBase::getElementReferences(_boxes[2]).empty());
    EXPECT_EQ(*survivor->updateCount, 1);
}

TEST_F(PropertyLinkElementReferencesTest, featureUpdateSkipsPropertyUnregisteredByEarlierUpdate)
{
    // Arrange
    auto first = addLink(_boxes[0]);
    auto second = addLink(_boxes[0]);
    first->onUpdate = [second] {
        second->unregisterElementReference();
        return false;
    };
    second->onUpdate = [first] {
        first->unregisterElementReference();
        return false;
    };

    // Act
    App::PropertyLinkBase::updateElementReferences(_boxes[0]);

    // Assert
    EXPECT_EQ(*first->updateCount + *second->updateCount, 1);
}

TEST_F(PropertyLinkElementReferencesTest, featureUpdateSkipsPropertyDestroyedByEarlierUpdate)
{
    // Arrange
    auto first = addLink(_boxes[0]);
    auto second = addLink(_boxes[0]);
    auto firstCount = first->updateCount;
    auto secondCount = second->updateCount;
    first->onUpdate = [this, second] {
        destroyLink(second);
        return false;
    };
    second->onUpdate = [this, first] {
        destroyLink(first);
        return false;
    };

    // Act
    App::PropertyLinkBase::updateElementReferences(_boxes[0]);

    // Assert
    EXPECT_EQ(*firstCount + *secondCount, 1);
    EXPECT_EQ(_links.size(), 1);
}

TEST_F(PropertyLinkElementReferencesTest, referencesRegisteredDuringUpdateWaitForNextPass)
{
    // Arrange
    const int newTargetCount = 64;  // Enough new keys to force the map to rehash
    std::vector<App::DocumentObject*> newTargets;
    for (int index = 0; index < newTargetCount; ++index) {
        newTargets.push_back(_doc->addObject<Part::Box>());
    }
    _doc->recompute();
    auto trigger = addLink(_boxes[0]);
    std::vector<RecordingLinkSub*> added;
    trigger->onUpdate = [this, &newTargets, &added] {
        for (auto target : newTargets) {
            added.push_back(addLink(target));
        }
        return false;
    };

    // Act
    App::PropertyLinkBase::updateAllElementReferences();

    // Assert
    EXPECT_EQ(*trigger->updateCount, 1);
    ASSERT_EQ(added.size(), newTargetCount);
    for (auto link : added) {
        EXPECT_EQ(*link->updateCount, 0);
    }
}
