cmake_minimum_required(VERSION 3.22.1 FATAL_ERROR)

project(function)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Use build directory different from source directory")
endif()

if(CMAKE_CONFIGURATION_TYPES)
    if(NOT "Profile" IN_LIST CMAKE_CONFIGURATION_TYPES)
        list(APPEND CMAKE_CONFIGURATION_TYPES Profile)
    endif()
else()
    set(allowableBildTypes Debug Release Profile)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${allowableBildTypes}")
    if(NOT CMAKE_BUILD_TYPE)
        message(STATUS "Setting build type to 'Debug' as none was specified.")
        set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
    elseif(NOT CMAKE_BUILD_TYPE IN_LIST allowableBildTypes)
        message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
    endif()
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} \
    -D_GLIBCXX_DEBUG  -D_GLIBCXX_DEBUG_PEDANTIC \
    -fsanitize=undefined \ -fsanitize=address -fstack-protector"
    CACHE STRING "Set C++ Compiler Flags" FORCE
)

# generates a compile_commands.json which containing the exact compiler commands
# needed for LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(GCC_LIKE_CXX "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(MSVC_CXX "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/function.h)

add_executable(
    unit_test
    test/unit_test.cpp
)

target_include_directories(unit_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_include_directories(unit_test PRIVATE
    ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(unit_test PRIVATE
    ${GTEST_BOTH_LIBRARIES}
)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    if(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(
            ${PROJECT_NAME} PRIVATE
              -Wall -Wextra -Wpedantic -pedantic-errors
              -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual
              -Wold-style-cast -Wcast-qual -Wfloat-equal -Wsign-promo
              -Wduplicated-cond -Wduplicated-branches
              -Wshadow=compatible-local -Wlogical-op -Wextra-semi
              -g -O0
        )
    endif()
endif()
