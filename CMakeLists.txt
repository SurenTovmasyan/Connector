cmake_minimum_required(VERSION 3.15)
project(Connector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "/usr/local/lib/cmake" ${CMAKE_PREFIX_PATH})

find_package(spdlog REQUIRED)

file(GLOB PROJECT_SOURCES src/*)

add_library(connector STATIC ${PROJECT_SOURCES})

target_include_directories(connector PUBLIC include)

target_link_libraries(connector 
    PRIVATE 
        spdlog::spdlog
)

# add_executable(exm1 tests/example1.cpp)
# target_link_libraries(exm1 PRIVATE connector)

# add_executable(exm2 tests/example2.cpp)
# target_link_libraries(exm2 PRIVATE connector)