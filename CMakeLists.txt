cmake_minimum_required(VERSION 3.15)
project(Connector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "/usr/local/lib/cmake" ${CMAKE_PREFIX_PATH})

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

file(GLOB PROJECT_SOURCES src/*)

add_library(connector STATIC ${PROJECT_SOURCES})

target_include_directories(connector PUBLIC include)

target_link_libraries(connector 
    PRIVATE 
        nlohmann_json::nlohmann_json 
        spdlog::spdlog
)