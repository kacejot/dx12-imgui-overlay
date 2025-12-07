project(imgui)

file(GLOB sources
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/misc/cpp/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/*.h"
)	

file (GLOB backend_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_win32.*"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_dx11.*"
#    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.*"
#    "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_impl_glfw.*"
)

add_library(${PROJECT_NAME}
    ${sources}
	${backend_sources}
)

target_compile_features(${PROJECT_NAME} PUBLIC
    cxx_std_20
)

target_link_libraries(${PROJECT_NAME} PUBLIC
#    glfw
#	 opengl32
	d3d11
	d3dcompiler
	dxgi
)

target_include_directories(${PROJECT_NAME} PUBLIC 
    imgui
)
