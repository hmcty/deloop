add_library(dlp STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/counter.c
  ${CMAKE_CURRENT_LIST_DIR}/src/engine.c
  ${CMAKE_CURRENT_LIST_DIR}/src/ringbuf.c
  ${CMAKE_CURRENT_LIST_DIR}/src/track.c
)
target_include_directories(dlp PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include
)
target_compile_options(dlp PRIVATE
  -Wall
  -Werror
)

macro(dlp_unit_test name)
  add_executable(${name} tests/${name}.cpp)
  target_link_libraries(${name}
  PRIVATE
    GTest::gtest_main
    dlp
  )
  add_test(NAME ${name} COMMAND ${name})
  add_dependencies(all_tests ${name})
endmacro()

if(BUILD_TESTS)
  add_custom_target(all_tests)
  dlp_unit_test(test_counter)
  dlp_unit_test(test_track)
endif()
