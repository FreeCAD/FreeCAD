// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <QRegularExpression>

#include <Gui/TreeSearchUtil.h>

namespace
{

bool matches(const QString& pattern, const QString& haystack)
{
    const auto re = Gui::TreeSearchUtil::wildcardToRegex(pattern);
    EXPECT_TRUE(re.isValid());
    return Gui::TreeSearchUtil::regexMatches(re, haystack);
}

}  // namespace

TEST(TreeSearchUtil, PlainTextSearchMatchesSubstrings)
{
    EXPECT_TRUE(matches(QStringLiteral("Cube"), QStringLiteral("Cube")));
    EXPECT_TRUE(matches(QStringLiteral("Cube"), QStringLiteral("Cube001")));
    EXPECT_TRUE(matches(QStringLiteral("Cube"), QStringLiteral("MyCube")));
}

TEST(TreeSearchUtil, MatchingIsCaseInsensitive)
{
    EXPECT_TRUE(matches(QStringLiteral("cube"), QStringLiteral("CUBE001")));
}

TEST(TreeSearchUtil, PrefixWildcardIsAnchored)
{
    EXPECT_TRUE(matches(QStringLiteral("Cube*"), QStringLiteral("Cube")));
    EXPECT_TRUE(matches(QStringLiteral("Cube*"), QStringLiteral("Cube001")));
    EXPECT_FALSE(matches(QStringLiteral("Cube*"), QStringLiteral("MyCube")));
}

TEST(TreeSearchUtil, SuffixWildcardIsAnchored)
{
    EXPECT_TRUE(matches(QStringLiteral("*Cube"), QStringLiteral("MyCube")));
    EXPECT_TRUE(matches(QStringLiteral("*Cube"), QStringLiteral("Cube")));
    EXPECT_FALSE(matches(QStringLiteral("*Cube"), QStringLiteral("Cube001Extra")));
}

TEST(TreeSearchUtil, MiddleWildcardIsAnchored)
{
    EXPECT_TRUE(matches(QStringLiteral("C*e"), QStringLiteral("Cube")));
    EXPECT_TRUE(matches(QStringLiteral("C*e"), QStringLiteral("C123e")));
    EXPECT_TRUE(matches(QStringLiteral("C*e"), QStringLiteral("Ce")));
    EXPECT_FALSE(matches(QStringLiteral("C*e"), QStringLiteral("ACube")));
}

TEST(TreeSearchUtil, QuestionMarkMatchesExactlyOneCharacter)
{
    EXPECT_TRUE(matches(QStringLiteral("Cube???"), QStringLiteral("Cube001")));
    EXPECT_FALSE(matches(QStringLiteral("Cube???"), QStringLiteral("Cube01")));
    EXPECT_FALSE(matches(QStringLiteral("Cube???"), QStringLiteral("Cube0001")));
}

TEST(TreeSearchUtil, SingleWildcardsHandleEmptyAndSingleCharacters)
{
    EXPECT_TRUE(matches(QStringLiteral("*"), QStringLiteral("")));
    EXPECT_TRUE(matches(QStringLiteral("*"), QStringLiteral("anything")));
    EXPECT_TRUE(matches(QStringLiteral("?"), QStringLiteral("A")));
    EXPECT_FALSE(matches(QStringLiteral("?"), QStringLiteral("")));
    EXPECT_FALSE(matches(QStringLiteral("?"), QStringLiteral("AB")));
}

TEST(TreeSearchUtil, StarAndQuestionMarkCanBeCombined)
{
    EXPECT_TRUE(matches(QStringLiteral("Part*??"), QStringLiteral("Part001")));
    EXPECT_TRUE(matches(QStringLiteral("P?rt*1"), QStringLiteral("Part001")));
    EXPECT_FALSE(matches(QStringLiteral("Part*??"), QStringLiteral("Part1")));
    EXPECT_FALSE(matches(QStringLiteral("P?rt*1"), QStringLiteral("Port002")));
}

TEST(TreeSearchUtil, RegexMetacharactersAreEscaped)
{
    EXPECT_TRUE(matches(QStringLiteral("a.b+c"), QStringLiteral("a.b+c")));
    EXPECT_FALSE(matches(QStringLiteral("a.b+c"), QStringLiteral("aXbXc")));
}

TEST(TreeSearchUtil, IncompleteRegexSyntaxIsTreatedLiterally)
{
    EXPECT_TRUE(matches(QStringLiteral("["), QStringLiteral("[")));
    EXPECT_FALSE(matches(QStringLiteral("["), QStringLiteral("x")));
}

TEST(TreeSearchUtil, EmptyPatternProducesValidRegex)
{
    const auto re = Gui::TreeSearchUtil::wildcardToRegex(QString {});

    EXPECT_TRUE(re.isValid());
}

TEST(TreeSearchUtil, RegexMatchesRejectsInvalidRegex)
{
    const QRegularExpression invalid(QStringLiteral("["));

    EXPECT_FALSE(invalid.isValid());
    EXPECT_FALSE(Gui::TreeSearchUtil::regexMatches(invalid, QStringLiteral("anything")));
}
