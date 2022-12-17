#include "function.h"

#include <gtest/gtest.h>


int foo(int x) {
  return x * 10;
}


struct S final {
  int bar(int x) {
    return x;
  }
};


struct Functor final {
  int operator()(int x) {
    return x * 42;
  }
};


// function //----------------------------------------------------------//
TEST(Function, initialization_from_function) {
  Function<int(int)> function{foo};

  ASSERT_EQ(function(2), 20);
}

TEST(Function, initialization_from_function_move) {
  Function<int(int)> function{std::move(foo)};

  ASSERT_EQ(function(2), 20);
}

TEST(Function, copy_assignment_from_function) {
  Function<int(int)> function;
  function = foo;

  ASSERT_EQ(function(2), 20);
}

TEST(Function, move_assignment_from_function) {
  Function<int(int)> function;
  function = std::move(foo);

  ASSERT_EQ(function(2), 20);
}
//----------------------------------------------------------------------//


// lambda //------------------------------------------------------------//
TEST(Function, initialization_from_lambda) {
  auto lambda = [](int x) { return x * 5; };
  Function<int(int)> function{lambda};

  ASSERT_EQ(function(2), 10);
}

TEST(Function, initialization_from_lambda_move) {
  auto lambda = [](int x) { return x * 5; };
  Function<int(int)> function{std::move(lambda)};

  ASSERT_EQ(function(2), 10);
}

TEST(Function, copy_assignment_from_lambda) {
  auto lambda = [](int x) { return x * 5; };
  Function<int(int)> function;
  function = lambda;

  ASSERT_EQ(function(2), 10);
}

TEST(Function, move_assignment_from_lambda) {
  auto lambda = [](int x) { return x * 5; };
  Function<int(int)> function;
  function = lambda;

  ASSERT_EQ(function(2), 10);
}
//----------------------------------------------------------------------//


// functor //-----------------------------------------------------------//
TEST(Function, initialization_from_functor) {
  Functor functor;
  Function<int(int)> function{functor};

  ASSERT_EQ(function(1), 42);
}

TEST(Function, initialization_from_functor_move) {
  Functor functor;
  Function<int(int)> function{std::move(functor)};

  ASSERT_EQ(function(1), 42);
}

TEST(Function, copy_assignment_from_functor) {
  Functor functor;
  Function<int(int)> function;
  function = functor;

  ASSERT_EQ(function(1), 42);
}

TEST(Function, move_assignment_from_functor) {
  Functor functor;
  Function<int(int)> function;
  function = functor;

  ASSERT_EQ(function(1), 42);
}
//----------------------------------------------------------------------//


int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
