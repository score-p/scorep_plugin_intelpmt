cmake_minimum_required(VERSION 2.8)

project(intelpmt_plugin)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(PLUGIN_SOURCE src/intelpmt.cpp)

add_subdirectory(lib/scorep_plugin_cxx_wrapper)
add_subdirectory(lib/libintelpmt)

add_library(${PROJECT_NAME} SHARED ${PLUGIN_SOURCE})
target_link_libraries(${PROJECT_NAME} PRIVATE  libintelpmt Scorep::scorep-plugin-cxx)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
