cmake_minimum_required(VERSION 3.16)
project(oculus_driver VERSION 2.0.0)

option(BUILD_TESTS "Build unit tests" OFF)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DefaultBuildType.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Boost COMPONENTS system thread REQUIRED)
pkg_check_modules(FMT fmt)
message(STATUS "FMT_VERSION: ${FMT_VERSION}")
message(STATUS "FMT_LIBRARY_DIRS = ${FMT_LIBRARY_DIRS}")
message(STATUS "FMT_LIBRARIES = ${FMT_LIBRARIES}")

find_package(spdlog REQUIRED)
set(SPDLOG_LIB spdlog::spdlog)
if("${SPDLOG_LIBRARIES}" STREQUAL "")
  set(SPDLOG_LIBRARIES spdlog::spdlog)
endif()

find_package(eventpp CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
if("${magic_enum_LIBRARIES}" STREQUAL "")
  set(magic_enum_LIBRARIES magic_enum::magic_enum)
endif()


add_definitions(-DSPDLOG_COMPILED_LIB)
add_definitions(-DSPDLOG_FMT_EXTERNAL=1)

message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")

set(EXT_LIBS
    Boost::system
    Boost::thread
    ${magic_enum_LIBRARIES}
    ${SPDLOG_LIBRARIES}
    ${FMT_LIBRARIES}
)

set(EXT_LIB_DIRS
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${EVENTPP_INCLUDE_DIR}
    ${magic_enum_INCLUDE_DIRS}
    ${FMT_LIBRARY_DIRS}
)
message(STATUS "EXT_LIB_DIRS: ${EXT_LIB_DIRS}")
message(STATUS "EXT_LIBS: ${EXT_LIBS}")

add_library(oculus_driver SHARED
    src/AsyncService.cpp
    src/print_utils.cpp
    src/Recorder.cpp
    src/SonarClient.cpp
    src/SonarDriver.cpp
    src/StatusListener.cpp
)
set_target_properties(oculus_driver PROPERTIES
    PUBLIC_HEADER "${oculus_driver_headers}"
    INSTALL_RPATH_USE_LINK_PATH True
)
target_include_directories(oculus_driver PUBLIC ${EXT_LIB_DIRS})
target_link_directories(oculus_driver PUBLIC ${EXT_LIB_DIRS})
target_link_libraries(oculus_driver PUBLIC ${EXT_LIBS})
target_compile_features(oculus_driver PUBLIC cxx_std_17)
target_compile_definitions(oculus_driver PUBLIC MAGIC_ENUM_RANGE_MAX=1024)

#############
## Install ##
#############
include(GNUInstallDirs)

install(DIRECTORY include/oculus_driver
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        EXPORT ${PROJECT_NAME}Targets)

# install the exported targets
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    # NAMESPACE ${PROJECT_NAME}:: # namespacing does not play well with fetch content
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

############
## Config ##
############
include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${CMAKE_PROJECT_VERSION}"
    COMPATIBILITY AnyNewerVersion
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)
export(EXPORT ${PROJECT_NAME}Targets
    # NAMESPACE ${PROJECT_NAME}:: # namespacing does not play well with fetch content
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
