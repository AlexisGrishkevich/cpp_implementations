project(long_arithmetic VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_VISIBILITY_PRESET HIDDEN)
set(CMAKE_VISIBILITY_INLINES_HIDDEN HIDDEN)

add_library(${PROJECT_NAME}
    src/bigInteger.cpp
    src/rational.cpp
)

add_library(long_arithmetic::long_arithmetic ALIAS long_arithmetic)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    if(CMAKE_COMPILER_IS_GNUCXX)
        target_compile_options(${PROJECT_NAME} PRIVATE
              -Wall -Wextra -Wpedantic -pedantic-errors
              -Wctor-dtor-privacy -Wnon-virtual-dtor -Woverloaded-virtual
              -Wold-style-cast -Wcast-qual -Wfloat-equal -Wsign-promo
              -Wduplicated-cond -Wduplicated-branches
              -Wshadow=compatible-local -Wlogical-op -Wextra-semi
              -g -O0
        )
    endif()
endif()
