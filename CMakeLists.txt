cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME pve-ddns-client)
project(${PROJECT_NAME})

message("CMake version: " ${CMAKE_VERSION})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-DCURL_STATICLIB)
if(WIN32)
    add_definitions(-DHAVE_SNPRINTF)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cc")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
        "3rdparty/cmdline" "3rdparty/fmt/include" "3rdparty/rapidjson/include" "3rdparty/prebuilt/include")
target_link_directories(${PROJECT_NAME} PRIVATE "3rdparty/prebuilt/lib" "3rdparty/prebuilt/lib64")
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE libcurl-d glogd yaml-cppd wldap32 crypt32 Ws2_32)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE curl glog yaml-cpp ssl crypto z pthread dl)
endif()


add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/res/pve-ddns-client.yml ${CMAKE_CURRENT_BINARY_DIR}/pve-ddns-client.yml
        COMMENT "Copy config yaml file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
)

# Add tests and install targets if needed.
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# uninstall xargs rm < install_manifest.txt
