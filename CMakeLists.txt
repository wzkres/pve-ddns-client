cmake_minimum_required(VERSION 3.8)
project(pve-ddns-client)

message("CMake version: " ${CMAKE_VERSION})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DHAVE_SNPRINTF)
#add_definitions(-DGLOG_STATIC)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.cc")

add_subdirectory(3rdparty/glog)
add_subdirectory(3rdparty/yaml-cpp)

add_executable(pve-ddns-client ${SOURCES})

target_include_directories(pve-ddns-client PRIVATE "3rdparty/cmdline")
target_link_libraries(pve-ddns-client PRIVATE glog::glog yaml-cpp)

add_custom_command(
    TARGET pve-ddns-client
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:glog> ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Copy glog lib to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
)

add_custom_command(
        TARGET pve-ddns-client
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/res/pve-ddns-client.yml ${CMAKE_CURRENT_BINARY_DIR}/pve-ddns-client.yml
        COMMENT "Copy config yaml file to ${CMAKE_CURRENT_BINARY_DIR} directory" VERBATIM
)

# Add tests and install targets if needed.
install(TARGETS pve-ddns-client DESTINATION bin)

# uninstall xargs rm < install_manifest.txt
