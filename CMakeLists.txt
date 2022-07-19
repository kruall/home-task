cmake_minimum_required(VERSION 3.1)

project(hometask)
set(CMAKE_CXX_STANDARD 20)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_BUILD_TYPE "Release" CACHE INTERNAL "" FORCE)

set(BUILD_SHARED_LIBS OFF)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

find_package(Threads REQUIRED)

include_directories(SYSTEM "./src")

add_subdirectory("src")
