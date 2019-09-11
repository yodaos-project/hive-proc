cmake_minimum_required(VERSION 3.0)
project(yoda-hive-proc CXX)
set(CMAKE_CXX_STANDARD 11)

set(HIVEPROC_INSTALL_DIR "/usr/lib/hive-proc/" CACHE PATH "hiveproc install directory")
set(NODEJS_VARIANT "shadow-node" CACHE STRING "Node.js variant to be used")
set(HIVEPROC_SOCKET "/var/run/hive.sock" CACHE STRING "hiveproc ")

find_path(NODE_API_INCLUDE_DIRS
  NAMES node_api.h
  PATH_SUFFIXES ${NODEJS_VARIANT}
)
find_path(CAPS_INCLUDE_DIRS
  NAMES caps.h
  PATH_SUFFIXES caps
)
find_library(CAPS_LIBRARY
  NAMES libcaps.a)

file(GLOB HIVEPROC_SOURCE src/addon.cc src/hiveproc.cc)

add_library(hiveproc SHARED ${HIVEPROC_SOURCE})
set_target_properties(hiveproc PROPERTIES PREFIX "" SUFFIX ".node")
if(APPLE)
  set_target_properties(hiveproc PROPERTIES
                        LINK_FLAGS "-rdynamic -undefined dynamic_lookup")
else()
  set_target_properties(hiveproc PROPERTIES
                        LINK_FLAGS "-rdynamic -Wl,--unresolved-symbols=ignore-all")
endif()
target_compile_definitions(hiveproc PRIVATE
  HIVE_SOCKET="${HIVEPROC_SOCKET}"
  NODE_ADDON_API_DISABLE_DEPRECATED)
target_include_directories(hiveproc PUBLIC src PRIVATE vendor/node-addon-api ${NODE_API_INCLUDE_DIRS} ${CAPS_INCLUDE_DIRS})
target_link_libraries(hiveproc ${CAPS_LIBRARY})

add_executable(echo-server test/echo-server.cc src/hiveproc.cc)
target_include_directories(echo-server PUBLIC src PRIVATE vendor/node-addon-api ${NODE_API_INCLUDE_DIRS} ${CAPS_INCLUDE_DIRS})
target_link_libraries(echo-server ${CAPS_LIBRARY})

install(DIRECTORY etc DESTINATION /)
install(DIRECTORY lib DESTINATION ${HIVEPROC_INSTALL_DIR})
install(DIRECTORY preset DESTINATION ${HIVEPROC_INSTALL_DIR})
install(FILES package.json DESTINATION ${HIVEPROC_INSTALL_DIR})
