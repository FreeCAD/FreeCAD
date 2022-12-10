#include "gtest/gtest.h"
#include "Base/Builder3D.h"
#include "Base/Builder3D.cpp"

TEST(Branding, FirstTest){
    EXPECT_EQ(1, 1) << "Dummy test";
}

TEST(Builder, one){
    Base::ColorRGB color {};
    EXPECT_EQ(color.blue(), 1.0F);
}
