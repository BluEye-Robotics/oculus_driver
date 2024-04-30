cmake_minimum_required(VERSION 3.16)
project(oculus_driver_tests VERSION 0.1)

list(APPEND test_files
    src/statusraw_test.cpp
    src/status_display.cpp
    src/client_test.cpp
    src/async_client_test.cpp
    src/config_test.cpp
    src/recorder_test.cpp
    src/filereader_test.cpp
    src/helpers_test.cpp
)

message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "test_files: ${test_files}")
message(STATUS "EXT_LIBS: ${EXT_LIBS}")
message(STATUS "EXT_LIB_DIRS: ${EXT_LIB_DIRS}")

foreach(filename ${test_files})
    get_filename_component(test_name ${filename} NAME_WE)
    set(target_name ${test_name}_${PROJECT_NAME})
    message(STATUS "Adding test ${target_name} with file ${filename}")

    add_executable(${target_name} ${filename})
    target_include_directories(${target_name} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${target_name} oculus_driver ${EXT_LIBS} )
    target_link_directories(${target_name} PUBLIC ${EXT_LIB_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
    target_compile_features(${target_name} PUBLIC cxx_std_17)
    target_compile_definitions(${target_name} PUBLIC MAGIC_ENUM_RANGE_MAX=1024)
endforeach()
