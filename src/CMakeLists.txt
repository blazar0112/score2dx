set(CMAKE_CXX_EXTENSIONS OFF)
set(COMMON_MSVC_FLAGS "/permissive-;/W4;/WX;/Zi;/Zc:__cplusplus;/Zc:preprocessor;/wd5105;/DFMT_HEADER_ONLY")
set(COMMON_GCC_FLAGS "-pedantic;-Wall;-Wextra;-Werror;-g;-fPIC;-DFMT_HEADER_ONLY")

if (MSVC)
    set(COMMON_FLAGS ${COMMON_MSVC_FLAGS})
else()
    set(COMMON_FLAGS ${COMMON_GCC_FLAGS})
endif()

if (UNIX)
    # UNIX use following env vars set by script outside.
    # gtest: GTEST_ROOT
endif()
if (WIN32)
    if (MSVC)
        set(ENV{GTEST_ROOT})
    endif()
    if (MINGW)
        set(ENV{GTEST_ROOT} $ENV{INSTALL_ROOT}/gtest/1.10.0/mingw64-10.2.0)
    endif()
endif()

find_package(GTest REQUIRED)
message(GTEST_INCLUDE_DIRS = ${GTEST_INCLUDE_DIRS})

set(ENV{icl_s2_DIR} $ENV{INSTALL_ROOT}/icl_s2/2.0.0/mingw64-10.2.0)
find_package(icl_s2 REQUIRED)
message(ICL_S2_INCLUDE_DIRS=${ICL_S2_INCLUDE_DIRS})

# Header only libraries
set(FMT_INCLUDE_DIRS $ENV{FMT_ROOT}/include)
set(JSON_INCLUDE_DIRS $ENV{JSON_ROOT}/include)

set(MINGW_LIBRARY_NAMES
    gcc_s_seh-1 stdc++-6 winpthread-1
    curl-4 brotlicommon brotlidec crypto-1_1-x64 idn2-0 nghttp2-14 psl-5 ssh2-1 ssl-1_1-x64 zstd iconv-2
    intl-8 unistring-2
)
set(MINGW_LIBRARIES)
foreach(LIB ${MINGW_LIBRARY_NAMES})
    set(MINGW_LIB_PATH $ENV{MINGW64_ROOT}/bin/lib${LIB}.dll)
    if (EXISTS ${MINGW_LIB_PATH} AND NOT IS_DIRECTORY ${MINGW_LIB_PATH})
        file(TO_CMAKE_PATH ${MINGW_LIB_PATH} FOUND_MINGW_LIB)
        list(APPEND MINGW_LIBRARIES ${FOUND_MINGW_LIB})
        message("Found MINGW Lib: ${FOUND_MINGW_LIB}")
    endif()
endforeach(LIB)

list(APPEND MINGW_LIBRARIES $ENV{MINGW64_ROOT}/bin/zlib1.dll)

set_property(GLOBAL PROPERTY PROP_HEADERS)
set_property(GLOBAL PROPERTY PROP_SOURCES)
set_property(GLOBAL PROPERTY PROP_PUBLIC_HEADERS)
set_property(GLOBAL PROPERTY PROP_TEST_SOURCES)

add_subdirectory(score2dx)

get_property(HEADERS GLOBAL PROPERTY PROP_HEADERS)
get_property(SOURCES GLOBAL PROPERTY PROP_SOURCES)
get_property(PUBLIC_HEADERS GLOBAL PROPERTY PROP_PUBLIC_HEADERS)
get_property(TEST_SOURCES GLOBAL PROPERTY PROP_TEST_SOURCES)

add_library(score2dx SHARED ${PUBLIC_HEADERS} ${HEADERS} ${SOURCES})
if (NOT MSVC)
    set_target_properties(score2dx PROPERTIES COMPILE_FLAGS "-m64" LINK_FLAGS "-m64")
endif()
set_target_properties(score2dx PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_include_directories(score2dx SYSTEM PUBLIC
    ${FMT_INCLUDE_DIRS}
    ${JSON_INCLUDE_DIRS}
)
target_include_directories(score2dx PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_BINARY_DIR}/exports>"
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}>
)
set_target_properties(score2dx PROPERTIES CXX_STANDARD 20)
if (MSVC)
    target_compile_options(score2dx PRIVATE ${COMMON_MSVC_FLAGS}
        /Dscore2dx_EXPORTS
        $<$<CONFIG:DEBUG>:/Od>
        $<$<CONFIG:RELEASE>:/O2>
    )
else()
    target_compile_options(score2dx PRIVATE ${COMMON_GCC_FLAGS}
        -Dscore2dx_EXPORTS
        $<$<CONFIG:DEBUG>:-O0 -time>
        $<$<CONFIG:RELEASE>:-O2>
    )
endif()
target_link_libraries(score2dx
    stdc++fs
    icl_s2::icl_s2
    curl
)

add_custom_command(TARGET score2dx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MINGW_LIBRARIES} $<TARGET_FILE:icl_s2::icl_s2> ${GTEST_LIBRARY}
        $<TARGET_FILE_DIR:score2dx>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/table
        $<TARGET_FILE_DIR:score2dx>/table
)

add_executable(experiment app/experiment.cpp)
set_target_properties(experiment PROPERTIES CXX_STANDARD 20)
target_compile_options(experiment PUBLIC ${COMMON_FLAGS})
target_link_libraries(experiment score2dx)

add_executable(interactive app/interactive.cpp)
set_target_properties(interactive PROPERTIES CXX_STANDARD 20)
target_compile_options(interactive PUBLIC ${COMMON_FLAGS})
target_link_libraries(interactive score2dx)

add_executable(test-all app/test-all.cpp ${TEST_SOURCES})
target_include_directories(test-all PUBLIC ${FMT_INCLUDE_DIRS} ${GTEST_INCLUDE_DIRS})
set_target_properties(test-all PROPERTIES CXX_STANDARD 20)
target_compile_options(test-all PUBLIC ${COMMON_FLAGS})
target_link_libraries(test-all GTest::gtest score2dx)

add_executable(iidxme app/iidxme.cpp)
set_target_properties(iidxme PROPERTIES CXX_STANDARD 20)
target_compile_options(iidxme PUBLIC ${COMMON_FLAGS})
target_link_libraries(iidxme score2dx)

set(MINGW_LIBRARY_NAMES gcc_s_seh-1 stdc++-6 winpthread-1)
set(MINGW_LIBRARIES)
foreach (LIB ${MINGW_LIBRARY_NAMES})
    set(MINGW_LIB_PATH $ENV{MINGW64_ROOT}/bin/lib${LIB}.dll)
    if (EXISTS ${MINGW_LIB_PATH} AND NOT IS_DIRECTORY ${MINGW_LIB_PATH})
        file(TO_CMAKE_PATH ${MINGW_LIB_PATH} FOUND_MINGW_LIB)
        list(APPEND MINGW_LIBRARIES ${FOUND_MINGW_LIB})
    endif()
endforeach()

if (MINGW)
    add_custom_command(TARGET test-all POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MINGW_LIBRARIES} ${GTEST_LIBRARY}
        $ENV{MINGW64_ROOT}/ssl/certs/ca-bundle.crt
        $<TARGET_FILE_DIR:test-all>
    )
endif()

# Install
include(GenerateExportHeader)
generate_export_header(score2dx EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/score2dx/score2dx_export.h)
install(FILES ${CMAKE_BINARY_DIR}/exports/score2dx/score2dx_export.h DESTINATION include/score2dx)

message(CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX})
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(
    TARGETS score2dx
    EXPORT score2dx-targets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
set(CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/score2dx)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/score2dx-config.cmake.in
    score2dx-config.cmake
    INSTALL_DESTINATION ${CONFIG_INSTALL_DIR}
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

write_basic_package_version_file(
    score2dx-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(
    EXPORT score2dx-targets
    FILE score2dx-targets.cmake
    NAMESPACE score2dx::
    DESTINATION ${CONFIG_INSTALL_DIR}
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/score2dx-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/score2dx-config-version.cmake
    DESTINATION ${CONFIG_INSTALL_DIR}
)

install(DIRECTORY
    ${PROJECT_SOURCE_DIR}/table/
    DESTINATION table
)

foreach(file ${PUBLIC_HEADERS})
    string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR} include header ${file})
    get_filename_component(dir ${header} DIRECTORY)
    install(FILES ${file} DESTINATION ${dir})
endforeach()

add_test(test test-all)
