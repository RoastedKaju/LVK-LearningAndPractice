file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

add_executable("00-Setup" ${SOURCES})

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    PREFIX ""
    FILES ${SOURCES}
)

target_link_libraries("00-Setup" PRIVATE glfw)
target_link_libraries("00-Setup" PRIVATE LVKLibrary)
target_link_libraries("00-Setup" PRIVATE LVKstb)
target_link_libraries("00-Setup" PRIVATE ktx)

target_include_directories("00-Setup" PUBLIC ${CMAKE_SOURCE_DIR}/src/Shared)

if(WIN32)
  target_compile_definitions("00-Setup" PUBLIC "NOMINMAX")
endif()
