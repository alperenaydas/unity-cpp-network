cmake_minimum_required(VERSION 3.20)
project(PurposeNetwork)

set(CMAKE_CXX_STANDARD 17)

include_directories(Common)

set(ENET_BUILD_SHARED OFF CACHE BOOL "Build shared library" FORCE)
add_subdirectory(thirdparty/enet)
include_directories(thirdparty/enet/include)

add_subdirectory(Server)
add_subdirectory(ClientPlugin)
add_subdirectory(Client)