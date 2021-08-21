cmake_minimum_required(VERSION 3.8)

set(PROJECT_NAME pve-ddns-client)
project(${PROJECT_NAME})

message("CMake version: " ${CMAKE_VERSION})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DCURL_STATICLIB)
#add_definitions(-DHAVE_SNPRINTF)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cc")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE "3rdparty/cmdline" "3rdparty/prebuilt/include")
target_link_directories(${PROJECT_NAME} PRIVATE "3rdparty/prebuilt/lib" "3rdparty/prebuilt/lib64")
target_link_libraries(${PROJECT_NAME} PRIVATE curl glog ssl crypto yaml-cpp pthread dl)
#target_link_libraries(${PROJECT_NAME} PRIVATE glogd yaml-cppd libcurl-d wldap32 crypt32 Ws2_32)

add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/res/pve-ddns-client.yml ${CMAKE_CURRENT_BINARY_DIR}/pve-ddns-client.yml
        COMMENT "Copy config yaml file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
)

# Add tests and install targets if needed.
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# uninstall xargs rm < install_manifest.txt
