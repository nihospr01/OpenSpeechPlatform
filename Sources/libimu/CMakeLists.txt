cmake_minimum_required(VERSION 3.13)
project(libimu)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
    message(STATUS "Compiling for ARM Target")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -Wall -Wextra -Wno-reorder")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=armv8-a+crc -funsafe-math-optimizations -Wno-reorder")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -Wall -Wextra -Wno-reorder -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -Wno-unused-result -Wno-reorder")
endif()

# find_library(LIBLSL lsl)
find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
include_directories(include ${CMAKE_CURRENT_BINARY_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/)
add_executable(log_imu 
  log_imu.cpp
  imu.cpp
  imu160.cpp
  imu270.cpp
  gesture_detection.cpp 
  osp_socket.cpp
  BMI270-Sensor-API/examples/bmi270/common/common.c 
  BMI270-Sensor-API/bmi270.c 
  BMI270-Sensor-API/bmi2.c
  BMI160_driver/osp_bmi160.c
  BMI160_driver/bmi160.c
  )
# target_link_libraries(refimu ${LIBLSL})
target_link_libraries(log_imu Threads::Threads)