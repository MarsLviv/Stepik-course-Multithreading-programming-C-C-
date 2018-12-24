cmake_minimum_required(VERSION 2.8)
#message("CMake start")

project(final)

set(Boost_USE_STATIC_LIBS 	ON)
set(Boost_USE_MULTITHREADED 	ON)

#find_package(Boost 1.65.1 REQUIRED COMPONENTS system thread filesystem)
find_package(Boost REQUIRED COMPONENTS system thread filesystem)
find_package(Threads)

set(project_sources main.cpp server.cpp session.cpp Service.cpp)# Create a list with all .cpp source files
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

#target_link_libraries(final PRIVATE pthread)
#set(EXECUTABLE_OUTPUT_PATH "final")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
#sset(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpermissive")

add_executable(${PROJECT_NAME} ${project_sources})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} pthread)

#message("CMake finish")

