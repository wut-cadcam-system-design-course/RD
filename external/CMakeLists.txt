set(OpenCASCADE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/occt-install/lib/cmake/opencascade" PARENT_SCOPE)

set(IMGUI_PATH "imgui")
file(GLOB IMGUI_SOURCES
        ${IMGUI_PATH}/*.cpp
        ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
        ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_PATH}/misc/cpp/imgui_stdlib.cpp)

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${IMGUI_PATH})

set(IMGUI_FILE_DIALOG_PATH "ImGuiFileDialog")
file(GLOB IMGUI_FILE_DIALOG_PATH_SOURCES ${IMGUI_FILE_DIALOG_PATH}/ImGuiFileDialog.cpp)
add_library(ImGuiFileDialog STATIC ${IMGUI_FILE_DIALOG_PATH_SOURCES})
target_include_directories(ImGuiFileDialog PUBLIC ${IMGUI_FILE_DIALOG_PATH})
target_link_libraries(ImGuiFileDialog PUBLIC imgui)
