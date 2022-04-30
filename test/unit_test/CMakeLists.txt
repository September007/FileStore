# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.

project ("TESTS")
include_directories(.)
# Include sub-projects.
set(test_deps FileStore::FileStore 
GTest::gtest 
spdlog::spdlog_header_only
)

parse_regex_path( test_sources "./*.cpp")
message("unit_test find test_sources:${test_sources}")
# message("tests:${tests_path}")
foreach(test ${test_sources})
    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" TestName  ${test})
    # avoid empty test_main
    if(${TestName} MATCHES "test_main")
        continue()
    endif()
    add_executable(${TestName} ${test} "./test_main.cpp" )
    target_link_libraries(${TestName} ${test_deps})
    message("add unit_test ${TestName}")
endforeach()
# 
list(LENGTH tests_path tests_count)
if(tests_count GREATER 1)# with test_main
    add_executable(all_tests ${test_sources} "./test_main.cpp" )
    target_link_libraries(all_tests ${test_deps} )
endif()