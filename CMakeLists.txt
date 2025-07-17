cmake_minimum_required(VERSION 3.15)
project(Connector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "/usr/local/lib/cmake" ${CMAKE_PREFIX_PATH})

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

set(GLOABL_SOURCES 
    src/socket.cpp
    src/connector.cpp
    src/config_loader.cpp
)

set(RECV_SOURCES 
    src/server_socket.cpp
    ${GLOABL_SOURCES}
)

set(SEND_SOURCES 
    src/client_socket.cpp
    ${GLOABL_SOURCES}
)

add_library(connector_recv STATIC ${RECV_SOURCES})
target_compile_definitions(connector_recv PRIVATE RECV)
target_include_directories(connector_recv PUBLIC include)
target_link_libraries(connector_recv 
    PRIVATE 
        nlohmann_json::nlohmann_json 
        spdlog::spdlog
)

add_library(connector_send STATIC ${SEND_SOURCES})
target_compile_definitions(connector_send PRIVATE SEND)
target_include_directories(connector_send PUBLIC include)
target_link_libraries(connector_send 
    PRIVATE 
        nlohmann_json::nlohmann_json 
        spdlog::spdlog
)