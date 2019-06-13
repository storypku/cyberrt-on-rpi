#include "gtest/gtest.h"
#include "main/hello-greet.h"
#include "cyber/common/log.h"

TEST(HelloGreetTest, get_greet) {
    EXPECT_EQ(get_greet("bazel"), "Hello bazel");
    // EXPECT_EQ(get_greet("a"), "blah");
    LOG(ERROR) << "1111";
}
