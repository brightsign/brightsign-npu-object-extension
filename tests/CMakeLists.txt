# Test CMakeLists.txt for selective class detection feature
cmake_minimum_required(VERSION 3.4.1)

project(yolo_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenCV REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system)

# Include directories
include_directories(../include)
include_directories(../include/3rdparty)

# Add test executable
add_executable(test_class_parsing
    test_class_parsing.cpp
    ../src/utils.cc
    mock_registry.cpp
)

# Link libraries
target_link_libraries(test_class_parsing
    ${OpenCV_LIBS}
    ${Boost_LIBRARIES}
)

# Add test for integration
add_executable(test_integration
    test_integration.cpp
    ../src/utils.cc
    ../src/inference.cpp
    ../src/frame_writer.cpp
    ../src/publisher.cpp
    ../src/transports/file_transport.cpp
    ../src/transports/udp_transport.cpp
    ../src/image_utils.c
    ../src/file_utils.c
    ../src/postprocess.cc
    ../src/yolo.cc
    mock_registry.cpp
)

target_link_libraries(test_integration
    ${OpenCV_LIBS}
    ${Boost_LIBRARIES}
    ${CMAKE_SOURCE_DIR}/include/librknnrt.so
    turbojpeg
)

# Enable testing
enable_testing()

# Add tests
add_test(NAME ClassParsingTest COMMAND test_class_parsing)
add_test(NAME IntegrationTest COMMAND test_integration)