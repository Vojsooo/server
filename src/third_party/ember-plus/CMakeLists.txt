cmake_minimum_required(VERSION 3.16)

project(casparcg_ember_plus)

add_subdirectory(libs101)
add_subdirectory(libember)

target_include_directories(ember-static PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(ember-shared PRIVATE ${CMAKE_SOURCE_DIR})

if(MSVC)
    target_compile_options(ember-static PRIVATE /wd4267)
    target_compile_options(ember-shared PRIVATE /wd4267)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ember-static PRIVATE -Wno-error=maybe-uninitialized -Wno-maybe-uninitialized)
    target_compile_options(ember-shared PRIVATE -Wno-error=maybe-uninitialized -Wno-maybe-uninitialized)
endif()
