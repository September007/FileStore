# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.

project ("benchmark")
include_directories(.)
# Include sub-projects.
set(dependencis   benchmark::benchmark benchmark::benchmark_main)

parse_regex_path( benchmark_sources "./*.cpp")
message("benchmark find benchmark sources:${benchmark_sources}")
# message("tests:${tests_path}")
foreach(test ${benchmark_sources})
    STRING(REGEX REPLACE ".+/(.+)\\..*" "\\1" TestName  ${test})
    add_executable(${TestName} ${test})
    #link_directories(${TestName} ./FileStore)
    #TARGET_LINK_LIBRARIES(${TestName} ${dependencis} FileStore::FileStore)
    target_link_libraries(${TestName} ${dependencis} FileStore::FileStore)
    message("add bench ${TestName}")
    if (MSVC)
        set_target_properties(${TestName} PROPERTIES LINK_FLAGS "/PROFILE")
    endif()
endforeach()
