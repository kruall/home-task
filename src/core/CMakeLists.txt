file(GLOB_RECURSE CORE_SOURCES "*.cpp" "*.hpp")
add_library(core STATIC ${CORE_SOURCES})