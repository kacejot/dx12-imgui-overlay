cmake_minimum_required(VERSION 3.18)

project(s2-overlay)

add_subdirectory(deps/pluginloader)
add_subdirectory(deps/s2-hooking)
add_subdirectory(deps/imgui)

file(GLOB sources
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

add_library(${PROJECT_NAME} STATIC ${sources})

target_link_libraries(${PROJECT_NAME} PUBLIC
    minhook
    imgui
    s2-hooking
)

target_compile_features(${PROJECT_NAME} PUBLIC
    cxx_std_20
)

pluginloader_add(winmm STANDALONE_ROLE)

target_link_libraries(pluginloader_winmm PUBLIC
    ${PROJECT_NAME}
)
