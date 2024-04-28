# only for cmake --version >= 3.5.1
cmake_minimum_required(VERSION 3.5.1)

# project name
project(chess)

# creates the variable EXEC (name of my executable)
set(EXEC chess)

# set the C++20 standard
set(CMAKE_CXX_STANDARD 20)

# flags
set(CMAKE_CXX_FLAGS "-Wall -O3 -march=native -g")

# add gui files
#add_library(guiLib)
#add_subdirectory(gui)

# add files in src dir
file(GLOB SOURCES "*.cpp")

# compiles the files defined by SOURCES to generante the executable defined by EXEC
add_executable(${EXEC} ${SOURCES} )#${GUI_SOURCES})

# lib dependencies for SFML
#target_link_libraries(${EXEC} -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio)

# link gui library
#target_link_libraries(${EXEC} guiLib)