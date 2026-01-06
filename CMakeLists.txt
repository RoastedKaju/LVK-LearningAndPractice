cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project("Vulkan-Practice" LANGUAGES C CXX)

# Add LVK
add_subdirectory("external/lvk")
add_subdirectory("external/assimp")

# Add Chapters
add_subdirectory("src/00-Setup")
add_subdirectory("src/01-Triangle")
add_subdirectory("src/02-Model")

