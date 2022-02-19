cmake_minimum_required(VERSION 3.15)

# Disable in-source build.
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source build is not allowed. Remember to remove CMakeCache.txt and CMakeFiles/")
endif()

# Read project version
set(PROJ_VERSION_REGEX "#define SCORE_2DX_VERSION_.*[ \t]+(.+)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/score2dx/version.hpp" PROJ_VERSION REGEX ${PROJ_VERSION_REGEX})
list(TRANSFORM PROJ_VERSION REPLACE ${PROJ_VERSION_REGEX} "\\1")
string(JOIN "." PROJ_VERSION ${PROJ_VERSION})

project(Score2dx
    VERSION ${PROJ_VERSION}
    DESCRIPTION "IIDX CSV score library."
    LANGUAGES CXX
)
message(STATUS "Project [${PROJECT_NAME}] v${PROJECT_VERSION} (${CMAKE_BUILD_TYPE})")

add_subdirectory(src)
