file(GLOB_RECURSE ACTORS_SOURCES "*.cpp" "*.hpp")
add_library(actors STATIC ${ACTORS_SOURCES})
