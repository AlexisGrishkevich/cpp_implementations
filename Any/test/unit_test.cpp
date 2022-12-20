#include "any.h"

#include <gtest/gtest.h>

#include <iostream>
#include <string>
#include <typeinfo>


TEST(Any, has_value) {
  Any any;

  ASSERT_FALSE(any.has_value());

  any = 5;
  ASSERT_TRUE(any.has_value());
}

TEST(Any, copy_constuctor_from_any) {
  Any any(5);
  ASSERT_TRUE(any.has_value());

  Any any2(any);
  ASSERT_TRUE(any2.has_value());
  ASSERT_TRUE(any.has_value());
}

TEST(Any, move_constuctor_from_any) {
  Any any(5);
  ASSERT_TRUE(any.has_value());

  Any any2(std::move(any));
  ASSERT_TRUE(any2.has_value());
  ASSERT_FALSE(any.has_value());
}

TEST(Any, move_constuctor_from_value) {
  std::string str{"for any"};
  ASSERT_EQ(str.length(), 7);

  Any any{std::move(str)};
  ASSERT_TRUE(any.has_value());
  ASSERT_EQ(str.length(), 0);
}

TEST(Any, copy_assignment_from_any) {
  Any any(5);
  ASSERT_TRUE(any.has_value());

  Any any2;
  ASSERT_FALSE(any2.has_value());

  any2 = any;
  ASSERT_TRUE(any2.has_value());
  ASSERT_TRUE(any.has_value());
}

TEST(Any, move_assignment_from_any) {
  Any any(5);
  ASSERT_TRUE(any.has_value());

  Any any2;
  ASSERT_FALSE(any2.has_value());

  any2 = std::move(any);
  ASSERT_TRUE(any2.has_value());
  ASSERT_FALSE(any.has_value());
}

TEST(Any, move_assignment_from_value) {
  std::string str{"for any"};
  ASSERT_EQ(str.length(), 7);

  Any any;
  ASSERT_FALSE(any.has_value());

  any = std::move(str);
  ASSERT_TRUE(any.has_value());
  ASSERT_EQ(str.length(), 0);
}

TEST(Any, any_cast) {
  Any any(5);
  ASSERT_TRUE(any_cast<int>(any));
  EXPECT_THROW(any_cast<double>(any), details::bad_any_cast);
}

TEST(Any, type) {
  Any any(5);
  ASSERT_EQ(any.type(), typeid(int));
}

TEST(Any, emplace) {
  Any any;
  any.emplace<int>(5);
  ASSERT_TRUE(any.has_value());
  ASSERT_EQ(any.type(), typeid(int));
  ASSERT_EQ(any_cast<int>(any), 5);
}

TEST(Any, swap) {
  Any any1(5);
  ASSERT_EQ(any_cast<int>(any1), 5);

  Any any2(10);
  ASSERT_EQ(any_cast<int>(any2), 10);

  swap(any1, any2);
  ASSERT_EQ(any_cast<int>(any1), 10);
  ASSERT_EQ(any_cast<int>(any2), 5);
}

TEST(Any, reset) {
  Any any(5);
  ASSERT_EQ(any_cast<int>(any), 5);

  any.reset();
  ASSERT_FALSE(any.has_value());

}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
