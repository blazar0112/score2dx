set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# warning C4996: 'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
#   it maybe unsafe if use twice, but only use once and want C++ portable solution.
#   https://stackoverflow.com/questions/48568707/getenv-function-may-be-unsafe-really
# warning C5105: macro expansion producing 'defined' has undefined behavior
#   caused by /Zc:preprocessor will expose windows header inconform to standard.
set(COMMON_MSVC_FLAGS "/permissive-;/W4;/WX;/Zi;/Zc:__cplusplus;/Zc:preprocessor;/wd4996;/wd5105;/utf-8")
set(COMMON_GCC_FLAGS "-pedantic;-Wall;-Wextra;-Werror;-g;-fPIC")

if (MSVC)
    set(COMMON_FLAGS ${COMMON_MSVC_FLAGS})
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    set(COMMON_FLAGS ${COMMON_GCC_FLAGS})
endif()

set(TOOLCHAIN_BUILD_TYPE "gcc-11.2.0")
if (WIN32)
    if (MSVC)
        # use vcpkg instead
        unset(ENV{fmt_DIR})
        unset(ENV{nlohmann_json_DIR})
        unset(ENV{GTest_DIR})
        unset(ENV{benchmark_DIR})
        set(TOOLCHAIN_BUILD_TYPE "msvc-vs2019")
    endif()
    if (MINGW)
        set(TOOLCHAIN_BUILD_TYPE "mingw64-11.2.0")
    endif()
endif()

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
# MINGW directly use MSYS2 installed
if (MINGW)
    find_package(CURL REQUIRED)
endif()
if (MSVC)
    find_package(CURL CONFIG REQUIRED)
endif()

set(ENV{ies_DIR} $ENV{INSTALL_ROOT}/ies/latest/${TOOLCHAIN_BUILD_TYPE})
find_package(ies CONFIG REQUIRED)
file(TO_CMAKE_PATH $ENV{ies_DIR} IES_PATH)
message(STATUS "Found ies: " ${IES_PATH})

if (MINGW)
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
            message(STATUS "Found MINGW Lib: ${FOUND_MINGW_LIB}")
        endif()
    endforeach(LIB)

    list(APPEND MINGW_LIBRARIES $ENV{MINGW64_ROOT}/bin/zlib1.dll)
endif()

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
if (MINGW)
    target_link_libraries(score2dx
        stdc++fs
        ies::ies
        fmt::fmt-header-only
        nlohmann_json::nlohmann_json
        curl
    )
endif()
if (MSVC)
    target_link_libraries(score2dx
        ies::ies
        fmt::fmt-header-only
        nlohmann_json::nlohmann_json
        CURL::libcurl
    )
endif()

add_custom_command(TARGET score2dx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MINGW_LIBRARIES} $<TARGET_FILE:ies::ies>
        $ENV{MINGW64_ROOT}/ssl/certs/ca-bundle.crt
        $<TARGET_FILE_DIR:score2dx>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/table
        $<TARGET_FILE_DIR:score2dx>/table
)

add_executable(experiment app/experiment.cpp)
set_target_properties(experiment PROPERTIES CXX_STANDARD 20)
target_compile_options(experiment PUBLIC ${COMMON_FLAGS})
target_link_libraries(experiment PRIVATE score2dx fmt::fmt-header-only nlohmann_json::nlohmann_json)

add_executable(interactive app/interactive.cpp)
set_target_properties(interactive PROPERTIES CXX_STANDARD 20)
target_compile_options(interactive PUBLIC ${COMMON_FLAGS})
target_link_libraries(interactive PRIVATE score2dx fmt::fmt-header-only nlohmann_json::nlohmann_json)

add_executable(test-all app/test-all.cpp ${TEST_SOURCES})
set_target_properties(test-all PROPERTIES CXX_STANDARD 20)
target_compile_options(test-all PUBLIC ${COMMON_FLAGS})
target_link_libraries(test-all PRIVATE GTest::gtest score2dx fmt::fmt-header-only nlohmann_json::nlohmann_json)

add_executable(iidxme app/iidxme.cpp)
set_target_properties(iidxme PROPERTIES CXX_STANDARD 20)
target_compile_options(iidxme PUBLIC ${COMMON_FLAGS})
target_link_libraries(iidxme PRIVATE score2dx fmt::fmt-header-only nlohmann_json::nlohmann_json)

add_executable(regression app/regression.cpp)
set_target_properties(regression PROPERTIES CXX_STANDARD 20)
target_compile_options(regression PUBLIC ${COMMON_FLAGS})
target_link_libraries(regression PRIVATE score2dx fmt::fmt-header-only nlohmann_json::nlohmann_json)

# Install
include(GenerateExportHeader)
generate_export_header(score2dx EXPORT_FILE_NAME ${CMAKE_BINARY_DIR}/exports/score2dx/score2dx_export.h)
install(FILES ${CMAKE_BINARY_DIR}/exports/score2dx/score2dx_export.h DESTINATION include/score2dx)

message(STATUS "CMAKE_INSTALL_PREFIX: " ${CMAKE_INSTALL_PREFIX})
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
