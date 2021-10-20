find_package(PkgConfig REQUIRED)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(adaptive_filter_test adaptive_filter_test.cpp)
target_include_directories(adaptive_filter_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (adaptive_filter_test PUBLIC OSP ${GTEST_LIBRARIES} pthread)


add_executable(filter_test filter_test.cpp)
target_include_directories(filter_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (filter_test PUBLIC OSP ${GTEST_LIBRARIES} pthread)

add_executable(wdrc11_test wdrc11_test.cpp)
target_include_directories(wdrc11_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (wdrc11_test PUBLIC OSP ${GTEST_LIBRARIES} matio pthread)

add_executable(elevenband_test elevenband_test.cpp)
target_include_directories(elevenband_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (elevenband_test PUBLIC OSP ${GTEST_LIBRARIES} matio pthread)

add_executable(upsample_test upsample_test.cpp)
target_include_directories(upsample_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (upsample_test PUBLIC OSP ${GTEST_LIBRARIES} matio pthread)

find_package(benchmark REQUIRED)

add_executable(array_test array_test.cpp)
target_include_directories(array_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (array_test PUBLIC OSP ${GTEST_LIBRARIES} pthread)

add_executable(array_bench array_bench.cpp)
target_include_directories(array_bench PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (array_bench PUBLIC OSP ${GTEST_LIBRARIES} pthread benchmark::benchmark)

add_executable(fir_test fir_test.cpp)
target_include_directories(fir_test PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (fir_test PUBLIC OSP ${GTEST_LIBRARIES} pthread)

add_executable(resample_bench resample_bench.cpp)
target_include_directories(resample_bench PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (resample_bench PUBLIC OSP benchmark::benchmark pthread)

add_executable(fir_bench fir_bench.cpp)
target_include_directories(fir_bench PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../
  ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
)
target_link_libraries (fir_bench PUBLIC OSP benchmark::benchmark pthread)


# Missing golden files
# add_executable(freping_test freping_test.cpp)
# target_include_directories(freping_test PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/../
#   ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
# )
# target_link_libraries (freping_test LINK_PUBLIC OSP)

# BROKEN
# add_executable(afc_test afc_test.cpp)
# target_include_directories(afc_test PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/../
#   ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
# )
# target_link_libraries (afc_test LINK_PUBLIC OSP)

# BROKEN
# add_executable(beamformer_test beamformer_test.cpp)
# target_include_directories(beamformer_test PRIVATE
#   ${CMAKE_CURRENT_SOURCE_DIR}/../
#   ${CMAKE_CURRENT_SOURCE_DIR}/../OSP/
# )
# target_link_libraries (beamformer_test LINK_PUBLIC OSP)
