cmake_minimum_required(VERSION 2.8)
#message("CMake start")

set(Boost_USE_STATIC_LIBS 	ON)
set(Boost_USE_MULTITHREADED 	ON)

find_package(Boost 1.65.1 REQUIRED COMPONENTS system thread filesystem)
find_package(Threads)

set(project_sources main.cpp server.cpp session.cpp Service.cpp)# Create a list with all .cpp source files
#set(EXECUTABLE_OUTPUT_PATH "final")

add_executable(final ${project_sources})
target_link_libraries(final ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

#message("CMake finish")

