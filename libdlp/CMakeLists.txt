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

if(BUILD_TESTS)
  add_executable(test_track tests/test_track.cpp)
  target_link_libraries(test_track
  PRIVATE
    GTest::gtest_main
    dlp
  )
  add_test(NAME test_track COMMAND test_track)

  # Add target for all tests
  add_custom_target(tests ALL DEPENDS test_track)
endif()
