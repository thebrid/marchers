#include <gtest/gtest.h>

#include "march.h"

TEST(MarchTest, ReturnsOne) {
    EXPECT_EQ(march(), 1);
}
