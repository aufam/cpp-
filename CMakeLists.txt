cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# project settings
project(cppxx
    VERSION     0.1.0
    LANGUAGES   C CXX
)

option(CPPXX_BUILD_CMD    "Build the executable tool"                   OFF)
option(CPPXX_BUILD_TESTS  "Build unit tests"                            OFF)
option(CPPXX_REQUIREMENTS "Only install the requirements for the cmd"   OFF)

message(STATUS "CPPXX_VERSION       : ${cppxx_VERSION}")
message(STATUS "CPPXX_BUILD_CMD     : ${CPPXX_BUILD_CMD}")
message(STATUS "CPPXX_BUILD_TESTS   : ${CPPXX_BUILD_TESTS}")
message(STATUS "CPPXX_REQUIREMENTS  : ${CPPXX_REQUIREMENTS}")


# core interface
add_library(cppxx INTERFACE)
add_library(cppxx::cppxx ALIAS cppxx)

target_include_directories(cppxx INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if (NOT CPPXX_BUILD_CMD AND NOT CPPXX_BUILD_TESTS AND NOT CPPXX_REQUIREMENTS)
    return()
endif()


# system libraries
find_package(SQLite3 REQUIRED)


# external libraries
include(cmake/CPM.cmake)

## String formatter using fmtlib
CPMAddPackage(
    NAME            fmt
    GIT_REPOSITORY  "https://github.com/fmtlib/fmt"
    GIT_TAG         11.2.0
    COMPONENTS      fmt
)

## Logging
CPMAddPackage(
    NAME            spdlog
    GIT_REPOSITORY  "https://github.com/gabime/spdlog"
    GIT_TAG         v1.15.3
    OPTIONS         "SPDLOG_FMT_EXTERNAL ON"
    COMPONENTS      spdlog
)

## JSON lib
CPMAddPackage(
    NAME            nlohmann_json
    URL             "https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz"
)

## Faster JSON lib
CPMAddPackage(
    NAME            yyjson
    GIT_REPOSITORY  "https://github.com/ibireme/yyjson"
    GIT_TAG         0.12.0
    OPTIONS         "YYJSON_INSTALL OFF"
)

## toml lib
CPMAddPackage(
    NAME            tomlplusplus
    GIT_REPOSITORY  "https://github.com/marzer/tomlplusplus"
    GIT_TAG         v3.4.0
)

## Boost's reflection lib
CPMAddPackage(
    NAME            pfr
    GIT_REPOSITORY  "https://github.com/boostorg/pfr"
    GIT_TAG         boost-1.88.0
    OPTIONS         "BOOST_USE_MODULES OFF"
    COMPONENTS      Boost::pfr
)

## Reflection for enum
CPMAddPackage(
    NAME            magic_enum
    GIT_REPOSITORY  "https://github.com/Neargye/magic_enum"
    GIT_TAG         v0.9.7
)

## Option parser
CPMAddPackage(
    NAME            cxxopts
    GIT_REPOSITORY  "https://github.com/jarro2783/cxxopts"
    GIT_TAG         v3.2.1
)

## sha256
CPMAddPackage(
    NAME            sha256
    GIT_REPOSITORY  "https://github.com/LekKit/sha256"
    GIT_TAG         master
)
add_library(sha256 STATIC "${sha256_SOURCE_DIR}/sha256.c")
target_include_directories(sha256 PUBLIC "${sha256_SOURCE_DIR}")

## http lib
CPMAddPackage(
    NAME            httplib
    GIT_REPOSITORY  "https://github.com/yhirose/cpp-httplib"
    GIT_TAG         v0.26.0
    OPTIONS         "HTTPLIB_COMPILE ON" "HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF" "HTTPLIB_REQUIRE_OPENSSL ON" "HTTPLIB_INSTALL OFF"
)
set_target_properties(httplib PROPERTIES POSITION_INDEPENDENT_CODE ON)
set(HTTPLIB_LICENSE "${httplib_SOURCE_DIR}/LICENSE")

## Tests using GoogleTest
CPMAddPackage(
    NAME            googletest
    GIT_REPOSITORY  "https://github.com/google/googletest"
    GIT_TAG         v1.15.2
    OPTIONS         "INSTALL_GTEST OFF"
    COMPONENTS      gtest_main
)

# only setup the exeternal libraries
if (CPPXX_REQUIREMENTS)
    return()
endif()


# private components
add_library(cppxx_private INTERFACE)
add_library(cppxx::private ALIAS cppxx_private)

target_link_libraries(cppxx_private INTERFACE
    fmt
    spdlog
    nlohmann_json
    yyjson
    tomlplusplus::tomlplusplus
    Boost::pfr
    magic_enum
    cxxopts
    sha256
)

target_compile_options(cppxx_private INTERFACE
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

target_compile_options(cppxx_private INTERFACE
    -Wall
    -Wextra
    -fmacro-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}=cpp++
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-redundant-move>
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

target_link_options(cppxx_private INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)


# executable
if (CPPXX_BUILD_CMD)
    file(GLOB_RECURSE CMD_SOURCES cmd/*)
    add_executable(cmd ${CMD_SOURCES})
    target_compile_features(cmd PRIVATE cxx_std_17)
    set_target_properties(cmd PROPERTIES OUTPUT_NAME cpp++)

    target_link_libraries(cmd PRIVATE
        cppxx
        cppxx_private
    )

    target_link_options(cmd PRIVATE
        $<$<CONFIG:Release>:-static>
    )
endif()


# tests
if (CPPXX_BUILD_TESTS)
    file(GLOB_RECURSE TEST_SOURCES tests/*)
    add_executable(test_all ${TEST_SOURCES})
    target_compile_features(test_all PRIVATE cxx_std_17)

    target_link_libraries(test_all PRIVATE
        cppxx
        cppxx_private
        gtest_main
        httplib
        SQLite::SQLite3
    )

	enable_testing()
	add_test(NAME test_all COMMAND test_all)
endif()
