# add files in gui dir
file(GLOB SOURCES "*.cpp")

# add src to the gui lib
target_sources(guiLib PUBLIC ${SOURCES})
target_include_directories(guiLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})