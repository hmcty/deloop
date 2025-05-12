add_executable(test_wm8960 cpp/test_wm8960.cpp cpp/utils.cpp)
target_link_libraries(test_wm8960
PRIVATE
  fff
  GTest::gtest_main
  wm8960_stm32f4
  stm32f4xx_hal_interface
)
target_include_directories(test_wm8960
PRIVATE
  ${CMAKE_SOURCE_DIR}/src/config
)

include(GoogleTest)
gtest_discover_tests(test_wm8960)
