# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()
#set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH};$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}/share)
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    message("set vcpkg as env says $ENV{VCPKG_DEFAULT_TRIPLET}")
    set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
else()
    message("set vcpkg as cache says ${VCPKG_TARGET_TRIPLET}")
endif()

# parse regex_path
function( parse_regex_path return_set regex_path)
    set(regex_path ${regex_path} ${ARGN})
    file(GLOB SourceFiles ${regex_path})
    set(${return_set} )
    foreach(whom ${SourceFiles})
        STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME  ${whom})
        set(${return_set} ${${return_set}} ${whom})
    endforeach()
    set(${return_set} ${${return_set}} CACHE STRING "seems like only cache varible can stay alive out of function scope" )
endfunction()


project ("FileStore")

find_package(GTest CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED) 
find_package(RocksDB CONFIG REQUIRED) 
find_package(benchmark CONFIG REQUIRED)

include_directories( ./test ./src ./src/deps "$ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")
message("add include vcpkg dirs for intelli: $ENV{VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include")

# Include sub-projects.
add_subdirectory ("FileStore")
add_subdirectory ("test/unit_test")
add_subdirectory ("test/benchmark")
