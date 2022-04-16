#
cmake_minimum_required (VERSION 3.8)

set(filestore_header_dir "./headers")
include_directories(${filestore_header_dir})
aux_source_directory("." lib_sources)
set( dependencies fmt::fmt-header-only RocksDB::rocksdb)
add_library(FileStore  ${lib_sources})
target_link_libraries(FileStore ${dependencies} )
target_include_directories(FileStore PUBLIC ${filestore_header_dir})
add_library(FileStore::FileStore ALIAS FileStore)


