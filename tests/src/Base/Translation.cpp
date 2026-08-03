// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "Base/Translation.h"
#include "TranslationTestHelpers.h"

TEST(Translation, FallbackReturnsSource)
{
    Base::Translation::setTranslator(nullptr);
    EXPECT_EQ(std::string("Hello"), Base::Translation::translate("Ctx", "Hello"));
}

TEST(Translation, TranslateHandlerOverrides)
{
    Base::Translation::Test::RecordingTranslator translator;
    translator.translateMode = Base::Translation::Test::RecordingTranslator::TranslateMode::Constant;
    translator.constantTranslation = "Bonjour";

    Base::Translation::Test::ScopedTranslator scoped(&translator);
    EXPECT_EQ(std::string("Bonjour"), Base::Translation::translate("Ctx", "Hello"));

    EXPECT_EQ(translator.translateCalls, 1);
    EXPECT_EQ(translator.lastContext, "Ctx");
    EXPECT_EQ(translator.lastSourceText, "Hello");
    EXPECT_EQ(translator.lastDisambiguation, "");
    EXPECT_EQ(translator.lastN, -1);
}

TEST(Translation, InstallRemoveHandlersDefaultToFalse)
{
    Base::Translation::setTranslator(nullptr);
    EXPECT_FALSE(Base::Translation::installTranslator("some.qm"));
    EXPECT_FALSE(Base::Translation::removeTranslators({}));
}

TEST(Translation, InstallRemoveHandlersCalled)
{
    Base::Translation::Test::RecordingTranslator translator;
    translator.installResult = true;
    translator.removeResult = true;
    Base::Translation::Test::ScopedTranslator scoped(&translator);

    EXPECT_TRUE(Base::Translation::installTranslator("a.qm"));
    EXPECT_EQ(translator.installCalls, 1);
    EXPECT_EQ(translator.lastInstalledFilename, "a.qm");

    const std::vector<std::string> files {"a.qm", "b.qm"};
    EXPECT_TRUE(Base::Translation::removeTranslators(files));
    EXPECT_EQ(translator.removeCalls, 1);
    EXPECT_EQ(translator.lastRemovedFilenames, files);
}
