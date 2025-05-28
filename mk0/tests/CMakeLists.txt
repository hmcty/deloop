# CPP TESTS
add_executable(test_wm8960 cpp/test_wm8960.cpp cpp/utils.cpp)
target_link_libraries(test_wm8960
PRIVATE
  fff
  GTest::gtest_main
  wm8960_stm32
)
add_test(NAME test_wm8960 COMMAND test_wm8960)

# PYTHON TESTS
find_package(Python3 REQUIRED)
set(PYTHON_TEST_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python)
add_test(
  NAME test_log_table
  COMMAND ${Python3_EXECUTABLE} -m unittest tests/python/test_log_table.py
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_custom_target(all_tests)
add_dependencies(all_tests test_wm8960)
