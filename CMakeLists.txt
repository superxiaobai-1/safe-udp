cmake_minimum_required(VERSION 3.1)

project(safe-udp)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g")

set(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/lib")


add_subdirectory(udp_transport)
add_subdirectory(test)
