cmake_minimum_required(VERSION 3.10.0)
project(deci-detect VERSION 1.0.0 LANGUAGES C CXX)

add_executable(deci-detect main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
