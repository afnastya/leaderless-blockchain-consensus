cmake_minimum_required(VERSION 3.2)
set(PROJECT_NAME "blockchain")
project(${PROJECT_NAME})

find_package(Boost COMPONENTS program_options fiber system thread)
include_directories( ${Boost_INCLUDE_DIRS} )

find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(GTest 1.13.0 REQUIRED)
find_package (glog 0.6.0 REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -O2 -Wall -Wextra -fsanitize=address")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)