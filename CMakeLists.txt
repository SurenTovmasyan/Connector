cmake_minimum_required(VERSION 3.15)
project(Connector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "/usr/local/lib/cmake" ${CMAKE_PREFIX_PATH})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g -O1")
endif()

# Gather source files
file(GLOB PROJECT_SOURCES src/*)

# Create the connector library
add_library(connector STATIC ${PROJECT_SOURCES})
target_include_directories(connector PUBLIC include)

# Link libraries to connector
target_link_libraries(connector PRIVATE nlohmann_json::nlohmann_json spdlog::spdlog)

# Find used packages
# find_package(GTest REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# add_executable(main ${PROJECT_SOURCES} example.cpp)
