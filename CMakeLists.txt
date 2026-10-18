cmake_minimum_required(VERSION 3.16)

project(lithub LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(lithub
	src/main.cc
	
	src/commands/LitCommands.cc
	
	src/repository/LitConfig.cc
	src/repository/LitRepository.cc
	src/repository/LitRefs.cc
	
	src/objects/LitBlob.cc
    src/objects/LitCommit.cc
    src/objects/LitTree.cc
    src/objects/LitKvlm.cc
    src/objects/LitObjectStore.cc

    src/index/LitIndex.cc
)

target_include_directories(lithub PRIVATE
	src
	external/argparse/include
)

target_link_libraries(lithub PRIVATE
    OpenSSL::Crypto
    ZLIB::ZLIB
)

target_compile_options(lithub PRIVATE
    -Wall
    -Wextra
    -pedantic
)