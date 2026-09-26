cmake_minimum_required(VERSION 3.21)

# project name
project(chess)

# creates the variable EXEC (name of my executable)
set(EXEC chess)

# set the C++20 standard
set(CMAKE_CXX_STANDARD 20)

# A bare `cmake` with no preset matches the release preset.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Debug or Release")
endif()

# Debug keeps assert(). Release passes -DNDEBUG, which removes them.
set(CMAKE_CXX_FLAGS "-Wall -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# export compiler_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

# add gui files
# add_library(guiLib)
# add_subdirectory(gui)

# add files in src dir
file(GLOB SOURCES "*.cpp")

# compiles the files defined by SOURCES to generante the executable defined by EXEC
add_executable(${EXEC} ${SOURCES} )#${GUI_SOURCES})

# lib dependencies for SFML
#target_link_libraries(${EXEC} -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio)

# link gui library
#target_link_libraries(${EXEC} guiLib)
