file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

set(ChapterName "02-Model")

add_executable(${ChapterName} ${SOURCES})

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}
    PREFIX ""
    FILES ${SOURCES}
)

target_link_libraries(${ChapterName} PRIVATE glfw)
target_link_libraries(${ChapterName} PRIVATE LVKLibrary)
target_link_libraries(${ChapterName} PRIVATE LVKstb)
target_link_libraries(${ChapterName} PRIVATE ktx)
target_link_libraries(${ChapterName} PRIVATE assimp)

target_include_directories(${ChapterName} PUBLIC ${CMAKE_SOURCE_DIR}/src/Shared)

if(WIN32)
  target_compile_definitions(${ChapterName} PUBLIC "NOMINMAX")
endif()