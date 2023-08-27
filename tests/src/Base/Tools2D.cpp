#include "gtest/gtest.h"
#include <Base/Tools2D.h>

class Line2D: public ::testing::Test
{
protected:
    Line2D()
        : pt1(0.0, 0.0)
        , pt2(3.0, 4.0)
    {}
    void SetUp() override
    {}
    void TearDown() override
    {}
    Base::Vector2d GetFirst() const
    {
        return pt1;
    }
    Base::Vector2d GetSecond() const
    {
        return pt2;
    }
    Base::Line2d GetLine() const
    {
        Base::Line2d line(pt1, pt2);
        return line;
    }

private:
    Base::Vector2d pt1, pt2;
};

TEST_F(Line2D, TestLength)
{
    Base::Line2d line(GetLine());
    EXPECT_DOUBLE_EQ(line.Length(), 5.0);
}

TEST_F(Line2D, TestPoints)
{
    Base::Line2d line(GetLine());
    EXPECT_EQ(line.clV1, GetFirst());
    EXPECT_EQ(line.clV2, GetSecond());
}

TEST_F(Line2D, TestFromPos)
{
    Base::Line2d line(GetLine());
    EXPECT_EQ(line.FromPos(2.5), Base::Vector2d(1.5, 2.0));
}

TEST_F(Line2D, TestContains)
{
    Base::Line2d line(GetLine());
    EXPECT_EQ(line.Contains(Base::Vector2d(1.5, 2.0)), true);
}

TEST(Polygon2D, TestDefault)
{
    Base::Polygon2d poly;
    EXPECT_EQ(poly.GetCtVectors(), 0);
}

TEST(Polygon2D, TestAdd)
{
    Base::Polygon2d poly;
    poly.Add(Base::Vector2d());
    EXPECT_EQ(poly.GetCtVectors(), 1);
}

TEST(Polygon2D, TestRemove)
{
    Base::Polygon2d poly;
    poly.Add(Base::Vector2d());
    EXPECT_EQ(poly.GetCtVectors(), 1);
    EXPECT_EQ(poly.Delete(1), false);
    EXPECT_EQ(poly.Delete(0), true);
    EXPECT_EQ(poly.GetCtVectors(), 0);
}

TEST(Polygon2D, TestClear)
{
    Base::Polygon2d poly;
    poly.Add(Base::Vector2d());
    poly.DeleteAll();
    EXPECT_EQ(poly.GetCtVectors(), 0);
}
