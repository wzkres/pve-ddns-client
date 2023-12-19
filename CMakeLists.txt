cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME pve-ddns-client)
project(${PROJECT_NAME})

message("CMake version: " ${CMAKE_VERSION})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(MIPS_TARGET "Building for mips target" OFF)

add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DCURL_STATICLIB)
if(WIN32)
    add_definitions(-DHAVE_SNPRINTF)
endif()
if(DEFINED PVE_DDNS_CLIENT_VER)
    add_definitions(-DPVE_DDNS_CLIENT_VER="${PVE_DDNS_CLIENT_VER}")
endif()

if(APPLE)
    find_library(LIB_FOUNDATION Foundation)
    if (NOT LIB_FOUNDATION)
        message(FATAL_ERROR "Lib Foundation not found!")
    endif()

    find_library(LIB_SYSTEMCONFIGURATION SystemConfiguration)
    if (NOT LIB_SYSTEMCONFIGURATION)
        message(FATAL_ERROR "Lib SystemConfiguration not found")
    endif()
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cc")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_SOURCE_DIR}/3rdparty/cmdline"
        "${CMAKE_SOURCE_DIR}/3rdparty/fmt/include"
        "${CMAKE_SOURCE_DIR}/3rdparty/rapidjson/include"
        "${CMAKE_SOURCE_DIR}/3rdparty/prebuilt/include")

target_link_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdparty/prebuilt/lib")
if(EXISTS ${CMAKE_SOURCE_DIR}/3rdparty/prebuilt/lib64)
    target_link_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/3rdparty/prebuilt/lib64")
endif()

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(${PROJECT_NAME} PRIVATE libcurl-d glogd yaml-cppd)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE libcurl glog yaml-cpp)
    endif()
    target_link_libraries(${PROJECT_NAME} PRIVATE wldap32 crypt32 Ws2_32)
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE curl glog yaml-cpp ssl crypto z pthread dl
        ${LIB_FOUNDATION} ${LIB_SYSTEMCONFIGURATION})
else()
    if(MIPS_TARGET)
        target_link_libraries(${PROJECT_NAME} PRIVATE curl glog yaml-cpp ssl crypto pthread dl)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE curl glog yaml-cpp ssl crypto z pthread dl)
    endif()
endif()

add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/res/pve-ddns-client.yml
                                         ${CMAKE_CURRENT_BINARY_DIR}/pve-ddns-client.yml
        COMMENT "Copy config yaml file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
)

# Add tests and install targets if needed.
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# uninstall xargs rm < install_manifest.txt
