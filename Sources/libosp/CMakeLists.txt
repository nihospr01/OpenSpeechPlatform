cmake_minimum_required(VERSION 3.8)
project(OSP)

set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
    message(STATUS "Compiling for ARM Target")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -Wall -Wextra -Wno-reorder")
    # set(CMAKE_CXX_FLAGS_RELEASE "--save-temps -fverbose-asm -O3 -march=armv8-a+crc -funsafe-math-optimizations")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=armv8-a+crc -funsafe-math-optimizations -Wno-reorder")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -Wall -Wextra -Wno-reorder -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls")
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -march=native -Wno-unused-result -Wno-reorder")
endif()

macro(print_all_variables)
    message(STATUS "print_all_variables------------------------------------------{")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
    message(STATUS "print_all_variables------------------------------------------}")
endmacro()

# print_all_variables()


install(PROGRAMS bin/osp_cli.py DESTINATION bin RENAME osp_cli)
install(PROGRAMS bin/osp_convert.py DESTINATION bin RENAME osp_convert)

# add_subdirectory(portaudio)

add_subdirectory (OSP)

if(CMAKE_PROJECT_NAME STREQUAL OSP AND BUILD_TESTING)
  add_subdirectory (test)
endif()

add_subdirectory (src)
