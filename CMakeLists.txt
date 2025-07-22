cmake_minimum_required(VERSION 3.15)
project(Connector)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_PREFIX_PATH "/usr/local/lib/cmake" ${CMAKE_PREFIX_PATH})

find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)

set(GLOBAL_SOURCES 
    src/socket.cpp
    src/connector.cpp
    src/config_loader.cpp
)

set(RECV_SOURCES 
    src/recv_socket.cpp
    ${GLOBAL_SOURCES}
)

set(SEND_SOURCES 
    src/send_socket.cpp
    ${GLOBAL_SOURCES}
)

add_library(connector_recv STATIC ${RECV_SOURCES})
target_compile_definitions(connector_recv PUBLIC RECV)
target_include_directories(connector_recv PUBLIC include)
target_link_libraries(connector_recv 
    PRIVATE 
        nlohmann_json::nlohmann_json 
        spdlog::spdlog
)

add_library(connector_send STATIC ${SEND_SOURCES})
target_compile_definitions(connector_send PUBLIC SEND)
target_include_directories(connector_send PUBLIC include)
target_link_libraries(connector_send 
    PRIVATE 
        nlohmann_json::nlohmann_json 
        spdlog::spdlog
)

add_executable(exm1 example1.cpp)
target_link_libraries(exm1 
    PRIVATE 
        connector_send
)

add_executable(exm2 example2.cpp)
target_link_libraries(exm2 
    PRIVATE 
        connector_recv
)