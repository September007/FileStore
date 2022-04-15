# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.

project ("benchmark")
include_directories(.)
# Include sub-projects.
set(dependencis FileStore::FileStore benchmark::benchmark benchmark::benchmark_main)

parse_regex_path( benchmark_sources "./*.cpp")
message("benchmark find benchmark sources:${benchmark_sources}")
# message("tests:${tests_path}")
foreach(test ${benchmark_sources})
    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" TestName  ${test})
    add_executable(${TestName} ${test} )
    target_link_libraries(${TestName} ${dependencis})
endforeach()

