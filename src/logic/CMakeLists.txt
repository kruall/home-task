file(GLOB_RECURSE LOGIC_SOURCES "*.cpp" "*.hpp")
add_library(logic ${LOGIC_SOURCES})
target_link_libraries(logic core)
