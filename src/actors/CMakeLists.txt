file(GLOB_RECURSE ACTORS_SOURCES "*.cpp" "*.hpp")
add_library(actors ${ACTORS_SOURCES})
target_link_libraries(actors core logic ${CMAKE_THREAD_LIBS_INIT})
