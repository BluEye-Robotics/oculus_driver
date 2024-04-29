cmake_minimum_required(VERSION 3.16)
project(oculus_driver VERSION 2.0.0)

option(BUILD_TESTS "Build unit tests" OFF)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DefaultBuildType.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Boost COMPONENTS system thread REQUIRED)
pkg_check_modules(FMT fmt)
find_package(spdlog REQUIRED)
find_package(eventpp CONFIG REQUIRED)
find_package(magic_enum REQUIRED)

add_definitions(-DSPDLOG_COMPILED_LIB)
add_definitions(-DSPDLOG_FMT_EXTERNAL=on)

add_library(oculus_driver SHARED
    src/print_utils.cpp
    src/StatusListener.cpp
    src/SonarClient.cpp
    src/SonarDriver.cpp
)
set_target_properties(oculus_driver PROPERTIES
    PUBLIC_HEADER "${oculus_driver_headers}"
    INSTALL_RPATH_USE_LINK_PATH True
)
target_include_directories(oculus_driver PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${EVENTPP_INCLUDE_DIR}
    ${magic_enum_INCLUDE_DIRS}
)
target_link_libraries(oculus_driver PUBLIC
    Boost::system
    Boost::thread
    ${SPDLOG_LIBRARIES}
    ${FMT_LIBRARIES}
)
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
