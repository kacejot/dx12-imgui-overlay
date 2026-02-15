cmake_minimum_required(VERSION 3.18)

project(dx12-imgui-overlay)

# Wait till minhook updates required version
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/minhook)
set(CMAKE_WARN_DEPRECATED ON CACHE BOOL "" FORCE)

add_subdirectory(deps/imgui)

file(GLOB sources
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

add_library(${PROJECT_NAME} STATIC ${sources})

target_link_libraries(${PROJECT_NAME} PUBLIC
    minhook
    imgui
)

target_compile_features(${PROJECT_NAME} PUBLIC
    cxx_std_20
)
