# CMakeList.txt : CMake project for FileStore, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

aux_source_directory("." lib_sources)

add_library(FileStore  ${lib_sources})
target_include_directories(FileStore PUBLIC .)
add_library(FileStore::FileStore ALIAS FileStore)
# TODO: Add tests and install targets if needed.
