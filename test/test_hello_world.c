#include "unity.h"

void test_hello_world(void) {
    TEST_ASSERT_EQUAL_STRING("Hello, World!", "Hello, World!");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hello_world);
    return UNITY_END();
}