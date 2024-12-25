cmake_minimum_required(VERSION 3.10)
project(Project2)
find_package( OpenCV REQUIRED )

add_executable(main src/main.cpp)
add_executable(golomb_test src/golomb_tester.cpp)
add_executable(coder src/coder.cpp)
add_executable(BitStreamTest src/BitStreamTest.cpp)

# Add include directories
include_directories(src)
include_directories(include)
include_directories( ${OpenCV_INCLUDE_DIRS} include )
include_directories(${CMAKE_SOURCE_DIR})


target_link_libraries(main ${OpenCV_LIBS} )
target_link_libraries(golomb_test ${OpenCV_LIBS} )
target_link_libraries(coder ${OpenCV_LIBS} )
target_link_libraries(BitStreamTest ${OpenCV_LIBS} )
