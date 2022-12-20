#include "variant.h"

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

#include <gtest/gtest.h>


struct Foo final {
  int value_{0};
  std::string str_{"Unit-Test"};

  Foo() = default;
  Foo(int value, std::string str) : value_{value}, str_{std::move(str)}
  { }
};


struct FooWithException final {
  int value_{0};
  std::string str_{"Unit-Test"};

  FooWithException() = default;
  FooWithException(int value, std::string str)
      : value_{value}, str_{std::move(str)} {
    throw std::runtime_error("check variant functions");
  }
};


struct BarWithException final {
  int value_{0};
  std::string str_{"Unit-Test"};

  BarWithException() = default;
  BarWithException(int value, std::string str)
      : value_{value}, str_{std::move(str)}
  { }

  BarWithException& operator=(const BarWithException& rhs) {
    value_ = rhs.value_;
    throw std::runtime_error("Ha-ha-ha!!!");
    str_ = rhs.str_;
    return *this;
  }
};

// constructors from values //------------------------------------------//
TEST(Variant, default_constructor) {
  Variant<int, double, std::string> variant;

  ASSERT_EQ(variant.index(), 0);
  testing::StaticAssertTypeEq<decltype(get<0>(variant)), int&>();
}

TEST(Variant, constructor_from_float) {
  Variant<int, double, std::string> variant(5.0f);

  ASSERT_EQ(variant.index(), 1);
  testing::StaticAssertTypeEq<decltype(get<1>(variant)), double&>();
}

TEST(Variant, constructor_from_pointer_one) {
  char symbol{'c'};
  std::variant<int*, const int*, char*> variant(&symbol);

  ASSERT_EQ(variant.index(), 2);
  testing::StaticAssertTypeEq<decltype(get<2>(variant)), char*&>();
}

TEST(Variant, constructor_from_pointer_two) {
  const int c{42};
  Variant<int*, const int*, char*> variant(&c);

  ASSERT_EQ(variant.index(), 1);
  testing::StaticAssertTypeEq<decltype(get<1>(variant)), const int*&>();
}

TEST(Variant, copy_constructor_from_value) {
  Foo foo;
  ASSERT_FALSE(foo.str_.empty());

  Variant<int, double, std::string, Foo> variant(foo);
  ASSERT_EQ(variant.index(), 3);
  testing::StaticAssertTypeEq<decltype(get<3>(variant)), Foo&>();
  ASSERT_FALSE(foo.str_.empty());
}

TEST(Variant, move_constructor_from_value) {
  Foo foo;
  ASSERT_FALSE(foo.str_.empty());

  Variant<int, double, std::string, Foo> variant(std::move(foo));

  ASSERT_EQ(variant.index(), 3);
  testing::StaticAssertTypeEq<decltype(get<3>(variant)), Foo&>();
  ASSERT_TRUE(foo.str_.empty());
}
//----------------------------------------------------------------------//


// assignment operator from values //------------------------------------//
TEST(Variant, copy_assignment_operator_from_value) {
  Foo foo(42, "Snowbording");
  Variant<int, double, std::string, Foo> variant("string");
  ASSERT_EQ(variant.index(), 2);

  variant = foo;
  ASSERT_EQ(variant.index(), 3);
}

TEST(Variant, move_assignment_operator_from_value) {
  Foo foo(42, "Snowbording");
  ASSERT_FALSE(foo.str_.empty());
  Variant<int, double, std::string, Foo> variant("string");
  ASSERT_EQ(variant.index(), 2);

  variant = std::move(foo);
  ASSERT_EQ(variant.index(), 3);
  ASSERT_TRUE(foo.str_.empty());
}
//----------------------------------------------------------------------//


// constructors from another variant //---------------------------------//
TEST(Variant, copy_constructor_from_variant) {
  Variant<int, double, std::string> variant1("Snowbording");
  ASSERT_EQ(variant1.index(), 2);

  Variant<int, double, std::string> variant2(variant1);
  ASSERT_EQ(variant2.index(), 2);
}

TEST(Variant, move_constructor_from_variant) {
  std::string str{"Snowbording"};
  Variant<int, double, std::string> variant1(str);
  ASSERT_EQ(variant1.index(), 2);
  ASSERT_FALSE(str.empty());

  Variant<int, double, std::string> variant2(std::move(variant1));
  ASSERT_EQ(variant2.index(), 2);
  ASSERT_TRUE(get<2>(variant1).empty());
}
//----------------------------------------------------------------------//


// assignment operator from another variant //--------------------------//
TEST(Variant, copy_assignment_operator_from_variant) {
  Foo foo(1, "Skiing");
  Variant<int, double, std::string, Foo> variant1(foo);
  ASSERT_EQ(variant1.index(), 3);
  ASSERT_FALSE(get<3>(variant1).str_.empty());

  Variant<int, double, std::string, Foo> variant2;
  ASSERT_EQ(variant2.index(), 0);

  variant2 = variant1;
  ASSERT_EQ(variant1.index(), 3);
  ASSERT_EQ(get<3>(variant2).value_, 1);
  ASSERT_FALSE(get<3>(variant1).str_.empty());
}

TEST(Variant, move_assignment_operator_from_variant) {
  Foo foo(1, "Skiing");
  Variant<int, double, std::string, Foo> variant1(foo);
  ASSERT_EQ(variant1.index(), 3);
  ASSERT_FALSE(get<3>(variant1).str_.empty());

  Variant<int, double, std::string, Foo> variant2;
  ASSERT_EQ(variant2.index(), 0);

  variant2 = std::move(variant1);
  ASSERT_EQ(variant1.index(), 3);
  ASSERT_EQ(get<3>(variant2).value_, 1);
  ASSERT_TRUE(get<3>(variant1).str_.empty());
}

TEST(Variant, copy_assignment_operator_from_variant_with_exception) {
  BarWithException foo1(1, "Skiing");
  BarWithException foo2(42, "Snowbording");

  Variant<int, double, std::string, BarWithException> variant1(foo1);
  ASSERT_EQ(variant1.index(), 3);
  ASSERT_EQ(get<3>(variant1).value_, 1);
  ASSERT_EQ(get<3>(variant1).str_.length(), 6);
  ASSERT_FALSE(variant1.valueless_by_exception());

  Variant<int, double, std::string, BarWithException> variant2(foo2);
  ASSERT_EQ(variant2.index(), 3);
  ASSERT_EQ(get<3>(variant2).value_, 42);
  ASSERT_EQ(get<3>(variant2).str_.length(), 11);

  EXPECT_THROW(variant2 = variant1, std::runtime_error);
  ASSERT_EQ(variant2.index(), details::variant_npos);
  EXPECT_THROW(get<3>(variant2), details::bad_variant_access);
}
//----------------------------------------------------------------------//

// function holds_alternative() //--------------------------------------//
TEST(Variant, holds_alternative) {
  Foo foo;
  Variant<int, double, std::string, Foo> variant(foo);

  ASSERT_EQ(variant.index(), 3);
  ASSERT_TRUE(holds_alternative<Foo>(variant));
  ASSERT_FALSE(holds_alternative<int>(variant));
}
//----------------------------------------------------------------------//


// function get() from index//------------------------------------------//
TEST(Variant, get_from_index_return_const_lvalue_ref) {
  const Variant<int, double, std::string> variant("Unit-Test");

  ASSERT_EQ(variant.index(), 2);
  testing::StaticAssertTypeEq<decltype(get<2>(variant)), const std::string&>();
  EXPECT_THROW(get<0>(variant), details::bad_variant_access);
}

TEST(Variant, get_from_index_return_rvalue_ref) {
  Variant<int, double, std::string> variant("Unit-Test");

  ASSERT_EQ(variant.index(), 2);
  testing::StaticAssertTypeEq<decltype(get<2>(std::move(variant))),
                                       std::string&&>();
}

TEST(Variant, get_from_index_return_const_rvalue_ref) {
  const Variant<int, double, std::string> variant("Unit-Test");

  ASSERT_EQ(variant.index(), 2);
  testing::StaticAssertTypeEq<decltype(get<2>(std::move(variant))),
                                       const std::string&&>();
}
//----------------------------------------------------------------------//


// function get() from type //--------------------------------------------//
TEST(Variant, get_from_type_return_lvalue_ref) {
  Variant<int, double, std::string> variant("Unit-Test");

  ASSERT_EQ(variant.index(), 2);
  testing::StaticAssertTypeEq<decltype(get<std::string>(variant)),
                                       std::string&>();
  EXPECT_THROW(get<int>(variant), details::bad_variant_access);
}

TEST(Variant, get_from_type_return_const_lvalue_ref) {
  const Variant<int, double, std::string> variant("Unit-Test");

  ASSERT_EQ(variant.index(), 2);
  testing::StaticAssertTypeEq<decltype(get<std::string>(variant)),
                                       const std::string&>();
}

TEST(Variant, get_from_type_return_rvalue_ref) {
  Variant<int, double, std::string> variant("Unit-Test");

  ASSERT_EQ(variant.index(), 2);
  testing::StaticAssertTypeEq<decltype(get<std::string>(std::move(variant))),
                                       std::string&&>();
}

TEST(Variant, get_from_type_return_const_rvalue_ref) {
  const Variant<int, double, std::string> variant("Unit-Test");

  ASSERT_EQ(variant.index(), 2);
  testing::StaticAssertTypeEq<decltype(get<std::string>(std::move(variant))),
                                       const std::string&&>();
  ASSERT_EQ(get<2>(variant).length(), 9);
}
//----------------------------------------------------------------------//


// functions emplace() and valueless_by_exception() //------------------//
TEST(Variant, emplace) {
  Variant<int, double, std::string, FooWithException> variant;
  testing::StaticAssertTypeEq<decltype(get<0>(variant)), int&>();
  ASSERT_FALSE(variant.valueless_by_exception());

  EXPECT_THROW(variant.emplace<FooWithException>(42, "Snowbording!!!"),
               std::runtime_error);

  ASSERT_EQ(variant.index(), details::variant_npos);
  ASSERT_TRUE(variant.valueless_by_exception());
}
//----------------------------------------------------------------------//


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
