cmake_minimum_required(VERSION 3.15)
project(Connector VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionally extend search path for find_package
list(PREPEND CMAKE_PREFIX_PATH "/usr/local/lib/cmake")

# Enable ASan in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address -g -O1)
    add_link_options(-fsanitize=address)
endif()

# Find required packages
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

# Gather source files
file(GLOB PROJECT_SOURCES src/*.cpp)

# Define the library target
add_library(connector STATIC ${PROJECT_SOURCES})
target_include_directories(connector PUBLIC include)

# Link dependencies
target_link_libraries(connector
    PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
)

# Create an alias for modern CMake usage
add_library(Connector::connector ALIAS connector)

# Optional install step (for use in other projects)
include(GNUInstallDirs)

install(TARGETS connector
    EXPORT connectorTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT connectorTargets
    FILE ConnectorTargets.cmake
    NAMESPACE Connector::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/connector
)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ConnectorConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ConnectorConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ConnectorConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/connector
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ConnectorConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ConnectorConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/connector
)