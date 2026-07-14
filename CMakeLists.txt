cmake_minimum_required(VERSION 3.20)

project(SignalingServer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    set(USE_OPENSSL ON CACHE BOOL "" FORCE)
    set(USE_GNUTLS OFF CACHE BOOL "" FORCE)
else()
    set(USE_GNUTLS ON CACHE BOOL "" FORCE)
    set(USE_OPENSSL OFF CACHE BOOL "" FORCE)
endif()

set(USE_LIBWEBSOCKETS ON CACHE BOOL "" FORCE)
set(BUILD_WEBSOCKET ON CACHE BOOL "" FORCE)

set(RTC_ENABLE_MEDIA OFF CACHE BOOL "" FORCE)

set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
set(NO_TESTS ON CACHE BOOL "" FORCE)

add_subdirectory(ext/libdatachannel)

add_library(nlohmann INTERFACE)
target_include_directories(nlohmann INTERFACE
    ext/json
)

add_executable(SignalingServer
    src/main.cpp
    src/SignalingServer.cpp
)

target_include_directories(SignalingServer PRIVATE
    src
)

target_link_libraries(SignalingServer PRIVATE
    LibDataChannel::LibDataChannel
    nlohmann
)

if(UNIX)
    target_link_libraries(SignalingServer PRIVATE pthread)
endif()